#ifndef lint
static	char sccsid[] = "@(#)ypbind.c	1.4 88/07/29 4.0NFSSRC"; /* from 1.31 88/02/07 SMI Copyr 1985 Sun Micro */
#endif

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This constructs a list of servers by domains, and keeps more-or-less up to
 * date track of those server's reachability.
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <rpc/rpc.h>
#include <rpc/svc.h>
#include <sys/dir.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypv1_prot.h>
#include <rpcsvc/ypclnt.h>

#define CACHE_DIR	"/etc/yp/binding"

/*
 * The domain struct is the data structure used by the yp binder to remember
 * mappings of domain to a server.  The list of domains is pointed to by
 * known_domains.  Domains are added when the yp binder gets binding requests
 * for domains which are not currently on the list.  Once on the list, an
 * entry stays on the list forever.  Bindings are initially made by means of
 * a broadcast method, using functions ypbind_broadcast_bind and
 * ypbind_broadcast_ack.  This means of binding is re-done any time the domain
 * becomes unbound, which happens when a server doesn't respond to a ping.
 * current_domain is used to communicate among the various functions in this
 * module; it is set by ypbind_get_binding.
 *  
 */
struct domain {
	struct domain *dom_pnext;
	char dom_name[MAXNAMLEN + 1];
	unsigned short dom_vers;	/* YPVERS or YPOLDVERS */
	bool dom_boundp;
	CLIENT *ping_clnt;
	struct in_addr dom_serv_addr;
	unsigned short int dom_serv_port;
	int dom_report_success;		/* Controls msg to /dev/console*/
	int dom_broadcaster_pid;
	int bindfile;			/* File with binding info in it */
};
static int ping_sock = RPC_ANYSOCK;
struct domain *known_domains = (struct domain *) NULL;
struct domain *current_domain;		/* Used by ypbind_broadcast_ack, set
					 *   by all callers of clnt_broadcast */
struct domain *broadcast_domain;	/* Set by ypbind_get_binding, used
					 *   by the mainline. */
SVCXPRT *tcphandle;
SVCXPRT *udphandle;

#define BINDING_TRIES 1			/* Number of times we'll broadcast to
					 *   try to bind default domain.  */
#define PINGTOTTIM 20			/* Total seconds for ping timeout */
#define PINGINTERTRY 10
#define SETDOMINTERTRY 20
#define SETDOMTOTTIM 60
#ifdef VERBOSE
int silent = FALSE;
#else
int silent = TRUE;
#endif

static int secure = FALSE;  /* running c2 secure; associated with the -s flag */

extern int errno;

void dispatch();
void ypbind_dispatch();
void ypbind_olddispatch();
void ypbind_get_binding();
void ypbind_set_binding();
void ypbind_send_setdom();
struct domain *ypbind_point_to_domain();
bool ypbind_broadcast_ack();
bool ypbind_ping();
void ypbind_init_default();
void broadcast_proc_exit();

extern bool xdr_ypdomain_wrap_string();
extern bool xdr_ypbind_resp();

main(argc, argv)
	int argc;
	char **argv;
{
	int pid;
	int t;
	int readfds;
	char *pname;
	bool true;

	(void) pmap_unset(YPBINDPROG, YPBINDVERS);
	(void) pmap_unset(YPBINDPROG, YPBINDOLDVERS);
	/*
	 * Scrap the initial binding processing for now, and see if we like
	 * fast startup better than initial bindings.
	 */
#ifdef INIT_DEFAULT
	 ypbind_init_default();
#endif

	/*
	 * Check to see if we are running "secure" which means that we should
	 * require that the server be using a reserved port.  We are running
	 * secure if the -s option is specified
	 */
	argc--;
	argv++;
	while (argc > 0) {
		if (!strcmp(*argv,"-s")) {
			secure = TRUE;
			fprintf(stderr, "running secure\n");
		}
		else {
			fprintf(stderr, "usage: ypbind [ -s ]\n");
			exit(1);
		}
		argc--,
		argv++;
	}

	if (silent) {
		
		pid = fork();
		
		if (pid == -1) {
			(void) fprintf(stderr, "ypbind:  fork failure.\n");
			(void) fflush(stderr);
			abort();
		}
	
		if (pid != 0) {
			exit(0);
		}
	
		for (t = 0; t < 20; t++) {
			(void) close(t);
		}
	
 		(void) open("/dev/console", 2);
 		(void) dup2(0, 1);
 		(void) dup2(0, 2);
 		t = open("/dev/tty", 2);
	
 		if (t >= 0) {
 			(void) ioctl(t, TIOCNOTTY, (char *)0);
 			(void) close(t);
 		}
	}

	if ((int) signal(SIGCHLD, broadcast_proc_exit) == -1) {
		(void) fprintf(stderr,
		    "ypbind:  Can't catch broadcast process exit signal.\n");
		(void) fflush(stderr);
		abort();
	}
        /* Open a socket for pinging everyone can use */
        ping_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (ping_sock < 0) {
                (void) fprintf(stderr,
                  "ypbind: Cannot create socket for pinging.\n");
                (void) fflush(stderr);
                abort();
        }

	/* 
	 * if not running c2 secure, don't use priviledged ports.
	 * Accomplished by side effect of not being root when creating
	 * rpc based sockets.
	 */
	if (! secure) {
		(void) setreuid(-1, 3);
	}

	if ((tcphandle = svctcp_create(RPC_ANYSOCK,
	    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == NULL) {
		(void) fprintf(stderr, "ypbind:  can't create tcp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if (!svc_register(tcphandle, YPBINDPROG, YPBINDVERS,
	    ypbind_dispatch, IPPROTO_TCP) ) {
		(void) fprintf(stderr,
		    "ypbind:  can't register tcp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if ((udphandle = svcudp_bufcreate(RPC_ANYSOCK,
	    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == (SVCXPRT *) NULL) {
		(void) fprintf(stderr, "ypbind:  can't create udp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if (!svc_register(udphandle, YPBINDPROG, YPBINDVERS,
	    ypbind_dispatch, IPPROTO_UDP) ) {
		(void) fprintf(stderr,
		    "ypbind:  can't register udp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if (!svc_register(tcphandle, YPBINDPROG, YPBINDOLDVERS,
	    ypbind_olddispatch, IPPROTO_TCP) ) {
		(void) fprintf(stderr,
		    "ypbind:  can't register tcp service.\n");
		(void) fflush(stderr);
		abort();
	}

	if (!svc_register(udphandle, YPBINDPROG, YPBINDOLDVERS,
	    ypbind_olddispatch, IPPROTO_UDP) ) {
		(void) fprintf(stderr,
		    "ypbind:  can't register udp service.\n");
		(void) fflush(stderr);
		abort();
	}
	/* undo the gross hack associated with c2 security */
	(void) setreuid(-1, 0);

	for (;;) {
		readfds = svc_fds;
		errno = 0;

		switch ( (int) select(32, &readfds, NULL, NULL, NULL) ) {

		case -1:  {
		
			if (errno != EINTR) {
			    (void) fprintf (stderr,
			    "ypbind: bad fds bits in main loop select mask.\n");
			}

			break;
		}

		case 0:  {
			(void) fprintf (stderr,
			    "ypbind:  invalid timeout in main loop select.\n");
			break;
		}

		default:  {
			svc_getreq (readfds);
			break;
		}
		
		}
	}
}

/*
 * ypbind_dispatch and ypbind_olddispatch are wrappers for dispatch which
 * remember which protocol the requestor is looking for.  The theory is,
 * that since YPVERS and YPBINDVERS are defined in the same header file, if
 * a request comes in on the old binder protocol, the requestor is looking
 * for the old yp server.
 */
void
ypbind_dispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	dispatch(rqstp, transp, (unsigned short) YPVERS);
}

void
ypbind_olddispatch(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	dispatch(rqstp, transp, (unsigned short) YPOLDVERS);
}

/*
 * This dispatches to binder action routines.
 */
void
dispatch(rqstp, transp, vers)
	struct svc_req *rqstp;
	SVCXPRT *transp;
	unsigned short vers;
{

	switch (rqstp->rq_proc) {

	case YPBINDPROC_NULL:

		if (!svc_sendreply(transp, xdr_void, 0) ) {
			(void) fprintf(stderr,
			    "ypbind:  Can't reply to rpc call.\n");
		}

		break;

	case YPBINDPROC_DOMAIN:
		ypbind_get_binding(rqstp, transp, vers);
		break;

	case YPBINDPROC_SETDOM:
		ypbind_set_binding(rqstp, transp, vers);
		break;

	default:
		svcerr_noproc(transp);
		break;

	}
}

/*
 * This is a Unix SIGCHILD handler which notices when a broadcaster child
 * process has exited, and retrieves the exit status.  The broadcaster pid
 * is set to 0.  If the broadcaster succeeded, dom_report_success will be
 * be set to -1.
 */

void
broadcast_proc_exit()
{
	int pid;
	union wait wait_status;
	register struct domain *pdom;

	pid = 0;

	for (;;) {
		pid = wait3(&wait_status, WNOHANG, NULL);

		if (pid == 0) {
			return;
		} else if (pid == -1) {
			return;
		}
		
		for (pdom = known_domains; pdom != (struct domain *)NULL;
		    pdom = pdom->dom_pnext) {
			    
			if (pdom->dom_broadcaster_pid == pid) {
				pdom->dom_broadcaster_pid = 0;

				if ((wait_status.w_termsig == 0) &&
				    (wait_status.w_retcode == 0))
					pdom->dom_report_success = -1;

			}
		}
	}

}

/*
 * This returns the current binding for a passed domain.
 */
void
ypbind_get_binding(rqstp, transp, vers)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
	unsigned short vers;
{
	char domain_name[YPMAXDOMAIN + 1];
	char *pdomain_name = domain_name;
	char *pname;
	struct stat	buf;
	struct ypbind_resp response;
	bool newbinding;
	char outstring[YPMAXDOMAIN + 256];
	int broadcaster_pid;
	struct domain *v1binding;

	if (!svc_getargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		svcerr_decode(transp);
		return;
	}

	if ( (current_domain = ypbind_point_to_domain(pdomain_name, vers) ) !=
	    (struct domain *) NULL) {

		/*
		 * Ping the server to make sure it is up.
		 */
		 
		if (current_domain->dom_boundp) {
			newbinding = ypbind_ping(current_domain);
		}

		/*
		 * Bound or not, return the current state of the binding.
		 */

		if (current_domain->dom_boundp) {
			response.ypbind_status = YPBIND_SUCC_VAL;
			response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr =
			    current_domain->dom_serv_addr;
			response.ypbind_respbody.ypbind_bindinfo.ypbind_binding_port = 
			    current_domain->dom_serv_port;
		} else {
			response.ypbind_status = YPBIND_FAIL_VAL;
			response.ypbind_respbody.ypbind_error =
			    YPBIND_ERR_NOSERV;
		}
		    
	} else {
		response.ypbind_status = YPBIND_FAIL_VAL;
		response.ypbind_respbody.ypbind_error = YPBIND_ERR_RESC;
	}

#ifndef NO_BINDING_FILE
	/* If this is a new binding, update the binding file */
	if (newbinding) {
		/* 
		 * This is pretty much a gross hack to speed up binding 
		 * operations using the yp_bind library call. By saving 
		 * this info in a file we save an rpc call and a server 
	  	 * 'ping'
		 */
tryagain:
		/* If no file is open then try to open it. */
		if (current_domain->bindfile == -1) {
			/* Generate the filename required ... */
			sprintf(outstring, "%s/%s.%d", CACHE_DIR,
				current_domain->dom_name, 
				current_domain->dom_vers);
			current_domain->bindfile = 
				open(outstring, O_RDWR+O_CREAT, 0644);
			/* XXX remove when the new libc routines are ready */
			if (current_domain->bindfile != -1)
				flock(current_domain->bindfile, LOCK_EX);
		}
		/* Write the binding information to it ... */
		if (current_domain->bindfile != -1) {
			lseek(current_domain->bindfile, 0L, L_SET);
			write(current_domain->bindfile, &(udphandle->xp_port),
				sizeof(u_short));
			write(current_domain->bindfile, &response,
				sizeof(response));
		} else {
			/* 
			 * If it failed to open, check to see that the 
			 * directory exists, if it does not exist, create 
			 * it and try again. If it does exist then abort
		 	 * and don't bother with the cache file. 
			 */
			if (stat(CACHE_DIR,&buf) < 0) {
				if (errno == ENOENT) mkdir("/var/yp", 0655);
				mkdir(CACHE_DIR, 0655);
				goto tryagain;
			}
		}
	}
#endif
        
	if (!svc_sendreply(transp, xdr_ypbind_resp, &response) ) {
		(void) fprintf(stderr,
		    "ypbind:  Can't respond to rpc request.\n");
	}

#if	NeXT
	/*
	 * Don't free things on the stack!
	 */
#else	NeXT
	if (!svc_freeargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		(void) fprintf(stderr,
		    "ypbind:  ypbind_get_binding can't free args.\n");
	}
#endif	NeXT

	if ((current_domain) && (!current_domain->dom_boundp) &&
	    (!current_domain->dom_broadcaster_pid)) {
		/*
		 * The current domain is unbound, and there is no broadcaster 
		 * process active now.  Fork off a child who will yell out on 
		 * the net.  Because of the flavor of request we're making of 
		 * the server, we only expect positive ("I do serve this
		 * domain") responses.
		 */
		broadcast_domain = current_domain;
		broadcast_domain->dom_report_success++;
		pname = current_domain->dom_name;
				
		if ( (broadcaster_pid = fork() ) == 0) {
			int	true = 1;

			(void) clnt_broadcast(YPPROG, vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			    
			if (current_domain->dom_boundp) {
				
				/*
				 * Send out a set domain request to our parent
				 */
				ypbind_send_setdom(pname,
				    current_domain->dom_serv_addr,
				    current_domain->dom_serv_port, vers);
				    
				if (current_domain->dom_report_success > 0) {
					(void) sprintf(outstring,
					 "yp: server for domain \"%s\" OK",
					    pname);
					writeit(outstring);
				}
					
				exit(0);
			} else {
				/*
				 * Hack time.  If we're looking for a current-
				 * version server and can't find one, but we
				 * do have a previous-version server bound, then
				 * suppress the console message.
				 */
				if (vers == YPVERS && ((v1binding =
				   ypbind_point_to_domain(pname, YPOLDVERS) ) !=
				    (struct domain *) NULL) &&
				    v1binding->dom_boundp) {
					    exit(1);
				}
				
				(void) sprintf(outstring,
	      "yp: server not responding for domain \"%s\"; still trying",
				    pname);
				writeit(outstring);
				exit(1);
			}

		} else if (broadcaster_pid == -1) {
			(void) fprintf(stderr,
			    "ypbind:  broadcaster fork failure.\n");
		} else {
			current_domain->dom_broadcaster_pid = broadcaster_pid;
		}
	}
}

static int
writeit(s)
	char *s;
{
	FILE *f;

	if ((f = fopen("/dev/console", "w")) != NULL) {
		(void) fprintf(f, "%s.\n", s);
		(void) fclose(f);
	}
	
}

/*
 * This sends a (current version) ypbind "Set domain" message back to our
 * parent.  The version embedded in the protocol message is that which is passed
 * to us as a parameter.
 */
void
ypbind_send_setdom(dom, addr, port, vers)
	char *dom;
	struct in_addr addr;
	unsigned short int port;
	unsigned short int vers;
{
	struct ypbind_setdom req;
	struct sockaddr_in myaddr;
	int socket;
	struct timeval timeout;
	struct timeval intertry;
	CLIENT *client;

	strcpy(req.ypsetdom_domain, dom);
	req.ypsetdom_addr = addr;
	req.ypsetdom_port = port;
	req.ypsetdom_vers = vers;
	get_myaddress(&myaddr);
	myaddr.sin_port = ntohs(udphandle->xp_port);
	socket = RPC_ANYSOCK;
	timeout.tv_sec = SETDOMTOTTIM;
	intertry.tv_sec = SETDOMINTERTRY;
	timeout.tv_usec = intertry.tv_usec = 0;

	if ((client = clntudp_bufcreate (&myaddr, YPBINDPROG, YPBINDVERS,
	    intertry, &socket, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE) ) != NULL) {

		client->cl_auth = authunix_create_default();
		clnt_call(client, YPBINDPROC_SETDOM, xdr_ypbind_setdom,
		    &req, xdr_void, 0, timeout);
		auth_destroy(client->cl_auth);
		clnt_destroy(client);
		close(socket);
	} else {
		clnt_pcreateerror(
		    "ypbind(ypbind_send_setdom): clntudp_create error");
	}
}

/*
 * This sets the internet address and port for the passed domain to the
 * passed values, and marks the domain as supported.  This accepts both the
 * old style message (in which case the version is assumed to be that of the
 * requestor) or the new one, in which case the protocol version is included
 * in the protocol message.  This allows our child process (which speaks the
 * current protocol) to look for yp servers on behalf old-style clients.
 */
void
ypbind_set_binding(rqstp, transp, vers)
	struct svc_req *rqstp;
	register SVCXPRT *transp;
	unsigned short vers;
{
	struct ypbind_setdom req;
	struct ypbind_oldsetdom oldreq;
	unsigned short version;
	struct in_addr addr;
	struct sockaddr_in *who; 
	unsigned short int port;
	char *domain;

	if (vers == YPVERS) {

#if	NeXT
		/*
		 * Zero for force malloced pointers. Otherwise, we may
		 * use bogus pointers on the stack.
		 */
		bzero(&req, sizeof(req));
#endif	NeXT
		if (!svc_getargs(transp, xdr_ypbind_setdom, &req) ) {
			svcerr_decode(transp);
			return;
		}

		version = req.ypsetdom_vers;
		addr = req.ypsetdom_addr;
		port = req.ypsetdom_port;
		domain = req.ypsetdom_domain;
	} else {

#if	NeXT
		/*
		 * Zero for force malloced pointers. Otherwise, we may
		 * use bogus pointers on the stack.
		 */
		bzero(&oldreq, sizeof(oldreq));
#endif	NeXT
		if (!svc_getargs(transp, _xdr_ypbind_oldsetdom, &oldreq) ) {
			svcerr_decode(transp);
			return;
		}

		version = vers;
		addr = oldreq.ypoldsetdom_addr;
		port = oldreq.ypoldsetdom_port;
		domain = oldreq.ypoldsetdom_domain;
	}

	/* find out who originated the request */
	who = svc_getcaller(transp);
	/* This code implements some restrictions on who can set the	*
 	 * yp server for this host 					*/

	/* This policy is that root can set the yp server to anything, 	*
     	 * everyone else can't. This should also check for a valid yp 	*
	 * server but time is short, 4.1 for sure			*/

   	if (ntohs(who->sin_port) > IPPORT_RESERVED) {
		fprintf(stderr,"ypbind: Set domain request to host %s, ",
			inet_ntoa(addr));
		fprintf(stderr,"from host %s, failed (bad port).\n",
			inet_ntoa(who->sin_addr));
		svcerr_systemerr(transp);
		return;
	}

	/* Now check the credentials */
	if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
		if (((struct authunix_parms *)rqstp->rq_clntcred)->aup_uid != 0) {
			fprintf(stderr,"ypbind: Set domain request to host %s,",
					inet_ntoa(addr));
			fprintf(stderr," from host %s, failed (not root).\n", 
					inet_ntoa(who->sin_addr));
			svcerr_systemerr(transp);
			return;
		}
	} else {
		fprintf(stderr, "ypbind: Set domain request to host %s,",
				inet_ntoa(addr));
		fprintf(stderr," from host %s, failed (credentials).\n", 
				inet_ntoa(who->sin_addr));
		svcerr_weakauth(transp);
		return;
	}

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		fprintf(stderr, "ypbind:  Can't reply to rpc call.\n");
	}

	if ( (current_domain = ypbind_point_to_domain(domain,
	    version) ) != (struct domain *) NULL) {
		current_domain->dom_serv_addr = addr;
		current_domain->dom_serv_port = port;
		current_domain->dom_boundp = TRUE;
		/* get rid of old pinging client if one exists */
		if (current_domain->ping_clnt != (CLIENT *)NULL) {
			clnt_destroy(current_domain->ping_clnt);
			current_domain->ping_clnt = (CLIENT *)NULL;
		}
	}
}
/*
 * This returns a pointer to a domain entry.  If no such domain existed on
 * the list previously, an entry will be allocated, initialized, and linked
 * to the list.  Note:  If no memory can be malloc-ed for the domain structure,
 * the functional value will be (struct domain *) NULL.
 */
static struct domain *
ypbind_point_to_domain(pname, vers)
	register char *pname;
	unsigned short vers;
{
	register struct domain *pdom;
	
	for (pdom = known_domains; pdom != (struct domain *)NULL;
	    pdom = pdom->dom_pnext) {
		if (!strcmp(pname, pdom->dom_name) && vers == pdom->dom_vers)
			return (pdom);
	}
	
	/* Not found.  Add it to the list */
	
	if (pdom = (struct domain *)malloc(sizeof (struct domain))) {
		pdom->dom_pnext = known_domains;
		known_domains = pdom;
		strcpy(pdom->dom_name, pname);
		pdom->dom_vers = vers;
		pdom->dom_boundp = FALSE;
		pdom->ping_clnt = (CLIENT *)NULL;
		pdom->dom_report_success = -1;
		pdom->dom_broadcaster_pid = 0;
		pdom->bindfile = -1;
	}
	
	return (pdom);
}

/*
 * This is called by the broadcast rpc routines to process the responses 
 * coming back from the broadcast request. Since the form of the request 
 * which is used in ypbind_broadcast_bind is "respond only in the positive  
 * case", we know that we have a server.  If we should be running secure,
 * return FALSE if this server is not using a reserved port.  Otherwise,
 * the internet address of the responding server will be picked up from
 * the saddr parameter, and stuffed into the domain.  The domain's boundp
 * field will be set TRUE.  The first responding server (or the first one
 * which is on a reserved port) will be the bound server for the domain.
 */
bool
ypbind_broadcast_ack(ptrue, saddr)
	bool *ptrue;
	struct sockaddr_in *saddr;
{
	/* if we should be running secure and the server is not using
	 * a reserved port, return FALSE
	 */
	if (secure && (saddr->sin_family != AF_INET ||
	    saddr->sin_port >= IPPORT_RESERVED) ) {
		return (FALSE);
	}
	current_domain->dom_boundp = TRUE;
	current_domain->dom_serv_addr = saddr->sin_addr;
	current_domain->dom_serv_port = saddr->sin_port;
	return(TRUE);
}

/*
 * This checks to see if a server bound to a named domain is still alive and
 * well.  If he's not, boundp in the domain structure is set to FALSE.
 */
bool
ypbind_ping(pdom)
	struct domain *pdom;

{
	struct sockaddr_in addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout;
	struct timeval intertry;
	bool	new_binding = FALSE;
	
	timeout.tv_sec = PINGTOTTIM;
	timeout.tv_usec = intertry.tv_usec = 0;
	if (pdom->ping_clnt == (CLIENT *)NULL) {
		new_binding = TRUE;
		intertry.tv_sec = PINGINTERTRY;
		addr.sin_addr = pdom->dom_serv_addr;
		addr.sin_family = AF_INET;
		addr.sin_port = pdom->dom_serv_port;
		bzero(addr.sin_zero, 8);
		if ((pdom->ping_clnt = clntudp_bufcreate(&addr, YPPROG,
		    pdom->dom_vers, intertry, &ping_sock,
		    RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == (CLIENT *)NULL) {
			clnt_pcreateerror("ypbind_ping) clntudp_create error");
			pdom->dom_boundp = FALSE;
			return (new_binding);
		} else {
			pdom->dom_serv_port = addr.sin_port;
		}
	}
	if ((clnt_stat = (enum clnt_stat) clnt_call(pdom->ping_clnt,
	    YPPROC_NULL, xdr_void, 0, xdr_void, 0, timeout)) != RPC_SUCCESS) {
		new_binding = TRUE;
		pdom->dom_boundp = FALSE;
		clnt_destroy(pdom->ping_clnt);	
		pdom->ping_clnt = (CLIENT *)NULL;
	}
	return (new_binding);
}

#ifdef INIT_DEFAULT
/*
 * Preloads the default domain's domain binding. Domain binding for the
 * local node's default domain for both the current version, and the
 * previous version will be set up.  Bindings to servers which serve the
 * domain for both versions may additionally be made.  
 */
static void
ypbind_init_default()
{
	char domain[256];
	char *pname = domain;
	int true;
	int binding_tries;

	if (getdomainname(domain, 256) == 0) {
		current_domain = ypbind_point_to_domain(domain, YPVERS);

		if (current_domain == (struct domain *) NULL) {
			abort();
		}
		
		for (binding_tries = 0;
		    ((!current_domain->dom_boundp) &&
		    (binding_tries < BINDING_TRIES) ); binding_tries++) {
			(void) clnt_broadcast(YPPROG, current_domain->dom_vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			
		}
		
		current_domain = ypbind_point_to_domain(domain, YPOLDVERS);

		if (current_domain == (struct domain *) NULL) {
			abort();
		}
		
		for (binding_tries = 0;
		    ((!current_domain->dom_boundp) &&
		    (binding_tries < BINDING_TRIES) ); binding_tries++) {
			(void) clnt_broadcast(YPPROG, current_domain->dom_vers,
			    YPPROC_DOMAIN_NONACK, xdr_ypdomain_wrap_string,
			    &pname, xdr_int, &true, ypbind_broadcast_ack);
			
		}
	}
}
#endif

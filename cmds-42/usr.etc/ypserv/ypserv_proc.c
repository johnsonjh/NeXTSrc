
#ifndef lint
static char sccsid[] = 	"@(#)ypserv_proc.c	1.1 88/03/07 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * This contains yellow pages server code which supplies the set of
 * functions requested using rpc.   The top level functions in this module
 * are those which have symbols of the form YPPROC_xxxx defined in
 * yp_prot.h, and symbols of the form YPOLDPROC_xxxx defined in ypsym.h.
 * The latter exist to provide compatibility to the old version of the yp
 * protocol/server, and may emulate the behaviour of the previous software
 * by invoking some other program.
 * 
 * This module also contains functions which are used by (and only by) the
 * top-level functions here.
 *  
 */

#include "ypsym.h"
#include "ypdefs.h"
USE_YP_PREFIX
#include <netdb.h>

extern char *environ;
#ifndef YPXFR_PROC
#define YPXFR_PROC "/usr/etc/yp/ypxfr"
#endif
static char ypxfr_proc[] = YPXFR_PROC;
#ifndef YPPUSH_PROC
#define YPPUSH_PROC "/usr/etc/yp/yppush"
#endif
static char yppush_proc[] = YPPUSH_PROC;
struct yppriv_sym {
	char *sym;
	unsigned len;
};
static char err_fork[] = "ypserv:  %s fork failure.\n";
#define FORK_ERR logprintf( err_fork, fun)
static char err_execl[] = "ypserv:  %s execl failure.\n";
#define EXEC_ERR logprintf( err_execl, fun)
static char err_respond[] = "ypserv: %s can't respond to rpc request.\n";
#define RESPOND_ERR logprintf( err_respond, fun)
static char err_free[] = "ypserv: %s can't free args.\n";
#define FREE_ERR logprintf( err_free, fun)
static char err_map[] = "ypserv: %s no such map or access denied.\n";
#define MAP_ERR logprintf( err_map, fun)

void ypfilter();
bool isypsym();
bool xdrypserv_ypall();

extern char *inet_ntoa();

/*
 * This determines whether or not a passed domain is served by this server,
 * and returns a boolean.  Used by both old and new protocol versions.
 */
void
ypdomain(rqstp, transp, always_respond)
	struct svc_req *rqstp;
	SVCXPRT *transp;
	bool always_respond;
{
	char domain_name[YPMAXDOMAIN + 1];
	char *pdomain_name = domain_name;
	bool isserved;
	char *fun = "ypdomain";

	if (!svc_getargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		svcerr_decode(transp);
		return;
	}

	isserved = (bool) ypcheck_domain(domain_name);

	if (isserved || always_respond) {
		
		if (!svc_sendreply(transp, xdr_bool, &isserved) ) {
			RESPOND_ERR;
		}
		if (!isserved)
			logprintf("Domain %s not supported\n",
				domain_name);

	} else {
		/*
		 * This case is the one in which the domain is not
		 * supported, and in which we are not to respond in the
		 * unsupported case.  We are going to make an error happen
		 * to allow the portmapper to end his wait without the
		 * normal udp timeout period.  The assumption here is that
		 * the only process in the world which is using the function
		 * in its no-answer-if-nack form is the portmapper, which is
		 * doing the krock for pseudo-broadcast.  If some poor fool
		 * calls this function as a single-cast message, the nack
		 * case will look like an incomprehensible error.  Sigh...
		 * (The traditional Unix disclaimer)
		 */
	
		svcerr_decode(transp);
		logprintf("Domain %s not supported (broadcast)\n",
				domain_name);
	}
#if	NeXT
	/*
	 * Don't free things on the stack!
	 */
#else	NeXT
	if (!svc_freeargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		FREE_ERR ;
	}
#endif	NeXT

}

/*
 * Inter-domain name support
 */
#define YPINTERDOMAIN_KEY "yp_interdomain"

DieAfterTimeout()
{  exit(0);
}

/*
 * When we don't find a match in our local domain, this function is
 * invoked to try the match using the inter-domain binding.
 * Returns one of the following three values to deftermine
 * if we forked or not, and if so, if we are in the child or
 * the parent process.  The child blocks while it queries the name
 * server trying to answer the request, while the parent immediately
 * returns to avoid any deadlock problems.
 */
# define NEITHER_PROC 0
# define CHILD_PROC 1
# define PARENT_PROC 2


int TryInterDomain(map, keydat, valdatp, statusp)
    char *map;		/* map name */
    datum keydat;	/* key to match (e.g. host name) */
    datum *valdatp;	/* returned value if found */
    unsigned *statusp;	/* returns the status */
{
	struct hostent *h;
	static char buf[1024];
	static char idkeyval[] = YPINTERDOMAIN_KEY;
	datum idkey, idval;
	int pid;
	int byname, byaddr;

        idkey.dptr = idkeyval;
        idkey.dsize = strlen(idkeyval);
	idval = fetch(idkey); /* See if it supports inter domain attempts */
	byname = strcmp(map,"hosts.byname") == 0;
	byaddr = strcmp(map,"hosts.byaddr") == 0;
	if (!idval.dptr || (!byname && !byaddr)) {
		*statusp = YP_NOKEY;
		return(NEITHER_PROC);
	}
#ifndef DEBUG
	pid = fork();
	if (pid < 0) {
		*statusp  = YP_YPERR;
		return(NEITHER_PROC);
	}
	if (pid != 0)
		return(PARENT_PROC);
	signal(SIGALRM, DieAfterTimeout);
	  /*
	   * should really tune this timeout more intelligently.
	   * 3 is OK, but a little small.  
	   * 10 causes many yserv's.
	   */
	alarm(8);
#endif DEBUG
	keydat.dptr[keydat.dsize] = 0;
	if (byname)
	    h = gethostbyname(keydat.dptr);
	else {
	    long addr;
	    addr = inet_addr(keydat.dptr);
	    h = gethostbyaddr( &addr, sizeof(addr), AF_INET);
	}
#ifndef DEBUG
	alarm(0);
#endif  DEBUG
	if (h == NULL) {
		if (h_errno == TRY_AGAIN || h_errno == NO_ADDRESS) exit(0);
		*statusp = YP_NOKEY;
#ifdef DEBUG
		return(NEITHER_PROC);
#else
		return(CHILD_PROC);
#endif DEBUG
	}
	sprintf(buf,"%s\t%s\n", inet_ntoa( *(int *)(h->h_addr_list[0]) ),
		h->h_name);
	valdatp->dptr = buf;
	valdatp->dsize = strlen(buf);
	*statusp = YP_TRUE;
#ifdef DEBUG
	return(NEITHER_PROC);
#else
	return(CHILD_PROC);
#endif DEBUG
}

/*
 * The following procedures are used only in the new protocol.
 */

/*
 * This implements the yp "match" function.
 */
void
ypmatch(rqstp, transp) 
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_key req;
	struct ypresp_val resp;
	char *fun = "ypmatch";
	int didfork = NEITHER_PROC;
	
	req.domain = req.map = NULL;
	req.keydat.dptr = NULL;
	resp.valdat.dptr = NULL;
	resp.valdat.dsize = 0;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, xdr_ypreq_key, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (ypset_current_map(req.map, req.domain, &resp.status) &&
	    yp_map_access(rqstp, transp, &resp.status) ) {

		resp.valdat = fetch(req.keydat);

		if (resp.valdat.dptr != NULL) {
			resp.status = YP_TRUE;
		} else {
	/*
	 * Try to do an inter-domain binding on the same name
	 * if this is for hosts.byname map.
	 */
 	    	didfork = TryInterDomain(req.map,req.keydat, &resp.valdat, &resp.status);
		}
	}

        if (didfork != PARENT_PROC)
	    if (!svc_sendreply(transp, xdr_ypresp_val, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_key, &req) ) {
		FREE_ERR;
	}
	if (didfork == CHILD_PROC) exit(0);
}


/*
 * This implements the yp "get first" function.
 */
void
ypfirst(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_nokey req;
	struct ypresp_key_val resp;
	char *fun = "ypfirst";
	
	req.domain = req.map = NULL;
	resp.keydat.dptr = resp.valdat.dptr = NULL;
	resp.keydat.dsize = resp.valdat.dsize = 0;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (ypset_current_map(req.map, req.domain, &resp.status) &&
	    yp_map_access(rqstp, transp, &resp.status) ) {
		ypfilter(NULL, &resp.keydat, &resp.valdat, &resp.status);
	}

	if (!svc_sendreply(transp, xdr_ypresp_key_val, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		FREE_ERR;
	}
}

/*
 * This implements the yp "get next" function.
 */
void
ypnext(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_key req;
	struct ypresp_key_val resp;
	char *fun = "ypnext";
	
	req.domain = req.map = req.keydat.dptr = NULL;
	req.keydat.dsize = 0;
	resp.keydat.dptr = resp.valdat.dptr = NULL;
	resp.keydat.dsize = resp.valdat.dsize = 0;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, xdr_ypreq_key, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (ypset_current_map(req.map, req.domain, &resp.status) &&
	    yp_map_access(rqstp, transp, &resp.status) ) {
		ypfilter(&req.keydat, &resp.keydat, &resp.valdat, &resp.status);

	}
	
	if (!svc_sendreply(transp, xdr_ypresp_key_val, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_key, &req) ) {
		FREE_ERR;
	}

}

/*
 * This implements the  "transfer map" function.  It takes the domain and
 * map names and the callback information provided by the requester (yppush
 * on some node), and execs a ypxfr process to do the actual transfer.  
 */
void
ypxfr(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_xfr req;
	struct ypresp_val resp;  /* not returned to the caller */
	char transid[10];
	char proto[15];
	char port[10];
	struct sockaddr_in *caller;
	char *ipaddr;
	int pid;
	char *fun = "ypxfr";

	req.ypxfr_domain = req.ypxfr_map = req.ypxfr_owner = NULL;
	req.ypxfr_ordernum = 0;
	pid = -1;
		
#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, xdr_ypreq_xfr, &req) ) {
		svcerr_decode(transp);
		return;
	}

	(void) sprintf(transid, "%d", req.transid);
	(void) sprintf(proto, "%d", req.proto);
	(void) sprintf(port, "%d", req.port);
	caller = svc_getcaller(transp);
	ipaddr = inet_ntoa(caller->sin_addr);

	/* Check that the map exists and is accessable */
	if (ypset_current_map(req.ypxfr_map, req.ypxfr_domain, &resp.status) &&
	    yp_map_access(rqstp, transp, &resp.status) ) {
		pid = vfork();
		if (pid == -1) {
			FORK_ERR;
		} else if (pid == 0) {
			if (execl(ypxfr_proc, "ypxfr", "-d", req.ypxfr_domain, 
			    		"-C", transid, proto, ipaddr, port, 
					req.ypxfr_map, NULL)) EXEC_ERR;
			_exit(1);
		}

	} else {
		MAP_ERR;
	}
	if (!svc_sendreply(transp, xdr_void, 0) ) {
		RESPOND_ERR;
	}
	
	if (!svc_freeargs(transp, xdr_ypreq_xfr, &req) ) {
		FREE_ERR;
	}
}

/*
 * This implements the "get all" function.
 */
void
ypall(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_nokey req;
	struct ypresp_val resp;  /* not returned to the caller */
	int pid;
	char *fun = "ypall";

	req.domain = req.map = NULL;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		svcerr_decode(transp);
		return;
	}

	pid = fork();
	
	if (pid) {
		
		if (pid == -1) {
			FORK_ERR;
		}

		if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
			FREE_ERR;
		}

		return;
	}
	
	/*
	 * access control hack:  If denied then invalidate the map name.
	 */
	ypclr_current_map();
	if (ypset_current_map(req.map, req.domain, &resp.status) &&
	    ! yp_map_access(rqstp, transp, &resp.status) ) {
		req.map[0] = '-';
	}
	/*
	 * This is the child process.  The work gets done by xdrypserv_ypall/
	 * we must clear the "current map" first so that we do not
	 * share a seek pointer with the parent server.
	 */
	
	if (!svc_sendreply(transp, xdrypserv_ypall, &req) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		FREE_ERR;
	}
	
	exit(0);
}

/*
 * This implements the "get master name" function.
 */
void
ypmaster(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_nokey req;
	struct ypresp_master resp;
	char *nullstring = "";
	char *fun = "ypmaster";
	
	req.domain = req.map = NULL;
	resp.master = nullstring;
	resp.status  = YP_TRUE;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (ypset_current_map(req.map, req.domain, &resp.status)) {
		
		if (!ypget_map_master(req.map, req.domain, &resp.master) ) {
			resp.status = YP_BADDB;
		}
	}
	
	if (!svc_sendreply(transp, xdr_ypresp_master, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		FREE_ERR;
	}
}

/*
 * This implements the "get order number" function.
 */
void
yporder(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct ypreq_nokey req;
	struct ypresp_order resp;
	char *fun = "yporder";
	
	req.domain = req.map = NULL;
	resp.status  = YP_TRUE;
	resp.ordernum  = 0;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, xdr_ypreq_nokey, &req) ) {
		svcerr_decode(transp);
		return;
	}

	resp.ordernum = 0;

	if (ypset_current_map(req.map, req.domain, &resp.status)) {

		if (!ypget_map_order(req.map, req.domain, &resp.ordernum) ) {
			resp.status = YP_BADDB;
		}
	}

	if (!svc_sendreply(transp, xdr_ypresp_order, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, xdr_ypreq_nokey, &req) ) {
		FREE_ERR;
	}
}

void
ypmaplist(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char domain_name[YPMAXDOMAIN + 1];
	char *pdomain_name = domain_name;
	char *fun = "ypmaplist";
	struct ypresp_maplist maplist;
	struct ypmaplist *tmp;

	maplist.list = (struct ypmaplist *) NULL;

	if (!svc_getargs(transp, xdr_ypdomain_wrap_string, &pdomain_name) ) {
		svcerr_decode(transp);
		return;
	}

	maplist.status = yplist_maps(domain_name, &maplist.list);
	
	if (!svc_sendreply(transp, xdr_ypresp_maplist, &maplist) ) {
		RESPOND_ERR;
	}

	while (maplist.list) {
		tmp = maplist.list->ypml_next;
		(void) free(maplist.list);
		maplist.list = tmp;
	}
}
/*
 * The following procedures are used only to support the old protocol.
 */

/*
 * This implements the yp "match" function.
 */
void
ypoldmatch(rqstp, transp) 
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	bool dbmop_ok = TRUE;
	struct yprequest req;
	struct ypresponse resp;
	char *fun = "ypoldmatch";
	int didfork = NEITHER_PROC;
	
	req.ypmatch_req_domain = req.ypmatch_req_map = NULL;
	req.ypmatch_req_keyptr = NULL;
	resp.ypmatch_resp_valptr = NULL;
	resp.ypmatch_resp_valsize = 0;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (req.yp_reqtype != YPMATCH_REQTYPE) {
		resp.ypmatch_resp_status = YP_BADARGS;
		dbmop_ok = FALSE;
	};

	if (dbmop_ok && ypset_current_map(req.ypmatch_req_map,
	    req.ypmatch_req_domain, &resp.ypmatch_resp_status) &&
	    yp_map_access(rqstp, transp, &resp.ypmatch_resp_status) ) {
		    
		resp.ypmatch_resp_valdat = fetch(req.ypmatch_req_keydat);

		if (resp.ypmatch_resp_valptr != NULL) {
			resp.ypmatch_resp_status = YP_TRUE;
		} else {
	/*
	 * Try to do an inter-domain binding on the same name
	 * if this is for hosts.byname map.
	 */
	 	    didfork = TryInterDomain(req.ypmatch_req_map,
		        req.ypmatch_req_keydat,
		        &resp.ypmatch_resp_valdat,
		    	&resp.ypmatch_resp_status);
		}

	}

	resp.yp_resptype = YPMATCH_RESPTYPE;

        if (didfork != PARENT_PROC)
	    if (!svc_sendreply(transp, _xdr_ypresponse, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		FREE_ERR;
	}
	if (didfork == CHILD_PROC) exit(0);
}

/*
 * This implements the yp "get first" function.
 */
void
ypoldfirst(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	bool dbmop_ok = TRUE;
	struct yprequest req;
	struct ypresponse resp;
	char *fun = "ypoldfirst";
	
	req.ypfirst_req_domain = req.ypfirst_req_map = NULL;
	resp.ypfirst_resp_keyptr = resp.ypfirst_resp_valptr = NULL;
	resp.ypfirst_resp_keysize = resp.ypfirst_resp_valsize = 0;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (req.yp_reqtype != YPFIRST_REQTYPE) {
		resp.ypfirst_resp_status = YP_BADARGS;
		dbmop_ok = FALSE;
	};

	if (dbmop_ok && ypset_current_map(req.ypfirst_req_map,
	    req.ypfirst_req_domain, &resp.ypfirst_resp_status) &&
	    yp_map_access(rqstp, transp, &resp.ypfirst_resp_status) ) {

		resp.ypfirst_resp_keydat = firstkey();

		if (resp.ypfirst_resp_keyptr != NULL) {
			resp.ypfirst_resp_valdat =
			    fetch(resp.ypfirst_resp_keydat);

			if (resp.ypfirst_resp_valptr != NULL) {
				resp.ypfirst_resp_status = YP_TRUE;
			} else {
				resp.ypfirst_resp_status = YP_BADDB;
			}

		} else {
			resp.ypfirst_resp_status = YP_NOKEY;
		}
	}

	resp.yp_resptype = YPFIRST_RESPTYPE;

	if (!svc_sendreply(transp, _xdr_ypresponse, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		FREE_ERR;
	}
}

/*
 * This implements the yp "get next" function.
 */
void
ypoldnext(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	bool dbmop_ok = TRUE;
	struct yprequest req;
	struct ypresponse resp;
	char *fun = "ypoldnext";
	
	req.ypnext_req_domain = req.ypnext_req_map = NULL;
	req.ypnext_req_keyptr = NULL;
	resp.ypnext_resp_keyptr = resp.ypnext_resp_valptr = NULL;
	resp.ypnext_resp_keysize = resp.ypnext_resp_valsize = 0;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (req.yp_reqtype != YPNEXT_REQTYPE) {
		resp.ypnext_resp_status = YP_BADARGS;
		dbmop_ok = FALSE;
	};

	if (dbmop_ok && ypset_current_map(req.ypnext_req_map,
	    req.ypnext_req_domain, &resp.ypnext_resp_status)  &&
	    yp_map_access(rqstp, transp, &resp.ypnext_resp_status) ) {
		resp.ypnext_resp_keydat = nextkey(req.ypnext_req_keydat);

		if (resp.ypnext_resp_keyptr != NULL) {
			resp.ypnext_resp_valdat =
			    fetch(resp.ypnext_resp_keydat);

			if (resp.ypnext_resp_valptr != NULL) {
				resp.ypnext_resp_status = YP_TRUE;
			} else {
				resp.ypnext_resp_status = YP_BADDB;
			}

		} else {
			resp.ypnext_resp_status = YP_NOMORE;
		}

	}
	
	resp.yp_resptype = YPNEXT_RESPTYPE;

	if (!svc_sendreply(transp, _xdr_ypresponse, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		FREE_ERR;
	}

}

/*
 * This retrieves the order number and master peer name from the map.
 * The conditions for the various message fields are:
 * 	domain is filled in iff the domain exists.
 *	map is filled in iff the map exists.
 * 	order number is filled in iff it's in the map.
 * 	owner is filled in iff the master peer is in the map.
 */
void
ypoldpoll(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yprequest req;
	struct ypresponse resp;
	char *map = "";
	char *domain = "";
	char *owner = "";
	unsigned error;
	char *fun = "ypoldpoll";
	
	req.yppoll_req_domain = req.yppoll_req_map = NULL;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

	resp.yppoll_resp_ordernum = 0;

	if (req.yp_reqtype == YPPOLL_REQTYPE) {
		if (strcmp(req.yppoll_req_domain,"yp_private")==0 ||
		    strcmp(req.yppoll_req_map,"ypdomains")==0 ||
		    strcmp(req.yppoll_req_map,"ypmaps")==0 ) {
		  /*
		   * backward compatibility for 2.0 YP servers
		   */
			domain = req.yppoll_req_domain;
			map = req.yppoll_req_map;
			resp.yppoll_resp_ordernum = 0;		    
		}

		else if (ypset_current_map(req.yppoll_req_map,
		    req.yppoll_req_domain, &error)) {
			domain = req.yppoll_req_domain;
			map = req.yppoll_req_map;
			(void) ypget_map_order(map, domain,
			    &resp.yppoll_resp_ordernum);
			(void) ypget_map_master(map, domain,
				    &owner);
		} else {
			
			switch (error) {
				
			case YP_BADDB:
				map = req.yppoll_req_map;
				/* Fall through to set the domain, too. */
			
			case YP_NOMAP:
				domain = req.yppoll_req_domain;
				break;
			}
		}
	}
	
	resp.yp_resptype = YPPOLL_RESPTYPE;
	resp.yppoll_resp_domain = domain;
	resp.yppoll_resp_map = map;
	resp.yppoll_resp_owner = owner;

	if (!svc_sendreply(transp, _xdr_ypresponse, &resp) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		FREE_ERR;
	}
}

/*
 * yppush
 */
void
yppush(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yprequest req;
	struct ypresp_val resp;  /* not returned to the caller */
	int pid = -1;
	char *fun = "yppush";

	req.yppush_req_domain = req.yppush_req_map = NULL;

#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (ypset_current_map(req.yppush_req_map, req.yppush_req_domain,
	    &resp.status) &&
	    yp_map_access(rqstp, transp, &resp.status) ) {
		pid = vfork();
	}
		
	if (pid == -1) {
		FORK_ERR;
	} else if (pid == 0) {

		ypclr_current_map();

		if (execl(yppush_proc, "yppush", "-d", req.yppush_req_domain, 
		    req.yppush_req_map, NULL) ) {
			EXEC_ERR;
		}

		_exit(1);
	}

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		RESPOND_ERR;
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		FREE_ERR;
	}
}

/*
 * This clears the current map, vforks, and execs the ypxfr process to get
 * the map referred to in the request.
 */
void
ypget(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yprequest req;
	struct ypresp_val resp;  /* not returned to the caller */
	int pid = -1;
	char *fun = "ypget";

	req.ypget_req_domain = req.ypget_req_map = req.ypget_req_owner = NULL;
	req.ypget_req_ordernum = 0;
	
#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		RESPOND_ERR;
	}

	if (req.yp_reqtype == YPGET_REQTYPE) {
		
		if (!ypset_current_map(req.ypget_req_map, req.ypget_req_domain,
		    &resp.status) ||
		    yp_map_access(rqstp, transp, &resp.status) ) {
			pid = vfork();
		}
		
		if (pid == -1) {
			FORK_ERR;
		} else if (pid == 0) {

			ypclr_current_map();

			if (execl(ypxfr_proc, "ypxfr", "-d",
			    req.ypget_req_domain, "-h", req.ypget_req_owner,
			    req.ypget_req_map, NULL) ) {
				EXEC_ERR;
			}

			_exit(1);
		}
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		RESPOND_ERR;
	}
}

/*
 * This clears the current map, vforks, and execs the ypxfr process to get
 * the map referred to in the request.
 */
void
yppull(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct yprequest req;
	struct ypresp_val resp;  /* not returned to the caller */
	int pid = -1;
	char *fun = "yppull";

	req.yppull_req_domain = req.yppull_req_map = NULL;
	
#if	NeXT
	/*
	 * Zero to force malloced pointers. Otherwise we may reuse
	 * bogus pointers on the stack.
	 */
	bzero(&req, sizeof(req));
#endif	NeXT
	if (!svc_getargs(transp, _xdr_yprequest, &req) ) {
		svcerr_decode(transp);
		return;
	}

	if (!svc_sendreply(transp, xdr_void, 0) ) {
		RESPOND_ERR;
	}

	if (req.yp_reqtype == YPPULL_REQTYPE) {
		
		if (!ypset_current_map(req.yppull_req_map,req.yppull_req_domain,
		    &resp.status) ||
		    yp_map_access(rqstp, transp, &resp.status) ) {
			pid = vfork();
		}
		
		if (pid == -1) {
			FORK_ERR;
		} else if (pid == 0) {

			ypclr_current_map();

			if (execl(ypxfr_proc, "ypxfr", "-d",
			    req.yppull_req_domain, req.yppull_req_map, NULL) ) {
				EXEC_ERR;
			}

			_exit(1);
		}
	}

	if (!svc_freeargs(transp, _xdr_yprequest, &req) ) {
		FREE_ERR;
	}
}

/*
 * Ancillary functions used by the top-level functions within this module
 */

/*
 * This returns TRUE if a given key is a yp-private symbol, otherwise FALSE
 */
static bool
isypsym(key)
	datum *key;
{

	if ((key->dptr == NULL) || (key->dsize < yp_prefix_sz) ||
	    bcmp(key->dptr, yp_prefix, yp_prefix_sz) ) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * This provides private-symbol filtration for the enumeration functions.
 */
static void
ypfilter(inkey, outkey, val, status)
	datum *inkey;
	datum *outkey;
	datum *val;
	int *status;
{
	datum k;

	if (inkey) {

		if (isypsym(inkey) ) {
			*status = YP_BADARGS;
			return;
		}
		
		k = nextkey(*inkey);
	} else {
		k = firstkey();
	}
	
	while (k.dptr && isypsym(&k)) {
		k = nextkey(k);
	}
		
	if (k.dptr == NULL) {
		*status = YP_NOMORE;
		return;
	}

	*outkey = k;
	*val = fetch(k);

	if (val->dptr != NULL) {
		*status = YP_TRUE;
	} else {
		*status = YP_BADDB;
	}
}
		
/*
 * Serializes a stream of struct ypresp_key_val's.  This is used
 * only by the ypserv side of the transaction.
 */
static bool
xdrypserv_ypall(xdrs, req)
	XDR * xdrs;
	struct ypreq_nokey *req;
{
	bool more = TRUE;
	struct ypresp_key_val resp;

	resp.keydat.dptr = resp.valdat.dptr = (char *) NULL;
	resp.keydat.dsize = resp.valdat.dsize = 0;
	
	if (ypset_current_map(req->map, req->domain, &resp.status)) {
		ypfilter(NULL, &resp.keydat, &resp.valdat, &resp.status);

		while (resp.status == YP_TRUE) {
			if (!xdr_bool(xdrs, &more) ) {
				return (FALSE);
			}

			if (!xdr_ypresp_key_val(xdrs, &resp) ) {
				return (FALSE);
			}

			ypfilter(&resp.keydat, &resp.keydat, &resp.valdat,
			    &resp.status);
		}
		
	}
	
	if (!xdr_bool(xdrs, &more) ) {
		return (FALSE);
	}

	if (!xdr_ypresp_key_val(xdrs, &resp) ) {
		return (FALSE);
	}

	more = FALSE;
	
	if (!xdr_bool(xdrs, &more) ) {
		return (FALSE);
	}

	return (TRUE);
}

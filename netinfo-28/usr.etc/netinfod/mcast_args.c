/*
 * NeXT Note - many_cast_args()
 *
 * many_cast() replacement. Allows different args for different hosts.
 *
 */
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = 	"@(#)many_cast_args.c	1.1 88/03/07 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


/*
 * many_cast_args: like broadcast, but to a specific group of hosts
 * TODO: different xid's per host, and return via callback.
 */
		
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include "clib.h"
#include "alert.h"
#include "socket_lock.h"

extern int errno;

/*
 * Structures and XDR routines for parameters to and replys from
 * the pmapper remote-call-service.
 */

struct rmtcallargs {
	u_long prog, vers, proc, arglen;
	caddr_t args_ptr;
	xdrproc_t xdr_args;
};
static bool_t xdr_rmtcall_args();

struct rmtcallres {
	u_long *port_ptr;
	u_long resultslen;
	caddr_t results_ptr;
	xdrproc_t xdr_results;
};
static bool_t xdr_rmtcallres();

/*
 * XDR remote call arguments
 * written for XDR_ENCODE direction only
 */
static bool_t
xdr_rmtcall_args(xdrs, cap)
	register XDR *xdrs;
	register struct rmtcallargs *cap;
{
	u_int lenposition, argposition, position;

	if (xdr_u_long(xdrs, &(cap->prog)) &&
	    xdr_u_long(xdrs, &(cap->vers)) &&
	    xdr_u_long(xdrs, &(cap->proc))) {
		lenposition = XDR_GETPOS(xdrs);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		argposition = XDR_GETPOS(xdrs);
		if (! (*(cap->xdr_args))(xdrs, cap->args_ptr))
		    return (FALSE);
		position = XDR_GETPOS(xdrs);
		cap->arglen = (u_long)position - (u_long)argposition;
		XDR_SETPOS(xdrs, lenposition);
		if (! xdr_u_long(xdrs, &(cap->arglen)))
		    return (FALSE);
		XDR_SETPOS(xdrs, position);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * XDR remote call results
 * written for XDR_DECODE direction only
 */
static bool_t
xdr_rmtcallres(xdrs, crp)
	register XDR *xdrs;
	register struct rmtcallres *crp;
{

	if (xdr_reference(xdrs, &crp->port_ptr, sizeof (u_long), xdr_u_long) &&
		xdr_u_long(xdrs, &crp->resultslen))
		return ((*(crp->xdr_results))(xdrs, crp->results_ptr));
	return (FALSE);
}

typedef bool_t (*resultproc_t)();

#define	MAXHOSTS	20

enum clnt_stat 
_many_cast_args(
	       unsigned naddrs,		/* number of addresses */
	       struct in_addr *addrs, 	/* list of host addresses */
	       u_long  prog,		/* program number */
	       u_long vers,		/* version number */
	       u_long proc,		/* procedure number */
	       xdrproc_t xargs,		/* xdr routine for args */
	       void *argsvec,		/* pointer to args vec */
	       unsigned argsize,	/* size of each arg */
	       xdrproc_t xresults,	/* xdr routine for results */
	       void *resultsp,		/* pointer to results */
	       resultproc_t eachresult,	/* call with each result obtained */
	       int timeout		/* maximum time to wait */
	       )
{
	enum clnt_stat stat = RPC_FAILED;
	AUTH *unix_auth = authunix_create_default();
	XDR xdr_stream;
	register XDR *xdrs = &xdr_stream;
	int outlen, inlen, fromlen;
	fd_set readfds;
	register int sock, i;
	fd_set mask;
	bool_t done = FALSE;
	register u_long xid;
	u_long port;
	struct sockaddr_in baddr, raddr; /* broadcast and response addresses */
	struct rmtcallargs a;
	struct rmtcallres r;
	struct rpc_msg msg;
	struct timeval t; 
	char outbuf[UDPMSGSIZE], inbuf[UDPMSGSIZE];
	int waited;
	int dtsize = getdtablesize();

	/*
	 * initialization: create a socket, a broadcast address, and
	 * preserialize the arguments into a send buffer.
	 */
	socket_lock();
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		syslog(LOG_ERR, "Cannot create socket for manycast rpc: %m");
		stat = RPC_CANTSEND;
		socket_unlock();
		goto done_broad;
	}
	socket_unlock();
	FD_ZERO(&mask);
	FD_SET(sock, &mask);
	bzero((char *)&baddr, sizeof (baddr));
	baddr.sin_family = AF_INET;
	baddr.sin_port = htons(PMAPPORT);
	(void)gettimeofday(&t, (struct timezone *)0);
	xid = getpid() ^ t.tv_sec ^ t.tv_usec;
	t.tv_usec = 0;
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = PMAPPROG;
	msg.rm_call.cb_vers = PMAPVERS;
	msg.rm_call.cb_proc = PMAPPROC_CALLIT;
	msg.rm_call.cb_cred = unix_auth->ah_cred;
	msg.rm_call.cb_verf = unix_auth->ah_verf;
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	r.xdr_results = xresults;
	r.results_ptr = resultsp;
	a.xdr_args = xargs;
	r.port_ptr = &port;

	/*
	 * Basic loop: send packet to all hosts and wait for response(s).
	 * The response timeout grows larger per iteration.
	 */
	waited = 0;
	for (t.tv_sec = 4; t.tv_sec <= 14; t.tv_sec += 2) {
		for (i = 0; i < naddrs; i++) {
			a.args_ptr = argsvec + (i * argsize);
			xdrmem_create(xdrs, outbuf, sizeof outbuf, XDR_ENCODE);
			msg.rm_xid = xid + i;
			if ((! xdr_callmsg(xdrs, &msg)) || 
			    (! xdr_rmtcall_args(xdrs, &a))) {
				stat = RPC_CANTENCODEARGS;
				goto done_broad;
			}
			outlen = (int)xdr_getpos(xdrs);
			xdr_destroy(xdrs);
			baddr.sin_addr = addrs[i];
			if (sendto(sock, outbuf, outlen, 0,
			    (struct sockaddr *)&baddr,
			    sizeof (struct sockaddr)) != outlen) {
				syslog(LOG_ERR,
				    "Cannot send manycast packet to %s: %m",
				       inet_ntoa(baddr.sin_addr));
				stat = RPC_CANTSEND;
			}
		}
	recv_again:
		msg.acpted_rply.ar_verf = _null_auth;
		msg.acpted_rply.ar_results.where = (caddr_t)&r;
		msg.acpted_rply.ar_results.proc = xdr_rmtcallres;
		readfds = mask;
		if (alert_aborted()) {
			goto done_broad;
		}
		switch (select(dtsize, (fd_set *)&readfds, (fd_set *)NULL, 
			       (fd_set *)NULL, &t)) {

		case 0:  /* timed out */
			stat = RPC_TIMEDOUT;
			waited += t.tv_sec;
			if (timeout && waited >= timeout) {
				goto done_broad;
			}
			continue;

		case -1:  /* some kind of error */
			if (errno == EINTR)
				goto recv_again;
			syslog(LOG_ERR, "Many_cast select problem: %m");
			stat = RPC_CANTRECV;
			goto done_broad;

		}  /* end of select results switch */
		if (!FD_ISSET(sock, &readfds)) {
			goto recv_again;
		}
	try_again:
		fromlen = sizeof(struct sockaddr);
		inlen = recvfrom(sock, inbuf, sizeof inbuf, 0,
			(struct sockaddr *)&raddr, &fromlen);
		if (inlen < 0) {
			if (errno == EINTR)
				goto try_again;
			syslog(LOG_ERR,
			    "Cannot receive reply to many_cast: %m");
			stat = RPC_CANTRECV;
			goto done_broad;
		}
		if (inlen < sizeof(u_long))
			goto recv_again;
		/*
		 * see if reply transaction id matches sent id.
		 * If so, decode the results.
		 */
		xdrmem_create(xdrs, inbuf, inlen, XDR_DECODE);
		if (xdr_replymsg(xdrs, &msg)) {
			if ((msg.rm_xid >= xid && msg.rm_xid < xid + naddrs) &&
			    (msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
			    (msg.acpted_rply.ar_stat == SUCCESS)) {
				raddr.sin_port = htons((u_short)port);
				done = (*eachresult)(resultsp, &raddr,
						     msg.rm_xid - xid);
			}
			/* otherwise, we just ignore the errors ... */
		}
		xdrs->x_op = XDR_FREE;
		msg.acpted_rply.ar_results.proc = xdr_void;
		(void)xdr_replymsg(xdrs, &msg);
		(void)(*xresults)(xdrs, resultsp);
		xdr_destroy(xdrs);
		if (done) {
			stat = RPC_SUCCESS;
			goto done_broad;
		} else {
			goto recv_again;
		}
	}
done_broad:
	socket_lock();
	(void)close(sock);
	socket_unlock();
	AUTH_DESTROY(unix_auth);
	return (stat);
}

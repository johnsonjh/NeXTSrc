#ifndef lint
static char sccsid[] = "@(#)prot_pnlm.c	1.1 88/08/05 NFSSRC4.0 1.10 88/02/07 Copyr 1986 Sun Micro";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* prot_pnlm.c
	 * consists of all procedures called bu nlm_prog
	 */

#include <stdio.h>
#include "prot_lock.h"

extern int debug;
extern reclock *call_q;	
extern SVCXPRT *nlm_transp;

extern msg_entry *search_msg();
extern remote_result *local_lock(), *local_unlock(), *local_test(), *local_cancel(), *local_granted(), *local_granted_msg();
extern remote_result *cont_test(), *cont_lock(), *cont_unlock(), *cont_cancel(), *cont_reclaim();

proc_nlm_test(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("proc_nlm_test(%x) \n", a);
	result = local_test(a);
	nlm_reply(NLM_TEST, result, a);
}

proc_nlm_lock(a)
	reclock *a;
{
	remote_result *result;


	if (debug)
		printf("enter proc_nlm_lock(%x) \n", a);
	result = local_lock(a);
	nlm_reply(NLM_LOCK, result, a);
}

proc_nlm_cancel(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_cancel(%x) \n", a);
	result = local_cancel(a);
	nlm_reply(NLM_CANCEL, result, a);
}

proc_nlm_unlock(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_unlock(%x) \n", a);
	result = local_unlock(a);
	nlm_reply(NLM_UNLOCK, result, a);
}

proc_nlm_granted(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_granted(%x)\n", a);
	result = local_granted(a, RPC);
	if (result != NULL) {
		nlm_reply(NLM_GRANTED, result, a);
	}
}

proc_nlm_test_msg(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_test_msg(%x)\n", a);
	result = local_test(a);
	nlm_reply(NLM_TEST_MSG, result, a);
}

proc_nlm_lock_msg(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_lock_msg(%x)\n", a);
	result = local_lock(a);
	nlm_reply(NLM_LOCK_MSG, result, a);
}

proc_nlm_cancel_msg(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_cancel_msg(%x)\n", a);
	result = local_cancel(a);
	nlm_reply(NLM_CANCEL_MSG, result, a);
}

proc_nlm_unlock_msg(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_unlock_msg(%x)\n", a);
	result = local_unlock(a);
	nlm_reply(NLM_UNLOCK_MSG, result, a);
}

proc_nlm_granted_msg(a)
	reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_granted_msg(%x)\n", a);
	result = local_granted(a, MSG);
	if (result != NULL) 
		nlm_reply(NLM_GRANTED_MSG, result, a);
}

/* 
 * return rpc calls;
 * if rpc calls, directly reply to the request;
 * if msg passing calls, initiates one way rpc call to reply!
 */
nlm_reply(proc, reply, a)
	int proc;
	remote_result *reply;
	reclock *a;
{
	bool_t (*xdr_reply)();
	int act;
	int nlmreply = 1;
	int newcall = 2;
	int rpc_err;
	char *name;
	int valid;

	switch(proc) {
	case NLM_TEST:
		xdr_reply = xdr_nlm_testres;
		act = nlmreply;
		break;
	case NLM_LOCK:
	case NLM_CANCEL:
	case NLM_UNLOCK:
	case NLM_GRANTED:
		xdr_reply = xdr_nlm_res;
		act = nlmreply;
		break;
	case NLM_TEST_MSG:
		xdr_reply = xdr_nlm_testres;
		act = newcall;
		proc = NLM_TEST_RES;
		name = a->lck.clnt;
		break;
	case NLM_LOCK_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_LOCK_RES;
		name = a->lck.clnt;
		break;
	case NLM_CANCEL_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_CANCEL_RES;
		name = a->lck.clnt;
		break;
	case NLM_UNLOCK_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_UNLOCK_RES;
		name = a->lck.clnt;
		break;
	case NLM_GRANTED_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_GRANTED_RES;
		name = a->lck.svr;
		break;
	default:
		printf("unknown nlm_reply proc vaule: %d\n", proc);
		return;
	}
	if (act == nlmreply) { /* reply to nlm_transp */
		if (debug)
			printf("rpc nlm_reply %d: %d\n", proc, reply->lstat);
		 if (!svc_sendreply(nlm_transp, xdr_reply, reply)) 
			svcerr_systemerr(nlm_transp);
		return;
	}
	else { /* issue a one way rpc call to reply */
		if (debug)
			printf("nlm_reply: (%s, %d), result = %d\n",
			name, proc, reply->lstat);
		reply->cookie_len = a->cookie_len;
		reply->cookie_bytes = a->cookie_bytes;
		valid = 1;
		if ((rpc_err = call_udp(name, NLM_PROG, NLM_VERS, proc, xdr_reply, reply, xdr_void, NULL, valid, 0))
		 != (int) RPC_TIMEDOUT && rpc_err != (int) RPC_SUCCESS) {
			/* in case of error, print out error msg */
			clnt_perrno(rpc_err);
			fprintf(stderr, "\n");
		}
	}

}

proc_nlm_test_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply,  cont_test);
}

proc_nlm_lock_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply,  cont_lock);
}

proc_nlm_cancel_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply,  cont_cancel);
}

proc_nlm_unlock_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply,  cont_unlock);
}

/*
 * common routine shared by all nlm routines that expects replies from svr nlm: 
 * nlm_lock_res, nlm_test_res, nlm_unlock_res, nlm_cancel_res
 * private routine "cont" is called to continue local operation;
 * reply is match with msg in msg_queue according to cookie
 * and then attached to msg_queue;
 */
nlm_res_routine(reply, cont)
	remote_result *reply;
	remote_result *(*cont)();
{
	msg_entry *msgp;
	remote_result *resp;

	if ((msgp = search_msg(reply)) != NULL) {	/* found */
		if (msgp->reply != NULL) { /* reply already exists */
			if (msgp->reply->lstat != reply->lstat) {
				fprintf(stderr, "inconsistent  lock reply exists, ignored \n"); 
				if (debug)
					printf("inconsistent reply (%d, %d) exists for lock(%x)\n", msgp->reply->lstat, reply->lstat, msgp->req);
			}
			release_res(reply);
			return;
		}
		/* continue process req according to remote reply */
		if (debug) {
			printf("nlm_res_routine(%x)\n", msgp->req);
			(void) fflush(stdout);
		}
		if (msgp->proc == NLM_LOCK_RECLAIM) 
			/* reclaim response */
			resp = cont_reclaim(msgp->req, reply);
		else 
			/* normal response */
			resp = cont(msgp-> req , reply);	
		add_reply(msgp, resp);
	}
	else
		release_res(reply);			/* discard this resply */
}

proc_nlm_granted_res(reply)
	remote_result *reply;
{
	msg_entry *msgp;

	if (debug)
		printf("enter nlm_granted_res\n");
	if ((msgp = search_msg(reply)) != NULL)
		dequeue(msgp);
}

/*
 * rpc msg passing calls to nlm msg procedure;
 * used by local_lock, local_test, local_cancel and local_unloc;
 * proc specifis the name of nlm procedures;
 * retransmit indicate whether this is retransmission;
 * rpc_call return -1 if rpc call is not successful, clnt_perrno is printed out;
 * rpc_call return 0 otherwise
 */
nlm_call(proc, a, retransmit)
	int proc;
	reclock *a;
	int retransmit;
{
	int rpc_err;
	bool_t (*xdr_arg)();
	char *name;
	int func;
	int valid;

	func = proc;		/* this is necc for NLM_LOCK_RECLAIM */
	if (retransmit == 0)
		valid = 1;		/* use cache value for first time calls */
	else
		valid = 0;		/* invalidate cache */
	switch(proc) {
	case NLM_TEST_MSG:
		xdr_arg = xdr_nlm_testargs;
		name = a->lck.svr;
		break;
	case NLM_LOCK_MSG:
		xdr_arg = xdr_nlm_lockargs;
		name = a->lck.svr;
		break;
	case NLM_LOCK_RECLAIM:
		xdr_arg = xdr_nlm_lockargs;
		name = a->lck.svr;
		func = NLM_LOCK_MSG;
		valid = 0; 	/* turn off udp cache */
		break;
	case NLM_CANCEL_MSG:
		xdr_arg = xdr_nlm_cancargs;
		name = a->lck.svr;
		break;
	case NLM_UNLOCK_MSG:
		xdr_arg = xdr_nlm_unlockargs;
		name = a->lck.svr;
		break;
	case NLM_GRANTED_MSG:
		xdr_arg = xdr_nlm_testargs;
		name = a->lck.clnt;
		a->lck.caller_name = a->lck.server_name; /* modify caller name */
		break;
	default:
		printf("%d not supported in nlm_call\n", proc);
		return(-1);
	}

	if (debug)
		printf("nlm_call to (%s, %d) op=%d, (%d, %d); retran = %d, valid = %d\n",
			name, proc, a->lck.op, a->lck.l_offset, a->lck.l_len, retransmit, valid);
	/* 
 	 * call is a one way rpc call to simulate msg passing
 	 * no timeout nor reply is specified;
 	 */
	if ((rpc_err = call_udp(name, NLM_PROG, NLM_VERS, func, xdr_arg, a, xdr_void, NULL, valid, 0)) == (int) RPC_TIMEDOUT ) {
		/* if rpc call is successful, add msg to msg_queue */
		if (retransmit == 0)	/* first time calls */
			if (queue(a, proc) == NULL) {
				return(-1);
			}
		return(0);
	}
	else {
		if (debug) {
			clnt_perrno(rpc_err);
			fprintf(stderr, "\n");
		}
		return(-1);
	}
} 

call_back()
{
	reclock *nl;

	if (call_q == NULL)		/* no need to call back */
		return;
	nl = call_q;
	while ( nl!= NULL) {
		if (debug)
			printf("enter call_back(%d, %d), op =%d\n",
			nl->lck.l_offset, nl->lck.l_len, nl->lck.op);
		if (nlm_call(NLM_GRANTED_MSG, nl, 0) == -1)
			abort();
		nl = nl->nxt;
	}
	call_q = NULL;
}

#ifndef lint
static char sccsid[] = "@(#)prot_pklm.c	1.1 88/08/05 NFSSRC4.0 1.8 88/07/16 Copyr 1986 Sun Micro";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* prot_pklm.c
	 * consists of all procedures called by klm_prog
	 */

#include <stdio.h>
#include "prot_lock.h"
#define same_proc(x, y) (obj_cmp(&x->lck.oh, &y->lck.oh))

int tmp_ck;				/* flag to trigger tmp check*/
extern int debug;
extern msg_entry *klm_msg;		/* record last klm msg */
extern msg_entry *msg_q;
extern SVCXPRT *klm_transp;

extern msg_entry *retransmitted(), *queue();
extern bool_t remote_data();
extern reclock *search_block_lock();

remote_result *local_test();
remote_result *local_lock();
remote_result *local_unlock();
remote_result *local_cancel();
remote_result *remote_test();
remote_result *remote_lock();
remote_result *remote_unlock();
remote_result *remote_cancel();

proc_klm_test(a)
	reclock *a;
{
	klm_msg_routine(a, KLM_TEST, local_test, remote_test);
}

proc_klm_lock(a)
	reclock *a;
{
	klm_msg_routine(a, KLM_LOCK, local_lock, remote_lock);
};

proc_klm_cancel(a)
	reclock *a;
{
	tmp_ck = 1;
	klm_msg_routine(a, KLM_CANCEL, local_cancel, remote_cancel);
};

proc_klm_unlock(a)
	reclock *a;
{
	klm_msg_routine(a, KLM_UNLOCK, local_unlock, remote_unlock);
};

/*
 * common routine to handle msg passing form of communication;
 * klm_msg_routine is shared among all klm procedures: 
 * proc_klm_test, proc_klm_lock, proc_klm_cancel, proc_klm_unlock;
 * proc specifies the name of the routine to branch to for reply purpose;
 * local and remote specify the name of routine that handles the call
 *
 * when a msg arrives, it is first checked to see
 *   if retransmitted;
 *	 if a reply is ready,
 *	   a reply is sent back and msg is erased from the queue 
 *	 or msg is ignored!
 *   else if this is a new msg;
 *	 if data is remote
 *           a rpc request is send and msg is put into msg_queue,
 *	 else (request lock or similar lock)
		 reply is sent back immediately.
 */
klm_msg_routine(a, proc, local, remote )
	reclock *a;
	int proc;		
	remote_result *(*local)();
	remote_result *(*remote)();
{
	struct msg_entry *msgp;
	remote_result *result;
	reclock *nl;
	reclock *reqp;

	if (debug) {
		printf("enter klm_msg_routine(proc =%d): op=%d, (%d, %d) by ", proc, a->lck.op, a->lck.l_offset, a->lck.l_len);
		pr_oh(&a->lck.oh);
		printf("\n");
		pr_lock(a);
		(void) fflush(stdout);
	}

	if ((msgp = retransmitted(a, proc)) != NULL) {/* retransmitted msg */
		if (debug)
			printf("retransmitted msg!\n");
		a->rel = 1;			/* set release bit */
		if (msgp->reply == NULL) {
			klm_msg = msgp;	/* record last received klm msg */
			return;
		}
		else {
			if (msgp->reply->lstat != blocking) {
				klm_reply(proc, msgp->reply);
				dequeue(msgp);
			}
			else {
				klm_msg = msgp;
			}
			return;
		}
	}
	else {
		/* tmp check inconsistency of process! */
		msgp = msg_q;
		while (msgp != NULL) {
			reqp = msgp->req;
			if (tmp_ck == 0 && proc != KLM_CANCEL && msgp->proc != NLM_LOCK_RECLAIM  && same_proc(reqp, a)) {
				fprintf(stderr, "*****warning:*******process issues request %x (proc = %d) before obtaining response for %x\n",
				 a, proc, msgp->req);
			}
			msgp = msgp ->nxt;
		}

		if (proc == KLM_LOCK && !remote_data(a) && (nl =  search_block_lock(a)) != NULL) { /* set up entry in msg queue */
		/* retransmitted local block lock */
		if (debug)
			printf("retransmitted local block lock (%x)\n, nl");
		a->rel = 1;
		msgp = queue(nl, KLM_LOCK); /* o.k., if queue returns NULL; */
		klm_msg = msgp;
		return;
		}
	}

	if (remote_data(a))
		result = remote(a, MSG);	/* specify msg passing type of comm */
	else 
		result = local(a);
	if (result != NULL)
		klm_reply(proc, result);
	
}

/*
 * klm_reply send back reply from klm to requestor(kernel):
 * proc specify the name of the procedure return the call;
 * corresponding xdr routines are then used;
 */
klm_reply(proc, reply)
	int proc;
	remote_result *reply;
{
	bool_t (*xdr_reply)();

	switch(proc) {
	case KLM_TEST:
	case NLM_TEST_MSG:	/* record in msgp->proc */
		xdr_reply = xdr_klm_testrply; 
		break;
	case KLM_LOCK:
	case NLM_LOCK_MSG:
	case NLM_LOCK_RECLAIM:
	case KLM_CANCEL:
	case NLM_CANCEL_MSG:
	case KLM_UNLOCK:
	case NLM_UNLOCK_MSG:
	case NLM_GRANTED_MSG:
		xdr_reply = xdr_klm_stat;
		break;
	default:
		xdr_reply = xdr_void;
		printf("unknown klm_reply proc(%d)\n", proc);
	}
	if (!svc_sendreply(klm_transp, xdr_reply, &reply->stat))
		svcerr_systemerr(klm_transp);
	if (debug)
		printf("klm_reply: stat=%d\n", reply->lstat);
	return;
}

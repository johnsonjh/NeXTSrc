#ifndef lint
static char sccsid[] = "@(#)prot_proc.c	1.1 88/08/05 NFSSRC4.0 1.11 88/02/07 Copyr 1986 Sun Micro";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* prot_proc.c 
	 * consists all local, remote, and continuation routines:
	 * local_xxx, remote_xxx, and cont_xxx.
	 */

#include <stdio.h>
#include <sys/file.h>
#include "prot_lock.h"
#define same_proc(x, y) (obj_cmp(&x->lck.oh, &y->lck.oh))

remote_result nlm_result;		/* local nlm result */
remote_result *nlm_resp = &nlm_result;	/* ptr to klm result */

remote_result *remote_cancel(), *remote_lock(), *remote_test(), *remote_unlock();
remote_result *local_test(), *local_lock(), *local_cancel(), *local_unlock();

msg_entry *search_msg(), *retransmitted();
char *xmalloc();
struct fs_rlck *find_fe();
reclock *search_lock(), *search_block_lock();

extern int debug;
extern int res_len;
extern msg_entry *klm_msg;

remote_result *
	local_lock(a)
	reclock *a;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if (debug)
		printf("enter local_lock\n");
	if (blocked(&fp, &insrtp, a) == 1) {  /*blocked*/
		if (a->lck.op & LOCK_NB) {
			nlm_resp->lstat = denied;
			a->rel = 1;		/* to release a */
		}
		else { 				/* add_wait */
			add_wait(a);
			nlm_resp->lstat = blocking;
		}
	}
	else { /* not blocked*/
		if (add_reclock(fp, insrtp, a) == -1) {
			nlm_resp->lstat= nolocks;
			a->rel = 1;
		}
		else 
			nlm_resp->lstat = granted;
	}
	return(nlm_resp);
}

/*
 * remote_lock and local_lock have great similarities!
 * choice == RPC; rpc calls to remote;
 * choice == MSG; msg passing calls to remote;
 */
remote_result *
remote_lock(a, choice)
	reclock *a;
	int choice;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if (debug)
		printf("enter remote_lock\n");
	if (blocked(&fp, &insrtp, a) == 1) { /*blocked*/
		if (a->lck.op & LOCK_NB) {
			nlm_resp->lstat = denied;
			a->rel = 1;
			return(nlm_resp);
		}
	}
	/* blocked == 0 or blocked == 1 and blocking */
	if ( insrtp != NULL && same_proc(insrtp, a) && same_op(insrtp, a) && inside(a, insrtp)) {
		/*lock exists*/
		a->rel = 1;		
		nlm_resp->lstat = granted;
		return(nlm_resp);
	}
	else {		/* consult with svr lock manager*/
		if (choice == MSG) { /* msg passing */
			if (nlm_call(NLM_LOCK_MSG, a, 0) == -1)
				a->rel = 1;		/* rpc error, discard */
			return(NULL);			/* no reply available */
		}
		else { /*rpc*/
			printf("rpc not supported\n");
			a->rel = 1;
			return(NULL);
		}

	}
}

remote_result *
local_unlock(a)
	reclock *a;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if (debug)
		printf("enter local_unlock\n");
	insrtp = NULL;
	if ((fp = find_fe(a)) != NULL)			/*found*/
		if (delete_reclock(fp, &insrtp, a) >0 )	/* more than one entry deleted */
			(void) wakeup(a);
	a->rel = 1;
	nlm_resp->lstat = granted;
	return(nlm_resp);
}

remote_result *
remote_unlock(a, choice)
	reclock *a;
	int choice;
{
	if (debug)
		printf("enter remote_unlock\n");
	if ( find_fe(a) != NULL) {  /*found*/
		if (choice == MSG) { 
			if (nlm_call(NLM_UNLOCK_MSG, a, 0) == -1)
				a->rel = 1;
			return(NULL);
			}

		/* rpc case */
		else {
			printf("rpc not supported\n");
			a->rel = 1;
			return(NULL);
		}
	}
	a->rel = 1;		/* not found */
	nlm_resp->lstat = granted;
	return(nlm_resp);
}

	
remote_result *
local_test(a)
	reclock *a;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if (debug)
		printf("enter local_test\n");
	a->rel = 1;
	if (blocked(&fp, &insrtp, a) == 0){ /* nonblocking */
		nlm_resp->lstat = granted;
		return(nlm_resp);
	}
	else{
		nlm_resp->lstat = denied;
		nlm_resp->lholder.svid = insrtp->lck.svid;
		nlm_resp->lholder.l_offset = insrtp->lck.l_offset;
		nlm_resp->lholder.l_len = insrtp->lck.l_len;
		nlm_resp->lholder.exclusive = insrtp->exclusive;
		(void) obj_copy( &nlm_resp->lholder.oh, &insrtp->lck.oh);
		if (debug)
			printf("lock blocked by %d, (%d, %d)\n",
			nlm_resp->lholder.svid, nlm_resp->lholder.l_offset, nlm_resp->lholder.l_len );
		return(nlm_resp);
	}
}

remote_result *
remote_test(a, choice)
	reclock *a;
	int choice;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if (debug)
		printf("enter remote_test\n");
	if (blocked(&fp, &insrtp, a) == 0) { /* nonblocking */
		a->rel = 1;
		nlm_resp->lstat = granted;
		return(nlm_resp);
	}
	else {
		if (choice == MSG) {
			if (nlm_call(NLM_TEST_MSG, a, 0) == -1)
				a->rel = 1;
			return(NULL);
		}

		/* rpc case */
		else {
			printf("rpc not supported\n");
			a->rel = 1;
			return(NULL);
		}
	}
}

remote_result *
local_cancel(a)
	reclock *a;
{
	reclock *nl;
	msg_entry *msgp;

	if (debug)
		printf("enter local_cancel(%x)\n", a);
	a->rel = 1;
	if ((nl = search_lock(a)) == NULL)
	/* the use of grant is confusing here */
		nlm_resp->lstat = denied;
	else {
		if (nl->w_flag == 0)
			nlm_resp->lstat = granted;	
		else {
			remove_wait(nl);
			nl->rel = 1;
			if (!remote_clnt(nl)) {
				if ((msgp = retransmitted(nl, KLM_LOCK)) != NULL) {
					if (debug)
						printf(" local_cancel: dequeue(%x)\n", msgp->req);
					dequeue(msgp);
				}
				else {
					release_le(nl);
				}
			}
			else
				release_le(nl);
			nlm_resp->lstat = denied;
			}
		}
	return(nlm_resp);
}

remote_result *
remote_cancel(a, choice)
	reclock *a;
	int choice;
{
	reclock *nl;
	msg_entry *msgp;

	if (debug)
		printf("enter remote_cancel(%x)\n", a);
	if ((nl = search_lock(a)) == NULL) {
		if ((msgp = retransmitted(a, KLM_LOCK)) == NULL) {
			/* msg was never received */
			a->rel = 1;
			nlm_resp->lstat = denied;
			return(nlm_resp);
		}
		else { /* msg is being processed */
			if (debug)
				printf("remove msg(%x) due to remote cancel\n", msgp->req);
			msgp->req->rel = 1;
			if (a->pre_le != NULL || a->pre_fe != NULL) {
				fprintf(stderr, "rpc.lockd: cancel request pre_le=%x pre_fe = %x\n", a->pre_le, a->pre_fe);
			}
			else { /* take over the pre_fe and pre_le */
				a->pre_le = msgp->req->pre_le;
				a->pre_fe = msgp->req->pre_fe;
				msgp->req->pre_le = NULL;
				msgp->req->pre_fe = NULL;
			}
			dequeue(msgp);
		}
	}
	else {
		if (nl->w_flag == 0) {
			a->rel = 1;
			nlm_resp->lstat = granted;	
			return(nlm_resp);
		}
	}				

	if (choice == MSG){
		if (nlm_call(NLM_CANCEL_MSG, a, 0) == -1)
			a->rel = 1;
		return(NULL);
	}
	else { /* rpc case */
		a->rel = 1;
		printf("rpc not supported\n");
		return(NULL);
	}

}


/*
 * local_granted reply to kernel if a is still in wait_queue;
 * return NULL if msg is discarded
 */
remote_result *
local_granted(a, choice)
	reclock *a;
	int choice;
{
	reclock *nl;
	struct fs_rlck *fp;
	reclock *insrtp;
	msg_entry *msgp;
	remote_result *resp;

	if (debug)
		printf("enter local_granted\n");
	a->rel = 1;
	nl = search_block_lock(a);
	if (nl != NULL ) { /* nl still in wait_queue, reply to kernel*/
		if ( choice == MSG) { /* create reply */
			if (blocked(&fp, &insrtp, nl) != NULL) {
				fprintf(stderr, "rpc.lockd: lock_granted msg discarded due to lock tbl inconsistent (unlock reply may be lost!)\n");
				return(NULL);
			}
			if ((msgp = retransmitted(a, KLM_LOCK)) != NULL) { /*find */

					if ((resp = (remote_result *) xmalloc(res_len)) != NULL) {
						bzero((char *) resp, res_len);
						resp->lstat = granted;
						add_reply(msgp, resp);
					}
					else { /* malloc error! */
						nlm_resp->lstat = denied;
						fprintf(stderr, "rpc.lockd: msg(%x) is deleted from msg queue because reply cannot be created due to malloc prob\n", msgp->req);
						dequeue(msgp);
					}
			}
			else { /* msg no longer in msg_queue */
				nlm_resp->lstat = denied;
				printf("msg no longer in msg_queue\n");
				}
		}
		else { /*rpc */
			printf("local granted: rpc not supported\n");
			return(NULL);
		}
		if (add_reclock(fp, insrtp, nl) == -1)
			fprintf(stderr, "rpc.lockd: no locks available\n");
		remove_wait(nl);
		nlm_resp->lstat = granted;	/* what else can it be */
	}
	else { 	/* nl no longer in wait_queue */
		nlm_resp->lstat = denied;
		printf("msg no longer in wait_queue, this may be a retransmitted msg\n");
	}
	return(nlm_resp);
}


remote_result *
cont_lock(a, resp)
	reclock *a;
	remote_result *resp;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if (debug)
		printf("enter cont_lock\n");
	switch(resp->lstat) {
	case granted:
		if ((blocked(&fp, &insrtp, a)) == 1) { /* redundant, no way to avoid */
			fprintf(stderr, "rpc.lockd: cont_lock: remote and local lock tbl inconsistent\n");
			release_res(resp);	/* discard this response */
			return(NULL);
		}
		else {
			if (add_reclock(fp, insrtp, a) == -1) {
				/* if this ever happen, there is a serious prob of how to back out the remote lock */
					resp->lstat = nolocks;
					a->rel = 1;
					return(resp);
				}
				else
					return(resp);
		}

		case denied:
			a->rel = 1;
			return(resp);
		case nolocks:
			a->rel = 1;
			return(resp);
		case blocking:
			add_wait(a);
			return(resp);
		case grace:
			release_res(resp);
			return(NULL);
		default:
			release_res(resp);
			printf("unknown lock return: %d\n", resp->lstat);
			return(NULL);
			}
}
			

remote_result *
cont_unlock(a, resp)
	reclock *a;
	remote_result *resp;
{
	struct fs_rlck *fp;
	reclock *insrtp;

	if (debug)
		printf("enter cont_unlock\n");
	insrtp = NULL;
	a->rel = 1;
	switch(resp->lstat) {
		case granted:
			if ((fp = find_fe(a)) != NULL)
				if (delete_reclock(fp, &insrtp, a) == -1)
					fprintf(stderr, "rpc.lockd: can't find lock in table.\n");
			return(resp);
		case denied:		/* impossible */
		case nolocks:
		case blocking:		/* impossible */
			return(resp);
		case grace:
			a->rel = 0;		/* keep this lock*/
			release_res(resp);
			return(NULL);
		default:
			a->rel = 0;		/* discard the response */
			release_res(resp);
			fprintf(stderr, "rpc.lockd: unkown rpc_unlock return: %d\n", resp->lstat);
			return(NULL);
		}
}

remote_result *
cont_test(a, resp)
	reclock *a;
	remote_result *resp;
{
	if (debug)
		printf("enter cont_test\n");

	a->rel = 1;
	switch (resp->lstat) {
	case grace:
		a->rel = 0;	/* keep this msg */
		release_res(resp);
		return(NULL);
	case denied:
		if (debug)
			printf("lock blocked by %d, (%d, %d)\n",
			resp->lholder.svid, resp->lholder.l_offset, resp->lholder.l_len );
		return(resp);
	case granted:
	case nolocks:
	case blocking:
		return(resp);
	default:
		fprintf(stderr, "rpc.lockd: cont_test: unknown return: %d\n", resp->lstat);
		release_res(resp);
		return(NULL);
	}
}

remote_result *
cont_cancel(a, resp)
	reclock *a;
	remote_result *resp;
{
	reclock *nl;
	reclock *insrtp;
	struct fs_rlck *fp;
	msg_entry *msgp;

	if (debug)
		printf("enter cont_cancel\n");

	a->rel = 1;
	switch(resp->lstat) {
	case granted:			
		if (search_lock(a) == NULL) { /* lock is not found */
			if (debug)
				printf("cont_cancel: msg must be removed from msg queue due to remote_cancel, now has to be put back\n");

			if ((blocked(&fp, &insrtp, a)) == 1) { /* redundant, no way to avoid */
				fprintf(stderr, "rpc.lockd: cont_cancel: remote and local lock tbl inconsistent\n");
				release_res(resp);	/* discard this response */
				return(NULL);
			}
			else {
				if (add_reclock(fp, insrtp, a) == -1)
					fprintf(stderr, "rpc.lockd: no locks available\n");
				a->rel = 0;
				return(resp);
			}
		}
		return(resp);
	case blocking:		/* should not happen */
	case nolocks:
		return(resp);
	case grace:
		a->rel = 0;
		release_res(resp);
		return(NULL);
	case denied:
		if ((nl = search_lock(a)) != NULL && nl->w_flag == 1) {
			(void) remove_wait(nl);
			nl->rel = 1;
			if ((msgp = retransmitted(nl, KLM_LOCK)) != NULL) {
				if (debug)
					printf("cont_cancel: dequeue(%x)\n", msgp->req);
				dequeue(msgp);
			}
			else {
				fprintf(stderr, "rpc.lockd: cont_cancel: cannot find blocked lock request in msg queue! \n");
				release_le(nl);
			}
			return(resp);
		}
		else if ( nl!= NULL && nl->w_flag == 0) { /* remote and local lock tbl inconsistent */
			printf("remote and local lock tbl inconsistent\n");
			release_res(resp);		/* discard this msg*/
			return(NULL);
		}
		else {
			return(resp);
		}
	default:
		fprintf(stderr, "rpc.lockd: unexpected remote_cancel %d\n", resp->lstat);
		release_res(resp);
		return(NULL);
	}		/* end of switch */
}

remote_result *
cont_reclaim(a, resp)
	reclock *a;
	remote_result *resp;
{
	remote_result *local;
	struct msg_entry *msgp;
	struct fs_rlck *fp;
	reclock *insrtp;

	if (debug)
		printf("enter cont_reclaim\n");
	switch(resp->lstat) {
	case granted:
		if (a->reclaim) {
			if (debug)
				printf("reclaim request(%x) is granted\n", a);
			if ((msgp = retransmitted(a, NLM_LOCK_RECLAIM)) != NULL)
				dequeue(msgp);
			local = NULL;
		}
		else { /* reclaim block request is granted */
			if (debug)
				printf("reclaim block request (%x) is granted!!!\n", a);
			/* use the same steps as in local_granted instead of using cont_lock */
			if (a->w_flag == 1) {
				if ((blocked(&fp, &insrtp, a)) == 1) /* blocked */
					fprintf(stderr, "rpc.lockd: cont_reclaim: blocked\n");
				if (add_reclock(fp, insrtp, a) == -1)
					fprintf(stderr, "rpc.lockd: no locks available\n");
				remove_wait(a);
			}
			else {
				fprintf(stderr, "rpc.lockd: reclaim blocked request already granted, impossible\n");
			}
			local =  resp;
		}
		break;
	case denied:
	case nolocks:
		if (a->reclaim) {
			fprintf(stderr, "rpc.lockd: reclaim lock(%x) fail!\n", a);
			kill_process(a);
			local = NULL;
		}
		else {
			if (debug)
				printf("reclaim block lock fail due to(%x)\n", resp->lstat);
			if (a->w_flag == 1) {
				remove_wait(a);
				a->rel = 1;
			}
			else {
				fprintf(stderr, "rpc.lockd: reclaim block lock has been granted with reclaim rejected as nolocks\n");
			}
			local = resp;
		}
		break;
	case blocking:
		if (a->reclaim) {
			fprintf(stderr, "rpc.lockd: reclaim lock(%x) fail!\n", a);
			kill_process(a);
			/* should cancel remote_add_wait! */
			local = NULL;
		}
		else {
			if (a->w_flag == 0)
				fprintf(stderr, "rpc.lockd: blocked reclaim request already granted locally, impossible\n");
			local = resp;
		}
		break;
	case grace:
		if (a->reclaim)
			fprintf(stderr, "rpc.lockd: reclaim lock req(%x) is returned due to grace period, impossible\n", a);
		local = NULL;
		break;
	default:
		printf("unknown cont_reclaim return: %d\n", resp->lstat);
		local = NULL;
		break;
	}

	if (local == NULL)
		release_res(resp);
	return(local);
}

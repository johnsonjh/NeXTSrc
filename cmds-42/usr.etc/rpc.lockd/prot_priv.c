#ifndef lint
static char sccsid[] = "@(#)prot_priv.c	1.1 88/08/05 NFSSRC4.0 1.10 88/02/07 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

	/*
	 * consists of all private protocols for comm with
	 * status monitor to handle crash and recovery
	 */

#include <stdio.h>
#include <sys/param.h>
#include "prot_lock.h"
#include "priv_prot.h"
#include <rpcsvc/sm_inter.h>

extern int debug;
extern int pid;
extern char hostname[MAXHOSTNAMELEN];
extern int local_state;
extern struct msg_entry *retransmitted();
extern struct fs_rlck *find_fe();
void proc_priv_crash(), proc_priv_recovery();

void
priv_prog(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	char *(*Local)();
	struct status stat;
	extern bool_t xdr_status();

	switch(rqstp->rq_proc) {
	case PRIV_CRASH:
		Local = (char *(*)()) proc_priv_crash;
		break;
	case PRIV_RECOVERY:
		Local = (char *(*)()) proc_priv_recovery;
		break;
	default:
		svcerr_noproc(transp);
		return;
	}

	bzero(&stat, sizeof(struct status));
	if (!svc_getargs(transp, xdr_status, &stat)) {
		svcerr_decode(transp);
		return;
	}
	(*Local)(&stat);
	if (!svc_sendreply(transp, xdr_void, NULL)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_status, &stat)) {
		fprintf(stderr,"unable to free arguments\n");
		exit(1);
	}
}

void
proc_priv_crash(statp)
	struct status *statp;
{
	struct fs_rlck *mp, *fp;
	reclock *next, *nl;
	struct priv_struct *privp;

	privp = (struct priv_struct *) statp->priv;
	if (privp->pid != pid) {
		if (debug)
			printf("this is not for me(%d): %d\n", privp->pid, pid); 
		return;
	}
	if (debug)
		printf("enter proc_lm_crash due to %s failure\n",
		statp->mon_name);

	destroy_client_shares(statp->mon_name);

	mp = (struct fs_rlck *) privp->priv_ptr; 
	if (strcmp(statp->mon_name, mp->svr) != 0) {
		if (debug)
			printf("crashed site is not my concern(%s)\n", mp->svr);
		return;
	}
	delete_hash(statp->mon_name);
	next = mp->rlckp;
	while ((nl = next) != NULL) {
		if (debug)
			printf("...checking lock (%x) state=%d\n", nl, nl->state);
		next = next->mnt_nxt;
		if (nl->state >= statp->state) {	/* notice obsolete */
			if (debug)
				printf("... but it's status is shiny new\n");
			continue;
		}
		if (nl->w_flag == 1) {	/* lock blocked */
			if (debug)
				printf("remove blocked lock (%x)\n", nl);
			remove_wait(nl);
		}
		else {
			if (debug)
				printf("...wasn't blocked\n");
			fp = find_fe(nl);	/* return is not checked! */
			delete_le(fp, nl);
			wakeup(nl);
		}
		nl->rel = 1;
		release_le(nl);
	}
	release_fe();		/* should I move it into while loop ? */
	release_me();
}

void
proc_priv_recovery(statp)
	struct status *statp;
{
	struct fs_rlck *mp;
	reclock *next, *nl;
	struct msg_entry *msgp;
	struct priv_struct *privp;

	privp = (struct priv_struct *) statp->priv;
	if (privp->pid != pid) {
		if (debug)
			printf("this is not for me(%d): %d\n", privp->pid, pid); 
		return;
	}

	if (debug)
		printf("enter proc_lm_recovery due to %s state(%d)\n",
		statp->mon_name, statp->state);

	destroy_client_shares(statp->mon_name);

	delete_hash(statp->mon_name);
	if (!up(statp->state)) 
		return;
	if (strcmp(statp->mon_name, hostname) == 0) {
		if (debug)
			printf("I have been declared as failed!!!\n");
		/* update local status monitor number */
		local_state = statp->state;
	}

	mp = (struct fs_rlck *) privp->priv_ptr;
	if (strcmp(statp->mon_name, mp->svr) != 0) {
		if (debug)
			printf("recovered site is not my concern(%s)\n", mp->svr);
		return;
	}
	next = mp->rlckp;
	while ((nl = next) != NULL) {
		next = next->mnt_nxt;
		if (search_lock(nl) != NULL) { /* make sure the lock is not in the middle of being processed */
			if (nl-> w_flag == 0) {
				nl->reclaim = 1;
			}
			else { /**** need to fix the prob of sending reclaim test and cancel, unlock request as reclaimed blocked request !*/
				if ((msgp = retransmitted(nl, KLM_LOCK)) != NULL) {
					dequeue(msgp);
				}
				else 
					fprintf(stderr, "blocked req (%x) cannot be found in msg queue\n", nl);
			}
			if (nlm_call(NLM_LOCK_RECLAIM, nl, 0) == -1) /*rpc error */
				if (queue(nl, NLM_LOCK_RECLAIM) == NULL)
					fprintf(stderr, "reclaim requet (%x) cannot be sent and cannot be queued for resend later!\n", nl);
		}
	}
}

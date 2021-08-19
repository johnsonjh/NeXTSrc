#ifndef lint
static char sccsid[] = "@(#)prot_main.c	1.1 88/08/05 NFSSRC4.0 1.11 88/08/04 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include "prot_lock.h"
#include "priv_prot.h"
#include "prot_time.h"

#define DEBUG_ON	17
#define DEBUG_OFF	18
#define KILL_IT		19

int HASH_SIZE;
int debug;
int klm, nlm;
int report_sharing_conflicts;
XDR x;
FILE *fp;
SVCXPRT *klm_transp;			/* export klm transport handle */
SVCXPRT *nlm_transp;			/* export nlm transport handle */

extern int grace_period;
extern msg_entry *klm_msg;	
extern int lock_len, res_len;
extern remote_result res_nolock;
extern remote_result res_working;
extern remote_result res_grace;

extern reclock *get_le();
extern msg_entry *queue();
extern remote_result *get_res();
extern int xtimer();
extern void priv_prog();

extern reclock *copy_le();
extern struct fs_rlck *copy_fe();


static void
nlm_prog(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	bool_t (*xdr_Argument)(), (*xdr_Result)();
	char *(*Local)();
	extern nlm_testres *proc_nlm_test();
	extern nlm_res *proc_nlm_lock();
	extern nlm_res *proc_nlm_cancel();
	extern nlm_res *proc_nlm_unlock();
	extern nlm_res *proc_nlm_granted();
	extern void *proc_nlm_test_msg();
	extern void *proc_nlm_lock_msg();
	extern void *proc_nlm_cancel_msg();
	extern void *proc_nlm_unlock_msg();
	extern void *proc_nlm_granted_msg();
	extern void *proc_nlm_test_res();
	extern void *proc_nlm_lock_res();
	extern void *proc_nlm_cancel_res();
	extern void *proc_nlm_unlock_res();
	extern void *proc_nlm_granted_res();

	extern void *proc_nlm_share();
	extern void *proc_nlm_freeall();
	int	monitor_this_lock = 1;

	reclock *req;
	remote_result *reply;
	int oldmask;

	if (debug)
		printf("NLM_PROG+++ version %d proc %d\n",
			Rqstp->rq_vers, Rqstp->rq_proc);

	oldmask = sigblock (1 << (SIGALRM -1));
	nlm_transp = Transp;		/* export the transport handle */
	switch (Rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case DEBUG_ON:
		debug = 2;
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case DEBUG_OFF:
		debug = 0;
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case NLM_TEST:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_nlm_testres;
		Local = (char *(*)()) proc_nlm_test;
		break;

	case NLM_LOCK:
		xdr_Argument = xdr_nlm_lockargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_lock;
		break;

	case NLM_CANCEL:
		xdr_Argument = xdr_nlm_cancargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_cancel;
		break;

	case NLM_UNLOCK:
		xdr_Argument = xdr_nlm_unlockargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_unlock;
		break;

	case NLM_GRANTED:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_granted;
		break;

	case NLM_TEST_MSG:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_test_msg;
		break;

	case NLM_LOCK_MSG:
		xdr_Argument = xdr_nlm_lockargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_lock_msg;
		break;

	case NLM_CANCEL_MSG:
		xdr_Argument = xdr_nlm_cancargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_cancel_msg;
		break;

	case NLM_UNLOCK_MSG:
		xdr_Argument = xdr_nlm_unlockargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_unlock_msg;
		break;
	case NLM_GRANTED_MSG:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_granted_msg;
		break;
	case NLM_TEST_RES:
		xdr_Argument = xdr_nlm_testres;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_test_res;
		break;

	case NLM_LOCK_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_lock_res;
		break;

	case NLM_CANCEL_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_cancel_res;
		break;

	case NLM_UNLOCK_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_unlock_res;
		break;

	case NLM_GRANTED_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_granted_res;
		break;

	case NLM_SHARE:
	case NLM_UNSHARE:
		if (Rqstp->rq_vers != NLM_VERSX)  {
			svcerr_noproc(Transp);
			(void) sigsetmask(oldmask);
			return;
		}
		proc_nlm_share(Rqstp, Transp);
		(void) sigsetmask(oldmask);
		return;
 
	case NLM_NM_LOCK:
		if (Rqstp->rq_vers != NLM_VERSX)  {
			svcerr_noproc(Transp);
			(void) sigsetmask(oldmask);
			return;
		}
		Rqstp->rq_proc = NLM_LOCK; /* fake it */
		monitor_this_lock = 0;
		xdr_Argument = xdr_nlm_lockargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_lock;
		break;
 
	case NLM_FREE_ALL:
		if (Rqstp->rq_vers != NLM_VERSX)  {
			svcerr_noproc(Transp);
			(void) sigsetmask(oldmask);
			return;
		}
		proc_nlm_freeall(Rqstp, Transp);
		(void) sigsetmask(oldmask);
		return;
 
	default:
		svcerr_noproc(Transp);
		(void) sigsetmask(oldmask);
		return;
	}

	if ( Rqstp->rq_proc != NLM_LOCK_RES && Rqstp->rq_proc != NLM_CANCEL_RES
	&& Rqstp->rq_proc != NLM_UNLOCK_RES && Rqstp->rq_proc != NLM_TEST_RES
	&& Rqstp->rq_proc != NLM_GRANTED_RES) {
		/* lock request */
		if ((req = get_le()) != NULL) {
			if (!svc_getargs(Transp, xdr_Argument, req)) {
				svcerr_decode(Transp);
				(void) sigsetmask(oldmask);
				return;
			}
			if (debug == 3){
			if (fwrite(&nlm, sizeof(int), 1, fp) == 0)
				fprintf(stderr, "fwrite nlm error\n");
			if (fwrite(&Rqstp->rq_proc, sizeof(int), 1, fp) == 0)
				fprintf(stderr, "fwrite nlm_proc error\n");
			(*xdr_Argument)(&x, req);
			printf("range[%d, %d] \n", req->lck.l_offset, req->lck.l_len);
			(void) fflush(fp);
			}


			if ((map_klm_nlm(req, (int) Rqstp->rq_proc)) != -1 ) {
			/*
			 * only lock, unlockd need to preassign le;
			 * only lock needs to preassign fe;
			 */
				if (Rqstp->rq_proc == NLM_LOCK || Rqstp->rq_proc == NLM_LOCK_MSG 
				|| Rqstp->rq_proc == NLM_UNLOCK || Rqstp->rq_proc == NLM_UNLOCK_MSG) {  
				
					if ((req->pre_le = copy_le(req)) != NULL) {
						if (Rqstp->rq_proc == NLM_LOCK || Rqstp->rq_proc == NLM_LOCK_MSG) {
							if ((req->pre_fe = (char *) copy_fe(req)) == NULL)
								goto abnormal;
						}
					}
					else
						goto abnormal;
				}
			}
			else 
				goto abnormal;
			
			if (grace_period >0 && !(req->reclaim) ) {
				if (debug)
					printf("during grace period, please retry later\n");
				nlm_reply(Rqstp->rq_proc, &res_grace, req); 
				req->rel = 1;
				release_le(req);
				(void) sigsetmask(oldmask);
				return;
			}
			if ( grace_period >0 && debug)
				printf("accept reclaim request(%x)\n", req);
			if (monitor_this_lock &&
				(Rqstp->rq_proc == NLM_LOCK ||
				 Rqstp->rq_proc == NLM_LOCK_MSG))
				/* only monitor lock req */
				if (add_mon(req, 1) == -1) {
					req->rel = 1;
					release_le(req);
					fprintf(stderr, "req discard due status monitor problem\n");
					(void) sigsetmask(oldmask);
					return;
				}

			(*Local)(req);
			release_le(req);
			call_back();		/* check if req cause nlm calling klm back */
			release_me();
			release_fe();
			if (debug)
				pr_all();
		}
		else { /* malloc err, return nolock */
			nlm_reply((int) Rqstp->rq_proc, &res_nolock, req);
		}
	}
	else {
		/* msg reply */
		if ((reply = get_res()) != NULL) {
			if (!svc_getargs(Transp, xdr_Argument, reply)) {
				svcerr_decode(Transp);
				(void) sigsetmask(oldmask);
				return;
			}
			if (debug == 3){
			if (fwrite(&nlm, sizeof(int), 1, fp) == 0)
				fprintf(stderr, "fwrite nlm_reply error\n");
			if (fwrite(&Rqstp->rq_proc, sizeof(int), 1, fp) == 0)
				fprintf(stderr, "fwrite nlm_reply_proc error \n");
			(*xdr_Argument)(&x, reply);
			(void) fflush(fp);

			}
			if (debug)
				printf("msg reply(%d) to procedure(%d)\n",
				reply->lstat, Rqstp->rq_proc);
			(*Local)(reply);
			release_me();
			release_fe();
			if (debug && reply->lstat != blocking);
				pr_all();
		}
		else {/* malloc failure, do nothing */
		}
	}
	(void) sigsetmask(oldmask);
	return;

abnormal:
	/* malloc error, release allocated space and error return*/
	nlm_reply((int) Rqstp->rq_proc, &res_nolock, req);
	req->rel = 1;
	release_le(req);
	(void) sigsetmask(oldmask);
	return;
}

static void
klm_prog(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	bool_t (*xdr_Argument)(), (*xdr_Result)();
	char *(*Local)();
	extern klm_testrply *proc_klm_test();
	extern klm_stat *proc_klm_lock();
	extern klm_stat *proc_klm_cancel();
	extern klm_stat *proc_klm_unlock();

	reclock *req;
	msg_entry *msgp;
	int oldmask;

	oldmask = sigblock (1 << (SIGALRM -1));
		
	klm_transp = Transp;
	klm_msg = NULL;

	switch (Rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case DEBUG_ON:
		debug = 2;
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case DEBUG_OFF:
		debug = 0;
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case KILL_IT:
		svc_sendreply(Transp, xdr_void, NULL);
		fprintf(stderr, "rpc.lockd killed upon request\n");
		exit(2);

	case KLM_TEST:
		xdr_Argument = xdr_klm_testargs;
		xdr_Result = xdr_klm_testrply;
		Local = (char *(*)()) proc_klm_test;
		break;

	case KLM_LOCK:
		xdr_Argument = xdr_klm_lockargs;
		xdr_Result = xdr_klm_stat;
		Local = (char *(*)()) proc_klm_lock;
		break;

	case KLM_CANCEL:
		xdr_Argument = xdr_klm_lockargs;
		xdr_Result = xdr_klm_stat;
		Local = (char *(*)()) proc_klm_cancel;
		break;

	case KLM_UNLOCK:
		xdr_Argument = xdr_klm_unlockargs;
		xdr_Result = xdr_klm_stat;
		Local = (char *(*)()) proc_klm_unlock;
		break;

	default:
		svcerr_noproc(Transp);
		(void) sigsetmask(oldmask);
		return;
	}

	if ((req = get_le()) != NULL) {
		if (!svc_getargs(Transp, xdr_Argument, req)) {
			svcerr_decode(Transp);
			(void) sigsetmask(oldmask);
			return;
		}
		if (debug == 3){
			if (fwrite(&klm, sizeof(int), 1, fp) == 0)
				fprintf(stderr, "fwrite klm error\n");
			if (fwrite(&Rqstp->rq_proc, sizeof(int), 1, fp) == 0)
				fprintf(stderr, "fwrite klm_proc error\n");
			(*xdr_Argument)(&x, req);
			printf("range[%d, %d]\n", req->lck.l_offset, req->lck.l_len);
			(void) fflush(fp);
		}

		if (map_kernel_klm(req) != -1) {
			if (Rqstp->rq_proc == KLM_LOCK ||
			 Rqstp->rq_proc == KLM_UNLOCK) {
				if ((req->pre_le = copy_le(req)) != NULL) {
					if (Rqstp->rq_proc == KLM_LOCK) {
						if ((req->pre_fe =  (char *)copy_fe(req)) == NULL)
							goto abnormal;
					}
				}
				else
					goto abnormal;
			}
		}
		else
			goto abnormal;
		if (grace_period > 0 && !(req->reclaim)) {
			/* put msg in queue and delay reply, unless there is no queue space */
			if (debug)
				printf("during grace period, please retry later\n");
			if ((msgp = queue(req, (int) Rqstp->rq_proc)) == NULL)
			{
				klm_reply(Rqstp->rq_proc, &res_working);
				req->rel = 1;
				release_le(req);
				(void) sigsetmask(oldmask);
				return;
			}
			req->rel = 1;
			klm_msg = msgp;
			(void) sigsetmask(oldmask);
			return;
		}
		if (grace_period >0 && debug)
			printf("accept reclaim request\n");
		if (Rqstp->rq_proc == KLM_LOCK) /* only monitor lock req */
			if (add_mon(req, 1) == -1) {
				req->rel = 1;
				release_le(req);
				fprintf(stderr, "req discard due status monitor problem\n");
			(void) sigsetmask(oldmask);
				return;
				}


		/* local routine replies individually */
		(*Local)(req);
		release_le(req);
		call_back();
		release_me();
		release_fe();
	}
	else { /* malloc failure */
		klm_reply((int) Rqstp->rq_proc, &res_nolock);
	}
	if (debug)
		pr_all();
	(void) sigsetmask(oldmask);
	return;

abnormal:

	klm_reply((int) Rqstp->rq_proc, &res_nolock);
	req->rel = 1;
	release_le(req);
	(void) sigsetmask(oldmask);
	return;
}


main(argc, argv)
	int argc;
	char ** argv;
{
	SVCXPRT *Transp;
	int c;
	int t;
	int ppid;
	FILE *fopen();
	extern int optind;
	extern char *optarg;

	LM_GRACE = LM_GRACE_DEFAULT;
	LM_TIMEOUT = LM_TIMEOUT_DEFAULT;
	HASH_SIZE = 29;
	report_sharing_conflicts = 0;

	while ((c = getopt(argc, argv, "s:t:d:g:h:")) != EOF)
		switch(c) {
		case 's':
			report_sharing_conflicts++;
			break;
		case 't':
			(void) sscanf(optarg, "%d", &LM_TIMEOUT);
			break;
		case 'd':
			(void) sscanf(optarg, "%d", &debug);
			break;
		case 'g':
			(void) sscanf(optarg, "%d", &t);
			LM_GRACE = 1 + t/LM_TIMEOUT;
			break;
		case 'h':
			(void) sscanf(optarg, "%d", &HASH_SIZE);
			break;
		default:
			fprintf(stderr, "rpc.lockd -t[timeout] -g[grace_period] -d[debug]\n");
			return(0);
		}
	if (debug)
		printf("lm_timeout = %d secs, grace_period = %d secs, hashsize = %d\n",
		 LM_TIMEOUT,  LM_GRACE * LM_TIMEOUT, HASH_SIZE);

	if (!debug) {
		ppid = fork();
		if (ppid == -1) {
			(void) fprintf(stderr, "rpc.lockd: fork failure\n");
			(void) fflush(stderr);
			abort();
		}
		if (ppid != 0) {
			exit(0);
		}
		for (t = 0; t< 20; t++) {
			(void) close(t);
		}

		(void) open("/dev/console", 2);
		(void) open("/dev/console", 2);
		(void) open("/dev/console", 2);

		(void) setpgrp(0, 0);
	}
	else {
		setlinebuf(stderr);
		setlinebuf(stdout);
	}

	(void) signal(SIGALRM, xtimer);
	/* NLM declaration */
	pmap_unset(NLM_PROG, NLM_VERS);

	Transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (Transp == NULL) {
		fprintf(stderr,"cannot create tcp service.\n");
		exit(1);
	}
	if (!svc_register(Transp, NLM_PROG, NLM_VERS, nlm_prog, IPPROTO_TCP)) {
		fprintf(stderr,"unable to register (NLM_PROG, NLM_VERS, tcp).\n");
		exit(1);
	}

	Transp = svcudp_bufcreate(RPC_ANYSOCK, 1000, 1000);
	if (Transp == NULL) {
		fprintf(stderr,"cannot create udp service.\n");
		exit(1);
	}
	if (!svc_register(Transp, NLM_PROG, NLM_VERS, nlm_prog, IPPROTO_UDP)) {
		fprintf(stderr,"unable to register (NLM_PROG, NLM_VERS, udp).\n");
		exit(1);
	}
	if (!svcudp_enablecache(Transp, 15)) {
		fprintf(stderr,"svcudp_enablecache failed\n");
		exit(1);
	}

	/* NLM V3 declaration */
	pmap_unset(NLM_PROG, NLM_VERSX);
 
	Transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (Transp == NULL) {
		fprintf(stderr,"cannot create tcp service.\n");
		exit(1);
	}
	if (!svc_register(Transp, NLM_PROG, NLM_VERSX, nlm_prog, IPPROTO_TCP)) {
		fprintf(stderr,"unable to register (NLM_PROG, NLM_VERSX, tcp).\n");
		exit(1);
	}
 
	Transp = svcudp_bufcreate(RPC_ANYSOCK, 1000, 1000);
	if (Transp == NULL) {
		fprintf(stderr,"cannot create udp service.\n");
		exit(1);
	}
	if (!svc_register(Transp, NLM_PROG, NLM_VERSX, nlm_prog, IPPROTO_UDP)) {
		fprintf(stderr,"unable to register (NLM_PROG, NLM_VERSX, udp).\n");
		exit(1);
	}
	if (!svcudp_enablecache(Transp, 15)) {
		fprintf(stderr,"svcudp_enablecache failed\n");
		exit(1);
	}

	/* KLM declaration */
	pmap_unset(KLM_PROG, KLM_VERS);

	Transp = svcudp_bufcreate(RPC_ANYSOCK, 1000, 1000);
	if (Transp == NULL) {
		fprintf(stderr,"cannot create udp service.\n");
		exit(1);
	}
	if (!svc_register(Transp, KLM_PROG, KLM_VERS, klm_prog, IPPROTO_UDP)) {
		fprintf(stderr,"unable to register (KLM_PROG, KLM_VERS, udp).\n");
		exit(1);
	}
	if (!svcudp_enablecache(Transp, 15)) {
		fprintf(stderr,"svcudp_enablecache failed\n");
		exit(1);
	}

	Transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (Transp == NULL) {
		fprintf(stderr,"cannot create tcp service.\n");
		exit(1);
	}
	if (!svc_register(Transp, KLM_PROG, KLM_VERS, klm_prog, IPPROTO_TCP)) {
		fprintf(stderr,"unable to register (KLM_PROG, KLM_VERS, tcp).\n");
		exit(1);
	}

	/* PRIV declaration */
	pmap_unset(PRIV_PROG, PRIV_VERS);

	Transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (Transp == NULL) {
		fprintf(stderr,"cannot create tcp service.\n");
		exit(1);
	}
	if (!svc_register(Transp, PRIV_PROG, PRIV_VERS, priv_prog, IPPROTO_TCP)) {
		fprintf(stderr,"unable to register (PRIV_PROG, PRIV_VERS, tcp).\n");
		exit(1);
	}

	init();
	init_nlm_share();

	if (debug == 3) {
		printf("lockd create logfile\n");
		klm = KLM_PROG;
		nlm = NLM_PROG;
		if ((fp = fopen("logfile", "w+")) == NULL) {
			perror("logfile fopen:");
			exit(1);
		}
		xdrstdio_create(&x, fp, XDR_ENCODE);
	}
	(void) alarm(LM_TIMEOUT);
	svc_run();
	fprintf(stderr,"svc_run returned\n");
	exit(1);
	/* NOTREACHED */
}

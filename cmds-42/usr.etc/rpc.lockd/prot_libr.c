#ifndef lint
static char sccsid[] = "@(#)prot_libr.c	1.1 88/08/05 NFSSRC4.0 1.12 88/02/07 SMI";
#endif

	/*
	 * Copyright (c) 1987 by Sun Microsystems, Inc.
	 */

	/* prot_libr.c
	 * consists of routines used for initialization, mapping and debugging
	 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/param.h>
#include <signal.h>
#include "prot_lock.h"
#include "prot_time.h"

char hostname[MAXHOSTNAMELEN];		/* for generating oh */
int pid;				/* id for monitor usage */
int host_len;				/* for generating oh */
int lock_len;
int res_len;
int msg_len;
int local_state;
int grace_period;
remote_result res_nolock;
remote_result res_working;
remote_result res_grace;

int cookie;				/* monitonically increasing # */

extern int used_le;
extern int used_fe;
extern int used_me;
extern int rel_fe;
extern int rel_me;
extern struct fs_rlck *grant_q;
extern reclock *wait_q;
extern struct fs_rlck *monitor_q;
extern msg_entry *msg_q;
extern int debug;
extern int HASH_SIZE;
extern struct fs_rlck *table_fp[];
extern char *strcpy();

char *xmalloc();
reclock *get_le();
void release_le();
void release_fe();
void release_me();

init()
{

#ifdef NeXT_MOD
	(void) gethostname(hostname, MAXHOSTNAMELEN); 
	/* used to generate owner handle */
#else 
	(void) gethostname(hostname, 20); /* used to generate owner handle */
#endif NeXT_MOD
	host_len = strlen(hostname) +1;
	msg_len = sizeof (msg_entry);
	lock_len = sizeof (reclock);
	res_len = sizeof (remote_result);
	pid = getpid();			/* used to generate return id for status monitor */
	res_nolock.lstat = nolocks;
	res_working.lstat = blocking;
	res_grace.lstat = grace;
	grace_period = LM_GRACE;
	cancel_mon();
}

/*
 * map input (from kernel) to lock manager internal structure
 * returns -1 if cannot allocate memory;
 * returns 0 otherwise
 */
int
map_kernel_klm(a)
	reclock *a;
{
	/* common code shared between map_kernel_klm and map_klm_nlm */
	/* generate op */
	if (a->exclusive)
		a->lck.op = LOCK_EX;
	else
		a->lck.op = LOCK_SH;
	if (!a->block)
		a->lck.op = a->lck.op | LOCK_NB;
	/* generate upper bound */
	if (a->lck.l_len == 0)
		a->lck.ub = MAXLEN;
	else
		a->lck.ub = a->lck.l_offset + a->lck.l_len; 
	if (a->lck.l_len > MAXLEN) {
		fprintf(stderr, " len(%d) greater than max len(%d)\n",
			a->lck.l_len, MAXLEN);
		a->lck.l_len = MAXLEN;
	}

	/* generate svid holder */
	a->lck.svid = a->lck.pid;

	/* owner handle == (hostname, pid);
	 * cannot generate owner handle use obj_alloc
	 * because additioanl pid attached at the end */
	a->lck.oh_len = host_len + sizeof (int);
	if ((a->lck.oh_bytes = xmalloc(a->lck.oh_len) ) == NULL)
		return (-1);
	(void) strcpy(a->lck.oh_bytes, hostname);
	bcopy((char *) &a->lck.pid, &a->lck.oh_bytes[host_len], sizeof (int));
	/* generate cookie */
	/* cookie is generated from monitonically increasing # */
	cookie++;
	if (obj_alloc(&a->cookie, (char *) &cookie, sizeof (int))== -1)
		return (-1);


	/* generate clnt_name */
	if ((a->lck.clnt= xmalloc(host_len)) == NULL)
		return (-1);
	(void) strcpy(a->lck.clnt, hostname);
	a->lck.caller_name = a->lck.clnt; 	/* ptr to same area */
	return (0);
}

/*
 * nlm map input from klm to lock manager internal structure
 * return -1, if cannot allocate memory!
 * returns 0, otherwise
 */ 
int
map_klm_nlm(a, choice)
	reclock *a;
	int choice;
{

	/* common code shared between map_kernel_klm and map_klm_nlm */
	if (choice == NLM_GRANTED || choice == NLM_GRANTED_MSG) 
		a->block = 1;	/* only blocked req will cause call back */
	/* generate op */
	if (a->exclusive)
		a->lck.op = LOCK_EX;
	else
		a->lck.op = LOCK_SH;
	if (!a->block)
		a->lck.op = a->lck.op | LOCK_NB;
	/* generate upper bound */
	if (a->lck.l_len == 0)
		a->lck.ub = MAXLEN;
	else
		a->lck.ub = a->lck.l_offset + a->lck.l_len; 

	if (choice == NLM_GRANTED || choice == NLM_GRANTED_MSG) {
 		/* nlm call back */
		if ((a->lck.clnt= xmalloc(host_len)) == NULL)
			return (-1);
		(void) strcpy(a->lck.clnt, hostname);
		a->lck.svr = a->lck.caller_name;
	}
	else {
 		/* normal klm to nlm calls */
		if ((a->lck.svr = xmalloc(host_len)) == NULL) {
			return (-1);
		}
		(void) strcpy(a->lck.svr, hostname);
		a->lck.clnt = a->lck.caller_name;
	}
	return (0);
}

pr_oh(a)
	netobj *a;
{
	int i;
	int j;
	unsigned p = 0;

	if (a->n_len - sizeof (int) > 4 )
		j = 4;
	else
		j = a->n_len - sizeof (int);

	/* only print out part of oh */
	for (i = 0; i< j; i++) {
		printf("%c", a->n_bytes[i]);
	}
	for (i = a->n_len - sizeof (int); i< a->n_len ; i++) {
		p = (p << 8) | (((unsigned)a->n_bytes[i]) & 0xff);
	}
	printf("%u", p);
}

pr_fh(a)
	netobj *a;
{
	int i;

	for (i = 0; i< a->n_len; i++) {
		printf("%02x", (a->n_bytes[i] & 0xff));
	}
}


pr_lock(a)
	reclock *a;
{

	printf("(%x), oh= ", a);
	pr_oh(&a->lck.oh);
	printf(", svr= %s, fh = ", a->lck.svr);
	pr_fh(&a->lck.fh);
	printf(", op=%d, ranges= [%d, %d)\n",
 		a->lck.op,
		a->lck.l_offset, a->lck.ub);
}
 
pr_all()
{
	struct fs_rlck *fp;
	reclock *nl;
	msg_entry *msgp;
	int i;

	if (debug < 2) 
		return;
	/* print grant_q */
	printf("***** granted reclocks *****\n");
	for (i = 0; i< HASH_SIZE; i++) {
		if ((fp = table_fp[i]) != NULL) {
			while (fp != NULL) {
				nl = fp->rlckp;
				while (nl != NULL) {
					pr_lock(nl);
					nl = nl->nxt;
				}
				fp = fp->nxt;
			}
		}
	}
	/* print msg queue */
	if (msg_q != NULL) {
		printf("***** msg queue *****\n");
		msgp= msg_q;
		while (msgp != NULL) {
			printf(" (%x, ", msgp->req);
			if (msgp->reply != NULL)
				printf(" lstat =%d),", msgp->reply->lstat);
			else 
				printf(" NULL),");
			msgp = msgp->nxt;
		}
		printf("\n");
	}
	else 
		printf("*****no entry in msg queue *****\n");


	/* print wait_q */
	if (wait_q != NULL) {
		printf("***** blocked reclocks *****\n");
		nl = wait_q;
		while ( nl != NULL) {
			pr_lock(nl);
			nl = nl->wait_nxt;
		}
	}
	else 
		printf("***** no blocked reclocks ****\n");

	/* print monitor_q */
	/*
	fp = monitor_q;
	while (fp != NULL) {
		printf("***** monitor queue on (%s, %d) *****\n",
			fp->svr, fp->fs.procedure);
		nl = fp->rlckp;
		while (nl != NULL) {
			pr_lock(nl);
			nl = nl->mnt_nxt;
		}
		fp = fp->nxt;
	}
	*/
	printf("used_le=%d, used_fe=%d, used_me=%d\n", used_le, used_fe, used_me);

	(void) fflush(stdout);
}

up(x)
	int x;
{
	return ((x % 2 == 1) || (x %2 == -1));
}

kill_process(a)
	reclock *a;
{
	fprintf(stderr, "kill process (%d)\n", a->lck.pid);
	(void) kill(a->lck.pid, SIGLOST);
}

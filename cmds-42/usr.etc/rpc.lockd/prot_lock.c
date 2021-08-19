#ifndef lint
static char sccsid[] = "@(#)prot_lock.c	1.1 88/08/05 NFSSRC4.0 1.18 88/02/07 Copyr 1984 Sun Micro";
#endif

	/*
	 * Copyright (c) 1984 by Sun Microsystems, Inc.
	 */

	/*
	 * prot_lock.c consists of low level routines that
	 * manipulates lock entries;
	 * place where real locking codes reside;
	 * it is (in most cases) independent of network code
	 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/param.h>
#include "prot_lock.h"
#include "priv_prot.h"
#include <rpcsvc/sm_inter.h>
#include "sm_res.h"
#define same_proc(x, y) (obj_cmp(&x->lck.oh, &y->lck.oh))

static struct priv_struct priv;

#define lb		l_offset

reclock 	*wait_q;		/* ptr to wait queue*/
reclock 	*call_q;		/* ptr to call back queue */

struct fs_rlck *rel_fe;			/* delayed fe release */
struct fs_rlck *rel_me;			/* delayed me release */

extern int pid;				/* used by status monitor*/
extern char hostname[MAXHOSTNAMELEN];	/* used by remote_data() */
extern int debug;
extern char 		*xmalloc();
extern int local_state;
extern int used_me;

extern msg_entry *retransmitted();
extern struct stat_res *stat_mon();

int		blocked();
int		add_reclock();
int 		delete_reclock();
int		cancel();
int		obj_alloc();
int		obj_copy();
int 		contact_monitor();

void		add_wait();
void		remove_wait();
void		wakeup();
void		find_insert();
void		adj_len();

void		insert_fe();
void		insert_me();
void		insert_le();
void		delete_le();
void		insert_mp();
void		delete_mp();

struct fs_rlck	*find_fe();
struct fs_rlck	*find_me();
struct fs_rlck	*get_fe();
struct fs_rlck  *copy_fe();
struct fs_rlck	*get_me();

reclock		*get_le();
reclock 	*copy_le();
reclock 	*search_lock();
reclock 	*search_block_lock();

bool_t		inside();
bool_t		overlap();
bool_t		same_op();
bool_t		same_bound();
bool_t		same_lock();
bool_t		obj_cmp();
bool_t		remote_data();
bool_t		remote_clnt();


/* blocked checks whether a new lock (a) will be blocked by 
 * any previously granted lock (owned by another process).
 * 
 * fp is set to point to struct fs_rlckp that points to the list
 * of granted reclock on the same file system.
 * all reclocks are in [lb, ub) in increasing lb order.
 * Blocked returns NULL if "a" is not blocked; 
 * insrtp ptr to the lock where new lock should be added;
 *
 * Blocked returns 1; if "a" is blocked; rlckp ptr to the first
 * blocking lock;
 * 
 */

blocked(fp, insrtp, a)
	struct fs_rlck **fp;
	reclock **insrtp;
	reclock *a;
{
	reclock *nl;

	if ((*fp = find_fe(a)) == NULL) { /*cannot find fe*/
		*insrtp = NULL;
		return(NULL);
	} else {	/* fp is found*/
		*insrtp = NULL;
		nl = (*fp) -> rlckp;

		/*set up initial insrtp value*/
		while (nl != NULL && nl->lck.lb <= a->lck.lb) {
			*insrtp = nl;
			if (same_proc(nl, a) && a->lck.lb <= nl->lck.ub)
				/* identify an overlapped lock owned by same process */
				break;
			if (!same_proc(nl,a) && a->lck.lb < nl->lck.ub 
			&& (nl->lck.op & LOCK_EX
			 || a->lck.op & LOCK_EX)) 
				/* blocked */
				return(1);

			nl = nl->nxt;
		}

		while (nl != NULL && nl->lck.lb < a->lck.ub) {
			if (!same_proc(nl,a) && a->lck.lb < nl->lck.ub 
			&& (nl->lck.op & LOCK_EX 
			|| a->lck.op & LOCK_EX)) {
			 /*blocked*/
				*insrtp = nl;
				return(1);
			}
			nl = nl->nxt;
		}
		return(NULL);
	}

}

/* add_reclock modifies existing reclock list ptr to by fp->rlckp and
 * add new lock requets "a" starting from position ptr to by
 * insrtp;
 * add_reclock returns -1; if no more fe or le entry is available
 */

add_reclock(fp, insrtp, a)
	struct fs_rlck *fp;
	reclock *insrtp;
	reclock *a;
{
	reclock *nl;
	int ind;

	if (fp == NULL)  { 	/*create new fe entry*/
		if ((fp = (struct fs_rlck *) a->pre_fe) == NULL) {
			fprintf(stderr, "(%x)pre_fe == NULL\n", a);
				abort();
		}
		a->pre_fe = NULL;
		insert_fe(fp);
		insert_le(fp, insrtp, a);		/*simplest case*/
		if (a->pre_le != NULL) {
			free_le(a->pre_le);
			a->pre_le = NULL;
		}
		return(0);
	}
	else {
		if (a->pre_fe != NULL) {
			free_fe(a->pre_fe);
			a->pre_fe = NULL;
		}
		if ( insrtp != NULL && same_proc(insrtp, a) && same_op(insrtp, a) && inside(a, insrtp)) { /* lock exists */
			if (insrtp == a) { /* extra protection */
				fprintf(stderr, "add_reclock: insrtp = a = %x should not happen!!!\n", a);
			}
			else {
				a->rel = 1;
			}
			if (a->pre_le != NULL) {
				free_le(a->pre_le);
				a->pre_le = NULL;
			}
			return(0);
		}
		/* delete all reclock owned by the same process from *insrtp */
		ind = delete_reclock(fp, &insrtp, a);
		if (a->pre_le != NULL) {
			free_le(a->pre_le);
			a->pre_le = NULL;
		}

		/* check to see if a'lower bound is connected with *insrtp */
		if (insrtp != NULL) {
			nl = insrtp;
			if (same_proc(nl, a) && same_op(nl, a) &&
				nl->lck.ub == a->lck.lb){
				a->lck.lb = nl->lck.lb;
				adj_len(a);
				insrtp = insrtp->prev;
				delete_le(fp, nl);
				nl->rel = 1;
				release_le(nl);
			}
		}
		/* check to see if a's upper bound is connected to another lock */
		nl = fp->rlckp;
		while (nl != NULL && nl->lck.lb <= a->lck.ub) {
			if (same_proc(nl, a) && same_op(nl, a) &&
			nl->lck.lb == a->lck.ub) { /*entend*/
				a->lck.ub = nl->lck.ub;
				adj_len(a);
				delete_le(fp, nl);
				nl->rel = 1;
				release_le(nl);
				break;
			}
			else
				nl = nl->nxt;
		}
		insert_le(fp, insrtp, a);
		if (!remote_data(a) && a->lck.op & LOCK_SH && ind == 2) /*lock_ex => lock_sh*/
			wakeup(a);
		return(0);

	}
}

/* 
 * delete_reclock delete locks in the reclock list ptr to by fp->rlckp
 * (starting from insrtp position)
 * that are owned by the same process as "a" and in the
 * ranges specified by "a".
 * if the lock ptr to by *insrtp has been deleted; insrtp is modified
 * to ptr to the lock before; this parameter is used for add_reclock only
 *
 * delete_reclock returns -1 if deletion requires new le and no more le  
 * is available;
 * delete_reclock returns 0 if no lock is deleted;
 * delete_reclock returns 1 if only shared locks have been deleted;
 * delele_reclock returns 2 if some exclusive locks have been deleted;
 * this return value is used to determine if add_reclock cause downgrade 
 * an exclusive and should call wakeup() or if remote lock manager
 * needs to be contacted for deletion. 
 */

delete_reclock(fp, insrtp, a)
	struct fs_rlck *fp;
	reclock **insrtp;
	reclock *a;
{
	reclock *nl;
	reclock *next;
	int lock_ex, lock_sh;
	reclock *new;

	lock_ex = 0;
	lock_sh = 0;
	if ( *insrtp == NULL)
		next = fp->rlckp;
	else
		next = *insrtp;

	while ((nl = next) != NULL && nl->lck.lb < a->lck.ub) {

		/*nl->nxt may change; has to assign next value here*/
		next = nl ->nxt;
		if (same_proc(nl, a) && nl->lck.ub > a->lck.lb) { /*overlap*/
			if (nl->lck.op & LOCK_EX)
				lock_ex ++;
			else
				lock_sh ++;
			if (inside(nl, a)) { /*delete complete*/
				if ( nl == *insrtp)
					*insrtp = (*insrtp)->prev;
				delete_le(fp, nl);
				nl->rel = 1;
				release_le(nl);	/*return to free list*/
			}
			else if ( nl->lck.ub > a->lck.ub &&
				nl->lck.lb < a->lck.lb) { /*break into half*/
				if ((new = a->pre_le) == NULL) { 
				 /* no more lock entry */
					fprintf(stderr, "(%x) pre_le is NULL\n", a);
					abort();
				}
				else {
					a->pre_le = NULL;
			 /* no need to test return value here,
			 since add_mon returns -1 only with "new" mp addition */
					if (add_mon(new, 1) == -1)
						fprintf(stderr, "add_mon = -1 in delete, should not happen\n");
					new->lck.ub = nl->lck.ub;
					nl->lck.ub = a->lck.lb;
					adj_len(nl);
					new->lck.lb = a->lck.ub;
					new->lck.op = nl->lck.op;
					new->block = nl->block;
					new->exclusive = nl->exclusive;
					adj_len(new);
					find_insert(fp, new);
				}
			}
			else if (a->lck.lb > nl->lck.lb) { /* first half remains */
				nl->lck.ub = a->lck.lb;
				adj_len(nl);
			}
			else if (a->lck.ub < nl->lck.ub) { /*second half remains */
				nl->lck.lb = a->lck.ub;
				adj_len(nl);
				if (nl == *insrtp)
					*insrtp = (*insrtp)->prev;
				delete_le(fp, nl);
				find_insert(fp, nl);
				}
			else 
			    	printf("impossible!\n");
		}
	}
	if (lock_ex > 0) 		/* some exclusive lock has been deleted */
		return(2);
	else if (lock_sh > 0 )		/* some shared lock has been deleted */
		return(1);
	else				/* no deletion*/
		return(0);
}

/*
 * cancel returns 0, if lock is cancelled:
 * either not found or remove from wait_q;
 * cancel return -1; if lock is already granted;
 */
int
cancel(a)
	reclock *a;
{
	reclock *nl;

	if ((nl = search_lock(a)) == NULL)
		/* lock not found */
		return(0);
	else {
		if (nl->w_flag == 0) 	/* lock already granted */
			return(-1);
		else {
			remove_wait(nl);
			nl->rel = 1;
			release_le(nl);
			return(0);
		}
	}
}


/*
 * search_lock locates an identical lock as a in either grant_q or wait_q
 * search_lock returns NULL if not found;
 */
reclock *
search_lock(a)
	reclock *a;
{
	struct fs_rlck *fp;
	reclock *nl;

	if ( blocked(&fp, &nl, a) == NULL) {	/* not blocked */
		if (nl != NULL && same_proc(nl, a) &&
		 same_bound(nl, a) && same_op(nl, a))
			return(nl);		/* found in grant_q */
	}
	/* search in wait_q */
	return(search_block_lock(a));
}

/*
 * return nl if nl in wait queue matches a;
 * return NULL if not found
 */
reclock *
search_block_lock(a)
	reclock *a;
{
	reclock *nl;

	nl = wait_q;
	while (nl != NULL) {
		if (same_lock(nl, a))
			return(nl);
		else
			nl = nl->wait_nxt;
	}
	return(NULL);
}

/*
 * add wait adds a to the end of wait queue wait_q;
 */
void
add_wait(a)
	reclock *a;
{
	reclock *nl, *next;

	a->w_flag = 1;			/*set wait_flag */
	if (a->pre_le != NULL) {
		free_le(a->pre_le);
		a->pre_le = NULL;
	}
	if ((nl = wait_q) == NULL) {
		wait_q = a;
		return;
	}
	else {
		while (nl != NULL) {
			if (same_lock(nl, a)) {
				if (debug)
					printf("same blocking lock already exists\n");
				a->rel = 1;
				return;
			}
			next = nl;
			nl = nl->wait_nxt;
		}
		next->wait_nxt = a;
		a->wait_prev = next;
	}
}

void
remove_wait(a)
	reclock *a;
{
	a->w_flag = 0;			/* remove wait flag */
	if (a->wait_prev == NULL)
		wait_q = a->wait_nxt;
	else
		a->wait_prev->wait_nxt = a->wait_nxt;
	if (a->wait_nxt != NULL)
		a->wait_nxt->wait_prev =a->wait_prev;
}

/*
 * wakeup searches wait_queue to wake up lock that is
 * waiting for area [a->lb, a->ub)
 * wakeup is called when a delete is successful or when an
 * exclusive lock is downgraded to a shared lock.
 */
void
wakeup(a)
	reclock *a;
{
	reclock *nl;
	struct fs_rlck *fp;
	reclock *insrtp;
	msg_entry *msgp;

	if ((nl = wait_q) == NULL)
		return;
	else
		while (nl != NULL) {
			if (overlap(nl, a) &&
			 blocked(&fp, &insrtp, nl) == 0) {
				if (remote_clnt(nl)) {
					if (add_call(nl) != -1){ 
						if (add_reclock(fp, insrtp, nl) == -1)
							fprintf(stderr, "no lock available\n");
						remove_wait(nl);
					}
					else {
						fprintf(stderr, "wakeup(%x) cannot take place due to add_call malloc error\n", nl);
					}
				}
				else {/* local clnt, check msg queue */
					if (add_reclock(fp, insrtp, nl) == -1)
						fprintf(stderr, "no lock available\n");
					remove_wait(nl);
					if ((msgp = retransmitted(nl, KLM_LOCK)) != NULL) 
						dequeue(msgp);
				}
			}
			nl = nl->wait_nxt;
		}
	return;
}

/* 
 * add to the list of call backs to klm(call_q is not doubled linked!)
 * this is necc because of original nl may be altered while the call
 * to klm needs to be queued for retransmission
 * add_call returns -1 upon error returns,
 * otherwise if returns 0
 */
int
add_call(nl)
	reclock *nl;
{
	reclock *new ;

	if ((new = copy_le(nl)) == NULL)
		return(-1);
	new->rel = 1;
	new->nxt = call_q;
	call_q = new;
	return(0);
}


/*
 * find "a" in the proper order in the fp list and insert;
 * does not worry about overlap or merge with lock from same process;
 * it all should have been taken care of before find_insert is called;
 */
void
find_insert(fp, a)
	struct fs_rlck *fp;
	reclock *a;
{
	reclock *nl;
	reclock *insrtp;
	
	nl = fp->rlckp;
	insrtp = NULL;
	while (nl != NULL && nl->lck.lb < a->lck.ub) {
		insrtp = nl;
		if (nl->lck.lb < a->lck.lb)
			nl = nl->nxt;
		else
			break;
	}
	insert_le(fp, insrtp, a);
	return;
}




/*
 * insert adds a lock entry "a" to the list ptr to by fp->rlckp
 * starting at position ptr to by insrtp
 */
void
insert_le(fp, insrtp, a)
	struct fs_rlck *fp;
	reclock *insrtp;
	reclock *a;
{
	if (insrtp == NULL) {
		a->nxt = fp->rlckp;
		if (a->nxt != NULL)
			a->nxt->prev = a;
		fp->rlckp = a;
	}
	else {
		a->nxt = insrtp->nxt;
		if (insrtp->nxt != NULL)
			insrtp->nxt->prev = a;
		insrtp->nxt = a;
	}
	a->prev = insrtp;
	return;
}

/*
 * delete a lock entry "a" from rlck list ptr to by fp->rlckp;
 * fp can be deleted and returned to the free list if no more reclock
 * on the same file system exists
 */
void
delete_le(fp, a)
	struct fs_rlck *fp;
	reclock *a;
{
	if (a->prev != NULL)
		a->prev->nxt = a->nxt;
	else {
		fp->rlckp = a->nxt;
		if (a->nxt == NULL)
	/* prepare to be released if no lock is added to fp */
			rel_fe = fp;
	}
	if (a->nxt != NULL)
		a->nxt->prev =a->prev;
}

/*
 * copy_fe allocates a new fe entry *fp and copies a into fp;
 * it returns *fp if succeeds,
 * otherwise it returns NULL
 */
struct fs_rlck *
copy_fe(a)
	reclock *a;
{
	struct fs_rlck *fp;

	if ((fp = get_fe()) == NULL) {	/*no more fe entry*/
		fprintf(stderr, "get_fe: out of fs_rlck entry\n");
		return(NULL); 
	}
	else {	/*add new fe entry*/
		if ((fp->svr = xmalloc(strlen(a->lck.svr)+1)) == NULL) {
			free_fe(fp);
			return(NULL);
		}
		(void) strcpy(fp->svr, a->lck.svr);
		/* copy fh structure: use malloc */
		if (obj_alloc(&fp->fs.fh, a->lck.fh_bytes, a->lck.fh_len) == -1) {
			free_fe(fp);
			return(NULL);
		}
		return(fp);
	}
}

/*
 * copy_le allocates a new le entry a and copies b into a;
 * it returns *a if succeeds,
 * otherwise returns NULL
 */
reclock *
copy_le(b)
	reclock *b;
{
	reclock *a;

	if (( a= get_le()) == NULL)
		return(NULL);
	if ((a->lck.svr = xmalloc(strlen(b->lck.svr)+1)) == NULL) {
		free_le(a);
		return(NULL);
	}
	(void) strcpy(a->lck.svr, b->lck.svr);
	if (obj_alloc(&a->lck.fh, b->lck.fh_bytes, b->lck.fh_len) == -1) {
		free_le(a);
		return(NULL);
	}
	a->lck.pid = b->lck.pid;
	a->lck.lb = b->lck.lb;
	a->lck.l_len = b->lck.l_len;

	/* callername is not copied */
	if (obj_alloc(&a->lck.oh, b->lck.oh_bytes, b->lck.oh_len) == -1) {
		free_le(a);
		return(NULL);
	}
	if (obj_alloc(&a->cookie, b->cookie_bytes, b->cookie_len) == -1) {
		free_le(a);
		return(NULL);
	}
	
	a->lck.svid = b->lck.svid;

	if ((a->lck.clnt = xmalloc(strlen(b->lck.clnt)+1)) == NULL) {
		free_le(a);
		return(NULL);
	}
	(void) strcpy(a->lck.clnt, b->lck.clnt);
	a->lck.caller_name = a->lck.clnt;
	a->lck.ub = b->lck.ub;
	a->lck.op = b->lck.op;

	a->block = b->block;
	a->exclusive = b->exclusive;
	return(a);
}



/*
 * insert a into the list chained by mnt_ptr; ptr to by mp;
 * insert_mp is similar to insert_le except that no insrtp and 
 * mnt_nxt, mnt_prev are used
 */
void
insert_mp(mp,  a)
	struct fs_rlck *mp;
	reclock *a;
{
	a->mnt_nxt = mp->rlckp;
	if (a->mnt_nxt != NULL)
		a->mnt_nxt->mnt_prev = a;
	mp->rlckp = a;
	a->mnt_prev = NULL;
	return;
}

/*
 * delete_mp remove a from the list chained by mnt_prt; ptr by mp;
 * delete_mp is similar to delete_le; except mnt_prev, mnt_nxt are used;
 * and rel_me is set for future release_me
 */
void
delete_mp(mp, a)
	struct fs_rlck *mp;
	reclock *a;
{
	if (a->mnt_prev != NULL)
		a->mnt_prev->mnt_nxt = a->mnt_nxt;
	else if (mp->rlckp == a) { /* this is necc because a may not
have any monitor chain in the case of call back */
		mp->rlckp = a->mnt_nxt;
		if (a->mnt_nxt == NULL)
		/* prepare to release mp; later release_me is called */
			rel_me = mp;
	}
	if (a->mnt_nxt != NULL)
		a->mnt_nxt->mnt_prev =a->mnt_prev;
}

bool_t
obj_cmp(a, b)
	struct netobj *a, *b;
{
	if (a->n_len != b->n_len) 
		return(FALSE);
	if (bcmp(&a->n_bytes[0], &b->n_bytes[0], a->n_len) != 0) 
		return(FALSE);
	else
		return(TRUE);
}

/*
 * duplicate b in a;
 * return -1, if malloc error;
 * returen 0, otherwise;
 */
int
obj_alloc(a, b, n)
	netobj *a;
	char *b;
	u_int n;
{
	a->n_len = n;
	if ((a->n_bytes = xmalloc(n)) == NULL) {
		return(-1);	
	}
	else
		bcopy(b, a->n_bytes, a->n_len);
	return(0);
}

/*
 * copy b into a
 * returns 0, if succeeds
 * return -1 upon error
 */
int
obj_copy(a, b)
	netobj *a, *b;
{
	if (b == NULL) {
		/* trust a is already NULL */
		if (debug)
			printf(" obj_copy(a = %x, b = NULL), a\n", a);
		return(0);
	}
	return(obj_alloc(a, b->n_bytes, b->n_len));
}

/*
 * adj_len modified the l_len field, since lb or ub has changed
 */
void
adj_len(a)
	reclock *a;
{
	if (a->lck.ub == MAXLEN)
		a->lck.l_len = 0;
	else
		a->lck.l_len = a->lck.ub - a->lck.lb;
}

/*
 * if nl is inside a, inside returns TRUE;
 */
bool_t
inside(nl,a)
	reclock *nl,*a;
{
	if (a == NULL | nl == NULL)
		return( FALSE);
	else
		return(a->lck.lb <=nl->lck.lb && a->lck.ub >=nl->lck.ub);
}

/*
 * if nl overlaps with a, overlap returns TRUE;
 */
bool_t
overlap(nl, a)
	reclock *nl, *a;
{
	if (nl->lck.ub <= a->lck.lb || nl->lck.lb >= a->lck.ub)
		return(FALSE);
	else
		return(TRUE);
}

bool_t
same_bound(a,b)
	reclock *a, *b;
{
/*
	if (a == NULL || b == NULL)
		return( FALSE);
	else
*/
		return(a->lck.lb == b ->lck.lb && a->lck.ub == b ->lck.ub);
}

bool_t
same_op(a,b)
	reclock *a, *b;
{
		return(( (a-> lck.op & LOCK_EX) && (b-> lck.op & LOCK_EX)) ||
		((a-> lck.op & LOCK_SH) && (b-> lck.op & LOCK_SH)));

}

bool_t
same_lock(a, b)
	reclock *a, *b;
{
	if (same_proc(a, b) && same_op(a, b) && same_bound(a, b))
		return(TRUE);
	else
		return(FALSE);
}

bool_t
simi_lock(a, b)
	reclock *a, *b;
{
	if (same_proc(a, b) && same_op(a, b) && inside(b, a))
		return(TRUE);
	else
		return(FALSE);
}


bool_t
remote_data(a)
	reclock *a;
{
	if (strcmp(a->lck.svr, hostname) == 0)
		return(FALSE);
	else
		return(TRUE);
}


bool_t
remote_clnt(a)
	reclock *a;
{
	if (strcmp(a->lck.clnt, hostname) == 0)
		return(FALSE);
	else
		return(TRUE);
}


/* 
 * translate monitor calls into modifying monitor chains
 * returns 0, if success
 * returns -1, in case of error
 */
int
add_mon(a,i)
	reclock *a;
	int i;
{
	if (strcmp(a->lck.svr, a->lck.clnt) == 0)
		/* local case, no need for monitoring */
		return(0);
	if (remote_data(a)) {
		if (mond(hostname, PRIV_RECOVERY, a, i) == -1)
			return(-1);
		if (mond(a->lck.svr, PRIV_RECOVERY, a, i) == -1)
			return(-1);
		}
	else
		if (mond(a->lck.clnt, PRIV_CRASH, a, i) == -1)
			return(-1);
		return(0);

}

/*
 * mond set up the monitor ptr; 
 * it return -1, if no more free mp entry is available when needed
 *		 or cannot contact status monitor
 */
int
mond(site, proc, a, i)
	char *site;
	int proc;
	reclock *a;
	int i;
{
	struct fs_rlck * new;

	if (i == 1) { /* insert! */
		if ((new = find_me(site, proc)) == NULL) { /* not found*/
			if (( new = get_me()) == NULL) /* no more me entry */
				return(-1);
			else  {		/* create a new mp */
				if ((new->svr = xmalloc(strlen(site)+1)) == NULL) {
					used_me--;
					free((char *) new);
					return(-1);
				}

				(void) strcpy((char *) new->svr, site);
				new->fs.procedure = proc;
				/* contact status monitor */
				if (contact_monitor(new, 1) == -1) {
					used_me--;
					xfree(&new->svr);
					free((char *) new);
					return(-1);
				}
				else {
					insert_me(new);
				}
			}
		}
		insert_mp(new, a);
		return(0);
	}
	else { /* i== 0; delete! */
		if ((new = find_me(site, proc)) == NULL)
			return(0);			/* happen due to call back */
		else
			delete_mp(new, a);		/* delete_mp may be no op if a is introduced due to call back */
			return(0);
	}
}


int
contact_monitor(new, i)
	struct fs_rlck *new;
	int i;
{
	struct stat_res *resp;
	int priv_size;
	int func;

	switch(i) {
	case 0:
		func = SM_UNMON;
		break;
	case 1:
		func = SM_MON;
		break;
	default:
		fprintf(stderr, "unknown contact monitor (%d)\n", i);
		abort();
	}

	priv.pid = pid;
	priv.priv_ptr = (int *) new;
	if ((priv_size = sizeof(struct priv_struct)) > 16) 	/* move to init*/
		fprintf(stderr, "contact_mon: problem with private data size (%d) to status monitor\n",
		priv_size);

	resp = stat_mon(new->svr, hostname, PRIV_PROG, PRIV_VERS,
	 new->fs.procedure, func, priv_size, &priv);
	if (resp->res_stat == stat_succ) {
		if (resp->sm_stat == stat_succ) {
			local_state = resp->sm_state;	/* update local state */
			return(0);
		}
		else {
			fprintf(stderr, "rpc.lockd: site %s does not subscribe to status monitor service \n", new->svr);
			return(-1);
		}
	}
	else {
		fprintf(stderr, "rpc.lockd cannot contact local statd\n");
		return(-1);
	}
}

#ifndef lint
static char sccsid[] = "@(#)hash.c	1.1 88/08/05 NFSSRC4.0 1.7 88/05/09 Copyr 1986 Sun Micro";
#endif

	/*
	 * Copyright (c) 1986 by Sun Microsystems, Inc.
	 */
	/* hash.c
	 * rotuines handle insertion, deletion of hashed monitor, file entries
	 */

#include "prot_lock.h"

#define MAX_HASHSIZE 100


char *malloc();
char *xmalloc();
extern int debug;
extern int HASH_SIZE;
extern struct fs_rlck *rel_fe;

typedef struct fs_rlck cache_fp;
typedef struct fs_rlck cache_me;

/* FIXME*FIXME*FIXME*FIXME*FIXME*FIXME*FIXME*FIXME*FIXME*FIXME*FIXME  
 * 
 * cmp_fh() compares file handles. It is not nice at all in that it
 * presupposes to know what a file handle looks like. Basically, this
 * is a screaming, last minute hack to keep ecd afloat, but it illustrates
 * a weakness with the lock manager implementation. Specifically, the
 * file handle was treated as a unique identifier in all of name space
 * when in reality it was something quite different.
 */

int
cmp_fh(a,b)
	netobj	*a, *b;
{
        if (a->n_len != b->n_len)
                return(FALSE);
        if (bcmp(&a->n_bytes[0], &b->n_bytes[0], 20) != 0)
                return(FALSE);
        else
                return(TRUE);
}

cache_fp *table_fp[MAX_HASHSIZE];
cache_me *table_me[MAX_HASHSIZE];

/*
 * find_fe returns the cached entry;
 * it returns NULL if not found;
 */
struct fs_rlck *
	find_fe(a)
	reclock *a;
{
	cache_fp *cp;

	cp = table_fp[hash(a->lck.fh_bytes)];
	while ( cp != NULL) {
		if (strcmp(cp->svr, a->lck.svr) == 0 &&
		cmp_fh(&cp->fs.fh, &a->lck.fh)) {
			/*found */
			return(cp);
		}
		cp = cp->nxt;
	}
	return(NULL);
}

/*
 * find_me returns the cached entry;
 * it returns NULL if not found;
 */
struct fs_rlck *
find_me(svr, proc)
	char *svr;
	int proc;
{
	cache_me *cp;

	cp = table_me[hash(svr)];
	while ( cp != NULL) {
		if (strcmp(cp->svr, svr) == 0 &&
		cp->fs.procedure == proc) {
			/*found */
			return(cp);
		}
		cp = cp->nxt;
	}
	return(NULL);
}

void
insert_fe(fp)
	struct fs_rlck *fp;
{
	int h;

	h = hash(fp->fs.fh_bytes);
	fp->nxt = table_fp[h];
	table_fp[h] = fp;
}

void
insert_me(mp)
	struct fs_rlck *mp;
{
	int h;

	h = hash(mp->svr);
	mp->nxt = table_me[h];
	table_me[h] = mp;
}

void
release_fe()
{
	cache_fp *cp, *fp;
	cache_fp *cp_prev = NULL;
	cache_fp *next;
	int h;

	if (rel_fe == NULL) 
		return;
	fp = rel_fe;
	if (fp->rlckp == NULL) {
		h = hash(fp->fs.fh_bytes);
		next = table_fp[h];
		while ((cp = next) != NULL) {
			next = cp->nxt;
			if (strcmp(cp->svr, fp->svr) == 0 &&
			cmp_fh(&cp->fs.fh, &fp->fs.fh)) {
				if (cp_prev == NULL) {
					table_fp[h] = cp->nxt;
				}
				else {
					cp_prev->nxt = cp->nxt;
				}
				free_fe(cp);
				rel_fe = NULL;
				return;
			}
			else {
				cp_prev = cp;
			}
		}
	}
}

release_me()
{
	/* we never free up monitor entry, the knowledge of contacting
	 * status monitor accumulates
	 */
}

/*
 * zap_all_locks_for(client) zips throughthe table_fp looking for
 * all fs_rlck which reference reclock's from client, and delete them.
 * this is used by prot_freeall to implement the PC-NFS cleanup...
 */
void
zap_all_locks_for(client)
	char *client;
{
	cache_fp *cp;
	cache_fp *nextcp;
	reclock *le;
	reclock *nextle;

	int i;

	if (debug)
		printf("zap_all_locks_for %s\n", client);
	for (i = 0; i< MAX_HASHSIZE; i++) {
		cp = table_fp[i];
		while (cp != NULL) {
			nextcp = cp->nxt;	/*  cp might change */
			le = cp->rlckp;
			while (le) {
				nextle = le->nxt;
				if (strcmp(le->alock.clnt_name, client) == 0) {
	if (debug)
		printf("...zapping: cp@0x%x, le@0x%x\n", cp, le);

					delete_le(cp, le);
					le->rel = 1;
					release_le(le);
				}
				le = nextle;
			}
			cp = nextcp;
		}
	}
	if (debug)
		printf("DONE zap_all_locks_for %s\n", client);
}

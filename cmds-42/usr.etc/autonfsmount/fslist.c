/*
 * getmntent() for automount entries with caching (60 second timeout)
 * 
 * Copyright (C) 1990 by NeXT, Inc.
 */
#include <stdio.h>
#include <mntent.h>
#include "fslist.h"

extern void *malloc(unsigned);
extern char *strdup(const char *);

#define MOUNTLIST_TIMEOUT 60

#ifndef MNTOPT_NET
#define MNTOPT_NET "net"
#endif

#define shouldignore(mnt) (hasmntopt(mnt, MNTOPT_NET) == NULL)


typedef struct savemnt_node *savemnt_list;

typedef struct savemnt_node {
	struct mntent mnt;
	savemnt_list next;
} savemnt_node;

static savemnt_list savedmounts;
static savemnt_list cur;
static struct timeval fslist_time;
static long deathtime;

void
freemounts(savemnt_list *mlist)
{
	savemnt_list ent;

	while (*mlist != NULL) {
		ent = *mlist;
		*mlist = (*mlist)->next;

		free(ent->mnt.mnt_fsname);
		free(ent->mnt.mnt_dir);
		free(ent->mnt.mnt_opts);
		free(ent->mnt.mnt_type);
		free(ent);
	}
}


int
mnt_eq(
       struct mntent m1,
       struct mntent m2
       )
{
	return (strcmp(m1.mnt_fsname, m2.mnt_fsname) == 0 &&
		strcmp(m1.mnt_dir, m2.mnt_dir) == 0 &&
		strcmp(m1.mnt_opts, m2.mnt_opts) == 0 &&
		strcmp(m1.mnt_type, m2.mnt_type) == 0 &&
		m1.mnt_freq == m2.mnt_freq &&
		m1.mnt_passno == m2.mnt_passno);
}

int
mntlist_eq(
	   savemnt_list m1,
	   savemnt_list m2
	   )
{
	while (m1 != NULL && m2 != NULL) {
		if (!mnt_eq(m1->mnt, m2->mnt)) {
			return (0);
		}
		m1 = m1->next;
		m2 = m2->next;
	}
	if (m1 != NULL || m2 != NULL) {
		return (0);
	}
	return (1);
}

void
savemount(struct mntent *mnt, savemnt_list *mlist)
{
	savemnt_list ent;

	ent = (savemnt_list)malloc(sizeof(*ent));
	ent->mnt.mnt_fsname = strdup(mnt->mnt_fsname);
	ent->mnt.mnt_dir = strdup(mnt->mnt_dir);
	ent->mnt.mnt_type = strdup(mnt->mnt_type);
	ent->mnt.mnt_opts = strdup(mnt->mnt_opts);
	ent->mnt.mnt_freq = mnt->mnt_freq;
	ent->mnt.mnt_passno = mnt->mnt_passno;
	ent->next = *mlist;
	*mlist = ent;
	
}

void
fslist_gettime(struct timeval *when)
{
	*when = fslist_time;
}

void
fslist_set(struct timeval *when)
{
	FILE *f;
	savemnt_list newmounts;
	struct mntent *mnt;

	if (savedmounts != NULL &&
	    time(0) < deathtime) {
		cur = savedmounts;
		*when = fslist_time;
		return;
	} 

	
	f = setmntent(NULL, "r");
	newmounts = NULL;
	while (mnt = getmntent(f)) {
		if (!shouldignore(mnt)) {
			savemount(mnt, &newmounts);
		}
	}
	endmntent(f);

	if (mntlist_eq(newmounts, savedmounts)) {
		freemounts(&newmounts);
	} else {
		freemounts(&savedmounts);
		savedmounts = newmounts;
		gettimeofday(&fslist_time, NULL);
	}
	deathtime = time(0) + MOUNTLIST_TIMEOUT;
	*when = fslist_time;
	cur = savedmounts;
}




struct mntent *
fslist_get(void)
{
	struct mntent *mnt;

	if (cur != NULL) {
		mnt = &cur->mnt;
		cur = cur->next;
	} else {
		mnt = NULL;
	}
	return (mnt);
}


void
fslist_end(void)
{
	cur = NULL;
}



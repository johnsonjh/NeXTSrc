/*
 * Stuff for managing the "fstab" type map
 * 
 * Copyright (C) 1990 by NeXT, Inc.
 */
#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "nfs_prot.h"
typedef nfs_fh fhandle_t;
#define NFSCLIENT
#define NFS                     /* for backwards compatibility */
#include <sys/mount.h>
#include <nfs/nfs_mount.h>
#include <mntent.h>
#ifndef M_NEWTYPE
#define OLDMOUNT
#endif
#include "automount.h"
#include "fslist.h"

/*
 * XXX: Because the fslist stuff is non-reentrant, we have to be careful
 * about not calling fslist_set() twice. The only time this happens is
 * in fstab_loadall(). fstab_loadall() calls lookup() which calls 
 * fstab_inlist(). We don't want to do another fslist_set(), so we just
 * detect "loading" and always return TRUE for fstab_inlist() in this case.
 * Gross...
 */
static int loading;

static char **
strlist_alloc(void)
{
	char **res = (char **)malloc(sizeof(char *));

	res[0] = NULL;
	return (res);
}

static int 
strlist_len(
	    char **strlist
	    )
{
	int i;

	for (i = 0; strlist[i] != NULL; i++) {
	}
	return (i);
}

static char **
strlist_insert(
	       char **strlist,
	       char *str
	       )
{
	int len = strlist_len(strlist);
	char **res;

	res = (char **)realloc(strlist, (len + 2) * sizeof(char *));
	res[len] = strdup(str);
	res[len+1] = NULL;
	return (res);
}

static int
strlist_match(
	      char **strlist,
	      char *str
	      )
{
	int i;

	for (i = 0; strlist[i] != NULL; i++) {
		if (strcmp(strlist[i], str) == 0) {
			return (1);
		}
	}
	return (0);
}

static void
strlist_free(
	     char **strlist
	     )
{
	int i;

	for (i = 0; strlist[i] != NULL; i++) {
		free(strlist[i]);
	}
	free(strlist);
}

static int
isautomount(
	    struct mntent *mnt,
	    char *name
	    )
{
	if (strcmp(mnt->mnt_dir, name) != 0) {
		return (0);
	}
	return (1);
}

static char *
gethostof(
	  struct mntent *mnt
	  )
{
	char *colon;
	char *str = strdup(mnt->mnt_fsname);

	colon = index(str, ':');
	if (colon == NULL) {
		free(str);
		return (NULL);
	}
	*colon = 0;
	return (str);
}

static char *
getdirof(
	  struct mntent *mnt
	  )
{
	char *colon;

	colon = index(mnt->mnt_fsname, ':');
	if (colon == NULL) {
		return (NULL);
	}
	return (strdup(colon + 1));
}

void
fstab_loadall(
	      struct autodir *dir,
	      struct timeval *when
	      )
{
	struct vnode *vp;
	char **hostlist;
	char *host;
	struct mntent *mnt;

	hostlist = strlist_alloc();
	fslist_set(when);
	loading = 1;
	while (mnt = fslist_get()) {
		if (!isautomount(mnt, dir->dir_name)) {
			continue;
		}
		host = gethostof(mnt);
		if (host != NULL) {
			if (!strlist_match(hostlist, host)) {
				(void)lookup(dir, host, &vp);
				hostlist = strlist_insert(hostlist, host);
			}
			free(host);
		}
	}
	loading = 0;
	fslist_end();
	strlist_free(hostlist);
}

int
fstab_inlist(
	     struct autodir *dir,
	     char *name
	     )
{
	char *host;
	struct mntent *mnt;
	struct timeval when;

	if (loading) {
		return (1);
	}
	fslist_set(&when);
	while (mnt = fslist_get()) {
		if (!isautomount(mnt, dir->dir_name)) {
			continue;
		}
		host = gethostof(mnt);
		if (host == NULL) {
			continue;
		}
		if (strcmp(host, name) == 0) {
			free(host);
			fslist_end();
			return (1);
		}
		free(host);
	}
	fslist_end();
	return (0);
}

static int
matchhost(
	  char *fsname,
	  char *host
	  )
{
	int hostlen;

	hostlen = strlen(host);
	return (strncmp(fsname, host, hostlen) == 0 &&
		fsname[hostlen] == ':');
}

nfsstat
fstab_mountall(
	       struct autodir *dir,
	       char *host
	       )
{
	struct groups *gr;
	enum clnt_stat clnt_stat, pingmount(), callrpc();
	char dirbuf[1024];
	int elen;
	int ok;
	struct stat stbuf;
	struct filsys *fs;
	int imadeit;
	nfsstat status;
	struct mntent *mnt;
	char *dirname;
	struct timeval when;

	/* ping the null procedure of the remote mount daemon */
	if (pingmount(host) != RPC_SUCCESS)
		return (NFSERR_NOENT);


	/* now sort by length of names - to get order right */

	ok = 0;
	status = NFSERR_NOENT;
	fslist_set(&when);
	while (mnt = fslist_get()) {
		if (!matchhost(mnt->mnt_fsname, host) ||
		    !isautomount(mnt, dir->dir_name)) {
			continue;
		}
		dirname = getdirof(mnt);
		if (dirname == NULL) {
			continue;
		}
		if (dirname[1] == '\0' && dirname[0] == '/')
			(void) sprintf(dirbuf, "%s%s/%s", tmpdir,
					dir->dir_name, host);
		else
			(void) sprintf(dirbuf, "%s%s/%s%s", tmpdir,
					dir->dir_name, host, 
				       dirname);
		imadeit = 0;
		if (stat(dirbuf, &stbuf) != 0) {
			if (mkdir_r(dirbuf) == 0)
				imadeit = 1;
			else {
				free(dirname);
				continue;
			}
		}
		fs = already_mounted(host, dirname, 
				     mnt->mnt_opts);
		if (fs && strcmp(dirbuf, fs->fs_mntpnt) == 0) {
			/* leftover from before somehow */
			fs->fs_mine = 1;
			fs->fs_hostdir = dir;
			ok++;
		}  else
			fs = NULL;	/* for nfsmount */
		if (fs == NULL && 
		    (status = nfsmount(host, 
				       dirname,
				       dirbuf,
				       mnt->mnt_opts, 
				       &fs)) == NFS_OK) {
			ok++;
			fs->fs_hostdir = dir;
		}
		free(dirname);
		if (fs == NULL && imadeit)
			safe_rmdir(dirbuf);
	}
	fslist_end();
	return (ok ? NFS_OK : status);
}

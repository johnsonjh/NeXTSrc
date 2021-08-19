#ifndef lint
static char sccsid[] = 	"@(#)auto_all.c	1.3 88/07/28 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <mntent.h>
#include <netdb.h>
#include <errno.h>
#include "nfs_prot.h"
#define NFSCLIENT
#define NFS                     /* for backwards compatibility */
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#include <sys/mount.h>
#ifdef NeXT
#include <nfs/nfs_mount.h>
#endif
#include "automount.h"

nfsstat
mountall(dir, host)
	struct autodir *dir;
	char *host;
{
	struct exports *ex = NULL;
	struct exports *exlist, *texlist, **texp, *exnext;
	struct groups *gr;
	enum clnt_stat clnt_stat, pingmount(), callrpc();
	char dirbuf[1024];
	int elen;
	int ok;
	struct stat stbuf;
	struct filsys *fs;
	int imadeit;
	nfsstat status;

	/* ping the null procedure of the remote mount daemon */
	if (pingmount(host) != RPC_SUCCESS)
		return (NFSERR_NOENT);

	/* get export list of host */
        if (clnt_stat = callrpc(host, MOUNTPROG, MOUNTVERS, MOUNTPROC_EXPORT,
	    xdr_void, 0, xdr_exports, &ex)) {
		syslog(LOG_ERR, "%s: exports: %s", host,
		    clnt_sperrno(clnt_stat));
		return (NFSERR_IO);
	}
	if (ex == NULL)
		return (NFSERR_ACCES);
	/* now sort by length of names - to get order right */
	exlist = ex;
	texlist = NULL;
	for (ex = exlist; ex; ex = exnext) {
		exnext = ex->ex_next;
		ex->ex_next = 0;
		elen = strlen(ex->ex_name);
		/*  check netgroup list  */
		if (ex->ex_groups && usingYP) {
			for ( gr = ex->ex_groups ; gr ; gr = gr->g_next) {
				if (strcmp(gr->g_name, self) == 0 ||
					innetgr(gr->g_name, self, NULL, mydomain))
					break;
			}
			if (gr == NULL) {
				freeex(ex);
				continue;
			}
		}
		for (texp = &texlist; *texp; texp = &((*texp)->ex_next))
			if (elen < strlen((*texp)->ex_name))
				break;
		ex->ex_next = *texp;
		*texp = ex;
	}
	exlist = texlist;

	ok = 0;
	status = NFSERR_NOENT;
	for (ex = exlist; ex; ex = ex->ex_next) {
		if (ex->ex_name[1] == '\0' && ex->ex_name[0] == '/')
			(void) sprintf(dirbuf, "%s%s/%s", tmpdir,
					dir->dir_name, host);
		else
			(void) sprintf(dirbuf, "%s%s/%s%s", tmpdir,
					dir->dir_name, host, ex->ex_name);
		imadeit = 0;
		if (stat(dirbuf, &stbuf) != 0) {
			if (mkdir_r(dirbuf) == 0)
				imadeit = 1;
			else
				continue;
		}
		fs = already_mounted(host, ex->ex_name, dir->dir_opts);
		if (fs && strcmp(dirbuf, fs->fs_mntpnt) == 0) {
			/* leftover from before somehow */
			fs->fs_mine = 1;
			fs->fs_hostdir = dir;
			ok++;
		}  else
			fs = NULL;	/* for nfsmount */
		if (fs == NULL && (status = nfsmount(host, ex->ex_name, dirbuf,
		    dir->dir_opts, &fs)) == NFS_OK) {
			ok++;
			fs->fs_hostdir = dir;
		}
		if (fs == NULL && imadeit)
			safe_rmdir(dirbuf);
	}
	freeex(exlist);
	return (ok ? NFS_OK : status);
}

freeall(dir, host)
	struct autodir *dir;
	char *host;
{
	struct queue tmpq;
	struct filsys *fs, *nextfs;
	nfsstat remount();

	tmpq.q_head = tmpq.q_tail = NULL;
	for (fs = HEAD(struct filsys, fs_q); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_hostdir == dir && strcmp(fs->fs_host, host) == 0) {
			REMQUE(fs_q, fs);
			INSQUE(tmpq, fs);
		}
	}
	for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_refcnt)
			goto inuse;
	}
	/* walk backwards trying to unmount */
	for (fs = TAIL(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = PREV(struct filsys, fs);
		if (unmount(fs->fs_mntpnt) < 0) {
			/* remount previous unmounted ones */
			for (fs = NEXT(struct filsys, fs); fs; fs = nextfs) {
				nextfs = NEXT(struct filsys, fs);
				(void) remount(fs);
			}
			goto inuse;
		}
		clean_mtab(fs->fs_mntpnt);
		nfsunmount(fs);
	}
	/* all ok - walk backwards removing directories */
	for (fs = TAIL(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = PREV(struct filsys, fs);
		safe_rmdir(fs->fs_mntpnt);
		REMQUE(tmpq, fs);
		INSQUE(fs_q, fs);
		free_filsys(fs);
	}
	/* success */
	return (1);
inuse:	/* put things back on the correct list */
	for (fs = HEAD(struct filsys, tmpq); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		REMQUE(tmpq, fs);
		INSQUE(fs_q, fs);
	}
	return (0);
}

freeex(ex)
	struct exports *ex;
{
	struct groups *groups, *tmpgroups;
	struct exports *tmpex;

	while (ex) {
		free(ex->ex_name);
		groups = ex->ex_groups;
		while (groups) {
			tmpgroups = groups->g_next;
			free((char *)groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free((char *)ex);
		ex = tmpex;
	}
}

mkdir_r(dir)
	char *dir;
{
	int err;
	char *slash;
	char *rindex();

	if (mkdir(dir, 0777) == 0)
		return (0);
	if (errno != ENOENT)
		return (-1);
	slash = rindex(dir, '/');
	if (slash == NULL)
		return (-1);
	*slash = '\0';
	err = mkdir_r(dir);
	*slash = '/';
	if (err)
		return (err);
	return mkdir(dir, 0777);
}

safe_rmdir(dir)
	char *dir;
{
	dev_t tmpdev;
	struct stat stbuf;

	if (stat(tmpdir, &stbuf))
		return;
	tmpdev = stbuf.st_dev;
	if (stat(dir, &stbuf))
		return;
	if (tmpdev == stbuf.st_dev)
		rmdir(dir);
}

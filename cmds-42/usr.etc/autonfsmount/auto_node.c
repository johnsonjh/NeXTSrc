#ifndef lint
static char sccsid[] = 	"@(#)auto_node.c	1.3 88/06/18 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef NeXT
#include <sys/dir.h>
#define dirent direct
#else
#include <dirent.h>
#endif
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <netinet/in.h>
#include "nfs_prot.h"
#define NFSCLIENT
#define NFS                     /* for backwards compatibility */
typedef nfs_fh fhandle_t;
#include <sys/mount.h>
#ifdef NeXT
#include <nfs/nfs_mount.h>
#endif
#include "automount.h"

struct internal_fh {
	int	fh_pid;
	long	fh_time;
	int	fh_num;
};

static int fh_cnt = 3;
static right_pid = -1;
static long right_time;

#define	FH_HASH_SIZE	8

struct queue fh_q_hash[FH_HASH_SIZE];

new_fh(vnode)
	struct vnode *vnode;
{
	long time();
	register struct internal_fh *ifh =
		(struct internal_fh *)(&vnode->vn_fh);

	if (right_pid == -1) {
		right_pid = getpid();
		(void) time(&right_time);
	}
	ifh->fh_pid = right_pid;
	ifh->fh_time = right_time;
	ifh->fh_num = ++fh_cnt;

	INSQUE(fh_q_hash[ifh->fh_num % FH_HASH_SIZE], vnode);
}

free_fh(vnode)
	struct vnode *vnode;
{
	register struct internal_fh *ifh =
		(struct internal_fh *)(&vnode->vn_fh);

	REMQUE(fh_q_hash[ifh->fh_num % FH_HASH_SIZE], vnode);
}

struct vnode *
fhtovn(fh)
	nfs_fh *fh;
{
	register struct internal_fh *ifh = 
		(struct internal_fh *)fh;
	int num;
	struct vnode *vnode;

	if (ifh->fh_pid != right_pid || ifh->fh_time != right_time)
		return ((struct vnode *)0);
	num = ifh->fh_num;
	vnode = HEAD(struct vnode, fh_q_hash[num % FH_HASH_SIZE]);
	while (vnode) {
		ifh = (struct internal_fh *)(&vnode->vn_fh);
		if (num == ifh->fh_num)
			return (vnode);
		vnode = NEXT(struct vnode, vnode);
	}
	return ((struct vnode *)0);
}

int
fileid(vnode)
	struct vnode *vnode;
{
	register struct internal_fh *ifh =
		(struct internal_fh *)(&vnode->vn_fh);

	return (ifh->fh_num);
}

dirinit(name, map, opts)
	char *name, *map, *opts;
{
	struct autodir *dir;
	register fattr *fa;
	struct stat stbuf;
	int maptype;
	int mydir = 0;

	for (dir = HEAD(struct autodir, dir_q); dir;
	    dir = NEXT(struct autodir, dir))
		if (strcmp(dir->dir_name, name) == 0)
			return;
	if (*name != '/') {
		syslog(LOG_ERR, "dir %s must start with '/'", name);
		return;
	}
	if (lstat(name, &stbuf) == 0) {
		if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
			syslog(LOG_ERR, "%s: Not a directory", name);
			return;
		}
		if (!emptydir(name))
			syslog(LOG_ERR, "WARNING: %s not empty!", name);
	} else {
		if (mkdir_r(name)) {
			syslog(LOG_ERR, "Cannot create directory %s: %m", name);
			return;
		}
		mydir = 1;
		chmod(name, 0777);
	}

	if (strcmp(map, "-hosts") == 0) {
		char tmp[MAXPATHLEN];

		maptype = MAPTYPE_HOSTS;
		(void) sprintf(tmp, "%s%s", tmpdir, name);
		if (lstat(tmp, &stbuf) ||
		    (stbuf.st_mode & S_IFMT) != S_IFDIR) {
			if (mkdir_r(tmp)) {
				syslog(LOG_ERR,
				    "Cannot create directory %s: %m", tmp);
				return;
			}
		}
	} else if (strcmp(map, "-fstab") == 0) {
		char tmp[MAXPATHLEN];

		maptype = MAPTYPE_FSTAB;
		(void) sprintf(tmp, "%s%s", tmpdir, name);
		if (lstat(tmp, &stbuf) ||
		    (stbuf.st_mode & S_IFMT) != S_IFDIR) {
			if (mkdir_r(tmp)) {
				syslog(LOG_ERR,
				    "Cannot create directory %s: %m", tmp);
				return;
			}
		}
	} else if (strcmp(map, "-passwd") == 0)
		maptype = MAPTYPE_PASSWD;
	else
		maptype = MAPTYPE_FILE;

	dir = (struct autodir *)malloc(sizeof *dir);
	if (dir == NULL)
		goto alloc_failed;
	bzero((char *)dir, sizeof *dir);
	dir->dir_name = strdup(name);
	if (dir->dir_name == NULL) {
		free((char *)dir);
		goto alloc_failed;
	}
	dir->dir_map = strdup(map);
	if (dir->dir_map == NULL) {
		free((char *)dir->dir_name);
		free((char *)dir);
		goto alloc_failed;
	}
	dir->dir_opts = strdup(opts);
	if (dir->dir_opts == NULL) {
		free((char *)dir->dir_name);
		free((char *)dir->dir_map);
		free((char *)dir);
		goto alloc_failed;
	}

	dir->dir_maptype = maptype;
	dir->dir_remove = mydir;
	INSQUE(dir_q, dir);

	new_fh(&dir->dir_vnode);
	dir->dir_vnode.vn_data = (char *)dir;
	dir->dir_vnode.vn_type = VN_DIR;
	fa = &dir->dir_vnode.vn_fattr;
	fa->type = NFDIR;
	fa->mode = NFSMODE_DIR + 0555;
	fa->nlink = 1;
	fa->uid = 0;
	fa->gid = 0;
	fa->size = 512;
	fa->blocksize = 512;
	fa->rdev = 0;
	fa->blocks = 1;
	fa->fsid = 0;
	fa->fileid = fileid(&dir->dir_vnode);
	gettimeofday((struct timeval *)&fa->atime, (struct timezone *)0);
	fa->mtime = fa->atime;
	fa->ctime = fa->atime;
	return;

alloc_failed:
	syslog(LOG_ERR, "Memory allocation failed: %m");
	return;
}

emptydir(name)
	char *name;
{
	DIR *dirp;
	struct dirent *d;

	dirp = opendir(name);
	if (dirp == NULL) 
		return (0);
	while (d = readdir(dirp)) {
		if (strcmp(d->d_name, ".") == 0)
			continue;
		if (strcmp(d->d_name, "..") == 0)
			continue;
		break;
	}
	closedir(dirp);
	if (d)
		return (0);
	return (1);
}

struct link *
makelink(dir, name, fs, subdir)
	struct autodir *dir;
	char *name;
	struct filsys *fs;
	char *subdir;
{
	struct link *link;
	char *malloc();
	register fattr *fa;

	link = findlink(dir, name);
	if (link == NULL) {
		link = (struct link *)malloc(sizeof *link);
		if (link == NULL)
			goto alloc_failed;
		link->link_name = strdup(name);
		if (link->link_name == NULL) {
			free((char *)link);
			goto alloc_failed;
		}
		INSQUE(dir->dir_head, link);
		link->link_fs = NULL;
		link->link_subdir = NULL;
		new_fh(&link->link_vnode);
		link->link_vnode.vn_data = (char *)link;
		link->link_vnode.vn_type = VN_LINK;
	}
	link->link_dir = dir;
	if (link->link_fs) {
		link->link_fs->fs_refcnt--;
		link->link_fs = NULL;
	}
	if (link->link_subdir) {
		free(link->link_subdir);
		link->link_subdir = NULL;
	}
	if (fs) {
		link->link_fs = fs;
		fs->fs_refcnt++;
	}
	if (subdir) {
		link->link_subdir = strdup(subdir);
		if (link->link_subdir == NULL) {
			REMQUE(link->link_dir->dir_head, link);
			if (link->link_name)
				free(link->link_name);
			if (link->link_fs)
				link->link_fs->fs_refcnt--;
			free((char *)link);
			goto alloc_failed;
		}
	}
	link->link_death = time_now + max_link_time;

	fa = &link->link_vnode.vn_fattr;
	fa->type = NFLNK;
	fa->mode = NFSMODE_LNK + 0777;
	fa->nlink = 1;
	fa->uid = 0;
	fa->gid = 0;
	fa->size = strlen(subdir);
	if (fs) 
		fa->size += strlen(fs->fs_dir) + 1;
	fa->blocksize = 512;
	fa->rdev = 0;
	fa->blocks = 1;
	fa->fsid = 0;
	fa->fileid = fileid(&link->link_vnode);
	gettimeofday((struct timeval *)&fa->atime, (struct timezone *)0);
	fa->mtime = fa->atime;
	fa->ctime = fa->atime;

	return (link);

alloc_failed:
	syslog(LOG_ERR, "Memory allocation failed: %m");
	return (NULL);
}

free_link(link)
	struct link *link;
{

	REMQUE(link->link_dir->dir_head, link);
	free_fh(&link->link_vnode);
	if (link->link_name)
		free(link->link_name);
	if (link->link_subdir)
		free(link->link_subdir);
	if (link->link_fs)
		link->link_fs->fs_refcnt--;
	free((char *)link);
}

struct link *
findlink(dir, name)
	struct autodir *dir;
	char *name;
{
	struct link *link;

	for (link = HEAD(struct link, dir->dir_head); link;
	    link = NEXT(struct link, link))
		if (strcmp(name, link->link_name) == 0)
			return (link);
	return ((struct link *)0);
}

free_filsys(fs)
	struct filsys *fs;
{
	REMQUE(fs_q, fs);
	free(fs->fs_host);
	free(fs->fs_dir);
	free(fs->fs_mntpnt);
	if (fs->fs_opts)
		free(fs->fs_opts);
}

struct filsys *
alloc_fs(host, dir, mntpnt, opts)
	char *host, *dir, *mntpnt, *opts;
{
	struct filsys *fs;

	fs = (struct filsys *)malloc(sizeof *fs);
	if (fs == NULL)
		goto alloc_failed;
	bzero((char *)fs, sizeof *fs);
	fs->fs_host = strdup(host);
	if (fs->fs_host == NULL) {
		free((char *)fs);
		goto alloc_failed;
	}
	fs->fs_dir = strdup(dir);
	if (fs->fs_dir == NULL) {
		free(fs->fs_host);
		free((char *)fs);
		goto alloc_failed;
	}
	fs->fs_mntpnt = strdup(mntpnt);
	if (fs->fs_mntpnt == NULL) {
		free(fs->fs_dir);
		free(fs->fs_host);
		free((char *)fs);
		goto alloc_failed;
	}
	if (opts != NULL) {
		fs->fs_opts = strdup(opts);
		if (fs->fs_opts == NULL) {
			free(fs->fs_mntpnt);
			free(fs->fs_dir);
			free(fs->fs_host);
			free((char *)fs);
			goto alloc_failed;
		}
	} else
		fs->fs_opts = NULL;
	INSQUE(fs_q, fs);
	return (fs);

alloc_failed:
	syslog(LOG_ERR, "Memory allocation failed: %m");
	return (NULL);
}

my_insque(head, item)
	struct queue *head, *item;
{
	item->q_next = head->q_head;
	item->q_prev = NULL;
	head->q_head = item;
	if (item->q_next)
		item->q_next->q_prev = item;
	if (head->q_tail == NULL)
		head->q_tail = item;
}

my_remque(head, item)
	struct queue *head, *item;
{
	if (item->q_prev)
		item->q_prev->q_next = item->q_next;
	else
		head->q_head = item->q_next;
	if (item->q_next)
		item->q_next->q_prev = item->q_prev;
	else
		head->q_tail = item->q_prev;
	item->q_next = item->q_prev = NULL;
}

do_timeouts()
{
	struct autodir *dir;
	struct link *link, *nextlink;
	struct filsys *fs, *nextfs;

	for (dir = HEAD(struct autodir, dir_q); dir;
	    dir = NEXT(struct autodir, dir)) {
		link = HEAD(struct link, dir->dir_head);
		while (link) {
			nextlink = NEXT(struct link, link);
			if (link->link_death && time_now >= link->link_death) {
				if (fs = link->link_fs) {
					fs->fs_refcnt--;
					link->link_fs = 0;
				}
				if (link->link_dir->dir_maptype ==
				    MAPTYPE_HOSTS ||
				    link->link_dir->dir_maptype ==
				    MAPTYPE_FSTAB) {
					if (freeall(link->link_dir,
					    link->link_name) == 0) {
						link->link_death = time_now +
							max_link_time;
					} else {
						link->link_death = 0;
#ifdef NeXT
						if (link->link_dir->
						    dir_maptype ==
						    MAPTYPE_FSTAB) {
							(link->
							 link_vnode.
							 vn_fattr.mode |= 
							 NFSMODE_STICKY);
						}
#endif
					}
				}
			}
			link = nextlink;
		}
	}
	for (fs = HEAD(struct filsys, fs_q); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_mine) {
			if (fs->fs_refcnt)
				continue;
			if (fs->fs_hostdir)
				continue;
			if (try_unmount(fs->fs_mntpnt, 1)) {
				nfsunmount(fs);
				free_filsys(fs);
			}
		}
	}
}

flush_links(fs)
	struct filsys *fs;
{
	struct link *link;
	struct vnode *vnode;
	int i;

	for (i = 0; i < FH_HASH_SIZE; i++) {
		vnode = HEAD(struct vnode, fh_q_hash[i]);
		for (; vnode; vnode = NEXT(struct vnode, vnode)) {
			if (fs->fs_refcnt == 0)
				return;
			if (vnode->vn_type != VN_LINK)
				continue;
			link = (struct link *)vnode->vn_data;
			if (link->link_fs != fs)
				continue;
			free_link(link);
		}
	}
	if (fs->fs_refcnt != 0)
		syslog(LOG_ERR, "flush_link: refcnt error");
}

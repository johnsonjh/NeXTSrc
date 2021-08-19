#ifndef lint
static char sccsid[] = 	"@(#)auto_mount.c	1.3 88/07/26 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/param.h>
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
typedef nfs_fh fhandle_t;
#include <rpcsvc/mount.h>
#define NFSCLIENT
#define NFS                     /* for backwards compatibility */
#include <sys/mount.h>
#ifdef NeXT
#include <nfs/nfs_mount.h>
#ifndef M_NEWTYPE
#define OLDMOUNT
#endif
#endif
#include "automount.h"


#define MNTREQUESTSIZE 512
#define MNTREPLYSIZE 512

static getlist();
static struct mntent *dupmntent();

static long last_mtab_time = 0;

nfsstat
trymount(mfs, opts, fsp)
	struct mapfs *mfs;
	char *opts;
	struct filsys **fsp;
{
	static int mountcount = 0;
	char mntpnt[MAXPATHLEN];
	enum clnt_stat pingmount();
	nfsstat status;

	/* ping the null procedure of the remote mount daemon */
	switch (pingmount(mfs->mfs_host)) {
	case RPC_SUCCESS:
		break;
	case RPC_UNKNOWNHOST:
		return (NFSERR_NOENT);
	default:
		syslog(LOG_ERR, "host %s not responding", mfs->mfs_host);
		return (NFSERR_NOENT);
	}
	/* mktemp alone not good enough! */
	(void) sprintf(mntpnt, "%s/auto%dXXXXXX", tmpdir, mountcount++);
	mktemp(mntpnt);
	if (mkdir(mntpnt, 0777)) {
		syslog(LOG_ERR, "Can't make directory %s: %m", mntpnt);
		return (NFSERR_IO);
	}
	*fsp = NULL;
	status = nfsmount(mfs->mfs_host, mfs->mfs_dir, mntpnt, opts, fsp);
	if (status)
		safe_rmdir(mntpnt);
	return (status);
}

/*
 * Search the mount table to see if the given file system is already
 * mounted. 
 */
struct filsys *
already_mounted(host, fsname, opts)
	char *host;
	char *fsname;
	char *opts;
{
	struct filsys *fs, *fsnext;
	FILE *table;
	register struct mntent *mt;
	struct stat stbuf;
	char *index();
	struct mntent m1, m2;
	int has1, has2;
	int fd;

	/* 
	 * read in /etc/mtab if it's changed
	 */
	if (stat(MOUNTED, &stbuf) != 0) {
		syslog(LOG_ERR, "Cannot stat %s: %m", MOUNTED);
		return ((struct filsys *)0);
	}
	if (stbuf.st_mtime != last_mtab_time) {
		last_mtab_time = stbuf.st_mtime;
		/* first see what's been unmounted */
		for (fs = HEAD(struct filsys, fs_q); fs; fs = fsnext) {
			fsnext = NEXT(struct filsys, fs);
			if (fs->fs_mine)
				continue;
			if (stat(fs->fs_mntpnt, &stbuf) == 0 &&
			    fs->fs_dev == stbuf.st_dev)
				continue;
			flush_links(fs);
			free_filsys(fs);
		}
		/* now see what's been mounted */
		table = setmntent(MOUNTED, "r");
		while ((mt = getmntent(table)) != NULL) {
			char *tmphost, *tmppath, *p, tmpc;

			if (strcmp(mt->mnt_type, MNTTYPE_NFS) != 0)
				continue;
			p = index(mt->mnt_fsname, ':');
			if (p == NULL)
				continue;
			tmpc = *p;
			*p = '\0';
			tmphost = mt->mnt_fsname;
			tmppath = p+1;
			for (fs = HEAD(struct filsys, fs_q); fs;
			    fs = NEXT(struct filsys, fs))
				if (strcmp(tmphost, fs->fs_host) == 0 &&
				    strcmp(tmppath, fs->fs_dir) == 0)
					break;
			if (fs == NULL && tmppath[0] == '/') {
				if (stat(mt->mnt_dir, &stbuf) == 0) {
					fs = alloc_fs(tmphost, tmppath,
						mt->mnt_dir, mt->mnt_opts);
					if (fs == NULL)
						return ((struct filsys *)0);
					fs->fs_dev = stbuf.st_dev;
				}
			}
			*p = tmpc;
		}
		endmntent(table);
	}

	m1.mnt_opts = opts;
	for (fs = HEAD(struct filsys, fs_q); fs; fs = NEXT(struct filsys, fs)){
		if (strcmp(host, fs->fs_host) != 0 ||
		    strcmp(fsname, fs->fs_dir) != 0)
			continue;
		m2.mnt_opts = fs->fs_opts;
		has1 = hasmntopt(&m1, MNTOPT_RO) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_RO) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_NOSUID) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_NOSUID) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_SOFT) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_SOFT) != NULL;
		if (has1 != has2)
			continue;
		has1 = hasmntopt(&m1, MNTOPT_INTR) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_INTR) != NULL;
		if (has1 != has2)
			continue;
#ifdef MNTOPT_SECURE
		has1 = hasmntopt(&m1, MNTOPT_SECURE) != NULL;
		has2 = hasmntopt(&m2, MNTOPT_SECURE) != NULL;
		if (has1 != has2)
			continue;
#endif
		/*
		 * dir under mountpoint may have been removed by other means
		 * If so, we get a stale file handle error here if we fsync
		 * the dir to remove the attr cache info
		 */
		fd = open(fs->fs_mntpnt, 0);
		if (fd < 0)
			continue;
		if (fsync(fd) != 0 || fstat(fd, &stbuf) != 0) {
			close(fd);
			continue;
		}
		close(fd);
		return (fs);
	}
	return(0);
}

try_unmount(mntpnt, rmdir)
	char *mntpnt;
	int rmdir;
{
	extern int errno;

	if (unmount(mntpnt) < 0) {
		if (errno != EBUSY)
			syslog(LOG_ERR, "%s: %m", mntpnt);
		return (0);
	}
	if (rmdir)
		safe_rmdir(mntpnt);
	clean_mtab(mntpnt);
	return (1);
}

nfsunmount(fs)
	struct filsys *fs;
{
	struct sockaddr_in sin;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	int s;

	sin = fs->fs_addr;
	/*
	 * Port number of "fs->fs_addr" is NFS port number; make port
	 * number 0, so "clntudp_create" finds port number of mount
	 * service.
	 */
	sin.sin_port = 0;
	s = RPC_ANYSOCK;
	timeout.tv_usec = 0;
	timeout.tv_sec = 10;
	if ((client = clntudp_bufcreate(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s, MNTREQUESTSIZE, MNTREPLYSIZE)) == NULL) {
		syslog(LOG_WARNING, "%s:%s %s",
		    fs->fs_host, fs->fs_dir,
		    clnt_spcreateerror("server not responding"));
		return;
	}
#ifdef OLDMOUNT
#ifndef NeXT
	if (bindresvport(s, NULL))
		syslog(LOG_ERR, "Warning: cannot do local bind");
#endif
#endif
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 25;
	rpc_stat = clnt_call(client, MOUNTPROC_UMNT, xdr_path, &fs->fs_dir,
	    xdr_void, (char *)NULL, timeout);
	close(s);
	AUTH_DESTROY(client->cl_auth);
#if	NeXT
	if (rpc_stat != RPC_SUCCESS)
		syslog(LOG_WARNING, "%s", clnt_sperror(client, "unmount"));
	clnt_destroy(client);
#else	NeXT
	clnt_destroy(client);
	if (rpc_stat != RPC_SUCCESS)
		syslog(LOG_WARNING, "%s", clnt_sperror(client, "unmount"));
#endif	NeXT 
}

enum clnt_stat
pingmount(host)
	char *host;
{
	struct timeval tottime;
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	u_long port;
	struct hostent *hp;

	tottime.tv_sec = mount_timeout;
	tottime.tv_usec = 0;

	/* 
	 * We use the pmap_rmtcall interface because the normal
	 * portmapper has a wired-in 60 second timeout
	 */
	if ((hp = gethostbyname(host)) == NULL)
		return (RPC_UNKNOWNHOST);
	bcopy(hp->h_addr, (char *)&server_addr.sin_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = 0;
	clnt_stat = pmap_rmtcall(&server_addr, MOUNTPROG, MOUNTVERS, NULLPROC,
			xdr_void, 0, xdr_void, 0, tottime, &port);
	return (clnt_stat);
}

struct in_addr gotaddr;

/* ARGSUSED */
catchfirst(ignore, raddr)
	struct sockaddr_in *raddr;
{
	gotaddr = raddr->sin_addr;
	return (1);	/* first one ends search */
}

/*
 * ping a bunch of hosts at once and find out who
 * responds first
 */
struct in_addr
trymany(addrs)
	struct in_addr *addrs;
{
	enum clnt_stat many_cast();
	enum clnt_stat clnt_stat;

	gotaddr.s_addr = 0;
	clnt_stat = many_cast(addrs, MOUNTPROG, MOUNTVERS, NULLPROC,
			xdr_void, 0, xdr_void, 0, catchfirst);
	if (clnt_stat)
		syslog(LOG_ERR, "trymany: %s", clnt_sperrno(clnt_stat));
	return (gotaddr);
}

nfsstat
nfsmount(host, dir, mntpnt, opts, fsp)
	char *host, *dir, *mntpnt, *opts;
	struct filsys **fsp;
{
	struct filsys *fs;
	char netname[MAXNETNAMELEN+1];
	char remname[MAXPATHLEN];
	struct mntent m;
	struct nfs_args args;
	int flags;
	struct sockaddr_in sin;
	struct hostent *hp;
	static struct fhstatus fhs;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	int s;
	u_short port;
	nfsstat status;
	char hissite[64], siteopts[64];
	char mntopts[100];

	(void) sprintf(remname, "%s:%s", host, dir);
	m.mnt_fsname = remname;
	m.mnt_dir = mntpnt;
	m.mnt_type = MNTTYPE_NFS;
	if (opts == NULL || opts[0] == '\0')
		strcpy(mntopts, "rw");
	else
		strcpy(mntopts, opts);
	m.mnt_opts = mntopts;
	m.mnt_freq = m.mnt_passno = 0;

	/*
	 * Get server's address
	 */
	if ((hp = gethostbyname(host)) == NULL) {
		syslog(LOG_ERR, "%s not in hosts database", host);
		return (NFSERR_NOENT);
	}

	/*
	 * get fhandle of remote path from server's mountd
	 */
	bzero((char *)&sin, sizeof(sin));
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_family = AF_INET;
	timeout.tv_usec = 0;
	timeout.tv_sec = mount_timeout;
	s = RPC_ANYSOCK;
	if ((client = clntudp_bufcreate(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s, MNTREQUESTSIZE, MNTREPLYSIZE)) == NULL) {
		syslog(LOG_ERR, "%s %s", m.mnt_fsname,
		    clnt_spcreateerror("server not responding"));
		return (NFSERR_NOENT);
	}
#ifdef OLDMOUNT
#ifndef NeXT
	if (bindresvport(s, NULL))
		syslog(LOG_ERR, "Warning: cannot do local bind");
#endif
#endif
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = mount_timeout;
	rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &dir,
	    xdr_fhstatus, &fhs, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		/*
		 * Given the way "clnt_sperror" works, the "%s" immediately
		 * following the "not responding" is correct.
		 */
		syslog(LOG_ERR, "%s server not responding%s", m.mnt_fsname,
		    clnt_sperror(client, ""));
		close(s);
		clnt_destroy(client);
		return (NFSERR_NOENT);
	}
	close(s);
	AUTH_DESTROY(client->cl_auth);
	clnt_destroy(client);

	if (errno = fhs.fhs_status)  {
                if (errno == EACCES) {
			status = NFSERR_ACCES;
                } else {
			syslog(LOG_ERR, "%s: %m", m.mnt_fsname);
			status = NFSERR_IO;
                }
                return (status);
        }        

	/*
	 * set mount args
	 */
	args.fh = (fhandle_t *)&fhs.fhs_fh;
	args.hostname = host;
	args.flags = 0;
	args.flags |= NFSMNT_HOSTNAME;
	args.addr = &sin;
	if (hasmntopt(&m, MNTOPT_SOFT) != NULL) {
		args.flags |= NFSMNT_SOFT;
	}
	if (hasmntopt(&m, MNTOPT_INTR) != NULL) {
		args.flags |= NFSMNT_INT;
	}
#ifdef SECURE_RPC
#ifdef MNTOPT_SECURE
	if (hasmntopt(&m, MNTOPT_SECURE) != NULL) {
		args.flags |= NFSMNT_SECURE;
		/*
                 * XXX: need to support other netnames outside domain
                 * and not always just use the default conversion
                 */
                if (!host2netname(netname, host, NULL)) {
                        return (NFSERR_NOENT); /* really unknown host */
                }
                args.netname = netname;
	}
#endif MNTOPT_SECURE
#endif SECURE_RPC
	if (port = nopt(&m, "port")) {
		sin.sin_port = htons(port);
	} else {
		sin.sin_port = htons(NFS_PORT);	/* XXX should use portmapper */
	}

	/*
	 * Get location-dependent mount options by using site information
	 */
	getsite(sin.sin_addr, hissite);
	getoptsbysites(mysite, hissite, siteopts);
	if (siteopts[0]) {
		strcat(mntopts, ",");
		strcat(mntopts, siteopts);
	}
	if (args.rsize = nopt(&m, "rsize")) {
		args.flags |= NFSMNT_RSIZE;
	}
	if (args.wsize = nopt(&m, "wsize")) {
		args.flags |= NFSMNT_WSIZE;
	}
	if (args.timeo = nopt(&m, "timeo")) {
		args.flags |= NFSMNT_TIMEO;
	}
	if (args.retrans = nopt(&m, "retrans")) {
		args.flags |= NFSMNT_RETRANS;
	}

	flags = 0;
	flags |= (hasmntopt(&m, MNTOPT_RO) == NULL) ? 0 : M_RDONLY;
	flags |= (hasmntopt(&m, MNTOPT_NOSUID) == NULL) ? 0 : M_NOSUID;

#ifdef OLDMOUNT
#define MOUNT_NFS 1
	if (mount(MOUNT_NFS, mntpnt, flags, &args)) {
#else
	if (mount("nfs", mntpnt, flags | M_NEWTYPE, &args)) {
#endif
		syslog(LOG_ERR, "Mount of %s on %s: %m", remname, mntpnt);
		return (NFSERR_IO);
	}
 	addtomtab(&m);    
	if (*fsp)
		fs = *fsp;
	else {
		fs = alloc_fs(host, dir, mntpnt, opts);
		if (fs == NULL)
			return (NFSERR_NOSPC);
	}
	fs->fs_mine = 1;
	fs->fs_nfsargs = args;
	fs->fs_mflags = flags;
	fs->fs_nfsargs.hostname = fs->fs_host;
	fs->fs_nfsargs.addr = &fs->fs_addr;
	fs->fs_nfsargs.fh = (fhandle_t *)&fs->fs_rootfh;
	fs->fs_addr = sin;
	bcopy(&fhs.fhs_fh, &fs->fs_rootfh, sizeof fs->fs_rootfh);
	*fsp = fs;
	return (NFS_OK);
}

nfsstat
remount(fs)
	struct filsys *fs;
{
	struct mntent m;
	char remname[1024];

	if (fs->fs_nfsargs.fh == 0) 
		return nfsmount(fs->fs_host, fs->fs_dir, fs->fs_mntpnt, 
				fs->fs_opts, &fs);
	(void) sprintf(remname, "%s:%s", fs->fs_host, fs->fs_dir);
#ifdef OLDMOUNT
	if (mount(MOUNT_NFS, fs->fs_mntpnt, fs->fs_mflags, &fs->fs_nfsargs)) {
#else
	if (mount("nfs", fs->fs_mntpnt, fs->fs_mflags | M_NEWTYPE, &fs->fs_nfsargs)) {
#endif
		syslog(LOG_ERR, "Remount of %s on %s: %m", remname,
		    fs->fs_mntpnt);
		return (NFSERR_IO);
	}
	m.mnt_fsname = remname;
	m.mnt_dir = fs->fs_mntpnt;
	m.mnt_type = MNTTYPE_NFS;
	m.mnt_opts = fs->fs_opts;
	m.mnt_freq = m.mnt_passno = 0;
 	addtomtab(&m);    
	return (NFS_OK);
}


/*
 * update /etc/mtab
 */
addtomtab(mnt)
        struct mntent *mnt;
{
	FILE *mnted;
	struct stat stbuf;

	mnted = setmntent(MOUNTED, "a");
	if (mnted == NULL) {
		syslog(LOG_ERR, "%s: %m", MOUNTED);
		exit(1);
	}
	if (addmntent(mnted, mnt)) {
		syslog(LOG_ERR, "%s: %m", MOUNTED);
		exit(1);
	}
	endmntent(mnted);
	stat(MOUNTED, &stbuf);
	last_mtab_time = stbuf.st_mtime;

}

static struct mntent **list;

clean_mtab(mntpnt)
	char *mntpnt;
{
	FILE *fp;
	struct mntent *mnt;
	struct stat stbuf;
	int i;

	fp = setmntent(MOUNTED, "r+");
	if (fp == NULL) {
		syslog(LOG_ERR, "%s: %m", MOUNTED);
		return;
	}
	fstat(fileno(fp), &stbuf);
	if (last_mtab_time != stbuf.st_mtime)
		last_mtab_time = 0;
	if (!getlist(fp, mntpnt, 0)) {
		endmntent(fp);
		syslog(LOG_ERR, "Memory allocation failed: %m");
		return;
	}
	rewind(fp);
	ftruncate(fileno(fp), 0);
	for (i = 0; list[i]; i++) {
		mnt = list[i];
		addmntent(fp, mnt);
		freemntent(mnt);
	}
	free(list);
	endmntent(fp);
	if (last_mtab_time) {
		stat(MOUNTED, &stbuf);
		last_mtab_time = stbuf.st_mtime;
	}
}

static
getlist(fp, mntpnt, count)
	FILE *fp;
	char *mntpnt;
	int count;
{
	struct mntent *mnt, *dupmntent();

	mnt = getmntent(fp);
	if (mnt == NULL) {
		list = (struct mntent **)calloc(count+1,
						sizeof (struct mntent *));
		if (list == NULL)
			return (0);
		return (1);
	}
	if (strcmp(mntpnt, mnt->mnt_dir) == 0) {        /* hit */
		getlist(fp, mntpnt, count);
		return (1);
	}
	mnt = dupmntent(mnt);
	if (mnt == NULL)
		return (0);
	if (!getlist(fp, mntpnt, count+1)) {
		freemntent(mnt);
		return (0);
	}
	list[count] = mnt;
	return (1);
}

static struct mntent *
dupmntent(mnt)
	struct mntent *mnt;
{
	struct mntent *m;

	m = (struct mntent *)calloc(1, sizeof(struct mntent));
	if (m == NULL)
		return (NULL);
	m->mnt_fsname = strdup(mnt->mnt_fsname);
	if (m->mnt_fsname == NULL) {
		free((char *)m);
		return (NULL);
	}
	m->mnt_dir = strdup(mnt->mnt_dir);
	if (m->mnt_dir == NULL) {
		free(m->mnt_fsname);
		free((char *)m);
		return (NULL);
	}
	m->mnt_type = strdup(mnt->mnt_type);
	if (m->mnt_type == NULL) {
		free(m->mnt_dir);
		free(m->mnt_fsname);
		free((char *)m);
		return (NULL);
	}
	m->mnt_opts = strdup(mnt->mnt_opts);
	if (m->mnt_opts == NULL) {
		free(m->mnt_type);
		free(m->mnt_dir);
		free(m->mnt_fsname);
		free((char *)m);
		return (NULL);
	}
	m->mnt_freq = mnt->mnt_freq;
	m->mnt_passno = mnt->mnt_passno;
	return (m);
}

freemntent(mnt)
	struct mntent *mnt;
{
	free(mnt->mnt_fsname);
	free(mnt->mnt_dir);
	free(mnt->mnt_type);
	free(mnt->mnt_opts);
	free((char *)mnt);
}

#ifdef notdef
	(void) strcpy(tmpname, "/etc/automtabXXXXXX");
	(void) mktemp(tmpname);
	from = setmntent(MOUNTED, "r");
	if (from == NULL) {
		syslog(LOG_ERR, "%s: %m", MOUNTED);
		return;
	}
	to = setmntent(tmpname, "w");
	if (to == NULL) {
		syslog(LOG_ERR, "%s: %m", tmpname);
		endmntent(from);
		return;
	}
	while (mnt = getmntent(from)) {
		if (strcmp(mntpnt, mnt->mnt_dir) == 0) {	/* hit */
			ok++;
			continue;
		}
		addmntent(to, mnt);
	}
	endmntent(from);
	endmntent(to);
	if (rename(tmpname, MOUNTED))
		syslog(LOG_ERR, "Cannot rename %s: %m", MOUNTED);
	stat(MOUNTED, &stbuf);
	last_mtab_time = stbuf.st_mtime;
}
#endif

/*
 * Return the value of a numeric option of the form foo=x, if
 * option is not found or is malformed, return 0.
 */
nopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	int val = 0;
	char *equal;
	char *str;

	if (str = hasmntopt(mnt, opt)) {
		if (equal = index(str, '=')) {
			val = atoi(&equal[1]);
		} else {
			syslog(LOG_ERR, "Bad numeric option '%s'", str);
		}
	}
	return (val);
}


#ifndef lint
static char sccsid[] = 	"@(#)auto_main.c	1.3 88/07/27 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "nfs_prot.h"
#include <mntent.h>
#include <netdb.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#define NFSCLIENT
typedef nfs_fh fhandle_t;
#include <sys/mount.h>
#if	NeXT
#include <nfs/nfs_mount.h>
#include <sys/resource.h>
#ifndef M_NEWTYPE
#define OLDMOUNT
#endif
#endif

#include "automount.h"

extern errno;

void catch();

#define REQUESTSIZE 512
#define REPLYSIZE   2048

#define	MAXDIRS	10

#define	MASTER_MAPNAME	"auto.master"

int maxwait = 60;
int mount_timeout = 30;
int max_link_time = 5*60;

u_short myport;

main(argc, argv)
	int argc;
	char *argv[];
{
	SVCXPRT *transp;
	extern void nfs_program_2();
	static struct sockaddr_in sin;	/* static causes zero init */
	struct nfs_args args;
	struct autodir *dir;
	int pid;
	int bad;
	int master = 1;
	struct hostent *hp;
	extern int trace;
	char pidbuf[20];
	int timeo;
	char *alttmpdir = NULL;
#if	NeXT
	struct rlimit rlim;
	extern void exit();
#endif

#if NeXT
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);
	signal (SIGUSR2, exit);
#endif

	argc--;
	argv++;

	openlog("automount", 0, LOG_DAEMON);

	(void) setbuf(stdout, (char *)NULL);
	(void) gethostname(self, sizeof self);
	hp = gethostbyname(self);
	if (hp == NULL) {
		syslog(LOG_ERR, "Can't get my address");
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&my_addr, hp->h_length);
	(void) getdomainname(mydomain, sizeof mydomain);
#if	NeXT
	usingYP = !yp_bind(mydomain);
#else
	if (bad = yp_bind(mydomain))
		syslog(LOG_ERR, "YP bind failed: %s", yperr_string(bad));
#endif

	getsite(my_addr, mysite);

	time_now = time((long *)0);

	while (argc && argv[0][0] == '-') switch (argv[0][1]) {
	case 'a':
		if (argc < 2) {
			usage();
		}
		alttmpdir = argv[1];
		argc -= 2;
		argv += 2;
		break;
	case 'n':
		nomounts++;
		argc--;
		argv++;
		break;
	case 'm':
		master = 0;
		argc--;
		argv++;
		break;
	case 't':	/* timeout */
		if (argc < 2 || (timeo = atoi(argv[1])) <= 0) {
			fprintf(stderr, "Bad timeout value\n");
			usage();
		}
		switch(argv[0][2]) {
		case 'm':
			mount_timeout = timeo;
			break;
		case 'l':
			max_link_time = timeo;
			break;
		case 'w':
			maxwait = timeo;
			break;
		default:
			fprintf(stderr, "automount: bad timeout switch\n");
			usage();
		}
		argc -= 2;
		argv += 2;
		break;

	case 'T':
		trace++;
		argc--;
		argv++;
		break;
	default:
		usage();
	}

#if	NeXT
	strcpy(tmpdir, alttmpdir ? alttmpdir : "/private/tmp_mnt");
#else
	strcpy(tmpdir, "/tmp_mnt");
#endif
	mkdir_r(tmpdir);

	if (argc == 0 && master == 0) {
		syslog(LOG_ERR, "no mount maps specified");
		usage();
	}
	while (argc >= 2) {
		if (argc >= 3 && argv[2][0] == '-') {
			dirinit(argv[0], argv[1], argv[2]+1);
			argc -= 3;
			argv += 3;
		} else {
			dirinit(argv[0], argv[1], "rw");
			argc -= 2;
			argv += 2;
		}
	}
	if (argc)
		usage();

	if (master) {
		int loadmaster();
		static struct ypall_callback callback = { loadmaster, 0 };

		bad = yp_all(mydomain, MASTER_MAPNAME, &callback);
		if (bad)
			syslog(LOG_ERR, "Master yp map: %s",
				yperr_string(bad));
	}

	if (HEAD(struct autodir, dir_q) == NULL)   /* any maps ? */
		exit(1);

	transp = svcudp_bufcreate(RPC_ANYSOCK, REPLYSIZE, REQUESTSIZE);
	if (transp == NULL) {
		syslog(LOG_ERR, "Cannot create UDP service");
		exit(1);
	}
	if (!svc_register(transp, NFS_PROGRAM, NFS_VERSION, nfs_program_2, 0)) {
		syslog(LOG_ERR, "svc_register failed");
		exit(1);
	}

#ifdef DEBUG
	pid = getpid();
	if (fork()) {
		/* parent */
		signal(SIGTERM, (int (*)())catch);
		auto_run();
		syslog(LOG_ERR, "svc_run returned");
		exit(1);
	}
#else NODEBUG
	switch (pid = fork()) {
	case -1:
		syslog(LOG_ERR, "Cannot fork: %m");
		exit(1);
	case 0:
		/* child */
		{ int tt = open("/dev/tty", O_RDWR);
		  if (tt > 0) {
			ioctl(tt, TIOCNOTTY, (char *)0);
			(void) close(tt);
		  }
		}
		signal(SIGTERM, (int (*)())catch);
		auto_run();
		syslog(LOG_ERR, "svc_run returned");
		exit(1);
	}
#endif

	/* parent */
	sin.sin_family = AF_INET;
	sin.sin_port = htons(transp->xp_port);
	myport = transp->xp_port;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	args.addr = &sin;
	args.flags = NFSMNT_INT + NFSMNT_TIMEO +
		     NFSMNT_HOSTNAME + NFSMNT_RETRANS;
	args.timeo = (mount_timeout + 5) * 10;
	args.retrans = 0;
#if	NeXT
	sprintf(pidbuf, "(autonfsmount[%d])", pid);
#else
	sprintf(pidbuf, "(pid%d)", pid);
#endif	NeXT
	args.hostname = pidbuf;
	bad = 1;
	for (dir = HEAD(struct autodir, dir_q); dir;
	    dir = NEXT(struct autodir, dir)) {
	  	args.fh = (fhandle_t *)&dir->dir_vnode.vn_fh; 
#ifdef OLDMOUNT
#define MOUNT_NFS 1
		if (mount(MOUNT_NFS, dir->dir_name, M_RDONLY, &args)) {
#else
		if (mount("nfs", dir->dir_name, M_RDONLY|M_NEWTYPE, &args)) {
#endif
			syslog(LOG_ERR, "Can't mount %s: %m", dir->dir_name);
			bad++;
		} else {
			domntent(pid, dir->dir_name);
			bad = 0;
		}
	}
	if (bad)
		kill(pid, SIGKILL);
#ifdef DEBUG
fprintf(stderr, "mounted\n");
#endif
	exit(0);
	/*NOTREACHED*/
}

domntent(pid, mntpoint)
	int pid;
	char *mntpoint;
{
	FILE *f;
	struct mntent mnt;
	char fsname[64];
	char mntopts[100];

	f = setmntent(MOUNTED, "a");
	if (f == NULL) {
		syslog(LOG_ERR, "Can't update %s", MOUNTED);
		return;
	}
#if	NeXT
	(void) sprintf(fsname, "%s:(autonfsmount[%d])", self, pid);
#else
	(void) sprintf(fsname, "%s:(pid%d)", self, pid);
#endif	NeXT
	mnt.mnt_fsname = fsname;
	mnt.mnt_dir = mntpoint;
#ifdef OLDMOUNT
	mnt.mnt_type = MNTTYPE_IGNORE;
#else
	mnt.mnt_type = MNTTYPE_NFS;
#endif 
	(void) sprintf(mntopts, "ro,intr,port=%d", myport);
	mnt.mnt_opts = mntopts;
	mnt.mnt_freq = 0;
	mnt.mnt_passno = 0;	
	addmntent(f, &mnt);	
	endmntent(f);
}

void
catch()
{
	struct autodir *dir;
	int child;
	struct filsys *fs;

	if ((child = fork()) == 0)
		return;
	for (dir = HEAD(struct autodir, dir_q); dir;
	    dir = NEXT(struct autodir, dir)) {
		if (unmount(dir->dir_name) < 0) {
			if (errno != EBUSY)
				syslog(LOG_ERR, "unmount %s: %m", dir->dir_name);
		} else {
			if (dir->dir_remove)
				safe_rmdir(dir->dir_name);
			clean_mtab(dir->dir_name);
		}
	}
	kill (child, SIGKILL);
	for (fs = HEAD(struct filsys, fs_q); fs; fs = NEXT(struct filsys, fs)) {
		if (fs->fs_mine) {
			if (try_unmount(fs->fs_mntpnt, 1)) {
/*				clean_mtab(fs->fs_mntpnt);*/
				nfsunmount(fs);
			}
		}
	}
	syslog(LOG_ERR, "exiting");
	exit(0);
}

auto_run()
{
	int read_fds, n;
	long time();
	long last = 0;
	struct timeval tv;

	tv.tv_sec = maxwait;
	tv.tv_usec = 0;
	for (;;) {
		read_fds = svc_fds;
		n = select(32, &read_fds, (int *)0, (int *)0, &tv);
		time_now = time((long *)0);
		if (n)
			svc_getreq(read_fds);
		if (time_now >= last + maxwait) {
			last = time_now;
			do_timeouts();
		}
	}
}

usage()
{
	fprintf(stderr,
		"Usage: automount [-n] [-m] [-tl s] [-tm s] [-tw s] [dir map [-mntopts]] ...\n");
	exit(1);
}

/* ARGSUSED */
loadmaster(status, key, kl, val, vl, notused)
	int status;
	char *key;
	int kl;
	char *val;
	int vl;
	char *notused;
{
        int e;
	char dir[100], map[100];
	char *p, *opts;

	if (status == YP_TRUE) {
		if (kl >= 100 || vl >= 100)
			return (FALSE);
		if (kl < 2 || vl < 1)
			return (FALSE);
		if (isspace(*key) || *key == '#')
			return (FALSE);
		(void) strncpy(dir, key, kl);
		dir[kl] = '\0';
		(void) strncpy(map, val, vl);
		map[vl] = '\0';
		p = map;
		while (*p && !isspace(*p))
			p++;
		opts = "rw";
		if (*p) {
			*p++ = '\0';
			while (*p && isspace(*p))
				p++;
			if (*p == '-')
				opts = p+1;
		}
		dirinit(dir, map, opts);
		return (FALSE);
	} else {
		e = ypprot_err(status);
		if (e != YPERR_NOMORE && e != YPERR_MAP)
			syslog(LOG_ERR, "%s: %s", MASTER_MAPNAME,
			    yperr_string(e));
		return (TRUE);
        }
}

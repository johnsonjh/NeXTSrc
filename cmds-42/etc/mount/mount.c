#ifndef lint
static  char sccsid[] = "@(#)mount.c	1.4 88/06/02 4.0NFSSRC; from 2.61 88/02/07 SMI Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#define	NFSCLIENT
/*
 * mount
 */
#include <ctype.h>
#include <strings.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <mntent.h>
#include <sys/mount.h>
#include <sys/wait.h>
#if	NeXT
#include <nfs/nfs_mount.h>
#endif	NeXT

#if	NeXT

#ifndef MNTOPT_NET
#define MNTOPT_NET "net"
#endif

#ifndef MNTOPT_DYNAMIC
#define MNTOPT_DYNAMIC "dynamic"
#endif

#define MOUNT_TRIES 5	/* number of retries on mount call */
#undef SANITY_CHECK_FSTAB /* We can't depend on an /etc/fstab at NeXT */
#endif	NeXT

int	ro = 0;
int	quota = 0;
int	fake = 0;
int	freq = 1;
int	passno = 2;
int	all = 0;
int	verbose = 0;
int	printed = 0;
#if	NeXT
int	nomtab = 0;
int	exitflag = 0;
#endif	NeXT


#define	NRETRY	10000	/* number of times to retry a mount request */
#define	BGSLEEP	5	/* initial sleep time for background mount in seconds */
#define MAXSLEEP 30	/* max sleep time for background mount in seconds */
/*
 * Fake errno for RPC failures that don't map to real errno's
 */
#define ERPC	(10000)

extern int errno;

extern char	*realpath();
#if	NeXT
extern char	*vmount_realpath(char *, char *);
#endif	NeXT

void	replace_opts();
void	printmtab();

char	*index(), *rindex();
char	host[MNTMAXSTR];
char	name[MNTMAXSTR];
char	dir[MNTMAXSTR];
char	type[MNTMAXSTR];
char	opts[MNTMAXSTR];
char	netname[MAXNETNAMELEN+1];

/*
 * Structure used to build a mount tree.  The tree is traversed to do
 * the mounts and catch dependancies
 */
struct mnttree {
	struct mntent *mt_mnt;
	struct mnttree *mt_sib;
	struct mnttree *mt_kid;
};
struct mnttree *maketree();

main(argc, argv)
	int argc;
	char **argv;
{
	struct mntent mnt;
	struct mntent *mntp;
	FILE *mnttab;
	char *options;
	char *colon;
	struct mnttree *mtree;

	if (argc == 1) {
		mnttab = setmntent(MOUNTED, "r");
		while ((mntp = getmntent(mnttab)) != NULL) {
			if (strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) {
				continue;
			}
			printent(mntp);
		}
		endmntent(mnttab);
		exit(0);
	}

	close(2);
	if (dup2(1, 2) < 0) {
		perror("dup");
		exit(1);
	}

	opts[0] = '\0';
	type[0] = '\0';

	/*
	 * Set options
	 */
	while (argc > 1 && argv[1][0] == '-') {
		options = &argv[1][1];
		while (*options) {
			switch (*options) {
			case 'a':
				all++;
				break;
			case 'f':
				fake++;
				break;
#if	NeXT
			case 'n':
				nomtab++;
				break;
#endif	NeXT
			case 'o':
				if (argc < 3) {
					usage();
				}
				strcpy(opts, argv[2]);
				argv++;
				argc--;
				break;
			case 'p':
				if (argc != 2) {
					usage();
				}
				printmtab(stdout);
				exit(0);
			case 'q':
				quota++;
				break;
			case 'r':
				ro++;
				break;
			case 't':
				if (argc < 3) {
					usage();
				}
				strcpy(type, argv[2]);
				argv++;
				argc--;
				break;
			case 'v':
				verbose++;
				break;
			default:
				fprintf(stderr, "mount: unknown option: %c\n",
				    *options);
				usage();
			}
			options++;
		}
		argc--, argv++;
	}

#if	NeXT
	/*
	 * NeXT supports user-level mounting
	 */
#else
	if (geteuid() != 0) {
		fprintf(stderr, "Must be root to use mount\n");
		exit(1);
	}
#endif	NeXT

	if (all) {
#ifdef SANITY_CHECK_FSTAB
		struct stat mnttab_stat;
		long mnttab_size;
#endif
		int count;

		if (argc != 1) {
			usage();
		}
		mnttab = setmntent(NULL, "r");
		if (mnttab == NULL) {
			fprintf(stderr, 
				"mount: can't locate filesystem list\n");
			exit(1);
		}
#ifdef SANITY_CHECK_FSTAB
		if (fstat(fileno(mnttab), &mnttab_stat) == -1) {
			(void) fprintf(stderr, "mount: ");
			perror(MNTTAB);
			exit(1);
		}
		mnttab_size = mnttab_stat.st_size;
#endif
		mtree = NULL;

		for (count = 1 ; ; count++) {
		        if ((mntp = getmntent(mnttab)) == NULL) {
#ifdef SANITY_CHECK_FSTAB
				if (ftell(mnttab) >= mnttab_size)
					break;		/* it's EOF */
				(void) fprintf(stderr, 
				    "mount: %s: illegal entry on line %d\n",
				    MNTTAB, count);
				continue;
#else
				break;
#endif
		        }
			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0) ||
			    hasmntopt(mntp, MNTOPT_NOAUTO) ||
#if	NeXT
			    hasmntopt(mntp, MNTOPT_NET) ||
#endif	NeXT
			    (strcmp(mntp->mnt_dir, "/") == 0) ) {
				continue;
			}
			if (type[0] != '\0' &&
			    strcmp(mntp->mnt_type, type) != 0) {
				continue;
			}
			mtree = maketree(mtree, mntp);
		}
		endmntent(mnttab);
		mounttree(mtree);
#if	NeXT
		exit(exitflag);
#else
		exit(0);
#endif	NeXT
	}

	/*
	 * Command looks like: mount <dev>|<dir>
	 * we walk through /etc/fstab til we match either fsname or dir.
	 */
	if (argc == 2) {
#ifdef SANITY_CHECK_FSTAB
		struct stat mnttab_stat;
		long mnttab_size;
#endif
		int count;

		mnttab = setmntent(NULL, "r");
		if (mnttab == NULL) {
			fprintf(stderr, 
				"mount: can't locate filesystem list\n");
			exit(1);
		}
#ifdef SANITY_CHECK_FSTAB
		if (fstat(fileno(mnttab), &mnttab_stat) == -1) {
			(void) fprintf(stderr, "mount: ");
			perror(MNTTAB);
			exit(1);
		}
		mnttab_size = mnttab_stat.st_size;
#endif

		for (count = 1;; count++) {
			if ((mntp = getmntent(mnttab)) == NULL) {
#ifdef SANITY_CHECK_FSTAB
				if (ftell(mnttab) >= mnttab_size)
					break;		/* it's EOF */
				(void) fprintf(stderr, 
				    "mount: %s: illegal entry on line %d\n",
				    MNTTAB, count);
				continue;			   
#else
				break;
#endif
		        }
			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
#if	NeXT
			    hasmntopt(mntp, MNTOPT_NET) ||
#endif	NeXT
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0) ) {
				continue;
			}
			if ((strcmp(mntp->mnt_fsname, argv[1]) == 0) ||
			    (strcmp(mntp->mnt_dir, argv[1]) == 0) ) {
				if (opts[0] != '\0') {
					/*
					 * "-o" specified; override fstab with
					 * command line options, unless it's
					 * "-o remount", in which case do
					 * nothing if the fstab says R/O (you
					 * can't remount from R/W to R/O, and
					 * remounting from R/O to R/O is not
					 * only invalid but pointless).
					 */
					if (strcmp(opts, MNTOPT_REMOUNT) == 0
					  && hasmntopt(mntp, MNTOPT_RO))
						exit(0);
					mntp->mnt_opts = opts;
				}
				replace_opts(mntp->mnt_opts, ro, MNTOPT_RO,
				    MNTOPT_RW);
				if (strcmp(type, MNTTYPE_43) == 0)
					replace_opts(mntp->mnt_opts, quota,
					    MNTOPT_QUOTA, MNTOPT_NOQUOTA);
#if	NeXT
				exit(mounttree(maketree((struct mnttree *)NULL,
				    mntp)));
#else
				mounttree(maketree((struct mnttree *)NULL,
				    mntp));
				exit(0);
#endif	NeXT
			}
		}
		fprintf(stderr, "mount: %s not found\n", argv[1]);
#if	NeXT
		exit(1);
#else
		exit(0);
#endif	NeXT
	}

	if (argc != 3) {
		usage();
	}
#if	NeXT
	if (vmount_realpath(argv[2], dir) == NULL) {
#else
	if (realpath(argv[2], dir) == NULL) {
#endif	NeXT
		(void) fprintf(stderr, "mount: ");
		perror(dir);
		exit(1);
	}
	(void) strcpy(name, argv[1]);


	/*
	 * If the file system type is not given then
	 * assume "nfs" if the name is of the form
	 *     host:path
	 * otherwise assume "4.3".
	 */
	if (type[0] == '\0')
		(void) strcpy(type,
		    index(name, ':') ? MNTTYPE_NFS : MNTTYPE_43);


	if (dir[0] != '/') {
#if	NeXT
		char tmpdir[MNTMAXSTR];

		/*
		 * Prepend our current working directory to the path
		 */
		strcpy(tmpdir, dir);
		if (getwd(dir) == NULL) {
			fprintf(stderr, "mount: getwd: %s\n", dir);
		}
		strcat(dir, "/");
		strcat(dir, tmpdir);
#else
		fprintf(stderr, "mount: directory path must begin with '/'\n");
		exit(1);
#endif	NeXT
	}

	replace_opts(opts, ro, MNTOPT_RO, MNTOPT_RW);
	if (strcmp(type, MNTTYPE_43) == 0)
		replace_opts(opts, quota, MNTOPT_QUOTA, MNTOPT_NOQUOTA);

	if (strcmp(type, MNTTYPE_NFS) == 0) {
		passno = 0;
		freq = 0;
	}

	mnt.mnt_fsname = name;
	mnt.mnt_dir = dir;
	mnt.mnt_type = type;
	mnt.mnt_opts = opts;
	mnt.mnt_freq = freq;
	mnt.mnt_passno = passno;
#if	NeXT
	exit (mounttree(maketree((struct mnttree *)NULL, &mnt)));
#else
	mounttree(maketree((struct mnttree *)NULL, &mnt));
#endif	NeXT
}

/*
 * attempt to mount file system, return errno or 0
 */
int
mountfs(print, mnt, opts)
	int print;
	struct mntent *mnt;
	char *opts;
{
	int error;
	extern int errno;
	int flags = 0;
	union data {
		struct ufs_args	ufs_args;
		struct nfs_args nfs_args;
#ifdef NeXT
		struct pc_args 	pc_args;
		struct cfs_args cfs_args;
#endif
	} data;

#if	NeXT
	if (strcmp(mnt->mnt_type, "nfs") == 0 && 
	    hasmntopt(mnt, MNTOPT_NET) != NULL) {
		if (preparenetmount(mnt, print)) {
			return (0);
		}
	}
#endif	NeXT
	if (hasmntopt(mnt, MNTOPT_REMOUNT) == 0) {
		if (mounted(mnt)) {
#if	NeXT
			exitflag = 1;
#endif	NeXT
			if (print && verbose) {
				(void) fprintf(stderr,
				    "mount: %s already mounted\n",
				    mnt->mnt_fsname);
			}
			return (0);
		}
	} else
		if (print && verbose)
			(void) fprintf (stderr,
			    "mountfs: remount ignoring mtab\n");
	if (fake) {
		addtomtab(mnt);
		return (0);
	}
	if (strcmp(mnt->mnt_type, MNTTYPE_43) == 0) {
		error = mount_43(mnt, &data.ufs_args);
	} else if (strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0) {
		error = mount_nfs(mnt, &data.nfs_args);
#ifdef NeXT
	} else if (strcmp(mnt->mnt_type, MNTTYPE_PC) == 0) {
		error = mount_pc(mnt, &data.pc_args);
	} else if (strcmp(mnt->mnt_type, MNTTYPE_CFS) == 0) {
		error = mount_cfs(mnt, &data.cfs_args);
#endif NeXT
	} else {
		/*
		 * Invoke "mount" command for particular file system type.
		 */
		char mountcmd[128];
		int pid;
		union wait status;

		pid = fork();
		switch (pid) {

		case -1:
			(void) fprintf(stderr,
			    "mount: %s on ", mnt->mnt_fsname);
			perror(mnt->mnt_dir);
			return (errno);
		case 0:
			(void) sprintf(mountcmd, "mount_%s", mnt->mnt_type);
			execlp(mountcmd, mountcmd, mnt->mnt_fsname,
			    mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts,
			    (char *)NULL);
			if (errno == ENOENT) {
				(void) fprintf(stderr,
				    "mount: unknown filesystem type: %s\n",
				    mnt->mnt_type);
			} else if (print) {
				(void) fprintf(stderr, "%s: %s on %s ",
				    mountcmd, mnt->mnt_fsname,
				    mnt->mnt_dir);
				perror("exec failed");
			}
			exit(errno);
		default:
			while (wait(&status) != pid);
			error = status.w_retcode;
			if (!error) {
				*mnt->mnt_opts = '\0';
				goto itworked;
			}
		}
	}
	if (error) {
#if	NeXT
		exitflag = 1;
#endif	NeXT
		return (error);
	}

	flags |= eatmntopt(mnt, MNTOPT_RO) ? M_RDONLY : 0;
	flags |= eatmntopt(mnt, MNTOPT_NOSUID) ? M_NOSUID : 0;
	flags |= eatmntopt(mnt, MNTOPT_GRPID) ? M_GRPID : 0;
	flags |= eatmntopt(mnt, MNTOPT_REMOUNT) ? M_REMOUNT : 0;
	flags |= M_NEWTYPE;

#if	NeXT
	if (mount(mnt->mnt_type, mnt->mnt_dir, flags, &data) < 0) {
#else
	if (mount(type, mnt->mnt_dir, flags, &data) < 0) {
#endif	NeXT
		if (errno == ENODEV) {
			/*
			 * might be an old kernel, need to try again
			 * with file type number instead of string
			 */
#define	MAXTYPE 3
			int typeno;
			static char *typetab[] =
			    { MNTTYPE_43, MNTTYPE_NFS, MNTTYPE_PC };

			for (typeno = 0; typeno < MAXTYPE; typeno++) {
				if (strcmp(typetab[typeno], mnt->mnt_type) == 0)
					break;
			}
			if (typeno < MAXTYPE) {
				error = errno;
				if (mount(typeno, mnt->mnt_dir, flags, &data)
				    >= 0) {
					goto itworked;
				}
				errno = error;  /* restore original error */
			}
		}
#if	NeXT
		exitflag = 1;
#endif	NeXT
		if (print) {
			fprintf(stderr, "mount: %s on ", mnt->mnt_fsname);
			perror(mnt->mnt_dir);
		}
		return (errno);
	}

itworked:
	fixopts(mnt, opts);
	if (*opts) {
		(void) fprintf(stderr,
	    "mount: %s on %s WARNING unknown or unsupported option(s) %s\n",
		    mnt->mnt_fsname, mnt->mnt_dir, opts);
	}
#if	NeXT
	if (! nomtab)
		addtomtab(mnt);
#else
	addtomtab(mnt);
#endif	NeXT
	return (0);
}

mount_43(mnt, args)
	struct mntent *mnt;
	struct ufs_args *args;
{
	static char name[MNTMAXSTR];

	strcpy(name, mnt->mnt_fsname);
	args->fspec = name;
	return (0);
}

mount_nfs(mnt, args)
	struct mntent *mnt;
	struct nfs_args *args;
{
	static struct sockaddr_in sin;
	struct hostent *hp;
	static struct fhstatus fhs;
	char *cp;
	char *hostp = host;
	char *path;
	int s;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	u_short port;
	long	mnttimeo;

	cp = mnt->mnt_fsname;
	while ((*hostp = *cp) != ':') {
		if (*cp == '\0') {
			fprintf(stderr,
			    "mount: nfs file system; use host:path\n");
			return (1);
		}
		hostp++;
		cp++;
	}
	*hostp = '\0';
	path = ++cp;
	/*
	 * Get server's address
	 */
	if ((hp = gethostbyname(host)) == NULL) {
		/*
		 * XXX
		 * Failure may be due to yellow pages, try again
		 */
		if ((hp = gethostbyname(host)) == NULL) {
			fprintf(stderr,
			    "mount: %s not in hosts database\n", host);
			return (1);
		}
	}

	args->flags = 0;
	if (eatmntopt(mnt, MNTOPT_SOFT)) {
		args->flags |= NFSMNT_SOFT;
	}
	if (eatmntopt(mnt, MNTOPT_INTR)) {
		args->flags |= NFSMNT_INT;
	}
	if (eatmntopt(mnt, MNTOPT_SECURE)) {
		args->flags |= NFSMNT_SECURE;
	}
	if (eatmntopt(mnt, "noac")) {
		args->flags |= NFSMNT_NOAC;
	}

	/*
	 * get fhandle of remote path from server's mountd
	 */
	if ((mnttimeo = (long) nopt(mnt, "mnttimeo")) == 0) {
		mnttimeo = 20L;		/* XXX */
	}
	bzero((char *)&sin, sizeof(sin));
	bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
	sin.sin_family = AF_INET;
	s = socket_open(&sin, MOUNTPROG, MOUNTVERS, mnttimeo, MOUNT_TRIES);
	if (s < 0) {
		return (ETIMEDOUT);
	}
	timeout.tv_usec = ((mnttimeo % MOUNT_TRIES) * 1000000) / MOUNT_TRIES;
	timeout.tv_sec = (mnttimeo / MOUNT_TRIES);
	if ((client = clntudp_create(&sin, (u_long)MOUNTPROG,
	    (u_long)MOUNTVERS, timeout, &s)) == NULL) {
		if (!printed) {
			fprintf(stderr, "mount: %s server not responding",
			    mnt->mnt_fsname);
			clnt_pcreateerror("");
			printed = 1;
		}
		close(s);
		return (ETIMEDOUT);
	}
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = mnttimeo;
	rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &path,
	    xdr_fhstatus, &fhs, timeout);
	errno = 0;
	if (rpc_stat != RPC_SUCCESS) {
		if (!printed) {
			fprintf(stderr, "mount: %s server not responding",
			    mnt->mnt_fsname);
			clnt_perror(client, "");
			printed = 1;
		}
		switch (rpc_stat) {
		case RPC_TIMEDOUT:
		case RPC_PMAPFAILURE:
		case RPC_PROGNOTREGISTERED:
			errno = ETIMEDOUT;
			break;
		case RPC_AUTHERROR:
			errno = EACCES;
			break;
		default:
			errno = ERPC;
			break;
		}
	}
	clnt_destroy(client);
	if (errno) {
		return(errno);
	}

	if (errno = fhs.fhs_status) {
		if (errno == EACCES) {
			fprintf(stderr, "mount: access denied for %s:%s\n",
			    host, path);
		} else {
			fprintf(stderr, "mount: ");
			perror(mnt->mnt_fsname);
		}
		return (errno);
	}
	if (printed) {
		fprintf(stderr, "mount: %s server ok\n", mnt->mnt_fsname);
		printed = 0;
	}

	/*
	 * set mount args
	 */
	args->fh = (caddr_t)&fhs.fhs_fh;
	args->hostname = host;
	args->flags |= NFSMNT_HOSTNAME;
	if (args->rsize = nopt(mnt, "rsize")) {
		args->flags |= NFSMNT_RSIZE;
	}
	if (args->wsize = nopt(mnt, "wsize")) {
		args->flags |= NFSMNT_WSIZE;
	}
	if (args->timeo = nopt(mnt, "timeo")) {
		args->flags |= NFSMNT_TIMEO;
	}
	if (args->retrans = nopt(mnt, "retrans")) {
		args->flags |= NFSMNT_RETRANS;
	}
	if (args->acregmin = nopt(mnt, "actimeo")) {
		args->acregmax = args->acregmin;
		args->acdirmin = args->acregmin;
		args->acdirmax = args->acregmin;
		args->flags |= NFSMNT_ACREGMIN;
		args->flags |= NFSMNT_ACREGMAX;
		args->flags |= NFSMNT_ACDIRMIN;
		args->flags |= NFSMNT_ACDIRMAX;
	} else {
		if (args->acregmin = nopt(mnt, "acregmin")) {
			args->flags |= NFSMNT_ACREGMIN;
		}
		if (args->acregmax = nopt(mnt, "acregmax")) {
			args->flags |= NFSMNT_ACREGMAX;
		}
		if (args->acdirmin = nopt(mnt, "acdirmin")) {
			args->flags |= NFSMNT_ACDIRMIN;
		}
		if (args->acdirmax = nopt(mnt, "acdirmax")) {
			args->flags |= NFSMNT_ACDIRMAX;
		}
	}
#ifdef SECURE_RPC
	if (args->flags & NFSMNT_SECURE) {
		/*
		 * XXX: need to support other netnames outside domain
		 * and not always just use the default conversion
		 */
		if (!host2netname(netname, host, (char *)NULL)) {
			return (EHOSTDOWN); /* really unknown host */
		}
		args->netname = netname;
	}
#endif SECURE_RPC
	if (port = nopt(mnt, "port")) {
		sin.sin_port = htons(port);
	} else {
		sin.sin_port = htons(NFS_PORT);	/* XXX should use portmapper */
	}
	args->addr = &sin;

	/*
	 * should clean up mnt ops to not contain defaults
	 */
	return (0);
}

#ifdef NeXT
mount_pc(mnt, args)
	struct mntent *mnt;
	struct pc_args *args;
{
	args->fspec = mnt->mnt_fsname;
	return (0);
}

mount_cfs(mnt, args)
	struct mntent *mnt;
	struct cfs_args *args;
{
	args->fspec = mnt->mnt_fsname;
	return (0);
}
#endif NeXT

printent(mnt)
	struct mntent *mnt;
{
	fprintf(stdout, "%s on %s type %s (%s)\n",
	    mnt->mnt_fsname, mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts);
}

void printmtab(outp)
	FILE *outp;
{
	FILE *mnttab;
	struct mntent *mntp;
	int maxfsname = 0;
	int maxdir = 0;
	int maxtype = 0;
	int maxopts = 0;

	/*
	 * first go through and find the max width of each field
	 */
	mnttab = setmntent(MOUNTED, "r");
	while ((mntp = getmntent(mnttab)) != NULL) {
		if (strlen(mntp->mnt_fsname) > maxfsname) {
			maxfsname = strlen(mntp->mnt_fsname);
		}
		if (strlen(mntp->mnt_dir) > maxdir) {
			maxdir = strlen(mntp->mnt_dir);
		}
		if (strlen(mntp->mnt_type) > maxtype) {
			maxtype = strlen(mntp->mnt_type);
		}
		if (strlen(mntp->mnt_opts) > maxopts) {
			maxopts = strlen(mntp->mnt_opts);
		}
	}
	endmntent(mnttab);

	/*
	 * now print them oput in pretty format
	 */
	mnttab = setmntent(MOUNTED, "r");
	while ((mntp = getmntent(mnttab)) != NULL) {
		fprintf(outp, "%-*s", maxfsname+1, mntp->mnt_fsname);
		fprintf(outp, "%-*s", maxdir+1, mntp->mnt_dir);
		fprintf(outp, "%-*s", maxtype+1, mntp->mnt_type);
		fprintf(outp, "%-*s", maxopts+1, mntp->mnt_opts);
		fprintf(outp, " %d %d\n", mntp->mnt_freq, mntp->mnt_passno);
	}
	endmntent(mnttab);
	return;
}

/*
 * Check to see if mntck is already mounted.
 * We have to be careful because getmntent modifies its static struct.
 */
mounted(mntck)
	struct mntent *mntck;
{
	int found = 0;
	struct mntent *mnt, mntsave;
	FILE *mnttab;

	mnttab = setmntent(MOUNTED, "r");
	if (mnttab == NULL) {
		fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	mntcp(mntck, &mntsave);
	while ((mnt = getmntent(mnttab)) != NULL) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) {
			continue;
		}
		if ((strcmp(mntsave.mnt_fsname, mnt->mnt_fsname) == 0) &&
		    (strcmp(mntsave.mnt_dir, mnt->mnt_dir) == 0) &&
		    (strcmp(mntsave.mnt_opts, mnt->mnt_opts) == 0) ) {
			found = 1;
			break;
		}
	}
	endmntent(mnttab);
	*mntck = mntsave;
	return (found);
}

mntcp(mnt1, mnt2)
	struct mntent *mnt1, *mnt2;
{
	static char fsname[128], dir[128], type[128], opts[128];

	mnt2->mnt_fsname = fsname;
	strcpy(fsname, mnt1->mnt_fsname);
	mnt2->mnt_dir = dir;
	strcpy(dir, mnt1->mnt_dir);
	mnt2->mnt_type = type;
	strcpy(type, mnt1->mnt_type);
	mnt2->mnt_opts = opts;
	strcpy(opts, mnt1->mnt_opts);
	mnt2->mnt_freq = mnt1->mnt_freq;
	mnt2->mnt_passno = mnt1->mnt_passno;
}

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
			fprintf(stderr, "mount: bad numeric option '%s'\n",
			    str);
		}
	}
	rmopt(mnt, opt);
	return (val);
}

/*
 * same as hasmntopt but remove the option from the option string and return
 * true or false
 */
eatmntopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	int has;

	has = (hasmntopt(mnt, opt) != NULL);
	rmopt(mnt, opt);
	return (has);
}

/*
 * remove an option string from the option list
 */
rmopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	char *str;
	char *optstart;

#if	NeXT
	/*
	 * Check for duplicate mount options and remove.
	 */
	while (optstart = hasmntopt(mnt, opt)) {
#else
	if (optstart = hasmntopt(mnt, opt)) {
#endif	NeXT
		for (str = optstart; *str != ',' && *str != '\0'; str++)
			;
		if (*str == ',') {
			str++;
		} else if (optstart != mnt->mnt_opts) {
			optstart--;
		}
		while (*optstart++ = *str++)
			;
	}
}

/*
 * mnt->mnt_ops has un-eaten opts, opts is the original opts list.
 * Set mnt->mnt_opts to the original list minus the un-eaten opts.
 * Set "opts" to the un-eaten opts minus the "default" options ("rw",
 * "hard", "noquota", "noauto") and "bg".  If there are any options left after
 * this, they are uneaten because they are unknown; our caller will print a
 * warning message.
 */
fixopts(mnt, opts)
	struct mntent *mnt;
	char *opts;
{
	char *comma;
	char *ue;
	char uneaten[1024];

	rmopt(mnt, MNTOPT_RW);
	rmopt(mnt, MNTOPT_HARD);
	rmopt(mnt, MNTOPT_NOQUOTA);
	rmopt(mnt, MNTOPT_NOAUTO);
	rmopt(mnt, "bg");
	(void) strcpy(uneaten, mnt->mnt_opts);
	(void) strcpy(mnt->mnt_opts, opts);
	(void) strcpy(opts, uneaten);

	for (ue = uneaten; *ue; ) {
		for (comma = ue; *comma != '\0' && *comma != ','; comma++)
			;
		if (*comma == ',') {
			*comma = '\0';
			rmopt(mnt, ue);
			ue = comma+1;
		} else {
			rmopt(mnt, ue);
			ue = comma;
		}
	}
	if (*mnt->mnt_opts == '\0') {
		(void) strcpy(mnt->mnt_opts, MNTOPT_RW);
	}
}

/*
 * update /etc/mtab
 */
addtomtab(mnt)
	struct mntent *mnt;
{
	FILE *mnted;

	mnted = setmntent(MOUNTED, "r+");
	if (mnted == NULL) {
		fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	if (addmntent(mnted, mnt)) {
		fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	endmntent(mnted);

	if (verbose) {
		fprintf(stdout, "%s mounted on %s\n",
		    mnt->mnt_fsname, mnt->mnt_dir);
	}
}

char *
xmalloc(size)
	int size;
{
	char *ret;
	
	if ((ret = (char *)malloc(size)) == NULL) {
		fprintf(stderr, "mount: ran out of memory!\n");
		exit(1);
	}
	return (ret);
}

struct mntent *
mntdup(mnt)
	struct mntent *mnt;
{
	struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = (char *)xmalloc(strlen(mnt->mnt_fsname) + 1);
	strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);
}

/*
 * Build the mount dependency tree
 */
struct mnttree *
maketree(mt, mnt)
	struct mnttree *mt;
	struct mntent *mnt;
{
	struct mnttree *newmt;

	if (mt == NULL) {
		mt = (struct mnttree *)xmalloc(sizeof (struct mnttree));
		mt->mt_mnt = mntdup(mnt);
		mt->mt_sib = NULL;
		mt->mt_kid = NULL;
	} else {
		if (substr(mt->mt_mnt->mnt_dir, mnt->mnt_dir)) {
			mt->mt_kid = maketree(mt->mt_kid, mnt);
		} else {
			(newmt = (struct mnttree *)
			 xmalloc(sizeof(struct mnttree)));
			newmt->mt_mnt = mntdup(mnt);
			newmt->mt_sib = mt;
			newmt->mt_kid = NULL;
			mt = newmt;
		}
	}
	return (mt);
}

printtree(mt)
	struct mnttree *mt;
{
	if (mt) {
		printtree(mt->mt_sib);
		printf("   %s\n", mt->mt_mnt->mnt_dir);
		printtree(mt->mt_kid);
	}
}

mounttree(mt)
	struct mnttree *mt;
{
	int error;
	int slptime;
	int forked;
	int retry;
	int firsttry;
	int background;

	if (mt) {
		char opts[1024];

		mounttree(mt->mt_sib);
		(void) strcpy(opts, mt->mt_mnt->mnt_opts);
		forked = 0;
		printed = 0;
		firsttry = 1;
		slptime = BGSLEEP;
		retry = nopt(mt->mt_mnt, "retry");
		if (retry == 0) {
			retry = NRETRY;
		}

		do {
			error = mountfs(!forked, mt->mt_mnt, opts);
			if (error != ETIMEDOUT && error != ENETDOWN &&
			    error != ENETUNREACH && error != ENOBUFS &&
			    error != ECONNREFUSED && error != ECONNABORTED) {
				break;
			}
			/*
			 * mount failed due to network problems, reset options
			 * string and retry (maybe)
			 */
			(void) strcpy(mt->mt_mnt->mnt_opts, opts);
			background = eatmntopt(mt->mt_mnt, "bg");
#if	NeXT
			(void)eatmntopt(mt->mt_mnt, "net");
#endif	NeXT
			if (!forked && background) {
				(void) fprintf(stderr,
				    "mount: backgrounding\n");
				(void) fprintf(stderr, "   %s\n",
				    mt->mt_mnt->mnt_dir);
				printtree(mt->mt_kid);
				if (fork()) {
					return;
				} else {
					forked = 1;
				}
			}
			if (!forked && firsttry) {
				fprintf(stderr, "mount: retrying\n");
				fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
				printtree(mt->mt_kid);
				firsttry = 0;
			}
			sleep(slptime);
			slptime = MIN(slptime << 1, MAXSLEEP);
		} while (retry--);

		if (!error) {
			mounttree(mt->mt_kid);
		} else {
			fprintf(stderr, "mount: giving up on:\n");
			fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
			printtree(mt->mt_kid);
		}
		if (forked) {
			exit(0);
		}
	}
#if	NeXT
	return (error);
#endif	NeXT
}

/*
 * Returns true if s1 is a pathname substring of s2.
 */
substr(s1, s2)
	char *s1;
	char *s2;
{
	while (*s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '/') {
		return (1);
	}
	return (0);
}

usage()
{
	fprintf(stdout,
	    "Usage: mount [-ravpfto [type|option]] ... [fsname] [dir]\n");
	exit(1);
}

/*
 * Returns the next option in the option string.
 */
static char *
getnextopt(p)
        char **p;
{
        char *cp = *p;
        char *retstr;

        while (*cp && isspace(*cp))
                cp++;
        retstr = cp;
        while (*cp && *cp != ',')
                cp++;
        if (*cp) {
                *cp = '\0';
                cp++;
        }
        *p = cp;
        return (retstr);
}

/*
 * "trueopt" and "falseopt" are two settings of a Boolean option.
 * If "flag" is true, forcibly set the option to the "true" setting; otherwise,
 * if the option isn't present, set it to the false setting.
 */
static void
replace_opts(options, flag, trueopt, falseopt)
	char *options;
	int flag;
	char *trueopt;
	char *falseopt;
{
	register char *f;
	char tmptopts[MNTMAXSTR];
	char *tmpoptsp;
	register int found;

	(void) strcpy(tmptopts, options);
	tmpoptsp = tmptopts;
	(void) strcpy(options, "");

	found = 0;
	for (f = getnextopt(&tmpoptsp); *f; f = getnextopt(&tmpoptsp)) {
		if (options[0] != '\0')
			(void) strcat(options, ",");
		if (strcmp(f, trueopt) == 0) {
			(void) strcat(options, f);
			found++;
		} else if (strcmp(f, falseopt) == 0) {
			if (flag)
				(void) strcat(options, trueopt);
			else
				(void) strcat(options, f);
			found++;
		} else
			(void) strcat(options, f);
        }
	if (!found) {
		if (options[0] != '\0')
			(void) strcat(options, ",");
		(void) strcat(options, flag ? trueopt : falseopt);
	}
}
#if	NeXT
/*
 * Net mounts look like this:
 *
 * $host:$dir on $netdir
 *
 * What should happen is the following: 
 *
 * If $host is the local host, then the following symlink is created (if
 * it does not exist already):
 *
 *	$netdir/$host -> /
 *
 * Else, $host:$dir is mounted on $netdir/$host/$dir. Directories are
 * created as needed to make the mount succeed.
 *
 * An assumption is that the /Net directory is cleared at boot time.
 * 
 */
preparenetmount(mnt, print)
	struct mntent *mnt;
	int print;
{
	static char hostname[MAXHOSTNAMELEN + 1];
	char *netpath;
	char *olddir;
	char *p;
	char *selfpath;

	if (hostname[0] == 0) {
		gethostname(hostname, sizeof(hostname));
	}
	p = index(mnt->mnt_fsname, ':');
	if (p == NULL) {
		/*
		 * mnt->mnt_fsname already parsed by mount_nfs(): should
		 * never happen
		 */
		return (0);
	}
	netpath = malloc(strlen(mnt->mnt_dir) + strlen(mnt->mnt_fsname) + 1);
	*p = 0;
	sprintf(netpath, "%s/%s%s", mnt->mnt_dir, mnt->mnt_fsname, p + 1);
	olddir = mnt->mnt_dir;
	mnt->mnt_dir = netpath;
	if (strcmp(mnt->mnt_fsname, hostname) == 0) {
		*p = ':';
		free(olddir);
		selfpath = malloc(strlen(olddir) + 1 + strlen(hostname) + 1);
		sprintf(selfpath, "%s/%s", olddir, hostname);
		if (symlink("/", selfpath) != 0) {
			if (errno != EEXIST) {
				fprintf(stderr, "cannot link %s to /:",
					selfpath);
				free(selfpath);
				perror("");
				return (0);
			}
		}
		if (print && verbose) {
			printf("%s linked to /\n", selfpath);
		}
		free(netpath);
		mnt->mnt_dir = selfpath;
		return (1);
	} 
	*p = ':';
	p = &netpath[strlen(olddir) + 1];
	for (;;) {
		p = index(p, '/');
		if (p == NULL) {
			eatmntopt(mnt, MNTOPT_NET);
			return (0);
		}
		if (p[1] == 0) {
			*p = 0;
			eatmntopt(mnt, MNTOPT_NET);
			return (0);
		}
		*p = 0;
		if (mkdir(netpath, 0755) != 0) {
			*p = '/';
			if (errno != EEXIST) {
				fprintf(stderr, "cannot create directory %s:",
					netpath);
				perror("");
				free(olddir);
				free(netpath);
				return (0);
			}
		}
		*p++ = '/';
	}

}

char *
vmount_realpath(
		char *in,
		char *out
		)
{
	char *slash;
	struct stat st;
	
	if (stat(in, &st) == 0) {
		return (realpath(in, out));
	}
	if (errno != ENOENT) {
		return (realpath(in, out));
	}
	slash = rindex(in, '/');
	if (slash == NULL) {
		strcpy(out, in);
		return (out);
	}
	*slash = 0;
	if (realpath(in, out) == NULL) {
		*slash = '/';
		return (NULL);
	} else {
		*slash = '/';
		strcat(out, slash);
		return (out);
	}
}


#endif	NeXT


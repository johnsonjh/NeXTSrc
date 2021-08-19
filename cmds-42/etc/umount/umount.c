/*
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n"
"All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)umount.c	1.2 88/03/15 4.0NFSSRC";	
/* from 5.1 (Berkeley) 5/28/85 */
#endif not lint

/*
 * umount
 *
 * 8/9/90	Simson Garfinkel at NeXT
 *		Modified to keep track of number of failed mounts.  This
 *		number is returned as an error code, unless the user
 *		asked to unmount all files.
 */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <stdio.h>
#include <mntent.h>
#include <errno.h>
#include <sys/time.h>
#include <rpc/rpc.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 * This structure is used to build a list of mntent structures
 * in reverse order from /etc/mtab.
 */
struct mntlist {
	struct mntent *mntl_mnt;
	struct mntlist *mntl_next;
};

int	all = 0;
int	verbose = 0;
int	host = 0;
int	type = 0;

char	*typestr;
char	*hoststr;

char	*xmalloc();
char	*index();
char	*realpath() ;
struct mntlist *mkmntlist();
struct mntent *mntdup();

struct stat statb;

int eachreply();

extern	int errno;

#ifdef NeXT_MOD
int	failed_unmounts=0;
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	char 	*options;

	argc--, argv++;
	sync();
	umask(0);
	while (argc && *argv[0] == '-') {
		options = &argv[0][1];
		while (*options) {
			switch (*options) {
			case 'a':
				all++;
				break;
			case 'h':
				all++;
				host++;
				hoststr = argv[1];
				argc--;
				argv++;
				break;
			case 't':
				all++;
				type++;
				typestr = argv[1];
				argc--;
				argv++;
				break;
			case 'v':
				verbose++;
				break;
			default:
				fprintf(stderr,"umount: unknown option '%c'\n",
				    *options);
				usage();
			}
			options++;
		}
		argv++;
		argc--;
	}

	if ((all && argc) || (!all && !argc)){
		usage();
	}

#ifndef NeXT_NFS
	/*
	 * Because of the optical, we allow user level mounting and
	 * unmounting.
	 */
	if (geteuid() != 0) {
		fprintf(stderr, "Must be root to use umount\n");
		exit(1);
	}
#endif NeXT_NFS
#ifdef NeXT_MOD
	umountlist(argc, argv);
	exit(all ? 0 : failed_unmounts);
#endif
}

umountlist(argc, argv)
	int argc;
	char *argv[];
{
	int i, pid;
	struct mntent *mnt;
	struct mntlist *mntl;
	struct mntlist *mntcur = NULL;
	struct mntlist *mntrev = NULL;
	int tmpfd;
	char *colon;
	FILE *tmpmnt;
#ifdef NeXT_MOD
	char linkname[MAXPATHLEN+1];
	char tmpname[MAXPATHLEN+1];
	extern char *lnresolve();
	int fails=0;
#else
	char *tmpname = "/etc/umountXXXXXX";
#endif NeXT_MOD

#ifdef NeXT_MOD
	strcpy(linkname, lnresolve(MOUNTED));
	strcpy(tmpname, linkname);
	if (strlen(tmpname)+6 > MAXPATHLEN) {
		fprintf(stderr, "path \"%s\" too long\n", tmpname);
		exit(1);
	}
	strcat(tmpname, "XXXXXX");
#endif NeXT_MOD
	mktemp(tmpname);
	if ((tmpfd = open(tmpname, O_RDWR|O_CREAT|O_TRUNC, 0664)) < 0) {
		perror(tmpname);
		exit(1);
	}
	close(tmpfd);
	tmpmnt = setmntent(tmpname, "w");
	if (tmpmnt == NULL) {
		perror(tmpname);
		exit(1);
	}
	if (all) {
		if (!host &&
		    (!type || (type && strcmp(typestr, MNTTYPE_NFS) == 0))) {
			pid = fork();
			if (pid < 0)
				perror("umount: fork");
			if (pid == 0) {
				endmntent(tmpmnt);
				clnt_broadcast(MOUNTPROG,
				    MOUNTVERS, MOUNTPROC_UMNTALL,
				    xdr_void, NULL, xdr_void, NULL, eachreply);
				exit(0);
			}
		}
	}
	/*
	 * get a last first list of mounted stuff, reverse list and
	 * null out entries that get unmounted.
	 */
	for (mntl = mkmntlist(); mntl != NULL;
	    mntcur = mntl, mntl = mntl->mntl_next,
	    mntcur->mntl_next = mntrev, mntrev = mntcur) {
		mnt = mntl->mntl_mnt;
		if (strcmp(mnt->mnt_dir, "/") == 0) {
			continue;
		}
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) {
			continue;
		}
		if (all) {
			if (type && strcmp(typestr, mnt->mnt_type)) {
				continue;
			}
			if (host) {
				if (strcmp(MNTTYPE_NFS, mnt->mnt_type)) {
					continue;
				}
				colon = index(mnt->mnt_fsname, ':');
				if (colon) {
					*colon = '\0';
					if (strcmp(hoststr, mnt->mnt_fsname)) {
						*colon = ':';
						continue;
					}
					*colon = ':';
				} else {
					continue;
				}
			}
			if (umountmnt(mnt)) {
				mntl->mntl_mnt = NULL;
			}
			continue;
		}

		for (i=0; i<argc; i++) {
		   if (*argv[i]) {
			if ((strcmp(mnt->mnt_dir, argv[i]) == 0) ||
			    (strcmp(mnt->mnt_fsname, argv[i]) == 0)) {
				if (umountmnt(mnt)) {
					mntl->mntl_mnt = NULL;
				}
				*argv[i] = '\0';
				break;
			}
		    }
		}
	}

	for (i = 0 ; i < argc ; i++) {
	    if (*argv[i]) break ;
	}
	if (i < argc) {
	    struct mntlist *temp, *previous;
	    int oldi ;
	    oldi = i ;
	    /*
	     * Now find those arguments which are links, resolve them
	     * and then umount them. This is done separately because it
	     * "stats" the arg, and it may hang if the server is down.
	     */
	    previous = NULL ;
	    for (; mntcur; temp = mntcur, mntcur = mntcur->mntl_next,
		 temp->mntl_next = previous, previous = temp) ;

	    mntrev = NULL ;
	    for (mntl = previous; mntl != NULL;
		mntcur = mntl, mntl = mntl->mntl_next,
		mntcur->mntl_next = mntrev, mntrev = mntcur) {
		if ((mnt = mntl->mntl_mnt)==NULL) 
			continue; /* Already unmounted */
		if (strcmp(mnt->mnt_dir, "/") == 0)
			continue;
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0)
			continue;
		for (i=oldi; i<argc; i++) {
		    char resolvebuf[MAXPATHLEN] ;
		    char *resolve = resolvebuf ;

		    if (*argv[i]) {
			if (realpath(argv[i], resolve) == 0) {
				fprintf(stderr, "umount: ") ;
				perror(resolve) ;
				*argv[i] = NULL ;
#ifdef NeXT_MOD
				failed_unmounts++;
#endif
				continue ;
			}
			if ((strcmp(mnt->mnt_dir, resolve) == 0)) {
				if (umountmnt(mnt)) {
					mntl->mntl_mnt = NULL;
				}
				*argv[i] = '\0';
				break;
			}
		    }
		}
	    }
	    /*
	     * For all the remaining ones including the ones which have
	     * no corresponding entry in /etc/mtab file.
	     */
	    for (i=oldi; i<argc; i++) {
		if (*argv[i] && *argv[i] != '/') {
#if	NeXT_MOD
			char *tmparg = (char *)malloc(BUFSIZ);

			if (tmparg == NULL) {
				fprintf(stderr, "umount: no memory\n");
				exit(1);
			}

			/*
			 * Prepend our current working directory to the path.
			 */
			if (getwd(tmparg) == NULL) {
				fprintf(stderr, "umount: getwd: %s\n",
					tmparg);
				exit(1);
			}
			strcat(tmparg, "/");
			strcat(tmparg, argv[i]);
			argv[i] = tmparg;

			/*
			 * I am not going to sweat the memory leak because
			 * this is a short lived program.
			 */
#else	NeXT_MOD
			fprintf(stderr,
			    "umount: must use full path, %s not unmounted\n",
			    argv[i]);
			continue;
#endif	NeXT_MOD
		}
		if (*argv[i]) {
			struct mntent tmpmnt;

			tmpmnt.mnt_fsname = NULL;
			tmpmnt.mnt_dir = argv[i];
			tmpmnt.mnt_type = MNTTYPE_43;
			umountmnt(&tmpmnt);
		}
	    }
	}

	/*
	 * Build new temporary mtab by walking mnt list
	 */
	for (; mntcur != NULL; mntcur = mntcur->mntl_next) {
		if (mntcur->mntl_mnt) {
			addmntent(tmpmnt, mntcur->mntl_mnt);
		}
	}
	endmntent(tmpmnt);

	if (rename(tmpname, linkname) < 0) {
		perror(MOUNTED);
		exit(1);
	}
}

umountmnt(mnt)
	struct mntent *mnt;
{
	if (unmount(mnt->mnt_dir) < 0) {
#ifdef NeXT_MOD
		failed_unmounts++;
#endif
		if (errno != EINVAL) {
			perror(mnt->mnt_dir);
			return(0);
		}
		fprintf(stderr, "%s not mounted\n", mnt->mnt_dir);
		return(1);
	} else {
		if (strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0) {
			rpctoserver(mnt);
#ifdef NeXT_MOD
		/*
		 * I think we should use both 4.2 and 4.3 for compatibility.
		 * 	Morris Meyer 9/9/89
		 */
		} else if (strcmp(mnt->mnt_type, MNTTYPE_42) == 0) {
#endif NeXT_MOD
		} else if (strcmp(mnt->mnt_type, MNTTYPE_43) == 0) {
#ifdef PCFS
		} else if (strcmp(mnt->mnt_type, MNTTYPE_PC) == 0) {
#endif
		} else {
			char umountcmd[128];
			int pid;
			union wait status;

			/*
			 * user-level filesystem...attempt to exec
			 * external umount_FS program.
			 */
			pid = fork();
			switch (pid) {
			case -1:
				fprintf(stderr, "umount: ");
				perror(mnt->mnt_fsname);
				break;
			case 0:
				sprintf(umountcmd, "umount_%s", mnt->mnt_type);
				(void) execlp(umountcmd, umountcmd,
				    mnt->mnt_fsname, mnt->mnt_dir,
				    mnt->mnt_type, mnt->mnt_opts, 0);
				/*
				 * Don't worry if there is no user-written
				 * umount_FS program.
				 */
				exit(1);
			default:
				while (wait(&status) != pid);
				/*
				 * Ignore errors in user-written umount_FS.
				 */
			}
		}
		if (verbose) {
			fprintf(stderr, "%s: Unmounted\n", mnt->mnt_dir);
		}
		return(1);
	}
}

usage()
{
	fprintf(stderr, "usage: umount -a[v] [-t <type>] [-h <host>]\n");
	fprintf(stderr, "       umount [-v] <path> | <dev> ...\n");
	exit(1);
}

rpctoserver(mnt)
	struct mntent *mnt;
{
	char *p;
	struct sockaddr_in sin;
	struct hostent *hp;
	int s;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
		
	if ((p = index(mnt->mnt_fsname, ':')) == NULL)
		return;
	*p++ = 0;
	if ((hp = gethostbyname(mnt->mnt_fsname)) == NULL) {
		fprintf(stderr, "%s not in hosts database\n", mnt->mnt_fsname);
		return;
	}
	bzero(&sin, sizeof(sin));
	bcopy(hp->h_addr, (char *) & sin.sin_addr, hp->h_length);
	sin.sin_family = AF_INET;
	s = RPC_ANYSOCK;
	timeout.tv_usec = 0;
	timeout.tv_sec = 10;
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s)) == NULL) {
		clnt_pcreateerror("Warning: umount");
		return;
	}
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 25;
	rpc_stat = clnt_call(client, MOUNTPROC_UMNT, xdr_path, &p,
	    xdr_void, NULL, timeout);
	if (rpc_stat != RPC_SUCCESS) {
		clnt_perror(client, "Warning: umount:");
		return(1);
	}
}

/*ARGSUSED*/
eachreply(resultsp, addrp)
	char *resultsp;
	struct sockaddr_in *addrp;
{

	return (1);
}

char *
xmalloc(size)
	int size;
{
	char *ret;
	
	if ((ret = (char *)malloc(size)) == NULL) {
		fprintf(stderr, "umount: ran out of memory!\n");
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

struct mntlist *
mkmntlist()
{
	FILE *mounted;
	struct mntlist *mntl;
	struct mntlist *mntst = NULL;
	struct mntent *mnt;

	mounted = setmntent(MOUNTED, "r");
	if (mounted == NULL) {
		perror(MOUNTED);
		exit(1);
	}
	while ((mnt = getmntent(mounted)) != NULL) {
		mntl = (struct mntlist *)xmalloc(sizeof(*mntl));
		mntl->mntl_mnt = mntdup(mnt);
		mntl->mntl_next = mntst;
		mntst = mntl;
	}
	endmntent(mounted);
	return(mntst);
}

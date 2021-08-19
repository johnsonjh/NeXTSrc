#ifndef lint
static char sccsid[] = 	"@(#)rpc.mountd.c	1.2 88/07/14 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc. 
 */
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <exportent.h>

#define MAXRMTABLINELEN		(MAXPATHLEN + MAXHOSTNAMELEN + 2)

extern int errno;

char RMTAB[] = "/etc/rmtab";

int mnt();
char *exmalloc();
struct groups **newgroup();
struct exports **newexport();
void log_cant_reply();

/*
 * mountd's version of a "struct mountlist". It is the same except
 * for the added ml_pos field.
 */
struct mountdlist {
/* same as XDR mountlist */
	char *ml_name;
	char *ml_path;
	struct mountdlist *ml_nxt;
/* private to mountd */
	long ml_pos;		/* position of mount entry in RMTAB */
};

struct mountdlist *mountlist;
int nfs_portmon = 1;
char *domain;

void rmtab_load();
void rmtab_delete();
long rmtab_insert();

main(argc, argv)
	int argc;
	char **argv;
{
	SVCXPRT *transp;
	int pid;
	register int i;
	int sock;
	int proto;

	if (argc == 2) {
		if (strcmp(argv[1], "-n") == 0) {
			nfs_portmon = 0;
		} else {
			usage();
		}
	} else if (argc > 2) {
		usage();
	}

	if (issock(0)) {
		/*
		 * Started from inetd 
		 */
		sock = 0;
		proto = 0;	/* don't register with portmapper */
	} else {
		/*
		 * Started from shell, background.
		 */
		pid = fork();
		if (pid < 0) {
			perror("mountd: can't fork");
			exit(1);
		}
		if (pid) {
			exit(0);
		}

		/*
		 * Close existing file descriptors, open "/dev/null" as
		 * standard input, output, and error, and detach from
		 * controlling terminal.
		 */
		i = getdtablesize();
		while (--i >= 0)
			(void)close(i);
		(void)open("/dev/null", O_RDONLY);
		(void)open("/dev/null", O_WRONLY);
		(void)dup(1);
		i = open("/dev/tty", O_RDWR);
		if (i >= 0) {
			ioctl(i, TIOCNOTTY, (char *)0);
			(void)close(i);
		}
		pmap_unset(MOUNTPROG, MOUNTVERS);
		sock = RPC_ANYSOCK;
		proto = IPPROTO_UDP;
	}

	openlog("mountd", LOG_PID, LOG_DAEMON);

	/*
	 * Create UDP service
	 */
	if ((transp = svcudp_create(sock)) == NULL) {
		syslog(LOG_ERR, "couldn't create UDP transport");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, proto)) {
		syslog(LOG_ERR, "couldn't register MOUNTPROG");
		exit(1);
	}

	/*
	 * Create TCP service
	 */
	if ((transp = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL) {
		syslog(LOG_ERR, "couldn't create TCP transport");
		exit(1);
	}
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, 
			  IPPROTO_TCP)) {
		syslog(LOG_ERR, "couldn't register MOUNTPROG");
		exit(1);
	}

	/*
	 * Initalize the world 
	 */
	(void)yp_get_default_domain(&domain);

	/*
	 * Start serving 
	 */
	rmtab_load();
	svc_run();
	syslog(LOG_ERR, "Error: svc_run shouldn't have returned");
	abort();
}


/*
 * Determine if a descriptor belongs to a socket or not 
 */
issock(fd)
	int fd;
{
	struct stat st;

	if (fstat(fd, &st) < 0) {
		return (0);
	}
	/*
	 * SunOS returns S_IFIFO for sockets, while 4.3 returns 0 and does not
	 * even have an S_IFIFO mode.  Since there is confusion about what the
	 * mode is, we check for what it is not instead of what it is. 
	 */
	switch (st.st_mode & S_IFMT) {
	case S_IFCHR:
	case S_IFREG:
	case S_IFLNK:
	case S_IFDIR:
	case S_IFBLK:
		return (0);
	default:
		return (1);
	}
}

/*
 * Server procedure switch routine 
 */
mnt(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	switch (rqstp->rq_proc) {
	case NULLPROC:
		errno = 0;
		if (!svc_sendreply(transp, xdr_void, (char *)0))
			log_cant_reply(transp);
		return;
	case MOUNTPROC_MNT:
		mount(rqstp);
		return;
	case MOUNTPROC_DUMP:
		errno = 0;
		if (!svc_sendreply(transp, xdr_mountlist, (char *)&mountlist))
			log_cant_reply(transp);
		return;
	case MOUNTPROC_UMNT:
		umount(rqstp);
		return;
	case MOUNTPROC_UMNTALL:
		umountall(rqstp);
		return;
	case MOUNTPROC_EXPORT:
	case MOUNTPROC_EXPORTALL:
		export(rqstp);
		return;
	default:
		svcerr_noproc(transp);
		return;
	}
}

struct hostent *
getclientsname(transp)
	SVCXPRT *transp;
{
	struct sockaddr_in actual;
	struct hostent *hp;

	actual = *svc_getcaller(transp);
	if (nfs_portmon) {
		if (ntohs(actual.sin_port) >= IPPORT_RESERVED) {
			return (NULL);
		}
	}
	/*
	 * Don't use the unix credentials to get the machine name, instead use
	 * the source IP address. 
	 */
	hp = gethostbyaddr((char *) &actual.sin_addr, sizeof(actual.sin_addr), 
			   AF_INET);
	return (hp);
}

void
log_cant_reply(transp)
	SVCXPRT *transp;
{
	int saverrno;
	struct sockaddr_in actual;
	register struct hostent *hp;
	register char *name;

	saverrno = errno;	/* save error code */
	actual = *svc_getcaller(transp);
	/*
	 * Don't use the unix credentials to get the machine name, instead use
	 * the source IP address. 
	 */
	if ((hp = gethostbyaddr(&actual.sin_addr, sizeof(actual.sin_addr),
	   AF_INET)) != NULL)
		name = hp->h_name;
	else
		name = inet_ntoa(actual.sin_addr);

	errno = saverrno;
	if (errno == 0)
		syslog(LOG_ERR, "couldn't send reply to %s", name);
	else
		syslog(LOG_ERR, "couldn't send reply to %s: %m", name);
}

/*
 * Check mount requests, add to mounted list if ok 
 */
mount(rqstp)
	struct svc_req *rqstp;
{
	SVCXPRT *transp;
	fhandle_t fh;
	struct fhstatus fhs;
	char *path;
	struct mountdlist *ml;
	char *gr;
	char *grl;
	struct exportent *xent;
	FILE *f;
	char *machine;
	char **aliases;
	struct hostent *client;

	transp = rqstp->rq_xprt;
	f = NULL;
	path = NULL;
	client = getclientsname(transp);
	if (client == NULL) {
		fhs.fhs_status = EACCES;
		goto fail;	
	}
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}
	if (do_getfh(path, &fh) < 0) {
		if (errno == EINVAL) {
			fhs.fhs_status = EACCES;
		} else {
			fhs.fhs_status = errno;
		}
		syslog(LOG_DEBUG, "mount request: getfh failed on %s: %m",
		    path);
		goto fail;
	} else
		fhs.fhs_status = 0;
	f = setexportent();
	if (f == NULL) {
		fhs.fhs_status = EACCES;
		goto fail;
	}
	while ((xent = getexportent(f)) && !issubdir(path, xent->xent_dirname))
	       ;
	if (xent == NULL) {
		fhs.fhs_status = EACCES;
		goto fail;
	}

	/* Check access list */

	grl = getexportopt(xent, ACCESS_OPT);
	if (grl == NULL) {
		goto hit;
	}
	while ((gr = strtok(grl, ":")) != NULL) {
		grl = NULL;
		if (innetgr(gr, client->h_name, (char *)NULL, domain) ||
		    strcmp(gr, client->h_name) == 0) {
			goto hit;
		}
		for (aliases = client->h_aliases; *aliases != NULL;
		     aliases++) {
			if (innetgr(gr, *aliases, (char *)NULL, domain) ||
			    strcmp(gr, *aliases) == 0) {
				goto hit;
			}
		}
	}

 	/* Check root and rw lists */
 
 	grl = getexportopt(xent, ROOT_OPT);
 	if (grl != NULL) {
 		while ((gr = strtok(grl, ":")) != NULL) {
 			grl = NULL;
 			if (strcmp(gr, client->h_name) == 0)
 				goto hit;
 		}
 	}
 	grl = getexportopt(xent, RW_OPT);
 	if (grl != NULL) {
 		while ((gr = strtok(grl, ":")) != NULL) {
 			grl = NULL;
 			if (strcmp(gr, client->h_name) == 0)
 				goto hit;
 		}
 	}
 
	fhs.fhs_status = EACCES;
	goto fail;

hit:
	fhs.fhs_fh = fh;
	machine = NULL;
	for (ml = mountlist; ml != NULL && machine == NULL; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_path, path) == 0) {
			if (strcmp(ml->ml_name, client->h_name) == 0) {
				goto found;
			}
			for (aliases = client->h_aliases; *aliases != NULL;
			     aliases++) {
				if (strcmp(ml->ml_name, *aliases) == 0) {
					goto found;
				}
			}
		}
	}
found:
	if (ml == NULL) {
		ml = (struct mountdlist *) exmalloc(sizeof(struct mountdlist));
		ml->ml_path = (char *) exmalloc(strlen(path) + 1);
		(void)strcpy(ml->ml_path, path);
		ml->ml_name = (char *) exmalloc(strlen(client->h_name) + 1);
		(void)strcpy(ml->ml_name, client->h_name);
		ml->ml_nxt = mountlist;
		ml->ml_pos = rmtab_insert(client->h_name, path);
		mountlist = ml;
	}
fail:
	if (f != NULL) {
		endexportent(f);
	}
	errno = 0;
	if (!svc_sendreply(transp, xdr_fhstatus, (char *)&fhs))
		log_cant_reply(transp);
	svc_freeargs(transp, xdr_path, &path);
}

/*
 * Remove an entry from mounted list 
 */
umount(rqstp)
	struct svc_req *rqstp;
{
	char *path;
	struct mountdlist *ml, *oldml;
	struct hostent *client;
	SVCXPRT *transp;

	transp = rqstp->rq_xprt;
	path = NULL;
	client = getclientsname(transp);
	if (client == NULL) {
		goto done;
	}
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}
	oldml = mountlist;
	for (ml = mountlist; ml != NULL;
	     oldml = ml, ml = ml->ml_nxt) {
		if (strcmp(ml->ml_path, path) == 0 &&
		    strcmp(ml->ml_name, client->h_name) == 0) {
			if (ml == mountlist) {
				mountlist = ml->ml_nxt;
			} else {
				oldml->ml_nxt = ml->ml_nxt;
			}
			rmtab_delete(ml->ml_pos);
			free(ml->ml_path);
			free(ml->ml_name);
			free((char *)ml);
			break;
		}
	}
done:
	errno = 0;
	if (!svc_sendreply(transp, xdr_void, (char *)NULL))
		log_cant_reply(transp);
	svc_freeargs(transp, xdr_path, &path);
}

/*
 * Remove all entries for one machine from mounted list 
 */
umountall(rqstp)
	struct svc_req *rqstp;
{
	struct mountdlist *ml, *oldml;
	struct hostent *client;
	SVCXPRT *transp;

	transp = rqstp->rq_xprt;
	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}
	/*
	 * We assume that this call is asynchronous and made via the portmapper
	 * callit routine.  Therefore return control immediately. The error
	 * causes the portmapper to remain silent, as apposed to every machine
	 * on the net blasting the requester with a response. 
	 */
	svcerr_systemerr(transp);
	client = getclientsname(transp);
	if (client == NULL) {
		return;
	}
	oldml = mountlist;
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_name, client->h_name) == 0) {
			if (ml == mountlist) {
				mountlist = ml->ml_nxt;
				oldml = mountlist;
			} else {
				oldml->ml_nxt = ml->ml_nxt;
			}
			rmtab_delete(ml->ml_pos);
			free(ml->ml_path);
			free(ml->ml_name);
			free((char *)ml);
		} else {
			oldml = ml;
		}
	}
}

/*
 * send current export list 
 */
export(rqstp)
	struct svc_req *rqstp;
{
	FILE *f;
	struct exportent *xent;
	struct exports *ex;
	struct exports **tail;
	char *grl;
	char *gr;
	struct groups *groups;
	struct groups **grtail;
	SVCXPRT *transp;

	transp = rqstp->rq_xprt;
	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}
	ex = NULL;
	f = setexportent();
	if (f == NULL) {
		goto done;
	}
	tail = &ex;
	while (xent = getexportent(f)) {
		grl = getexportopt(xent, ACCESS_OPT);
		groups = NULL;
		if (grl != NULL) {
			grtail = &groups;
			while ((gr = strtok(grl, ":")) != NULL) {
				grl = NULL;
				grtail = newgroup(gr, grtail);
			}
		}
		tail = newexport(xent->xent_dirname, groups, tail);
	}

done:
	errno = 0;
	if (!svc_sendreply(transp, xdr_exports, (char *)&ex))
		log_cant_reply(transp);
	if (f != NULL) {
		endexportent(f);
	}
	freeexports(ex);
}


freeexports(ex)
	struct exports *ex;
{
	struct groups *groups, *tmpgroups;
	struct exports *tmpex;

	while (ex) {
		groups = ex->ex_groups;
		while (groups) {
			tmpgroups = groups->g_next;
			free(groups->g_name);
			free((char *)groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free(ex->ex_name);
		free((char *)ex);
		ex = tmpex;
	}
}


struct groups **
newgroup(name, tail)
	char *name;
	struct groups **tail;
{
	struct groups *new;
	char *newname;

	new = (struct groups *) exmalloc(sizeof(*new));
	newname = (char *) exmalloc(strlen(name) + 1);
	(void)strcpy(newname, name);

	new->g_name = newname;
	new->g_next = NULL;
	*tail = new;
	return (&new->g_next);
}


struct exports **
newexport(name, groups, tail)
	char *name;
	struct groups *groups;
	struct exports **tail;
{
	struct exports *new;
	char *newname;

	new = (struct exports *) exmalloc(sizeof(*new));
	newname = (char *) exmalloc(strlen(name) + 1);
	(void)strcpy(newname, name);

	new->ex_name = newname;
	new->ex_groups = groups;
	new->ex_next = NULL;
	*tail = new;
	return (&new->ex_next);
}

char *
exmalloc(size)
	int size;
{
	char *ret;

	if ((ret = (char *) malloc((u_int)size)) == 0) {
		syslog(LOG_ERR, "Out of memory");
		exit(1);
	}
	return (ret);
}

usage()
{
	(void)fprintf(stderr, "usage: rpc.mountd [-n]\n");
	exit(1);
}

/*
 * Old geth() took a file descriptor. New getfh() takes a pathname.
 * So the the mount daemon can run on both old and new kernels, we try
 * the old version of getfh() if the new one fails.
 */
do_getfh(path, fh)
	char *path;
	fhandle_t *fh;
{
	int fd;
	int res;
	int save;

	res = getfh(path, fh);
	if (res < 0 && errno == EBADF) {	
		/*
		 * This kernel does not have the new-style getfh()
		 */
		fd = open(path, 0, 0);
		if (fd >= 0) {
			res = getfh((char *)fd, fh);
			save = errno;
			(void)close(fd);
			errno = save;
		}
	}
	return (res);
}


FILE *f;

void
rmtab_load()
{
	char *path;
	char *name;
	char *end;
	struct mountdlist *ml;
	char line[MAXRMTABLINELEN];

	f = fopen(RMTAB, "r");
	if (f != NULL) {
		while (fgets(line, sizeof(line), f) != NULL) {
			name = line;
			path = strchr(name, ':');
			if (*name != '#' && path != NULL) {
				*path = 0;
				path++;
				end = strchr(path, '\n');
				if (end != NULL) {
					*end = 0;
				}
				ml = (struct mountdlist *) 
					exmalloc(sizeof(struct mountdlist));
				ml->ml_path = (char *)
					exmalloc(strlen(path) + 1);
				(void)strcpy(ml->ml_path, path);
				ml->ml_name = (char *)
					exmalloc(strlen(name) + 1);
				(void)strcpy(ml->ml_name, name);
				ml->ml_nxt = mountlist;
				mountlist = ml;
			}
		}
		fclose(f);
		(void)truncate(RMTAB, (off_t)0);
	} 
#if	NeXT
	f = fopen(RMTAB, "w");
#else	NeXT
	f = fopen(RMTAB, "w+");
#endif	NeXT
	if (f != NULL) {
		setlinebuf(f);
		for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
			ml->ml_pos = rmtab_insert(ml->ml_name, ml->ml_path);
		}
	}
}


long
rmtab_insert(name, path)
	char *name;
	char *path;
{
	long pos;

	if (f == NULL || fseek(f, 0L, 2) == -1) {
		return (-1);
	}
	pos = ftell(f);
	if (fprintf(f, "%s:%s\n", name, path) == EOF) {
		return (-1);
	}
#if	NeXT
	fflush(f);
#endif	NeXT
	return (pos);
}

void
rmtab_delete(pos)
	long pos;
{
	if (f != NULL && pos != -1 && fseek(f, pos, 0) == 0) {
		fprintf(f, "#");
		fflush(f);
	}
}


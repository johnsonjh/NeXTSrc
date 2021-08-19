/*
 * nibindd - NetInfo binder
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/resource.h>
#include "clib.h"
#include "mm.h"
#include "system.h"

const char NETINFO_PROG[] = "/usr/etc/netinfod";
const char NETINFO_DIR[] = "/private/etc/netinfo";

extern void nibind_prog_1();

int isnidir(char *, ni_name *);

extern void parentexit(int);
extern void catchchild(int);
extern void killchildren(int);
extern void storepid(int, ni_name);

static fd_set get_open_mask(void);
static void closedes(fd_set *);
static void detach(void);


extern int waitreg;

void
main(
     int argc,
     char **argv
     )
{
	SVCXPRT *transp;
	DIR *dp;
	struct direct *d;
	ni_name tag;
	ni_namelist nl;
	ni_index i;
	fd_set mask;
	int pid;
	struct rlimit rlim;

	/*
	 * Fork child to be real server. Parent hangs around waiting
	 * for child to detach.
	 */
	if (fork()) {
		(void)signal(SIGTERM, parentexit);
		for (;;) {
			pause();
		}
	}

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);
	(void)signal(SIGCHLD, catchchild);
	(void)signal(SIGTERM, killchildren);
	mask = get_open_mask();
	
	/*
	 * cd to netinfo directory, find out which databases should
	 * be served and lock the directory before registering service.
	 */
	if (chdir(NETINFO_DIR) < 0) {
		detach();
		sys_panic("cannot chdir to netinfo directory");
	}
	dp = opendir(NETINFO_DIR);
	if (dp == NULL) {
		detach();
		sys_panic("cannot open netinfo directory");
	}
	MM_ZERO(&nl);
	while (d = readdir(dp)) {
		if (isnidir(d->d_name, &tag)) {
			if (ni_namelist_match(nl, tag) == NI_INDEX_NULL) {
				ni_namelist_insert(&nl, tag, NI_INDEX_NULL);
			} 
			ni_name_free(&tag);
		}
	}
	/*
	 * Do not close the directory: keep it locked so another nibindd
	 * won't run.
	 */
	if (flock(dp->dd_fd, LOCK_EX|LOCK_NB) < 0) {
		detach();
		sys_panic("nibindd already running");
	}


	/*
	 * Register service
	 */
	pmap_unset(NIBIND_PROG, NIBIND_VERS);
	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		detach();
		sys_panic("cannot start udp service");
	}
	if (!svc_register(transp, NIBIND_PROG, NIBIND_VERS, nibind_prog_1,
			  IPPROTO_UDP)) {
		detach();
		sys_panic("cannot register udp service");
	}
	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		detach();
		sys_panic("cannot start tcp service");
	}
	if (!svc_register(transp, NIBIND_PROG, NIBIND_VERS, nibind_prog_1,
			  IPPROTO_TCP)) {
		detach();
		sys_panic("cannot register tcp service");
	}

	closedes(&mask);
	for (i = 0; i < nl.ninl_len; i++) {
		pid = sys_spawn(NETINFO_PROG, nl.ninl_val[i], 0);
		if (pid >= 0)  {
			waitreg++;
			storepid(pid, nl.ninl_val[i]);
		}
	}
	ni_namelist_free(&nl);
	svc_run();
	sys_panic("svc_run returned");
	exit(1);
}

static char *suffixes[] = {
	".nidb",
	".move",
	".temp",
	NULL
};

int
isnidir(
	char *dir,
	ni_name *tag
	)
{
	char *s;
	int i;
	
	s = rindex(dir, '.');
	if (s == NULL) {
		return (0);
	}
	for (i = 0; suffixes[i] != NULL; i++) {
		if (ni_name_match(s, suffixes[i])) {
			*tag = ni_name_dup(dir);
			s = rindex(*tag, '.');
			*s = 0;
			return (1);
		}
	}
	return (0);
}



static void
closedes(
	 fd_set *mask
	 )
{
	int i;

	for (i = getdtablesize() - 1; i >= 0; i--) {
		if (FD_ISSET(i, mask)) {
			(void)close(i);
		}
	}
	open("/dev/console", O_RDONLY, 0);
	dup(0);
	dup(0);
}

static void
detach(void)
{
	int ttyfd;

	kill(getppid(), SIGTERM);
	ttyfd = open("/dev/tty", O_RDWR, 0);
	if (ttyfd > 0) {
		ioctl(ttyfd, TIOCNOTTY, (char *)0);
		close(ttyfd);
	}
}

static fd_set
get_open_mask(
	      void
	      )
{
	int i;
	struct stat st;
	fd_set mask;
	int fd;

	FD_ZERO(&mask);
	for (i = getdtablesize() - 1; i >= 3; i--) {
		if (fstat(i, &st) == 0) {
			FD_SET(i, &mask);
		} else {
			FD_CLR(i, &mask);
		}
	}

	/*
	 * Open 0, 1, 2 if they are not already
	 */
	for (i = 2; i >= 0; i--) {
		if (fstat(i, &st) != 0) {
			FD_SET(i, &mask);
			fd = open("/dev/null", O_RDONLY, 0);
			dup2(fd, i);
			close(fd);
		}
	}
	return (mask);
}

void
svc_run(void)
{
	fd_set readfds;
	int maxfds;

	maxfds = getdtablesize();
	for (;;) {
		readfds = svc_fdset;
		switch (select(maxfds, &readfds, NULL, NULL, NULL)) {
		case -1:
			if (errno != EINTR) {
				sys_panic("unexpected errno: %m, aborting");
			}
			break;
		case 0:
			break;
		default:
			svc_getreqset(&readfds);
			break;
		}
		if (waitreg == 0) {
			waitreg = -1;
			detach();
		}
	}
}

	 
void
parentexit(int x)
{
	exit(0);
}

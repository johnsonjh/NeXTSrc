/* @(#)biod.c	1.1 87/09/21 3.2/4.3NFSSRC */
#ifndef lint
static  char sccsid[] = "@(#)biod.c	1.1 88/03/08 4.0NFSSRC; from 1.6 88/02/07 Copyr 1983 Sun Micro";
#endif

#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>

/*
 * This is the NFS asynchronous block I/O daemon
 */

main(argc, argv)
	int argc;
	char *argv[];
{
	extern int errno;
	int pid;
	int count;

	if (argc > 2) {
		usage(argv[0]);
	}

	if (argc == 2) {
		count = atoi(argv[1]);
		if (count < 0) {
			usage(argv[0]);
		}
	} else {
		count = 1;
	}

	{ int tt = open("/dev/tty", O_RDWR);
		if (tt > 0) {
			ioctl(tt, TIOCNOTTY, 0);
			close(tt);
		}
	}
	while (count--) {
		pid = fork();
		if (pid == 0) {
			async_daemon();		/* Should never return */
			fprintf(stderr, "%s: async_daemon ", argv[0]);
			perror("");
			exit(1);
		}
		if (pid < 0) {
			fprintf(stderr, "%s: cannot fork", argv[0]);
			perror("");
			exit(1);
		}
	}
}

usage(name)
	char	*name;
{

	fprintf(stderr, "usage: %s [<count>]\n", name);
	exit(1);
}

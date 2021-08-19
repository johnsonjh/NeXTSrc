#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = 	"@(#)svc_run.c	1.2 88/07/27 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *  1.2 88/02/08 
 */


/*
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */
#include <rpc/rpc.h>
#if	NeXT
#include <errno.h>
#include <string.h>
#include <syslog.h>
#else	NeXT
#include <sys/errno.h>
#endif	NeXT
#include <sys/syslog.h>

void
svc_run()
{
	fd_set readfds;
#if	NeXT
#else	NeXT
	extern int errno;
#endif  NeXT

	for (;;) {
		readfds = svc_fdset;
		switch (select(_rpc_dtablesize(), &readfds, (int *)0, (int *)0,
			       (struct timeval *)0)) {
		case -1:
			if (errno == EINTR) {
				continue;
			}
#if	NeXT
			/*
			 * Keeps servers like portmap from exiting on error
			 */
			syslog(LOG_ERR, "svc_run: select failed: %m");
			continue;
#else	NeXT
			perror("svc_run: - select failed");
			return;
#endif	NeXT
		case 0:
			continue;
		default:
			svc_getreqset(&readfds);
		}
	}
}

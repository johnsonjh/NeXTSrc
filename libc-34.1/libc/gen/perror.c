/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)perror.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

/*
 * Print the error indicated
 * in the cerror cell.
 */
#include <sys/types.h>
#include <sys/uio.h>
#if	NeXT
#include <errno.h>

void perror(const char *s)
#else
extern int	errno;
extern int	sys_nerr;
extern char	*sys_errlist[];

perror(s)
        char *s;
#endif
{
	struct iovec iov[4];
	register struct iovec *v = iov;

	if (s && *s) {
		v->iov_base = (char *) s;
		v->iov_len = strlen(s);
		v++;
		v->iov_base = ": ";
		v->iov_len = 2;
		v++;
	}
#if	NeXT
	if ((errno > sys_nerr) || (errno < 0))
		v->iov_base = "Unknown error";
	else
		v->iov_base = sys_errlist[errno];
#else
	v->iov_base = errno < sys_nerr ? sys_errlist[errno] : "Unknown error";
#endif	NeXT
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = "\n";
	v->iov_len = 1;
	writev(2, iov, (v - iov) + 1);
}


#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = 	"@(#)closedir.c	1.5 88/05/10 4.0NFSSRC SMI"; /* from UCB 5.2 3/9/86 */
#endif

/*
 * From SMI 1.13
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	NeXT
#include <stdlib.h>
#include <libc.h>
#endif	NeXT
#include <sys/param.h>
#include <sys/dir.h>

/*
 * close a directory.
 */
int
closedir(dirp)
	register DIR *dirp;
{
	int fd;

	fd = dirp->dd_fd;
	dirp->dd_fd = -1;
	dirp->dd_loc = 0;
	free(dirp->dd_buf);
	free((char *)dirp);
	return (close(fd));
}

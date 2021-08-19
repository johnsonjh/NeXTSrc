/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)seekdir.c	1.2 88/04/11 4.0NFSSRC; from	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <sys/param.h>
#include <sys/dir.h>

/*
 * seek to an entry in a directory.
 * Only values returned by "telldir" should be passed to seekdir.
 */
void
seekdir(dirp, loc)
	register DIR *dirp;
	long loc;
{
	long curloc, base, entno;
	struct direct *dp;
	extern long lseek();

	curloc = telldir(dirp);
	if (loc == curloc)
		return;
        base = loc / dirp->dd_bsize;
        entno = loc % dirp->dd_bsize;
	(void) lseek(dirp->dd_fd, base, 0);
	dirp->dd_loc = 0;
        dirp->dd_entno = 0;
	while (dirp->dd_entno < entno) {
		dp = readdir(dirp);
		if (dp == NULL)
			return;
	}
}

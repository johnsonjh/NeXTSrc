/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)readdir.c	1.2 88/04/11 4.0NFSSRC; from	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <sys/param.h>
#include <sys/dir.h>

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
	register DIR *dirp;
{
	register struct direct *dp;
        extern int getdirentries();

	for (;;) {
		if (dirp->dd_loc == 0) {
                        dirp->dd_size = getdirentries(dirp->dd_fd,
                            dirp->dd_buf, dirp->dd_bsize, &dirp->dd_bbase);
			if (dirp->dd_size <= 0)
				return NULL;
                        dirp->dd_entno = 0;
		}
		if (dirp->dd_loc >= dirp->dd_size) {
			dirp->dd_loc = 0;
			continue;
		}
		dp = (struct direct *)(dirp->dd_buf + dirp->dd_loc);
		if (dp->d_reclen <= 0)
			return NULL;
		dirp->dd_loc += dp->d_reclen;
                dirp->dd_entno++;
		if (dp->d_fileno == 0)
			continue;
		return (dp);
	}
}

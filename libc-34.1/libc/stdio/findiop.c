/*
 * Copyright (c) 1983, 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)findiop.c	5.6 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "iob.h"

#define active(iop)	((iop)->_flag & (_IOREAD|_IOWRT|_IORW))

static	FILE	**iobglue;
static	FILE	**endglue;

/*
 * Find a free FILE for fopen et al.
 * We have a fixed static array of entries, and in addition
 * may allocate additional entries dynamically, up to the kernel
 * limit on the number of open files.
 * At first just check for a free slot in the fixed static array.
 * If none are available, then we allocate a structure to glue together
 * the old and new FILE entries, which are then no longer contiguous.
 */
FILE *
_findiop()
{
	register FILE **iov, *iop;
	register FILE *fp;

	if (iobglue == 0) {
		for (iop = _iob; iop < _iob + NSTATIC; iop++)
			if (!active(iop))
				return (iop);

		if (_f_morefiles() == 0) {
			errno = ENOMEM;
			return (NULL);
		}
	}

	iov = iobglue;
	while (*iov != NULL && active(*iov))
		if (++iov >= endglue) {
			errno = EMFILE;
			return (NULL);
		}

	if (*iov == NULL)
		*iov = (FILE *)calloc(1, sizeof **iov);

	return (*iov);
}

_f_morefiles()
{
	register FILE **iov;
	register FILE *fp;
	register char *cp;
	int nfiles;

	nfiles = getdtablesize();

	iobglue = (FILE **)calloc(nfiles, sizeof *iobglue);
	if (iobglue == NULL)
		return (0);

	endglue = iobglue + nfiles;

	for (fp = _iob, iov = iobglue; fp < &_iob[NSTATIC]; /* void */)
		*iov++ = fp++;

	return (1);
}

f_prealloc()
{
	register FILE **iov;
	register FILE *fp;

	if (iobglue == NULL && _f_morefiles() == 0)
		return;

	for (iov = iobglue; iov < endglue; iov++)
		if (*iov == NULL)
			*iov = (FILE *)calloc(1, sizeof **iov);
}

_fwalk(function)
	register int (*function)();
{
	register FILE **iov;
	register FILE *fp;

	if (iobglue == NULL) {
		for (fp = _iob; fp < &_iob[NSTATIC]; fp++)
			if (active(fp))
				(*function)(fp);
	} else {
		for (iov = iobglue; iov < endglue; iov++)
			if (*iov && active(*iov))
				(*function)(*iov);
	}
}

_cleanup()
{
	extern int fclose();

	_fwalk(fclose);
}

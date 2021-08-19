/*
 * Copyright (c) 1983, 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  Also
 * this file is loaded first in the shared library so that the addresses of
 * stdin, stdout and stderr are not likely to ever change (unless the sizeof
 * (FILE) changes).  Many pointers in other libraries are initialized point
 * at these and if new such pointers are added we can get by without having
 * the executable initializing these if we staticly initialize these to the
 * correct address (thus making it easier to keep the libraries compatable).
 */

#include <stdio.h>
#include "iob.h"

FILE _iob[NSTATIC] = {
	{ 0, NULL, NULL, 0, _IOREAD,		0, 0 },	/* stdin  */
	{ 0, NULL, NULL, 0, _IOWRT,		1, 0 },	/* stdout */
	{ 0, NULL, NULL, 0, _IOWRT|_IONBF,	2, 0 },	/* stderr */
};



/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dbm.h	5.1 (Berkeley) 3/27/86
 */

#ifndef NULL
/*
 * this is lunacy, we no longer use it (and never should have
 * unconditionally defined it), but, this whole file is for
 * backwards compatability - someone may rely on this.
 */
#define	NULL	((char *) 0)
#endif

#include <ndbm.h>

#ifdef __STRICT_BSD__
datum	fetch();
datum	firstkey();
datum	nextkey();
#else
datum	fetch(char *file);
datum	firstkey(void);
datum	nextkey(datum key);
void	store (datum key, datum content);
void	delete (datum key);
#endif __STRICT_BSD__
#if 0
datum	makdatum();
datum	firsthash();
long	calchash();
long	hashinc();
#endif

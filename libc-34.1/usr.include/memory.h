/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)memory.h	5.1 (Berkeley) 85/08/05
 */

/*
 * Definitions of the Sys5 compat memory manipulation routines
 */

extern char *memccpy();
extern int memcmp();
#ifdef	__STRICT_BSD__
extern char *memchr();
extern char *memcpy();
extern char *memset();
#else
extern void *memchr();
extern void *memcpy();
extern void *memset();
#endif	__STRICT_BSD__
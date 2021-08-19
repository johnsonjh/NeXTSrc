/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)strings.h	5.1 (Berkeley) 5/30/85
 */

/*
 * External function definitions
 * for routines described in string(3).
 */

#ifdef __STRICT_BSD__
extern char *strcat();
extern char *strncat();
extern int strcmp();
extern int strncmp();
extern int strcasecmp();
extern int strncasecmp();
extern char *strcpy();
extern char *strncpy();
extern int strlen();
extern char *strchr();
extern char *strrchr();
extern char *index();
extern char *rindex();
#else
#include <string.h>
#endif

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
_sccsid:.asciz	"@(#)ntohl.c	5.3 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/* hostorder = ntohl(netorder) */

#include "cframe.h"

PROCENTRY(ntohl)
	movl	a_p0,d0
	rts

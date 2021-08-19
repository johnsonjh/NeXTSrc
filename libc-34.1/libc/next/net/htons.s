/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
_sccsid:.asciz	"@(#)htons.c	5.3 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/* hostorder = htons(netorder) */

#include "cframe.h"

PROCENTRY(htons)
	movl	a_p0,d0
	andl	#0xffff,d0
	rts

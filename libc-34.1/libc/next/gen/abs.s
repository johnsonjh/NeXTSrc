/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)abs.s	5.3 (Berkeley) 3/9/86"
#endif LIBC_SCCS


/* abs - int absolute value */

#include "cframe.h"

PROCENTRY(abs)
	movl	a_p0,d0
	bpl	1f
	negl	d0
1:
	rts

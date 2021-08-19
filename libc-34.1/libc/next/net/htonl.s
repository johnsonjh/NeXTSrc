/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
_sccsid:.asciz	"@(#)htonl.c	5.3 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/* netorder = htonl(hostorder) */

#include "cframe.h"

PROCENTRY(htonl)
	movl	a_p0,d0
	rts

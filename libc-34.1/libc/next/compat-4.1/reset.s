/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef LIBC_SCCS
_sccsid:.asciz	"@(#)reset.c	5.4 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/*
 * C library -- reset, setexit
 *
 *	reset(x)
 * will generate a "return" from
 * the last call to
 *	setexit()
 * by restoring a7-a2, d7-d2
 * and doing a return.
 * The returned value is x; on the original
 * call the returned value is 0.
 *
 * useful for going back to the main loop
 * after a horrible error in a lowlevel
 * routine.
 */
#include "cframe.h"

	.lcomm	setsav,13*4

PROCENTRY(setexit)
	movl	#setsav,a0
	movl	a_ra,a0@		| save return address
	moveml	#0xfcfc,a0@(4)		| save a7-a2, d7-d2
	moveq	#0,d0			| setexit returns 0
	rts


PROCENTRY(reset)
	movl	a_p0,d0			| return value
	movl	#setsav,a0
	moveml	a0@(4),#0xfcfc		| restore a7-a2, d7-d2
	movl	a0@,sp@			| restore return address
	rts

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)frexp.s	5.3 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/* C library -- frexp(value, eptr) */

#include "cframe.h"

PROCENTRY(frexp)
	movl	sp@(12),a0		| eptr
	fmoved	sp@(4),fp0
	fgetmanx	fp0,fp1			| extract mantissa
	fscaleb	#-1,fp1			| scale to > 1
	fmoved	fp1,sp@-
	fgetexpx	fp0,fp1			| extract exponent
	fmovel	fp1,d0
	addql	#1,d0			| compensate for scaling
	movl	d0,a0@			| store exponent
	movl	sp@+,d0			| return mantissa
	movl	sp@+,d1
	rts

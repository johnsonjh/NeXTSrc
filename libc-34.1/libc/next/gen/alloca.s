/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)alloca.s	5.3 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/* like alloc, but automatic free in return */

#include "cframe.h"

PROCENTRY(alloca)
	movl	a_ra,a0		| save return address
	movl	sp,d0
	moveq	#-4,d1
	subl	d1,d0		| pop return address
	subl	a_p0,d0		| allocate space
	andl	d1,d0		| long align
	movl	d0,sp		| new sp
	addl	d1,sp		| account for param cleanup
	jmp	a0@		| return to caller

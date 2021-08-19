/*
 * Copyright (c) 1987 NeXT, INC.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strrchr.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/*
 * Find the last occurence of c in the string cp.
 * Return pointer to match or null pointer.
 *
 * char *
 * strrchr(cp, c)
 *	char *cp, c;
 */
#include "cframe.h"

PROCENTRY(strrchr)
	movl	a_p0,a0		| cp
	movl	a_p1,d1		| c
	moveq	#0,d0		| last *cp == c
1:	cmpb	a0@,d1
	bne	2f
	movl	a0,d0
2:	tstb	a0@+
	bne	1b
	rts

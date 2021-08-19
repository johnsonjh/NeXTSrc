/*
 * Copyright (c) 1987 NeXT, INC.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strncat.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/*
 * Concatenate string s2 on the end of s1
 * and return the base of s1.  The parameter
 * n is the maximum length of string s2 to
 * concatenate.
 *
 * char *
 * strncat(s1, s2, n)
 *	char *s1, *s2;
 *	int n;
 */
#include "cframe.h"

PROCENTRY(strncat)
	movl	a_p0,a0
	movl	a_p1,a1
	movl	a_p2,d1
	movl	a0,d0
1:	tstb	a0@+
	bne	1b
	subqw	#1,a0
2:	movb	a1@+,a0@+
	beq	4f
	subql	#1,d1
	bge	2b
	clrb	a0@-
4:	rts

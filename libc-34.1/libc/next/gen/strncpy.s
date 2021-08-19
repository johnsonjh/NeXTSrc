/*
 * Copyright (c) 1987 NeXT, INC.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strncpy.s	1.0 (NeXT) July 8, 1987"
#endif LIBC_SCCS

/*
 * Copy string s2 over top of string s1.
 * Truncate or null-pad to n bytes.
 *
 * char *
 * strncpy(s1, s2, n)
 *	char *s1, *s2;
 */
#include "cframe.h"

PROCENTRY(strncpy)
	movl	a_p0,a0		| dst
	movl	a_p1,a1		| src
	movl	a_p2,d1		| n
	movl	a0,d0
1:	subql	#1,d1
	blt	4f		| n exhausted
	movb	a1@+,a0@+
	bne	1b		| more string to move
	bra	3f		| clear to null until n exhausted

2:	clrb	a0@+
3:	subql	#1,d1
	bge	2b		| n not exhausted
4:	rts

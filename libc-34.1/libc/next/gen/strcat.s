/*
 * Copyright (c) 1987 NeXT, INC.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strcat.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/*
 * Concatenate string s2 to the end of s1
 * and return the base of s1.
 *
 * char *
 * strcat(s1, s2)
 *	char *s1, *s2;
 */
#include "cframe.h"

PROCENTRY(strcat)
	movl	a_p0,a0		| s1
	movl	a_p1,a1		| s2
	movl	a0,d0
1:	tstb	a0@+
	bne	1b
	subqw	#1,a0
2:	movb	a1@+,a0@+
	bne	2b
	rts

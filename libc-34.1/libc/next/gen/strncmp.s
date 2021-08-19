/*
 * Copyright (c) 1987 NeXT, INC
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strncmp.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/*
 * Compare at most n characters of string
 * s1 lexicographically to string s2.
 * Return:
 *	0	s1 == s2
 *	> 0	s1 > s2
 *	< 0	s2 < s2
 *
 * strncmp(s1, s2, n)
 *	char *s1, *s2;
 *	int n;
 */
#include "cframe.h"

PROCENTRY(strncmp)
	movl	a_p0,a0
	movl	a_p1,a1
	movl	a_p2,d1
1:	subql	#1,d1
	blt	2f
	movb	a0@+,d0
	cmpb	a1@+,d0
	bne	3f
	tstb	d0
	bne	1b
2:	moveq	#0,d0
	rts

3:	extbl	d0
	movb	a1@-,d1
	extbl	d1
	subl	d1,d0
	rts

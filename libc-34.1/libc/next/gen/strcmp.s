/*
 * Copyright (c) 1987 NeXT, INC.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strcmp.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/*
 * Compare string s1 lexicographically to string s2.
 * Return:
 *	0	s1 == s2
 *	> 0	s1 > s2
 *	< 0	s2 < s2
 *
 * strcmp(s1, s2)
 *	char *s1, *s2;
 */
#include "cframe.h"

PROCENTRY(strcmp)
	movl	a_p0,a0
	movl	a_p1,a1
1:	movb	a0@+,d0
	beq	3f
	cmpb	a1@+,d0
	bne	2f
	movb	a0@+,d0
	beq	3f
	cmpb	a1@+,d0
	beq	1b

2:	extbl	d0
	movb	a1@-,d1
	extbl	d1
	subl	d1,d0
	rts

3:	cmpb	a1@+,d0
	bne	2b
	moveq	#0,d0
	rts

/*
 * Copyright (c) 1987 NeXT, INC
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strcpy.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/*
 * Copy string s2 over top of s1.
 * Return base of s1.
 *
 * char *
 * strcpy(s1, s2)
 *	char *s1, *s2;
 */
#include "cframe.h"

PROCENTRY(strcpy)
	movl	a_p0,a0
	movl	a_p1,a1
	movl	a0,d0
1:	movb	a1@+,a0@+
	bne	1b
2:	rts

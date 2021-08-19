/*
 * Copyright (c) 1987 NeXT, INC
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strlen.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/*
 * Return the length of cp (not counting '\0').
 *
 * strlen(cp)
 *	char *cp;
 */
#include "cframe.h"

PROCENTRY(strlen)
	movl	a_p0,a0
	moveq	#-1,d0
1:	addql	#1,d0
	tstb	a0@+
	bne	1b
	rts

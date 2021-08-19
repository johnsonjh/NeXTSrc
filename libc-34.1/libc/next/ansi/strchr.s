/*
 * (c) NeXT, INC.   July 7, 1987.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)strchr.s	5.3 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/*
 * Find the first occurence of c in the string cp.
 * Return pointer to match or null pointer.
 *
 * char *
 * strchr(cp, c)
 *	char *cp, c;
 */
#include "cframe.h"

PROCENTRY(strchr)
	movl	a_p0,a0		| cp
	movl	a_p1,d0		| c
	beq	3f		| c == 0 special cased
1:	movb	a0@+,d1		| *cp++
	beq	2f		| end of string
	cmpb	d1,d0
	bne	1b		| not c
	subw	#1,a0		| undo post-increment
	movl	a0,d0
	rts

2:	moveq	#0,d0		| didnt find c
	rts

3:	tstb	a0@+
	bne	3b
	subw	#1,a0		| undo post-increment
	movl	a0,d0
	rts

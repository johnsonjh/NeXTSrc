/*
 * (c) NeXT, INC.  July 7, 1987
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)insque.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/* insque(new, pred) */

#include "cframe.h"

PROCENTRY(insque)
	movl	a_p0,a0		| new
	movl	a_p1,a1		| pred
	movl	a2,sp@-
	movl	a1@,a2		| pred->next
	movl	a2,a0@		| new->next = pred->next
	movl	a1,a0@(4)	| new->prev = pred
	movl	a0,a2@(4)	| pred->next->prev = new
	movl	a0,a1@		| pred->next = new
	movl	sp@+,a2
	rts

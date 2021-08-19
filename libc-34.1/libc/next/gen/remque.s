/*
 * (c) NeXT, INC. July 7, 1987.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)remque.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/* remque(entry) */

#include "cframe.h"

PROCENTRY(remque)
	movl	a_p0,a0
	movl	a0@(4),a1	| pred = entry->prev
	movl	a0@,a1@		| pred->next = entry->next
	movl	a0@,a0		| succ = entry->next
	movl	a1,a0@(4)	| succ->prev = pred
	rts

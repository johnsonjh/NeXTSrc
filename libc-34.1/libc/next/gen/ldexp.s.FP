/*
 * (c) NeXT, INC
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)ldexp.s	1.0 (NeXT) July 7, 1987"
#endif LIBC_SCCS

/*
 * double ldexp (value, exp)
 * double value;
 * int exp;
 *
 * Ldexp returns value*2**exp.
 */

#include "cframe.h"
#include <errno.h>

	.globl	_errno

PROCENTRY(ldexp)
	fmoved	sp@(4),fp0
	fscalel	sp@(12),fp0
	fmovel	fps,d0
	btst #12,d0
	jeq L40
	moveq #ERANGE,d0
	movel d0,_errno
L40:
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	rts

#ifdef LIBC_SCCS
	.asciz	"@(#)modf.s	5.2 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/*
 * double modf (value, iptr)
 * double value, *iptr;
 *
 * Modf returns the fractional part of "value",
 * and stores the integer part indirectly through "iptr".
 */

#include "cframe.h"

/*
 * Round to minus infinity
 */
#define	FPCR_RNDMINUS	0x20

PROCENTRY(modf)
	fmoved	sp@(4),fp0		| value
	movl	sp@(12),a0		| iptr
#ifdef __GNU__
	.long	0xf227b000
	.long	0xf23c9000
	.long	0x20
	.long	0xf2800000
#else !__GNU__
	fmovel	fpcr,sp@-
	fmovel	#FPCR_RNDMINUS,fpcr
	fnop
#endif !__GNU__
	fintx	fp0,fp1			| integer part
	fmoved	fp1,a0@
	fsubx	fp1,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
#ifdef __GNU__
	.long	0xf21f9000
#else !__GNU__
	fmovel	sp@+,fpcr
#endif !__GNU__
	rts

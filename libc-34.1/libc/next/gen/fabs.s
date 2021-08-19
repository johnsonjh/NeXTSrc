#ifdef LIBC_SCCS
	.asciz	"@(#)fabs.s	5.2 (Berkeley) 3/9/86"
#endif LIBC_SCCS

/* fabs - floating absolute value */

#include "cframe.h"

PROCENTRY(fabs)
	fmoved	a_p0,fp0
#ifdef __GNU__
	.long	0xf200003a
#else !__GNU__
	ftestx	fp0
#endif !__GNU___
	fjnlt	1f
	fnegx	fp0,fp0
1:	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	rts

/*
 * Copyright (c) 1987 NeXT, INC.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)_dbltopdfp.s	1.0 (NeXT) July 16, 1987"
#endif LIBC_SCCS

/* _dbltopdfp(k, &pdfp, fp) -- converts fp to packed decimal fp */

#include "cframe.h"

PROCENTRY(_dbltopdfp)
	movl	a_p0,d0		| k factor
	movl	a_p1,a0		| &pdfp
	fmoved	a_p2,fp0	| fp value
	.word	0xf210, 0x7c00	| fmovep	fp0,a0@{d0}
	rts

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)_setjmp.s	5.5 (Berkeley) 3/9/86"
#endif LIBC_SCCS


/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	_setjmp(a)
 * by restoring registers from the stack,
 * The previous signal state is NOT restored.
 */

#include "cframe.h"
#include <setjmp.h>

PROCENTRY(_setjmp)
	movl	a_p0,a0			| address of jmpbuf
	movl	a_ra,a0@(JB_PC*4)	| return address
	lea	a_p0,a1
	movl	a1,a0@(JB_SP*4)		| callers sp
	movl	#JB_MAGICNUM,a0@(JB_MAGIC*4)
	moveml	#0x7cfc,a0@(JB_D2*4)	| save d2-d7, a2-a6
#ifndef FP_BUG
#ifdef __GNU__
	.long	0xf228bc00
	.word	JB_FPCR*4
	.long	0xf228f03f
	.word	JB_FP2*4
#else !__GNU__
	fmovem	fpc/fps/fpi,a0@(JB_FPCR*4)
	fmovem	fp2/fp3/fp4/fp5/fp6/fp7,a0@(JB_FP2*4)
#endif !__GNU_
#endif FP_BUG
	moveq	#0,d0
	rts

PROCENTRY(_longjmp)
	movl	a_p0,a0			| address of jmp_buf
	movl	#JB_MAGICNUM,d0
	cmpl	a0@(JB_MAGIC*4),d0
	bne	botch			| jmp_buf was trashed
	movl	a0@(JB_SP*4),a1
#ifndef FP_BUG
#ifdef __GNU__
	.long	0xf228d03f
	.word	JB_FP2*4
	.long	0xf2289c00
	.word	JB_FPCR*4
#else !__GNU__
	fmovem	a0@(JB_FP2*4),fp2/fp3/fp4/fp5/fp6/fp7
	fmovem	a0@(JB_FPCR*4),fpc/fps/fpi
#endif !__GNU__
#endif FP_BUG
	movl	a_p1,d0			| return value
	movl	a1,sp			| back to old stack
	moveml	a0@(JB_D2*4),#0x7cfc	| restore d2-d7, a2-a7
	movl	a0@(JB_PC*4),a0		| return to _setjmps ra
	jmp	a0@

botch:
	jsr	_longjmperror		| prints error message
	jsr	_abort

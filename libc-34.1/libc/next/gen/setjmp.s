/*
 * Copyright (c) 1987 NeXT, INC.
 */

#ifdef LIBC_SCCS
	.asciz	"@(#)setjmp.s	1.0 (NeXT) July 8, 1987"
#endif LIBC_SCCS

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers from the stack,
 * and a struct sigcontext, see <signal.h>
 */

#include "cframe.h"
#include <setjmp.h>
#include <syscall.h>

PROCENTRY(setjmp)
	movl	a_p0,a0			| jmp_buf pointer
	movl	a_ra,a0@(JB_PC*4)	| stash return pc in jmp_buf
	lea	a_p0,a1			| where caller expects sp on return
	movl	a1,a0@(JB_SP*4)		| callers stack pointer
	clrl	a0@(JB_PS*4)		| reasonable SR on return
	moveml	#0x7cfc,a0@(JB_D2*4)	| save d2-d7,a2-a6
	movl	a2,sp@-
	movl	a0,a2			| use a2, it is saved.
	subql	#8,sp			| make space for a struct sigstack
	movl	sp,sp@-
	clrl	sp@-
	jsr	_sigstack		| get current struct sigstack
	addql	#8,sp			| pop sigstack args
	movl	sp@+,a2@(JB_ONSTACK*4)	| current onsigstack status
	clrl	sp@			| reuse struct sigstack space
	jsr	_sigblock
	addql	#4,sp			| pop sigblock arg
	movl	d0,a2@(JB_SIGMASK*4)	| save current signal mask
	movl	a2,a0
	movl	sp@+,a2
#ifndef FP_BUG
#ifdef __GNU__
	.long	0xf228bc00
	.word	JB_FPCR*4
	.long	0xf228f03f
	.word	JB_FP2*4
#else !__GNU__
	fmovem	fpc/fps/fpi,a0@(JB_FPCR*4)
	fmovem	fp2/fp3/fp4/fp5/fp6/fp7,a0@(JB_FP2*4)
#endif !__GNU__
#endif FP_BUG
	movl	#JB_MAGICNUM,a0@(JB_MAGIC*4)
	moveq	#0,d0
	rts

PROCENTRY(longjmp)
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
	movl	a_p1,a0@(JB_D0*4)	// return value
	moveml	a0@(JB_D2*4),#0x7cfc	// restore d2-d7, a2-a7
	pea	a0@			// sigcontext (jmp_buf) address
	pea	SYS_sigreturn
	movl	#SYS_sigreturn,d0
	trap	#4			// let the kernel do the rest!

botch:
	jsr	_longjmperror		| prints error message
	jsr	_abort

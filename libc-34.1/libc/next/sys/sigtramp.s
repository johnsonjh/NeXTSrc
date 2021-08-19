#include "SYS.h"

/*
 *	Install a signal trampoline before calling user signal handler.
 *	Saves and restores state of thread interrupted by signal.
 *	Called from sigvec syscall code in libc.
 */

	.globl	__sigtramp
__sigtramp:
	moveml	#0xc0c0,sp@-		/* save d0,d1,a0,a1 */
#define	RS	16			/* byte size of saved registers */
	movl	sp@(RS),d0		/* signal # */
#define	FS	36			/* byte size of save FP registers */
#ifndef FP_BUG
#if	__GNU__
	.long	0xf227e003
	.long	0xf227bc00
#else
	fmovem	fp0/fp1,sp@-		/* save fp temps */
	fmovem	fpc/fps/fpi,sp@-
#endif
#else FP_BUG
	subl	#FS,sp
#endif FP_BUG
	lea	_sigcatch,a0		/* get user's signal handler */
	movl	a0@(0,d0:l:4),a0

	/* copy argument stack for signal handler */

	movl	sp@(8+0+RS+FS),sp@-	/* scp */
	movl	sp@(4+4+RS+FS),sp@-	/* code */
	movl	sp@(0+8+RS+FS),sp@-	/* signal # */
	jsr	a0@			/* call user's signal handler */
	addl	#12,sp
#ifndef FP_BUG
#if	__GNU__
	.long	0xf21f9c00
	.long	0xf21fd0c0
#else
	fmovem	sp@+,fpc/fps/fpi	/* restore fp temps */
	fmovem	sp@+,fp0/fp1
#endif
#else FP_BUG
	addl	#FS,sp
#endif FP_BUG
	moveml	sp@+,#0x0303		/* restore d0,d1,a0,a1 */

	/* the sigreturn syscall only needs scp arg, so pop signal # & code */
	/* but leave a fake address to simulate call frame for trap macro */

	/* Note: can now only use d0, which is used for syscode */

	addql	#4,sp
	movl	#SYS_sigreturn,d0
	trap	#4

	/* we get here if sigretsurn found a bad signal stack */

	jmp	_abort

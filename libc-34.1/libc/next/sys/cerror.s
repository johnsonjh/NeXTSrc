/*
 *	File:	cerror.s
 *
 *	FIXME:  Fix for threads.
 */
	.globl	_errno
	.globl	cerror
cerror:
	movl	d0,_errno
	movl	d0,sp@-
	jsr	_cthread_set_errno_self
	addqw	#4,sp
	moveq	#-1,d0
	rts

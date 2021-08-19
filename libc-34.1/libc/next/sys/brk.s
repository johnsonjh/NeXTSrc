#include "SYS.h"

#define	SYS_brk		17

	.globl	curbrk
	.globl	minbrk

PROCENTRY(brk)
	movl	minbrk,d0
	cmpl	a_p0,d0
	bls	1f
	movl	minbrk,a_p0
SYSCALL_NONAME(brk, 1)
	movl	a_p0,curbrk
	moveq	#0,d0
	rts

	.globl	_end

	.data
minbrk: .long	_end
curbrk:	.long	_end
	.text

PROCENTRY(sbrk)
	movl	a_p0,d0
	addql	#3,d0
	moveq	#-4,d1
	andl	d1,d0
	addl	curbrk,d0
	movl	d0,a_p0
SYSCALL_NONAME(brk, 1)
	movl	curbrk,d0
	movl	a_p0,curbrk
	rts

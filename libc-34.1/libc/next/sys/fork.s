#include "SYS.h"

PROCENTRY(fork)
	jsr __cthread_fork_prepare
SYSCALL_NONAME(fork, 0)
	tstl	d1
	beq	1f	| parent, since d1 == 0 in parent, 1 in child
	jsr	_fork_mach_init
	jsr	__cthread_fork_child
	moveq	#0,d0
	rts		| pid = fork()

1:	movl	d0,sp@-	| save return value
	jsr	__cthread_fork_parent
	movl	sp@+,d0
	rts		| pid = fork()
		

	.globl	_vfork
_vfork:	bras	_fork

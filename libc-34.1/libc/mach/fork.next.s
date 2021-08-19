/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

_sccsid:.asciz	"@(#)fork.c	5.3 (Berkeley) 3/9/86"

#include "SYS.next.h"

	.text
	.even
SYSCALL(fork, 0)
	tstl	d1
	beq	1f	| parent, since d1 == 0 in parent, 1 in child
	link	a6,#0
	jsr	_mach_init
	unlk	a6
	moveq	#0,d0
1:	rts		| pid = fork()

	.globl	_vfork
_vfork:	jmp	_fork

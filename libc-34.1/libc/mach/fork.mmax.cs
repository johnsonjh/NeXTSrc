/*
 * HISTORY
 * 10-Nov-86  David L. Black (dlb) at Carnegie-Mellon University
 *	Created based on UMAX fork.s and vax fork.cs
 *
 * 19-Aug-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added call to "mach_init".  Note that we're punting the
 *	"profiling" version of the system call.
 *
 *	Companion "SYS.h" file is as yet unmodified from the
 *	vanilla Berkeley version, and should be updated as
 *	necessary.
 */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
_sccsid:.asciz	"@(#)fork.c	5.2.M (Mach) 19-aug-86"
#endif not lint

#include "SYS.mmax.h"

	.globl	_mach_init

SYSCALL(fork)
	cmpqd	$0,r1	# parent since r1 == 0 in parent, 1 in child
	beq	.Lparent
	jsr	@_mach_init
	movqd	$0,r0
.Lparent:
	ret	$0	# pid = fork()

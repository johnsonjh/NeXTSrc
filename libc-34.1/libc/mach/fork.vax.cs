/*
 * HISTORY
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

#include "SYS.h"

	.globl	_mach_init

SYSCALL(fork)
	jlbc	r1,1f	# parent, since r1 == 0 in parent, 1 in child
	calls	$0,_mach_init
	clrl	r0
1:
	ret		# pid = fork()

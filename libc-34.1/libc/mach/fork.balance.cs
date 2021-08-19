/*
 * HISTORY
 * 18-Feb-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Redone for sequent
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

#include <syscall.h>

	.globl	_mach_init

#ifdef PROF
#define	ENTRY(x)	.globl _/**/x; .align 2; _/**/x: \
			.data; 1:; .long 0; .text; addr 1b,r0; bsr mcount
#else
#define	ENTRY(x)	.globl _/**/x; .align 2; _/**/x:
#endif PROF


	.globl	cerror

#define svc0(name) \
	ENTRY(name)\
	enter	[], 0;\
	movzbd	SYS_/**/name, r0;\
	svc;\
	bfc	1f;\
	bsr	cerror;\
1:

svc0(fork)
	tbitb	0,r1
	bfc	1f		# parent, since r1 == 0 in parent, 1 in child
	bsr	_mach_init
	movqd	0,r0

1:	exit	[]
	ret 	0
   
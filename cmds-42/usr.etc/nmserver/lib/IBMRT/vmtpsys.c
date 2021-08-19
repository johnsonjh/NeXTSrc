/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 * Library for VMTP system calls.
 */

/*
 * HISTORY:
 * 28-May-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 *
 */

#include "SYS.h"
SYSCALL(invoke)
	.align 2
	.ltorg

SYSCALL(recvreq)
	.align 2
	.ltorg

SYSCALL(sendreply)
	.align 2
	.ltorg

SYSCALL(forward)
	.align 2
	.ltorg

SYSCALL(probeentity)
	.align 2
	.ltorg

SYSCALL(getreply)
	.align 2
	.ltorg



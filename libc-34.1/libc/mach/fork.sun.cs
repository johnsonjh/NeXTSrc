/* 
 **********************************************************************
 * Mach Operating System
 * Copyright (c) 1986 Carnegie-Mellon University
 *  
 * This software was developed by the Mach operating system
 * project at Carnegie-Mellon University's Department of Computer
 * Science. Software contributors as of May 1986 include Mike Accetta, 
 * Robert Baron, William Bolosky, Jonathan Chew, David Golub, 
 * Glenn Marcy, Richard Rashid, Avie Tevanian and Michael Young. 
 * 
 * Some software in these files are derived from sources other
 * than CMU.  Previous copyright and other source notices are
 * preserved below and permission to use such software is
 * dependent on licenses from those institutions.
 * 
 * Permission to use the CMU portion of this software for 
 * any non-commercial research and development purpose is
 * granted with the understanding that appropriate credit
 * will be given to CMU, the Mach project and its authors.
 * The Mach project would appreciate being notified of any
 * modifications and of redistribution of this software so that
 * bug fixes and enhancements may be distributed to users.
 *
 * All other rights are reserved to Carnegie-Mellon University.
 **********************************************************************
 * HISTORY
 * 25-Nov-86  Jonathan J. Chew (jjc) at Carnegie-Mellon University
 *	Added call to mach_init().
 *
 **********************************************************************
 */ 
/* @(#)fork.c 1.1 86/02/03 SMI; from UCB 4.1 82/12/04 */

#include "SYS.sun.h"

	.globl	_mach_init

SYSCALL(fork)
#if sun
	tstl	d1
	beqs	2$	/* parent, since ...  */
	link	a6,#0
	jsr	_mach_init
	unlk	a6
	clrl	d0
2$:
#endif
	RET		/* pid = fork() */

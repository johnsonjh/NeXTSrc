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
 * 10-Nov-86  David L. Black (dlb) at Carnegie-Mellon University
 *	Created by cutting SYS.h down to size from UMAX version.
 **********************************************************************
 */ 

/* SYS.h 4.1 83/05/10 */

#include <syscall.h>
/*
 * Macros to implement system calls
 */

#ifdef PROF
#define	ENTRY(x) \
				.data ;\
				.bss .P/**/x,4,4 ;\
				.text ;\
				.globl _/**/x ;\
			_/**/x:	addr	.P/**/x,r0 ;\
				jsr mcount ;
#else
#define	ENTRY(x) \
				.globl _/**/x ;\
			_/**/x: ;
#endif PROF

#define	SYSCALL(x) \
				.globl cerror ;\
			.L/**/x:	jump cerror ;\
			ENTRY(x) ;\
				addr	@SYS_/**/x,r0 ;\
				addr	4(sp),r1 ;\
				svc ;\
				bcs	.L/**/x ;

#define	PSEUDO(x,y) \
				.globl cerror ;\
			.L/**/x:	jump cerror ;\
			ENTRY(x) ;\
				addr @SYS_/**/y,r0 ;\
				addr 4(sp),r1 ;\
				svc ;\
				bcs .L/**/x ;

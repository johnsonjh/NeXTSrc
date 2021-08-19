/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * $Log:	options.h,v $
 * Revision 1.3  89/06/21  13:03:21  mbj
 * 	Removed the old (! IPC_WAIT) form of condition waiting/signalling.
 * 
 * Revision 1.2  89/05/05  18:59:43  mrt
 * 	Cleanup for Mach 2.5
 * 
 */
/*
 * options.h
 *
 * This file normalizes the option values
 * to 1 for the one in use, 0 for the others.
 * If none of the options is defined,
 * it will cause a compile-time error.
 */


#if defined(MTHREAD)
#	if defined(COROUTINE) || defined(MTASK)
		compile_time_check() {
			undefined(UNIQUE_IMPLEMENTATION_OPTION);
		}
#	else
#		undef	MTHREAD
#		undef	COROUTINE
#		undef	MTASK
#		define	MTHREAD		1
#		define	COROUTINE	0
#		define	MTASK		0
#	endif
#else
#if defined(COROUTINE)
#	if defined(MTASK)
		compile_time_check() {
			undefined(UNIQUE_IMPLEMENTATION_OPTION);
		}
#	else
#		undef	MTHREAD
#		undef	COROUTINE
#		undef	MTASK
#		define	MTHREAD		0
#		define	COROUTINE	1
#		define	MTASK		0
#	endif
#else
#if defined(MTASK)
#	undef	MTHREAD
#	undef	COROUTINE
#	undef	MTASK
#	define	MTHREAD		0
#	define	COROUTINE	0
#	define	MTASK		1
#else
	compile_time_check() {
		undefined(IMPLEMENTATION_OPTION);
	}
#endif
#endif
#endif

/*
 * The following should be defined
 * as 1 to enable, 0 to disable.
 */
#define	SCHED_HINT	0

/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * $Log:	mach.h,v $
 * 31-May-90  Gregg Kellogg (gk) at NeXT
 *	Added <sys/thread_switch.h> and <mach_host.h>.
 *
 * Revision 2.2  89/10/28  11:30:47  mrt
 * 	Changed location of mach_interface from mach/ to
 * 	top level include.
 * 	[89/10/27            mrt]
 * 
 * Revision 2.1  89/06/13  16:47:44  mrt
 * Created.
 * 
 */
/* 
 *  Includes all the types that a normal user
 *  of Mach programs should need
 */

#ifndef	_MACH_H_
#define	_MACH_H_

#include <kern/mach_types.h>
#include <sys/thread_switch.h>
#include <mach_interface.h>
#include <mach_host.h>
#include <mach_init.h>
#include <mach_extra.h>

#endif	_MACH_H_




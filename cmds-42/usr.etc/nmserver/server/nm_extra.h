/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nm_extra.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/nm_extra.h,v $
 *
 * $Header: nm_extra.h,v 1.1 88/09/30 15:44:36 osdev Exp $
 *
 */

/*
 * External definitions of functions provided by nm_extra.c.
 */

/*
 * HISTORY:
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Make netmsg_receive a macro.
 *
 * 15-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed external definition of panic.
 *
 * 15-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_NM_EXTRA_
#define	_NM_EXTRA_

#include "config.h"
#include "debug.h"

#if	LOCK_THREADS
#define netmsg_receive(msg_ptr) netmsg_receive_locked(msg_ptr)
#else	LOCK_THREADS
#define netmsg_receive(msg_ptr) msg_receive(msg_ptr, MSG_OPTION_NONE, 0)
#endif	LOCK_THREADS

extern void ipaddr_to_string();
/*
char		*output_string;
netaddr_t	input_address;
*/

#endif	_NM_EXTRA_

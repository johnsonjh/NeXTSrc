/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * uid.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/uid.h,v $
 *
 * $Header: uid.h,v 1.1 88/09/30 15:45:58 osdev Exp $
 *
 */

/*
 * Public definitions for generator of locally unique identifiers.
 */

#ifndef	_UID_
#define	_UID_

#include <sys/boolean.h>

extern boolean_t uid_init();
/*
*/

extern long uid_get_new_uid();
/*
*/

#endif	_UID_

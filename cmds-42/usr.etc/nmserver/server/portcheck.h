/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * portcheck.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/portcheck.h,v $
 *
 * $Header: portcheck.h,v 1.1 88/09/30 15:45:00 osdev Exp $
 */

/*
 * Definitions exported by the port checkups module.
 */

/*
 * HISTORY:
 *  6-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed pc_checkup_interval - now obtained from param record.
 *
 * 19-Nov-86  Sanjay Agrawal (agrawal) and Healfdene Goguen (hhg) at Carnegie Mellon University
 *	Started.
 */

#ifndef _PORTCHECK_
#define _PORTCHECK_

#include <sys/boolean.h>

extern boolean_t pc_init();

#endif _PORTCHECKUPS_

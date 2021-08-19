/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * portsearch.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/portsearch.h,v $
 *
 * $Header: portsearch.h,v 1.1 88/09/30 15:45:08 osdev Exp $
 *
 */

/*
 * External definitions for the port search module
 */

/*
 * HISTORY:
 * 18-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_PORTSEARCH_
#define	_PORTSEARCH_

#include <sys/boolean.h>


extern void ps_do_port_search();
/*
port_rec_ptr_t		port_rec_ptr;
boolean_t		new_information;
network_port_ptr_t	new_nport_ptr;
int			(*retry)();
*/

extern boolean_t ps_init();
/*
*/

#endif	_PORTSEARCH_

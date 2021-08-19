/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nn.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/nn.h,v $
 *
 * $Header: nn.h,v 1.1 88/09/30 15:44:42 osdev Exp $
 *
 */

/*
 * External definitions for the network name service module.
 */

/*
 * HISTORY:
 * 23-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_NN_
#define	_NN_

#include <sys/boolean.h>

extern boolean_t netname_init();
/*
*/

extern void nn_remove_entries();
/*
port_t	port_id;
*/

#endif	_NN_

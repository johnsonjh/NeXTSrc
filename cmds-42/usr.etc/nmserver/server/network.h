/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * network.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/network.h,v $
 *
 * $Header: network.h,v 1.1 88/09/30 15:44:30 osdev Exp $
 *
 */

/*
 * Random network-level definitions and externs.
 */

/*
 * HISTORY:
 * 22-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added udp_checksum and last_ip_id definition.
 *
 *  3-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_NETWORK_
#define	_NETWORK_

#include <sys/boolean.h>
#include "nm_defs.h"

#define HOST_NAME_SIZE	40
extern char			my_host_name[HOST_NAME_SIZE];
extern netaddr_t		my_host_id;
extern netaddr_t		broadcast_address;
extern short			last_ip_id;

extern boolean_t network_init();

extern int udp_checksum();

#endif	_NETWORK_

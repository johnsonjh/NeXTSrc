/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nm_defs.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/nm_defs.h,v $
 *
 * $Header: nm_defs.h,v 1.1 88/09/30 15:44:33 osdev Exp $
 *
 */

/*
 * Random definitions for the network service that everyone needs!
 */

/*
 * HISTORY:
 * 27-Mar-90  Gregg Kellogg (gk) at NeXT
 *	include <sys/netport.h> rather than <sys/ipc_netport.h>
 *
 * 24-Aug-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Replace sys/mach_ipc_netport.h with kern/ipc_netport.h. Sigh.
 *
 * 24-May-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Replace mach_ipc_vmtp.h with mach_ipc_netport.h.
 *
 *  4-Sep-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Fixed for new kernel include files which declare a lot
 *	of network server stuff internally, because of the NETPORT
 *	option.
 *
 *  5-Nov-86  Robert Sansom (rds) at Carnegie-Mellon University
 *	Started.
 *
 */

#ifndef	_NM_DEFS_
#define	_NM_DEFS_

/*
 * netaddr_t is declared with the kernel files,
 * in <sys/netport.h>.
 */
#include	<sys/netport.h>

#ifdef	notdef
typedef unsigned long	netaddr_t;
#endif	notdef

typedef union {
    struct {
	unsigned char ia_net_owner;
	unsigned char ia_net_node_type;
	unsigned char ia_host_high;
	unsigned char ia_host_low;
    } ia_bytes;
    netaddr_t ia_netaddr;
} ip_addr_t;

#endif	_NM_DEFS_


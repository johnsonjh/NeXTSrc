/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * ipc.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/ipc.h,v $
 *
 * $Header: ipc.h,v 1.1 88/09/30 15:43:36 osdev Exp $
 *
 */

/*
 * External definitions for the IPC module.
 */

/*
 * HISTORY:
 *  6-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added macros for NETPORT option and declarations for RPCMOD.
 *
 *  4-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added external definitions of ipc_retry, ipc_port_dead and 
 *	ipc_msg_accepted.
 *
 *  2-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_IPC_
#define	_IPC_

#include <sys/boolean.h>

extern boolean_t ipc_init();
/*
*/


extern void ipc_msg_accepted();
/*
port_rec_ptr_t	port_rec_ptr;
*/


extern void ipc_port_dead();
/*
port_rec_ptr_t	port_rec_ptr;
*/


#if	RPCMOD
extern void ipc_port_moved();
/*
port_rec_ptr_t	port_rec_ptr;
*/
#endif	RPCMOD


extern ipc_retry();
/*
port_rec_ptr_t	port_rec_ptr;
*/


extern void ipc_freeze();
/*
port_rec_ptr_t	port_rec_ptr;
*/


#if	NETPORT
/*
 * Macros to manipulate the kernel NETPORT (MACH_NP) tables.
 */

#define	ipc_netport_enter(netport,localport,local) {			\
	if (param.conf_netport) {					\
		netport_enter(task_self(),(netport),(localport),(local)); \
	}								\
}

#define	ipc_netport_remove(netport) {					\
	if (param.conf_netport) {					\
		netport_remove(task_self(),(netport));			\
	}								\
}
#endif	NETPORT

#endif	_IPC_

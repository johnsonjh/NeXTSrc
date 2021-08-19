/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * ipc_hdr.h 
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/ipc_hdr.h,v $ 
 *
 * $Header: ipc_hdr.h,v 1.1 88/09/30 15:43:40 osdev Exp $ 
 *
 */

/*
 * Definitions for the network headers used by the IPC module. 
 */

/*
 * HISTORY: 
 * 28-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added ipc_seq_no.
 *
 * 12-Dec-86  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added IPC_INFO_RPC.
 *
 * 15-Nov-86  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 *
 *
 */

#ifndef	_IPC_HDR_
#define	_IPC_HDR_

#include	"disp_hdr.h"
#include	"port_defs.h"

/*
 * Header for network IPC messages 
 */
typedef struct {
	disp_hdr_t      disp_hdr;	/* dispatcher header */
	network_port_t  local_port;
	network_port_t  remote_port;
	unsigned long   info;		/* info bits */
	unsigned long   npd_size;	/* size of Network Port Dictionary */
	unsigned long   inline_size;	/* size of inline part of message */
	unsigned long   ool_size;	/* size of ool part of message */
	unsigned long   ool_num;	/* number of ool sections (for assembly) */
	unsigned long	ipc_seq_no;	/* the IPC sequence number of this message */
}               ipc_netmsg_hdr_t;

/*
 * Bits for info field 
 */
#define IPC_INFO_SIMPLE		0x1
#define	IPC_INFO_RPC		0x2

#endif	_IPC_HDR_

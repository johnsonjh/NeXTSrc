/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * tcp_defs.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/tcp_defs.h,v $
 *
 * $Header: tcp_defs.h,v 1.1 88/09/30 15:45:42 osdev Exp $
 *
 */

/*
 * Definitions for the TCP module.
 */

/*
 * HISTORY:
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  3-Dec-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_TCP_DEFS_
#define	_TCP_DEFS_

#include	"mem.h"
#include	"sbuf.h"
#include	"sys_queue.h"

/*
 * Transaction records.
 */
typedef struct tcp_trans {
	int			state;	/* see defines below */
	unsigned long		trid;
	int			client_id;
	sbuf_ptr_t		data;
	int			crypt_level;
	int			(*reply_proc)();
	sys_queue_chain_t	transq;	/* list of pending/waiting transactions */
} tcp_trans_t, *tcp_trans_ptr_t;

#define	TCP_TR_INVALID	0
#define	TCP_TR_PENDING	1	/* awaiting a reply */
#define	TCP_TR_WAITING	2	/* awaiting transmission */


/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_TCPTRANS;


#endif	_TCP_DEFS_

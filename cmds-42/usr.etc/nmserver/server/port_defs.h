/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * port_defs.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/port_defs.h,v $
 *
 * $Header: port_defs.h,v 1.1 88/09/30 15:44:56 osdev Exp $
 *
 */

/*
 * Definitions of network port structures and associated constants.
 */

/*
 * HISTORY:
 * 24-May-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Replace mach_ipc_vmtp.h with mach_ipc_netport.h.
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 14-Nov-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for new ipc_rec states and outgoing queue.
 *
 *  7-Sep-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Many modifications for RPCMOD. Added port record reference counts.
 *
 * 24-Aug-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Defined NPORT_HAVE_REC_RIGHTS. Removed PORT_INFO_PROBED, not
 *	needed with the new RPC module.
 *
 * 18-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added port_stat field to port record.
 *
 * 29-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added NPORT_EQUAL macro.
 *
 * 26-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added PORT_INFO_ACTIVE - set if there is a message in transit to a port.
 *
 *  5-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Made the hash indirection record inline in a port record.
 *	Made the lock_t inline in a port record.
 *
 *  4-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added PORT_INFO_DEAD.  Removed pq_hash external definitions.
 *	Added PORT_REC_OWNER, PORT_REC_RECEIVER, NPORT_HAVE_SEND_RIGHTS,
 *	NPORT_HAVE_RO_RIGHTS and NPORT_HAVE_ALL_RIGHTS.
 *
 *  2-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added portrec_random and portrec_clock to a port_rec_t.
 *
 * 17-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Made portrec_secure_info be a real part of a port record.
 *	Added PORT_REC_NULL.
 *
 * 12-Dec-86  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Defined PORT_INFO_PROBED.
 *
 * 10-Dec-86  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Renamed {current,backlog}_outmsg to {current,backlog}_ipcrec to
 *	reflect new handling of IPC records.
 *
 *  4-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Moved external function definitions to portrec.h and portops.h.
 *	Added portrec_token_list to port_rec_t.
 *
 *  5-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_PORT_DEFS_
#define	_PORT_DEFS_

#include <mach_types.h>

#include "key_defs.h"
#include "ls_defs.h"
#include "nm_defs.h"
#include "rwlock.h"
#include "mem.h"

#include "sys_queue.h"

/*
 * np_uid_t and network_port_t are declared with the kernel files,
 * in sys/mach_ipc_netport.h.
 */
#ifdef	notdef
/*
 * Network Port structure.
 */
typedef struct {
    long	np_uid_high;
    long	np_uid_low;
} np_uid_t;

typedef struct {
    netaddr_t	np_receiver;
    netaddr_t	np_owner;
    np_uid_t	np_puid;
    np_uid_t	np_sid;
} network_port_t;
#endif	notdef

typedef network_port_t	*network_port_ptr_t;

/*
 * Macros to determine the rights that we have to a port.
 */
#define NPORT_HAVE_SEND_RIGHTS(nport) \
    (((nport).np_receiver != my_host_id) && ((nport).np_owner != my_host_id))
#define NPORT_HAVE_REC_RIGHTS(nport) \
    ((nport).np_receiver == my_host_id)
#define NPORT_HAVE_RO_RIGHTS(nport) \
    (((nport).np_receiver == my_host_id) || ((nport).np_owner == my_host_id))
#define NPORT_HAVE_ALL_RIGHTS(nport) \
    (((nport).np_receiver == my_host_id) && ((nport).np_owner == my_host_id))

/*
 * Macro to test for network port equality.
 */
#define NPORT_EQUAL(nport1,nport2) (					\
	((nport1).np_puid.np_uid_high == (nport2).np_puid.np_uid_high)	\
	&& ((nport1).np_puid.np_uid_low == (nport2).np_puid.np_uid_low)	\
	&& ((nport1).np_sid.np_uid_low == (nport2).np_sid.np_uid_low)	\
	&& ((nport1).np_sid.np_uid_low == (nport2).np_sid.np_uid_low))


/*
 * Stucture used by pr_list.
 */
typedef struct port_item {
	struct port_item	*next;
	port_t			pi_port;
} port_item_t, *port_item_ptr_t;


/*
 * Information maintained about ports.
 */
typedef struct pq_hash {
    struct pq_hash	*next;
    struct port_rec_	*pqh_portrec;
} pq_hash_t, *pq_hash_ptr_t;

typedef struct port_rec_{
    pq_hash_t		portrec_localitem;
    pq_hash_t		portrec_networkitem;
#if	RPCMOD
    int			portrec_refcount;
#endif	RPCMOD
    int			portrec_info;
    int			portrec_port_rights;
    port_t		portrec_local_port;
    network_port_t	portrec_network_port;
    secure_info_t	portrec_secure_info;
    long		portrec_random;
    long		portrec_clock;
    pointer_t		portrec_po_host_list;	/* List of network servers for port ops module. */
    short		portrec_security_level;
    short		portrec_aliveness;
#if	RPCMOD
    long		portrec_retry_level;
    long		portrec_waiting_count;
    long		portrec_transit_count;
    sys_queue_head_t	portrec_out_ipcrec;
    pointer_t		portrec_lazy_ipcrec;
    pointer_t		portrec_reply_ipcrec;
#else	RPCMOD
    pointer_t		portrec_current_ipcrec;
    pointer_t		portrec_backlog_ipcrec;
#endif	RPCMOD
    pointer_t		portrec_block_queue;	/* List of senders waiting if the port is blocked. */
    struct lock		portrec_lock;
    port_stat_ptr_t	portrec_stat;
} port_rec_t, *port_rec_ptr_t;

#define PORT_REC_NULL	(port_rec_ptr_t)0
#define PORT_REC_OWNER(port_rec_ptr) ((port_rec_ptr)->portrec_network_port.np_owner)
#define PORT_REC_RECEIVER(port_rec_ptr) ((port_rec_ptr)->portrec_network_port.np_receiver)


/*
 *  Values of portrec_info field.
 */
#define	PORT_INFO_SUSPENDED	0x1
#define PORT_INFO_DISABLED	0x2
#define PORT_INFO_BLOCKED	0x4
#define	PORT_INFO_PROBED	0x8
#define PORT_INFO_DEAD		0x10
#define PORT_INFO_ACTIVE	0x20
#define	PORT_INFO_NOLOOKUP	0x40

#define	PORT_BUSY(port_rec_ptr) (				\
	((port_rec_ptr->portrec_info) & 			\
		(PORT_INFO_SUSPENDED | PORT_INFO_BLOCKED)) ||	\
	(port_rec_ptr->portrec_transit_count > 0))		\


/*
 * Values of portrec_aliveness field.
 */
#define PORT_ACTIVE		2
#define PORT_INACTIVE		0

/*
 * Values for port access rights.
 * These are the same as the values in /usr/mach/include/sys/message.h
 */
#define PORT_OWNERSHIP_RIGHTS	3
#define PORT_RECEIVE_RIGHTS	4
#define PORT_ALL_RIGHTS		5
#define PORT_SEND_RIGHTS	6


#if	RPCMOD
/*
 * Port reference counts.
 *
 * Any entity that intends to use a port record after having released
 * the lock on it must make a reference to it before releasing that lock.
 * There is normally a single reference for all the lookup queues, one
 * reference for each ipc_rec pending on the destination port, and one
 * reference for the reply_ipcrec, if any (for simplicity, that last
 * reference is kept until the ipc_rec is deallocated, even if the ipc_rec
 * is unlinked from the port record long before that).
 *
 * The port lookup procedures do not create any extra references, but use
 * the one made when the port is created. This is acceptable since they
 * keep the port record locked on exit. The port record is protected from
 * early removal from the queues by holding the queue lock(s) while acquiring
 * the port record lock, and by checking for the PORT_INFO_NOLOOKUP flag when
 * this is not possible. To avoid deadlocks, all procedures must acquire queue
 * locks before port locks (and only one port lock should ever be held at
 * a time).
 */
#define	pr_reference(port_rec_ptr) {					\
	/* port_rec_ptr LOCK RW/RW */					\
	port_rec_ptr->portrec_refcount++;				\
	/* port_rec_ptr LOCK RW/RW */					\
}

#define	pr_release(port_rec_ptr) {					\
	/* port_rec_ptr LOCK RW/RW */					\
	if (--port_rec_ptr->portrec_refcount) {				\
		lk_unlock(&port_rec_ptr->portrec_lock);			\
		/* port_rec_ptr LOCK -/- */				\
	} else {							\
		lk_clear(&port_rec_ptr->portrec_lock);			\
		MEM_DEALLOCOBJ(port_rec_ptr, MEM_PORTREC);		\
	}								\
	/* port_rec_ptr LOCK -/- */					\
}
#endif	RPCMOD

/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_PORTREC;
extern mem_objrec_t		MEM_PORTITEM;


#endif	_PORT_DEFS_

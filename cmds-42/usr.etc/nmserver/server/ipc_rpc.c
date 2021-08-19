/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * ipc_rpc.c
 *
 * Main file for the IPC module, to handle incoming and outgoing
 * IPC/RPC requests and responses.
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/ipc_rpc.c,v $
 *
 */
#ifndef	lint
char ipc_rpc_rcsid[] = "$Header: ipc_rpc.c,v 1.1 88/09/30 15:39:41 osdev Exp $";
#endif not lint
/*
 *
 */

/*
 * HISTORY:
 * 09-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Conditionalize encrypt code.
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 19-Feb-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Made ipc_rpc_init PUBLIC instead of EXPORT.
 *
 * 19-Jan-88  Robert Sansom (rds) at Carnegie Mellon University
 *	Use libmach port_enable and port_disable.
 *	Swap the incoming src_format in ipc_in_reply before checking it.
 *
 *  6-Dec-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added handling of TR_OVERLOAD: pretend the port is blocked,
 *	and let the checkup mechanism retry later.
 *
 * 17-Nov-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Fixed ipc_retry_internal to use sys_queue_end correctly.
 *
 * 14-Nov-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for new ipc_rec states and outgoing queue.
 *
 *  7-Nov-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for dispatcher with version number.
 *
 * 18-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for byte-swapping of ipc sequence number.
 *	Added checking of sequence numbers for secure messages.
 *	Added Camelot support.
 *
 *  6-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Started.
 *
 */

#include	"netmsg.h"
#include	"nm_defs.h"

#include	<sys/message.h>
#include	<msg_type.h>
#include	<sys/time.h>

#include	"port_defs.h"
#include	"ipc_rec.h"
#include	"transport.h"
#include	"crypt.h"
#include	"sys_queue.h"
#include	"portrec.h"
#include	"portops.h"
#include	"network.h"
#include	"dispatcher.h"
#include	"ipc.h"
#include	"ipc_internal.h"
#include	"nm_extra.h"
#include	"portsearch.h"
#include	"keyman.h"
#include	"ipc_swap.h"

#if	RPCMOD
#else	RPCMOD
 ! You lose !
#endif	RPCMOD

#if	CAMELOT
#include	"../camelot/cam.h"
extern struct mutex	camelot_lock;
#endif	CAMELOT


/*
 * Locking strategy for IPC records.
 *
 * In order to minimize locking overhead, there is no lock specific to the
 * IPC records. Instead, locks on port records are used to control access
 * to the IPC records attached to them, following a protocol described below.
 * The operation is different on the client and server sides of a transaction.
 * In all cases, at most one port record lock is held at any time.
 *
 * Client side:
 *
 * Each record can be referenced from the server port record, from the
 * reply port record and a from transport module (when a transaction is
 * in progress). Some parts of the record are used for a single function and
 * are always protected by the same lock: the queue of pending messages by
 * the lock on the server port record, and the re-send queue by the re-send lock.
 * The access protocol for the rest of the IPC record depends on the mode of
 * reference, and on the value of the ipc_ptr->status field.
 *
 * - For any status other than IPC_REC_ACTIVE, the only link to the
 * IPC record is through the server port record, and the lock on this
 * port record controls read/write access to the whole IPC record. 
 *
 * - While the IPC record is in the IPC_REC_ACTIVE state, it may be linked
 * to a reply port record and passed to a transport module.
 * In that case, the link to the reply port record (if any) must be made
 * before the call to the transport module. After that link has been made,
 * the thread that is to call the transport module has ownership of the IPC
 * record until a reply is delivered.
 * Access via the server port record is forbidden. Access via the reply port
 * record is controlled by the lock on that port record; it is limited to
 * removing that reply port link and to reading ipc_ptr->type, ipc_ptr->out.dest 
 * and ipc_ptr->out.netmsg_hdr. The transport thread is free to read all fields
 * of the IPC record, and to write those fields not covered by the reply port
 * lock. In addition, it may write ipc_ptr->type, provided this is done in one
 * atomic operation to avoid confusing other threads simultaneously reading
 * this field
 *
 * The IPC record may not leave the IPC_REC_ACTIVE state until the transport
 * module relinquishes ownership by delivering a reply. At that time, any link
 * to the reply port record must be removed to re-establish a single access path,
 * and the lock on the server port record must be re-acquired. The link to the
 * reply port record is naturally controlled by the port record lock, so there
 * are no concurrency problems between the transport thread and any other
 * user of the IPC record via the reply port record.
 *
 * The field ipc_ptr->server_port_ptr, needed to find the server port record
 * when exiting the IPC_REC_ACTIVE state, is written before entering that
 * state, and never changes. It may be read at any time by anyone.
 * The field ipc_ptr->reply_port_ptr, needed to remove the reply port link
 * when exiting the IPC_REC_ACTIVE state, is also written before entering that
 * state. It is not modified in the IPC_REC_ACTIVE state, even if a thread
 * accessing the IPC record via the reply port link, removes that link; a non-zero
 * value in that field thus constitutes a necessary but not a sufficient condition
 * for the existence of a reply port link.
 * The field ipc_ptr->status itself is protected by the lock on the server port
 * record. According to the rules above, it is impossible to find this field with
 * a value other than IPC_REC_ACTIVE when referencing it without going through
 * the server port record.
 *
 * Server side:
 *
 * The IPC record is only linked to the reply port, and only if
 * processing a RPC. In that case, it is protected for read/write
 * access by the lock on the reply port record. The section containing 
 * the incoming data is not shared, and can be accessed at will.
 *
 */

/*
 * Operation of the queue for pending messages.
 *
 * All messages outstanding for a given destination port on the client side
 * are queued on the out queue in that port record. The order on this queue
 * corresponds to the order of arrival in ipc_out_main(). IPC records on the
 * queue can be in one of three states:
 * 	IPC_REC_ACTIVE: in transit to the destination, waiting for a reply.
 *	IPC_REC_READY: ready to be transmitted, waiting for the re-send thread
 *			to get to them.
 *	IPC_REC_WAITING: not to be transmitted at once, waiting for some
 *			external condition to make them ready.
 *
 * An active record can be waiting either for a RPC reply (RPC case), to be
 * delivered after an indeterminate time, or for an immediate acknowledgment
 * (IPC case), sent by the server as soon as it receives the request.
 * The fields transit_count and waiting_count in the port record reflect
 * respectively the number of outstanding active or ready records waiting
 * for an immediate acknowledgment, and the number of records in the
 * IPC_REC_WAITING state. 
 *
 * The transmission policy is not to allow the transit_count to become greater
 * than one. This means that as soon as one simple IPC request is transmitted,
 * the system must wait for the reception of the acknowledgment before initiating
 * a new transmission. New IPC records queued in that interval are placed in
 * the IPC_REC_WAITING state. As a result, the queue can contain many active
 * records for RPC's, but at most one active record for IPC, and this record must
 * be the last active record on the queue.
 *
 * Whenever a reply or acknowledgment is received, its success code is examined.
 * If the transaction was successful, the record is removed from the queue;
 * otherwise, it is kept on the queue, and the system takes steps to insure a
 * successful retransmission. This may imply waiting for a notification from the
 * destination that the port is again available, doing a port search or obtaining
 * a new transmission key. The port record is marked PORT_INFO_SUSPENDED when
 * some operation is in progress locally to attempt to obtain new information,
 * and PORT_INFO_BLOCKED when an external notification is expected. In addition,
 * a retry_level is maintained for each outstanding IPC record, and for the
 * destination port. This retry_level is incremented each time new information
 * is obtained about the destination port; when a transmission fails, but the
 * level of information under which it was initiated is lower than the current
 * retry_level (because some other transaction has obtained new information in
 * the meantime), a retransmission is attempted at once with the new information.
 *
 * Retransmissions (or first-time transmissions for records that have never been
 * active) are initiated when a reply is received or when new information about
 * the destination port is obtained, provided that transit_count is 0 and that
 * the port is neither suspended nor blocked. Retransmissions are always effected
 * by activating the first IPC_REC_WAITING record on the queue, so as to maintain
 * a strict ordering of delivery. Note that since RPC's do not block the queue,
 * records are not necessarily removed from the queue in the order of their
 * arrival; further, since RPC's are always retransmitted as IPC's, waiting IPC
 * records can find themselves ahead of active IPC records on the queue. The
 * ordering guarantees apply only to messages that were treated as simple IPC's
 * from the time of their arrival on the queue.
 */

/*
 * Static variables used for the re-send mechanism.
 */
PRIVATE struct mutex		ipc_re_send_lock;
PRIVATE struct condition	ipc_re_send_signal;
PRIVATE sys_queue_head_t	ipc_re_send_q;


PUBLIC unsigned long		ipc_sequence_number;

#define	IPC_MAX_RETRIES		10


/*
 * Macro to transmit or retransmit a request.
 */
#define	ipc_out_transmit(reply_port_ptr,ipc_ptr,destination,crypt_level) {	\
	int		tr_ret;							\
	port_rec_ptr_t	rp_ptr = reply_port_ptr;				\
										\
	/* reply_port_ptr LOCK RW/RW */						\
	if ((ipc_ptr)->type == IPC_REC_TYPE_CLIENT) {				\
		rp_ptr->portrec_reply_ipcrec = (pointer_t) (ipc_ptr);		\
		(ipc_ptr)->trmod = tr_default_entry;				\
	} else {								\
		(ipc_ptr)->trmod = tr_default_entry;				\
	}									\
	(ipc_ptr)->trid = 0;							\
	(ipc_ptr)->out.dest = (destination);					\
	(ipc_ptr)->out.crypt_level = (crypt_level);				\
										\
	DEBUG0(debug.ipc_out,0,2519);						\
	tr_ret = transport_sendrequest((ipc_ptr)->trmod,			\
				(int)(ipc_ptr),&((ipc_ptr)->out.msg),		\
				(destination),(crypt_level),ipc_in_reply);	\
										\
	if (rp_ptr) {								\
		lk_unlock(&rp_ptr->portrec_lock);				\
	}									\
	/* reply_port_ptr LOCK -/- */						\
										\
	if (tr_ret != TR_SUCCESS) {						\
		DEBUG1(debug.ipc_out,0,2500,tr_ret);				\
		ipc_in_reply((ipc_ptr),tr_ret,(sbuf_ptr_t)0);			\
	}									\
}


/*
 * Macro to translate an incoming netport for the ipc module, and
 * initialize it if necessary.
 *
 * Watch out for the two versions because of the NETPORT conditional.
 */
#if	NETPORT
#define ipc_find_netport(net_port_ptr,local_port,net_port,crypt_level) {	\
	(net_port_ptr) = (port_rec_ptr_t) pr_nportlookup(&(net_port));		\
	if ((net_port_ptr) == PORT_REC_NULL) {					\
		(net_port_ptr) = pr_ntran(&(net_port));				\
		if ((net_port_ptr) == PORT_REC_NULL) {				\
			(local_port) = PORT_NULL;				\
		} else {							\
			/* net_port_ptr LOCK RW/RW */				\
			/*							\
			 * First time that we have seen this port -		\
			 * remember its security level.				\
			 */							\
			(net_port_ptr)->portrec_security_level = (crypt_level);	\
			ipc_netport_enter((net_port_ptr)->portrec_network_port,	\
					(net_port_ptr)->portrec_local_port,	\
		NPORT_HAVE_REC_RIGHTS((net_port_ptr->portrec_network_port)));	\
			(local_port) = (net_port_ptr)->portrec_local_port;	\
		}								\
	} else {								\
		/* net_port_ptr LOCK RW/RW */					\
		(local_port) = (net_port_ptr)->portrec_local_port;		\
	}									\
	/* net_port_ptr LOCK RW/RW */						\
}
#else	NETPORT
#define ipc_find_netport(net_port_ptr,local_port,net_port,crypt_level) {	\
	(net_port_ptr) = (port_rec_ptr_t) pr_nportlookup(&(net_port));		\
	if ((net_port_ptr) == PORT_REC_NULL) {					\
		(net_port_ptr) = pr_ntran(&(net_port));				\
		if ((net_port_ptr) == PORT_REC_NULL) {				\
			(local_port) = PORT_NULL;				\
		} else {							\
			/* net_port_ptr LOCK RW/RW */				\
			/*							\
			 * First time that we have seen this port -		\
			 * remember its security level.				\
			 */							\
			(net_port_ptr)->portrec_security_level = (crypt_level);	\
			(local_port) = (net_port_ptr)->portrec_local_port;	\
		}								\
	} else {								\
		/* net_port_ptr LOCK RW/RW */					\
		(local_port) = (net_port_ptr)->portrec_local_port;		\
	}									\
	/* net_port_ptr LOCK RW/RW */						\
}
#endif	NETPORT


/*
 * Note on the transit_count:
 *
 * To avoid re-acquiring the destination port record lock after making a decision
 * for a IPC or RPC transmission, ipc_out_request() does not directly increment 
 * the count of messages in the out queue waiting for an immediate acknowledgment.
 * Instead, all routines that care about this count must first check the port
 * record for the presence of an ipc_rec in the 'lazy' field. If this ipc_rec
 * corresponds to a IPC in progress, the count must be incremented.
 *
 * This transit_count may thus be wrong if examined after the 'lazy' message is
 * put on the out queue, but before it is marked for IPC. There are two cases
 * where this problem is relevant:
 * - when processing a new request, in a new call to ipc_out_request(): this
 *   cannot be a problem if ipc_out_request() is the only entry point for new
 *   requests, and it is not entered recursively.
 * - when processing a retry for another message in the out queue: if no new
 *   message is transmitted as long as there is a message in transit, this
 *   retry can only be for a message ahead of the 'lazy' message in the out queue.
 *   But then, this message must have been part of a RPC transaction, and its
 *   ordering properties with respect to other messages are undefined.
 *
 * The following macro performs the necessary evaluation. Since it examines the
 * type field of an ipc_rec wihtout any protection, it relies on the fact that
 * stores to a single memory location are atomic. XXX
 */
#define	FIX_TRANSIT_COUNT(port_rec_ptr) {					\
	/* port_rec_ptr LOCK RW/RW */						\
	if ((port_rec_ptr->portrec_lazy_ipcrec) &&				\
		(((ipc_rec_ptr_t)port_rec_ptr->portrec_lazy_ipcrec)->type ==	\
							IPC_REC_TYPE_SINGLE)) {	\
		port_rec_ptr->portrec_lazy_ipcrec = 0;				\
		port_rec_ptr->portrec_transit_count++;				\
	}									\
	/* port_rec_ptr LOCK RW/RW */						\
}


/*
 * Forward declarations.
 */
void		ipc_out_request();
void		ipc_out_reply();
void		ipc_in_reply();
void		ipc_retry_internal();
/*int		ipc_retry();*/
/*void		ipc_freeze();*/
void		ipc_redo_request();
extern int	ipc_in_abortreq();


/*
 * ipc_out_main -- 
 *
 * Parameters: 
 *
 * none 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * Loops forever, waiting to receive an IPC message on a local port, to be
 * transmitted on the network. Whenever such a message is received, the
 * procedure locates the destination port and calls the request or reply
 * handling procedure as appropriate.
 *
 * Notes:
 *
 * This procedure is the main loop of the IPC Send thread. 
 *
 */
PUBLIC void ipc_out_main()
BEGIN("ipc_out_main")
	msg_header_t		*rcvbuff;	/* buffer for IPC receive */
	msg_return_t		msg_ret;	/* return from netmsg_receive */
	port_rec_ptr_t		dest_port_ptr;	/* port record for destination */

#if	LOCK_THREADS
	mutex_lock(thread_lock);
#endif	LOCK_THREADS

	for (;;) {
		/*
		 * Wait to receive a message.
		 */
		MEM_ALLOCOBJ(rcvbuff,msg_header_t *,MEM_IPCBUFF);

/*		DEBUG2(debug.ipc_out,0,2583,log_cur_ptr,log_end_ptr); */

		LOGCHECK;

		rcvbuff->msg_local_port = PORT_ENABLED;
		rcvbuff->msg_size = MSG_SIZE_MAX;

		msg_ret = netmsg_receive(rcvbuff);

		if (msg_ret != RCV_SUCCESS) {
			ERROR((msg, "ipc_out_main: netmsg_receive returned %d",
				msg_ret));
			MEM_DEALLOCOBJ(rcvbuff,MEM_IPCBUFF);
			continue;
		}

		DEBUG2(debug.ipc_out,1,2514,
		       (long)rcvbuff->msg_local_port,rcvbuff->msg_id);
		INCSTAT(ipc_out_messages);

		/*
		 * Find the port record for the destination.
		 */
		dest_port_ptr = (port_rec_ptr_t)pr_lportlookup(rcvbuff->msg_local_port);
		/* dest_port_ptr LOCK RW/RW */
		if (dest_port_ptr == 0) {
			ERROR((msg,
				"ipc_out_main: received a message on an unknown port"));
			MEM_DEALLOCOBJ(rcvbuff,MEM_IPCBUFF);
			/*
			 * Should also deallocate any ool areas. XXX
			 */
			continue;
		}

		DEBUG1(debug.ipc_out,0,2515,(long)dest_port_ptr);

		if (awaiting_local_reply(dest_port_ptr))
			ipc_out_reply(rcvbuff,dest_port_ptr);
		else
			ipc_out_request(rcvbuff,dest_port_ptr);
		/* dest_port_ptr LOCK -/- */
	}

	/* NOTREACHED */

END


/*
 * ipc_out_request --
 *
 * Put an IPC request on the net.
 *
 * Parameters:
 *
 * rcvbuff: buffer in which the IPC message was received.
 * server_port_ptr: port record for the destination.
 *
 * Results:
 *
 * Side effects:
 *
 * Calls a transport module.
 *
 * Design:
 *
 * Note:
 *
 * Should be called with server_port_ptr locked. The record
 * is unlocked on exit.
 *
 */
PRIVATE void ipc_out_request(rcvbuff,server_port_ptr)
	msg_header_t		*rcvbuff;
	port_rec_ptr_t		server_port_ptr;
BEGIN("ipc_out_request")
	port_rec_ptr_t		reply_port_ptr;
	ipc_rec_ptr_t		ipc_ptr;
	netaddr_t		destination;
	int			crypt_level;
	int			error_status;
	key_t			key;

	/* server_port_ptr LOCK RW/RW */

	/*
	 * Start by being optimistic. As we check for various problems,
	 * we will decide what error action is appropriate, if any.
	 */
	error_status = 0;
	FIX_TRANSIT_COUNT(server_port_ptr);
	server_port_ptr->portrec_aliveness = PORT_ACTIVE;

	/*
	 * The server port is not awaiting a local reply, by
	 * the conditions under which this procedure is called.
	 *
	 * The server port cannot be local, or this would not be
	 * called at all. We never make a remote port wait for
	 * a remote reply with the MSG_TYPE_RPC convention.
	 */

	/*
	 * Allocate an IPC record.
	 */
	MEM_ALLOCOBJ(ipc_ptr,ipc_rec_ptr_t,MEM_IPCREC);
	ipc_ptr->status = IPC_REC_ACTIVE;
	DEBUG1(debug.ipc_out,0,2516,(long)ipc_ptr);

	/*
	 * Translate the server port and queue the IPC record.
	 */
	ipc_ptr->server_port_ptr = server_port_ptr;
	pr_reference(server_port_ptr);
	destination = PORT_REC_RECEIVER(server_port_ptr);
	ipc_ptr->out.netmsg_hdr.local_port = server_port_ptr->portrec_network_port;
	sys_queue_enter(&server_port_ptr->portrec_out_ipcrec,
			ipc_ptr,ipc_rec_ptr_t,out_q);
	ipc_ptr->retry_level = server_port_ptr->portrec_retry_level;
	DEBUG1(debug.ipc_out,0,2513,ipc_ptr->retry_level);
	if (server_port_ptr->portrec_info & PORT_INFO_DEAD) {
		error_status = IPC_ABORT_REQUEST;
	} else {
		if (PORT_BUSY(server_port_ptr)) {
			DEBUG2(debug.ipc_out,0,2517,server_port_ptr->portrec_info,
					server_port_ptr->portrec_transit_count);
			ipc_freeze(server_port_ptr);
			error_status = IPC_PORT_BUSY;
		}
	}
	ipc_ptr->type = IPC_REC_TYPE_UNKNOWN;
	server_port_ptr->portrec_lazy_ipcrec = (pointer_t)ipc_ptr;
	lk_unlock(&server_port_ptr->portrec_lock);
	/* server_port_ptr LOCK -/- */

	/*
	 * Translate the message.
	 */
	crypt_level = (rcvbuff->msg_type & MSG_TYPE_ENCRYPTED) ? 
					CRYPT_ENCRYPT : CRYPT_DONT_ENCRYPT;
	ipc_outmsg(ipc_ptr, rcvbuff, destination, crypt_level);

	if (error_status == 0) {
		/*
		 * If the message is to be encrypted, check that
		 * we have a key for the destination.
		 */
#if	USE_CRYPT
		if (((crypt_level) != CRYPT_DONT_ENCRYPT) &&
			(!(km_get_key((destination), &key)))) {
			DEBUG0(debug.ipc_out,0,2518);
			error_status = TR_CRYPT_FAILURE;
		}
#endif	USE_CRYPT
	}

	/*
	 * Translate the reply port.
	 */
	if (rcvbuff->msg_remote_port == PORT_NULL) {
		ipc_ptr->out.netmsg_hdr.remote_port = null_network_port;
		reply_port_ptr = NULL;
		ipc_ptr->reply_port_ptr = NULL;
		ipc_ptr->type = IPC_REC_TYPE_SINGLE;
		DEBUG0(debug.ipc_out,0,2509);
	} else if ((reply_port_ptr = pr_ltran(rcvbuff->msg_remote_port)) ==
			PORT_REC_NULL) {
		panic("ipc_out_request.pr_ltran");
	} else {	/* valid reply port */

		/* reply_port_ptr LOCK RW/RW */
    		DEBUG1(debug.ipc_out,0,2531,reply_port_ptr);
		ipc_ptr->out.netmsg_hdr.remote_port = 
			reply_port_ptr->portrec_network_port;

		/*
		 * Check for pending transactions.
		 */
		if (reply_port_ptr->portrec_reply_ipcrec != NULL) {
			DEBUG3(debug.ipc_out,0,2532,
					reply_port_ptr->portrec_reply_ipcrec,
		((ipc_rec_ptr_t)reply_port_ptr->portrec_reply_ipcrec)->type,
		((ipc_rec_ptr_t)reply_port_ptr->portrec_reply_ipcrec)->status);
			/*
			 * Abort the pending transaction.
			 */
			switch(((ipc_rec_ptr_t)reply_port_ptr->
						portrec_reply_ipcrec)->status) {
				case IPC_REC_ACTIVE:
					if ((((ipc_rec_ptr_t)reply_port_ptr->
						portrec_reply_ipcrec)->type) != 
							IPC_REC_TYPE_CLIENT) {
						ERROR((msg,
		"unexpected ipc record type waiting in reply port record: %d", 
		((ipc_rec_ptr_t)reply_port_ptr->portrec_reply_ipcrec)->type));
						panic("ipc_out_request");
					}
					/*
					 * Next operation while still
					 * waiting for previous RPC
					 * to complete.
					 */
					ipc_client_abort(reply_port_ptr);
					break;
				case IPC_REC_REPLY:
					/*
					 * Somebody is trying to forward
					 * an RPC with another RPC.
					 */
					ipc_server_abort(reply_port_ptr);
					break;
				default:
					ERROR((msg,
		"unexpected ipc record status waiting in reply port record: %d", 
		((ipc_rec_ptr_t)reply_port_ptr->portrec_reply_ipcrec)->status));
					panic("ipc_out_request");
					break;
			}
		}

		if ((rcvbuff->msg_type & MSG_TYPE_RPC) && (error_status == 0) &&
		    	(NPORT_HAVE_REC_RIGHTS(reply_port_ptr->portrec_network_port))) {
			/*
			 * Valid RPC. Go for it.
			 */
			DEBUG0(debug.ipc_out,0,2533);
			ipc_ptr->out.netmsg_hdr.info |= IPC_INFO_RPC;
			ipc_ptr->type = IPC_REC_TYPE_CLIENT;
		} else {
			lk_unlock(&reply_port_ptr->portrec_lock);
			/* reply_port_ptr LOCK -/- */
			DEBUG0(debug.ipc_out,0,2534);
			reply_port_ptr = NULL;
			ipc_ptr->type = IPC_REC_TYPE_SINGLE;
		}
		ipc_ptr->reply_port_ptr = reply_port_ptr;
		if (reply_port_ptr)
			pr_reference(reply_port_ptr);
	}		/* valid reply port */

	/* reply_port_ptr LOCK RW/RW */

	/*
	 * Transmit the message if appropriate.
	 */
	if (error_status == 0) {
		ipc_out_transmit(reply_port_ptr,ipc_ptr,destination,crypt_level);
		/* reply_port_ptr LOCK -/- */
	} else {
		if (reply_port_ptr) {
			lk_unlock(&reply_port_ptr->portrec_lock);
		}
		/* reply_port_ptr LOCK -/- */
		DEBUG1(debug.ipc_out,0,2535,error_status);
		ipc_ptr->trid = 0;
		ipc_in_reply(ipc_ptr,error_status,(sbuf_ptr_t)0);
	}

	/* reply_port_ptr LOCK -/- */

	RET;

END


/*
 * ipc_out_reply --
 *
 * Put an IPC reply on the net.
 *
 * Parameters:
 *
 * rcvbuff: buffer in which the IPC message was received.
 * reply_port_ptr: port record for the destination.
 *
 * Results:
 *
 * Side effects:
 *
 * Calls a transport module.
 *
 * Design:
 *
 * Note:
 *
 * Should be called with reply_port_ptr locked. The record
 * is unlocked on exit.
 *
 */
PRIVATE void ipc_out_reply(rcvbuff,reply_port_ptr)
	msg_header_t		*rcvbuff;
	port_rec_ptr_t		reply_port_ptr;
BEGIN("ipc_out_reply")
	ipc_rec_ptr_t		ipc_ptr;
	port_rec_ptr_t		server_port_ptr;
	netaddr_t		destination;
	int			crypt_level;
	key_t			key;
	int			tr_ret;

	/* reply_port_ptr LOCK RW/RW */

	/*
	 * The reply port is awaiting a local reply, by
	 * the conditions under which this procedure is called.
	 *
	 * It is not possible to be waiting for more than one 
	 * reply at a time.
	 */

	/*
	 * Find the IPC record.
	 */
	ipc_ptr = (ipc_rec_ptr_t)reply_port_ptr->portrec_reply_ipcrec;
	DEBUG1(debug.ipc_out,0,2536,(long)ipc_ptr);

	/*
	 * Translate the reply port and clean up the port record.
	 */
	destination = PORT_REC_RECEIVER(reply_port_ptr);
	ipc_ptr->out.netmsg_hdr.local_port = reply_port_ptr->portrec_network_port;
	reply_port_ptr->portrec_reply_ipcrec = NULL;
	lk_unlock(&reply_port_ptr->portrec_lock);
	/* reply_port_ptr LOCK -/- */

	/*
	 * Translate the server port (or whatever port takes its place
	 * in the reply message).
	 *
	 * This port is not part of the RPC mechanism at this point, so
	 * it does not matter whether it is waiting for some reply or not.
	 */
	if (rcvbuff->msg_remote_port == PORT_NULL) {
		ipc_ptr->out.netmsg_hdr.remote_port = null_network_port;
		server_port_ptr = NULL;
		DEBUG0(debug.ipc_out,0,2537);
	} else if ((server_port_ptr = pr_ltran(rcvbuff->msg_remote_port)) ==
			PORT_REC_NULL) {
		panic("ipc_out_reply.pr_ltran");
	} else {	/* valid server port */

		/* server_port_ptr LOCK RW/RW */
		DEBUG1(debug.ipc_out,0,2538,server_port_ptr);
		ipc_ptr->out.netmsg_hdr.remote_port = 
			server_port_ptr->portrec_network_port;
		lk_unlock(&server_port_ptr->portrec_lock);
		/* server_port_ptr LOCK -/- */
	}		/* valid server port */

	/*
	 * Translate the message.
	 */
	crypt_level = (rcvbuff->msg_type & MSG_TYPE_ENCRYPTED) ? 
					CRYPT_ENCRYPT : CRYPT_DONT_ENCRYPT;
	ipc_outmsg(ipc_ptr, rcvbuff, destination, crypt_level);

	ipc_ptr->out.dest = destination;
	ipc_ptr->out.crypt_level = crypt_level;

	/*
	 * If the message is to be encrypted, check that
	 * we have a key for the destination.
	 */
#if	USE_CRYPT
	if ((crypt_level != CRYPT_DONT_ENCRYPT) &&
		(!(km_get_key(ipc_ptr->out.dest, &key)))) {
		ERROR((msg,"ipc_out_reply: encryption problem"));
	} else {
#endif	USE_CRYPT
		DEBUG2(debug.ipc_out,0,2539,ipc_ptr->trmod,ipc_ptr->trid);
		tr_ret = transport_sendreply(ipc_ptr->trmod,ipc_ptr->trid,
					IPC_SUCCESS,&(ipc_ptr->out.msg),crypt_level);
		if (tr_ret != TR_SUCCESS) {
			ERROR((msg,"ipc_out_reply: transport_sendreply returned %d",
									tr_ret));
		}
#if	USE_CRYPT
	}
#endif	USE_CRYPT

	/*
	 * Since there will be no acknowledgment, we must pretend
	 * that everything is fine. If the message is lost because
	 * the user moved the reply port, the rights it contains
	 * may be lost too. This is the price of using MSG_TYPE_RPC.
	 */
	if (ipc_ptr->out.npd_exists) {
		po_port_rights_commit((int)ipc_ptr,PO_RIGHTS_XFER_SUCCESS,destination);
	}

	/*
	 * Deallocate resources used by this transaction.
	 */
	DEBUG0(debug.ipc_out,0,2546);
	ipc_in_gc(ipc_ptr);
	ipc_out_gc(ipc_ptr);

	RET;
END


/*
 * ipc_in_reply --
 *
 * Handle an incoming reply from the network.
 *
 * Parameters:
 *
 * ipc_ptr: pointer to the IPC record for the pending transaction.
 * code: success code for the transaction.
 * data_ptr: optional sbuf containing a response.
 *
 * Results:
 *
 * none.
 *
 * Side effects:
 *
 * May deliver a RPC reply.
 * Deallocates the IPC record.
 *
 * Design:
 *
 * Note:
 *
 * The data in *data_ptr must be kept valid while this procedure is
 * executing; it may be deallocated as soon as the procedure returns.
 *
 * This procedure must be called exactly once for each call to 
 * transport_sendrequest(). It is called from a transport module, via
 * the dispatcher, or may be called directly from the RPC module
 * if it does not pass the request to a transport module.
 *
 * According to the locking strategy for IPC records, *ipc_ptr is
 * readable on entry, but must be unlinked from the reply port record
 * before it can be modified.
 *
 */
EXPORT void ipc_in_reply(ipc_ptr,code,data_ptr)
	ipc_rec_ptr_t		ipc_ptr;
	int			code;
	sbuf_ptr_t		data_ptr;
BEGIN("ipc_in_reply")
	port_rec_ptr_t		server_port_ptr;
	port_rec_ptr_t		reply_port_ptr;
	port_rec_ptr_t		local_port_ptr;
	port_t			local_port;
	port_t			reply_port;
	ipc_netmsg_hdr_t	*nmh_ptr;	/* netmsg header */
	boolean_t		do_swap;
	sbuf_seg_ptr_t		sb_ptr;		/* pointer into data sbuf */
	msg_return_t		msg_ret;
	kern_return_t		kern_ret;
	boolean_t		retry;
	int			retry_action;
	int			completion_rights;
	netaddr_t		from;
	boolean_t		msg_ok;

	/*
	 * Values for retry_action.
	 */
#define	RETRY_NONE		0
#define	RETRY_WAIT		1
#define	RETRY_PORT_SEARCH	2
#define	RETRY_KEY_EXCHANGE	3

	DEBUG3(debug.ipc_in,0,2547,ipc_ptr,code,data_ptr);
	/*
	 * Find out if there is a reply port associated with
	 * this transaction, and if so unlink the ipc record
	 * from it.
	 *
	 * If the reply port is waiting for something else,
	 * leave it alone; we do not need it anymore at
	 * this point.
	 */
	reply_port_ptr = ipc_ptr->reply_port_ptr;
	if (reply_port_ptr) {
		lk_lock(&reply_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
		/* reply_port_ptr LOCK RW/RW */

		DEBUG2(debug.ipc_in,0,2548,reply_port_ptr,
					reply_port_ptr->portrec_reply_ipcrec);
		if (reply_port_ptr->portrec_reply_ipcrec == (pointer_t)ipc_ptr) {
			reply_port_ptr->portrec_reply_ipcrec = 0;
		}

		reply_port = reply_port_ptr->portrec_local_port;

		lk_unlock(&reply_port_ptr->portrec_lock);
		/* reply_port_ptr LOCK -/- */
	} else {
		DEBUG0(debug.ipc_in,0,2549);
		reply_port = PORT_NULL;
	}
	/* reply_port_ptr LOCK -/- */

	/*
	 * Find the (server) port record controlling this transaction.
	 */
	server_port_ptr = ipc_ptr->server_port_ptr;
	DEBUG1(debug.ipc_in,0,2550,server_port_ptr);

	/*
	 * If there is any response message, deliver it first.
	 */
	if (data_ptr) {

		/*
		 * Get a pointer to the netmsg header and fix the
		 * ipc sequence number.
		 */
		sb_ptr = data_ptr->segs;
		while (sb_ptr->s == 0)		/* skip past empty segments */
			sb_ptr++;
		nmh_ptr = (ipc_netmsg_hdr_t *) sb_ptr->p;
		nmh_ptr->disp_hdr.src_format = ntohs(nmh_ptr->disp_hdr.src_format);
		do_swap = DISP_CHECK_SWAP(nmh_ptr->disp_hdr.src_format);
		if (do_swap) {
			SWAP_DECLS;

			(void) SWAP_LONG(nmh_ptr->ipc_seq_no, nmh_ptr->ipc_seq_no);
		}
		from = ipc_ptr->out.dest;
		msg_ok = TRUE;

		/*
		 * Check the sequence number if appropriate.
		 *
		 * If it is not OK, there is nothing much we can do, since
		 * the transport module has already been fooled at this point.
		 * Just report the error and ignore the data. The transport
		 * module itself should protect us against this sort of problem.
		 */
#if	USE_CRYPT
		if (ipc_ptr->out.crypt_level != CRYPT_DONT_ENCRYPT) {
			if (!po_check_ipc_seq_no(server_port_ptr, from, nmh_ptr->ipc_seq_no)) {
				/*
				 * Sequence number check failed, reject.
				 */
				ERROR((msg,
		"*** Invalid sequence number on IPC reply apparently from 0x%x",
									from));
				msg_ok = FALSE;
			}
		}	
#endif	USE_CRYPT


		/*
		 * Assemble and translate the message.
		 */
		ipc_inmsg(ipc_ptr,data_ptr,nmh_ptr,
		  	ipc_ptr->out.dest,ipc_ptr->out.crypt_level,do_swap);

		/*
		 * The reply port in the message had better be
		 * the same as the one in the ipc record !
		 */
		ipc_ptr->in.assem_buff->msg_remote_port = reply_port;

		/*
		 * Translate the local port (= server port ?).
		 *
		 * Should have a special case if the local port is
		 * the original server port. XXX
		 */
		ipc_find_netport(local_port_ptr,local_port,
				 	nmh_ptr->remote_port,
					ipc_ptr->out.crypt_level);
		/* local_port_ptr LOCK RW/RW */

		/*
		 * This port is not part of the RPC mechanism at this point, so
		 * it does not matter whether it is waiting for some reply or not.
		 */

		if (local_port_ptr) {
			lk_unlock(&local_port_ptr->portrec_lock);
		}
		/* local_port_ptr LOCK -/- */

		ipc_ptr->in.assem_buff->msg_local_port = local_port;

		if (msg_ok) {
			/*
			 * Deliver the message.
			 */
			DEBUG1(debug.ipc_in,0,2551,local_port_ptr);

#if	CAMELOT
			if (Cam_Message(Cam_MsgHeader(ipc_ptr->in.assem_buff))) {
				char *data;

				mutex_lock(&camelot_lock);
				data = Cam_Receive(Cam_MsgHeader(
						ipc_ptr->in.assem_buff),from);

				if (data != NULL) {
					msg_ret = msg_send(ipc_ptr->in.assem_buff,
								SEND_TIMEOUT, 1);
#if	0
					Cam_Restore(Cam_MsgHeader(
						ipc_ptr->in.assem_buff),data);
#endif	0
				} else
					msg_ret = SEND_SUCCESS;	/* drop message */
				mutex_unlock(&camelot_lock);
			} else
#endif	CAMELOT
			msg_ret = msg_send(ipc_ptr->in.assem_buff,SEND_TIMEOUT,1);

			INCSTAT(ipc_in_messages);
			DEBUG2(debug.ipc_in,0,2569,ipc_ptr->in.assem_buff->msg_id,msg_ret);
		} else {
			msg_ret = SEND_SUCCESS;
		}

		if (msg_ret != SEND_SUCCESS) {
			ERROR((msg,"msg_send(reply) returned %d",msg_ret));
		}

		/*
		 * Deallocate resources used for this incoming message.
		 */
		DEBUG0(debug.ipc_in,0,2542);
		ipc_in_gc(ipc_ptr);
	}

	/*
	 * Worry about the success code for this transaction,
	 * and retry it if necessary.
	 */
	lk_lock(&server_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
	/* server_port_ptr LOCK RW/RW */

	server_port_ptr->portrec_aliveness = PORT_ACTIVE;

	switch(code) {
		case IPC_SUCCESS:
		case IPC_ABORT_REPLY:
			retry = FALSE;
			completion_rights = PO_RIGHTS_XFER_SUCCESS;
			break;

		case IPC_ABORT_REQUEST:
	    		retry = FALSE;
			completion_rights = PO_RIGHTS_XFER_FAILURE;
			break;

		case IPC_PORT_NOT_HERE:
			retry = TRUE;
			retry_action = RETRY_PORT_SEARCH;
			break;

		case IPC_PORT_BLOCKED:
		case TR_OVERLOAD:
			retry = TRUE;
			retry_action = RETRY_WAIT;
			break;

		case IPC_PORT_BUSY:
			retry = TRUE;
			retry_action = RETRY_NONE;
			break;

		case TR_FAILURE:
		case TR_SEND_FAILURE:
		    	/*
			 * XXX Should worry about total failure of a transport 
			 * module, and avoid retrying forever the same thing 
			 * after each successful port search.
			 */
			ERROR((msg,
  "Warning: ipc_in_reply: the transport module failed to get a message to machine 0x%x.",
				ipc_ptr->out.dest));
			retry = TRUE;
			retry_action = RETRY_PORT_SEARCH;
			break;

		case TR_CRYPT_FAILURE:
			ERROR((msg, 
  "Warning: ipc_in_reply: the transport module does not have a key for machine 0x%x.",
				ipc_ptr->out.dest));
			retry = TRUE;
			retry_action = RETRY_KEY_EXCHANGE;
			break;

		case IPC_BAD_SEQ_NO:
			ERROR((msg, 
	"ipc_in_reply: IPC_BAD_SEQ_NO response received from machine %x.",
				ipc_ptr->out.dest));
	    		retry = FALSE;
			completion_rights = PO_RIGHTS_XFER_FAILURE;
			break;

		default:
			ERROR((msg, "ipc_in_reply: unknown completion code: %d",
									code));
	    		retry = FALSE;
			completion_rights = PO_RIGHTS_XFER_FAILURE;
			break;
	}

	if (retry && (ipc_ptr->retry_level > IPC_MAX_RETRIES)) {
		ERROR((msg,
	"  Abandoning transmission of a message to port 0x%x: too many retries.",
							server_port_ptr));
    		retry = FALSE;
		completion_rights = PO_RIGHTS_XFER_FAILURE;
	}

	/*
	 * Make sure portrec_transit_count takes into account the last new
	 * request seen by ipc_out_request. We probably only need that if
	 * we are going to use PORT_BUSY, but let us be careful... XXX
	 */
	if (server_port_ptr->portrec_lazy_ipcrec == (pointer_t)ipc_ptr) {
		/*
		 * Even if the current transaction was a RPC, it is out
		 * of the 'lazy' phase now.
		 */
		server_port_ptr->portrec_lazy_ipcrec = 0;
	} else {
		FIX_TRANSIT_COUNT(server_port_ptr);
		if (ipc_ptr->type == IPC_REC_TYPE_SINGLE) {
			server_port_ptr->portrec_transit_count--;
			DEBUG0(debug.ipc_in,0,2579);
		}
	}

	DEBUG4(debug.ipc_in,0,2543,ipc_ptr->retry_level,
					server_port_ptr->portrec_retry_level,
					server_port_ptr->portrec_waiting_count,
					server_port_ptr->portrec_transit_count);
	DEBUG3(debug.ipc_in,0,2570,retry,completion_rights,retry_action);

	if (retry) {
		/*
		 * Set up the present message for a retry.
		 */
		ipc_ptr->status = IPC_REC_WAITING;
		server_port_ptr->portrec_waiting_count++;
		if (!PORT_BUSY(server_port_ptr)) {
			if (server_port_ptr->portrec_retry_level > 
							ipc_ptr->retry_level) {
				/*
				 * There is new information. Attempt to
				 * retransmit immediately.
				 *
				 * In the normal case, this message will
				 * be the first one in the waiting state
				 * on the out queue, but anyway, if this
				 * is not the case, the messages ahead of
				 * it will cause further retries.
				 *
				 * (We cannot avoid other messages going from
				 * 'active' to 'waiting' as soon as we release
				 * the lock on the port record).
				 */
				DEBUG0(debug.ipc_in,0,2571);
				ipc_retry_internal(server_port_ptr);
			} else {
				ipc_freeze(server_port_ptr);
				switch(retry_action) {
					case RETRY_NONE:
						/*
						 * Retry at once, since the
						 * port is no longer busy.
						 */
						DEBUG0(debug.ipc_in,0,2572);
						ipc_retry_internal(server_port_ptr);
						break;

					case RETRY_WAIT:
						/*
						 * Sit until a PORT_UNBLOCKED
						 * comes around.
						 */
						server_port_ptr->portrec_info |=
							PORT_INFO_BLOCKED;
						DEBUG0(debug.ipc_in,0,2573);
						break;

					case RETRY_PORT_SEARCH:
						server_port_ptr->portrec_info |= 
							PORT_INFO_SUSPENDED;
						DEBUG0(debug.ipc_in,0,2574);
						ps_do_port_search(server_port_ptr, 
							FALSE,
							((network_port_ptr_t) 0),
							ipc_retry);
						break;

					case RETRY_KEY_EXCHANGE:
						server_port_ptr->portrec_info |= 
							PORT_INFO_SUSPENDED;
						DEBUG0(debug.ipc_in,0,2575);
						km_do_key_exchange(server_port_ptr,
							ipc_retry,
							ipc_ptr->out.dest);
						break;

					default:
						ERROR((msg,
					"ipc_in_reply: unknown retry_action=%d",
							       retry_action));
						break;
				}
			}
		} else {
			/*
			 * Just wait. ipc_retry() will be called later
			 * when the port is freed.
			 */
			DEBUG2(debug.ipc_in,0,2576,server_port_ptr->portrec_info,
			       server_port_ptr->portrec_transit_count);
		}
		lk_unlock(&server_port_ptr->portrec_lock);
		/* server_port_ptr LOCK -/- */
	} else {
		/*
		 * Finish processing of the present message, take it off
		 * the queue and see if a newer message needs to be retried.
		 */
		sys_queue_remove(&server_port_ptr->portrec_out_ipcrec,
						ipc_ptr,ipc_rec_ptr_t,out_q);
		if (sys_queue_empty(&server_port_ptr->portrec_out_ipcrec)) {
			server_port_ptr->portrec_retry_level = 0;
			DEBUG0(debug.ipc_in,0,2578);
		}

		if (server_port_ptr->portrec_waiting_count > 0) {
			/*
			 * Retry the next message.
			 */
			if (!PORT_BUSY(server_port_ptr)) {
				DEBUG0(debug.ipc_in,0,2580);
				ipc_retry_internal(server_port_ptr);
			} else {
				/*
				 * Just wait. ipc_retry() will be called later
				 * when the port is freed.
				 */
				DEBUG2(debug.ipc_in,0,2577,
					server_port_ptr->portrec_info,
					server_port_ptr->portrec_transit_count);
			}
		} else {
			if ((server_port_ptr->portrec_transit_count == 0) &&
			    (server_port_ptr->portrec_info & PORT_INFO_DISABLED)) {
				DEBUG0(debug.ipc_in,0,2545);
				kern_ret = port_enable(task_self(),
					server_port_ptr->portrec_local_port);
				if (kern_ret == KERN_SUCCESS) {
					server_port_ptr->portrec_info &= 
							~PORT_INFO_DISABLED;
				} else {
					ERROR((msg,"port_enable() returned %d",
									kern_ret));
				}
			}
		}

		pr_release(server_port_ptr);
		/* server_port_ptr LOCK -/- */

		/*
		 * Indicate the message transfer to the
		 * Port Operations module
		 */
		if (ipc_ptr->out.npd_exists) {
			po_port_rights_commit((int)ipc_ptr,completion_rights,
								ipc_ptr->out.dest);
		}

		ipc_out_gc(ipc_ptr);
	}

	/* server_port_ptr LOCK -/- */

	RET;
END



/*
 * ipc_in_request --
 *
 * Handle an incoming request from the network.
 *
 * Parameters:
 *
 * trmod: index of transport module delivering this request.
 * trid: ID used by the transport module for this request.
 * data_ptr: sbuf containing the request message.
 * from: the address of the network server where the message originated 
 * crypt_level: encryption level for this message.
 * broadcast: TRUE if the message was a broadcast. (IGNORED)
 *
 * Results:
 *
 * a code indicating how the request was accepted, and if a reply
 * will be forthcoming.
 *
 * Side effects:
 *
 * May deliver a RPC/IPC request.
 *
 * Design:
 *
 * Note:
 *
 * The data in *data_ptr must be kept valid until this procedure returns.
 *
 */
EXPORT int ipc_in_request(trmod,trid,data_ptr,from,crypt_level,broadcast)
	int		trmod;
	int		trid;
	sbuf_ptr_t	data_ptr;
	netaddr_t	from;
	int		crypt_level;
	boolean_t	broadcast;
BEGIN("ipc_in_request")
	ipc_rec_ptr_t		ipc_ptr;
	port_rec_ptr_t		server_port_ptr;
	port_rec_ptr_t		reply_port_ptr;
	ipc_netmsg_hdr_t	*nmh_ptr;	/* netmsg header */
	boolean_t		do_swap;
	sbuf_seg_ptr_t		sb_ptr;		/* pointer into data sbuf */
	port_t			server_port;
	port_t			reply_port;
	int			retval;
	msg_return_t		msg_ret;

#ifdef	lint
	broadcast = !broadcast;
#endif	lint

	DEBUG5(debug.ipc_in,0,2501,trmod,trid,data_ptr,from,crypt_level);

	/*
	 * Get a pointer to the netmsg header and fix the
	 * ipc sequence number.
	 */
	sb_ptr = data_ptr->segs;
	while (sb_ptr->s == 0)		/* skip past empty segments */
		sb_ptr++;
	nmh_ptr = (ipc_netmsg_hdr_t *) sb_ptr->p;
	do_swap = DISP_CHECK_SWAP(nmh_ptr->disp_hdr.src_format);
	if (do_swap) {
		SWAP_DECLS;

		(void) SWAP_LONG(nmh_ptr->ipc_seq_no, nmh_ptr->ipc_seq_no);
	}

	/*
	 * Find the destination port.
	 */
	server_port_ptr = (port_rec_ptr_t) pr_nportlookup(&nmh_ptr->local_port);
	/* server_port_ptr LOCK RW/RW */

	DEBUG2(debug.ipc_in,0,2528,server_port_ptr,server_port_ptr->portrec_info);

	/*
	 * Check the acceptability of the message.
	 */
	if ((server_port_ptr == PORT_REC_NULL) ||
		(!NPORT_HAVE_REC_RIGHTS(server_port_ptr->portrec_network_port)) ||
		(server_port_ptr->portrec_info & PORT_INFO_DEAD)) {
		/*
		 * Signal port_not_here 
		 */
		DEBUG0(debug.ipc_in,0,2502);
		DEBUG_NPORT(debug.ipc_in,0,nmh_ptr->local_port);
		if (server_port_ptr != PORT_REC_NULL)
			lk_unlock(&server_port_ptr->portrec_lock);
		/* server_port_ptr LOCK -/- */
		RETURN(IPC_PORT_NOT_HERE);
	}

	/*
	 * Check the sequence number if appropriate.
	 */
#if	USE_CRYPT
	if (crypt_level != CRYPT_DONT_ENCRYPT) {
		if (!po_check_ipc_seq_no(server_port_ptr,
						from,nmh_ptr->ipc_seq_no)) {
			/*
			 * Sequence number check failed, reject this message.
			 */
			lk_unlock(&server_port_ptr->portrec_lock);
			/* server_port_ptr LOCK -/- */
			ERROR((msg,
		"*** Invalid sequence number on IPC request apparently from 0x%x",
									from));
			DEBUG1(debug.ipc_in,0,2503,nmh_ptr->ipc_seq_no);
			RETURN(IPC_BAD_SEQ_NO);
		}
	}	
#endif	USE_CRYPT

	/*
	 * Worry about flow control.
	 */
	if (server_port_ptr->portrec_info & PORT_INFO_BLOCKED) {
		/*
		 * Signal port_blocked and add the sender to the waiting list 
		 */
		DEBUG0(debug.ipc_in,0,2504);
		ipc_in_block(server_port_ptr,from);
		lk_unlock(&server_port_ptr->portrec_lock);
		/* server_port_ptr LOCK -/- */
		RETURN(IPC_PORT_BLOCKED);
	}
	server_port_ptr->portrec_info |= PORT_INFO_BLOCKED;

	server_port = server_port_ptr->portrec_local_port;

	/*
	 * At this point, we are sure that we can accept the message.
	 * We could switch to another thread, to free the network thread,
	 * providing we make sure our data does not get deallocated
	 * too soon by the transport module and we are careful to avoid
	 * races in message delivery that could compromise ordering.
	 */
	DEBUG0(debug.ipc_in,0,2505);

	/*
	 * Check for a RPC reply coming as a new request.
	 *
	 * The server port is local, so it cannot be waiting
	 * for a local reply.
	 */
	if (awaiting_remote_reply(server_port_ptr)) {
		ipc_client_abort(server_port_ptr);
		nmh_ptr->info &= ~IPC_INFO_RPC;
	}

	lk_unlock(&server_port_ptr->portrec_lock);
	/* server_port_ptr LOCK -/- */

	/*
	 * Allocate an IPC record.
	 */
	MEM_ALLOCOBJ(ipc_ptr,ipc_rec_ptr_t,MEM_IPCREC);
	DEBUG1(debug.ipc_in,0,2552,ipc_ptr);
	ipc_ptr->status = IPC_REC_REPLY;
	ipc_ptr->type = IPC_REC_TYPE_SERVER;
	ipc_ptr->trmod = trmod;
	ipc_ptr->trid = trid;
	ipc_ptr->reply_port_ptr = PORT_REC_NULL;

	/*
	 * Assemble and translate the message.
	 */
	ipc_inmsg(ipc_ptr,data_ptr,nmh_ptr,from,crypt_level,do_swap);

	/*
	 * Find the reply port.
	 */
	ipc_find_netport(reply_port_ptr,reply_port,nmh_ptr->remote_port,crypt_level);
	/* reply_port_ptr LOCK RW/RW */

	DEBUG1(debug.ipc_in,0,2553,reply_port_ptr);

	if (reply_port_ptr != PORT_REC_NULL) {

		DEBUG1(debug.ipc_in,0,2554,reply_port_ptr->portrec_reply_ipcrec);
		reply_port_ptr->portrec_aliveness = PORT_ACTIVE;

		if (awaiting_local_reply(reply_port_ptr)) {
			/*
			 * We must abort the current RPC.
			 *
			 * The receiver for the reply port might also
			 * be sending us an abort request, but a double
			 * abort never hurt anybody...
			 */
			ipc_server_abort(reply_port_ptr);
		}

		/*
		 * If the reply port is local, it is possible that it is
		 * waiting for a remote reply. In that case, the present
		 * request cannot be handled as a RPC, but a simple IPC is OK.
		 * The RPC already pending on the reply port might be fine,
		 * but it is likely to run into trouble with multiple responses,
		 * so abort it just to be safe.
		 */

		/*
		 * Check for a RPC.
		 */
		if ((nmh_ptr->info & IPC_INFO_RPC) && 
			(transport_switch[trmod].sendreply != transport_no_function)) {
			DEBUG0(debug.ipc_in,0,2555);
			if (reply_port_ptr->portrec_reply_ipcrec) {
				ipc_client_abort(reply_port_ptr);
				retval = IPC_SUCCESS;
			} else {
				ipc_ptr->status = IPC_REC_REPLY;
				ipc_ptr->reply_port_ptr = reply_port_ptr;
				pr_reference(reply_port_ptr);
				reply_port_ptr->portrec_reply_ipcrec = 
								(pointer_t)ipc_ptr;
				retval = DISP_WILL_REPLY;
			}
		} else {
			retval = IPC_SUCCESS;
		}
		lk_unlock(&reply_port_ptr->portrec_lock);
	} else {
		retval = IPC_SUCCESS;
	}
	/* reply_port_ptr LOCK -/- */

	/*
	 * Set-up the remote and local ports.
	 */
	ipc_ptr->in.assem_buff->msg_remote_port = server_port;
	ipc_ptr->in.assem_buff->msg_local_port = reply_port;

	/*
	 * Deliver the message.
	 */
#if	CAMELOT
	if (Cam_Message(Cam_MsgHeader(ipc_ptr->in.assem_buff))) {
		char *data;

		mutex_lock(&camelot_lock);
		data = Cam_Receive(Cam_MsgHeader(ipc_ptr->in.assem_buff),from);

		if (data != NULL) {
			msg_ret = msg_send(ipc_ptr->in.assem_buff,SEND_NOTIFY, 0);
#if	0
			Cam_Restore(Cam_MsgHeader(ipc_ptr->in.assem_buff),data);
#endif	0
		} else
			msg_ret = SEND_SUCCESS;	/* drop message on floor */
		mutex_unlock(&camelot_lock);
	} else
#endif	CAMELOT

	msg_ret = msg_send(ipc_ptr->in.assem_buff, SEND_NOTIFY, 0);

	INCSTAT(ipc_in_messages);
	DEBUG2(debug.ipc_in,0,2507,ipc_ptr->in.assem_buff->msg_id,msg_ret);

	if (msg_ret == SEND_WILL_NOTIFY) {
		/*
		 * The port remains blocked. Do nothing. 
		 */
	} else {
		if (msg_ret != SEND_SUCCESS) {
			/*
			 * Something strange happened. Report it, then go on as usual. 
			 */
			ERROR((msg, "ipc_in_request: cannot deliver the message: %d", msg_ret));
		}
		lk_lock(&server_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
		/* server_port_ptr LOCK RW/RW */
		server_port_ptr->portrec_info &= ~PORT_INFO_BLOCKED;
		if (server_port_ptr->portrec_block_queue) {
			/*
			 * Signal port_unblocked to network servers on blocked queue. 
			 */
			ipc_msg_accepted(server_port_ptr);
		}
		lk_unlock(&server_port_ptr->portrec_lock);
		/* server_port_ptr LOCK -/- */
	}

	/*
	 * Deallocate resources used for the incoming message.
	 * We may not touch the ipc_ptr for a RPC, because it
	 * may already have been deallocated by the handler
	 * for the reply.
	 *
	 * This should work, but a reference count on ipc_ptr
	 * would be more generic.
	 */
	if (retval == IPC_SUCCESS) {
		DEBUG1(debug.ipc_in,0,2556,ipc_ptr->reply_port_ptr);
		ipc_in_gc(ipc_ptr);
		if (ipc_ptr->reply_port_ptr) {
	    		lk_lock(&ipc_ptr->reply_port_ptr->portrec_lock,
							PERM_READWRITE,TRUE);
			pr_release(ipc_ptr->reply_port_ptr);
		}
		MEM_DEALLOCOBJ(ipc_ptr, MEM_IPCREC);
	}

	DEBUG1(debug.ipc_in,0,2508,retval);

	RETURN(retval);

END


/*
 * ipc_freeze -- 
 *
 * Parameters: 
 *
 * dest_port_ptr: pointer to the port record for the destination
 * of the messages to be suspended. 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * disables the local port.
 *
 * Note:
 *
 * dest_port_ptr must be locked throughout this procedure.
 *
 */
EXPORT void ipc_freeze(dest_port_ptr)
	port_rec_ptr_t	dest_port_ptr;
BEGIN("ipc_freeze")
	kern_return_t	kern_ret;

	/* dest_port_ptr LOCK RW/RW */

	DEBUG2(debug.ipc_out,0,2520,dest_port_ptr,dest_port_ptr->portrec_info);

	if (!(dest_port_ptr->portrec_info & PORT_INFO_DISABLED)) {
		kern_ret = port_disable(task_self(), dest_port_ptr->portrec_local_port);

		if (kern_ret == KERN_SUCCESS) {
			dest_port_ptr->portrec_info |= PORT_INFO_DISABLED;
		} else {
			ERROR((msg, "ipc_freeze: port_disable returned %d", kern_ret));
		}
	}

	/* dest_port_ptr LOCK RW/RW */

	RET;
END


/*
 * ipc_retry_internal --
 *
 * Private version of ipc_retry(), to re-transmit messages outgoing
 * on a port. Makes no assumptions about new information, but must
 * be called with a non-busy port.
 *
 * Parameters:
 *
 * dest_port_ptr: pointer to the port record for the destination
 * for which messages are to be retried. 
 *
 * Results:
 *
 * meaningless
 *
 * Side effects:
 *	signal the IPC Re-Send thread to retransmit the messages. 
 *
 * Note:
 *	assumes the port record to be locked. 
 *
 */
PRIVATE void ipc_retry_internal(dest_port_ptr)
	port_rec_ptr_t	dest_port_ptr;
BEGIN("ipc_retry_internal")
	ipc_rec_ptr_t	ipc_ptr;

	/* dest_port_ptr LOCK RW/RW */

	LOGCHECK;

	/*
	 * If there are no applicable messages, no need to do anything.
	 */
	if (dest_port_ptr->portrec_waiting_count) {
		/*
		 * Scan the out queue looking for the first waiting message.
		 */
		ipc_ptr = (ipc_rec_ptr_t)
			sys_queue_first(&dest_port_ptr->portrec_out_ipcrec);
		while (!sys_queue_end(&dest_port_ptr->portrec_out_ipcrec,
						(sys_queue_entry_t)ipc_ptr)) {
			if (ipc_ptr->status == IPC_REC_WAITING) {
				break;
			}
			ipc_ptr = (ipc_rec_ptr_t) sys_queue_next(&ipc_ptr->out_q);
		}
		if (ipc_ptr == 0) {
			ERROR((msg," Inconsistent waiting_count for portrec 0x%x",
			       dest_port_ptr));
			panic("ipc_retry_internal");
		}

		DEBUG1(debug.ipc_out,0,2558,ipc_ptr);
		ipc_ptr->status = IPC_REC_READY;
		dest_port_ptr->portrec_waiting_count--;
		dest_port_ptr->portrec_transit_count++;

		/*
		 * Queue the message for a retry.
		 */
		mutex_lock(&ipc_re_send_lock);
		sys_queue_enter(&ipc_re_send_q,ipc_ptr,ipc_rec_ptr_t,re_send_q);
		mutex_unlock(&ipc_re_send_lock);

		/*
		 * Wake-up the IPC re-send thread.
		 */
		condition_signal(&ipc_re_send_signal);
	}
	DEBUG3(debug.ipc_in,0,2581,dest_port_ptr,
					dest_port_ptr->portrec_waiting_count,
					dest_port_ptr->portrec_transit_count);

	/* dest_port_ptr LOCK RW/RW */

	RET;

END


/*
 * ipc_retry --
 *
 * Retry all pending messages on a destination port record,
 * for which there is new information.
 *
 * Parameters:
 *
 * dest_port_ptr: pointer to the port record for the destination
 * for which messages are to be retried. 
 *
 * Results:
 *
 * meaningless
 *
 * Side effects:
 *	signal the IPC Re-Send thread to retransmit the messages. 
 *
 * Note:
 *	assumes the port record to be locked. 
 *
 */
EXPORT int ipc_retry(dest_port_ptr)
	port_rec_ptr_t	dest_port_ptr;
BEGIN("ipc_retry")

	/* dest_port_ptr LOCK RW/RW */

	dest_port_ptr->portrec_info &= ~(PORT_INFO_BLOCKED | PORT_INFO_SUSPENDED);
	dest_port_ptr->portrec_retry_level++;
	FIX_TRANSIT_COUNT(dest_port_ptr);
	dest_port_ptr->portrec_aliveness = PORT_ACTIVE;

	DEBUG4(debug.ipc_out,0,2582,dest_port_ptr,dest_port_ptr->portrec_info,
					dest_port_ptr->portrec_waiting_count,
					dest_port_ptr->portrec_transit_count);

	if (!(PORT_BUSY(dest_port_ptr))) {
		ipc_retry_internal(dest_port_ptr);
	}

	/* dest_port_ptr LOCK RW/RW */

	RETURN(0);
END



/*
 * ipc_re_send -- 
 *
 * Main loop of the IPC re-send thread.
 *
 * Parameters: 
 *
 * none 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * Waits on ipc_re_send_signal, and attempts to transmit all the messages queued
 * on ipc_re_send_q 
 *
 * Note: 
 *
 */
PUBLIC void ipc_re_send()
BEGIN("ipc_re_send")
	ipc_rec_ptr_t	ipc_ptr;
	port_rec_ptr_t	server_port_ptr;
	netaddr_t	destination;
	int		crypt_level;
	int		error_status;
	key_t		key;

#if	LOCK_THREADS
	mutex_lock(thread_lock);
#endif	LOCK_THREADS

	mutex_lock(&ipc_re_send_lock);

	for (;;) {
		/*
		 * Wait for something to do. 
		 */
#if	LOCK_THREADS
		mutex_unlock(thread_lock);
#endif	LOCK_THREADS
		while (sys_queue_empty(&ipc_re_send_q))
			condition_wait(&ipc_re_send_signal, &ipc_re_send_lock);
#if	LOCK_THREADS
		mutex_lock(thread_lock);
#endif	LOCK_THREADS
		DEBUG0(debug.ipc_out,0,2523);
		while (!sys_queue_empty(&ipc_re_send_q)) {
			sys_queue_remove_first(&ipc_re_send_q,ipc_ptr,
						ipc_rec_ptr_t,re_send_q);
			mutex_unlock(&ipc_re_send_lock);

			server_port_ptr = ipc_ptr->server_port_ptr;
			lk_lock(&server_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
			/* server_port_ptr LOCK RW/RW */

			/*
			 * Start by being optimistic. As we check for various problems,
			 * we will decide what error action is appropriate, if any.
			 */
			error_status = 0;
			DEBUG3(debug.ipc_out,0,2559,ipc_ptr,server_port_ptr,
							server_port_ptr->portrec_info);

			/*
			 * XXX Should be able to retry a RPC, but
			 * fold it into a simple IPC for now. 
			 * Making a RPC might seriously complicate the 
			 * flow control/transit_count mechanism.
			 */
			ipc_ptr->type = IPC_REC_TYPE_SINGLE;
			ipc_ptr->out.netmsg_hdr.info &= ~IPC_INFO_RPC;
			ipc_ptr->retry_level = server_port_ptr->portrec_retry_level;
			destination = PORT_REC_RECEIVER(server_port_ptr);
			crypt_level = ipc_ptr->out.crypt_level;

			if (server_port_ptr->portrec_info & PORT_INFO_DEAD) {
				error_status = IPC_ABORT_REQUEST;
			} else {
				/*
				 * The request we are retrying here is counted
				 * in the transit_count, so we must fix it
				 * before using PORT_BUSY().
				 */
				server_port_ptr->portrec_transit_count--;
				if (PORT_BUSY(server_port_ptr)) {
					DEBUG2(debug.ipc_out,0,2526,
						server_port_ptr->portrec_info,
					server_port_ptr->portrec_transit_count);
					error_status = IPC_PORT_BUSY;
				}
				server_port_ptr->portrec_transit_count++;
			}

			lk_unlock(&server_port_ptr->portrec_lock);
			/* server_port_ptr LOCK -/- */

			if (error_status == 0) {
				/*
				 * If the message is to be encrypted, check that
				 * we have a key for the destination.
				 */
#if	USE_CRYPT
				if (((crypt_level) != CRYPT_DONT_ENCRYPT) &&
					(!(km_get_key((destination), &key)))) {
					DEBUG0(debug.ipc_out,0,2524);
					error_status = TR_CRYPT_FAILURE;
				}
#endif	USE_CRYPT
			}

			/*
			 * Transmit the message if appropriate.
			 */
			if (error_status == 0) {
				ipc_out_transmit(NULL,ipc_ptr,destination,crypt_level);
			} else {
				DEBUG1(debug.ipc_out,0,2525,error_status);
				ipc_ptr->trid = 0;
				ipc_ptr->status = IPC_REC_ACTIVE;
				ipc_in_reply(ipc_ptr,error_status,(sbuf_ptr_t)0);
			}
			
			mutex_lock(&ipc_re_send_lock);
		}
	}

	/* NOTREACHED */
END


/*
 * ipc_rpc_init
 *
 * Results:
 *	TRUE or FALSE.
 *
 * Design:
 *
 */
PUBLIC boolean_t ipc_rpc_init()
BEGIN("ipc_rpc_init")
	cthread_t	new_thread;
	struct timeval	tp;

	/*
	 * Initialize the dispatcher for incoming messages.
	 */
	dispatcher_switch[DISPE_IPC_MSG].disp_in_request = ipc_in_request;
	dispatcher_switch[DISPE_IPC_UNBLOCK].disp_indata_simple = ipc_in_unblock;
	dispatcher_switch[DISPE_IPC_ABORT].disp_in_request = ipc_in_abortreq;

	/*
	 * Initialize ipc_seqence_number to be twenty times the number of seconds
	 * since 1 Jan 1987 (approximately).  The assumes that messages are
	 * not sent at a greater rate than an average twenty per second.
	 */
	(void)gettimeofday(&tp, (struct timezone *)0);
	ipc_sequence_number = 20 * (tp.tv_sec - (17 * 365 * 24 * 60 * 60));
	DEBUG1(debug.ipc_out,3,2527,ipc_sequence_number);

	/*
	 * Initialize the re-send queue.
	 */
	mutex_init(&ipc_re_send_lock);
	condition_init(&ipc_re_send_signal);
	sys_queue_init(&ipc_re_send_q);

	/*
	 * Start up threads to execute ipc_out_main and ipc_re_send.
	 */
	new_thread = cthread_fork((cthread_fn_t)ipc_out_main, 0);
	cthread_set_name(new_thread, "ipc_out_main");
	cthread_detach(new_thread);

	new_thread = cthread_fork((cthread_fn_t)ipc_re_send, 0);
	cthread_set_name(new_thread, "ipc_re_send");
	cthread_detach(new_thread);

	RETURN(TRUE);

END



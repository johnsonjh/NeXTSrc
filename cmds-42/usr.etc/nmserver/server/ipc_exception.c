/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * ipc_exception.c 
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/ipc_exception.c,v $ 
 *
 */
#ifndef	lint
char ipc_exception_rcsid[] = "$Header: ipc_exception.c,v 1.1 88/09/30 15:39:19 osdev Exp $";
#endif not lint
/*
 * */

/*
 * HISTORY: 
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 18-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for byte-swapping of ipc sequence number.
 *
 *  6-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Started.
 *
 */


#include	"netmsg.h"
#include	"nm_defs.h"
#include	"ipc_rec.h"
#include	"transport.h"
#include	"mem.h"
#include	"crypt.h"
#include	"network.h"
#include	"ipc_internal.h"
#include	"portrec.h"
#include	"ipc_swap.h"

#if	RPCMOD
#else	RPCMOD
 ! You lose !
#endif	RPCMOD

extern void	ipc_in_abortreply();

/*
 * ipc_server_abort --
 *
 * Abort a pending RPC on the server side.
 *
 * Parameters:
 *
 * port_rec_ptr: record for reply port for the transaction to abort.
 *
 * Results:
 *
 * none.
 *
 * Side effects:
 *
 * Sends an immediate IPC_ABORT_REPLY reply.
 *
 * Design:
 *
 * Note:
 *
 * The port record must be locked throughout the execution of this precedure.
 *
 */
PUBLIC void ipc_server_abort(port_rec_ptr)
	port_rec_ptr_t		port_rec_ptr;
BEGIN("ipc_server_abort")
	ipc_rec_ptr_t		ipc_ptr;

	/* port_rec_ptr LOCK RW/RW */

	if (!awaiting_local_reply(port_rec_ptr)) {
		ERROR((msg,"ipc_server_abort called with no pending transaction"));
		RET;
	}

	/*
	 * Clean up the reply port.
	 */
	ipc_ptr = (ipc_rec_ptr_t)port_rec_ptr->portrec_reply_ipcrec;
	port_rec_ptr->portrec_reply_ipcrec = NULL;

	DEBUG2(debug.ipc_out,0,2562,port_rec_ptr,ipc_ptr);

	/*
	 * Send the reply.
	 */
	(void) transport_sendreply(ipc_ptr->trmod,ipc_ptr->trid,IPC_ABORT_REPLY,0,
								CRYPT_DONT_ENCRYPT);

	/*
	 * Get rid of the ipc_rec and related space.
	 */
	ipc_in_gc(ipc_ptr);
	if (ipc_ptr->reply_port_ptr == port_rec_ptr) {
		pr_release(ipc_ptr->reply_port_ptr);
	} else {
		ERROR((msg,
	"ipc_server_abort: port_rec_ptr=0x%x <> ipc_ptr->reply_port_ptr=0x%x",
					port_rec_ptr,ipc_ptr->reply_port_ptr));
	}
	MEM_DEALLOCOBJ(ipc_ptr, MEM_IPCREC);

	/* port_rec_ptr LOCK RW/RW */

	RET;
END


/*
 * ipc_client_abort --
 *
 * Abort a pending RPC on the client side.
 *
 * Parameters:
 *
 * port_rec_ptr: record for reply port for the transaction to abort.
 *
 * Results:
 *
 * none.
 *
 * Side effects:
 *
 * Sends a special request to cause the server side to send an
 * IPC_ABORT_REPLY reply.
 *
 * Design:
 *
 * Note:
 *
 * The port record must be locked throughout the execution of this precedure.
 *
 * The abort is asynchronous. The worst that can happen is that a new transaction
 * is attempted before the previous one has been aborted, but that in itself
 * would cause the old transaction to abort.
 *
 * For now, we will always send an abort request, and ignore the
 * problems that may occur if that request arrives at the server before
 * the transaction it should abort. At worst, the abort will not take place,
 * and the transaction will stay alive until the next request, or until the
 * ports checkup module kills it. XXX Any better solution for this problem must
 * address the fact that requests and aborts for different transactions may be
 * arbitrarily re-ordered by the network, with endless complications if the
 * reply port migrates at the same time.
 */
PUBLIC void ipc_client_abort(port_rec_ptr)
	port_rec_ptr_t		port_rec_ptr;
BEGIN("ipc_client_abort")
	ipc_rec_ptr_t		ipc_ptr;
	ipc_abort_rec_ptr_t	abort_ptr;
	int			tr_ret;

	/* port_rec_ptr LOCK RW/RW */

	if (!awaiting_remote_reply(port_rec_ptr)) {
		ERROR((msg,"ipc_client_abort called with no pending transaction"));
		RET;
	}

	/*
	 * Clean up the reply port.
	 */
	ipc_ptr = (ipc_rec_ptr_t)port_rec_ptr->portrec_reply_ipcrec;
	port_rec_ptr->portrec_reply_ipcrec = NULL;

	DEBUG2(debug.ipc_out,0,2563,port_rec_ptr,ipc_ptr);

	/*
	 * Prepare and transmit a request to abort the transactiopn.
	 */
	MEM_ALLOCOBJ(abort_ptr,ipc_abort_rec_ptr_t,MEM_IPCABORT);
	SBUF_SEG_INIT(abort_ptr->msg,(sbuf_seg_ptr_t)&abort_ptr->segs);
	SBUF_APPEND(abort_ptr->msg,&abort_ptr->abort_pkt,sizeof(struct abort_pkt));
	abort_ptr->abort_pkt.disp_hdr.disp_type = htons(DISP_IPC_ABORT);
	abort_ptr->abort_pkt.disp_hdr.src_format = conf_own_format;
	abort_ptr->abort_pkt.np_puid = 
			ipc_ptr->reply_port_ptr->portrec_network_port.np_puid;
	abort_ptr->abort_pkt.ipc_seq_no = ipc_ptr->out.netmsg_hdr.ipc_seq_no;

	DEBUG2(debug.ipc_out,0,2564,abort_ptr,ipc_ptr->out.netmsg_hdr.ipc_seq_no);

	tr_ret = transport_sendrequest(ipc_ptr->trmod,abort_ptr,&abort_ptr->msg,
					PORT_REC_RECEIVER(ipc_ptr->server_port_ptr),
					CRYPT_DONT_ENCRYPT,ipc_in_abortreply);

	if (tr_ret != TR_SUCCESS) {
		ipc_in_abortreply(abort_ptr,tr_ret,(sbuf_ptr_t)0);
	}

	/* port_rec_ptr LOCK RW/RW */

	RET;
END


/*
 * ipc_port_dead -- 
 *
 * Informs the IPC module that a local or a network port is dead. 
 *
 * Parameters: 
 *
 * port_rec_ptr: pointer to the port record for the dead port. 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * May abort a pending RPC if the reply port has died. Frees up the list of
 * senders waiting if the port was blocked. 
 *
 * Note: 
 *
 * Assumes that the port record is already locked. It remains locked throughout
 * this procedure.
 *
 */
EXPORT void
ipc_port_dead(port_rec_ptr)
	port_rec_ptr_t	port_rec_ptr;
BEGIN("ipc_port_dead")
	ipc_block_ptr_t	block_ptr, next;

	/* port_rec_ptr LOCK RW/RW */

	DEBUG1(debug.ipc_out, 3, 2521, port_rec_ptr);

	/*
	 * Abort any pending RPC for which this is a reply port.
	 * (at most one of these will be true)
	 */
	if (awaiting_remote_reply(port_rec_ptr)) {
		ipc_client_abort(port_rec_ptr);
	}
	if (awaiting_local_reply(port_rec_ptr)) {
		ipc_server_abort(port_rec_ptr);
	}

#if	NETPORT
	ipc_netport_remove(port_rec_ptr->portrec_network_port);
#endif	NETPORT

	/*
	 * Get rid of the list of blocked machines, if any.
	 */
	block_ptr = (ipc_block_ptr_t) port_rec_ptr->portrec_block_queue;
	while (block_ptr != IPC_BLOCK_NULL) {
		next = block_ptr->next;
		MEM_DEALLOCOBJ(block_ptr, MEM_IPCBLOCK);
		block_ptr = next;
	}

	/*
	 * Make sure that outgoing transactions get cleaned-up.
	 */
	port_rec_ptr->portrec_info |= PORT_INFO_DEAD;
	ipc_retry(port_rec_ptr);

	/* port_rec_ptr LOCK RW/RW */

	RET;

END


/*
 * ipc_port_moved -- 
 *
 * Informs the IPC module that a local or a network port has moved. 
 *
 * Parameters: 
 *
 * port_rec_ptr: pointer to the port record for the moved port. 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * May abort a pending RPC if the reply port has moved.
 *
 * Note: 
 *
 * Assumes that the port record is already locked. It remains locked throughout
 * this procedure.
 *
 */
EXPORT void
ipc_port_moved(port_rec_ptr)
	port_rec_ptr_t	port_rec_ptr;
BEGIN("ipc_port_moved")

	/* port_rec_ptr LOCK RW/RW */

	DEBUG1(debug.ipc_out, 3, 2565, port_rec_ptr);

	/*
	 * Abort any pending RPC for which this is a reply port.
	 * (at most one of these will be true)
	 */
	if (awaiting_remote_reply(port_rec_ptr)) {
		ipc_client_abort(port_rec_ptr);
	}
	if (awaiting_local_reply(port_rec_ptr)) {
		ipc_server_abort(port_rec_ptr);
	}

#if	NETPORT
	ipc_netport_enter(port_rec_ptr->portrec_network_port,
		port_rec_ptr->portrec_local_port,
		NPORT_HAVE_REC_RIGHTS(port_rec_ptr->portrec_network_port));
#endif	NETPORT

	/* port_rec_ptr LOCK RW/RW */

	RET;
END


/*
 * ipc_in_abortreq --
 *
 * Handle an abort request on the server side.
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
 * Always IPC_SUCCESS.
 *
 * Side effects:
 *
 * May call ipc_server_abort if appropriate.
 *
 * Design:
 *
 * Note:
 *
 * We assume that the whole abort packet fits in one sbuf segment.
 *
 * The data in the request can be deallocated by the transport module
 * as soon as this procedure returns.
 *
 */
EXPORT int ipc_in_abortreq(trmod,trid,data_ptr,from,crypt_level,broadcast)
	int			trmod;
	int			trid;
	sbuf_ptr_t		data_ptr;
	netaddr_t		from;
	int			crypt_level;
	boolean_t		broadcast;
BEGIN("ipc_in_abortreq")
	sbuf_seg_ptr_t		sb_ptr;		/* pointer into data sbuf */
	struct abort_pkt	*abort_ptr;
	port_rec_ptr_t		port_rec_ptr;
	unsigned long		ipc_seq_no;
	ipc_rec_ptr_t		ipc_ptr;

#ifdef	lint
	crypt_level++;
	broadcast++;
#endif	lint

	/*
	 * Get a pointer to the abort packet.
	 */
	sb_ptr = data_ptr->segs;
	while (sb_ptr->s == 0)		/* skip past empty segments */
		sb_ptr++;
	abort_ptr = (struct abort_pkt *) sb_ptr->p;
	ipc_seq_no = abort_ptr->ipc_seq_no;

	if(DISP_CHECK_SWAP(abort_ptr->disp_hdr.src_format)) {
		SWAP_DECLS;

		(void) SWAP_LONG(ipc_seq_no, ipc_seq_no);
	}

	DEBUG6(debug.ipc_in,0,2566,trmod,trid,from,
						abort_ptr->np_puid.np_uid_high,
						abort_ptr->np_puid.np_uid_low,
						ipc_seq_no);

	/*
	 * Find the port concerned.
	 */
	port_rec_ptr = (port_rec_ptr_t)pr_np_puid_lookup(abort_ptr->np_puid);
	DEBUG1(debug.ipc_in,0,2567,port_rec_ptr);
	if (port_rec_ptr == PORT_REC_NULL) {
		RETURN(IPC_SUCCESS);
	}
	/* port_rec_ptr LOCK RW/RW */

	/*
	 * Check the pending transaction.
	 */
	ipc_ptr = (ipc_rec_ptr_t)port_rec_ptr->portrec_reply_ipcrec;
	if ((ipc_ptr != NULL) &&
			(ipc_ptr->in.ipc_seq_no == ipc_seq_no) &&
			(ipc_ptr->in.from == from)) {
		ipc_server_abort(port_rec_ptr);
	} else {
		/*
		 * We don't know what's going on. Just ignore the abort
		 * for now.
		 */
	}

	lk_unlock(&port_rec_ptr->portrec_lock);
	/* port_rec_ptr LOCK -/- */

	RETURN(IPC_SUCCESS);

END


/*
 * ipc_in_abortreply --
 *
 * Handle an abort response on the client side.
 *
 * Parameters:
 *
 * abort_ptr: pointer to the abort_rec for the pending transaction.
 * code: success code for the transaction.
 * data_ptr: optional sbuf containing a response (always 0).
 *
 * Results:
 *
 * none.
 *
 * Side effects:
 *
 * Ignore the reply, and deallocate the abort record.
 *
 * Design:
 *
 * Note:
 *
 */
EXPORT void ipc_in_abortreply(abort_ptr,code,data_ptr)
	ipc_abort_rec_ptr_t	abort_ptr;
	int			code;
	sbuf_ptr_t		data_ptr;
BEGIN("ipc_in_abortreply")

	DEBUG3(debug.ipc_in,0,2568,abort_ptr,code,data_ptr);

	MEM_DEALLOCOBJ(abort_ptr, MEM_IPCABORT);

	RET;
END

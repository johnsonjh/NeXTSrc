/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * deltat_handler.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/deltat_handler.c,v $
 *
 */
#ifndef	lint
char deltat_handler_rcsid[] = "$Header: deltat_handler.c,v 1.1 88/09/30 15:38:50 osdev Exp $";
#endif not lint
/*
 * Functions which handle incoming Delta-T packets.
 */

/*
 * HISTORY:
 *  4-Sep-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added USE_DELTAT.
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  1-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for new request-reply transport interface. (RPCMOD)
 *
 * 30-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed deltat_recv_retry - it is now replaced by putting events
 *	on the deltat_recv_queue.  Update events on this queue.
 *	Handle special case of a single inline sbuf segment in a deltat event.
 *	deltat_make_recv_event replaced by deltat_get_recv_event.
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Changed deltat_handle_data_pkt for changed deltat_send_ack.
 *	There is now a statically allocated packet within a deltat event.
 *	Added some statistics gathering.
 *
 * 25-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Lock and timer in deltat event record are now inline.
 *	Replaced printf by ERROR and DEBUG/LOG macros.
 *
 * 20-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Changed for new definition of timer_stop.
 *
 * 20-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include	"netmsg.h"
#include	"nm_defs.h"

#if	USE_DELTAT

#define DELTAT_HANDLER_DEBUG	(debug.deltat & 0x2)

#include "debug.h"
#include "deltat.h"
#include "deltat_defs.h"
#include "disp_hdr.h"
#include "mem.h"
#include "netmsg.h"
#include "network.h"
#include "nm_defs.h"
#include "sbuf.h"
#include "timer.h"
#include "transport.h"



/*
 * deltat_cleanup
 *	Called by a higher-level event after disp_indata has been called.
 *	Also called internally with request-reply interface.
 *
 * Parameters:
 *	event_ptr	: pointer to the event to be cleaned up.
 *
 * Note:
 *	deltat_cleanup can be called synchronously via disp_indata.
 *
 */
PRIVATE deltat_cleanup(event_ptr)
deltat_event_ptr_t	event_ptr;
BEGIN("deltat_cleanup")
    register pointer_t	old_pkt_ptr, next_pkt_ptr;

    mutex_lock(&event_ptr->dte_lock);
    if (event_ptr->dte_status != DELTAT_COMPLETED) {
	ERROR((msg, "deltat_cleanup: incorrect event status (%d).", event_ptr->dte_status));
	mutex_unlock(&event_ptr->dte_lock);
	RETURN(TR_FAILURE);
    }

    /*
     * Deallocate the packets received for this event.
     */
    old_pkt_ptr = event_ptr->dte_in_packets;
    while (old_pkt_ptr != (pointer_t)0) {
	/*
	 * Get the address of the next packet to be deleted and then delete the current one.
	 */
	next_pkt_ptr = *(pointer_t *)old_pkt_ptr;
	MEM_DEALLOCOBJ(old_pkt_ptr, MEM_TRBUFF);
	old_pkt_ptr = next_pkt_ptr;
    }
    event_ptr->dte_in_packets = (pointer_t)0;

    mutex_unlock(&event_ptr->dte_lock);
    RETURN(TR_SUCCESS);

END


/*
 * deltat_handle_abort_pkt
 *	Handle an abort packet received from a remote network server.
 *
 * Parameters:
 *	in_packet_ptr	: pointer to incoming abort packet
 *
 * Results:
 *	A pointer to a packet that can be reused by deltat_main.
 *
 * Side effects:
 *	May call cleanup to inform the client of this abort.
 *
 * Design:
 *	Locate the event.
 *	If it is a send event, then inform the client of the failure.
 *	Mark the event as completed.
 *
 * Note:
 *	deltat_retry or deltat_recv_retry is responsible for destroying the event.
 *	Only call cleanup is the event has not already been aborted.
 *
 */
PUBLIC deltat_pkt_ptr_t deltat_handle_abort_pkt(in_packet_ptr)
register deltat_pkt_ptr_t	in_packet_ptr;
BEGIN("deltat_handle_abort_pkt")
    register deltat_event_ptr_t	event_ptr;
    netaddr_t			host_id;

    host_id = in_packet_ptr->deltat_pkt_header.nih_ip_header.ip_src.s_addr;
    if ((event_ptr = deltat_event_lookup(host_id, in_packet_ptr->deltat_pkt_uid, DELTAT_SEND_HASH_TABLE_ID))
				!= DELTAT_NULL_EVENT)
    {
	mutex_lock(&event_ptr->dte_lock);
	if (event_ptr->dte_status != DELTAT_COMPLETED) {
#if	RPCMOD
	    if (event_ptr->dte_reply) {
		(void)event_ptr->dte_reply(event_ptr->dte_client_id, 
							DELTAT_REMOTE_ABORT,NULL);
#else	RPCMOD
	    if (event_ptr->dte_cleanup) {
		(void)event_ptr->dte_cleanup(event_ptr->dte_client_id, DELTAT_REMOTE_ABORT);
#endif	RPCMOD
	    }
	    event_ptr->dte_status = DELTAT_COMPLETED;
	}
	mutex_unlock(&event_ptr->dte_lock);
    }
    else if ((event_ptr = deltat_event_lookup(host_id, in_packet_ptr->deltat_pkt_uid, DELTAT_RECV_HASH_TABLE_ID))
				!= DELTAT_NULL_EVENT)
    {
	mutex_lock(&event_ptr->dte_lock);
	event_ptr->dte_status = DELTAT_COMPLETED;
	mutex_unlock(&event_ptr->dte_lock);
    }
    else {
	LOG0(TRUE, 5, 1079);
    }

    RETURN(in_packet_ptr);

END



/*
 * deltat_handle_ack_pkt
 *	Handles an incoming ack packet.
 *
 * Parameters:
 *	in_packet_ptr	: the incoming packet.
 *
 * Results:
 *	A pointer to a packet that deltat_main can reuse.
 *
 * Side effects:
 *	May send off the next packet.
 *	Will call cleanup if this was the last packet.
 *
 * Design:
 *	Locate the event.
 *	If the event is finished, call cleanup and mark the event as completed.
 *	Otherwise send the next packet off.
 *
 * Note:
 *	deltat_retry is responsible for destroying the event.
 *
 */
PUBLIC deltat_pkt_ptr_t deltat_handle_ack_pkt(in_packet_ptr)
register deltat_pkt_ptr_t	in_packet_ptr;
BEGIN("deltat_handle_ack_pkt")
    register deltat_event_ptr_t	event_ptr;
    netaddr_t			host_id;

    host_id = in_packet_ptr->deltat_pkt_header.nih_ip_header.ip_src.s_addr;
    DEBUG3(DELTAT_HANDLER_DEBUG, 2, 1097, in_packet_ptr->deltat_pkt_uid.deltat_uid_incarnation,
		in_packet_ptr->deltat_pkt_uid.deltat_uid_sequence_no, in_packet_ptr->deltat_pkt_seq_no);
    if ((event_ptr = deltat_event_lookup(host_id, in_packet_ptr->deltat_pkt_uid, DELTAT_SEND_HASH_TABLE_ID))
				== DELTAT_NULL_EVENT)
    {
	LOG0(TRUE, 3, 1080);
	RETURN(in_packet_ptr);
    }

    mutex_lock(&event_ptr->dte_lock);

    /*
     * See what state the event is in and act accordingly.
     */
    switch (event_ptr->dte_status) {
	case DELTAT_COMPLETED: {
	    LOG0(TRUE, 3, 1081);
	    break;
	}
	case DELTAT_WAITING: {
	    /*
	     * Check to see whether this is the expected packet.
	     */
	    if (in_packet_ptr->deltat_pkt_seq_no == event_ptr->dte_pkt_seq_no) {
#if	RPCMOD
		if (event_ptr->dte_reply) {
		    (void)event_ptr->dte_reply(event_ptr->dte_client_id,
						in_packet_ptr->deltat_pkt_msg_type,NULL);
#else	RPCMOD
		if (event_ptr->dte_cleanup) {
		    (void)event_ptr->dte_cleanup(event_ptr->dte_client_id,
							in_packet_ptr->deltat_pkt_msg_type);
#endif	RPCMOD
		}
		event_ptr->dte_status = DELTAT_COMPLETED;
	    }
	    break;
	}
	case DELTAT_SENDING: {
	    /*
	     * Check to see whether this is the expected packet.
	     */
	    if (in_packet_ptr->deltat_pkt_seq_no == event_ptr->dte_pkt_seq_no) {
		kern_return_t	kr;

		kr = deltat_send_next_packet(event_ptr);
		if (kr != TR_SUCCESS) {
		    ERROR((msg, "deltat_handle_ack_pkt.deltat_send_next_packet fails, kr = %1d.", kr));
		}
	    }
	    break;
	}
	default: {
	    ERROR((msg, "deltat_handle_ack_pkt: illegal event status (%1d).", event_ptr->dte_status));
	    break;
	}
    }

    mutex_unlock(&event_ptr->dte_lock);
    RETURN(in_packet_ptr);

END



/*
 * deltat_handle_crypt_failure_pkt
 *	Deal with a crypt failure packet.
 *
 * Parameters:
 *	in_packet_ptr	: the incoming packet.
 *
 * Results:
 *	A pointer to a packet that can be reused by deltat_main.
 *
 * Design:
 *	Find the event.
 *	Call cleanup and mark the event as completed.
 *
 * Note:
 *	deltat_retry is responsible for destroying the event.
 *	The event uid can be extracted from the incoming packet since
 *	the remote host sent it back encrypted and we were able to decrypt it.
 *
 */
PUBLIC deltat_pkt_ptr_t deltat_handle_crypt_failure_pkt(in_packet_ptr)
register deltat_pkt_ptr_t	in_packet_ptr;
BEGIN("deltat_handle_crypt_failure")
    register deltat_event_ptr_t	event_ptr;
    netaddr_t			host_id;

    host_id = in_packet_ptr->deltat_pkt_header.nih_ip_header.ip_src.s_addr;
    if ((event_ptr = deltat_event_lookup(host_id, in_packet_ptr->deltat_pkt_uid, DELTAT_SEND_HASH_TABLE_ID))
				== DELTAT_NULL_EVENT)
    {
	LOG0(TRUE,5, 1082);
	RETURN(in_packet_ptr);
    }

    mutex_lock(&event_ptr->dte_lock);

    if (event_ptr->dte_status != DELTAT_COMPLETED) {
#if	RPCMOD
	if (event_ptr->dte_reply) {
	    (void)event_ptr->dte_reply(event_ptr->dte_client_id, TR_CRYPT_FAILURE, NULL);
#else	RPCMOD
	if (event_ptr->dte_cleanup) {
	    (void)event_ptr->dte_cleanup(event_ptr->dte_client_id, TR_CRYPT_FAILURE);
#endif	RPCMOD
	}
	event_ptr->dte_status = DELTAT_COMPLETED;
    }

    mutex_unlock(&event_ptr->dte_lock);
    RETURN(in_packet_ptr);

END



/*
 * deltat_handle_data_pkt
 *	handles an incoming data packet
 *
 * Parameters:
 *	in_packet_ptr	: the incoming packet
 *	data_size	: the size of the packet
 *	crypt_level	: the encryption level of the packet
 *
 * Results:
 *	Either null or a pointer to a packet which deltat_main can reuse
 *
 * Side effects:
 *	May create a new event.
 *
 * Design:
 *	Try and find the appropriate recv event.
 *	If one does not exist, create a new one.
 *	If this is the last packet expected for this event,
 *	then call disp_in_request to pass the data to the higher-level module
 *	Acknowledge the packet.
 *
 * Note:
 *	The event is on the deltat_recv_queue and deltat_process_recv_queue
 *	is responsible for eventually culling the event.
 *
 *	This module behaves slightly differently for the non RPCMOD version.
 *
 */
PUBLIC deltat_pkt_ptr_t deltat_handle_data_pkt(in_packet_ptr, data_size, crypt_level)
register deltat_pkt_ptr_t	in_packet_ptr;
int				data_size;
int				crypt_level;
BEGIN("deltat_handle_data_pkt")
    register deltat_event_ptr_t	event_ptr;
    netaddr_t			host_id;
    kern_return_t		kr;
    int				msg_status;
    deltat_uid_t		ack_uid;
    unsigned short		ack_seq_no;

    host_id = in_packet_ptr->deltat_pkt_header.nih_ip_header.ip_src.s_addr;
    DEBUG4(DELTAT_HANDLER_DEBUG, 2, 1098, data_size, in_packet_ptr->deltat_pkt_uid.deltat_uid_incarnation,
		in_packet_ptr->deltat_pkt_uid.deltat_uid_sequence_no, in_packet_ptr->deltat_pkt_seq_no);
    if ((event_ptr = deltat_get_recv_event(host_id, in_packet_ptr->deltat_pkt_uid)) == DELTAT_NULL_EVENT){
	ERROR((msg, "deltat_handle_data_pkt.deltat_get_recv_event fails."));
	RETURN(in_packet_ptr);
    }

    mutex_lock(&event_ptr->dte_lock);

    /*
     * Check to see if this is an old packet.
     */
    if (in_packet_ptr->deltat_pkt_seq_no < event_ptr->dte_pkt_seq_no) {
	/*
	 * We can assert that the sender is not expecting an ack for this packet
 	 * because it has already sent out a packet with a higher sequence number.
	 */
	INCSTAT(deltat_oldpkts_rcvd);
	LOG0(TRUE, 3, 1083);
	mutex_unlock(&event_ptr->dte_lock);
	RETURN(in_packet_ptr);
    }

    /*
     * Check to see if this is a duplicate packet.
     */
    if (event_ptr->dte_pkt_seq_no == in_packet_ptr->deltat_pkt_seq_no) {
	/*
	 * Just send the ack contained in the event.
	 */
	INCSTAT(deltat_retries_rcvd);
	if ((kr = netipc_send((netipc_header_ptr_t)&event_ptr->dte_packet)) != SEND_SUCCESS) {
	    ERROR((msg, "deltat_handle_data_pkt.netipc_send fails, kr = %1d.", kr));
	}
	else INCSTAT(deltat_acks_sent);
	mutex_unlock(&event_ptr->dte_lock);
	RETURN(in_packet_ptr);
    }

    /*
     * Make sure that this is the packet that we are expecting.
     */
    if ((event_ptr->dte_pkt_seq_no + 1) != in_packet_ptr->deltat_pkt_seq_no) {
	INCSTAT(deltat_oospkts_rcvd);
	LOG0(TRUE, 5, 1084);
	mutex_unlock(&event_ptr->dte_lock);
	RETURN(in_packet_ptr);
    }
    event_ptr->dte_pkt_seq_no ++;

    /*
     * Now that we have the correct next packet, store it.
     * If this is the last packet then call disp_in_request.
     * Acknowledge the packet.
     */

    INCSTAT(deltat_dpkts_rcvd);
    ack_uid = in_packet_ptr->deltat_pkt_uid;
    ack_seq_no = in_packet_ptr->deltat_pkt_seq_no;
    if (ack_seq_no == 1) event_ptr->dte_crypt_level = crypt_level;
    else if (ack_seq_no == 2)
    {
	/*
	 * Handle special case of growing a single inline sbuf segment.
	 */
	sbuf_seg_ptr_t	new_segs;
	MEM_ALLOC(new_segs,sbuf_seg_ptr_t,16 * sizeof(struct sbuf_seg), FALSE);
	*new_segs = event_ptr->dte_seg;
	event_ptr->dte_data.end = event_ptr->dte_data.segs = new_segs;
	event_ptr->dte_data.end ++;
	event_ptr->dte_data.size = 16;
	event_ptr->dte_data.free = 15;
    }
    SBUF_APPEND(event_ptr->dte_data, (pointer_t)&in_packet_ptr->deltat_pkt_data[0], data_size);
    *(pointer_t *)in_packet_ptr = event_ptr->dte_in_packets;
    event_ptr->dte_in_packets = (pointer_t)in_packet_ptr;

    if (in_packet_ptr->deltat_pkt_msg_type == DELTAT_MSG_END) {
	netaddr_t	source = in_packet_ptr->deltat_pkt_header.nih_ip_header.ip_src.s_addr;
	boolean_t	broadcast;

	broadcast = (broadcast_address == in_packet_ptr->deltat_pkt_header.nih_ip_header.ip_dst.s_addr);
	event_ptr->dte_status = DELTAT_COMPLETED;
	mutex_unlock(&event_ptr->dte_lock);
	DEBUG_SBUF(DELTAT_HANDLER_DEBUG, 0, event_ptr->dte_data);
#if	RPCMOD
	kr = disp_in_request(TR_DELTAT_ENTRY, (int)event_ptr, 
					(sbuf_ptr_t)&event_ptr->dte_data, source, 
					crypt_level, broadcast);
	if (kr == DISP_WILL_REPLY) {
	    panic("deltat: disp_in_request returned DISP_WILL_REPLY");
	(void)deltat_cleanup(event_ptr);
#else	RPCMOD
	kr = disp_indata((int)event_ptr, (sbuf_ptr_t)&event_ptr->dte_data, source, deltat_cleanup,
				TR_DELTAT_ENTRY, 0, crypt_level, broadcast);
	if (kr == DISP_FAILURE) {
	    /*
	     * Do the cleanup ourself.
	     */
	    (void)deltat_cleanup(event_ptr);
#endif	RPCMOD
	}
	mutex_lock(&event_ptr->dte_lock);
	msg_status = kr;
    }
    else {
	msg_status = DELTAT_MSG_NULL;
	event_ptr->dte_status = DELTAT_RECEIVING;
    }

    deltat_send_ack(event_ptr, ack_uid, ack_seq_no, crypt_level, msg_status);

    /*
     * Reset the active field of the event.
     */
    event_ptr->dte_recv_event.dtre_active = TRUE;
    mutex_unlock(&event_ptr->dte_lock);

    RETURN((deltat_pkt_ptr_t)0);

END


#else	USE_DELTAT
	/*
	 * Just a dummy to keep the loader happy.
	 */
static int	dummy;
#endif	USE_DELTAT


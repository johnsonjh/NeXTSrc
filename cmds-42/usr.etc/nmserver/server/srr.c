/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * srr.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/srr.c,v $
 *
 */

#ifndef	lint
char srr_rcsid[] = "$Header: srr.c,v 1.1 88/09/30 15:42:18 osdev Exp $";
#endif not lint

/*
 * Simple request-response (srr) transport protocol.
 * This protocol is only used within the network server.
 */

/*
 * HISTORY:
 * 05-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Added USE_SRR.
 *
 * 23-Jun-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added a LOGCHECK.
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  7-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed crypt_level parameter from srr_handle_crypt_failure.
 *
 * 15-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Crypt failure is marked by a negative crypt level.
 *	Do not loop on joint local and remote crypt failure.
 *	Removed srr_abort.  
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Increased the backlog on the srr_listen_port.
 *	Added some statistics gathering.
 *
 * 22-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Host info record cannot be locked when cleanup is called.
 *	Lock and timer in host info record are now inline.
 *	Replaced fprintf by ERROR and DEBUG/LOG macros.
 *	Use srr_max_tries from param record.
 *	Conditionally use thread_lock - ensures only one thread is executing.
 *	Added call to cthread_set_name.
 *
 *  1-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Changed srr_send and srr_main to distinguish between broadcast
 *	and directed packets.
 *
 * 17-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added srr_max_data_size; initialised from SRR_MAX_DATA_SIZE.
 *	Used mem_allocobj wherever possible.
 *
 *  5-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include "netmsg.h"
#include "nm_defs.h"

#if	USE_SRR

#include <mach.h>
#include <sys/message.h>
#include <cthreads.h>

#include "crypt.h"
#include "debug.h"
#include "ls_defs.h"
#include "mem.h"
#include "netipc.h"
#include "network.h"
#include "nm_extra.h"
#include "sbuf.h"
#include "srr.h"
#include "srr_defs.h"
#include "timer.h"
#include "transport.h"
#include "uid.h"


static port_t		srr_listen_port;
static cthread_t	srr_listen_thread;

netipc_header_t		srr_template;

extern srr_main();

int			srr_max_data_size = SRR_MAX_DATA_SIZE;

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_SRRREQ;


/*
 * srr_init
 *	Initialises the srr transport protocol.
 *
 * Parameters:
 *
 * Results:
 *	FALSE : we failed to initialise the srr transport protocol.
 *	TRUE  : we were successful.
 *
 * Side effects:
 *	Initialises the srr protocol entry point in the switch array.
 *	Initialises the template for sending network messages.
 *	Allocates the listener port and creates a thread to listen to the network.
 *
 */
EXPORT boolean_t srr_init()
BEGIN("srr_init")
    kern_return_t	kr;

    /*
     * Initialize the memory management facilities.
     */
    mem_initobj(&MEM_SRRREQ,"SRR request",sizeof(srr_request_q_t),
								FALSE,200,10);


    srr_utils_init();

    transport_switch[TR_SRR_ENTRY].send = srr_send;

    srr_template.nih_msg_header.msg_simple = TRUE;
    srr_template.nih_msg_header.msg_type = MSG_TYPE_NORMAL;
    srr_template.nih_msg_header.msg_size = sizeof(netipc_header_t) + SRR_HEADER_SIZE;
    srr_template.nih_msg_header.msg_id = NETIPC_MSG_ID;
    srr_template.nih_msg_header.msg_local_port = PORT_NULL;
    srr_template.nih_msg_header.msg_remote_port = task_self();

    srr_template.nih_ip_header.ip_hl = sizeof(struct ip) >> 2; /* 32 bit words */
    srr_template.nih_ip_header.ip_v = IPVERSION;
    srr_template.nih_ip_header.ip_tos = 0;
    srr_template.nih_ip_header.ip_len = NETIPC_PACKET_HEADER_SIZE + SRR_HEADER_SIZE;
    srr_template.nih_ip_header.ip_id = 0;
    srr_template.nih_ip_header.ip_off = 0;
    srr_template.nih_ip_header.ip_ttl = 30;	/*UDP_TTL*/
    srr_template.nih_ip_header.ip_p = IPPROTO_UDP;
    srr_template.nih_ip_header.ip_sum = 0;
    srr_template.nih_ip_header.ip_src.s_addr = my_host_id;

    srr_template.nih_udp_header.uh_sport = htons(SRR_UDP_PORT);
    srr_template.nih_udp_header.uh_dport = htons(SRR_UDP_PORT);
    srr_template.nih_udp_header.uh_sum = 0;

    srr_template.nih_crypt_header.ch_crypt_level = CRYPT_DONT_ENCRYPT;
    srr_template.nih_crypt_header.ch_checksum = 0;

    /*
     * Initialise the IPC interface to the network.
     */
    if ((kr = port_allocate(task_self(), &srr_listen_port)) != KERN_SUCCESS) {
	ERROR((msg, "srr_init.port_allocate fails, kr = %d.", kr));
	RETURN(FALSE);
    }
    if ((kr = port_set_backlog(task_self(), srr_listen_port, 16)) != KERN_SUCCESS) {
	ERROR((msg, "deltat_init.port_set_backlog fails, kr = %d.", kr));
	RETURN(FALSE);
    }
    if ((kr = netipc_listen(task_self(), 0, 0, 0, (int)(htons(SRR_UDP_PORT)),
				IPPROTO_UDP, srr_listen_port)) != KERN_SUCCESS)
    {
	ERROR((msg, "srr_init.netipc_listen fails, kr = %d.", kr));
	RETURN(FALSE);
    }

    /*
     * Now fork a thread to execute the receive loop of the srr transport protocol.
     */
    srr_listen_thread = cthread_fork((cthread_fn_t)srr_main, (any_t)0);
    cthread_set_name(srr_listen_thread, "srr_main");
    cthread_detach(srr_listen_thread);

    RETURN(TRUE);

END


/*
 * srr_main
 *	The main reception loop for the simple request-response protocol.
 *	Allocates a reception buffer and waits for incoming messages.
 *	Calls appropriate handling function.
 *
 * Parameters:
 *	None
 *
 * Results:
 *	Should never return
 *
 * Note:
 *	A new buffer must be allocated
 *	if the handling routine does not return one that can be reused.
 *
 */
PRIVATE srr_main()
BEGIN("srr_main")
    kern_return_t	kr;
    srr_packet_ptr_t	in_packet_ptr;
    srr_packet_ptr_t	old_packet_ptr;
    long		srr_packet_type;
    int			crypt_level;
    int			data_size;
    boolean_t		crypt_remote_failure;

#if	LOCK_THREADS
    mutex_lock(thread_lock);
#endif	LOCK_THREADS

    MEM_ALLOCOBJ(in_packet_ptr,srr_packet_ptr_t,MEM_TRBUFF);

    while (TRUE) {
	/*
	 * Fill in the message header and try to receive a message.
	 */
	in_packet_ptr->srr_pkt_header.nih_msg_header.msg_size = NETIPC_MAX_MSG_SIZE;
	in_packet_ptr->srr_pkt_header.nih_msg_header.msg_local_port = srr_listen_port;
	kr = netipc_receive((netipc_header_ptr_t)in_packet_ptr);
	if (kr != RCV_SUCCESS) {
	    ERROR((msg, "srr_main.netipc_receive fails, kr = %d.", kr));
	}
	else {
	    crypt_level = ntohl(in_packet_ptr->srr_pkt_header.nih_crypt_header.ch_crypt_level);
	    if (crypt_level < 0) {
		/*
		 * This is a remote crypt failure packet.
		 */
		crypt_remote_failure = TRUE;
		crypt_level = - crypt_level;
		in_packet_ptr->srr_pkt_header.nih_crypt_header.ch_crypt_level =
				htonl((unsigned long)crypt_level);
	    }
	    else crypt_remote_failure = FALSE;

	    if (crypt_level != CRYPT_DONT_ENCRYPT) {
		kr = crypt_decrypt_packet((netipc_ptr_t)in_packet_ptr, crypt_level);
	    }
	    else kr = CRYPT_SUCCESS;

	    if (kr == CRYPT_SUCCESS) {
		/*
		 * Swap the srr header and see what type of packet we got.
		 */
		NTOH_SRR_UID(in_packet_ptr->srr_pkt_uid);
		if (crypt_remote_failure) {
		    /*
		     * This packet was sent by us but could not be decrypted by the remote network server.
		     */
		    LOG0(TRUE, 5, 1055);
		    INCSTAT(srr_cfailures_rcvd);
		    old_packet_ptr = srr_handle_crypt_failure(in_packet_ptr);
		}
		else {
		    srr_packet_type = ntohl(in_packet_ptr->srr_pkt_type);
		    old_packet_ptr = SRR_NULL_PACKET;
		    data_size = ntohs(in_packet_ptr->srr_pkt_header.nih_crypt_header.ch_data_size);
		    data_size -= SRR_HEADER_SIZE;
		    switch ((int)srr_packet_type) {
			case SRR_REQUEST: {
			    INCSTAT(srr_requests_rcvd);
			    old_packet_ptr = srr_handle_request(in_packet_ptr, data_size, crypt_level, 0);
			    break;
			}
			case SRR_RESPONSE: {
			    INCSTAT(srr_replies_rcvd);
			    old_packet_ptr = srr_handle_response(in_packet_ptr, data_size, crypt_level, 0);
			    break;
			}
			case SRR_BCAST_REQUEST: {
			    INCSTAT(srr_bcasts_rcvd);
			    INCSTAT(srr_requests_rcvd);
			    old_packet_ptr = srr_handle_request(in_packet_ptr, data_size, crypt_level, 1);
			    break;
			}
			case SRR_BCAST_RESPONSE: {
			   INCSTAT(srr_replies_rcvd);
			   old_packet_ptr = srr_handle_response(in_packet_ptr, data_size, crypt_level, 1);
			   break;
			}
			default: {
			    ERROR((msg, "srr_main unknown packet type %d.", srr_packet_type));
			    break;
			}
		    }
		}
	    }
	    else {
		if (!crypt_remote_failure) srr_send_crypt_failure(in_packet_ptr, crypt_level);
		old_packet_ptr = in_packet_ptr;
	    }

	    if (old_packet_ptr == SRR_NULL_PACKET) {
		/*
		 *Allocate a new buffer.
		 */
		MEM_ALLOCOBJ(in_packet_ptr,srr_packet_ptr_t,MEM_TRBUFF);
	    }
	    else in_packet_ptr = old_packet_ptr;
	}
	LOGCHECK;
    }
END



/*
 * srr_retry
 *	Called by the timer service to retransmit a request packet.
 *
 * Parameters:
 *	timer_record	: the information previously supplied to the timer service
 *
 * Results:
 *	SRR_SUCCESS of SRR_FAILURE
 *
 * Side effects:
 *	May decide that this request should be aborted
 *	in which case cleanup is called to inform the client.
 *	May send a request packet over the network and schedule another retransmission.
 *
 * Note:
 *	If the request packet is empty, then this request has already been aborted.
 *	Host info record should not be locked whilst srq_cleanup is called.
 *
 */
PUBLIC int srr_retry(timer_record)
timer_t timer_record;
BEGIN("srr_retry")
    srr_host_info_ptr_t	host_info;
    kern_return_t	kr;
    srr_packet_ptr_t	request_packet_ptr;

    /*
     * Sanity check.
     */
    host_info = srr_hash_lookup(((srr_host_info_ptr_t)(timer_record->info))->shi_host_id);
    if (host_info != (srr_host_info_ptr_t)(timer_record->info)) {
	panic("srr_retry.srr_hash_lookup");
    }

    /*
     * Lock the host information record before attempting to update it.
     */
    mutex_lock(&host_info->shi_lock);

    /*
     * Check that the request actually needs to be retransmitted.
     */
    if (host_info->shi_request_status == SRR_HAVE_RESPONSE) {
	mutex_unlock(&host_info->shi_lock);
	RETURN(SRR_FAILURE);
    }

    /*
     * Check to see if this request has exceeded the maximum number of tries
     * and that it has not been aborted.
     */
    if ((host_info->shi_request_tries < param.srr_max_tries)
	&& (host_info->shi_request_q_head->srq_request_packet != SRR_NULL_PACKET))
    {
	/*
	 * Resend this request and requeue it with the timer service.
	 */
	if (host_info->shi_request_status == SRR_AWAITING_RESPONSE) {
	    request_packet_ptr = host_info->shi_request_q_head->srq_request_packet;
	    if ((kr = netipc_send((netipc_header_ptr_t)request_packet_ptr)) != SEND_SUCCESS) {
		ERROR((msg, "srr_retry.netipc_send fails, kr = %d.", kr));
	    }
	    else INCSTAT(srr_retries_sent);
	}
	host_info->shi_request_tries ++;

	/*
	 * Also queue this request up for retransmission.
	 */
	timer_start(&host_info->shi_timer);
    }
    else {
	srr_request_q_ptr_t	old_request;

	/*
	 * Dequeue the request and call cleanup to inform the client of the failure.
	 */
	if ((old_request = srr_dequeue(host_info)) == SRR_NULL_Q) {
	    LOG0(TRUE, 4, 1053);
	    mutex_unlock(&host_info->shi_lock);
	    RETURN(SRR_FAILURE);
	}
#if	NeXT
	/*
	 * Must mark inactive before unlocking
	 */
	host_info->shi_request_status = SRR_INACTIVE;
#endif	NeXT

	switch (host_info->shi_request_status) {
	    case SRR_LOCAL_CRYPT_FAILURE: case SRR_REMOTE_CRYPT_FAILURE: {
		if (old_request->srq_cleanup) {
		    mutex_unlock(&host_info->shi_lock);
		    old_request->srq_cleanup(old_request->srq_client_id, TR_CRYPT_FAILURE);
		    mutex_lock(&host_info->shi_lock);
		}
		break;
	    }
	    default: {
		/*
		 * Only call cleanup if the request packet is not null.
		 */
		if (old_request->srq_request_packet != SRR_NULL_PACKET) {
		    if (old_request->srq_cleanup) {
			mutex_unlock(&host_info->shi_lock);
			old_request->srq_cleanup(old_request->srq_client_id, TR_FAILURE);
			mutex_lock(&host_info->shi_lock);
		    }
		    MEM_DEALLOCOBJ(old_request->srq_request_packet, MEM_TRBUFF);
		}
		break;
	    }
	}

	MEM_DEALLOCOBJ(old_request, MEM_SRRREQ);
#if	NeXT
#else	NeXT
	host_info->shi_request_status = SRR_INACTIVE;
#endif	NeXT

	/*
	 * If there is another request waiting transmission
	 * send it off and queue it with the timer service.
	 */
	if (host_info->shi_request_q_head != SRR_NULL_Q) {
	    srr_process_queued_request(host_info);
	}
    }

    mutex_unlock(&host_info->shi_lock);
    RETURN(SRR_SUCCESS);
END



/*
 * srr_send
 *	Either sends a request packet out or queues it up for sending later.
 *
 * Parameters:
 *	client_id	: an identifier assigned by the client to this transaction
 *	data		: the data to be sent
 *	to		: the destination of the request
 *	service		: ignored 
 *	crypt_level	: whether the data should be encrypted
 *	cleanup		: a function to be called when this transaction has finished
 *
 * Returns:
 *	TR_SUCCESS or a specific failure code.
 *
 * Side effects:
 *	May send a packet out over the network and with the timer module.
 *	Will queue a packet on the information record associated with the destination host.
 *	May create a new host record if there is no prior information for this host.
 *
 * Design:
 *	Construct a packet and a request record.
 *	Maybe create a record for the destination host.
 *	Lock the record for the destination host and enqueue this request.
 *	If there are no other requests waiting to go out
 *		then send the request off and queue it up for retransmission
 *
 * Note:
 *	We only call cleanup if the request-response interaction failed.
 *	Otherwise the response provided directly to the client is the
 *	indication to the client that it can now cleanup its records.
 *
 */
/*ARGSUSED*/
EXPORT int srr_send(client_id,data,to,service,crypt_level,cleanup)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	to;
int		service;
int		crypt_level;
int		(*cleanup)();
BEGIN("srr_send")
    srr_packet_ptr_t		request_packet_ptr;
    int				size, packet_type;
    srr_host_info_ptr_t		host_info;
    srr_request_q_ptr_t 	q_record;

    /*
     * Allocate a buffer to hold the request packet.
     */
    MEM_ALLOCOBJ(request_packet_ptr,srr_packet_ptr_t,MEM_TRBUFF);

    /*
     * Copy the input sbuf into our local buffer.
     */
    SBUF_FLATTEN(data, request_packet_ptr->srr_pkt_data, size);

    /*
     * Sanity check.
     */
    if (size > SRR_MAX_DATA_SIZE) {
	ERROR((msg, "srr_send fails, size of data (%d) is too large.", size));
	MEM_DEALLOCOBJ(request_packet_ptr, MEM_TRBUFF);
	RETURN(SRR_TOO_LARGE);
    }

    /*
     * Fill in the netipc header.
     */
    packet_type = (to == broadcast_address) ? SRR_BCAST_REQUEST : SRR_REQUEST;
    SRR_SET_PKT_HEADER(request_packet_ptr, size, to, packet_type, crypt_level);

    /*
     * Find the host record for the destination host.
     */
    host_info = srr_hash_lookup(to);
    if (host_info == SRR_NULL_HOST_INFO) host_info = srr_hash_enter(to);
    if (host_info == SRR_NULL_HOST_INFO) {
	ERROR((msg, "srr_send.srr_hash_enter fails."));
	RETURN(SRR_FAILURE);
    }

    /*
     * Create a queue record for this new request.
     */
    MEM_ALLOCOBJ(q_record,srr_request_q_ptr_t,MEM_SRRREQ);
    q_record->srq_request_packet = request_packet_ptr;
    q_record->srq_destination = to;
    q_record->srq_client_id = client_id;
    q_record->srq_cleanup = cleanup;

    /*
     * Lock the host information record before attempting to update it.
     */
    mutex_lock(&host_info->shi_lock);

    /*
     * Queue the new request.
     * It it is queued at the head of the queue
     * then try to send this request off right now.
     */
    srr_enqueue(q_record, host_info);
    if (host_info->shi_request_q_head == q_record) {
	srr_process_queued_request(host_info);
    }

    mutex_unlock(&host_info->shi_lock);
    RETURN(TR_SUCCESS);
END

#endif	USE_SRR

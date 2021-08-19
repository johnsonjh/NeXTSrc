/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * po_message.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/po_message.c,v $
 *
 */

#ifndef	lint
char po_message_rcsid[] = "$Header: po_message.c,v 1.1 88/09/30 15:41:15 osdev Exp $";
#endif not lint

/*
 * Functions implementing the sending and handling of messages
 * which are a result of explicit port transfers.
 */

/*
 * HISTORY:
 * 09-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Conditionalize encrypt code.
 *
 *  2-Sep-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added RPCMOD.
 *
 * 30-Sep-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Return DISP_IGNORE instead of DISP_FAILURE in po_handle_token_request.
 *	Send a token request directly to a network port's receiver.
 *	Provide a clean-up function for the token request.
 *	km_token_cleanup calls km_do_key_exchange if there was a crypt failure.
 *
 * 21-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added port statistics.
 *
 *  6-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Bug fix to secure case in po_handle_ro_xfer_hint.
 *
 * 27-Jul-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added NETPORT.
 *
 *  5-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Transfer RO Keys in network byte order if multperm encryption used.
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added some statistics gathering.
 *
 * 29-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Replaced fprintf by ERROR and DEBUG/LOG macros.
 *	Lock is now inline in port record.
 *
 *  3-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Split off from po_utils.c.  Replaced SBUF_INIT by SBUF_SEG_INIT.
 *
 */

#include <mach_types.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "config.h"
#include "crypt.h"
#include "debug.h"
#include "disp_hdr.h"
#include "key_defs.h"
#include "ls_defs.h"
#include "netmsg.h"
#include "network.h"
#include "nm_defs.h"
#include "po_defs.h"
#include "port_defs.h"
#include "portops.h"
#include "portrec.h"
#include "portsearch.h"
#include "rwlock.h"
#include "sbuf.h"
#include "transport.h"
#include "ipc.h"



/*
 * po_handle_token_reply
 *	Handles a reply to a token request.
 *
 * Parameters:
 *	client_id	: ignored.
 *	reply		: pointer to the reply sbuf
 *	from		: host from which the reply was received - ignored.
 *	broadcast	: ignored
 *	crypt_level	: ignored
 *
 * Returns:
 *	DISP_SUCCESS.
 *
 * Design:
 *	Store the received token.
 *
 */
/* ARGSUSED */
PUBLIC po_handle_token_reply(client_id, reply, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	reply;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("po_handle_token_reply")
    po_message_ptr_t	message_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(po_token_replies_rcvd);
    SBUF_GET_SEG(*reply, message_ptr, po_message_ptr_t);

    if ((port_rec_ptr = pr_nportlookup(&(message_ptr->pom_po_data.pod_nport))) == PORT_REC_NULL) {
	LOG0(TRUE, 3, 1191);
	LOG_NPORT(TRUE, 3, message_ptr->pom_po_data.pod_nport);
	RETURN(DISP_SUCCESS);
    }
    /* port_rec_ptr LOCK RW/RW */
    if (!(NPORT_HAVE_SEND_RIGHTS(port_rec_ptr->portrec_network_port))) {
	LOG0(TRUE, 3, 1192);
	LOG_NPORT(TRUE, 3, port_rec_ptr->portrec_network_port);
	lk_unlock(&port_rec_ptr->portrec_lock);
	RETURN(DISP_SUCCESS);
    }

    DEBUG0(PO_DEBUG, 1, 1190);
    DEBUG_KEY(PO_DEBUG, 1, message_ptr->pom_po_data.pod_sinfo.si_key);
    DEBUG_NPORT(PO_DEBUG, 1, port_rec_ptr->portrec_network_port);
    /*
     * Remember the new token.
     */
    port_rec_ptr->portrec_secure_info = message_ptr->pom_po_data.pod_sinfo;
    port_rec_ptr->portrec_random = message_ptr->pom_po_data.pod_extra;

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

END



/*
 * po_handle_token_request
 *	handles a request for a token.
 *
 * Parameters:
 *	request		: the incoming token request
 *	from		: the sender of the request (ignored)
 *	broadcast	: ignored
 *	crypt_level	: ignored
 *
 * Results:
 *	DISP_SUCCESS or DISP_FAILURE.
 *
 * Design:
 *	Looks up the port.
 *	Creates a new token and places it in the request.
 *
 * Note:
 *	Assume that the transport module is doing the encryption.
 *
 */
/* ARGSUSED */
PUBLIC po_handle_token_request(request, from, broadcast, crypt_level)
sbuf_ptr_t	request;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("po_handle_token_request")
    po_message_ptr_t	message_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(po_token_requests_rcvd);
    SBUF_GET_SEG(*request, message_ptr, po_message_ptr_t);

    if ((port_rec_ptr = pr_nportlookup(&(message_ptr->pom_po_data.pod_nport))) == PORT_REC_NULL) {
	LOG0(TRUE, 3, 1193);
	LOG_NPORT(TRUE, 3, message_ptr->pom_po_data.pod_nport);
	RETURN(DISP_IGNORE);
    }
    /* port_rec_ptr LOCK RW/RW */
    if (!(NPORT_HAVE_RO_RIGHTS(port_rec_ptr->portrec_network_port))) {
	LOG0(TRUE, 3, 1194);
	LOG_NPORT(TRUE, 3, port_rec_ptr->portrec_network_port);
	lk_unlock(&port_rec_ptr->portrec_lock);
	RETURN(DISP_IGNORE);
    }

    /*
     * Create the reply.
     */
    message_ptr->pom_po_data.pod_extra = po_create_token(port_rec_ptr,
						&(message_ptr->pom_po_data.pod_sinfo));
    message_ptr->pom_disp_hdr.src_format = conf_own_format;

    DEBUG0(PO_DEBUG, 1, 1195);
    DEBUG_KEY(PO_DEBUG, 1, message_ptr->pom_po_data.pod_sinfo.si_key);
    DEBUG_NPORT(PO_DEBUG, 1, port_rec_ptr->portrec_network_port);
    INCPORTSTAT(port_rec_ptr, tokens_sent);

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

END



/*
 * po_token_km_retry
 *	called if a key exchange for a token request completes.
 *
 * Parameters:
 *	port_rec_ptr	: the port for which the token was requested.
 *
 * Results:
 *	ignored
 *
 * Design:
 *	Just call po_token_request.
 *
 */
PUBLIC po_token_km_retry(port_rec_ptr)
port_rec_ptr_t	port_rec_ptr;
BEGIN("po_token_km_retry")

    lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
    po_request_token(port_rec_ptr, CRYPT_ENCRYPT);
    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(0);

END


/*
 * po_token_cleanup
 *	called if a token request failed.
 *
 * Parameters:
 *	port_rec_ptr	: the port for which the token was requested.
 *	reason		: the reason for the failure
 *
 * Results:
 *	ignored
 *
 * Design:
 *	If the reason is TR_CRYPT_FAILURE then call km_do_key_exchange.
 *
 */
PUBLIC po_token_cleanup(port_rec_ptr, reason)
port_rec_ptr_t	port_rec_ptr;
int		reason;
BEGIN("po_token_cleanup")
    netaddr_t	destination;

    if (reason == TR_CRYPT_FAILURE) {
	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
	destination = PORT_REC_RECEIVER(port_rec_ptr);
	lk_unlock(&port_rec_ptr->portrec_lock);
	LOG1(TRUE, 5, 1202, destination);
	km_do_key_exchange(port_rec_ptr, po_token_km_retry, destination);
    }

    RETURN(0);
END




/*
 * po_request_token
 *	requests a token of authenticity of a receiver or owner
 *
 * Parameters:
 *	port_rec_ptr	: the record of the port that needs the token
 *	security_level	: the security level that should be used
 *
 * Design:
 *	Submits a token request using the SRR transport protocol.
 *
 */
PUBLIC void po_request_token(port_rec_ptr, security_level)
port_rec_ptr_t	port_rec_ptr;
int		security_level;
BEGIN("po_request_token")
    sbuf_t		sbuf;
    sbuf_seg_t		sbuf_seg;
    po_message_t	message;
    int			tr;
    netaddr_t		destination;

    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &message, sizeof(po_message_t));
    message.pom_disp_hdr.disp_type = htons(DISP_PO_TOKEN);
    message.pom_disp_hdr.src_format = conf_own_format;
    message.pom_po_data.pod_nport = port_rec_ptr->portrec_network_port;
    destination = PORT_REC_RECEIVER(port_rec_ptr);

    DEBUG0(PO_DEBUG, 1, 1196);
    DEBUG_NPORT(PO_DEBUG, 1, port_rec_ptr->portrec_network_port);

    tr = transport_switch[TR_SRR_ENTRY].send(port_rec_ptr, &sbuf, destination,
				TRSERV_NORMAL, security_level, po_token_cleanup);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "po_request_token.send fails, tr = %d.", tr));
    }
    else INCSTAT(po_token_requests_sent);

    RET;

END



/*
 * po_handle_ro_xfer_hint
 *	Handles an unreliable notification of a transfer of receive/ownership rights.
 *
 * Parameters:
 *	client_id	: ignored.
 *	data		: data received over the network.
 *	from		: host from which the data was received (ignored).
 *	broadcast	: ignored.
 *	crypt_level	: the security level of the incoming data.
 *
 * Returns:
 *	DISP_SUCCESS
 *
 * Side effects:
 *	May initiate a port search.
 *
 * Design:
 *	If we have no rights to the port, then ignore it.
 *	If we have both rights, then something is wrong.
 *	If we are the owner, then take note of the new receiver.
 *	If we are the receiver, then take note of the new owner.
 *	If we just have send rights, then trigger a port search.
 *
 * Note:
 *	Maybe we should not trigger a port search.
 *
 */
/* ARGSUSED */
PUBLIC po_handle_ro_xfer_hint(client_id, data, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("po_handle_ro_xfer_hint")
    po_message_ptr_t	message_ptr;
    network_port_t	nport;
    port_rec_ptr_t	port_rec_ptr;
    boolean_t		hint_ok;

    INCSTAT(po_ro_hints_rcvd);
    SBUF_GET_SEG(*data, message_ptr, po_message_ptr_t);
    nport = message_ptr->pom_po_data.pod_nport;
    if ((port_rec_ptr = pr_nportlookup(&nport)) == PORT_REC_NULL) {
	RETURN(DISP_SUCCESS);
    }
    /* port_rec_ptr LOCK RW/RW */
    INCPORTSTAT(port_rec_ptr, xfer_hints_rcvd);

    DEBUG0(PO_DEBUG, 1, 1197);
    DEBUG_NPORT(PO_DEBUG, 1, port_rec_ptr->portrec_network_port);

    if (NPORT_HAVE_ALL_RIGHTS(port_rec_ptr->portrec_network_port)) {
	/*
	 * We are the receiver and the owner.
	 */
	LOG0(TRUE, 3, 1198);
	LOG_NPORT(TRUE, 3, port_rec_ptr->portrec_network_port);
    }
    else if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
	/*
	 * We are the receiver.
	 */
#if	USE_ENCRYPT
	if (crypt_level != CRYPT_DONT_ENCRYPT) {
	    /*
	     * Check the Receiver/Owner key.
	     */
	    if (param.crypt_algorithm == CRYPT_MULTPERM)
		NTOH_KEY(message_ptr->pom_po_data.pod_sinfo.si_key);
	    if (!KEY_EQUAL(message_ptr->pom_po_data.pod_sinfo.si_key,
				port_rec_ptr->portrec_secure_info.si_key))
	    {
		LOG0(TRUE, 3, 1199);
		LOG_KEY(TRUE, 3, message_ptr->pom_po_data.pod_sinfo.si_key);
		LOG_KEY(TRUE, 3, port_rec_ptr->portrec_secure_info.si_key);
		LOG_NPORT(TRUE, 3, port_rec_ptr->portrec_network_port);
		hint_ok = FALSE;
	    }
	    else hint_ok = TRUE;
	}
	else hint_ok = TRUE;
#else	USE_ENCRYPT
	hint_ok = TRUE;
#endif	USE_ENCRYPT
	if (hint_ok) {
	    port_rec_ptr->portrec_network_port.np_owner = message_ptr->pom_po_data.pod_nport.np_owner;
#if	NETPORT | RPCMOD
	    ipc_port_moved(port_rec_ptr);
#endif	NETPORT | RPCMOD
	}
    }
    else if (port_rec_ptr->portrec_network_port.np_owner == my_host_id) {
	/*
	 * We are the owner.
	 */
#if	USE_ENCRYPT
	if (crypt_level != CRYPT_DONT_ENCRYPT) {
	    /*
	     * Check the Receiver/Owner key.
	     */
	    if (param.crypt_algorithm == CRYPT_MULTPERM)
		NTOH_KEY(message_ptr->pom_po_data.pod_sinfo.si_key);
	    if (!KEY_EQUAL(message_ptr->pom_po_data.pod_sinfo.si_key,
				port_rec_ptr->portrec_secure_info.si_key))
	    {
		LOG0(TRUE, 3, 1200);
		LOG_KEY(TRUE, 3, message_ptr->pom_po_data.pod_sinfo.si_key);
		LOG_KEY(TRUE, 3, port_rec_ptr->portrec_secure_info.si_key);
		LOG_NPORT(TRUE, 3, port_rec_ptr->portrec_network_port);
		hint_ok = FALSE;
	    }
	    else hint_ok = TRUE;
	}
	else hint_ok = TRUE;
#else	USE_ENCRYPT
	hint_ok = TRUE;
#endif	USE_ENCRYPT
	if (hint_ok) {
	    port_rec_ptr->portrec_network_port.np_receiver = message_ptr->pom_po_data.pod_nport.np_receiver;
#if	NETPORT | RPCMOD
	    ipc_port_moved(port_rec_ptr);
#endif	NETPORT | RPCMOD
	}
    }
    else {
	/*
	 * We must have send rights to this port - start a port search.
	 */
	/* XXX See research notes about how we should use the new information. XXX */
	ps_do_port_search(port_rec_ptr, TRUE, &nport, (int(*)())0);
    }

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

END



/*
 * po_send_ro_xfer_hint
 *	Send an unreliable notification of a transfer of receive or ownership rights.
 *
 * Parameters:
 *	port_rec_ptr	: pointer to the record for the port in question.
 *	destination	: network server to be sent the message.
 *	security_level	: the security level at which this notification should be sent.
 *
 * Design:
 *	Check that the destination is either the receiver or the owner.
 *	Construct a packet containing the data and send it using the datagram transport protocol.
 *
 */
PUBLIC void po_send_ro_xfer_hint(port_rec_ptr, destination, security_level)
port_rec_ptr_t		port_rec_ptr;
netaddr_t		destination;
int			security_level;
BEGIN("po_send_ro_xfer_hint")
    sbuf_t		sbuf;
    sbuf_seg_t		sbuf_seg;
    po_message_t	message;
    int			tr;

    if ((destination != port_rec_ptr->portrec_network_port.np_receiver)
	&& (destination != port_rec_ptr->portrec_network_port.np_owner))
    {
	LOG0(TRUE, 3, 1201);
	LOG_NETADDR(TRUE, 3, destination);
	LOG_NPORT(TRUE, 3, port_rec_ptr->portrec_network_port);
	RET;
    }

    /*
     * Now send the network port identifier and the RO key.
     * The datagram should be sent at security_level.
     */
    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &message, sizeof(po_message_t));
    message.pom_disp_hdr.disp_type = htons(DISP_PO_RO_HINT);
    message.pom_disp_hdr.src_format = conf_own_format;
    message.pom_po_data.pod_nport = port_rec_ptr->portrec_network_port;
    message.pom_po_data.pod_sinfo = port_rec_ptr->portrec_secure_info;
    if (param.crypt_algorithm == CRYPT_MULTPERM) NTOH_KEY(message.pom_po_data.pod_sinfo.si_key);

    tr = transport_switch[TR_DATAGRAM_ENTRY].send(0, &sbuf, destination, TRSERV_NORMAL,
							security_level, 0);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "po_send_ro_xfer_hint.send fails, tr = %d.", tr));
    }
    else {
	INCSTAT(po_ro_hints_sent);
	INCPORTSTAT(port_rec_ptr, xfer_hints_sent);
    }

    RET;

END

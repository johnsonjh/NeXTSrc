/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * po_notify.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/po_notify.c,v $
 *
 */

#ifndef	lint
char po_notify_rcsid[] = "$Header: po_notify.c,v 1.1 88/09/30 15:41:18 osdev Exp $";
#endif not lint

/*
 * Handle port notification messages received from the kernel and other network servers.
 */

/*
 * HISTORY:
 * 09-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Conditionalize encrypt code.
 *
 * 21-Jul-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	NOTIFY: explicitly allocate the notify port.
 *
 * 23-Jun-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added a LOGCHECK.
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  2-Sep-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added RPCMOD. Do not check for pending messages before calling
 *	ipc_retry() and ipc_msg_accepted().
 *
 *  3-Oct-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Add handling of TR_CRYPT_FAILURE to po_ro_xfer_cleanup.
 *
 * 21-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added port statistics.
 *
 * 10-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Go back to using mem_deallocobj to free the port death timer.
 *
 * 27-Jul-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added NETPORT.
 *
 * 22-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed (void) cast from lk_lock calls.
 *
 *  5-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Transfer RO Keys in network byte order if multperm encryption used.
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Enable a port for which receive rights have been transferred sucessfully.
 *	Added some statistics gathering.
 *
 * 23-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Conditionally use thread_lock - ensures only one thread is executing.
 *	Lock is now inline in port record.
 *	Added call to cthread_set_name.
 *
 * 16-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Call nn_remove_entries even if the port records module does not
 *	know about a dead port.  Replaced fprintfs by debugging macros.
 *
 *  4-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#define	PO_NOTIFY_DEBUG		1

#include <cthreads.h>
#include <sys/kern_return.h>
#include <sys/notify.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "config.h"
#include "crypt.h"
#include "debug.h"
#include "disp_hdr.h"
#include "ipc.h"
#include "key_defs.h"
#include "ls_defs.h"
#include "netmsg.h"
#include "network.h"
#include "nm_defs.h"
#include "nm_extra.h"
#include "nn.h"
#include "po_defs.h"
#include "port_defs.h"
#include "portops.h"
#include "portrec.h"
#include "portsearch.h"
#include "rwlock.h"
#include "sbuf.h"
#include "timer.h"
#include "transport.h"

typedef struct {
    disp_hdr_t	po_death_disp_hdr;
    np_uid_t	po_death_puid;
} po_death_t, *po_death_ptr_t;

/*
 * Number of seconds to wait between a port death and deleting the port record.
 */
#define PO_DEATH_WAIT	(5 * 60)



/*
 * po_delete_port
 *	called by timer module to actually delete a port record.
 *
 * Parameters:
 *	timer	: the timer that went off.
 *
 * Results:
 *	none
 *
 */
PRIVATE po_delete_port(timer)
timer_t		timer;
BEGIN("po_delete_port")
    port_rec_ptr_t	port_rec_ptr;

    port_rec_ptr = (port_rec_ptr_t)timer->info;
    pr_destroy(port_rec_ptr);
    MEM_DEALLOCOBJ(timer, MEM_TIMER);

END



/*
 * po_handle_nport_death
 *	handles a port death message received over the network.
 *
 * Parameters:
 *	client_id	: ignored.
 *	data		: data received over the network.
 *	from		: host from which the data was received.
 *	broadcast	: ignored.
 *	crypt_level	: ignored.
 *
 * Design:
 *	If we have send rights to this port and the source is the receiver or the owner
 *	then if the port security level is CRYPT_DONT_ENCRYPT
 *		then delete this port
 *		otherwise call port search to check up on this death.
 *
 */
/* ARGSUSED */
PUBLIC po_handle_nport_death(client_id, data, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("po_handle_nport_death")
    po_death_ptr_t	hint_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(po_deaths_rcvd);
    SBUF_GET_SEG(*data, hint_ptr, po_death_ptr_t);

    if ((port_rec_ptr = pr_np_puid_lookup(hint_ptr->po_death_puid)) == PORT_REC_NULL) {
	RETURN(DISP_SUCCESS);
    }
    /* port_rec_ptr LOCK RW/RW */
    if ((NPORT_HAVE_SEND_RIGHTS(port_rec_ptr->portrec_network_port))
	&& ((from == port_rec_ptr->portrec_network_port.np_receiver)
	    || (from == port_rec_ptr->portrec_network_port.np_owner)))
    {
	if (port_rec_ptr->portrec_security_level == CRYPT_DONT_ENCRYPT) {
	    /*
	     * Believe this port death and destroy the port record.
	     */
	    ipc_port_dead(port_rec_ptr);
	    pr_destroy(port_rec_ptr);
	    RETURN(DISP_SUCCESS);
	}
	else {
	    ps_do_port_search(port_rec_ptr, FALSE, (network_port_ptr_t)0, (int(*)())0);
	}
    }

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

END



/*
 * po_notify_port_death
 *	handles the death of a local port.
 *
 * Parameters:
 *	port_rec_ptr	: the relevant port record.
 *
 * Design:
 *	Marks the port as deleted.
 *	Sends out an unreliable hint about the death of the port.
 *	Sets up a timer to delete this port record after some period.
 *
 * Note:
 *	assumes that the port record is locked.
 *
 */
EXPORT void po_notify_port_death(port_rec_ptr)
port_rec_ptr_t	port_rec_ptr;
BEGIN("po_notify_port_death")
    timer_t	timer;
    po_death_t	death_hint;
    sbuf_t	sbuf;
    sbuf_seg_t	sbuf_seg;
    int		tr;

    port_rec_ptr->portrec_info |= PORT_INFO_DEAD;

    if (port_rec_ptr->portrec_network_port.np_receiver != my_host_id) {
	LOG0(TRUE, 3, 1002);
	port_rec_ptr->portrec_network_port.np_receiver = my_host_id;
    }
    if (port_rec_ptr->portrec_network_port.np_owner != my_host_id) {
	LOG0(TRUE, 3, 1003);
	port_rec_ptr->portrec_network_port.np_owner = my_host_id;
    }

    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &death_hint, sizeof(po_death_t));
    death_hint.po_death_disp_hdr.disp_type = htons(DISP_PO_DEATH);
    death_hint.po_death_disp_hdr.src_format = conf_own_format;
    death_hint.po_death_puid = port_rec_ptr->portrec_network_port.np_puid;
    tr = transport_switch[TR_DATAGRAM_ENTRY].send(0, &sbuf, broadcast_address,
							TRSERV_NORMAL, CRYPT_DONT_ENCRYPT, 0);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "po_notify_port_death.send fails, tr = %d.\n", tr));
    }
    else INCSTAT(po_deaths_sent);

    if ((timer = timer_alloc()) == (timer_t)0) {
	panic("po_notify_port_death.timer_alloc");
    }
    timer->interval.tv_sec = PO_DEATH_WAIT;
    timer->interval.tv_usec = 0;
    timer->action = po_delete_port;
    timer->info = (char *)port_rec_ptr;
    timer_start(timer);

    RET;

END



/*
 * po_ro_xfer_cleanup
 *	called if a notify rights transfer failed.
 *
 * Parameters:
 *	port_rec_ptr	: the relevant port record.
 *	reason		: why it failed.
 *
 * Results:
 *	meaningless.
 *
 * Design:
 *	If the reason is TR_CRYPT_FAILURE then call km_do_key_exchange 
 *	else call ps_do_port_search.
 *	The retry function is either po_xfer_receive or po_xfer_ownership.
 *
 */
PRIVATE po_ro_xfer_cleanup(port_rec_ptr, reason)
port_rec_ptr_t	port_rec_ptr;
int		reason;
BEGIN("po_ro_xfer_cleanup")
    boolean_t	owner_xfer, receiver_xfer;
    netaddr_t	destination = 0;

    DEBUG1(PO_NOTIFY_DEBUG, 3, 1004, reason);

    lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
    /* port_rec_ptr LOCK RW/RW */

    if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
	/*
	 * Receive rights were transferred.
	 */
	owner_xfer = FALSE;
	receiver_xfer = TRUE;
	destination = PORT_REC_OWNER(port_rec_ptr);
    }
    else if (port_rec_ptr->portrec_network_port.np_owner == my_host_id) {
	/*
	 * Ownership rights were transferred.
	 */
	owner_xfer = TRUE;
	receiver_xfer = FALSE;
	destination = PORT_REC_RECEIVER(port_rec_ptr);
    }
    else {
	LOG0(TRUE, 3, 1005);
	owner_xfer = FALSE;
	receiver_xfer = FALSE;
    }

    if (reason == TR_CRYPT_FAILURE) {
	if (destination) km_do_key_exchange(port_rec_ptr,
				(receiver_xfer ? po_xfer_receive :
				(owner_xfer ? po_xfer_ownership : (int(*)())0)),
				destination);
    }
    else {
	ps_do_port_search(port_rec_ptr, FALSE, (network_port_ptr_t)0,
			(receiver_xfer ? po_xfer_receive :
				(owner_xfer ? po_xfer_ownership : (int(*)())0)));
    }
    lk_unlock(&port_rec_ptr->portrec_lock);
    /* port_rec_ptr LOCK -/- */

    RETURN(0);

END



/*
 * po_handle_ro_xfer_reply
 *	handles a reply to a notify rights transfer.
 *
 * Parameters:
 *	client_id	: pointer to port record for this transfer.
 *	reply		: the data of the reply.
 *	from		: the host that got the rights.
 *	broadcast	: ignored.
 * 	crypt_level	: ignored.
 *
 * Returns:
 *	DISP_SUCCESS
 *
 * Design:
 *	If the status stored in the pod_size field of the message is not 0
 *	then call po_ro_xfer_cleanup directly since this transfer failed
 *	otherwise update the port record to reflect the success of this transfer.
 *
 */
/* ARGSUSED */
PUBLIC po_handle_ro_xfer_reply(client_id, reply, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	reply;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("po_handle_ro_xfer_reply")
    po_message_ptr_t	message_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(po_xfer_replies_rcvd);
    SBUF_GET_SEG(*reply, message_ptr, po_message_ptr_t);
    port_rec_ptr = (port_rec_ptr_t)client_id;

    if (message_ptr->pom_po_data.pod_size != 0) {
	LOG0(TRUE, 3, 1006);
	(void)po_ro_xfer_cleanup(port_rec_ptr, (int)ntohs(message_ptr->pom_po_data.pod_size));
	RETURN(DISP_SUCCESS);
    }

    lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
    /* port_rec_ptr LOCK RW/RW */

    /*
     * Update the port record to reflect the successful transfer.
     */
    if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
	/*
	 * Receive rights were transferred.
	 */
	port_rec_ptr->portrec_network_port.np_receiver = port_rec_ptr->portrec_network_port.np_owner;
	INCPORTSTAT(port_rec_ptr, rcv_rights_xferd);
	/*
	 * Unblock the port, enable the local port and call ipc_retry or ipc_msg_accepted.
	 */
	if ((port_enable(task_self(), port_rec_ptr->portrec_local_port)) != KERN_SUCCESS) {
	    ERROR((msg, "po_handler_ro_xfer_replt.port_enables fails."));
	}
	port_rec_ptr->portrec_info &= ~PORT_INFO_BLOCKED;
#if	RPCMOD
	(void)ipc_retry(port_rec_ptr);
	ipc_msg_accepted(port_rec_ptr);
#else	RPCMOD
	if (port_rec_ptr->portrec_current_ipcrec != 0) (void)ipc_retry(port_rec_ptr);
	if (port_rec_ptr->portrec_block_queue != 0) ipc_msg_accepted(port_rec_ptr);
#endif	RPCMOD
    }
    else if (port_rec_ptr->portrec_network_port.np_owner == my_host_id) {
	/*
	 * Ownership rights were transferred.
	 */
	port_rec_ptr->portrec_network_port.np_owner = port_rec_ptr->portrec_network_port.np_receiver;
	INCPORTSTAT(port_rec_ptr, own_rights_xferd);
    }
    else {
	LOG0(TRUE, 3, 1007);
    }

#if	NETPORT | RPCMOD
    ipc_port_moved(port_rec_ptr);
#endif	NETPORT | RPCMOD

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

END



/*
 * po_handle_ro_xfer_request
 *	handles reception of a notify rights transfer.
 *
 * Parameters:
 *	request		: the data of the xfer.
 *	from		: the host that sent the rights.
 *	broadcast	: ignored.
 * 	crypt_level	: ignored.
 *
 * Returns:
 *	DISP_SUCCESS
 *
 * Design:
 *	If we have the complentary rights to those sent
 *	then update our port record
 *	otherwise reject the xfer.
 *
 */
/* ARGSUSED */
PUBLIC po_handle_ro_xfer_request(request, from, broadcast, crypt_level)
sbuf_ptr_t	request;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("po_handle_ro_xfer_request")
    po_message_ptr_t	message_ptr;
    port_rec_ptr_t	port_rec_ptr;
    int			port_right;

    INCSTAT(po_xfer_requests_rcvd);
    SBUF_GET_SEG(*request, message_ptr, po_message_ptr_t);

    /*
     * Reset the dispatch header for the reply.
     */
    message_ptr->pom_disp_hdr.src_format = conf_own_format;

    if ((port_rec_ptr = pr_nportlookup(&(message_ptr->pom_po_data.pod_nport))) == PORT_REC_NULL) {
	LOG0(TRUE, 3, 1008);
	message_ptr->pom_po_data.pod_size = -1;
	RETURN(DISP_SUCCESS);
    }
    /* port_rec_ptr LOCK RW/RW */

    /*
     * Check that we are entitled to receive the rights transferred.
     */
    port_right = ntohs(message_ptr->pom_po_data.pod_right);
    switch (port_right) {
	case PORT_RECEIVE_RIGHTS: {
	    if (port_rec_ptr->portrec_network_port.np_owner != my_host_id) {
		LOG0(TRUE, 3, 1009);
		message_ptr->pom_po_data.pod_size = -1;
		lk_unlock(&port_rec_ptr->portrec_lock);
		RETURN(DISP_SUCCESS);
	    }
	    break;
	}
	case PORT_OWNERSHIP_RIGHTS: {
	    if (port_rec_ptr->portrec_network_port.np_receiver != my_host_id) {
		LOG0(TRUE, 3, 1010);
		message_ptr->pom_po_data.pod_size = -1;
		lk_unlock(&port_rec_ptr->portrec_lock);
		RETURN(DISP_SUCCESS);
	    }
	    break;
	}
	default: {
	    LOG1(TRUE, 3, 1011, port_right);
	    message_ptr->pom_po_data.pod_size = -1;
	    lk_unlock(&port_rec_ptr->portrec_lock);
	    RETURN(DISP_SUCCESS);
/*	    break; */
	}
    }

    /*
     * If this is a secure transfer, check the RO key.
     */
    if (param.crypt_algorithm == CRYPT_MULTPERM) NTOH_KEY(message_ptr->pom_po_data.pod_sinfo.si_key);
#if	USE_CRYPT
    if ((crypt_level != CRYPT_DONT_ENCRYPT)
	&& (!(KEY_EQUAL(port_rec_ptr->portrec_secure_info.si_key,
				message_ptr->pom_po_data.pod_sinfo.si_key))))
    {
	LOG0(TRUE, 3, 1012);
	message_ptr->pom_po_data.pod_size = -1;
    }
    else {
#endif	USE_CRYPT
	/*
	 * Accept the rights transferred.
	 */
	if (port_right == PORT_OWNERSHIP_RIGHTS) {
	    port_rec_ptr->portrec_network_port.np_owner = my_host_id;
	    INCPORTSTAT(port_rec_ptr, own_rights_xferd);
	}
	else {
	    port_rec_ptr->portrec_network_port.np_receiver = my_host_id;
	    INCPORTSTAT(port_rec_ptr, rcv_rights_xferd);
	}
	po_port_deallocate(port_rec_ptr->portrec_local_port);
	message_ptr->pom_po_data.pod_size = 0;
#if	NETPORT | RPCMOD
	ipc_port_moved(port_rec_ptr);
#endif	NETPORT | RPCMOD
#if	USE_CRYPT
    }
#endif	USE_CRYPT

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);
		
END



/*
 * po_xfer_ownership
 *	handles the kernel initiated transfer of ownership rights for a local port.
 *
 * Parameters:
 *	port_rec_ptr	: the relevant port record.
 *
 * Design:
 *	Package up the rights to be sent over the network to the receiver.
 *	Send it using the SRR transport protocol.
 *	If it fails, initiate a port search to find out the identity of the real receiver.
 *
 * Results:
 *	meaningless.
 *
 * Note:
 *	assumes that the port record is locked.
 *
 */
PUBLIC po_xfer_ownership(port_rec_ptr)
port_rec_ptr_t	port_rec_ptr;
BEGIN("po_xfer_ownership")
    sbuf_t		sbuf;
    sbuf_seg_t		sbuf_seg;
    po_message_t	message;
    int			tr;

    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &message, sizeof(po_message_t));

    /*
     * If we have got ownership rights from the kernel
     * then this host must be the owner for this port
     * but this host cannot be the receiver for this port.
     */
    if (port_rec_ptr->portrec_network_port.np_owner != my_host_id) {
	LOG0(TRUE, 3, 1013);
	port_rec_ptr->portrec_network_port.np_owner = my_host_id;
    }
    if (port_rec_ptr->portrec_network_port.np_receiver == my_host_id) {
	LOG0(TRUE, 3, 1014);
	po_notify_port_death(port_rec_ptr);
	RETURN(0);
    }

    /*
     * Fill in the information to be transferred.
     */
    message.pom_po_data.pod_right = htons(PORT_OWNERSHIP_RIGHTS);
    message.pom_po_data.pod_nport = port_rec_ptr->portrec_network_port;
#if	USE_CRYPT
    if (port_rec_ptr->portrec_security_level != CRYPT_DONT_ENCRYPT) {
	message.pom_po_data.pod_sinfo = port_rec_ptr->portrec_secure_info;
	if (param.crypt_algorithm == CRYPT_MULTPERM) HTON_KEY(message.pom_po_data.pod_sinfo.si_key);
    }
#endif	USE_CRYPT
    message.pom_disp_hdr.disp_type = htons(DISP_PO_RO_XFER);
    message.pom_disp_hdr.src_format = conf_own_format;

#if	NETPORT
    /*
     * Remove the netport until the rights have been successfully transferred.
     */
    ipc_netport_remove(port_rec_ptr->portrec_network_port);
#endif	NETPORT

    /*
     * Now send the data.
     */
    tr = transport_switch[TR_SRR_ENTRY].send(port_rec_ptr, &sbuf,
			port_rec_ptr->portrec_network_port.np_receiver, TRSERV_NORMAL,
			port_rec_ptr->portrec_security_level, po_ro_xfer_cleanup);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "po_xfer_ownership.send fails, tr = %d, calling po_ro_xfer_cleanup.\n", tr));
	lk_unlock(&port_rec_ptr->portrec_lock);
	/* port_rec_ptr LOCK -/- */
	(void)po_ro_xfer_cleanup(port_rec_ptr, tr);
	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
	/* port_rec_ptr LOCK RW/RW */
    }
    else INCSTAT(po_xfer_requests_sent);

    RETURN(0);
END



/*
 * po_xfer_receive
 *	handles the kernel initiated transfer of receive rights for a local port.
 *
 * Parameters:
 *	port_rec_ptr	: the relevant port record.
 *
 * Design:
 *	Package up the rights to be sent over the network to the owner.
 *	Send it using the SRR transport protocol.
 *	If it fails, initiate a port search to find out the identity of the real owner.
 *
 * Results:
 *	meaningless.
 *
 * Note:
 *	assumes that the port record is locked.
 *
 */
PUBLIC po_xfer_receive(port_rec_ptr)
port_rec_ptr_t	port_rec_ptr;
BEGIN("po_xfer_receive")
    sbuf_t		sbuf;
    sbuf_seg_t		sbuf_seg;
    po_message_t	message;
    int			tr;

    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &message, sizeof(po_message_t));

    /*
     * If we have got receive rights from the kernel
     * then this host must be the receiver for this port
     * but this host cannot be the owner for this port.
     */
    if (port_rec_ptr->portrec_network_port.np_receiver != my_host_id) {
	LOG0(TRUE, 3, 1015);
	port_rec_ptr->portrec_network_port.np_receiver = my_host_id;
    }
    if (port_rec_ptr->portrec_network_port.np_owner == my_host_id) {
	LOG0(TRUE, 3, 1016);
	po_notify_port_death(port_rec_ptr);
	RETURN(0);
    }

    /*
     * Block the port.
     */
    port_rec_ptr->portrec_info |= PORT_INFO_BLOCKED;

    /*
     * Fill in the information to be transferred.
     */
    message.pom_po_data.pod_right = htons(PORT_RECEIVE_RIGHTS);
    message.pom_po_data.pod_nport = port_rec_ptr->portrec_network_port;
#if	USE_CRYPT
    if (port_rec_ptr->portrec_security_level != CRYPT_DONT_ENCRYPT) {
	message.pom_po_data.pod_sinfo = port_rec_ptr->portrec_secure_info;
	if (param.crypt_algorithm == CRYPT_MULTPERM) HTON_KEY(message.pom_po_data.pod_sinfo.si_key);
    }
#endif	USE_CRYPT
    message.pom_disp_hdr.disp_type = htons(DISP_PO_RO_XFER);
    message.pom_disp_hdr.src_format = conf_own_format;

#if	NETPORT
    /*
     * Remove the netport until the rights have been successfully transferred.
     */
    ipc_netport_remove(port_rec_ptr->portrec_network_port);
#endif	NETPORT

    /*
     * Now send the data.
     */
    tr = transport_switch[TR_SRR_ENTRY].send(port_rec_ptr, &sbuf,
			port_rec_ptr->portrec_network_port.np_owner, TRSERV_NORMAL,
			port_rec_ptr->portrec_security_level, po_ro_xfer_cleanup);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "po_xfer_receive.send fails, tr = %d, calling po_ro_xfer_cleanup.\n", tr));
	lk_unlock(&port_rec_ptr->portrec_lock);
	/* port_rec_ptr LOCK -/- */
	(void)po_ro_xfer_cleanup(port_rec_ptr, tr);
	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
	/* port_rec_ptr LOCK RW/RW */
    }
    else INCSTAT(po_xfer_requests_sent);

    RETURN(0);
END



/*
 * po_notify_main
 *	Wait for message on the notify port and act accordingly.
 *
 * Results:
 *	should never exit.
 *
 */
PRIVATE po_notify_main()
BEGIN("po_notify_main")
    port_t		notify_port;
    notification_t	notify_msg;
    port_t		target_port;
    port_rec_ptr_t	port_rec_ptr;
    kern_return_t	kr;

#if	LOCK_THREADS
    mutex_lock(thread_lock);
#endif	LOCK_THREADS

#if	NOTIFY
    kr = port_allocate(task_self(),&notify_port);
    if (kr != KERN_SUCCESS) {
	ERROR((msg,"[NOTIFY] port_allocate(notify_port) failed, kr=%d",kr));
    }
    kr = task_set_special_port(task_self(),TASK_NOTIFY_PORT,notify_port);
    if (kr != KERN_SUCCESS) {
	ERROR((msg,"[NOTIFY] task_set_special_port(notify_port) failed, kr=%d",kr));
    }
#else	NOTIFY
    notify_port = task_notify();
#endif	NOTIFY

    kr = port_disable(task_self(), notify_port);

    while (TRUE) {
	notify_msg.notify_header.msg_size = sizeof(notification_t);
	notify_msg.notify_header.msg_local_port = notify_port;
	if ((kr = netmsg_receive((msg_header_t *)&notify_msg)) == RCV_SUCCESS) {
	    target_port = notify_msg.notify_port;
	    DEBUG2(PO_NOTIFY_DEBUG, 1, 1001, notify_msg.notify_header.msg_id, target_port);
	    if ((port_rec_ptr = pr_lportlookup(target_port)) == PORT_REC_NULL) {
		/*
		 * Even if the port is not known about by the port records module,
		 * it may be known about by the local name service module.
		 */
		if (notify_msg.notify_header.msg_id == NOTIFY_PORT_DELETED) {
		    nn_remove_entries(target_port);
		}
	    }
	    else {
		/* port_rec_ptr LOCK RW/RW */
		switch (notify_msg.notify_header.msg_id) {
		    case NOTIFY_PORT_DELETED: {
			po_notify_port_death(port_rec_ptr);
			ipc_port_dead(port_rec_ptr);
			nn_remove_entries(target_port);
			break;
		    }
		    case NOTIFY_MSG_ACCEPTED: {
			port_rec_ptr->portrec_info &= ~PORT_INFO_BLOCKED;
			ipc_msg_accepted(port_rec_ptr);
			break;
		    }
		    case NOTIFY_OWNERSHIP_RIGHTS: {
			(void)po_xfer_ownership(port_rec_ptr);
			break;
		    }
		    case NOTIFY_RECEIVE_RIGHTS: {
			(void)po_xfer_receive(port_rec_ptr);
			break;
		    }
		    default: {
			ERROR((msg, "po_notify_main: unknown message id %d.\n",
					notify_msg.notify_header.msg_id));
			break;
		    }
		}
		lk_unlock(&port_rec_ptr->portrec_lock);
		/* port_rec_ptr LOCK -/- */
	    }
	}
	else {
	    ERROR((msg, "po_notify_main.netmsg_receive fails, kr = %d.\n", kr));
	}
	LOGCHECK;
    }

END



/*
 * po_notify_init
 *	Starts up a thread to receive kernel notification messages.
 *
 *
 */
PUBLIC void po_notify_init()
BEGIN("po_notify_init")
    cthread_t		new_thread;

    new_thread = cthread_fork((cthread_fn_t)po_notify_main, (any_t)0);
    cthread_set_name(new_thread, "po_notify_main");
    cthread_detach(new_thread);

    RET;


END

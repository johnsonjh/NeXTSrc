/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * ps_auth.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/ps_auth.c,v $
 *
 */

#ifndef	lint
char ps_auth_rcsid[] = "$Header: ps_auth.c,v 1.1 88/09/30 15:41:55 osdev Exp $";
#endif not lint

/*
 * Functions handling authentication of a receiver/owner.
 */

/*
 * HISTORY:
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  6-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for new style of NETPORT calls.
 *
 *  3-Oct-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Add handling of TR_CRYPT_FAILURE to ps_cleanup.
 *
 * 27-Jul-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added NETPORT.
 *
 * 22-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed (void) cast from lk_lock calls.
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Set the aliveness of a port to PORT_ACTIVE on
 *	successful termination of an authentication request.
 *	Added some statistics gathering.
 *
 * 29-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Replaced fprintf by ERROR and DEBUG/LOG macros.
 *	Lock is now inline in port record.
 *	Added some debugging - controlled by PS_DEBUG.
 *
 *  9-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include <sys/types.h>
#include <netinet/in.h>

#include "config.h"
#include "disp_hdr.h"
#include "key_defs.h"
#include "multperm.h"
#include "netmsg.h"
#include "network.h"
#include "nm_defs.h"
#include "port_defs.h"
#include "portops.h"
#include "portrec.h"
#include "ps_defs.h"
#include "rwlock.h"
#include "sbuf.h"
#include "transport.h"
#include "ipc.h"



/*
 * ps_auth_km_retry
 *	called if a key exchange for an authentication request completes.
 *
 * Parameters:
 *	event_ptr	: the event for which transmission previously failed.
 *
 * Results:
 *	ignored
 *
 * Design:
 *	Just call ps_authenticate_port.
 *
 */
PUBLIC ps_auth_km_retry(event_ptr)
ps_event_ptr_t	event_ptr;
BEGIN("ps_auth_km_retry")
    port_rec_ptr_t	port_rec_ptr;

    if ((port_rec_ptr = pr_lportlookup(event_ptr->pse_lport)) == PORT_REC_NULL) {
	LOG1(TRUE, 4, 1253, event_ptr->pse_lport);
	RETURN(0);
    }
    ps_authenticate_port(event_ptr, port_rec_ptr);
    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(0);

END


/*
 * ps_auth_cleanup
 *	called if a transmission of an authentication request failed for some reason.
 *
 * Parameters:
 *	event_ptr	: pointer to the event for which the transmission failed.
 *	reason		: reason why the transmission failed.
 *
 * Results:
 *	meaningless.
 *
 * Design:
 *	If reason is TR_CRYPT_FAILURE then call km_do_key_exchange
 *	otherwise try searching for this port once more.
 *
 */
PRIVATE ps_auth_cleanup(event_ptr, reason)
ps_event_ptr_t	event_ptr;
int		reason;
BEGIN("ps_auth_cleanup")
    port_rec_ptr_t	port_rec_ptr;

    LOG1(TRUE, 3, 1240, reason);

    if (reason == TR_CRYPT_FAILURE) {
	LOG1(TRUE, 3, 1252, event_ptr->pse_destination);
	km_do_key_exchange(event_ptr, ps_auth_km_retry, event_ptr->pse_destination);
    }
    else {
	if ((port_rec_ptr = pr_lportlookup(event_ptr->pse_lport)) == PORT_REC_NULL) {
	    LOG1(TRUE, 4, 1241, event_ptr->pse_lport);
	    RETURN(0);
	}
	/* port_rec_ptr LOCK RW/RW */
	event_ptr->pse_state &= ~PS_AUTHENTICATION;
	event_ptr->pse_destination = broadcast_address;
	ps_send_query(event_ptr, port_rec_ptr);
	lk_unlock(&port_rec_ptr->portrec_lock);
    }


    RETURN(0);

END



/*
 * ps_authenticate_port
 *	perform an network port authentication.
 *
 * Parameters:
 *	event_ptr	: pointer to event for which an authentication is required.
 *	port_rec_ptr	: pointer to port for which an authentication is required.
 *
 * Design:
 *	If we are the receiver or owner for this port then we must create a new token.
 *	Send off an authentication request to the destination contained in the event.
 *
 * Note:
 *	the port record should be locked.
 *
 */
PUBLIC void ps_authenticate_port(event_ptr, port_rec_ptr)
ps_event_ptr_t	event_ptr;
port_rec_ptr_t	port_rec_ptr;
BEGIN("ps_authenticate_port")
    sbuf_t	sbuf;
    sbuf_seg_t	sbuf_seg;
    ps_auth_t	auth_data;
    int		tr;

    DEBUG1(PS_DEBUG, 3, 1242, port_rec_ptr->portrec_local_port);
    DEBUG_NETADDR(PS_DEBUG, 3, event_ptr->pse_destination);
    DEBUG_NPORT(PS_DEBUG, 3, port_rec_ptr->portrec_network_port);

    if (event_ptr->pse_state & PS_AUTHENTICATION) {
	LOG0(TRUE, 4, 1243);
	RET;
    }

    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &auth_data, sizeof(ps_auth_t));
    auth_data.psa_disp_hdr.disp_type = htons(DISP_PS_AUTH);
    auth_data.psa_disp_hdr.src_format = conf_own_format;
    auth_data.psa_puid = port_rec_ptr->portrec_network_port.np_puid;

    if (NPORT_HAVE_RO_RIGHTS(port_rec_ptr->portrec_network_port)) {
	/*
	 * Create a token and stuff the new random number into the port record.
	 */
	port_rec_ptr->portrec_random = po_create_token(port_rec_ptr, &(auth_data.psa_token));
    }
    else {
	auth_data.psa_token = port_rec_ptr->portrec_secure_info;
    }

    tr = transport_switch[TR_SRR_ENTRY].send(event_ptr, &sbuf, event_ptr->pse_destination, TRSERV_NORMAL,
						port_rec_ptr->portrec_security_level, ps_auth_cleanup);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "ps_authenticate_port.send fails, tr = %d.", tr));
	lk_unlock(&port_rec_ptr->portrec_lock);
	(void)ps_auth_cleanup(event_ptr, tr);
	lk_lock(&port_rec_ptr->portrec_lock, PERM_READWRITE, TRUE);
    }
    else INCSTAT(ps_auth_requests_sent);

    RET;

END



#if	USE_MULTPERM
/*
 * ps_handle_auth_reply
 *	Handles a reply to an authentication request.
 *
 * Parameters:
 *	client_id	: pointer to the event which triggered this authentication.
 *	reply		: pointer to the reply sbuf.
 *	from		: ignored.
 *	broadcast	: ignored.
 *	crypt_level	: ignored.
 *
 * Returns:
 *	DISP_SUCCESS
 *
 * Design:
 *	Check that the random number received is the same as the one stored in the port record.
 *	If it matches terminate the port search successfully
 *	otherwise continue the port search.
 *
 */
/* ARGSUSED */
PUBLIC ps_handle_auth_reply(client_id, reply, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	reply;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("ps_handle_auth_reply")
    ps_auth_ptr_t	auth_ptr;
    port_rec_ptr_t	port_rec_ptr;
    ps_event_ptr_t	event_ptr;

    INCSTAT(ps_auth_replies_rcvd);
    SBUF_GET_SEG(*reply, auth_ptr, ps_auth_ptr_t);
    event_ptr = (ps_event_ptr_t)client_id;

    if ((port_rec_ptr = pr_lportlookup(event_ptr->pse_lport)) == PORT_REC_NULL) {
	LOG1(TRUE, 4, 1243, event_ptr->pse_lport);
	RETURN(DISP_SUCCESS);
    }
    /* port_rec_ptr LOCK RW/R */

    if (port_rec_ptr->portrec_random == auth_ptr->psa_random) {
	/*
	 * We can terminate the port search successfully.
	 */
	DEBUG1(PS_DEBUG, 3, 1244, event_ptr->pse_lport);
	port_rec_ptr->portrec_info &= ~PORT_INFO_SUSPENDED;
	port_rec_ptr->portrec_aliveness = PORT_ACTIVE;
#if	NETPORT
	ipc_netport_enter(port_rec_ptr->portrec_network_port,
		port_rec_ptr->portrec_local_port,
		(port_rec_ptr->portrec_network_port.np_receiver == my_host_id));
#endif	NETPORT
	(void)event_ptr->pse_retry(port_rec_ptr);
	MEM_DEALLOCOBJ(event_ptr, MEM_PSEVENT);
    }
    else {
	/*
	 * We must continue the port search.
	 */
	DEBUG1(PS_DEBUG, 3, 1245, event_ptr->pse_lport);
	event_ptr->pse_state &= ~PS_AUTHENTICATION;
	event_ptr->pse_destination = broadcast_address;
	ps_send_query(event_ptr, port_rec_ptr);
    }

    lk_unlock(&port_rec_ptr->portrec_lock);
    RETURN(DISP_SUCCESS);

END



/*
 * ps_handle_auth_request
 *	handles an authentication request.
 *
 * Parameters:
 *	request		: the incoming authentication request.
 *	from		: ignored.
 *	broadcast	: ignored.
 *	crypt_level	: ignored.
 *
 * Results:
 *	DISP_SUCCESS or DISP_FAILURE.
 *
 * Design:
 *	If we are the receiver or owner for this port
 *	then decrypt the token, check the SID and return the random number inside it.
 *
 */
/* ARGSUSED */
PUBLIC ps_handle_auth_request(request, from, broadcast, crypt_level)
sbuf_ptr_t	request;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("ps_handle_auth_request")
    ps_auth_ptr_t	auth_ptr;
    port_rec_ptr_t	port_rec_ptr;
    key_t		ikey;
    secure_info_t	token;

    INCSTAT(ps_auth_requests_rcvd);
    SBUF_GET_SEG(*request, auth_ptr, ps_auth_ptr_t);
    auth_ptr->psa_disp_hdr.src_format = conf_own_format;

    if ((port_rec_ptr = pr_np_puid_lookup(auth_ptr->psa_puid)) == PORT_REC_NULL) {
	LOG2(TRUE, 4, 1246, auth_ptr->psa_puid.np_uid_high, auth_ptr->psa_puid.np_uid_low);
	RETURN(DISP_IGNORE);
    }
    /* port_rec_ptr LOCK RW/R */

    DEBUG0(PS_DEBUG, 3, 1247);
    DEBUG_NPORT(PS_DEBUG, 3, port_rec_ptr->portrec_network_port);

    if (!(NPORT_HAVE_RO_RIGHTS(port_rec_ptr->portrec_network_port))) {
	LOG0(TRUE, 4, 1248);
	LOG_NPORT(TRUE, 4, port_rec_ptr->portrec_network_port);
	RETURN(DISP_IGNORE);
    }

    /*
     * Decrypt the token.
     */
    ikey = port_rec_ptr->portrec_secure_info.si_key;
    invert_key(&ikey);
    token = auth_ptr->psa_token;
    multdecrypt(ikey, (mp_block_ptr_t)&(token.si_token.key_longs[0]));
    multdecrypt(ikey, (mp_block_ptr_t)&(token.si_token.key_longs[2]));

    /*
     * Check the SID.
     */
    if ((token.si_token.key_longs[1] != port_rec_ptr->portrec_network_port.np_sid.np_uid_high)
	 || (token.si_token.key_longs[2] != port_rec_ptr->portrec_network_port.np_sid.np_uid_low))
    {
	LOG2(TRUE, 4, 1249, token.si_token.key_longs[1], token.si_token.key_longs[2]);
	LOG_NPORT(TRUE, 4, port_rec_ptr->portrec_network_port);
	lk_unlock(&port_rec_ptr->portrec_lock);
	RETURN(DISP_IGNORE);
    }

    if (token.si_token.key_longs[0] != token.si_token.key_longs[3]) {
	LOG2(TRUE, 4, 1250, token.si_token.key_longs[0], token.si_token.key_longs[3]);
	lk_unlock(&port_rec_ptr->portrec_lock);
	RETURN(DISP_IGNORE);
    }

    auth_ptr->psa_random = token.si_token.key_longs[0];
    lk_unlock(&port_rec_ptr->portrec_lock);
    DEBUG0(PS_DEBUG, 3, 1251);
    RETURN(DISP_SUCCESS);

END

#endif	USE_MULTPERM

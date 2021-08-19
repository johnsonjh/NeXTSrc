/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * pc_startup.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/pc_startup.c,v $
 *
 */

#ifndef	lint
char pc_startup_rcsid[] = "$Header: pc_startup.c,v 1.1 88/09/30 15:41:12 osdev Exp $";
#endif not lint

/*
 * Sends and handles the startup hint sent out by a network server.
 */

/*
 * HISTORY:
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 10-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Call mem_deallocobj to free the port list queue.
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added some statistics gathering.
 *
 * 29-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Replaced fprintf by DEBUG and ERROR/LOG macros.
 *	Lock is now inline in port record.
 *
 * 11-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */


#include "config.h"
#include "crypt.h"
#include "debug.h"
#include "disp_hdr.h"
#include "dispatcher.h"
#include "lock_queue.h"
#include "mem.h"
#include "netmsg.h"
#include "network.h"
#include "nm_defs.h"
#include "pc_defs.h"
#include "port_defs.h"
#include "portrec.h"
#include "portsearch.h"
#include "rwlock.h"
#include "sbuf.h"
#include "transport.h"


/*
 * pc_handle_startup_hint
 *	reacts to a startup hint received from a network server.
 *
 * Parameters:
 *	client_id	: ignored.
 *	data		: ignored.
 *	from		: the host that sent the hint.
 *	broadcast	: ignored.
 *	crypt_level	: ignored.
 *
 * Returns:
 *	DISP_SUCCESS.
 *
 * Design:
 *	Calls pr_list to get hold of all the ports that we know about.
 *	For each port, if its owner or receiver is the restarted host call ps_do_port_search.
 *
 */
/* ARGSUSED */
PUBLIC pc_handle_startup_hint(client_id, data, from, broadcast, crypt_level)
int		client_id;
sbuf_ptr_t	data;
netaddr_t	from;
boolean_t	broadcast;
int		crypt_level;
BEGIN("pc_handle_startup_hint")
    lock_queue_t	port_queue;
    port_item_ptr_t	pi_ptr;
    port_rec_ptr_t	port_rec_ptr;

    INCSTAT(pc_startups_rcvd);
    DEBUG0(PC_DEBUG, 3, 1180);
    DEBUG_NETADDR(PC_DEBUG, 3, from);

    port_queue = pr_list();

    while ((pi_ptr = (port_item_ptr_t)lq_dequeue(port_queue)) != (port_item_ptr_t)0) {
	if ((port_rec_ptr = pr_lportlookup(pi_ptr->pi_port)) != PORT_REC_NULL) {
	    /* port_rec_ptr LOCK RW/RW */
	    /*
	     * Check the receiver and owner of the network port.
	     */
	    if ((PORT_REC_OWNER(port_rec_ptr) == from) || (PORT_REC_RECEIVER(port_rec_ptr) == from)) {
		ps_do_port_search(port_rec_ptr, FALSE, (network_port_ptr_t)0, (int(*)())0);
	    }
	    lk_unlock(&port_rec_ptr->portrec_lock);
	    /* port_rec_ptr LOCK -/- */
	}
	MEM_DEALLOCOBJ(pi_ptr, MEM_PORTITEM);
    }

    MEM_DEALLOCOBJ(port_queue, MEM_LQUEUE);

    RETURN(DISP_SUCCESS);

END



/*
 * pc_send_startup_hint
 *	sends out a hint to say that we have restarted.
 *
 */
PUBLIC void pc_send_startup_hint()
BEGIN ("pc_send_startup_hint")
    sbuf_t	sbuf;
    sbuf_seg_t	sbuf_seg;
    disp_hdr_t	disp_hdr;
    int		tr;

    SBUF_SEG_INIT(sbuf, &sbuf_seg);
    SBUF_APPEND(sbuf, &disp_hdr, sizeof(disp_hdr_t));
    disp_hdr.disp_type = htons(DISP_STARTUP);
    disp_hdr.src_format = conf_own_format;
    tr = transport_switch[TR_DATAGRAM_ENTRY].send(0, &sbuf, broadcast_address, TRSERV_NORMAL,
							CRYPT_DONT_ENCRYPT, 0);
    if (tr != TR_SUCCESS) {
	ERROR((msg, "pc_send_startup_hint.send fails, tr = %d."));
    }
    RET;

END

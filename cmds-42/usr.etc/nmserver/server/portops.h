/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * portops.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/portops.h,v $
 *
 * $Header: portops.h,v 1.1 88/09/30 15:45:03 osdev Exp $
 *
 */

/*
 * Definitions for the Port Operations module.
 */

/*
 * HISTORY:
 *  9-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added external definitions of po_create_token, po_port_deallocate and po_notify_port_death.
 *
 * 27-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added external definition of po_check_ipc_seq_no.
 *
 *  4-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added definitions of exported functions.
 *
 *  5-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef _PORTOPS_
#define _PORTOPS_

/*
 * Sizes used by po_translate_[ln]port_rights.
 */
#define PO_MAX_NPD_ENTRY_SIZE	48
#define PO_MIN_NPD_ENTRY_SIZE	28
#define PO_NPD_SEG_SIZE		256

/*
 * Completion codes passed to po_port_rights_commit.
 */
#define PO_RIGHTS_XFER_SUCCESS	0
#define PO_RIGHTS_XFER_FAILURE	1


/*
 * Functions exported by the port operations module.
 */
#include <sys/boolean.h>

extern boolean_t po_init();
/*
*/

extern boolean_t po_check_ipc_seq_no();
/*
port_rec_ptr_t	portrec_ptr;
netaddr_t	host_id;
long		ipc_seq_no;
*/

extern long po_create_token();
/*
port_rec_ptr_t		port_rec_ptr;
secure_info_ptr_t	token_ptr;
*/

extern void po_notify_port_death();
/*
port_rec_ptr_t	port_rec_ptr;
*/

extern void po_port_deallocate();
/*
port_t			lport;
*/

extern int po_translate_lport_rights();
/*
int		client_id;
port_t		lport;
int		right;
int		security_level;
netaddr_t	destination_hint;
pointer_t	port_data;	To be sent to the remote network server.
*/

extern void po_port_rights_commit();
/*
int		client_id;
int		completion_code;
netaddr_t	destination;
*/

extern int po_translate_nport_rights();
/*
netaddr_t	source;
pointer_t	port_data;	Received from the remote network server.
int		security_level;
port_t		*lport;
int		*right;
*/

#endif _PORTOPS_

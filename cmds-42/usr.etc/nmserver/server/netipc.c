/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * netipc.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/netipc.c,v $
 *
 */

#ifndef	lint
char netipc_rcsid[] = "$Header: netipc.c,v 1.1 88/09/30 15:40:28 osdev Exp $";
#endif not lint

/*
 * Functions to send and receive packets
 * using the IPC interface to the network.
 */

/*
 * HISTORY: 
 * 17-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Ignore the UDP checksum if a packet is (to be) encrypted.
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added include of nm_extra.h.
 *
 * 23-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Changed netipc_receive to reject any packets from ourself.
 *	Replaced fprintf by LOG/DEBUG macros.
 *
 * 22-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */


#define NETIPC_DEBUG	0

#include <mach.h>
#include <sys/message.h>

#include "crypt.h"
#include "debug.h"
#include "netipc.h"
#include "netmsg.h"
#include "network.h"
#include "nm_defs.h"
#include "nm_extra.h"


/*
 * netipc_receive
 *	Receive a packet over the network using netmsg_receive.
 *	Reject a packet from ourself otherwise check its UDP checksum if it is not encrypted.
 *
 * Parameters:
 *	pkt_ptr	: pointer to a buffer to hold the packet
 *
 * Results:
 *	NETIPC_BAD_UDP_CHECKSUM or the return code from netmsg_receive.
 *
 */
EXPORT netipc_receive(pkt_ptr)
register netipc_header_ptr_t	pkt_ptr;
BEGIN("netipc_receive")
    register kern_return_t	mr;
    register int		old_msg_size;

    old_msg_size = pkt_ptr->nih_msg_header.msg_size;
    while (1)
    {
	DEBUG1(NETIPC_DEBUG, 0, 1033, old_msg_size);
	mr = netmsg_receive((msg_header_t *)pkt_ptr);
	if (mr != RCV_SUCCESS) {
	    DEBUG1(NETIPC_DEBUG, 3, 1034, mr);
	    RETURN(mr);
	}
	else if (pkt_ptr->nih_ip_header.ip_src.s_addr == my_host_id) {
	    pkt_ptr->nih_msg_header.msg_size = old_msg_size;
	}
	else if ((pkt_ptr->nih_crypt_header.ch_crypt_level == CRYPT_DONT_ENCRYPT)
		|| (pkt_ptr->nih_udp_header.uh_sum))
	{
	    /*
	     * Not from ourself - check the UDP Checksum.
	     */
	    pkt_ptr->nih_ip_header.ip_ttl = 0;
	    pkt_ptr->nih_ip_header.ip_sum = pkt_ptr->nih_udp_header.uh_ulen;
	    if (pkt_ptr->nih_udp_header.uh_sum
		= udp_checksum((unsigned short *)&(pkt_ptr->nih_ip_header.ip_ttl),
							(pkt_ptr->nih_ip_header.ip_len - 8)))
	    {
		LOG0(TRUE, 5, 1032);
		RETURN(NETIPC_BAD_UDP_CHECKSUM);
	    }
	    else {
		RETURN(RCV_SUCCESS);
	    }
	}
	else {
	    RETURN(RCV_SUCCESS);
	}
    }

END


/*
 * netipc_send
 *	Calculate a UDP checksum for a packet if the packet is not encrypted.
 *	Insert a new ip_id into the IP header.
 *	Send the packet over the network using msg_send.
 *
 * Parameters:
 *	pkt_ptr	: pointer to the packet to be sent.
 *
 * Results:
 *	value returned by msg_send.
 *
 */
EXPORT netipc_send(pkt_ptr)
register netipc_header_ptr_t	pkt_ptr;
BEGIN("netipc_send")
    register short		saved_ttl;
    register msg_return_t	mr;

    if (pkt_ptr->nih_crypt_header.ch_crypt_level == CRYPT_DONT_ENCRYPT) {
	/*
	* Stuff the ip header with the right values for the UDP checksum.
	*/
	saved_ttl = pkt_ptr->nih_ip_header.ip_ttl;
	pkt_ptr->nih_ip_header.ip_ttl = 0;
	pkt_ptr->nih_ip_header.ip_sum = pkt_ptr->nih_udp_header.uh_ulen;
	pkt_ptr->nih_udp_header.uh_sum = 0;

	/*
	 * Calculate the checksum and restore the values in the ip header.
	 */
	pkt_ptr->nih_udp_header.uh_sum = udp_checksum((unsigned short *)&(pkt_ptr->nih_ip_header.ip_ttl),
							(pkt_ptr->nih_ip_header.ip_len - 8));
	pkt_ptr->nih_ip_header.ip_ttl = saved_ttl;
	pkt_ptr->nih_ip_header.ip_sum = 0;
    }
    else {
	/*
	 * Tell the destination to ignore the UDP Checksum.
	 */
	pkt_ptr->nih_udp_header.uh_sum = 0;
    }

    /*
     * Insert a new ip id into the header and send the packet.
     */
    pkt_ptr->nih_ip_header.ip_id = last_ip_id ++;
    DEBUG4(NETIPC_DEBUG, 0, 1030, pkt_ptr->nih_msg_header.msg_simple, pkt_ptr->nih_msg_header.msg_size,
			pkt_ptr->nih_msg_header.msg_id, pkt_ptr->nih_msg_header.msg_type);
    if ((mr = msg_send((msg_header_t *)pkt_ptr, SEND_TIMEOUT, 0)) != KERN_SUCCESS) {
	DEBUG1(NETIPC_DEBUG, 0, 1031, mr);
    }
    RETURN(mr);

END

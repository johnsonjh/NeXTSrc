/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * crypt.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/crypt.c,v $
 *
 */

#ifndef	lint
char crypt_rcsid[] = "$Header: crypt.c,v 1.1 88/09/30 15:38:23 osdev Exp $";
#endif not lint

/*
 * Functions performing basic encryption and decryption of packets.
 */

/*
 * HISTORY:
 * 21-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added DES encryption/decryption functions.
 *
 * 14-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added encryption statistics.
 *
 * 25-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Re-encrypt a failed and decrypted packet so that the remote
 *	transport module can handle the crypt failure gracefully.
 *
 *  5-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added crypt_functions - array of encryption/decryption functions.
 *	Call km_get_dkey from crypt_decrypt_packet.
 *
 * 28-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	crypt_algorithm is now obtained from the param record.
 *
 * 17-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Replaced fprintf by ERROR macro.
 *
 * 24-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include <mach_types.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "crypt.h"
#include "crypt_defs.h"
#include "debug.h"
#include "key_defs.h"
#include "ls_defs.h"
#include "netipc.h"
#include "netmsg.h"
#include "nm_defs.h"

int crypt_null() {}

#if	USE_CRYPT_XOR
#else	USE_CRYPT_XOR
#define encrypt_xor	crypt_null
#define decrypt_xor	crypt_null
#endif	USE_CRYPT_XOR

#if	USE_CRYPT_NEWDES
#else	USE_CRYPT_NEWDES
#define encrypt_newdes	crypt_null
#define decrypt_newdes	crypt_null
#endif	USE_CRYPT_NEWDES

#if	USE_CRYPT_MULTPERM
#else	USE_CRYPT_MULTPERM
#define encrypt_multperm	crypt_null
#define decrypt_multperm	crypt_null
#endif	USE_CRYPT_MULTPERM

#if	USE_CRYPT_DES
#else	USE_CRYPT_DES
#define encrypt_des	crypt_null
#define decrypt_des	crypt_null
#endif	USE_CRYPT_DES

crypt_function_t	crypt_functions[] = {	{crypt_null, crypt_null},
						{(int (*)())encrypt_xor, (int (*)())decrypt_xor},
						{(int (*)())encrypt_newdes, (int (*)())decrypt_newdes},
						{(int (*)())encrypt_multperm, (int (*)())decrypt_multperm},
						{(int (*)())encrypt_des, (int (*)())decrypt_des}
};



/*
 * crypt_encrypt_packet
 *	Encrypts the data in a packet.
 *
 * Parameters:
 *	packet_ptr	: pointer to packet which is to be encrypted
 *	crypt_level	: level at which this packet is to be encrypted
 *
 * Results:
 *	CRYPT_SUCCESS or CRYPT_FAILURE
 *
 * Design:
 *	Pad out data with zero bytes to a multiple of eight bytes.
 *	Calculate the checksum of the packet data.
 *	Look up the key for the destination host of this packet.
 *	Encrypt the checksum and the packet data using the key.
 *
 * Note:
 *	We should have a key for the host at this point.
 *
 */
EXPORT crypt_encrypt_packet(packet_ptr, crypt_level)
netipc_ptr_t	packet_ptr;
int		crypt_level;
BEGIN("crypt_encrypt_packet")
    int			udp_size, crypt_size, data_size;
    int			bytes_to_pad;
    char		*pad_ptr;
    key_t		key;

    /*
     * Round up the size of the encrypted data to a multiple of 8 bytes.
     * Note that the last two shorts of the crypt header are also encrypted.
     */
    data_size = ntohs(packet_ptr->ni_header.nih_crypt_header.ch_data_size);
    bytes_to_pad = crypt_size = data_size + (2 * (sizeof(short)));
    crypt_size = (crypt_size + 7) & (~(07));
    bytes_to_pad = crypt_size - bytes_to_pad;
    udp_size = crypt_size + sizeof(struct udphdr) + sizeof(long);
    packet_ptr->ni_header.nih_udp_header.uh_ulen = htons((unsigned short)udp_size);
    packet_ptr->ni_header.nih_ip_header.ip_len += bytes_to_pad;
    packet_ptr->ni_header.nih_msg_header.msg_size += bytes_to_pad;

    /*
     * Pad out the data with zero bytes.
     */
    pad_ptr = (char *)&(packet_ptr->ni_header.nih_crypt_header.ch_checksum);
    while (bytes_to_pad--) {
	pad_ptr[crypt_size - bytes_to_pad] = 0;
    }

    /*
     * Calculate the checksum.
     */
    packet_ptr->ni_header.nih_crypt_header.ch_checksum = 0;
    packet_ptr->ni_header.nih_crypt_header.ch_checksum =
	udp_checksum((unsigned short *)&(packet_ptr->ni_header.nih_crypt_header.ch_crypt_level),
			(crypt_size + sizeof(long)));

    if (crypt_level == CRYPT_DONT_ENCRYPT) {
	RETURN(CRYPT_SUCCESS);
    }

    /*
     * Look up the encryption key.
     */
    if (!(km_get_key(packet_ptr->ni_header.nih_ip_header.ip_dst.s_addr ,&key))) {
	ERROR((msg, "crypt_encrypt_packet.km_get_key fails, host id = %x.",
			packet_ptr->ni_header.nih_ip_header.ip_dst.s_addr));
	RETURN(CRYPT_FAILURE);
    }

    /*
     * Now do the encryption.
     */
    if (CHECK_ENCRYPT_ALGORITHM(param.crypt_algorithm)) {
	(void)crypt_functions[param.crypt_algorithm].encrypt
		(key, (pointer_t)&(packet_ptr->ni_header.nih_crypt_header.ch_checksum), crypt_size);
	INCSTAT(pkts_encrypted);
	RETURN(CRYPT_SUCCESS);
    }
    else {
	ERROR((msg, "crypt_encrypt_packet: illegal encryption algorithm (%d).", param.crypt_algorithm));
	RETURN(CRYPT_FAILURE);
    }

END



/*
 * crypt_decrypt_packet
 *	Decrypts the data in a packet.
 *
 * Parameters:
 *	packet_ptr	: pointer to packet which is to be decrypted
 *	crypt_level	: level at which this packet is to be decrypted
 *
 * Results:
 *	CRYPT_SUCCESS, CRYPT_FAILURE or CRYPT_CHECKSUM_FAILURE.
 *
 * Design:
 *	Look up the key for the destination host of this packet.
 *	Decrypt the checksum and the packet data using the key.
 *	Check the checksum of the packet data.
 *
 * Note:
 *	We should have a key for the host at this point.
 *
 */
EXPORT crypt_decrypt_packet(packet_ptr, crypt_level)
netipc_ptr_t	packet_ptr;
int		crypt_level;
BEGIN("crypt_decrypt_packet")
    int		udp_size;
    int		crypt_size;
    key_t	key;

    udp_size = ntohs((unsigned short)packet_ptr->ni_header.nih_udp_header.uh_ulen);
    crypt_size = udp_size - (sizeof(struct udphdr) + sizeof(long));

    if (crypt_level == CRYPT_DONT_ENCRYPT) {
	RETURN(CRYPT_SUCCESS);
    }

    /*
     * Look up the decryption key.
     */
    if (!(km_get_dkey(packet_ptr->ni_header.nih_ip_header.ip_src.s_addr ,&key))) {
	ERROR((msg, "crypt_decrypt_packet.km_get_dkey fails, host id = %x.",
				packet_ptr->ni_header.nih_ip_header.ip_src.s_addr));
	RETURN(CRYPT_FAILURE);
    }

    /*
     * Now do the decryption.
     */
    if (CHECK_DECRYPT_ALGORITHM(param.crypt_algorithm)) {
	(void)crypt_functions[param.crypt_algorithm].decrypt
		(key, (pointer_t)&(packet_ptr->ni_header.nih_crypt_header.ch_checksum), crypt_size);
    }
    else {
	ERROR((msg, "crypt_decrypt_packet: illegal decryption algorithm (%d).", param.crypt_algorithm));
	RETURN(CRYPT_FAILURE);
    }

    /*
     * Lastly, check the checksum.
     * If all is well the checksum algorithm should return zero.
     */

    if ((udp_checksum((unsigned short *)&(packet_ptr->ni_header.nih_crypt_header.ch_crypt_level),
			(crypt_size + sizeof(long)))) != 0)
    {
	ERROR((msg, "crypt_decrypt_packet.udp_checksum fails, host id = %x.",
				packet_ptr->ni_header.nih_ip_header.ip_src.s_addr));
	/*
	 * Re-encrypt the packet so that the transport modules
	 * can handle the remote crypt failure gracefully.
	 */
	if (!(km_get_key(packet_ptr->ni_header.nih_ip_header.ip_src.s_addr ,&key))) {
	    ERROR((msg, "crypt_decrypt_packet.km_get_key fails, host id = %x.",
			packet_ptr->ni_header.nih_ip_header.ip_dst.s_addr));
	    RETURN(CRYPT_FAILURE);
	}
	if (CHECK_ENCRYPT_ALGORITHM(param.crypt_algorithm)) {
	    (void)crypt_functions[param.crypt_algorithm].encrypt
		(key, (pointer_t)&(packet_ptr->ni_header.nih_crypt_header.ch_checksum), crypt_size);
	}
	else {
	    ERROR((msg, "crypt_decrypt_packet: illegal encryption algorithm (%d).", param.crypt_algorithm));
	    RETURN(CRYPT_FAILURE);
	}
	RETURN(CRYPT_CHECKSUM_FAILURE);
    }

    INCSTAT(pkts_decrypted);
    RETURN(CRYPT_SUCCESS);

END

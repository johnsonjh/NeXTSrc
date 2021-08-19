/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * crypt_multperm.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/crypt_multperm.c,v $
 *
 */

#ifndef	lint
char crypt_multperm_rcsid[] = "$Header: crypt_multperm.c,v 1.1 88/09/30 15:38:30 osdev Exp $";
#endif not lint

/*
 * Packet encryption and decryption using multperm encryption algorithm
 *	(with cipher block chaining).
 */

/*
 * HISTORY:
 * 09-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Make conditional.
 *
 * 19-Sep-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include <config.h>

#if	USE_MULTPERM

#include <mach_types.h>


#include "crypt_defs.h"
#include "key_defs.h"
#include "multperm.h"


/*
 * encrypt_multperm
 *	encrypt data using the mult-perm algorithm and cipher-block chaining.
 *
 * Parameters:
 *	key		: the key to use for encryption.
 *	data_ptr	: pointer to data to be encrypted.
 *	data_size	: number of bytes to be encrypted.
 *
 * Notes:
 *	data_size should be a multiple of eight bytes.
 *	The fourth key word is used to initialise the block chaining.
 *
 */
void encrypt_multperm(key, data_ptr, data_size)
key_t		key;
pointer_t	data_ptr;
int		data_size;
{
    int				blocks;
    unsigned int		encryptkey0, encryptkey1, encryptkey2;
    register unsigned int	plong0, plong1;
    register mp_block_ptr_t	encryptblock;

    encryptkey0 = key.key_longs[0];
    encryptkey1 = key.key_longs[1];
    encryptkey2 = key.key_longs[2];
    /*
     * Set initial value of cipher block text to be the fourth key word.
     */
    plong0 = plong1 = key.key_longs[3];

    encryptblock = (mp_block_ptr_t)data_ptr;
    blocks = data_size / 8;

    while (blocks--) {
	register unsigned int a0, a1;
	unsigned int b0, b1;

	a0 = mult((encryptblock->mp_block_high ^ plong0), encryptkey0);
	a1 = mult((encryptblock->mp_block_low ^ plong1), encryptkey2);
	permute (&b0, &b1, a0, a1);
	a0 = mult (b0, encryptkey1);
	a1 = mult (b1, encryptkey1);
	permute (&b0, &b1, a0, a1);
	encryptblock->mp_block_high = (plong0 = mult (b0, encryptkey2));
	encryptblock->mp_block_low = (plong1 = mult (b1, encryptkey0));

	encryptblock ++;
    }
}



/*
 * decrypt_multperm
 *	Decrypt data using the mult-perm algorithm and cipher-block chaining.
 *
 * Parameters:
 *	key		: the key to use for decryption.
 *	data_ptr	: pointer to the data to be decrypted.
 *	data_size	: the number of bytes to be decrypted.
 *
 * Notes:
 *	the data_size should be a multiple of eight bytes.
 *	The key should already be inverted.
 *
 */
void decrypt_multperm (key, data_ptr, data_size)
key_t		key;
pointer_t	data_ptr;
int		data_size;
{
    int				blocks;
    unsigned int		decryptkey0, decryptkey1, decryptkey2;
    register unsigned int	plong0, plong1, temp0, temp1;
    register mp_block_ptr_t	decryptblock;

    decryptkey0 = key.key_longs[2];
    decryptkey1 = key.key_longs[1];
    decryptkey2 = key.key_longs[0];
    /*
     * Set initial value of cipher block text to be the fourth key word.
     */
    plong0 = plong1 = key.key_longs[3];

    decryptblock = (mp_block_ptr_t)data_ptr;
    blocks = data_size / 8;

    while (blocks--) {
	register unsigned int a0, a1;
	unsigned int b0, b1;

	temp0 = decryptblock->mp_block_high;
	temp1 = decryptblock->mp_block_low;
	a0 = mult (temp0, decryptkey0);
	a1 = mult (temp1, decryptkey2);
	permute (&b0, &b1, a0, a1);
	a0 = mult (b0, decryptkey1);
	a1 = mult (b1, decryptkey1);
	permute (&b0, &b1, a0, a1);
	decryptblock->mp_block_high = plong0 ^ mult (b0, decryptkey2);
	decryptblock->mp_block_low = plong1 ^ mult (b1, decryptkey0);
	plong0 = temp0;
	plong1 = temp1;

	decryptblock ++;
    }
}

#endif	USE_MULTPERM

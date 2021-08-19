/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * crypt_newdes.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/crypt_newdes.c,v $
 *
 */

#ifndef	lint
char crypt_newdes_rcsid[] = "$Header: crypt_newdes.c,v 1.1 88/09/30 15:38:34 osdev Exp $";
#endif not lint

/*
 * Data encryption and decryption using newdes encryption algorithm
 *	and cipher block chaining.
 * Algorithm is from:
 *	"Wide-Open Encryption Design offers flexible Implementations"
 *		by Robert Scott, Cryptologia January 1985.
 *
 */

/*
 * HISTORY:
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Changed two #include to use quotes instead of brackets.
 *
 * 15-Sep-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */


#include <config.h>

#if	USE_NEWDES

#include <mach_types.h>

#include "crypt_defs.h"
#include "key_defs.h"


typedef unsigned int newdes_key_t[15];
typedef struct {
    unsigned char bytes[8];
} newdes_block_t, *newdes_block_ptr_t;


static int farray[256] = {
    32, 137, 239, 188, 102, 125, 221, 72, 212, 68, 81, 37, 86, 237, 147, 149,
    70, 229, 17, 124, 115, 207, 33, 20, 122, 143, 25, 215, 51, 183, 138, 142,
    146, 211, 110, 173, 1, 228, 189, 14, 103, 78, 162, 36, 253, 167, 116, 255,
    158, 45, 185, 50, 98, 168, 250, 235, 54, 141, 195, 247, 240, 63, 148, 2,
    224, 169, 214, 180, 62, 22, 117, 108, 19, 172, 161, 159, 160, 47, 43, 171,
    194, 175, 178, 56, 196, 112, 23, 220, 89, 21, 164, 130, 157, 8, 85, 251,
    216, 44, 94, 179, 226, 38, 90, 119, 40, 202, 34, 206, 35, 69, 231, 246,
    29, 109, 74, 71, 176, 6, 60, 145, 65, 13, 77, 151, 12, 127, 95, 199,
    57, 101, 5, 232, 150, 210, 129, 24, 181, 10, 121, 187, 48, 193, 139, 252,
    219, 64, 88, 233, 96, 128, 80, 53, 191, 144, 218, 11, 106, 132, 155, 104,
    91, 136, 31, 42, 243, 66, 126, 135, 30, 26, 87, 186, 182, 154, 242, 123,
    82, 166, 208, 39, 152, 190, 113, 205, 114, 105, 225, 84, 73, 163, 99, 111,
    204, 61, 200, 217, 170, 15, 198, 28, 192, 254, 134, 234, 222, 7, 236, 248,
    201, 41, 177, 156, 92, 131, 67, 249, 245, 184, 203, 9, 241, 0, 27, 46,
    133, 174, 75, 18, 93, 209, 100, 120, 76, 213, 16, 83, 4, 107, 140, 52,
    58, 55, 3, 244, 97, 197, 238, 227, 118, 49, 79, 230, 223, 165, 153, 59
};



/*
 * encrypt_newdes
 *	encrypt data using the newdes algorithm with cipher block chaining.
 *
 * Parameters:
 *	key		: the key to use for encryption.
 *	data_ptr	: pointer to data to be encrypted.
 *	data_size	: number of bytes to be encrypted.
 *
 * Note:
 *	data_size should be a multiple of eight bytes.
 *
 */
void encrypt_newdes (key, data_ptr, data_size)
key_t		key;
pointer_t	data_ptr;
int		data_size;
{
    int				blocks, i;
    newdes_block_ptr_t		encrypt_block;
#ifdef	lint
    newdes_key_t		encrypt_key;
#else	lint
    register newdes_key_t	encrypt_key;
#endif	lint
    register unsigned int	b4, b5, b0, b1, b2, b3, b6, b7;
    
    for (i = 0; i < 15; i++) encrypt_key[i] = key.key_bytes[i];
    blocks = data_size / 8;
    encrypt_block = (newdes_block_ptr_t)data_ptr;

    /*
     * Set initial value of cipher text to be the first 8 bytes of the key.
     */
    b0 = encrypt_key[0];
    b1 = encrypt_key[1];
    b2 = encrypt_key[2];
    b3 = encrypt_key[3];
    b4 = encrypt_key[4];
    b5 = encrypt_key[5];
    b6 = encrypt_key[6];
    b7 = encrypt_key[7];

    while (blocks--) {
	/*
	 * XOR in the previous encrypted block.
	 */
	b0 ^= encrypt_block->bytes[0];
	b1 ^= encrypt_block->bytes[1];
	b2 ^= encrypt_block->bytes[2];
	b3 ^= encrypt_block->bytes[3];
	b4 ^= encrypt_block->bytes[4];
	b5 ^= encrypt_block->bytes[5];
	b6 ^= encrypt_block->bytes[6];
	b7 ^= encrypt_block->bytes[7];

	b4 ^= farray[(b0 ^ encrypt_key[0])];
	b5 ^= farray[(b1 ^ encrypt_key[1])];
	b6 ^= farray[(b2 ^ encrypt_key[2])];
	b7 ^= farray[(b3 ^ encrypt_key[3])];
	b1 ^= farray[(b4 ^ encrypt_key[4])];
	b2 ^= farray[(b4 ^ b5)];
	b3 ^= farray[(b6 ^ encrypt_key[5])];
	b0 ^= farray[(b7 ^ encrypt_key[6])];
	b4 ^= farray[(b0 ^ encrypt_key[7])];
	b5 ^= farray[(b1 ^ encrypt_key[8])];
	b6 ^= farray[(b2 ^ encrypt_key[9])];
	b7 ^= farray[(b3 ^ encrypt_key[10])];
	b1 ^= farray[(b4 ^ encrypt_key[11])];
	b2 ^= farray[(b4 ^ b5)];
	b3 ^= farray[(b6 ^ encrypt_key[12])];
	b0 ^= farray[(b7 ^ encrypt_key[13])];
	b4 ^= farray[(b0 ^ encrypt_key[14])];
	b5 ^= farray[(b1 ^ encrypt_key[0])];
	b6 ^= farray[(b2 ^ encrypt_key[1])];
	b7 ^= farray[(b3 ^ encrypt_key[2])];
	b1 ^= farray[(b4 ^ encrypt_key[3])];
	b2 ^= farray[(b4 ^ b5)];
	b3 ^= farray[(b6 ^ encrypt_key[4])];
	b0 ^= farray[(b7 ^ encrypt_key[5])];
	b4 ^= farray[(b0 ^ encrypt_key[6])];
	b5 ^= farray[(b1 ^ encrypt_key[7])];
	b6 ^= farray[(b2 ^ encrypt_key[8])];
	b7 ^= farray[(b3 ^ encrypt_key[9])];
	b1 ^= farray[(b4 ^ encrypt_key[10])];
	b2 ^= farray[(b4 ^ b5)];
	b3 ^= farray[(b6 ^ encrypt_key[11])];
	b0 ^= farray[(b7 ^ encrypt_key[12])];
	b4 ^= farray[(b0 ^ encrypt_key[13])];
	b5 ^= farray[(b1 ^ encrypt_key[14])];
	b6 ^= farray[(b2 ^ encrypt_key[0])];
	b7 ^= farray[(b3 ^ encrypt_key[1])];
	b1 ^= farray[(b4 ^ encrypt_key[2])];
	b2 ^= farray[(b4 ^ b5)];
	b3 ^= farray[(b6 ^ encrypt_key[3])];
	b0 ^= farray[(b7 ^ encrypt_key[4])];
	b4 ^= farray[(b0 ^ encrypt_key[5])];
	b5 ^= farray[(b1 ^ encrypt_key[6])];
	b6 ^= farray[(b2 ^ encrypt_key[7])];
	b7 ^= farray[(b3 ^ encrypt_key[8])];
	b1 ^= farray[(b4 ^ encrypt_key[9])];
	b2 ^= farray[(b4 ^ b5)];
	b3 ^= farray[(b6 ^ encrypt_key[10])];
	b0 ^= farray[(b7 ^ encrypt_key[11])];
	b4 ^= farray[(b0 ^ encrypt_key[12])];
	b5 ^= farray[(b1 ^ encrypt_key[13])];
	b6 ^= farray[(b2 ^ encrypt_key[14])];
	b7 ^= farray[(b3 ^ encrypt_key[0])];
	b1 ^= farray[(b4 ^ encrypt_key[1])];
	b2 ^= farray[(b4 ^ b5)];
	b3 ^= farray[(b6 ^ encrypt_key[2])];
	b0 ^= farray[(b7 ^ encrypt_key[3])];
	b4 ^= farray[(b0 ^ encrypt_key[4])];
	b5 ^= farray[(b1 ^ encrypt_key[5])];
	b6 ^= farray[(b2 ^ encrypt_key[6])];
	b7 ^= farray[(b3 ^ encrypt_key[7])];
	b1 ^= farray[(b4 ^ encrypt_key[8])];
	b2 ^= farray[(b4 ^ b5)];
	b3 ^= farray[(b6 ^ encrypt_key[9])];
	b0 ^= farray[(b7 ^ encrypt_key[10])];
	b4 ^= farray[(b0 ^ encrypt_key[11])];
	b5 ^= farray[(b1 ^ encrypt_key[12])];
	b6 ^= farray[(b2 ^ encrypt_key[13])];
	b7 ^= farray[(b3 ^ encrypt_key[14])];

	encrypt_block->bytes[0] = b0;
	encrypt_block->bytes[1] = b1;
	encrypt_block->bytes[2] = b2;
	encrypt_block->bytes[3] = b3;
	encrypt_block->bytes[4] = b4;
	encrypt_block->bytes[5] = b5;
	encrypt_block->bytes[6] = b6;
	encrypt_block->bytes[7] = b7;

	encrypt_block ++;
    }
}



/*
 * decrypt_newdes
 *	decrypt data using the newdes algorithm and cipher block chaining.
 *
 * Parameters:
 *	key		: the key to use for decryption.
 *	data_ptr	: pointer to the data to be decrypted.
 *	data_size	: the number of bytes to be decrypted.
 *
 * Note:
 *	the data_size should be a multiple of eight bytes.
 *
 */
void decrypt_newdes(key, data_ptr, data_size)
key_t		key;
pointer_t	data_ptr;
int		data_size;
{
    int				blocks, even, i;
    newdes_block_ptr_t		decrypt_block;
#ifdef	lint
    newdes_key_t		decrypt_key;
#else	lint
    register newdes_key_t	decrypt_key;
#endif	lint
    register unsigned int	b4, b5, b0, b1, b2, b3, b6, b7;
    unsigned int		pb0o, pb1o, pb2o, pb3o, pb4o, pb5o, pb6o, pb7o;
    unsigned int		pb0e, pb1e, pb2e, pb3e, pb4e, pb5e, pb6e, pb7e;
    
    for (i = 0; i < 15; i++) decrypt_key[i] = key.key_bytes[i];
    decrypt_block = (newdes_block_ptr_t)data_ptr;
    even = TRUE;
    blocks = data_size / 8;

    /*
     * Set up the previous bytes for cipher block chaining.
     */
    pb0o = decrypt_key[0];
    pb1o = decrypt_key[1];
    pb2o = decrypt_key[2];
    pb3o = decrypt_key[3];
    pb4o = decrypt_key[4];
    pb5o = decrypt_key[5];
    pb6o = decrypt_key[6];
    pb7o = decrypt_key[7];

    while (blocks--) {
	b0 = decrypt_block->bytes[0];
	b1 = decrypt_block->bytes[1];
	b2 = decrypt_block->bytes[2];
	b3 = decrypt_block->bytes[3];
	b4 = decrypt_block->bytes[4];
	b5 = decrypt_block->bytes[5];
	b6 = decrypt_block->bytes[6];
	b7 = decrypt_block->bytes[7];

	if (even) {
	    pb0e = b0;
	    pb1e = b1;
	    pb2e = b2;
	    pb3e = b3;
	    pb4e = b4;
	    pb5e = b5;
	    pb6e = b6;
	    pb7e = b7;
	} else {
	    pb0o = b0;
	    pb1o = b1;
	    pb2o = b2;
	    pb3o = b3;
	    pb4o = b4;
	    pb5o = b5;
	    pb6o = b6;
	    pb7o = b7;
	}

	b4 ^=  farray[(b0 ^ decrypt_key[11])];
	b5 ^=  farray[(b1 ^ decrypt_key[12])];
	b6 ^=  farray[(b2 ^ decrypt_key[13])];
	b7 ^=  farray[(b3 ^ decrypt_key[14])];
	b1 ^=  farray[(b4 ^ decrypt_key[8])];
	b2 ^=  farray[(b4 ^ b5)];
	b3 ^=  farray[(b6 ^ decrypt_key[9])];
	b0 ^=  farray[(b7 ^ decrypt_key[10])];
	b4 ^=  farray[(b0 ^ decrypt_key[4])];
	b5 ^=  farray[(b1 ^ decrypt_key[5])];
	b6 ^=  farray[(b2 ^ decrypt_key[6])];
	b7 ^=  farray[(b3 ^ decrypt_key[7])];
	b1 ^=  farray[(b4 ^ decrypt_key[1])];
	b2 ^=  farray[(b4 ^ b5)];
	b3 ^=  farray[(b6 ^ decrypt_key[2])];
	b0 ^=  farray[(b7 ^ decrypt_key[3])];
	b4 ^=  farray[(b0 ^ decrypt_key[12])];
	b5 ^=  farray[(b1 ^ decrypt_key[13])];
	b6 ^=  farray[(b2 ^ decrypt_key[14])];
	b7 ^=  farray[(b3 ^ decrypt_key[0])];
	b1 ^=  farray[(b4 ^ decrypt_key[9])];
	b2 ^=  farray[(b4 ^ b5)];
	b3 ^=  farray[(b6 ^ decrypt_key[10])];
	b0 ^=  farray[(b7 ^ decrypt_key[11])];
	b4 ^=  farray[(b0 ^ decrypt_key[5])];
	b5 ^=  farray[(b1 ^ decrypt_key[6])];
	b6 ^=  farray[(b2 ^ decrypt_key[7])];
	b7 ^=  farray[(b3 ^ decrypt_key[8])];
	b1 ^=  farray[(b4 ^ decrypt_key[2])];
	b2 ^=  farray[(b4 ^ b5)];
	b3 ^=  farray[(b6 ^ decrypt_key[3])];
	b0 ^=  farray[(b7 ^ decrypt_key[4])];
	b4 ^=  farray[(b0 ^ decrypt_key[13])];
	b5 ^=  farray[(b1 ^ decrypt_key[14])];
	b6 ^=  farray[(b2 ^ decrypt_key[0])];
	b7 ^=  farray[(b3 ^ decrypt_key[1])];
	b1 ^=  farray[(b4 ^ decrypt_key[10])];
	b2 ^=  farray[(b4 ^ b5)];
	b3 ^=  farray[(b6 ^ decrypt_key[11])];
	b0 ^=  farray[(b7 ^ decrypt_key[12])];
	b4 ^=  farray[(b0 ^ decrypt_key[6])];
	b5 ^=  farray[(b1 ^ decrypt_key[7])];
	b6 ^=  farray[(b2 ^ decrypt_key[8])];
	b7 ^=  farray[(b3 ^ decrypt_key[9])];
	b1 ^=  farray[(b4 ^ decrypt_key[3])];
	b2 ^=  farray[(b4 ^ b5)];
	b3 ^=  farray[(b6 ^ decrypt_key[4])];
	b0 ^=  farray[(b7 ^ decrypt_key[5])];
	b4 ^=  farray[(b0 ^ decrypt_key[14])];
	b5 ^=  farray[(b1 ^ decrypt_key[0])];
	b6 ^=  farray[(b2 ^ decrypt_key[1])];
	b7 ^=  farray[(b3 ^ decrypt_key[2])];
	b1 ^=  farray[(b4 ^ decrypt_key[11])];
	b2 ^=  farray[(b4 ^ b5)];
	b3 ^=  farray[(b6 ^ decrypt_key[12])];
	b0 ^=  farray[(b7 ^ decrypt_key[13])];
	b4 ^=  farray[(b0 ^ decrypt_key[7])];
	b5 ^=  farray[(b1 ^ decrypt_key[8])];
	b6 ^=  farray[(b2 ^ decrypt_key[9])];
	b7 ^=  farray[(b3 ^ decrypt_key[10])];
	b1 ^=  farray[(b4 ^ decrypt_key[4])];
	b2 ^=  farray[(b4 ^ b5)];
	b3 ^=  farray[(b6 ^ decrypt_key[5])];
	b0 ^=  farray[(b7 ^ decrypt_key[6])];
	b4 ^=  farray[(b0 ^ decrypt_key[0])];
	b5 ^=  farray[(b1 ^ decrypt_key[1])];
	b6 ^=  farray[(b2 ^ decrypt_key[2])];
	b7 ^=  farray[(b3 ^ decrypt_key[3])];

	if (even) {
	    decrypt_block->bytes[0] = b0 ^ pb0o;
	    decrypt_block->bytes[1] = b1 ^ pb1o;
	    decrypt_block->bytes[2] = b2 ^ pb2o;
	    decrypt_block->bytes[3] = b3 ^ pb3o;
	    decrypt_block->bytes[4] = b4 ^ pb4o;
	    decrypt_block->bytes[5] = b5 ^ pb5o;
	    decrypt_block->bytes[6] = b6 ^ pb6o;
	    decrypt_block->bytes[7] = b7 ^ pb7o;
	    even = FALSE;
	} else {
	    decrypt_block->bytes[0] = b0 ^ pb0e;
	    decrypt_block->bytes[1] = b1 ^ pb1e;
	    decrypt_block->bytes[2] = b2 ^ pb2e;
	    decrypt_block->bytes[3] = b3 ^ pb3e;
	    decrypt_block->bytes[4] = b4 ^ pb4e;
	    decrypt_block->bytes[5] = b5 ^ pb5e;
	    decrypt_block->bytes[6] = b6 ^ pb6e;
	    decrypt_block->bytes[7] = b7 ^ pb7e;
	    even = TRUE;
	}
	decrypt_block ++;
    }
}

#endif	USE_NEWDES

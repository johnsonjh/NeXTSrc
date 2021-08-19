/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * multperm.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/multperm.c,v $
 *
 */

#ifndef	lint
char multperm_rcsid[] = "$Header: multperm.c,v 1.1 88/09/30 15:40:25 osdev Exp $";
#endif not lint

/*
 *	Encryption and decryption using multperm encryption algorithm,
 *	    a multiplication-substitution encryption function from:
 *		"Multiplication-Permutation Encryption Network"
 *		by Hank Meijer, Queen's University of Ontario, Technical Report 85-171.
 */

/*
 * HISTORY:
 * 09-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Make conditional.
 *
 * 13-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Fixed up for the new network server.
 *
 * 15-Jan-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */


#include "config.h"

#if	USE_MULTPERM

#include "key_defs.h"
#include "multperm.h"



/*
 * ADD
 *	addition MOD 2^32+1.
 *
 */
#define ADD(result,a,b) {result = ((((a) + (b)) < (a)) ? ((a) + (b) + 1) : ((a) + (b)));}

/*
 * mult
 *	Multiplication MOD 2^32+1.
 *
 */
unsigned int mult(a, b)
unsigned int	a,b;
{
    register unsigned int ha, hb, la, lb, temp1, temp2;

    ha = a >> 16;
    la = a & 0xffff;
    hb = b >> 16;
    lb = b & 0xffff;
    temp1 = ha * hb;
    temp2 = la * lb;
    ADD(temp1, temp1, temp2);
    ha = ha * lb;
    hb = hb * la;
    ADD(temp2, ha, hb);
    temp2 = (temp2 >> 16) | (temp2 << 16);
    ADD(temp2, temp1, temp2)
    return (temp2);
}



/*
 * permute
 *	permute the pair <in0,in1> and put the result in <out0,out1>.
 *
 */
void permute(out0, out1, in0, in1)
unsigned int	*out0, *out1, in0, in1;
{
    register int temp0, temp1, rin0, rin1;

    rin0 = in0;
    rin1 = in1;
    temp0 = 0;
    temp0 |= ((rin0 >> 16) & 01) << 0;
    temp0 |= ((rin1 >> 4) & 01) << 1;
    temp0 |= ((rin1 >> 16) & 01) << 2;
    temp0 |= ((rin0 >> 13) & 01) << 3;
    temp0 |= ((rin0 >> 20) & 01) << 4;
    temp0 |= ((rin1 >> 3) & 01) << 5;
    temp0 |= ((rin1 >> 19) & 01) << 6;
    temp0 |= ((rin0 >> 12) & 01) << 7;
    temp0 |= ((rin0 >> 29) & 01) << 8;
    temp0 |= ((rin1 >> 0) & 01) << 9;
    temp0 |= ((rin0 >> 14) & 01) << 10;
    temp0 |= ((rin0 >> 27) & 01) << 11;
    temp0 |= ((rin0 >> 7) & 01) << 12;
    temp0 |= ((rin0 >> 3) & 01) << 13;
    temp0 |= ((rin0 >> 10) & 01) << 14;
    temp0 |= ((rin1 >> 1) & 01) << 15;
    temp0 |= ((rin0 >> 0) & 01) << 16;
    temp0 |= ((rin1 >> 5) & 01) << 17;
    temp0 |= ((rin1 >> 14) & 01) << 18;
    temp0 |= ((rin0 >> 21) & 01) << 19;
    temp0 |= ((rin0 >> 4) & 01) << 20;
    temp0 |= ((rin0 >> 19) & 01) << 21;
    temp0 |= ((rin1 >> 13) & 01) << 22;
    temp0 |= ((rin1 >> 23) & 01) << 23;
    temp0 |= ((rin0 >> 26) & 01) << 24;
    temp0 |= ((rin1 >> 26) & 01) << 25;
    temp0 |= ((rin0 >> 24) & 01) << 26;
    temp0 |= ((rin0 >> 11) & 01) << 27;
    temp0 |= ((rin1 >> 27) & 01) << 28;
    temp0 |= ((rin0 >> 8) & 01) << 29;
    temp0 |= ((rin1 >> 25) & 01) << 30;
    temp0 |= ((rin1 >> 24) & 01) << 31;

    temp1 = 0;
    temp1 |= ((rin0 >> 9) & 01) << 0;
    temp1 |= ((rin0 >> 15) & 01) << 1;
    temp1 |= ((rin1 >> 7) & 01) << 2;
    temp1 |= ((rin0 >> 5) & 01) << 3;
    temp1 |= ((rin0 >> 1) & 01) << 4;
    temp1 |= ((rin0 >> 17) & 01) << 5;
    temp1 |= ((rin1 >> 29) & 01) << 6;
    temp1 |= ((rin1 >> 2) & 01) << 7;
    temp1 |= ((rin1 >> 18) & 01) << 8;
    temp1 |= ((rin1 >> 30) & 01) << 9;
    temp1 |= ((rin1 >> 17) & 01) << 10;
    temp1 |= ((rin1 >> 22) & 01) << 11;
    temp1 |= ((rin1 >> 21) & 01) << 12;
    temp1 |= ((rin0 >> 22) & 01) << 13;
    temp1 |= ((rin0 >> 18) & 01) << 14;
    temp1 |= ((rin1 >> 28) & 01) << 15;
    temp1 |= ((rin0 >> 2) & 01) << 16;
    temp1 |= ((rin1 >> 10) & 01) << 17;
    temp1 |= ((rin1 >> 8) & 01) << 18;
    temp1 |= ((rin0 >> 6) & 01) << 19;
    temp1 |= ((rin1 >> 31) & 01) << 20;
    temp1 |= ((rin1 >> 12) & 01) << 21;
    temp1 |= ((rin1 >> 11) & 01) << 22;
    temp1 |= ((rin0 >> 23) & 01) << 23;
    temp1 |= ((rin0 >> 31) & 01) << 24;
    temp1 |= ((rin0 >> 30) & 01) << 25;
    temp1 |= ((rin0 >> 25) & 01) << 26;
    temp1 |= ((rin0 >> 28) & 01) << 27;
    temp1 |= ((rin1 >> 15) & 01) << 28;
    temp1 |= ((rin1 >> 6) & 01) << 29;
    temp1 |= ((rin1 >> 9) & 01) << 30;
    temp1 |= ((rin1 >> 20) & 01) << 31;

    *out0 = temp0;
    *out1 = temp1;
}


/*
 * multcrypt
 *	encrypt text with input key.
 *	Result is left back in text.
 *
 */
void multcrypt(key, text)
key_t		key;
mp_block_ptr_t	text;
{
    unsigned int key0, key1, key2;
    unsigned int a0, a1, b0, b1, c0, c1, d0, d1;

    key0 = key.key_longs[0];
    key1 = key.key_longs[1];
    key2 = key.key_longs[2];
    
    a0 = mult(text->mp_block_high, key0);
    a1 = mult(text->mp_block_low, key2);
    permute(&b0, &b1, a0, a1);
    c0 = mult (b0, key1);
    c1 = mult (b1, key1);
    permute(&d0, &d1, c0, c1);
    text->mp_block_high = mult(d0, key2);
    text->mp_block_low = mult(d1, key0);
}



/*
 * multdecrypt
 *	decrypt text with input key.
 *	Result is left back in text.
 *
 */
void multdecrypt(key, text)
key_t		key;
mp_block_ptr_t	text;
{
    unsigned int key0, key1, key2;
    unsigned int a0, a1, b0, b1, c0, c1, d0, d1;

    key0 = key.key_longs[2];
    key1 = key.key_longs[1];
    key2 = key.key_longs[0];
    
    a0 = mult(text->mp_block_high, key0);
    a1 = mult(text->mp_block_low, key2);
    permute(&b0, &b1, a0, a1);
    c0 = mult (b0, key1);
    c1 = mult (b1, key1);
    permute(&d0, &d1, c0, c1);
    text->mp_block_high = mult(d0, key2);
    text->mp_block_low = mult(d1, key0);
}



/*
 * minverse
 *	returns the multiplicative inverse of its parameter, 0 if there is no inverse.
 *
*/
unsigned int minverse(x)
unsigned int	x;
{
    unsigned int temp, answer;
    int i;

    temp = x;
    if (((x % 3) == 0) || ((x % 5) == 0) || ((x % 17) == 0) || ((x % 257) == 0) || ((x % 65537) == 0))
    return 0;
    else {
	answer = 1;
	for (i = 0; i < 16; i++) {
	    answer = mult(answer, temp);
	    temp = mult(temp, temp);
	}
	return answer;
    }
}

/*
 * invert_key
 *	replaces the given key by its inverse using minverse.
 *
 */
void invert_key(key_ptr)
key_t		*key_ptr;
{
    key_ptr->key_longs[0] = minverse((unsigned int)key_ptr->key_longs[0]);
    key_ptr->key_longs[1] = minverse((unsigned int)key_ptr->key_longs[1]);
    key_ptr->key_longs[2] = minverse((unsigned int)key_ptr->key_longs[2]);
    key_ptr->key_longs[3] = key_ptr->key_longs[3];
}

#endif	USE_MULTPERM

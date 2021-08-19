/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * multperm.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/multperm.h,v $
 *
 * $Header: multperm.h,v 1.1 88/09/30 15:44:16 osdev Exp $
 *
 */

/*
 * External definitions for multperm.c
 */

/*
 * HISTORY:
 * 13-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Fixed up for the new network server.
 *
 * 15-Jan-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#ifndef	_MULTPERM_
#define	_MULTPERM_


typedef struct mp_block {
    unsigned int	mp_block_high;
    unsigned int	mp_block_low;
} mp_block_t, *mp_block_ptr_t;


extern unsigned int mult();
/*
unsigned int	a,b;
*/

extern void permute();
/*
unsigned int	*out, *out1, in0, in1;
*/


extern void multcrypt();
/*
key_t		key;
mp_block_ptr_t	text;
*/

extern void multdecrypt();
/*
key_t		key;
mp_block_ptr_t	text;
*/

extern unsigned int minverse();
/*
unsigned int	test_key;
*/

extern void invert_key();
/*
key_t		*key_ptr;
*/

#endif	_MULTPERM_

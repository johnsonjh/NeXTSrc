/* @(#)softdes.h	1.1 88/04/04 4.0NFSSRC SMI;     from 1.5 88/02/08 Copyr 1986 Sun Micro */
/*
 * softdes.h,  Data types and definition for software DES
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 * A chunk is an area of storage used in three different ways
 * - As a 64 bit quantity (in high endian notation)
 * - As a 48 bit quantity (6 low order bits per byte)
 * - As a 32 bit quantity (first 4 bytes)
 */
typedef union {
	struct {
#if defined(vax) || defined(i386)
		u_long	_long1;
		u_long	_long0;
#else
		u_long	_long0;
		u_long	_long1;
#endif
	} _longs;
#define	long0	_longs._long0
#define	long1	_longs._long1
	struct {
#if defined(vax) || defined(i386)
		u_char	_byte7;
		u_char	_byte6;
		u_char	_byte5;
		u_char	_byte4;
		u_char	_byte3;
		u_char	_byte2;
		u_char	_byte1;
		u_char	_byte0;
#else
		u_char	_byte0;
		u_char	_byte1;
		u_char	_byte2;
		u_char	_byte3;
		u_char	_byte4;
		u_char	_byte5;
		u_char	_byte6;
		u_char	_byte7;
#endif
	} _bytes;
#define	byte0	_bytes._byte0
#define	byte1	_bytes._byte1
#define	byte2	_bytes._byte2
#define	byte3	_bytes._byte3
#define	byte4	_bytes._byte4
#define	byte5	_bytes._byte5
#define	byte6	_bytes._byte6
#define	byte7	_bytes._byte7
} chunk_t;

/*
 * Intermediate key storage
 * Created by des_setkey, used by des_encrypt and des_decrypt
 * 16 48 bit values
 */
struct deskeydata {
	chunk_t	keyval[16];
};

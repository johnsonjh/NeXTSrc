/* bignum.h-arbitrary precision integers */

/* Copyright (C) 1987 Free Software Foundation, Inc.

This file is part of Gas, the GNU Assembler.

The GNU assembler is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY.  No author or distributor
accepts responsibility to anyone for the consequences of using it
or for whether it serves any particular purpose or works at all,
unless he says so in writing.  Refer to the GNU Assembler General
Public License for full details.

Everyone is granted permission to copy, modify and redistribute
the GNU Assembler, but only under the conditions described in the
GNU Assembler General Public License.  A copy of this license is
supposed to have been given to you along with the GNU Assembler
so you can know your rights and responsibilities.  It should be
in a file named COPYING.  Among other things, the copyright
notice and this notice must be preserved on all copies.  */

/***********************************************************************\
*									*
*	Arbitrary-precision integer arithmetic.				*
*	For speed, we work in groups of bits, even though this		*
*	complicates algorithms.						*
*	Each group of bits is called a 'littlenum'.			*
*	A bunch of littlenums representing a (possibly large)		*
*	integer is called a 'bignum'.					*
*	Bignums are >= 0.						*
*									*
\***********************************************************************/

#define	LITTLENUM_NUMBER_OF_BITS	(16)
#define	LITTLENUM_RADIX			(1 << LITTLENUM_NUMBER_OF_BITS)
#define	LITTLENUM_MASK			(0xFFFF)
#define LITTLENUM_SHIFT			(1)
#define CHARS_PER_LITTLENUM		(1 << LITTLENUM_SHIFT)
#ifndef BITS_PER_CHAR
#define BITS_PER_CHAR			(8)
#endif

typedef unsigned short int	LITTLENUM_TYPE;


/* JF truncated this to get around a problem with GCC */
#define	LOG_TO_BASE_2_OF_10		(3.321928 /* 0948873623478703194294893901758651 */)
				/* WARNING: I haven't checked that the trailing digits are correct! */

/* end: bignum.h */

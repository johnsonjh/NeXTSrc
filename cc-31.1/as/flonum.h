/* flonum.h - Floating point package */

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
*	Arbitrary-precision floating point arithmetic.			*
*									*
*									*
*	Notation: a floating point number is expressed as		*
*	MANTISSA * (2 ** EXPONENT).					*
*									*
*	If this offends more traditional mathematicians, then		*
*	please tell me your nomenclature for flonums!			*
*									*
\***********************************************************************/

#include "bignum.h"

/***********************************************************************\
*									*
*	Variable precision floating point numbers.			*
*									*
*	Exponent is the place value of the low littlenum. E.g.:		*
*	If  0:  low points to the units             littlenum.		*
*	If  1:  low points to the LITTLENUM_RADIX   littlenum.		*
*	If -1:  low points to the 1/LITTLENUM_RADIX littlenum.		*
*									*
\***********************************************************************/

struct FLONUM_STRUCT
{
  LITTLENUM_TYPE *	low;	/* low order littlenum of a bignum */
  LITTLENUM_TYPE *	high;	/* high order littlenum of a bignum */
  LITTLENUM_TYPE *	leader;	/* -> 1st non-zero littlenum */
				/* If flonum is 0.0, leader==low-1 */
  long int		exponent; /* base LITTLENUM_RADIX */
  char			sign;	/* '+' or '-' */
};

typedef struct FLONUM_STRUCT FLONUM_TYPE;


/***********************************************************************\
*									*
*	Since we can (& do) meet with exponents like 10^5000, it	*
*	is silly to make a table of ~ 10,000 entries, one for each	*
*	power of 10. We keep a table where item [n] is a struct		*
*	FLONUM_FLOATING_POINT representing 10^(2^n). We then		*
*	multiply appropriate entries from this table to get any		*
*	particular power of 10. For the example of 10^5000, a table	*
*	of just 25 entries suffices: 10^(2^-12)...10^(2^+12).		*
*									*
\***********************************************************************/


extern FLONUM_TYPE flonum_positive_powers_of_ten[];
extern FLONUM_TYPE flonum_negative_powers_of_ten[];
extern int table_size_of_flonum_powers_of_ten;
				/* Flonum_XXX_powers_of_ten[] table has */
				/* legal indices from 0 to */
				/* + this number inclusive. */



/***********************************************************************\
*									*
*	Declare worker functions.					*
*									*
\***********************************************************************/

void	flonum_multip();
void	flonum_copy();
void	flonum_print();
char *	flonum_get();		/* Returns "" or error string. */
void	flonum_normal();

int	atof_generic();


/***********************************************************************\
*									*
*	Declare error codes.						*
*									*
\***********************************************************************/

#define ERROR_EXPONENT_OVERFLOW (2)

/* end: flonum.h */

/* bignum_copy.c - */

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

#include "bignum.h"

#ifdef USG
#define bzero(s,n) memset(s,0,n)
#define bcopy(from,to,n) memcpy(to,from,n)
#endif

/*
 *			bignum_copy ()
 *
 * Copy a bignum from in to out.
 * If the output is shorter than the input, copy lower-order littlenums.
 * Return 0 or the number of significant littlenums dropped.
 * Assumes littlenum arrays are densely packed: no unused chars between
 * the littlenums. Uses bcopy() to move littlenums, and wants to
 * know length (in chars) of the input bignum.
 */

/* void */
bignum_copy (in, in_length, out, out_length)
     register LITTLENUM_TYPE *	in;
     register int		in_length; /* in sizeof(littlenum)s */
     register LITTLENUM_TYPE *	out;
     register int		out_length; /* in sizeof(littlenum)s */
{
  register int	significant_littlenums_dropped;

  if (out_length < in_length)
    {
      register LITTLENUM_TYPE *	p; /* -> most significant (non-zero) input littlenum. */

      bcopy ((char *)in, (char *)out, out_length << LITTLENUM_SHIFT);
      for (p = in + in_length - 1;   p >= in;   -- p)
	{
	  if (* p) break;
	}
      significant_littlenums_dropped = p - in - in_length + 1;
      if (significant_littlenums_dropped < 0)
	{
	  significant_littlenums_dropped = 0;
	}
    }
  else
    {
      bcopy ((char *)in, (char *)out, in_length << LITTLENUM_SHIFT);
      if (out_length > in_length)
	{
	  bzero ((char *)(out + out_length), (out_length - in_length) << LITTLENUM_SHIFT);
	}
      significant_littlenums_dropped = 0;
    }
  return (significant_littlenums_dropped);
}

/* end: bignum_copy.c */

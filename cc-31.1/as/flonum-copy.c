/* flonum_copy.c - */

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

#include "flonum.h"
#ifdef USG
#define bzero(s,n) memset(s,0,n)
#define bcopy(from,to,n) memcpy(to,from,n)
#endif

void
flonum_copy (in, out)
     FLONUM_TYPE *	in;
     FLONUM_TYPE *	out;
{
  int			in_length;	/* 0 origin */
  int			out_length;	/* 0 origin */

  out -> sign = in -> sign;
  in_length = in  -> leader - in -> low;
  if (in_length < 0)
    {
      out -> leader = out -> low - 1; /* 0.0 case */
    }
  else
    {
      out_length = out -> high - out -> low;
      /*
       * Assume no GAPS in packing of littlenums.
       * I.e. sizeof(array) == sizeof(element) * number_of_elements.
       */
      if (in_length <= out_length)
	{
	  {
	    /*
	     * For defensive programming, zero any high-order littlenums we don't need.
	     * This is destroying evidence and wasting time, so why bother???
	     */
	    if (in_length < out_length)
	      {
		bzero ((char *)(out->low + in_length + 1), out_length - in_length);
	      }
	  }
	  bcopy ((char *)(in->low), (char *)(out->low), (int)((in_length + 1) * sizeof(LITTLENUM_TYPE)));
	  out -> exponent = in -> exponent;
	  out -> leader   = in -> leader - in -> low + out -> low;
	}
      else
	{
	  int	shorten;		/* 1-origin. Number of littlenums we drop. */

	  shorten = in_length - out_length;
	  /* Assume out_length >= 0 ! */
	  bcopy ((char *)(in->low + shorten),(char *)( out->low), (int)((out_length + 1) * sizeof(LITTLENUM_TYPE)));
	  out -> leader = out -> high;
	  out -> exponent = in -> exponent + shorten;
	}
    }				/* if any significant bits */
}

/* end: flonum_copy.c */

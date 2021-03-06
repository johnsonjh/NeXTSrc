/* Subroutines needed by GCC output code on some machines.  */
/* Copyright (C) 1989 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, if you link this library with files
   compiled with GCC to produce an executable, this does not cause
   the resulting executable to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  */

#include "gnulib.h"

long long _muldi3 (long long u, long long v)
{
  long a[2], b[2], c[2][2];
  long_long w;
  long_long uu, vv;

  uu.ll = u;
  vv.ll = v;

  a[HIGH] = uu.s.high;
  a[LOW] = uu.s.low;
  b[HIGH] = vv.s.high;
  b[LOW] = vv.s.low;

  bmul (a, b, c, sizeof a, sizeof b);

  w.s.high = c[LOW][HIGH];
  w.s.low = c[LOW][LOW];
  return w.ll;
}

static void bmul (a, b, c, m, n)
    unsigned short *a, *b, *c;
    size_t m, n;
{
  int i, j;
  unsigned long acc;

  bzero (c, m + n);

  m /= sizeof *a;
  n /= sizeof *b;

  for (j = little_end (n); is_not_msd (j, n); j = next_msd (j))
    {
      unsigned short *c1 = c + j + little_end (2);
      acc = 0;
      for (i = little_end (m); is_not_msd (i, m); i = next_msd (i))
	{
	  /* Widen before arithmetic to avoid loss of high bits.  */
	  acc += (unsigned long) a[i] * b[j] + c1[i];
	  c1[i] = acc & low16;
	  acc = acc >> 16;
	}
      c1[i] = acc;
    }
}


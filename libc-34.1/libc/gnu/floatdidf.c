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

#define WORD_SIZE (sizeof (long) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((long long) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((long long) 1) << WORD_SIZE)

double _floatdidf (long long u)
{
  double d;
  int negate = 0;

  if (u < 0)
    u = -u, negate = 1;

  d = (unsigned int) (u >> WORD_SIZE);
  d *= HIGH_HALFWORD_COEFF;
  d *= HIGH_HALFWORD_COEFF;
  d += (unsigned int) (u & (HIGH_WORD_COEFF - 1));

  return (negate ? -d : d);
}


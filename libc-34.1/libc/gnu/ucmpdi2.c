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

SItype _ucmpdi2 (long long a, long long b)
{
  long_long au, bu;

  au.ll = a, bu.ll = b;

  if ((unsigned) au.s.high < (unsigned) bu.s.high)
    return 0;
  else if ((unsigned) au.s.high > (unsigned) bu.s.high)
    return 2;
  if ((unsigned) au.s.low < (unsigned) bu.s.low)
    return 0;
  else if ((unsigned) au.s.low > (unsigned) bu.s.low)
    return 2;
  return 1;
}


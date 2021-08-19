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
#include <limits.h>

unsigned long _fixunsdfsi (double a)
{
  unsigned long old_round;
  unsigned long round_to_zero;
  long x;
 
  /* Get the current rounding mode. */
  asm volatile  ("fmove%.l fpcr,%0" : "=d" (old_round) : /* no inputs */);
 
  round_to_zero = (old_round | (1 << 4)) & ~(1 << 5);
 
  if (a > (double) ((unsigned long) LONG_MAX + 1))
    {
      double b = a - (double) (((unsigned long) LONG_MAX) + 1);
     
      /* Set round-to-zero mode. */
      asm volatile  ("fmove%.l %0,fpcr" : /* no outputs */ : "d" (round_to_zero));
     
      /* Convert the double to an integer. */
      asm volatile  ("fmove%.l %1,%0" : "=d" (x) : "f" (b));
     
      x += (((unsigned int) LONG_MAX) + 1);
    }
  else
    {
      /* Set round-to-zero mode. */
      asm volatile  ("fmove%.l %0,fpcr" : /* no outputs */ : "d" (round_to_zero));
     
      /* Convert the double to an integer. */
      asm volatile  ("fmove%.l %1,%0" : "=d" (x) : "f" (a));
    }
 
  /* Restore the previous rounding mode. */
  asm volatile  ("fmove%.l %0,fpcr" : /* no outputs */ : "d" (old_round));
 
  return x;
}

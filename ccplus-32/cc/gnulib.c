/* Subroutines needed by GCC output code on some machines.  */
/* Compile this file with the Unix C compiler!  */
/* Copyright (C) 1987, 1988 Free Software Foundation, Inc.

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

#include "config.h"

/* Define the C data type to use for an SImode value.  */

#ifndef SItype
#define SItype long int
#endif

/* Define the type to be used for returning an SF mode value
   and the method for turning a float into that type.
   These definitions work for machines where an SF value is
   returned in the same register as an int.  */

#ifndef SFVALUE  
#define SFVALUE int
#endif

#ifndef INTIFY
#define INTIFY(FLOATVAL)  (intify.f = (FLOATVAL), intify.i)
#endif

union flt_or_int { int i; float f; };


#ifdef L_umulsi3
SItype
__umulsi3 (a, b)
     unsigned SItype a, b;
{
  return a * b;
}
#endif

#ifdef L_mulsi3
SItype
__mulsi3 (a, b)
     SItype a, b;
{
  return a * b;
}
#endif

#ifdef L_udivsi3
SItype
__udivsi3 (a, b)
     unsigned SItype a, b;
{
  return a / b;
}
#endif

#ifdef L_divsi3
SItype
__divsi3 (a, b)
     SItype a, b;
{
  return a / b;
}
#endif

#ifdef L_umodsi3
SItype
__umodsi3 (a, b)
     unsigned SItype a, b;
{
  return a % b;
}
#endif

#ifdef L_modsi3
SItype
__modsi3 (a, b)
     SItype a, b;
{
  return a % b;
}
#endif

#ifdef L_lshrsi3
SItype
__lshrsi3 (a, b)
     unsigned SItype a, b;
{
  return a >> b;
}
#endif

#ifdef L_lshlsi3
SItype
__lshlsi3 (a, b)
     unsigned SItype a, b;
{
  return a << b;
}
#endif

#ifdef L_ashrsi3
SItype
__ashrsi3 (a, b)
     SItype a, b;
{
  return a >> b;
}
#endif

#ifdef L_ashlsi3
SItype
__ashlsi3 (a, b)
     SItype a, b;
{
  return a << b;
}
#endif

#ifdef L_divdf3
double
__divdf3 (a, b)
     double a, b;
{
  return a / b;
}
#endif

#ifdef L_muldf3
double
__muldf3 (a, b)
     double a, b;
{
  return a * b;
}
#endif

#ifdef L_negdf2
double
__negdf2 (a)
     double a;
{
  return -a;
}
#endif

#ifdef L_adddf3
double
__adddf3 (a, b)
     double a, b;
{
  return a + b;
}
#endif

#ifdef L_subdf3
double
__subdf3 (a, b)
     double a, b;
{
  return a - b;
}
#endif

#ifdef L_cmpdf2
SItype
__cmpdf2 (a, b)
     double a, b;
{
  if (a > b)
    return 1;
  else if (a < b)
    return -1;
  return 0;
}
#endif

#ifdef L_eqdf2
SItype
__eqdf2 (a, b)
     double a, b;
{
  /* Value == 0 iff a == b.  */
  return !(a == b);
}
#endif

#ifdef L_nedf2
SItype
__nedf2 (a, b)
     double a, b;
{
  /* Value != 0 iff a != b.  */
  return a != b;
}
#endif

#ifdef L_gtdf2
SItype
__gtdf2 (a, b)
     double a, b;
{
  /* Value > 0 iff a > b.  */
  return a > b;
}
#endif

#ifdef L_gedf2
SItype
__gedf2 (a, b)
     double a, b;
{
  /* Value >= 0 iff a >= b.  */
  return (a >= b) - 1;
}
#endif

#ifdef L_ltdf2
SItype
__ltdf2 (a, b)
     double a, b;
{
  /* Value < 0 iff a < b.  */
  return -(a < b);
}
#endif

#ifdef L_ledf2
SItype
__ledf2 (a, b)
     double a, b;
{
  /* Value <= 0 iff a <= b.  */
  return 1 - (a <= b);
}
#endif

#ifdef L_fixdfsi
SItype
__fixdfsi (a)
     double a;
{
  return (SItype) a;
}
#endif

#ifdef L_floatsidf
double
__floatsidf (a)
     SItype a;
{
  return (double) a;
}
#endif

#ifdef L_addsf3
SFVALUE
__addsf3 (a, b)
     union flt_or_int a, b;
{
  union flt_or_int intify;
  return INTIFY (a.f + b.f);
}
#endif

#ifdef L_negsf2
SFVALUE
__negsf2 (a)
     union flt_or_int a;
{
  union flt_or_int intify;
  return INTIFY (-a.f);
}
#endif

#ifdef L_subsf3
SFVALUE
__subsf3 (a, b)
     union flt_or_int a, b;
{
  union flt_or_int intify;
  return INTIFY (a.f - b.f);
}
#endif

#ifdef L_cmpsf2
SItype
__cmpsf2 (a, b)
     union flt_or_int a, b;
{
  if (a.f > b.f)
    return 1;
  else if (a.f < b.f)
    return -1;
  return 0;
}
#endif

#ifdef L_eqsf2
SItype
__eqsf2 (a, b)
     union flt_or_int a, b;
{
  /* Value == 0 iff a == b.  */
  return !(a.f == b.f);
}
#endif

#ifdef L_nesf2
SItype
__nesf2 (a, b)
     union flt_or_int a, b;
{
  /* Value != 0 iff a != b.  */
  return a.f != b.f;
}
#endif

#ifdef L_gtsf2
SItype
__gtsf2 (a, b)
     union flt_or_int a, b;
{
  /* Value > 0 iff a > b.  */
  return a.f > b.f;
}
#endif

#ifdef L_gesf2
SItype
__gesf2 (a, b)
     union flt_or_int a, b;
{
  /* Value >= 0 iff a >= b.  */
  return (a.f >= b.f) - 1;
}
#endif

#ifdef L_ltsf2
SItype
__ltsf2 (a, b)
     union flt_or_int a, b;
{
  /* Value < 0 iff a < b.  */
  return -(a.f < b.f);
}
#endif

#ifdef L_lesf2
SItype
__lesf2 (a, b)
     union flt_or_int a, b;
{
  /* Value <= 0 iff a <= b.  */
  return 1 - (a.f >= b.f);
}
#endif

#ifdef L_mulsf3
SFVALUE
__mulsf3 (a, b)
     union flt_or_int a, b;
{
  union flt_or_int intify;
  return INTIFY (a.f * b.f);
}
#endif

#ifdef L_divsf3
SFVALUE
__divsf3 (a, b)
     union flt_or_int a, b;
{
  union flt_or_int intify;
  return INTIFY (a.f / b.f);
}
#endif

#ifdef L_truncdfsf2
SFVALUE
__truncdfsf2 (a)
     double a;
{
  union flt_or_int intify;
  return INTIFY (a);
}
#endif

#ifdef L_extendsfdf2
double
__extendsfdf2 (a)
     union flt_or_int a;
{
  union flt_or_int intify;
  return a.f;
}
#endif

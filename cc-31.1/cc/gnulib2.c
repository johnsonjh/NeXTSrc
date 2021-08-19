/* More subroutines needed by GCC output code on some machines.  */
/* Compile this one with gcc.  */
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

#include "config.h"
#include <stddef.h>

#ifndef SItype
#define SItype long int
#endif

/* long long ints are pairs of long ints in the order determined by
   WORDS_BIG_ENDIAN.  */

#ifdef WORDS_BIG_ENDIAN
  struct longlong {long high, low;};
#else
  struct longlong {long low, high;};
#endif

/* We need this union to unpack/pack longlongs, since we don't have
   any arithmetic yet.  Incoming long long parameters are stored
   into the `ll' field, and the unpacked result is read from the struct
   longlong.  */

typedef union
{
  struct longlong s;
  long long ll;
  SItype i[2];
  unsigned SItype ui[2];
} long_long;

/* Internally, long long ints are strings of unsigned shorts in the
   order determined by BYTES_BIG_ENDIAN.  */

#define B 0x10000
#define low16 (B - 1)

#ifdef BYTES_BIG_ENDIAN

/* Note that HIGH and LOW do not describe the order
   of words in a long long.  They describe the order of words
   in vectors ordered according to the byte order.  */

#define HIGH 0
#define LOW 1

#define big_end(n)	0 
#define little_end(n)	((n) - 1)
#define next_msd(i)	((i) - 1)
#define next_lsd(i)	((i) + 1)
#define is_not_msd(i,n)	((i) >= 0)
#define is_not_lsd(i,n)	((i) < (n))

#else

#define LOW 0
#define HIGH 1

#define big_end(n)	((n) - 1)
#define little_end(n)	0 
#define next_msd(i)	((i) + 1)
#define next_lsd(i)	((i) - 1)
#define is_not_msd(i,n)	((i) < (n))
#define is_not_lsd(i,n)	((i) >= 0)

#endif

/* These algorithms are all straight out of Knuth, vol. 2, sec. 4.3.1. */

static int badd ();
static int bsub ();
static void bmul ();
static int bneg ();
static int bshift ();

#ifdef L_adddi3
long long 
_adddi3 (u, v)
     long long u, v;
{
  long a[2], b[2], c[2];
  long_long w;
  long_long uu, vv;

  uu.ll = u;
  vv.ll = v;

  a[HIGH] = uu.s.high;
  a[LOW] = uu.s.low;
  b[HIGH] = vv.s.high;
  b[LOW] = vv.s.low;

  badd (a, b, c, sizeof c);

  w.s.high = c[HIGH];
  w.s.low = c[LOW];
  return w.ll;
}

static int 
badd (a, b, c, n)
     unsigned short *a, *b, *c;
     size_t n;
{
  unsigned long acc;
  int i;

  n /= sizeof *c;

  acc = 0;
  for (i = little_end (n); is_not_msd (i, n); i = next_msd (i))
    {
      /* Widen before adding to avoid loss of high bits.  */
      acc += (unsigned long) a[i] + b[i];
      c[i] = acc & low16;
      acc = acc >> 16;
    }
  return acc;
}
#endif

#ifdef L_anddi3
long long 
_anddi3 (u, v)
     long long u, v;
{
  long_long w;
  long_long uu, vv;

  uu.ll = u;
  vv.ll = v;

  w.s.high = uu.s.high & vv.s.high;
  w.s.low = uu.s.low & vv.s.low;

  return w.ll;
}
#endif

#ifdef L_iordi3
long long 
_iordi3 (u, v)
     long long u, v;
{
  long_long w;
  long_long uu, vv;

  uu.ll = u;
  vv.ll = v;

  w.s.high = uu.s.high | vv.s.high;
  w.s.low = uu.s.low | vv.s.low;

  return w.ll;
}
#endif

#ifdef L_xordi3
long long 
_xordi3 (u, v)
     long long u, v;
{
  long_long w;
  long_long uu, vv;

  uu.ll = u;
  vv.ll = v;

  w.s.high = uu.s.high ^ vv.s.high;
  w.s.low = uu.s.low ^ vv.s.low;

  return w.ll;
}
#endif

#ifdef L_one_cmpldi2
long long
_one_cmpldi2 (u)
     long long u;
{
  long_long w;
  long_long uu;

  uu.ll = u;

  w.s.high = ~uu.s.high;
  w.s.low = ~uu.s.low;

  return w.ll;
}
#endif

#ifdef L_lshldi3
long long
_lshldi3 (u, b1)
     long long u;
     long long b1;
{
  long_long w;
  unsigned long carries;
  int bm;
  long_long uu;
  int b = b1;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (int) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      w.s.low = 0;
      w.s.high = (unsigned long)uu.s.low << -bm;
    }
  else
    {
      carries = (unsigned long)uu.s.low >> bm;
      w.s.low = (unsigned long)uu.s.low << b;
      w.s.high = ((unsigned long)uu.s.high << b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_lshrdi3
long long
_lshrdi3 (u, b1)
     long long u;
     long long b1;
{
  long_long w;
  unsigned long carries;
  int bm;
  long_long uu;
  int b = b1;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (int) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      w.s.high = 0;
      w.s.low = (unsigned long)uu.s.high >> -bm;
    }
  else
    {
      carries = (unsigned long)uu.s.high << bm;
      w.s.high = (unsigned long)uu.s.high >> b;
      w.s.low = ((unsigned long)uu.s.low >> b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_ashldi3
long long
_ashldi3 (u, b1)
     long long u;
     long long b1;
{
  long_long w;
  unsigned long carries;
  int bm;
  long_long uu;
  int b = b1;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (int) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      w.s.low = 0;
      w.s.high = (unsigned long)uu.s.low << -bm;
    }
  else
    {
      carries = (unsigned long)uu.s.low >> bm;
      w.s.low = (unsigned long)uu.s.low << b;
      w.s.high = ((unsigned long)uu.s.high << b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_ashrdi3
long long
_ashrdi3 (u, b1)
     long long u;
     long long b1;
{
  long_long w;
  unsigned long carries;
  int bm;
  long_long uu;
  int b = b1;

  if (b == 0)
    return u;

  uu.ll = u;

  bm = (sizeof (int) * BITS_PER_UNIT) - b;
  if (bm <= 0)
    {
      w.s.high = uu.s.high >> 31; /* just to make w.s.high 1..1 or 0..0 */
      w.s.low = uu.s.high >> -bm;
    }
  else
    {
      carries = (unsigned long)uu.s.high << bm;
      w.s.high = uu.s.high >> b;
      w.s.low = ((unsigned long)uu.s.low >> b) | carries;
    }

  return w.ll;
}
#endif

#ifdef L_subdi3
long long 
_subdi3 (u, v)
     long long u, v;
{
  long a[2], b[2], c[2];
  long_long w;
  long_long uu, vv;

  uu.ll = u;
  vv.ll = v;

  a[HIGH] = uu.s.high;
  a[LOW] = uu.s.low;
  b[HIGH] = vv.s.high;
  b[LOW] = vv.s.low;

  bsub (a, b, c, sizeof c);

  w.s.high = c[HIGH];
  w.s.low = c[LOW];
  return w.ll;
}

static int 
bsub (a, b, c, n)
     unsigned short *a, *b, *c;
     size_t n;
{
  signed long acc;
  int i;

  n /= sizeof *c;

  acc = 0;
  for (i = little_end (n); is_not_msd (i, n); i = next_msd (i))
    {
      /* Widen before subtracting to avoid loss of high bits.  */
      acc += (long) a[i] - b[i];
      c[i] = acc & low16;
      acc = acc >> 16;
    }
  return acc;
}
#endif

#ifdef L_muldi3
long long 
_muldi3 (u, v)
     long long u, v;
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

static void 
bmul (a, b, c, m, n)
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
#endif

#ifdef L_divdi3
long long
_divdi3 (u, v)
     long long u, v;
{
  if (u < 0)
    if (v < 0)
      return (unsigned long long) -u / (unsigned long long) -v;
    else
      return - ((unsigned long long) -u / (unsigned long long) v);
  else
    if (v < 0)
      return - ((unsigned long long) u / (unsigned long long) -v);
    else
      return (unsigned long long) u / (unsigned long long) v;
}
#endif

#ifdef L_moddi3
long long
_moddi3 (u, v)
     long long u, v;
{
  if (u < 0)
    if (v < 0)
      return - ((unsigned long long) -u % (unsigned long long) -v);
    else
      return - ((unsigned long long) -u % (unsigned long long) v);
  else
    if (v < 0)
      return (unsigned long long) u % (unsigned long long) -v;
    else
      return (unsigned long long) u % (unsigned long long) v;
}
#endif

#ifdef L_udivdi3
long long 
_udivdi3 (u, v)
     long long u, v;
{
  unsigned long a[2][2], b[2], q[2], r[2];
  long_long w;
  long_long uu, vv;

  uu.ll = u;
  vv.ll = v;

  a[HIGH][HIGH] = 0;
  a[HIGH][LOW] = 0;
  a[LOW][HIGH] = uu.s.high;
  a[LOW][LOW] = uu.s.low;
  b[HIGH] = vv.s.high;
  b[LOW] = vv.s.low;

  _bdiv (a, b, q, r, sizeof a, sizeof b);

  w.s.high = q[HIGH];
  w.s.low = q[LOW];
  return w.ll;
}
#endif

#ifdef L_umoddi3
long long 
_umoddi3 (u, v)
     long long u, v;
{
  unsigned long a[2][2], b[2], q[2], r[2];
  long_long w;
  long_long uu, vv;

  uu.ll = u;
  vv.ll = v;

  a[HIGH][HIGH] = 0;
  a[HIGH][LOW] = 0;
  a[LOW][HIGH] = uu.s.high;
  a[LOW][LOW] = uu.s.low;
  b[HIGH] = vv.s.high;
  b[LOW] = vv.s.low;

  _bdiv (a, b, q, r, sizeof a, sizeof b);

  w.s.high = r[HIGH];
  w.s.low = r[LOW];
  return w.ll;
}
#endif

#ifdef L_negdi2
long long 
_negdi2 (u)
     long long u;
{
  unsigned long a[2], b[2];
  long_long w;
  long_long uu;

  uu.ll = u;

  a[HIGH] = uu.s.high;
  a[LOW] = uu.s.low;

  bneg (a, b, sizeof b);

  w.s.high = b[HIGH];
  w.s.low = b[LOW];
  return w.ll;
}

static int
bneg (a, b, n)
     unsigned short *a, *b;
     size_t n;
{
  signed long acc;
  int i;

  n /= sizeof (short);

  acc = 0;
  for (i = little_end (n); is_not_msd (i, n); i = next_msd (i))
    {
      acc -= a[i];
      b[i] = acc & low16;
      acc = acc >> 16;
    }
  return acc;
}
#endif

/* Divide a by b, producing quotient q and remainder r.

       sizeof a is m
       sizeof b is n
       sizeof q is m - n
       sizeof r is n

   The quotient must fit in m - n bytes, i.e., the most significant
   n digits of a must be less than b, and m must be greater than n.  */

/* The name of this used to be _div_internal,
   but that is too long for SYSV.  */

#ifdef L_bdiv
void 
_bdiv (a, b, q, r, m, n)
     unsigned short *a, *b, *q, *r;
     size_t m, n;
{
  unsigned long qhat, rhat;
  unsigned long acc;
  unsigned short *u = (unsigned short *) alloca (m);
  unsigned short *v = (unsigned short *) alloca (n);
  unsigned short *u0, *u1, *u2;
  unsigned short *v0;
  int d, qn;
  int i, j;

  m /= sizeof *a;
  n /= sizeof *b;
  qn = m - n;

  /* Remove leading zero digits from divisor, and the same number of
     digits (which must be zero) from dividend.  */

  while (b[big_end (n)] == 0)
    {
      r[big_end (n)] = 0;

      a += little_end (2);
      b += little_end (2);
      r += little_end (2);
      m--;
      n--;

      /* Check for zero divisor.  */
      if (n == 0)
	abort ();
    }
      
  /* If divisor is a single digit, do short division.  */

  if (n == 1)
    {
      acc = a[big_end (m)];
      a += little_end (2);
      for (j = big_end (qn); is_not_lsd (j, qn); j = next_lsd (j))
	{
	  acc = (acc << 16) | a[j];
	  q[j] = acc / *b;
	  acc = acc % *b;
	}
      *r = acc;
      return;
    }

  /* No such luck, must do long division. Shift divisor and dividend
     left until the high bit of the divisor is 1.  */

  for (d = 0; d < 16; d++)
    if (b[big_end (n)] & (1 << (16 - 1 - d)))
      break;

  bshift (a, d, u, 0, m);
  bshift (b, d, v, 0, n);

  /* Get pointers to the high dividend and divisor digits.  */

  u0 = u + big_end (m) - big_end (qn);
  u1 = next_lsd (u0);
  u2 = next_lsd (u1);
  u += little_end (2);

  v0 = v + big_end (n);

  /* Main loop: find a quotient digit, multiply it by the divisor,
     and subtract that from the dividend, shifted over the right amount. */

  for (j = big_end (qn); is_not_lsd (j, qn); j = next_lsd (j))
    {
      /* Quotient digit initial guess: high 2 dividend digits over high
	 divisor digit.  */

      if (u0[j] == *v0)
	{
	  qhat = B - 1;
	  rhat = (unsigned long) *v0 + u1[j];
	}
      else
	{
	  unsigned long numerator = ((unsigned long) u0[j] << 16) | u1[j];
	  qhat = numerator / *v0;
	  rhat = numerator % *v0;
	}

      /* Now get the quotient right for high 3 dividend digits over
	 high 2 divisor digits.  */

      while (rhat < B && qhat * *next_lsd (v0) > ((rhat << 16) | u2[j]))
	{
	  qhat -= 1;
	  rhat += *v0;
	}
	    
      /* Multiply quotient by divisor, subtract from dividend.  */

      acc = 0;
      for (i = little_end (n); is_not_msd (i, n); i = next_msd (i))
	{
	  acc += (unsigned long) (u + j)[i] - v[i] * qhat;
	  (u + j)[i] = acc & low16;
	  if (acc < B)
	    acc = 0;
	  else
	    acc = (acc >> 16) | -B;
	}

      q[j] = qhat;

      /* Quotient may have been too high by 1.  If dividend went negative,
	 decrement the quotient by 1 and add the divisor back.  */

      if ((signed long) (acc + u0[j]) < 0)
	{
	  q[j] -= 1;
	  acc = 0;
	  for (i = little_end (n); is_not_msd (i, n); i = next_msd (i))
	    {
	      acc += (unsigned long) (u + j)[i] + v[i];
	      (u + j)[i] = acc & low16;
	      acc = acc >> 16;
	    }
	}
    }

  /* Now the remainder is what's left of the dividend, shifted right
     by the amount of the normalizing left shift at the top.  */

  r[big_end (n)] = bshift (u + 1 + little_end (j - 1),
			   16 - d,
			   r + little_end (2),
			   u[little_end (m - 1)] >> d,
			   n - 1);
}

/* Left shift U by K giving W; fill the introduced low-order bits with
   CARRY_IN.  Length of U and W is N.  Return carry out.  K must be
   in 0 .. 16.  */

static int
bshift (u, k, w, carry_in, n)
     unsigned short *u, *w, carry_in;
     int k, n;
{
  unsigned long acc;
  int i;

  if (k == 0)
    {
      bcopy (u, w, n * sizeof *u);
      return 0;
    }

  acc = carry_in;
  for (i = little_end (n); is_not_msd (i, n); i = next_msd (i))
    {
      acc |= (unsigned long) u[i] << k;
      w[i] = acc & low16;
      acc = acc >> 16;
    }
  return acc;
}
#endif

#ifdef L_cmpdi2
SItype
_cmpdi2 (a, b)
     long long a, b;
{
  long_long au, bu;

  au.ll = a, bu.ll = b;

  if (au.s.high < bu.s.high)
    return 0;
  else if (au.s.high > bu.s.high)
    return 2;
  if ((unsigned) au.s.low < (unsigned) bu.s.low)
    return 0;
  else if ((unsigned) au.s.low > (unsigned) bu.s.low)
    return 2;
  return 1;
}
#endif

#ifdef L_ucmpdi2
SItype
_ucmpdi2 (a, b)
     long long a, b;
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
#endif

#ifdef L_fixunsdfdi
#define WORD_SIZE (sizeof (long) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((long long) 1) << WORD_SIZE)

long long
_fixunsdfdi (a)
     double a;
{
  double b;
  unsigned long long v;

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to long long!),
     and shift it into the high word.  */
  v = (unsigned long int) b;
  v <<= WORD_SIZE;
  /* Remove high part from the double, leaving the low part as flonum.  */
  a -= (double)v;
  /* Convert that to fixed (but not to long long!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (unsigned long int) (- a);
  else
    v += (unsigned long int) a;
  return v;
}
#endif

#ifdef L_fixdfdi
long long
_fixdfdi (a)
     double a;
{
  long long _fixunsdfdi (double a);

  if (a < 0)
    return - _fixunsdfdi (-a);
  return _fixunsdfdi (a);
}
#endif

#ifdef L_floatdidf
#define WORD_SIZE (sizeof (long) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((long long) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((long long) 1) << WORD_SIZE)

double
_floatdidf (u)
     long long u;
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
#endif

#ifdef L_fixunsdfsi
#include <limits.h>

unsigned SItype
_fixunsdfsi (a)
     double a;
{
  if (a > - (double) LONG_MIN)
    return (SItype) (a - LONG_MIN) + LONG_MIN;
  return (SItype) a;
}
#endif

#ifdef L_varargs
#ifdef __sparc__
	asm (".global ___builtin_saveregs");
	asm ("___builtin_saveregs:");
	asm ("st %i0,[%fp+68]");
	asm ("st %i1,[%fp+72]");
	asm ("st %i2,[%fp+76]");
	asm ("st %i3,[%fp+80]");
	asm ("st %i4,[%fp+84]");
	asm ("retl");
	asm ("st %i5,[%fp+88]");
#else /* not __sparc__ */
#if defined(__MIPSEL__) | defined(__R3000__) | defined(__R2000__) | defined(__mips__)

  asm ("	.ent __builtin_saveregs");
  asm ("	.globl __builtin_saveregs");
  asm ("__builtin_saveregs:");
  asm ("	sw	$4,0($30)");
  asm ("	sw	$5,4($30)");
  asm ("	sw	$6,8($30)");
  asm ("	sw	$7,12($30)");
  asm ("	j	$31");
  asm ("	.end __builtin_saveregs");
#else /* not mips */
__builtin_saveregs ()
{
  abort ();
}
#endif /* not mips */
#endif /* not __sparc__ */
#endif

#ifdef L__eprintf
#include <stdio.h>
/* This is used by the `assert' macro.  */
void
__eprintf (string, line, filename)
     char *string;
     int line;
     char *filename;
{
  fprintf (stderr, string, line, filename);
}
#endif

#ifdef L_bb_init_func
#if defined (sun) && defined (m68k)
struct bb
{
  int initialized;
  char *filename;
  int *counts;
  int ncounts;
  int zero_word;
  int *addresses;
};

__bb_init_function (blocks)
	struct bb *blocks;
{
  extern int __tcov_init;

  if (! ___tcov_init)
    ___tcov_init_func ();

  ___bb_link (blocks->filename, blocks->counts, blocks->nblocks);
}

#endif
#endif

/* frills for C++ */

#ifdef L__builtin_new
typedef void (*vfp)();

extern vfp __new_handler;

char *
__builtin_new (sz)
     long sz;
{
  char *p;

  p = (char *)malloc (sz);
  if (p == 0)
    (*__new_handler) ();
  return p;
}
#endif

#ifdef L__builtin_New
typedef void (*vfp)();

static void
default_new_handler ();

vfp __new_handler = default_new_handler;

char *
__builtin_vec_new (p, maxindex, size, ctor)
     char *p;
     int maxindex, size;
     void (*ctor)();
{
  int i, nelts = maxindex + 1;
  char *rval;

  if (p == 0)
    p = (char *)__builtin_new (nelts * size);

  rval = p;

  for (i = 0; i < nelts; i++)
    {
      (*ctor) (p);
      p += size;
    }

  return rval;
}

vfp
__set_new_handler (handler)
     vfp handler;
{
  vfp prev_handler;

  prev_handler = __new_handler;
  if (handler == 0) handler = default_new_handler;
  __new_handler = handler;
  return prev_handler;
}

vfp
set_new_handler (handler)
     vfp handler;
{
  return __set_new_handler (handler);
}

static void
default_new_handler ()
{
  /* don't use fprintf (stderr, ...) because it may need to call malloc.  */
  write (2, "default_new_handler: out of memory... aaaiiiiiieeeeeeeeeeeeee!\n", 65);
  /* don't call exit () because that may call global destructors which
     may cause a loop.  */
  _exit (-1);
}
#endif

#ifdef L__builtin_del
typedef void (*vfp)();

void
__builtin_delete (ptr)
     char *ptr;
{
  if (ptr)
    free (ptr);
}

void
__builtin_vec_delete (ptr, maxindex, size, dtor, auto_delete_vec, auto_delete)
     char *ptr;
     int maxindex, size;
     void (*dtor)();
     int auto_delete;
{
  int i, nelts = maxindex + 1;
  char *p = ptr;

  ptr += nelts * size;

  for (i = 0; i < nelts; i++)
    {
      ptr -= size;
      (*dtor) (ptr, auto_delete);
    }

  if (auto_delete_vec)
    free (p);
}

#endif

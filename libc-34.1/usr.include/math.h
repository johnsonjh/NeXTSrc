/*	math.h	4.6	9/11/85	*/

/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

/* ensure single inclusion of these definitions */
#ifndef _MATH_H
#define _MATH_H

#ifdef __STRICT_BSD__
extern int errno;
#else /* __STRICT_BSD__ */
#include <errno.h>
#endif /* __STRICT_BSD__ */

/* match definition in /usr/include/sys/types.h */
#define	EDOM		33		/* Argument too large */
#define	ERANGE		34		/* Result too large */

/* contained in the libm library */
#ifndef __STRICT_ANSI__
extern double asinh(double x), acosh(double x), erf(double x), erfc(double x);
extern double lgamma(double x), hypot(double x, double y);
/* cabs not prototyped because compiler cannot match types as documented */
extern double cabs(), copysign(double x, double y);
extern int finite(double x);
extern double j0(double x), j1(double x), jn(int n, double x);
extern double y0(double x), y1(double x), yn(int n, double x);
extern double cbrt(double x);
#endif
extern double pow(double x, double y), floor(double x), ceil(double x);
extern double atan2(double x, double y);
extern double drem(double x, double y);
extern double scalb(double x, int n);
extern double fmod(double x, double y);

/* contained in the libc library */
/* stdlib.h has def of atof */
extern double modf(double value, double *iptr);
extern double ldexp(double value, int exp);
extern double frexp(double value, int *eptr);

/* compiled inline by the GNU C compiler, non-inline version in libm */
#ifndef __STRICT_ANSI__
extern double atanh(double x), expm1(double x), logb(double x);
extern double log1p(double x), rint(double x);
#endif
extern double acos(double x), asin(double x), atan(double x);
extern double cos(double x), cosh(double x), exp(double x);
extern double fabs(double x), log(double x), log10(double x);
extern double sin(double x), sinh(double x), sqrt(double x);
extern double tan(double x), tanh(double x);

#ifdef INLINE_MATH
#define acos(x) _builtin_acos(x)
static inline double acos(double x) { double v; asm("facosx %1,%0": "=f" (v): "f" (x)); if (v!=v) errno=EDOM; return v; }
#define asin(x) _builtin_asin(x)
static inline double asin(double x) { double v; asm("fasinx %1,%0": "=f" (v): "f" (x)); if (v!=v) errno=EDOM; return v; }
#define atan(x) _building_atan(x)
static inline double atan(double x) { double v; asm("fatanx %1,%0": "=f" (v): "f" (x)); return v; }
#define atanh(x) _building_atanh(x)
static inline double atanh(double x) { double v; asm("fatanhx %1,%0": "=f" (v): "f" (x)); return v; }
#define cos(x) _builtin_cos(x)
static inline double cos(double x) { double v; asm("fcosx %1,%0": "=f" (v): "f" (x)); return v; }
#define cosh(x) _builtin_cosh(x)
static inline double cosh(double x) { double v; struct { unsigned pad:19; unsigned ovfl:1; unsigned pad2:12;} fpsr; asm("fcoshx %1,%0": "=f" (v): "f" (x)); asm("fmovel fps,%0": "=g" (fpsr)); if (fpsr.ovfl) errno = ERANGE; return v; }
#define exp(x) _builtin_exp(x)
static inline double exp(double x) { double v; struct { unsigned pad:19; unsigned ovfl:1; unsigned pad2:12;} fpsr; asm("fetoxx %1,%0": "=f" (v): "f" (x)); asm("fmovel fps,%0": "=g" (fpsr)); if (fpsr.ovfl) errno = ERANGE; return v; }
#define expm1(x) _builtin_expm1(x)
static inline double expm1(double x) { double v; asm("fetoxm1x %1,%0": "=f" (v): "f" (x)); return v; }
#define fabs(x) _builtin_fabs(x)
static inline double fabs(double x) { double v; asm("fabsx %1,%0": "=f" (v): "f" (x)); return v; }
#define log(x) _builtin_log(x)
static inline double log(double x) { double v; if (x<0.0) errno=EDOM; asm("flognx %1,%0": "=f" (v): "f" (x)); return v; }
#define logb(x) _builtin_logb(x)
static inline double logb(double x) { double v; asm("fgetexpx %1,%0": "=f" (v): "f" (x)); return v; }
#define log10(x) _builtin_log10(x)
static inline double log10(double x) { double v; if (x<0.0) errno=EDOM; asm("flog10x %1,%0": "=f" (v): "f" (x)); return v; }
#define log1p(x) _builtin_log1p(x)
static inline double log1p(double x) { double v; asm("flognp1x %1,%0": "=f" (v): "f" (x)); return v; }
#define rint(x) _builtin_rint(x)
static inline double rint(double x) { double v; asm("fintx %1,%0": "=f" (v): "f" (x)); return v; }
#define sin(x) _builtin_sin(x)
static inline double sin(double x) { double v; asm("fsinx %1,%0": "=f" (v): "f" (x)); return v; }
#define sinh(x) _builtin_sinh(x)
static inline double sinh(double x) { double v; struct { unsigned pad:19; unsigned ovfl:1; unsigned pad2:12;} fpsr; asm("fsinhx %1,%0": "=f" (v): "f" (x)); asm("fmovel fps,%0": "=g" (fpsr)); if (fpsr.ovfl) errno = ERANGE; return v; }
#define sqrt(x) _builtin_sqrt(x)
static inline double sqrt(double x) { double v; if (x<0.0) errno=EDOM; asm("fsqrtx %1,%0": "=f" (v): "f" (x)); return v; }
#define tan(x) _builtin_tan(x)
static inline double tan(double x) { double v; asm("ftanx %1,%0": "=f" (v): "f" (x)); return v; }
#define tanh(x) _builtin_tanh(x)
static inline double tanh(double x) { double v; asm("ftanhx %1,%0": "=f" (v): "f" (x)); return v; }
#endif INLINE_MATH

#define HUGE_VAL (1e999)

#ifndef __STRICT_ANSI__
#define HUGE	HUGE_VAL
#define NAN	(HUGE/HUGE)

#define MAXCHAR ((char)0x7f)
#define MAXSHORT ((short)0x7fff)
#define MAXINT	((int)0x7fffffff)	/* max pos 32-bit int */
#define MAXLONG ((long)0x7fffffff)

#define MINCHAR ((char)0x80)
#define MINSHORT ((short)0x8000)
#define MININT 	((int)0x80000000)	/* max negative 32-bit integer */
#define MINLONG ((long)0x80000000)

#define MAXFLOAT ((float)3.4028234663852886e38)
#define MINFLOAT ((float)1.4012984643248171e-45)
#define MAXDOUBLE ((double)1.7976931348623157e308)
#define MINDOUBLE ((double)4.9406564584124654e-324)

#define LN_MAXFLOAT ((float)8.872283935546875e1)
#define LN_MINFLOAT ((float)-1.032789306640625e2)
#define LN_MAXDOUBLE ((double)7.0978271289338397e2)
#define LN_MINDOUBLE ((double)-7.4444007192138122e2)

#define M_E	2.7182818284590452354
#define M_LOG2E	1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2	0.69314718055994530942
#define M_LN10	2.30258509299404568402
#define M_PI	3.14159265358979323846
#define M_PI_2	1.57079632679489661923
#define M_PI_4	0.78539816339744830962
#define M_1_PI	0.31830988618379067154
#define M_2_PI	0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2	1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440
#endif /* __STRICT_ANSI__ */

#endif /* _MATH_H */


/* 
 * Copyright (c) 1985 Regents of the University of California.
 * 
 * Use and reproduction of this software are granted  in  accordance  with
 * the terms and conditions specified in  the  Berkeley  Software  License
 * Agreement (in particular, this entails acknowledgement of the programs'
 * source, and inclusion of this notice) with the additional understanding
 * that  all  recipients  should regard themselves as participants  in  an
 * ongoing  research  project and hence should  feel  obligated  to report
 * their  experiences (good or bad) with these elementary function  codes,
 * using "sendbug 4bsd-bugs@BERKELEY", to the authors.
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char sccsid[] = "@(#)cosh.c	1.2 (Berkeley) 8/21/85";
#endif

#include <math.h>
#undef cosh

#ifdef NeXT
typedef struct {
	unsigned pad : 19;
	unsigned ovfl : 1;
	unsigned pad2 : 12;
} fpsr_t;

double cosh(double x)
{
    register double v;
    register fpsr_t fpsr;
    asm("fcoshx %1,%0": "=f" (v): "f" (x));
    asm("fmovel fps,%0": "=g" (fpsr));
    if (fpsr.ovfl) errno = ERANGE;
    return v;
}
#else
/* COSH(X)
 * RETURN THE HYPERBOLIC COSINE OF X
 * DOUBLE PRECISION (VAX D format 56 bits, IEEE DOUBLE 53 BITS)
 * CODED IN C BY K.C. NG, 1/8/85; 
 * REVISED BY K.C. NG on 2/8/85, 2/23/85, 3/7/85, 3/29/85, 4/16/85.
 *
 * Required system supported functions :
 *	copysign(x,y)
 *	scalb(x,N)
 *
 * Required kernel function:
 *	exp(x) 
 *	exp__E(x,c)	...return exp(x+c)-1-x for |x|<0.3465
 *
 * Method :
 *	1. Replace x by |x|. 
 *	2. 
 *		                                        [ exp(x) - 1 ]^2 
 *	    0        <= x <= 0.3465  :  cosh(x) := 1 + -------------------
 *			       			           2*exp(x)
 *
 *		                                   exp(x) +  1/exp(x)
 *	    0.3465   <= x <= 22      :  cosh(x) := -------------------
 *			       			           2
 *	    22       <= x <= lnovfl  :  cosh(x) := exp(x)/2 
 *	    lnovfl   <= x <= lnovfl+log(2)
 *				     :  cosh(x) := exp(x)/2 (avoid overflow)
 *	    log(2)+lnovfl <  x <  INF:  overflow to INF
 *
 *	Note: .3465 is a number near one half of ln2.
 *
 * Special cases:
 *	cosh(x) is x if x is +INF, -INF, or NaN.
 *	only cosh(0)=1 is exact for finite x.
 *
 * Accuracy:
 *	cosh(x) returns the exact hyperbolic cosine of x nearly rounded.
 *	In a test run with 768,000 random arguments on a VAX, the maximum
 *	observed error was 1.23 ulps (units in the last place).
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#ifdef VAX
/* double static  */
/* mln2hi =  8.8029691931113054792E1     , Hex  2^  7   *  .B00F33C7E22BDB */
/* mln2lo = -4.9650192275318476525E-16   , Hex  2^-50   * -.8F1B60279E582A */
/* lnovfl =  8.8029691931113053016E1     ; Hex  2^  7   *  .B00F33C7E22BDA */
static long    mln2hix[] = { 0x0f3343b0, 0x2bdbc7e2};
static long    mln2lox[] = { 0x1b60a70f, 0x582a279e};
static long    lnovflx[] = { 0x0f3343b0, 0x2bdac7e2};
#define   mln2hi    (*(double*)mln2hix)
#define   mln2lo    (*(double*)mln2lox)
#define   lnovfl    (*(double*)lnovflx)
#else	/* IEEE double */
#ifdef	NeXT
#define mln2hi (7.0978271289338397310E2)   /*Hex  2^ 10   *  1.62E42FEFA39EF */
#define mln2lo (2.3747039373786107478E-14) /*Hex  2^-45   *  1.ABC9E3B39803F */
#define lnovfl (7.0978271289338397310E2)   /*Hex  2^  9   *  1.62E42FEFA39EF */
#else
double static 
mln2hi =  7.0978271289338397310E2     , /*Hex  2^ 10   *  1.62E42FEFA39EF */
mln2lo =  2.3747039373786107478E-14   , /*Hex  2^-45   *  1.ABC9E3B39803F */
lnovfl =  7.0978271289338397310E2     ; /*Hex  2^  9   *  1.62E42FEFA39EF */
#endif	NeXT
#endif

#ifdef VAX
static max = 126                      ;
#else	/* IEEE double */
#ifdef	NeXT
#define max (1023)
#define one (1.0)
#define half (1.0/2.0)
#define small (1.0E-18)	/* fl(1+small)==1 */
#else
static max = 1023                     ;
#endif	NeXT
#endif

double cosh(x)
double x;
{	
#ifndef	NeXT
	static double half=1.0/2.0,one=1.0, small=1.0E-18; /* fl(1+small)==1 */
#endif	NeXT
	double scalb(),copysign(),exp(),exp__E(),t;

#ifndef VAX
	if(x!=x) return(x);	/* x is NaN */
#endif
	if((x=copysign(x,one)) <= 22)
	    if(x<0.3465) 
		if(x<small) return(one+x);
		else {t=x+exp__E(x,0.0);x=t+t; return(one+t*t/(2.0+x)); }

	    else /* for x lies in [0.3465,22] */
	        { t=exp(x); return((t+one/t)*half); }

	if( lnovfl <= x && x <= (lnovfl+0.7)) 
        /* for x lies in [lnovfl, lnovfl+ln2], decrease x by ln(2^(max+1)) 
         * and return 2^max*exp(x) to avoid unnecessary overflow 
         */
	    return(scalb(exp((x-mln2hi)-mln2lo), max)); 

	else 
	    return(exp(x)*half);	/* for large x,  cosh(x)=exp(x)/2 */
}
#endif NeXT

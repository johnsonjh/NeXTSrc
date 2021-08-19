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
static char sccsid[] = "@(#)support.c	1.1 (Berkeley) 5/23/85";
#endif

#include <math.h>
#undef drem
#undef copysign
#undef scalb
#undef logb
#undef finite

/* 
 * Some IEEE standard p754 recommended functions and remainder and sqrt for 
 * supporting the C elementary functions.
 ******************************************************************************
 * WARNING:
 *      These codes are developed (in double) to support the C elementary
 * functions temporarily. They are not universal, and some of them are very
 * slow (in particular, drem and sqrt is extremely inefficient). Each 
 * computer system should have its implementation of these functions using 
 * its own assembler.
 ******************************************************************************
 *
 * IEEE p754 required operations:
 *     drem(x,p) 
 *              returns  x REM y  =  x - [x/y]*y , where [x/y] is the integer
 *              nearest x/y; in half way case, choose the even one.
 *     sqrt(x) 
 *              returns the square root of x correctly rounded according to 
 *		the rounding mod.
 *
 * IEEE p754 recommended functions:
 * (a) copysign(x,y) 
 *              returns x with the sign of y. 
 * (b) scalb(x,N) 
 *              returns  x * (2**N), for integer values N.
 * (c) logb(x) 
 *              returns the unbiased exponent of x, a signed integer in 
 *              double precision, except that logb(0) is -INF, logb(INF) 
 *              is +INF, and logb(NAN) is that NAN.
 * (d) finite(x) 
 *              returns the value TRUE if -INF < x < +INF and returns 
 *              FALSE otherwise.
 *
 *
 * CODED IN C BY K.C. NG, 11/25/84;
 * REVISED BY K.C. NG on 1/22/85, 2/13/85, 3/24/85.
 */


#ifdef VAX      /* VAX D format */
    static unsigned short msign=0x7fff , mexp =0x7f80 ;
    static short  prep1=57, gap=7, bias=129           ;   
    static double novf=1.7E38, nunf=3.0E-39, zero=0.0 ;
#else           /*IEEE double format */ /* fixed NeXT mexp 21-Mar-89 */
#    ifdef NeXT
#	define msign ((unsigned short)0x7fff)
#	define mexp ((unsigned short)0x7ff0)
#    else
    const static unsigned short msign=0x7fff, mexp =0x7ff0  ;
    const static short prep1=54, gap=4, bias=1023           ;
    const static double novf=1.7E308, nunf=3.0E-308,zero=0.0;

#    endif
#endif

#ifdef NeXT
#pragma CC_OPT_OFF
double scalb(double x, int N) {
	register double v;
	asm("fscalel %2,%0": "=f" (v): "0" (x), "d" (N));
	return v;
}
#pragma CC_OPT_RESTORE
#else
double scalb(x,N)
double x; int N;
{
        int k;
        double scalb();

#ifdef NATIONAL
        unsigned short *px=(unsigned short *) &x + 3;
#else /* VAX, SUN, ZILOG */
        unsigned short *px=(unsigned short *) &x;
#endif

        if( x == zero )  return(x); 

#ifdef VAX
        if( (k= *px & mexp ) != ~msign ) {
            if( N<-260) return(nunf*nunf); else if(N>260) return(novf+novf);
#else   /* IEEE */
        if( (k= *px & mexp ) != mexp ) {
            if( N<-2100) return(nunf*nunf); else if(N>2100) return(novf+novf);
            if( k == 0 ) {
                 x *= scalb(1.0,(int)prep1);  N -= prep1; return(scalb(x,N));}
#endif

            if((k = (k>>gap)+ N) > 0 )
                if( k < (mexp>>gap) ) *px = (*px&~mexp) | (k<<gap);
                else x=novf+novf;               /* overflow */
            else
                if( k > -prep1 ) 
                                        /* gradual underflow */
                    {*px=(*px&~mexp)|(short)(1<<gap); x *= scalb(1.0,k-1);}
                else
                return(nunf*nunf);
            }
        return(x);
}
#endif NeXT

double copysign(x,y)
double x,y;
{
#ifdef NATIONAL
        unsigned short  *px=(unsigned short *) &x+3,
                        *py=(unsigned short *) &y+3;
#else /* VAX, SUN, ZILOG */
        unsigned short  *px=(unsigned short *) &x,
                        *py=(unsigned short *) &y;
#endif

#ifdef VAX
        if ( (*px & mexp) == 0 ) return(x);
#endif

        *px = ( *px & msign ) | ( *py & ~msign );
        return(x);
}

#ifdef NeXT
double logb(double x)
{
    register double v;
    asm("fgetexpx %1,%0": "=f" (v): "f" (x));
    return v;
}
#else
double logb(x)
double x; 
{

#ifdef NATIONAL
        short *px=(short *) &x+3, k;
#else /* VAX, SUN, ZILOG */
        short *px=(short *) &x, k;
#endif

#ifdef VAX
        return( ((*px & mexp)>>gap) - bias);
#else /* IEEE */
        if( (k= *px & mexp ) != mexp )
            if ( k != 0 )
                return ( (k>>gap) - bias );
            else if( x != zero)
                return ( -1022.0 );
            else        
                return(-(1.0/zero));    
        else if(x != x)
            return(x);
        else
            {*px &= msign; return(x);}
#endif
}
#endif

finite(x)
double x;    
{
#ifdef VAX
        return(1.0);
#else  /* IEEE */
#ifdef NATIONAL
        return( (*((short *) &x+3 ) & mexp ) != mexp );
#else /* SUN, ZILOG */
        return( (*((short *) &x ) & mexp ) != mexp );
#endif
#endif
}

#ifdef NeXT
#pragma CC_OPT_OFF
double drem(double x, double y) {
	register double v;
	asm("fremx %2,%0": "=f" (v): "0" (x), "f" (y));
	return v;
}
#pragma CC_OPT_RESTORE
#else
double drem(x,p)
double x,p;
{
        short sign;
        double hp,dp,tmp,drem(),scalb();
        unsigned short  k; 
#ifdef NATIONAL
        unsigned short
              *px=(unsigned short *) &x  +3, 
              *pp=(unsigned short *) &p  +3,
              *pd=(unsigned short *) &dp +3,
              *pt=(unsigned short *) &tmp+3;
#else /* VAX, SUN, ZILOG */
        unsigned short
              *px=(unsigned short *) &x  , 
              *pp=(unsigned short *) &p  ,
              *pd=(unsigned short *) &dp ,
              *pt=(unsigned short *) &tmp;
#endif

        *pp &= msign ;

#ifdef VAX
        if( ( *px & mexp ) == ~msign || p == zero )
#else /* IEEE */
        if( ( *px & mexp ) == mexp || p == zero )
#endif

                return( (x != x)? x:zero/zero );

        else  if ( ((*pp & mexp)>>gap) <= 1 ) 
                /* subnormal p, or almost subnormal p */
            { double b; b=scalb(1.0,(int)prep1);
              p *= b; x = drem(x,p); x *= b; return(drem(x,p)/b);}
        else  if ( p >= novf/2)
            { p /= 2 ; x /= 2; return(drem(x,p)*2);}
        else 
            {
                dp=p+p; hp=p/2;
                sign= *px & ~msign ;
                *px &= msign       ;
                while ( x > dp )
                    {
                        k=(*px & mexp) - (*pd & mexp) ;
                        tmp = dp ;
                        *pt += k ;

#ifdef VAX
                        if( x < tmp ) *pt -= 128 ;
#else /* IEEE */
                        if( x < tmp ) *pt -= 16 ;
#endif

                        x -= tmp ;
                    }
                if ( x > hp )
                    { x -= p ;  if ( x >= hp ) x -= p ; }

		*px = *px ^ sign;
                return( x);

            }
}
#endif NeXT

/* sqrt moved to another file (sqrt.c) for ANSI C compatibility */

#if 0
/* DREM(X,Y)
 * RETURN X REM Y =X-N*Y, N=[X/Y] ROUNDED (ROUNDED TO EVEN IN THE HALF WAY CASE)
 * DOUBLE PRECISION (VAX D format 56 bits, IEEE DOUBLE 53 BITS)
 * INTENDED FOR ASSEMBLY LANGUAGE
 * CODED IN C BY K.C. NG, 3/23/85, 4/8/85.
 *
 * Warning: this code should not get compiled in unless ALL of
 * the following machine-dependent routines are supplied.
 * 
 * Required machine dependent functions (not on a VAX):
 *     swapINX(i): save inexact flag and reset it to "i"
 *     swapENI(e): save inexact enable and reset it to "e"
 */

double drem(x,y)	
double x,y;
{

#ifdef NATIONAL		/* order of words in floating point number */
	static n0=3,n1=2,n2=1,n3=0;
#else /* VAX, SUN, ZILOG */
	static n0=0,n1=1,n2=2,n3=3;
#endif

    	static unsigned short mexp =0x7ff0, m25 =0x0190, m57 =0x0390;
	static double zero=0.0;
	double hy,y1,t,t1;
	short k;
	long n;
	int i,e; 
	unsigned short xexp,yexp, *px  =(unsigned short *) &x  , 
	      		nx,nf,	  *py  =(unsigned short *) &y  ,
	      		sign,	  *pt  =(unsigned short *) &t  ,
	      			  *pt1 =(unsigned short *) &t1 ;

	xexp = px[n0] & mexp ;	/* exponent of x */
	yexp = py[n0] & mexp ;	/* exponent of y */
	sign = px[n0] &0x8000;	/* sign of x     */

/* return NaN if x is NaN, or y is NaN, or x is INF, or y is zero */
	if(x!=x) return(x); if(y!=y) return(y);	     /* x or y is NaN */
	if( xexp == mexp )   return(zero/zero);      /* x is INF */
	if(y==zero) return(y/y);

/* save the inexact flag and inexact enable in i and e respectively
 * and reset them to zero
 */
	i=swapINX(0);	e=swapENI(0);	

/* subnormal number */
	nx=0;
	if(yexp==0) {t=1.0,pt[n0]+=m57; y*=t; nx=m57;}

/* if y is tiny (biased exponent <= 57), scale up y to y*2**57 */
	if( yexp <= m57 ) {py[n0]+=m57; nx+=m57; yexp+=m57;}

	nf=nx;
	py[n0] &= 0x7fff;	
	px[n0] &= 0x7fff;

/* mask off the least significant 27 bits of y */
	t=y; pt[n3]=0; pt[n2]&=0xf800; y1=t;

/* LOOP: argument reduction on x whenever x > y */
loop:
	while ( x > y )
	{
	    t=y;
	    t1=y1;
	    xexp=px[n0]&mexp;	  /* exponent of x */
	    k=xexp-yexp-m25;
	    if(k>0) 	/* if x/y >= 2**26, scale up y so that x/y < 2**26 */
		{pt[n0]+=k;pt1[n0]+=k;}
	    n=x/t; x=(x-n*t1)-n*(t-t1);
	}	
    /* end while (x > y) */

	if(nx!=0) {t=1.0; pt[n0]+=nx; x*=t; nx=0; goto loop;}

/* final adjustment */

	hy=y/2.0;
	if(x>hy||((x==hy)&&n%2==1)) x-=y; 
	px[n0] ^= sign;
	if(nf!=0) { t=1.0; pt[n0]-=nf; x*=t;}

/* restore inexact flag and inexact enable */
	swapINX(i); swapENI(e);	

	return(x);	
}
#endif

#if 0
/* SQRT
 * RETURN CORRECTLY ROUNDED (ACCORDING TO THE ROUNDING MODE) SQRT
 * FOR IEEE DOUBLE PRECISION ONLY, INTENDED FOR ASSEMBLY LANGUAGE
 * CODED IN C BY K.C. NG, 3/22/85.
 *
 * Warning: this code should not get compiled in unless ALL of
 * the following machine-dependent routines are supplied.
 * 
 * Required machine dependent functions:
 *     swapINX(i)  ...return the status of INEXACT flag and reset it to "i"
 *     swapRM(r)   ...return the current Rounding Mode and reset it to "r"
 *     swapENI(e)  ...return the status of inexact enable and reset it to "e"
 *     addc(t)     ...perform t=t+1 regarding t as a 64 bit unsigned integer
 *     subc(t)     ...perform t=t-1 regarding t as a 64 bit unsigned integer
 */

static unsigned long table[] = {
0, 1204, 3062, 5746, 9193, 13348, 18162, 23592, 29598, 36145, 43202, 50740,
58733, 67158, 75992, 85215, 83599, 71378, 60428, 50647, 41945, 34246, 27478,
21581, 16499, 12183, 8588, 5674, 3403, 1742, 661, 130, };

double newsqrt(x)
double x;
{
        double y,z,t,addc(),subc(),b54=134217728.*134217728.; /* b54=2**54 */
        long mx,scalx,mexp=0x7ff00000;
        int i,j,r,e,swapINX(),swapRM(),swapENI();       
        unsigned long *py=(unsigned long *) &y   ,
                      *pt=(unsigned long *) &t   ,
                      *px=(unsigned long *) &x   ;
#ifdef NATIONAL         /* ordering of word in a floating point number */
        int n0=1, n1=0; 
#else
        int n0=0, n1=1; 
#endif
/* Rounding Mode:  RN ...round-to-nearest 
 *                 RZ ...round-towards 0
 *                 RP ...round-towards +INF
 *		   RM ...round-towards -INF
 */
        int RN=0,RZ=1,RP=2,RM=3;/* machine dependent: work on a Zilog Z8070
                                 * and a National 32081 & 16081
                                 */

/* exceptions */
	if(x!=x||x==0.0) return(x);  /* sqrt(NaN) is NaN, sqrt(+-0) = +-0 */
	if(x<0) return((x-x)/(x-x)); /* sqrt(negative) is invalid */
        if((mx=px[n0]&mexp)==mexp) return(x);  /* sqrt(+INF) is +INF */

/* save, reset, initialize */
        e=swapENI(0);   /* ...save and reset the inexact enable */
        i=swapINX(0);   /* ...save INEXACT flag */
        r=swapRM(RN);   /* ...save and reset the Rounding Mode to RN */
        scalx=0;

/* subnormal number, scale up x to x*2**54 */
        if(mx==0) {x *= b54 ; scalx-=0x01b00000;}

/* scale x to avoid intermediate over/underflow:
 * if (x > 2**512) x=x/2**512; if (x < 2**-512) x=x*2**512 */
        if(mx>0x5ff00000) {px[n0] -= 0x20000000; scalx+= 0x10000000;}
        if(mx<0x1ff00000) {px[n0] += 0x20000000; scalx-= 0x10000000;}

/* magic initial approximation to almost 8 sig. bits */
        py[n0]=(px[n0]>>1)+0x1ff80000;
        py[n0]=py[n0]-table[(py[n0]>>15)&31];

/* Heron's rule once with correction to improve y to almost 18 sig. bits */
        t=x/y; y=y+t; py[n0]=py[n0]-0x00100006; py[n1]=0;

/* triple to almost 56 sig. bits; now y approx. sqrt(x) to within 1 ulp */
        t=y*y; z=t;  pt[n0]+=0x00100000; t+=z; z=(x-z)*y; 
        t=z/(t+x) ;  pt[n0]+=0x00100000; y+=t;

/* twiddle last bit to force y correctly rounded */ 
        swapRM(RZ);     /* ...set Rounding Mode to round-toward-zero */
        swapINX(0);     /* ...clear INEXACT flag */
        swapENI(e);     /* ...restore inexact enable status */
        t=x/y;          /* ...chopped quotient, possibly inexact */
        j=swapINX(i);   /* ...read and restore inexact flag */
        if(j==0) { if(t==y) goto end; else t=subc(t); }  /* ...t=t-ulp */
        b54+0.1;        /* ..trigger inexact flag, sqrt(x) is inexact */
        if(r==RN) t=addc(t);            /* ...t=t+ulp */
        else if(r==RP) { t=addc(t);y=addc(y);}/* ...t=t+ulp;y=y+ulp; */
        y=y+t;                          /* ...chopped sum */
        py[n0]=py[n0]-0x00100000;       /* ...correctly rounded sqrt(x) */
end:    py[n0]=py[n0]+scalx;            /* ...scale back y */
        swapRM(r);                      /* ...restore Rounding Mode */
        return(y);
}
#endif

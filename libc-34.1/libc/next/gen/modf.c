static inline double modf_rint(double x);
static inline double modf_copysign(double x, double y);

double modf(v, iptr)
	double v;
	double *iptr;
{
	double fract, tfract, fabs();
	
	*iptr = (double)modf_rint(v);
	if (*iptr < 0 && (*iptr > v))
		(*iptr)--;
	if (*iptr >= 0 && (*iptr > v))
		(*iptr)--;
	fract = fabs (v - *iptr);

	return (fract);
}
static double L = 4503599627370496.0E0;		/* 2**52 */
static int modf_init = 0;
#pragma CC_OPT_OFF
static inline double modf_rint(x)
double x;
{
	double s,t,one = 1.0;
	
	/* compiler screws up atof above -- generate 2**52 the hard way.. */
	if (modf_init == 0) {
		int i;
		L = 1.0;
		for (i = 52; i; i--)
			L *= 2.0;
		modf_init = 1;
	}
	if (x != x)				/* NaN */
		return (x);
	if (modf_copysign(x,one) >= L)		/* already an integer */
	    return (x);
	s = modf_copysign(L,x);
	t = x + s;				/* x+s rounded to integer */
	return (t - s);
}

#define msign ((unsigned short)0x7fff)
#define mexp ((unsigned short)0x7ff0)

static inline double modf_copysign(x,y)
double x,y;
{
        unsigned short  *px=(unsigned short *) &x,
                        *py=(unsigned short *) &y;
        *px = ( *px & msign ) | ( *py & ~msign );
        return(x);
}
#pragma CC_OPT_ON

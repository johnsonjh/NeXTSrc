/* Copyright (c) 1988 NeXT, Inc. - CCH 13-Dec-88 */

#include <math.h>
#undef fmod

#ifdef NeXT
#pragma CC_OPT_OFF
double fmod(double x, double y) {
	register double v;
	if (y==0.0) errno = EDOM;
	asm("fremx %2,%0": "=f" (v): "0" (x), "f" (y));
	if (copysign(v,x) != v) {
		v += y;
	}
	return v;
}
#pragma CC_OPT_RESTORE
#else
double fmod(x,p)
double x,p;
{
	register double v;
	v = drem(x,p);
	if (copysign(v,x) != v) {
		v += y;
	}
	return v;
}
#endif NeXT

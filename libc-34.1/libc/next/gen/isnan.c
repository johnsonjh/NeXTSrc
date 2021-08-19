/*
 * Copyright (c) 1987 NeXT, INC.
 */

/*
 * isnan -- returns 1 if IEEE NaN, -1 if IEEE SNaN, 0 otherwise
 */
isnan(d)
double d;
{
	register int i;

	i = *(int *)&d;
	if ((i & 0x7ff00000) != 0x7ff00000 || (i & 0xfffff) == 0)
		return(0);
	return((i & 0x80000) ? 1 : -1);
}

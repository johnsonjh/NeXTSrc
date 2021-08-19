/*
 * Copyright (c) 1987 NeXT, INC.
 */

/*
 * isinf -- returns 1 if positive IEEE infinity, -1 if negative
 *	    IEEE infinity, 0 otherwise.
 */
isinf(d)
double d;
{
	int i;

	i = *(int *)&d;
	if (i == 0x7ff00000)
		return(1);
	if (i == 0xfff00000)
		return(-1);
	return(0);
}

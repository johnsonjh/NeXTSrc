/*
 * Taken from bmove.11.s
 */

char *bmove(a, b, l)
	char	*a, *b;
	int	l;
{
	register int	n;
	register char	*p, *q;
	p = a;
	q = b;
	n = l;
	while (n--)
		*q++ = *p++;
	return (q);
}

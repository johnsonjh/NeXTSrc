/*  atoh  --  convert ascii to hexidecimal
 *
 *  Usage:  i = atoh (string);
 *	unsigned int i;
 *	char *string;
 *
 *  Atoo converts the value contained in "string" into an
 *  unsigned integer, assuming that the value represents
 *  a hexidecimal number.
 *
 *  HISTORY
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for VAX.
 *
 */

unsigned int atoh(ap)
char *ap;
{
	register char *p;
	register unsigned int n;
	register int digit,lcase;

	p = ap;
	n = 0;
	while(*p == ' ' || *p == '	')
		p++;
	while ((digit = (*p >= '0' && *p <= '9')) ||
		(lcase = (*p >= 'a' && *p <= 'f')) ||
		(*p >= 'A' && *p <= 'F')) {
		n *= 16;
		if (digit)	n += *p++ - '0';
		else if (lcase)	n += 10 + (*p++ - 'a');
		else		n += 10 + (*p++ - 'A');
	}
	return(n);
}

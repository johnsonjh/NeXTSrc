/*  atoo  --  convert ascii to octal
 *
 *  Usge:  i = atoo (string);
 *	unsigned int i;
 *	char *string;
 *
 *  Atoo converts the value contained in "string" into an
 *  unsigned integer, assuming that the value represents
 *  an octal number.
 *
 *  HISTORY
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX.
 *
 */

unsigned int atoo(ap)
char *ap;
{
	register unsigned int n;
	register char *p;

	p = ap;
	n = 0;
	while(*p == ' ' || *p == '	')
		p++;
	while(*p >= '0' && *p <= '7')
		n = n * 8 + *p++ - '0';
	return(n);
}

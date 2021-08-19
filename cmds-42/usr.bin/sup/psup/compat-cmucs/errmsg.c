/*****************************************************************
 * HISTORY
 * 04-Mar-85  Rudy Nedved (ern) at Carnegie-Mellon University
 *	Create a CMU version of the BBN errmsg routine from scratch. It
 *	differs from the BBN errmsg routine in the fact that it uses a
 *	negative value to indicate using the current errno value...the
 *	BBN uses a negative OR zero value.
 */

extern int	errno;
extern int	sys_nerr;
extern char	*sys_errlist[];

static char *itoa(p,n)
char *p;
unsigned n;
{
    if (n >= 10)
	p =itoa(p,n/10);
    *p++ = (n%10)+'0';
    return(p);
}

char *errmsg(cod)
int cod;
{
	static char unkmsg[] = "Unknown error ";
	static char unk[sizeof(unkmsg)+11];		/* trust us */

	if (cod < 0) cod = errno;

	if((cod >= 0) && (cod < sys_nerr))
	    return(sys_errlist[cod]);

	strcpy(unk,unkmsg);
	*itoa(&unk[sizeof(unkmsg)-1],cod) = '\0';

	return(unk);
}

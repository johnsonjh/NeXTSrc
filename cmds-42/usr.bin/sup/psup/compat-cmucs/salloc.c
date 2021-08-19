/*
 **********************************************************************
 * HISTORY
 * 02-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created from routine by same name in Steve Shafer's sup program.
 *
 **********************************************************************
 */
char *malloc();

char *salloc(p)
char *p;
{
	register char *q;
	q = malloc(strlen(p) + 1);
	if (q > 0) strcpy(q,p);
	return(q);
}

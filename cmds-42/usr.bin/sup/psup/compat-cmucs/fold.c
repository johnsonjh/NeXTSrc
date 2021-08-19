/*  fold  --  perform case folding
 *
 *  Usage:  p = fold (out,in,whichway);
 *	    p = foldup (out,in);
 *	    p = folddown (out,in);
 *	char *p,*in,*out;
 *	enum {FOLDUP, FOLDDOWN} whichway;
 *
 *  Fold performs case-folding, moving string "in" to
 *  "out" and folding one case to another en route.
 *  Folding may be upper-to-lower case (folddown) or
 *  lower-to-upper case.
 *  Foldup folds to upper case; folddown folds to lower case.
 *  The same string may be specified as both "in" and "out".
 *  The address of "out" is returned for convenience.
 *
 *  HISTORY
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX; now uses enumerated type for fold().  The
 *	foldup() and folddown() routines are new.
 *
 */

#include <libc.h>

char *fold (out,in,whichway)
char *in,*out;
FOLDMODE whichway;
{
	register char *i,*o;
	register char lower;
	char upper;
	int delta;

	switch (whichway)
	{
	case FOLDUP:
		lower = 'a';		/* lower bound of range to change */
		upper = 'z';		/* upper bound of range */
		delta = 'A' - 'a';	/* amount of change */
		break;
	case FOLDDOWN:
		lower = 'A';
		upper = 'Z';
		delta = 'a' - 'A';
	}

	i = in;
	o = out;
	do {
		if (*i >= lower && *i <= upper)		*o++ = *i++ + delta;
		else					*o++ = *i++;
	} 
	while (*i);
	*o = '\0';
	return (out);
}

char *foldup (out,in)
char *in,*out;
{
	return (fold(out,in,FOLDUP));
}

char *folddown (out,in)
char *in,*out;
{
	return (fold(out,in,FOLDDOWN));
}

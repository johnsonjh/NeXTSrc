/*  chrarg  --  parse character and return its index
 *
 *  Usage:  i = chrarg (ptr,brk,prompt,legals,defalt);
 *	int i;
 *	char **ptr,*brk,*prompt,*legals,defalt;
 *
 *  Chrarg will parse an argument from the string pointed to by "ptr",
 *  bumping ptr to point to the next argument.  The first character
 *  of the arg will be searched for in "legals", and its index
 *  returned; if it is not found, or if there is no argument, then
 *  getchr() will be used to ask the user for a character.
 *  "Brk" is the list of characters which may terminate an argument;
 *  if it is 0, then " " is used.
 *
 *  HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now puts output on stderr.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX.
 *
 */

#include <stdio.h>

int strcmp(), getchr();
char *index(),*nxtarg();

int chrarg (ptr,brk,prompt,legals,defalt)
char **ptr, *brk, *prompt, *legals, defalt;
{
	register int i;
	register char *arg,*p;

	i = -1;			/* bogus value */
	fflush (stdout);

	arg = nxtarg (ptr,brk);	/* parse argument */

	if (*arg) {		/* there was an arg */
		p = index (legals,*arg);
		if (p) {
			i = p - legals;
		} 
		else if (strcmp("?",arg) != 0) {
			fprintf (stderr,"%s: not valid.  ",arg);
		}
	}

	if (i < 0) {
		i = getchr (prompt,legals,defalt);
	}

	return (i);
}

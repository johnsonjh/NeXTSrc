/*  intarg  --  parse integer argument
 *
 *  Usage:  i = intarg (ptr,brk,prompt,min,max,default)
 *    int i,min,max,default;
 *    char **ptr,*brk,*prompt;
 *
 *  Will attempt to parse an argument from the string pointed to
 *  by "ptr", incrementing ptr to point to the next arg.  If
 *  an arg is found, it is converted into an integer.  If there is
 *  no arg or the value of the arg is not within the range
 *  [min..max], then "getint" is called to ask the user for an
 *  integer value.
 *  "Brk" is the list of characters which may terminate an argument;
 *  if 0, then " " is used.
 *
 *  HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now puts output on stderr.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX.
 *
 */

#include <ctype.h>
#include <stdio.h>

int getint();
char *nxtarg();
long atol();
int strcmp();

int intarg (ptr,brk,prompt,min,max,defalt)
char **ptr;
char *brk,*prompt;
int min,max,defalt;
{
	register int i;
	register char *arg,*p;

	arg = nxtarg (ptr,brk);
	fflush (stdout);

	if (*arg != '\0') {		/* if there was an arg */
		for (p=arg; *p && (isdigit(*p) || *p == '-' || *p == '+'); p++) ;
		if (*p) {
			if (strcmp(arg,"?") != 0)  fprintf (stderr,"%s not numeric.  ",arg);
		} 
		else {
			i = atol (arg);
			if (i<min || i>max) {
				fprintf (stderr,"%d out of range.  ",i);
			}
		}
	}

	if (*arg == '\0' || *p != '\0' || i<min || i>max) {
		i = getint (prompt,min,max,defalt);
	}

	return (i);
}

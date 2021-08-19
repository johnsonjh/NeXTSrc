/*  octarg  --  parse octal integer argument
 *
 *  Usage:  i = octarg (ptr,brk,prompt,min,max,default)
 *    unsigned int i,min,max,default;
 *    char **ptr,*brk,*prompt;
 *
 *  Will attempt to parse an argument from the string pointed to
 *  by "ptr", incrementing ptr to point to the next arg.  If
 *  an arg is found, it is converted into an octal integer.  If there is
 *  no arg or the value of the arg is not within the range
 *  [min..max], then "getoct" is called to ask the user for an
 *  octal integer value.
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

#include <stdio.h>
#include <ctype.h>

int getoct();
char *nxtarg();
unsigned int atoo();
int strcmp();

int octarg (ptr,brk,prompt,min,max,defalt)
char **ptr;
char *brk,*prompt;
unsigned int min,max,defalt;
{
	register unsigned int i;
	register char *arg,*p;

	arg = nxtarg (ptr,brk);
	fflush (stdout);

	if (*arg != '\0') {		/* if there was an arg */
		for (p=arg; *p && (*p >= '0' && *p <= '7'); p++) ;
		if (*p) {
			if (strcmp(arg,"?") != 0)  fprintf (stderr,"%s not octal number.  ",arg);
		} 
		else {
			i = atoo (arg);
			if (i<min || i>max) {
				fprintf (stderr,"%s%o out of range.  ",
					(i ? "0" : ""),i);
			}
		}
	}

	if (*arg == '\0' || *p != '\0' || i<min || i>max) {
		i = getoct (prompt,min,max,defalt);
	}

	return (i);
}

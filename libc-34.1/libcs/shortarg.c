/*  shortarg  --  parse short integer argument
 *
 *  Usage:  i = shortarg (ptr,brk,prompt,min,max,default)
 *    short i,min,max,default;
 *    char **ptr,*brk,*prompt;
 *
 *  Will attempt to parse an argument from the string pointed to
 *  by "ptr", incrementing ptr to point to the next arg.  If
 *  an arg is found, it is converted into a short.  If there is
 *  no arg or the value of the arg is not within the range
 *  [min..max], then "getshort" is called to ask the user for an
 *  short value.
 *  "brk" is the list of characters which terminate an argument;
 *  if 0, then " " is used.
 *
 *  HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now puts output on stderr.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for VAX.
 *
 */

#include <ctype.h>
#include <stdio.h>

short getshort();
char *nxtarg();
int atoi();
int strcmp();

short shortarg (ptr,brk,prompt,min,max,defalt)
char **ptr;
char *brk,*prompt;
short min,max,defalt;
{
	register short i;
	register char *arg,*p;

	arg = nxtarg (ptr,brk);
	fflush (stdout);

	if (*arg != '\0') {		/* if there was an arg */
		for (p=arg; *p && (isdigit(*p) || *p == '-' || *p == '+'); p++) ;
		if (*p) {
			if (strcmp(arg,"?") != 0)  fprintf (stderr,"%s not numeric.  ",arg);
		} 
		else {
			i = atoi (arg);
			if (i<min || i>max) {
				fprintf (stderr,"%d out of range.  ",i);
			}
		}
	}

	if (*arg == '\0' || *p != '\0' || i<min || i>max) {
		i = getshort (prompt,min,max,defalt);
	}

	return (i);
}

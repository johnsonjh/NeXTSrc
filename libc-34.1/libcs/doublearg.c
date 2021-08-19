/*  doublearg  --  parse double argument
 *
 *  Usage:  i = doublearg (ptr,brk,prompt,min,max,default)
 *    double i,min,max,default;
 *    char **ptr,*brk,*prompt;
 *
 *  Will attempt to parse an argument from the string pointed to
 *  by "ptr", incrementing ptr to point to the next arg.  If
 *  an arg is found, it is converted into an double.  If there is
 *  no arg or the value of the arg is not within the range
 *  [min..max], then "getdouble" is called to ask the user for an
 *  double value.
 *  "Brk is the list of characters which terminate an argument;
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

double getdouble();
char *nxtarg();
#include <math.h>
int strcmp();

double doublearg (ptr,brk,prompt,min,max,defalt)
char **ptr;
char *brk,*prompt;
double min,max,defalt;
{
	register double i;
	register char *arg,*p;

	arg = nxtarg (ptr,brk);
	fflush (stdout);

	if (*arg != '\0') {		/* if there was an arg */
		for (p=arg; *p && (isdigit(*p) || *p == '-' || *p == '+' || *p == '.' || *p == 'e' || *p == 'E'); p++) ;
		if (*p) {
			if (strcmp(arg,"?") != 0)  fprintf (stderr,"%s not numeric.  ",arg);
		} 
		else {
			i = atof (arg);
			if (i<min || i>max) {
				fprintf (stderr,"%g out of range.  ",i);
			}
		}
	}

	if (*arg == '\0' || *p != '\0' || i<min || i>max) {
		i = getdouble (prompt,min,max,defalt);
	}

	return (i);
}

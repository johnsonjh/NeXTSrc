/*  getdouble --  prompt user for double
 *
 *  Usage:  f = getdouble (prompt,min,max,defalt)
 *	double f,min,max,defalt;
 *	char *prompt;
 *
 *  Getdouble prints the message:  prompt  (min to max)  [defalt]
 *  and accepts a line of input from the user.  If the input
 *  is not null or numeric, an error message is printed; otherwise,
 *  the value is converted to a double (or the value "defalt" is
 *  substituted if the input is null).  Then, the value is
 *  checked to ensure that is lies within the range "min" to "max".
 *  If it does not, an error message is printed.  As long as
 *  errors occur, the cycle is repeated; when a legal value is
 *  entered, this value is returned by getdouble.
 *  On end-of-file or read error in the standard input, the default
 *  is returned.
 *
 *  HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now uses stderr for output.
 *
 * 23-Oct-82  Steven Shafer (sas) at Carnegie-Mellon University
 *	Added code to return default on EOF or error in standard input.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for VAX.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>

double getdouble (prompt,min,max,defalt)
	double min,max,defalt;
	char *prompt;
{
	char input [200];
	register char *p;
	register int err;
	double i;

	fflush (stdout);
	do {

		fprintf (stderr,"%s  (%g to %g)  [%g]  ",prompt,min,max,defalt);
		fflush (stderr);

		err = 0;
		if (gets(input) == NULL) {
			i = defalt;
			err = (i < min || max < i);
		}
		else {
			for (p=input; *p &&
		 	(isdigit(*p) || *p=='-' || *p=='.' || *p=='+'
		  	|| *p=='e' || *p=='E');
		 	p++);
	
			if (*p) {		/* non-numeric */
				err = 1;
			} else {
				if (*input)	i = atof (input);
				else		i = defalt;
				err = (i < min || max < i);
			}
		}

		if (err) fprintf (stderr,"Must be a number between %g and %g\n",
			min,max);
	} while (err);

	return (i);
}

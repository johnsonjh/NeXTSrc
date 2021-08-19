/*  getfloat --  prompt user for float
 *
 *  Usage:  f = getfloat (prompt,min,max,defalt)
 *	float f,min,max,defalt;
 *	char *prompt;
 *
 *  Getfloat prints the message:  prompt  (min to max)  [defalt]
 *  and accepts a line of input from the user.  If the input
 *  is not null or numeric, an error message is printed; otherwise,
 *  the value is converted to a float (or the value "defalt" is
 *  substituted if the input is null).  Then, the value is
 *  checked to ensure that is lies within the range "min" to "max".
 *  If it does not, an error message is printed.  As long as
 *  errors occur, the cycle is repeated; when a legal value is
 *  entered, this value is returned by getfloat.
 *  The default is returned on EOF or error in the standard input.
 *
 *  HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now uses stderr for output.
 *
 *  5-Nov-84  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed i to a double for extra precision in comparisons.
 *
 * 23-Oct-82  Steven Shafer (sas) at Carnegie-Mellon University
 *	Added code to return default on error or EOF on standard input.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for VAX.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>

float getfloat (prompt,min,max,defalt)
float min,max,defalt;
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
			} 
			else {
				if (*input)	i = atof (input);
				else		i = defalt;
				err = (i < min || max < i);
			}
		}

		if (err) fprintf (stderr,"Must be a number between %g and %g\n",
		min,max);
	} 
	while (err);

	return (i);
}

/*  getoct --  prompt user for octal integer
 *
 *  Usage:  i = getoct (prompt,min,max,defalt)
 *	unsigned int i,min,max,defalt;
 *	char *prompt;
 *
 *  Getoct prints the message:  prompt  (min to max, octal)  [defalt]
 *  and accepts a line of input from the user.  If the input
 *  is not null or numeric, an error message is printed; otherwise,
 *  the value is converted to an octal integer (or the value "defalt" is
 *  substituted if the input is null).  Then, the value is
 *  checked to ensure that is lies within the range "min" to "max".
 *  If it does not, an error message is printed.  As long as
 *  errors occur, the cycle is repeated; when a legal value is
 *  entered, this value is returned by getoct.
 *  On error or EOF in the standard input, the default is returned.
 *
 *  HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now uses stderr for output.
 *
 * 23-Oct-82  Steven Shafer (sas) at Carnegie-Mellon University
 *	Added code to return default on error or EOF in standard input.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX.  Now uses all unsigned ints, to avoid
 *	sign problems we had on PDP-11.
 *
 */

#include <stdio.h>
#include <ctype.h>

unsigned int atoo();

unsigned int getoct (prompt,min,max,defalt)
unsigned int min,max,defalt;
char *prompt;
{
	char input [200];
	register char *p;
	register unsigned int i;
	register int err;

	fflush (stdout);
	do {

		fprintf (stderr,"%s  (%s%o to %s%o, octal)  [%s%o]  ",
			prompt,(min ? "0" : ""),min,
			(max ? "0" : ""),max,(defalt ? "0" : ""),defalt);
		fflush (stderr);

		if (gets (input) == NULL) {
			i = defalt;
			err = (i < min || max < i);
		}
		else {
			err = 0;
			for (p=input; *p && (*p >= '0' && *p <= '7'); p++) ;
			if (*p) {		/* non-numeric */
				err = 1;
			} 
			else {
				if (*input)	i = atoo (input);
				else		i = defalt;
				err = (i < min || max < i);
			}
		}

		if (err) {
			fprintf (stderr,"Must be an unsigned octal number between %s%o and %s%o\n",
			(min ? "0" : ""),min,(max ? "0" : ""),max);
		}
	} 
	while (err);

	return (i);
}

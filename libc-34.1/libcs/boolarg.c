/*  boolarg  --  parse boolean from string
 *
 *  Usage:  i = boolarg (ptr,brk,prompt,defalt);
 *	int i,defalt;
 *	char **ptr,*brk,*prompt;
 *
 *  Boolarg will parse an argument from the string pointed to by "ptr",
 *  bumping ptr to point to the next argument in the string.  The
 *  argument parsed will be converted to a boolean (TRUE if it begins
 *  with 'y' or 'Y'; FALSE for 'n' or 'N').  If there is no
 *  argument, or if it is not a boolean value, then getbool() will
 *  be used to ask the user for a boolean value.  In any event,
 *  the boolean value obtained (from the string or from the user) is
 *  returned.
 *  "Brk" is the list of characters which may terminate an argument;
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

#include <stdio.h>
#include <c.h>

int strcmp();
char *nxtarg();
int getbool();

int boolarg (ptr,brk,prompt,defalt)
char **ptr,*brk,*prompt;
int defalt;
{
	register char *arg;
	register int valu;

	valu = 2;		/* meaningless value */
	fflush (stdout);

	arg = nxtarg (ptr,brk);	/* parse an argument */
	if (*arg) {		/* there was an argument */
		switch (*arg) {
		case 'n':
		case 'N':
			valu = FALSE;
			break;
		case 'y':
		case 'Y':
			valu = TRUE;
			break;
		case '?':
			break;
		default:
			fprintf (stderr,"%s not 'yes' or 'no'.  ",arg);
		}
	}

	if (valu == 2)  valu = getbool (prompt,defalt);

	return (valu);
}

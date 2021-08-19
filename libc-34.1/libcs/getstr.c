/*  getstr --  prompt user for a string
 *
 *  Usage:  p = getstr (prompt,defalt,answer);
 *	char *p,*prompt,*defalt,*answer;
 *
 *  Getstr prints this message:  prompt  [defalt]
 *  and accepts a line of input from the user.  This line is
 *  entered into "answer", which must be a big char array;
 *  if the user types just carriage return, then the string
 *  "defalt" is copied into answer.
 *  Value returned by getstr is just the same as answer,
 *  i.e. pointer to result string.
 *  The default value is used on error or EOF in the standard input.
 *
 *  HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now uses stderr for output.
 *
 * 23-Oct-82  Steven Shafer (sas) at Carnegie-Mellon University
 *	Added code to copy default to answer (in addition to Fil's code to
 *	return NULL) on error or EOF in the standard input.
 *
 * 21-Oct-80  Fil Alleva (faa) at Carnegie-Mellon University
 *	Getstr() now percuolates any errors from gets() up to the calling
 *	routine.
 *
 * 19-May-80  Steven Shafer (sas) at Carnegie-Mellon University
 *	Increased buffer size to 4000 characters.  Why not?
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX.  Mike thinks a 0 pointer for the default should
 *	print no default (i.e. not even braces); I'm not sure I like the idea
 *	of a routine that doesn't explicitly tell you what happens if you
 *	just hit Carriage Return.
 *
 */

#include <stdio.h>

char *getstr (prompt,defalt,answer)
char *prompt,*defalt,*answer;
{
	char defbuf[4000];
	register char *retval;

	fflush (stdout);
	fprintf (stderr,"%s  [%s]  ",prompt,defalt);
	fflush (stderr);
	strcpy (defbuf,defalt);
	retval = (char *) gets (answer);
	if (retval == NULL || *answer == '\0')  strcpy (answer,defbuf);
	if (retval == NULL)
	    return (retval);
	else
	    return (answer);
}

/*  hexarg  --  parse hexadecimal integer argument
 *
 *  Usage:  i = hexarg (ptr,brk,prompt,min,max,default)
 *    unsigned int i,min,max,default;
 *    char **ptr,*brk,*prompt;
 *
 *  Will attempt to parse an argument from the string pointed to
 *  by "ptr", incrementing ptr to point to the next arg.  If
 *  an arg is found, it is converted into an hexadecimal integer.  If there is
 *  no arg or the value of the arg is not within the range
 *  [min..max], then "gethex" is called to ask the user for an
 *  hexadecimal integer value.
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

#include <ctype.h>
#include <stdio.h>

int gethex();
char *nxtarg();
unsigned int atoh();
int strcmp();

int hexarg (ptr,brk,prompt,min,max,defalt)
char **ptr;
char *brk,*prompt;
unsigned int min,max,defalt;
{
	register unsigned int i;
	register char *arg,*p;

	arg = nxtarg (ptr,brk);
	fflush (stdout);

	if (*arg != '\0') {		/* if there was an arg */
		if (arg[0]=='0' && (arg[1]=='x' || arg[1]=='X'))
			strcpy (arg,arg+2);
		for (p=arg; *p && ((*p >= '0' && *p <= '9') ||
			(*p >= 'a' && *p <= 'f') ||
			(*p >= 'A' && *p <= 'F')); p++);
		if (*p) {
			if (strcmp(arg,"?") != 0)  fprintf (stderr,"%s not hexadecimal number.  ",arg);
		} 
		else {
			i = atoh (arg);
			if (i<min || i>max) {
				fprintf (stderr,"%s%x out of range.  ",
					(i ? "0x" : ""),i);
			}
		}
	}

	if (*arg == '\0' || *p != '\0' || i<min || i>max) {
		i = gethex (prompt,min,max,defalt);
	}

	return (i);
}

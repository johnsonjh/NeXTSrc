/*  searcharg  --  parse string from string table
 *
 *  Usage:  i = searcharg (ptr,brk,prompt,table,defalt);
 *	int i;
 *	char **ptr,*brk,*prompt,**table,*defalt;
 *
 *  Searcharg parses an argument from the string pointed to by "ptr",
 *  and bumps ptr to point to the next argument in the string.
 *  The argument thus parsed is looked up in the string array
 *  "table", and its index is returned.  If the argument is ambiguous
 *  or not found in the table, getsearch() is used to ask the user
 *  for a string and look it up.
 *  "Brk" is the list of characters which may terminate an argument;
 *  if 0, then " " is used.
 *
 *  The string table might be declared this way:
 *    char *table[] {"one","two","three",0};
 *  The last entry must be a zero.
 *
 *  HISTORY
 * 23-Jan-80  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created.
 *
 *
 */

#include <stdio.h>
#include <libc.h>

int getsearch(),stabsearch();
char *nxtarg();

int searcharg (ptr,brk,prompt,table,defalt)
char **ptr,*brk,*prompt,**table,*defalt;
{
	register char *arg;
	register int i;

	arg = nxtarg (ptr,brk);		/* parse an argument */
	fflush (stdout);

	if (*arg) {			/* if there's an argument */
		i = stabsearch (arg,table,0);
	} 
	else {
		i = -1;
	}

	if (i < 0)  i = getsearch (prompt,table,defalt);

	return (i);
}

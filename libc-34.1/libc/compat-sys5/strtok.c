/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Sys5 compat routine
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strtok.c	5.2 (Berkeley) 86/03/09";
#endif

#if	NeXT
#include <string.h>
#endif	NeXT

static char *scanpoint = NULL;

char *strtok(register char *s, register char *delim)
{
	register char *scan;
	char *tok;
	register char *dscan;

	if (s == NULL && scanpoint == NULL)
	      return(NULL);
	if (s != NULL)
	      scan = s;
	else
	      scan = scanpoint;
	
	/*
	 * Scan leading delimiters.
	 */
			
	for (; *scan != '\0'; scan++) {
	      for (dscan = delim; *dscan != '\0'; dscan++)
		      if (*scan == *dscan)
			      break;
	      if (*dscan == '\0')
		      break;
	}
	if (*scan == '\0') {
	      scanpoint = NULL;
	      return(NULL);
	}
	
	tok = scan;
	
	/*
	 * Scan token.
	 */
	for (; *scan != '\0'; scan++) {
	      for (dscan = delim; *dscan != '\0';)    /* ++ moved down. */
		      if (*scan == *dscan++) {
			      scanpoint = scan+1;
			      *scan = '\0';
			      return(tok);
		      }
	}
	
	/*
	* Reached end of string.
	*/
	scanpoint = NULL;
	return(tok);
}

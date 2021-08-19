#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = 	"@(#)fgetpwent.c	1.3 88/05/10 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *  1.6 88/02/08 SMI
 */

#if	NeXT
#include <stdlib.h>
#endif	NeXT
#include <stdio.h>
#include <pwd.h>


extern long strtol();
extern int strlen();
extern char *strncpy();

static char EMPTY[] = "";
static struct passwd *interpret();

#if	NeXT
/*
 * Just like fgets, but ignores comment lines
 */
static char *
striphash_fgets(
		char *buf,
		unsigned size,
		FILE *fptr
		)
{
	char *res;

	for (;;) {
		res = fgets(buf, size, fptr);
		if (res == NULL) {
			return (NULL);
		}
		if (*res != '#') {
			break;
		}
	}
	return (res);
}
/*
 * Make a define so we only have to change it once
 */
#define fgets(buf, size, fptr)  striphash_fgets(buf, size, fptr)

#endif	NeXT


struct passwd *
fgetpwent(f)
	FILE *f;
{
	char line1[BUFSIZ+1];

	if(fgets(line1, BUFSIZ, f) == NULL)
		return(NULL);
	return (interpret(line1, strlen(line1)));
}

static char *
pwskip(p)
	register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p != '\0')
		*p++ = '\0';
	return(p);
}

static struct passwd *
interpret(val, len)
	char *val;
{
	register char *p;
	char *end;
	long x;
	static struct passwd *pwp;
	static char *line;
	register int ypentry;

	if (line == NULL) {
		line = (char *)calloc(1,BUFSIZ+1);
		if (line == NULL)
			return (0);
	}
	if (pwp == NULL) {
		pwp = (struct passwd *)calloc(1, sizeof (*pwp));
		if (pwp == NULL)
			return (0);
	}
	(void) strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	line[len+1] = 0;

	/*
 	 * Set "ypentry" if this entry references the Yellow Pages;
	 * if so, null UIDs and GIDs are allowed (because they will be
	 * filled in from the matching Yellow Pages entry).
	 */
	ypentry = (*p == '+');

	pwp->pw_name = p;
	p = pwskip(p);
	pwp->pw_passwd = p;
	p = pwskip(p);
	if (*p == ':' && !ypentry)
		/* check for non-null uid */
		return (NULL);
	x = strtol(p, &end, 10);
	p = end;
	if (*p++ != ':' && !ypentry)
		/* check for numeric value - must have stopped on the colon */
		return (NULL);
	pwp->pw_uid = x;
	if (*p == ':' && !ypentry)
		/* check for non-null gid */
		return (NULL);
	x = strtol(p, &end, 10);	
	p = end;
	if (*p++ != ':' && !ypentry)
		/* check for numeric value - must have stopped on the colon */
		return (NULL);
	pwp->pw_gid = x;
	pwp->pw_quota = 0;
	pwp->pw_comment = EMPTY;
	pwp->pw_gecos = p;
	p = pwskip(p);
	pwp->pw_dir = p;
	p = pwskip(p);
	pwp->pw_shell = p;
	while(*p && *p != '\n') p++;
	*p = '\0';
	return (pwp);
}

/*
 **********************************************************************
 * HISTORY
 * 05-Mar-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed all special gecos code for 4.2 version.
 *
 * 03-Jan-86  Rudy Nedved (ern) at Carnegie-Mellon University
 *	Basically the entire module was re-written. Parts of the getpwambig
 *	routine are from the old package but that is it. The new code was
 *	written using the CMU TOPS-10 SAIL library algorithm for parsing
 *	account file names. The original motivation for this routine was
 *	based on the visible actions of that algorithm, it might as well
 *	used the actually algorithm.
 *	
 *	Hopefully all the compatibility issues have been handled. The
 *	getpwwho(NULL) behavior is handled and the special loading of
 *	middle names into the first name is handled.
 *	
 *	I would have coded the routine to really handle things in a mail
 *	context but that is suppose to be handled by the network name
 *	database.
 *
 * 06-Jan-84  Eric Patterson (egp) at Carnegie-Mellon University
 *	Fixed bug:  if a string exactly matches some name, but is also a
 *	substring of some other name, no ambiguity is observed.
 *
 * 18-Dec-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed two bugs:
 *	1) adjusted length passed to FoldEQ() to a maximum so that login name
 *	   comparison never succeeds on a partial match.
 *	2) added check for exact match after IsName() succeeds in case the
 *	   full name is a substring of some other full name with which it
 *	   would otherwise be ambiguous.
 *
 * 07-Dec-81  James Gosling (jag) at Carnegie-Mellon University
 *	Now does case-folded compares on login ID's.  eg. "ern" and "ERN"
 *	match.
 *
 * 11-Dec-80  Mike Accetta(mja) at Carnegie-Mellon University
 *	Changed to convert all separator characters to spaces when
 *	copying name into internal buffer so that dots in names with
 *	more than two parts will match password file entries.
 *
 * 03-Dec-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to treat everything before last blank in name as first
 *	name and modified to close password file when called with null
 *	pointer.
 *
 * 29-Nov-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed to allow '.' as separator in names.
 *
 * 07-Nov-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Removed reference to (extinct) alias name.
 *
 **********************************************************************
 */

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
extern struct passwd *getpwent();

#define MAXNAME 100
static char FirstPart[MAXNAME],LastPart[MAXNAME];
static char ThisFirst[MAXNAME],ThisLast[MAXNAME];
static int ExactFirst,ExactLast;
static int MatchLevel,Matches,ambigstate;

static namecheck();

/*
 * break the supplied name down in to last name and first name(s).
 * Translate dots into spaces. Note: middle names are part of the
 * first name.
 */
static BreakDown(name,firsts,last)
char *name,*firsts,*last;
{
    register char *p,*lp;

    strncpy(firsts,name,MAXNAME);
    lp = (char *)0;
    p = firsts;
    while (*p) {
	if (*p == '.' || *p == ' ') {
	    *(lp = p++) = ' ';
	}
	else
	    p++; 
    }
    if (lp != (char *)0) {
	strncpy(last,lp+1,MAXNAME);
	*lp = '\0';
    }
    else {
	strncpy(last,firsts,MAXNAME);
	*firsts = '\0';
    }

}

/*
 * We could just do a getpwuid() at the end of the name search
 * but this causes alot of extra time spent and for mail and finger
 * clients this is highly irritating especially on a loaded machine
 * with many entries in the /etc/passwd file. On of these days, an
 * additional binary index will be available making lookups on uid
 * and login names fast.
 */
static pw_entry_save(bufp,valp)
char **bufp;
register char *valp;
{
    register char *p;

    p = *bufp;
    while ((*p++ = *valp++) != '\0');
    *bufp = p;
}
static struct passwd *copypw(pw)
struct passwd *pw;
{
    static struct passwd mypw;
    static char mybuf[BUFSIZ+1];
    char *p;

    mypw = *pw;	/* copy things like pw_uid, pw_gid, etc. */
    p = mybuf;
    mypw.pw_name = p; pw_entry_save(&p,pw->pw_name);
    mypw.pw_passwd = p; pw_entry_save(&p,pw->pw_passwd);
    mypw.pw_comment = p; pw_entry_save(&p,pw->pw_comment);
    mypw.pw_gecos = p; pw_entry_save(&p,pw->pw_gecos);
    mypw.pw_dir = p; pw_entry_save(&p,pw->pw_dir);
    mypw.pw_shell = p; pw_entry_save(&p,pw->pw_shell);

    return &mypw;
}

static struct passwd *scanpw(stop)
int stop;
{
    struct passwd *bestpw,*pw;

    bestpw = (struct passwd *) 0;

    while (pw = getpwent()) {
	/* handle login ids, exact Matches win always */
	if (*FirstPart == '\0' && ulstrcmp(LastPart,pw->pw_name) == 0) {
	    Matches = 1;
	    MatchLevel = 7777777;	/* large number */
	    return copypw(pw);
	}
	/* check out names */
	BreakDown(pw->pw_gecos,ThisFirst,ThisLast);
	if (namecheck(ThisLast,LastPart,ExactLast)) {
	    /* last match */
	    if (namecheck(ThisFirst,FirstPart,ExactFirst)) {
		/* both match */
		if (!ExactLast)
		    if (ulstrcmp(ThisLast,LastPart) == 0)
			ExactLast = 1;
		if (ExactLast && !ExactFirst)
		    if (ulstrcmp(ThisFirst,FirstPart) == 0)
			ExactFirst = 1;
		/* see if we got something better */
		if (MatchLevel < (ExactFirst + ExactLast)) {
		    MatchLevel = ExactFirst + ExactLast;
		    bestpw = copypw(pw);
		    Matches = 1;
		    if (stop) 
			return bestpw;
		}
		else if (MatchLevel == (ExactFirst + ExactLast)) {
		    Matches++;
		    bestpw = copypw(pw);
		    if (stop) 
			return bestpw;
		}
	    }
	}
    }
    return bestpw;
}

struct passwd  *getpwwho (name)
char   *name;
{
    struct passwd *pw;

    /* initialize a few things */
    MatchLevel = -1;
    Matches = 0;
    ExactFirst = ExactLast = 0;
    ambigstate = -1;

    /* NULL for a name is a no-op...used to clear some internal state */
    if (name == NULL)
	return (struct passwd *) 0;

    /* break down the probe name for comparisons */
    BreakDown(name,FirstPart,LastPart);

    /* now search thru the password file for Matches */
    setpwent();
    pw = scanpw(0);
    endpwent();

    /* see what we got */
    if (Matches == 0)
	return (struct passwd *)0;
    else if (Matches == 1)
	return pw;
    ambigstate = 0;
    return (struct passwd *)-1;
}

struct passwd  *getpwambig ()
{
    struct passwd *pw;

    if (ambigstate < 0)
	return 0;
    if (ambigstate++ == 0) {
	Matches = 0;
	setpwent();
    }
    pw = scanpw(1);
    if (pw == 0) {
	ambigstate = -1;
	endpwent();
    }
    return pw;
}

static namecheck(a,b,exact)
char *a, *b;
int exact;
{
    if (exact) return(ulstrcmp(a,b) == 0);
    if (*b == 0) return 1;
    return(ulstrncmp(a,b,strlen(b)) == 0);
}

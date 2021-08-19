/*
 *  okaccess - validate access to an account
 *
 **********************************************************************
 * HISTORY
 * 23-May-86  Jonathan J. Chew (jjc) at Carnegie-Mellon University
 *	Changed parse routine to use ACCESS_TYPE_RFS instead of
 *	ACCESS_TYPE_REM.
 *
 * 08-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added type checks for REM, REXEC, RSH and RLOGIN.
 *
 * 29-Jul-83  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 *
 *  Call:  val = okaccess(name, flags, uid, dev, ttyloc)
 *
 *  name   = login name to validate
 *  flags  = options (see below)
 *  uid    = user ID attempting access
 *  dev    = device number of controlling terminal attempting access
 *  ttyloc = location of controlling terminal attempting access
 *
 *  Returns TRUE if access is granted to the login name according to
 *  the restrictions recorded in ACCESSFILE given the options, user ID,
 *  and terminal information attempting access, FALSE if access is denied
 *  and CERROR if the access file is missing or poorly formatted.
 */

#include <sys/types.h>
#include <c.h>
#include <stdio.h>
#include <setjmp.h>
#include <sys/ttyloc.h>
#include <access.h>

#define	Static static			/* define empty for SDB debugging */

#define	ACCESSFILE "/etc/access"

Static FILE *af;	/* access file pointer */
Static jmp_buf env;	/* environment for returning from parse errors */
Static long me;		/* INTERNET address of this host */
Static int lastc;	/* look-ahead character from parsing */

Static struct ttyloc g_ttyloc;	/* global terminal location parameter */
Static int g_uid;		/* global user ID parameter */
Static dev_t g_dev;		/* global terminal device number paramater */
Static int g_options;		/* global options parameter (see access.h) */

/*
 *  Note: val must be non-zero
 */
#define	longreturn(val)	longjmp(env, val)



/*
 *  Validate access to account (external interface)
 *
 *     Open the access file.  Scan each entry for the specified login
 *  name and evaluate the access restrictions on each such entry until
 *  access is denied or the end of file is encountered (indicating
 *  no access restrictions).
 */

okaccess(name, options, uid, dev, ttyloc)
char *name;
int options;
int uid;
dev_t dev;
struct ttyloc ttyloc;
{
    int ok = TRUE;

    af=fopen(ACCESSFILE, "r");
    if (af == NULL)
	return(CERROR);

    /*  these are needed globally  */
    g_options = options;
    g_ttyloc = ttyloc;
    g_uid = uid;
    g_dev = dev;

    while (search(name) && ok == TRUE)
    {
	if (setjmp(env) == 0)
	{
	    lastc = 0;
	    ok = parse();
	    if (lastc != ';')
		ok = CERROR;
	}
	else
	    /* parse error */
	    ok = CERROR;
    }
    fclose(af);
    return(ok);

}



/*
 *  Set my Internet address (once-only)
 */

Static
setme()
{

    if (!me)
	me = gethostid();
}

/*
 *  Search access file for specified name
 *
 *  Each entry in the access file is formatted as:
 *
 *  <name>,...,<name>:<access-list>;
 *
 *  (with newlines ignored).  The file is searched for an entry with
 *  a component name which matches the specified name.  If found, the
 *  file is left positioned at the beginning of the access list and TRUE
 *  is returned otherwise the file is left positioned at the end and
 *  FALSE is returned.
 */

Static
search(name)
char *name;
{
    char c;			/* current character from access file */
    int named;			/* found matching name flag */
    int seeking;		/* skipping to next entry flag */
    register char *np;		/* pointer to current character in name */

    np = name;
    named = FALSE;
    seeking = FALSE;
    while ((c=fgetc(af)) != EOF)
    {
	/*
	 *  Avoid processing special characters until we reach the end
	 *  of the previous entry.
	 */
	if (seeking && c != ';')
	    continue;
	switch(c)
	{
	    /*
	     *  name terminator
	     *
	     *  If the name pointer is currently at the end of the name, then
	     *  the specified name matched this entry in the access file (via
	     *  the comparison in the default case below).
	     */
	    case ',':
		if (np && *np == 0)
		    named = TRUE;
		/* fall through */

	    /*
	     *  entry terminator
	     *
	     *  Reset the name pointer to the beginning of the name to start
	     *  comparisons with names in the next entry anew.  We may fall
	     *  through to here from the name terminator case when there are
	     *  multiple names in entry (Note: since the success flag is set
	     *  once a name matches, it doesn't matter that this code is
	     *  executed after both successful and unsuccessful matches).
	     */
	    case ';':
		seeking = 0;
		np = name;
		break;

	    /*
	     *  item separator
	     *
	     *  Match the final name if necessary.  If a match was found in the
	     *  entry, return TRUE with the file positioned at the access list.
	     *  If there was no match in this entry, seek pass the access list
	     *  to the next entry.
	     */
	    case ':':
		if (np && *np == 0)
		    named = TRUE;
		if (named)
		    return(TRUE);
		seeking++;
		np = 0;
		break;

	    /*
	     *  new-line (ignore)
	     */
	    case '\n':
		break;

	    /*
	     *  wildcard name
	     *
	     *  Match any name by artificially setting the name pointer to an
	     *  empty string as if all preceding characters from the name had
	     *  been matched.
	     */
	    case '*':
		if (np == name)
		    np = "";
		break;

	    /*
	     *  any other character
	     *
	     *  Check against current position in name.  If it matches, move
	     *  the name pointer to the next position in anticipation of
	     *  matching the following character.  If it doesn't match, then
	     *  this component of the name list in the entry does not match
	     *  the name, and no further checks need be made until we reach
	     *  the next component or the next entry.
	     */
	    default:
		if (np && c == *np)
		    np++;
		else
		    np = 0;
		break;
	}
    }
    return(FALSE);
}

/*
 *  Parse an access list expression
 *
 *  Elements of the expression may be:
 *
 *  '!' <expression>
 *  '&' '(' <expression-1> ... <expression-n> ')'
 *  '|' '(' <expression-1> ... <expression-n> ')'
 *  'D' <device-number-range>
 *  'H' <host-address-range>
 *  '[LSCFR[MESL]]'
 *  'TH' <terminal-location-host-ID-range>
 *  'TT' <terminal-location-terminal-ID-range>
 *  'U' <user-ID-range>
 *  '{' <comment> '}'
 *  
 *  The syntax is designed to be easily parsed from left to right.
 *
 *  Return the value of the expression or CERROR on a parse error.
 */

Static
parse()
{
    int val = FALSE;

    pwhite();
    switch(pgetc())
    {
	/*
	 *  Should never see these if file is intact.  Reserving the
	 *  the ';' as a unique entry separator which is used nowhere else
	 *  insulates errors in a particular entry from affecting the
	 *  entire file.
	 */
	case ';':
	case EOF:
	    longreturn(CERROR);

	case '!':
	    val = !parse();
	    break;

	case '&':
	    val = pand();
	    break;

	case '|':
	    val = por();
	    break;

	case 'D':
	    val = pdev();
	    break;

	case 'H':
	    val = phost();
	    break;

	case 'L':
	    val = (g_options == ACCESS_TYPE_LOGIN);
	    break;

	case 'S':
	    val = (g_options == ACCESS_TYPE_SU);
	    break;

	case 'F':
	    val = (g_options == ACCESS_TYPE_FTP);
	    break;

	case 'C':
	    val = (g_options == ACCESS_TYPE_CMUFTP);
	    break;

	case 'R':
	    switch (pgetc()) {
	    case 'M':
		val = (g_options == ACCESS_TYPE_RFS);
		break;
	    case 'E':
		val = (g_options == ACCESS_TYPE_REXEC);
		break;
	    case 'S':
		val = (g_options == ACCESS_TYPE_RSH);
		break;
	    case 'L':
		val = (g_options == ACCESS_TYPE_RLOGIN);
		break;
	    default:
		longreturn(CERROR);
	    }
	    break;

	case 'T':
	    val = ptloc();
	    break;

	case 'U':
	    val = puid();
	    break;

	default:
	    longreturn(CERROR);
    }
    pwhite();
    return(val);
}

/*
 *  Parse a comment
 *
 *  Skip all characters in access file until '}', ';' or EOF.
 */

Static
pcomment()
{
    int c;

    while ((c=pgetc()) != '}' && c != ';' && c != EOF);
    if (c != '}')
	lastc = c;
}



/*
 *  Parse a number.
 *
 *  radix = 10 for decimal, 16 for hexadecimal
 *
 *  Return the accumulated number or to the top level with a parse
 *  error if no number is found.
 */

Static
unsigned long
pnum(radix)
{
    unsigned long num = 0;
    int len = 0;
    int c;

    c = pgetc();
    switch (c)
    {
	case '#':
	    radix = 8;
	    break;
	case '%':
	    radix = 10;
	    break;
	case '$':
	    radix = 16;
	    break;
	default:
	    lastc = c;
	    break;
    }

    while (((c=pgetc()) >= '0' && c <= '9') || (radix == 16 && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))))
    {
	if (c >= 'a')
	    c += 'A'-'a';
	if (c >= 'A')
	    c += '9'+1-'A';
	num = num*radix + c - '0';
	len++;
    }
    if (len == 0)
	longreturn(CERROR);
    lastc = c;
    return(num);
}

/*
 *  Parse a conjunctive expression.
 *
 *  The next token from the file must be a '('.  Expressions are
 *  parsed and conjunctively combined until a ')' is encountered.
 *
 *  Return the value of the expression combination.
 */

Static
pand()
{
    int val = TRUE;

    pwhite();
    if (lastc != '(')
	longreturn(CERROR);
    else
	lastc = 0;
    pwhite();
    while(lastc != ')')
	val = parse() && val;
    lastc = 0;
    pwhite();
    return(val);
}



/*
 *  Parse a disjunctive expression
 *
 *  The next token from the file must be a '('.  Expressions are
 *  parsed and disjunctively combined until a ')' is encountered.
 *
 *  Return the value of the expression combination.
 */

Static
por()
{
    int val = FALSE;

    pwhite();
    if (lastc != '(')
	longreturn(CERROR);
    else
	lastc = 0;
    pwhite();
    while(lastc != ')')
	val = parse() || val;
    lastc = 0;
    pwhite();
    return(val);
}

/*
 *  Parse a numeric range
 *
 *  radix = numeric radix passed to pnum()
 *
 *  The format of the range is:
 *
 *  <number>-<number>
 *
 *  There may be no white space between the first number and the '-'
 *  if it exists.  If the '-' is missing, the range consists of the
 *  single number as both low and high components.
 *  
 *  Return a pointer to a two-longword array of the low and high elements
 *  of the range.
 */
Static
unsigned long *
prange(radix)
{
    static unsigned long n[2];

    n[0] = pnum(radix);
    if (lastc == '-')
    {
	lastc = 0;
	n[1] = pnum(radix);
    }
    else
	n[1] = n[0];
    return(n);
}

/*
 *  Parse a host range
 *
 *  The host range is an (optional) pair of hexadecimal numbers:
 *
 *	<host-low>['-'<host-high>]
 *
 *  The high endpoint of the range may be omitted if it is identical to
 *  the low endpoint.
 *
 *  Return TRUE if my host address is in the range (inclusive of
 *  the endpoints) otherwise return FALSE.
 */

Static
phost()
{
    unsigned long *host;

    setme();
    pwhite();
    host = prange(16);
    return (me >= host[0] && me <= host[1]);
}



/*
 *  Parse a user ID range
 *
 *  The user ID range is an (optional) pair of decimal numbers:
 *
 *	<UID-low>['-'<UID-high>]
 *
 *  The high endpoint of the range may be omitted if it is identical to
 *  the low endpoint.
 *
 *  Return TRUE if the user ID parameter is in the range (inclusive of
 *  the endpoints) otherwise return FALSE.
 */

Static
puid()
{
    unsigned long *id;

    pwhite();
    id = prange(10);
    return (g_uid >= id[0] && g_uid <= id[1]);
}

/*
 *  Parse a device number range
 *
 *  The device number range is an (optional) pair of hexadecimal numbers:
 *
 *	<device-low>['-'<device-high>]
 *
 *  The high endpoint of the range may be omitted if it is identical to
 *  the low endpoint.
 *
 *  Return TRUE if the device number parameter is in the range (inclusive of
 *  the endpoints) otherwise return FALSE.
 */

Static
pdev()
{
    unsigned long *dev;

    pwhite();
    dev = prange(16);
    return (g_dev >= dev[0] && g_dev <= dev[1]);
}

/*
 *  Parse a terminal location range
 *
 *  If the next character is a 'H', then the range refers to the host
 *  identifier portion of the terminal location.  If the next character
 *  is a 'T', then the range refers to the terminal identifier portion
 *  of the terminal location.
 *  The terminal location is a (optional) pair hexadecimal numbers:
 *
 *	<value-low>['-'<value-high>]
 *
 *  The high endpoint of the range may be omitted if it is identical to
 *  the low endpoint.  The special host range consisting of the single
 *  character '.' represents the current host.
 *
 *  Return TRUE if the indicated component of the terminal location
 *  parameter lies within the range (inclusive of the endpoints) otherwise
 *  return FALSE.
 */
Static
ptloc()
{
    unsigned long *val;
    unsigned long dothost[2];
    unsigned long *tty;

    switch (pgetc())
    {
	case 'H':
	    pwhite();
	    if (lastc == '.')
	    {
		lastc = 0;
		lastc = pgetc();
		setme();
		dothost[0] = dothost[1] = me;
		val = dothost;
	    }
	    else
		val = prange(16);
	    tty = (unsigned long *)&g_ttyloc.tlc_hostid;
	    break;
	case 'T':
	    pwhite();
	    val = prange(16);
	    tty = (unsigned long *)&g_ttyloc.tlc_ttyid;
	    break;
	default:
	    longreturn(CERROR);
    }
    return (*tty >= val[0] && *tty <= val[1]);
}

/*
 *  Parse white space
 *
 *  Skip past any comments or space, tab or newline characters in
 *  the access file.
 */

Static
pwhite()
{
    int c;

    while ((c=pgetc()) == ' ' || c == '\t' || c == '\n' || c == '{')
	if (c == '{')
	    pcomment();
    lastc = c;
}



/*
 *  Get character from access file.
 *
 *  If the previously read character terminated some previous token
 *  but was not itself parsed, return this look-ahead character after clearing
 *  the lookahead indication.  The look-ahead character will be set when the
 *  processing of a particular parsing routine uses it to terminate the token
 *  being assembling but but without including it in the result.  If there
 *  is no look-ahead character, read and return the next character from the
 *  access file.
 */

Static
pgetc()
{
    int c;

    if (lastc)
    {
	c = lastc;
	lastc = 0;
    }
    else
	c = fgetc(af);
    return(c);
}

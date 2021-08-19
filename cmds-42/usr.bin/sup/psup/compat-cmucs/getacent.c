/*
 **********************************************************************
 * HISTORY
 * 12-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed setacent() to return open or rewind errors.  Changed
 *	ac_passwd to ac_attrs for new file format.
 *
 * 14-Jun-82  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created from getpwent();  fixed to terminate on new-line and parse
 *	over any extra trailing fields.
 *
 **********************************************************************
 */

#include <stdio.h>
#include <sys/types.h>
#include <acc.h>

#define MAXACTATTRS 32

static char ACCOUNT[]	= "/etc/account";
static FILE *acf = NULL;
static char line[BUFSIZ+1];
static struct account account;
static char *ac_attrs[MAXACTATTRS];

setacent()
{

    if( acf == NULL )
	acf = fopen( ACCOUNT, "r" );
    else
	rewind( acf );
    return( acf != NULL );
}

endacent()
{

    if( acf != NULL )
    {
	fclose( acf );
	acf = NULL;
    }

}

static char *
acskip(p,c)
register char *p;
register char c;
{

    while( *p && *p != c ) ++p;
    if( *p ) *p++ = 0;
    return(p);

}

struct account *
getacent()
{

    register char *p, *a, **q;

    if (acf == NULL)
    {
	if( (acf = fopen( ACCOUNT, "r" )) == NULL )
	    return(0);
    }
    p = fgets(line, BUFSIZ, acf);
    if (p==NULL)
	return(0);
    acskip(p,'\n');
    account.ac_name = p;
    a = acskip(p,':');
    p = acskip(a,':');
    account.ac_attrs = ac_attrs;
    q = ac_attrs;
    while( *a ) {
	if (q < &ac_attrs[MAXACTATTRS-1])
	    *q++ = a;
	a = acskip(a,',');
    }
    *q = NULL;
    account.ac_uid = atoi(p);
    p = acskip(p,':');
    account.ac_aid = atoi(p);
    p = acskip(p,':');
    account.ac_created = p;
    p = acskip(p,':');
    account.ac_expires = p;
    p = acskip(p,':');
    account.ac_sponsor = p;
    while(*p) p = acskip(p,':');
    /*
     *  Can't afford the expense of calling atot() for each line so
     *  for now these get initialize to zero.
     */
    account.ac_ctime = 0;
    account.ac_xtime = 0;
    return(&account);

}

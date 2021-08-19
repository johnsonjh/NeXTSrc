/*
 **********************************************************************
 * HISTORY
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */
#include "sup.h"
#define MSGSUBR
#define MSGFILE
#include "supmsg.h"

extern char	*crypttest;		/* encryption test string */

int msgfcrypt ()
{
	if (server)
		return (readmstr (MSGFCRYPT,&crypttest));
	return (writemstr (MSGFCRYPT,crypttest));
}

int msgfcryptok ()
{
	if (server)
		return (writemnull (MSGFCRYPTOK));
	return (readmnull (MSGFCRYPTOK));
}

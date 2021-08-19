/*
 **********************************************************************
 * HISTORY
 * 04-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed name server protocol to support multiple repositories
 *	per collection.
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */
#include <stdio.h>
#include <c.h>
#include "sup.h"
#define MSGSUBR
#define MSGNAME
#include "supmsg.h"

extern TREE *hostT;			/* host names of repositories */

int msgncoll ()
{
	if (server)
		return (readmstr (MSGNCOLL,&collname));
	return (writemstr (MSGNCOLL,collname));
}

static int hostone (t)
register TREE *t;
{
	return (writestring (t->Tname));
}

int msgnhost ()
{
	register int x;

	if (server) {
		if (protver < 5)
			return (writemstr (MSGNHOST,hostT->Tname));
		x = writemsg (MSGNHOST);
		if (x == SCMOK)  x = Tprocess (hostT,hostone);
		if (x == SCMOK)  x = writestring ((char *)NULL);
		if (x == SCMOK)  x = writemend ();
	} else {
		char *host;

		if (protver < 5) {
			x = readmstr (MSGNHOST,&host);
			if (x == SCMOK && host != NULL)
				(void) Tinsert (&hostT,host,FALSE);
			return (x);
		}
		x = readmsg (MSGNHOST);
		if (x == SCMOK)  x = readstring (&host);
		while (x == SCMOK) {
			if (host == NULL)  break;
			(void) Tinsert (&hostT,host,FALSE);
			free (host);
			x = readstring (&host);
		}
		if (x == SCMOK)  x = readmend ();
	}
	return (x);
}

/*
 **********************************************************************
 * HISTORY
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */
#include <stdio.h>
#include <c.h>
#include "sup.h"
#define MSGSUBR
#define MSGFILE
#include "supmsg.h"

extern TREE	*denyT;			/* tree of files to deny */

static int denyone (t)
register TREE *t;
{
	return (writestring (t->Tname));
}

int msgfdeny ()
{
	register int x;
	if (server) {
		x = writemsg (MSGFDENY);
		if (x == SCMOK)  x = Tprocess (denyT,denyone);
		if (x == SCMOK && protver >= 4)  x = writestring ((char *)NULL);
		if (x == SCMOK)  x = writemend ();
	} else {
		char *name;
		x = readmsg (MSGFDENY);
		if (x == SCMOK)  x = readstring (&name);
		while (x == SCMOK) {
			if (name == NULL)  break;
			(void) Tinsert (&denyT,name,FALSE);
			free (name);
			x = readstring (&name);
		}
		if (x == SCMOK)  x = readmend ();
	}
	return (x);
}

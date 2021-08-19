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

extern TREE	*refuseT;		/* tree of files to refuse */

static int refuseone (t)
register TREE *t;
{
	return (writestring (t->Tname));
}

int msgfrefuse ()
{
	register int x;
	if (server) {
		char *name;
		x = readmsg (MSGFREFUSE);
		if (protver < 4) {
			if (x == SCMOK)  x = readmend ();
			return (x);
		}
		if (x == SCMOK)  x = readstring (&name);
		while (x == SCMOK) {
			if (name == NULL)  break;
			(void) Tinsert (&refuseT,name,FALSE);
			free (name);
			x = readstring (&name);
		}
		if (x == SCMOK)  x = readmend ();
	} else {
		x = writemsg (MSGFREFUSE);
		if (x == SCMOK)  x = Tprocess (refuseT,refuseone);
		if (x == SCMOK)  x = writestring ((char *)NULL);
		if (x == SCMOK)  x = writemend ();
	}
	return (x);
}

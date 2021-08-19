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

extern TREE	*needT;			/* tree of files to need */

extern int stringend;			/* version 3 */

static int needone (t)
register TREE *t;
{
	register int x;
	x = writestring (t->Tname);
	if (x == SCMOK)  x = writeint ((t->Tflags&FUPDATE) != 0);
	return (x);
}

int msgfneed ()
{
	register int x;
	if (server) {
		char *name;
		int update;
		register TREE *t;
		x = readmsg (MSGFNEED);
		if (x == SCMOK)  x = readstring (&name);
		while (x == SCMOK) {
			if (protver < 4 && name == (char *)(-1))
				return (SCMOK);
			if (name == NULL)  break;
			x = readint (&update);
			if (x != SCMOK)  break;
			t = Tinsert (&needT,name,TRUE);
			free (name);
			if (update)  t->Tflags |= FUPDATE;
			x = readstring (&name);
		}
		if (x == SCMOK)  x = readmend ();
	} else {
		x = writemsg (MSGFNEED);
		if (x == SCMOK)  x = Tprocess (needT,needone);
		if (x == SCMOK)  x = writestring ((char *)NULL);
		if (x == SCMOK)  x = writemend ();
	}
	return (x);
}

/*
 **********************************************************************
 * HISTORY
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <c.h>
#include "sup.h"
#define MSGSUBR
#define MSGFILE
#include "supmsg.h"

extern TREE	*listT;			/* tree of files to list */
extern int	scantime;		/* time that collection was scanned */

static int namecount;			/* version 3 */

static int listone (t)			/* version 3 */
register TREE *t;
{
	register int x;

	if (protver < 4) {
		register int flags;

		flags = 0;
		if (t->Tflags&FNEW)  flags |= ALLNEW;
		if (t->Tflags&FBACKUP)  flags |= ALLBACKUP;
		if (t->Tflags&FNOACCT)  flags |= ALLNOACCT;
		if ((t->Tmode&S_IFMT) == S_IFLNK)  flags |= ALLSLINK;
		if ((t->Tmode&S_IFMT) == S_IFDIR)  flags |= ALLDIR;
		x = writeint (flags);
		if (x == SCMOK)  x = writestring (t->Tname);
	} else {
		x = writestring (t->Tname);
		if (x == SCMOK)  x = writeint ((int)t->Tmode);
		if (x == SCMOK)  x = writeint ((int)t->Tflags);
	}
	if (x == SCMOK)  x = writeint (t->Tmtime);
	if (x == SCMOK && protver < 4 && ++namecount >= BLOCKALL) {
		x = writemend ();
		if (x == SCMOK)  x = msgfrefuse ();
		if (x != SCMOK)  goaway ("Didn't receive request for filename list");
		x = writemsg (MSGFLIST);
		namecount = 0;
	}
	return (x);
}

int msgflist ()
{
	register int x;
	if (server) {
		if (protver < 4)  namecount = 0;
		x = writemsg (MSGFLIST);
		if (x == SCMOK)  x = Tprocess (listT,listone);
		if (x == SCMOK) {
			if (protver < 4) {
				x = ALLEND;
				x = writeint (x);
			} else
				x = writestring ((char *)NULL);
		}
		if (x == SCMOK)  x = writeint (scantime);
		if (x == SCMOK)  x = writemend ();
	} else {
		char *name;
		int mode,flags,mtime;
		register TREE *t;
		x = readmsg (MSGFLIST);
		if (x == SCMOK)  x = readstring (&name);
		while (x == SCMOK) {
			if (name == NULL)  break;
			x = readint (&mode);
			if (x == SCMOK)  x = readint (&flags);
			if (x == SCMOK)  x = readint (&mtime);
			if (x != SCMOK)  break;
			t = Tinsert (&listT,name,TRUE);
			free (name);
			t->Tmode = mode;
			t->Tflags = flags;
			t->Tmtime = mtime;
			x = readstring (&name);
		}
		if (x == SCMOK)  x = readint (&scantime);
		if (x == SCMOK)  x = readmend ();
	}
	return (x);
}

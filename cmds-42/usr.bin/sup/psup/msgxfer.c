/*
 **********************************************************************
 * HISTORY
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */
#include <stdio.h>
#include <sys/types.h>			/* version 3 */
#include <sys/stat.h>			/* version 3 */
#include <c.h>
#include "sup.h"
#define MSGSUBR
#define MSGFILE
#include "supmsg.h"

extern TREE	*upgradeT;		/* pointer to file being upgraded */

int msgfsend ()
{
	if (server)
		return (readmnull (MSGFSEND));
	return (writemnull (MSGFSEND));
}

static int oldlinkone (t,linkname)
register TREE *t;
char **linkname;
{
	register int x;

	if ((x = msgfsend ()) != SCMOK)  return (x);
	if ((x = writemsg (MSGFRECV)) != SCMOK)  return (x);
	if ((x = writestring (t->Tname)) != SCMOK)  return (x);
	x = MODELINK;
	if (t->Tflags&FNOACCT) x |= MODENOACCT;
	if ((x = writeint (x)) != SCMOK)  return (x);
	if ((x = writestring (*linkname)) != SCMOK)  return (x);
	return (writemend ());
}

static int writeone (t)
register TREE *t;
{
	return (writestring (t->Tname));
}

/* VARARGS1 */
int msgfrecv (xferfile,args)
int (*xferfile) ();
int args;
{
	register int x;
	register TREE *t = upgradeT;
	if (server) {
		x = writemsg (MSGFRECV);
		if (t == NULL) {
			if (protver < 4) {
				if (x == SCMOK)  x = writemend ();
				return (x);
			}
			if (x == SCMOK)  x = writestring ((char *)NULL);
			if (x == SCMOK)  x = writemend ();
			return (x);
		}
		if (x == SCMOK)  x = writestring (t->Tname);
		if (protver < 4) {
			int mode;
			if (x != SCMOK)  return (x);
			mode = t->Tmode&S_IMODE;
			if ((t->Tmode&S_IFMT) == S_IFLNK)  mode |= MODESYM;
			if ((t->Tmode&S_IFMT) == S_IFDIR)  mode |= MODEDIR;
			if (t->Tflags&FNOACCT) x |= MODENOACCT;
			if (t->Tflags&FUPDATE) x |= MODEUPDATE;
			x = writeint (mode);
			if (x == SCMOK)  x = writestring (t->Tuser);
			if (x == SCMOK)  x = writestring (t->Tgroup);
			if (x == SCMOK)  x = writeint (t->Tmtime); /* fake atime */
			if (x == SCMOK)  x = writeint (t->Tmtime);
			if (x != SCMOK)  return (x);
			if (mode&MODESYM)  x = writestring (t->Tlink->Tname);
			if (x == SCMOK)  x = (*xferfile) (t,&args);
			if (x == SCMOK)  x = writemend ();
			if (x == SCMOK)  t->Tflags |= FCOMPAT;
			if (x != SCMOK || (t->Tmode&S_IFMT) != S_IFREG || t->Tlink == NULL)
				return (x);
			x = Tprocess (t->Tlink,oldlinkone,t->Tname);
			return (x);
		}
		if (x == SCMOK)  x = writeint (t->Tmode);
		if (t->Tmode == 0) {
			if (x == SCMOK)  x = writemend ();
			return (x);
		}
		if (x == SCMOK)  x = writeint (t->Tflags);
		if (x == SCMOK)  x = writestring (t->Tuser);
		if (x == SCMOK)  x = writestring (t->Tgroup);
		if (x == SCMOK)  x = writeint (t->Tmtime);
		if (x == SCMOK)  x = Tprocess (t->Tlink,writeone);
		if (x == SCMOK)  x = writestring ((char *)NULL);
		if (x == SCMOK)  x = Tprocess (t->Texec,writeone);
		if (x == SCMOK)  x = writestring ((char *)NULL);
		if (x == SCMOK)  x = (*xferfile) (t,&args);
		if (x == SCMOK)  x = writemend ();
	} else {
		char *linkname,*execcmd;
		if (t == NULL)  return (SCMERR);
		x = readmsg (MSGFRECV);
		if (x == SCMOK)  x = readstring (&t->Tname);
		if (x == SCMOK && t->Tname == NULL) {
			x = readmend ();
			if (x == SCMOK)  x = (*xferfile) (NULL,&args);
			return (x);
		}
		if (x == SCMOK)  x = readint (&t->Tmode);
		if (t->Tmode == 0) {
			x = readmend ();
			if (x == SCMOK)  x = (*xferfile) (t,&args);
			return (x);
		}
		if (x == SCMOK)  x = readint (&t->Tflags);
		if (x == SCMOK)  x = readstring (&t->Tuser);
		if (x == SCMOK)  x = readstring (&t->Tgroup);
		if (x == SCMOK)  x = readint (&t->Tmtime);
		t->Tlink = NULL;
		if (x == SCMOK)  x = readstring (&linkname);
		while (x == SCMOK) {
			if (linkname == NULL)  break;
			(void) Tinsert (&t->Tlink,linkname,FALSE);
			free (linkname);
			x = readstring (&linkname);
		}
		t->Texec = NULL;
		if (x == SCMOK)  x = readstring (&execcmd);
		while (x == SCMOK) {
			if (execcmd == NULL)  break;
			(void) Tinsert (&t->Texec,execcmd,FALSE);
			free (execcmd);
			x = readstring (&execcmd);
		}
		if (x == SCMOK)  x = (*xferfile) (t,&args);
		if (x == SCMOK)  x = readmend ();
	}
	return (x);
}

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

extern char	*basedir;		/* base directory */
extern int	basedev;		/* base directory device */
extern int	baseino;		/* base directory inode */
extern int	lasttime;		/* time of last upgrade */
extern int	listonly;		/* only listing files, no data xfer */
extern int	newonly;		/* only send new files */
extern int	setupack;		/* ack return value for setup */

int msgfsetup ()
{
	register int x;
	if (server) {
		x = readmsg (MSGFSETUP);
		if (x == SCMOK)  x = readstring (&collname);
		if (x == SCMOK)  x = readint (&lasttime);
		if (x == SCMOK)  x = readstring (&basedir);
		if (x == SCMOK)  x = readint (&basedev);
		if (x == SCMOK)  x = readint (&baseino);
		if (x == SCMOK)  x = readint (&listonly);
		if (x == SCMOK)  x = readint (&newonly);
		if (x == SCMOK)  x = readmend ();
	} else {
		x = writemsg (MSGFSETUP);
		if (x == SCMOK)  x = writestring (collname);
		if (x == SCMOK)  x = writeint (lasttime);
		if (x == SCMOK)  x = writestring (basedir);
		if (x == SCMOK)  x = writeint (basedev);
		if (x == SCMOK)  x = writeint (baseino);
		if (x == SCMOK)  x = writeint (listonly);
		if (x == SCMOK)  x = writeint (newonly);
		if (x == SCMOK)  x = writemend ();
	}
	return (x);
}

int msgfsetupack ()
{
	if (server)
		return (writemint (MSGFSETUPACK,setupack));
	return (readmint (MSGFSETUPACK,&setupack));
}

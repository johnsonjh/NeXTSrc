/*
 **********************************************************************
 * HISTORY
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */
#include <c.h>
#include "sup.h"
#define MSGSUBR
#define MSGNAME
#define MSGFILE
#include "supmsg.h"

extern int	pgmver;			/* program version of partner */
extern int	pgmversion;		/* my program version */
extern char	*scmver;		/* scm version of partner */
extern int	fspid;			/* process id of fileserver */
extern int	nspid;			/* process id of nameserver */

int msgsignon (namesrv)
int namesrv;
{
	register int x;

	if (server) {
		x = readmsg (namesrv ? MSGNSIGNON : MSGFSIGNON);
		if (x != SCMOK) {
			printf("Error reading signon message code\n");
			return (x);
		}
		if (x == SCMOK)  x = readint (&protver);
		if (x != SCMOK) {
			printf("Error reading signon protocol version\n");
			return (x);
		}
		if (x == SCMOK)  x = readint (&pgmver);
		if (x != SCMOK) {
			printf("Error reading signon program version\n");
			return (x);
		}
		if (x == SCMOK)  x = readstring (&scmver);
		if (x != SCMOK) {
			printf("Error reading signon communications version\n");
			return (x);
		}
		if (x == SCMOK)  x = readmend ();
		if (x != SCMOK) {
			printf("Error reading signon message end\n");
			return (x);
		}
	} else {
		x = writemsg (namesrv ? MSGNSIGNON : MSGFSIGNON);
		if (x != SCMOK) {
			printf("Error writing signon message code\n");
			return (x);
		}
		if (x == SCMOK)  x = writeint (PROTOVERSION);
		if (x != SCMOK) {
			printf("Error writing signon protocol version\n");
			return (x);
		}
		if (x == SCMOK)  x = writeint (pgmversion);
		if (x != SCMOK) {
			printf("Error writing signon program version\n");
			return (x);
		}
		if (x == SCMOK)  x = writestring (scmversion);
		if (x != SCMOK) {
			printf("Error writing signon communications version\n");
			return (x);
		}
		if (x == SCMOK)  x = writemend ();
		if (x != SCMOK) {
			printf("Error writing signon message end\n");
			return (x);
		}
	}
	return (x);
}

int msgsignonack (namesrv)
int namesrv;
{
	register int x;

	if (server) {
		x = writemsg (namesrv ? MSGNSIGNONACK : MSGFSIGNONACK);
		if (x != SCMOK) {
			printf("Error writing signonack message code\n");
			return (x);
		}
		if (x == SCMOK)  x = writeint (PROTOVERSION);
		if (x != SCMOK) {
			printf("Error writing signonack protocol version\n");
			return (x);
		}
		if (x == SCMOK)  x = writeint (pgmversion);
		if (x != SCMOK) {
			printf("Error writing signonack program version\n");
			return (x);
		}
		if (x == SCMOK)  x = writestring (scmversion);
		if (x != SCMOK) {
			printf("Error writing signonack communications version\n");
			return (x);
		}
		if (x == SCMOK)  x = writeint (namesrv ? nspid : fspid);
		if (x != SCMOK) {
			printf("Error writing signonack process id\n");
			return (x);
		}
		if (x == SCMOK)  x = writemend ();
		if (x != SCMOK) {
			printf("Error writing signonack message end\n");
			return (x);
		}
	} else {
		x = readmsg (namesrv ? MSGNSIGNONACK : MSGFSIGNONACK);
		if (x != SCMOK) {
			printf("Error reading signonack message code\n");
			return (x);
		}
		if (x == SCMOK)  x = readint (&protver);
		if (x != SCMOK) {
			printf("Error reading signonack protocol version\n");
			return (x);
		}
		if (x == SCMOK)  x = readint (&pgmver);
		if (x != SCMOK) {
			printf("Error reading signonack program version\n");
			return (x);
		}
		if (x == SCMOK)  x = readstring (&scmver);
		if (x != SCMOK) {
			printf("Error reading signonack communications version\n");
			return (x);
		}
		if (x == SCMOK)  x = readint (namesrv ? &nspid : &fspid);
		if (x != SCMOK) {
			printf("Error reading signonack process id\n");
			return (x);
		}
		if (x == SCMOK)  x = readmend ();
		if (x != SCMOK) {
			printf("Error reading signonack message end\n");
			return (x);
		}
	}
	return (x);
}

int msgnsignon ()
{
	return (msgsignon (TRUE));
}

int msgnsignonack ()
{
	return (msgsignonack (TRUE));
}

int msgfsignon ()
{
	return (msgsignon (FALSE));
}

int msgfsignonack ()
{
	return (msgsignonack (FALSE));
}

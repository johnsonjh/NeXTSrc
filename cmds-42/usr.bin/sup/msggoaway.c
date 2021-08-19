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
#include "supmsg.h"

extern char	*goawayreason;		/* reason for goaway */

int msggoaway ()
{
	return (writemstr (MSGGOAWAY,goawayreason));
}

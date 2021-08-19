/*	@(#)setbrk.c	1.4	*/
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#ifdef	MACH
/*
 * Not needed.
 */
#else	MACH
#include	"defs.h"

char 	*sbrk();

char*
setbrk(incr)
{

	register char *a = sbrk(incr);

	brkend = a + incr;
	return(a);
}
#endif	MACH


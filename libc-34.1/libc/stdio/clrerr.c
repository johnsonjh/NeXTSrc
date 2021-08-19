#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)clrerr.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>

#undef	clearerr
void clearerr(register FILE *p)
{
	p->_flag &= ~(_IOERR|_IOEOF);
}


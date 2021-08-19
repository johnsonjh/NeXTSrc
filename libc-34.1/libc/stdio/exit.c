#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)exit.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#include <stdlib.h>

extern void _atexit(void);

#undef exit
void
exit(int code) {

	_atexit();
	_cleanup();
	_exit(code);
}

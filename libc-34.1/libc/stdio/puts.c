#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)puts.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>

#undef puts
int
puts(register const char *s) {
	register int c;

	while (c = *s++)
		putchar(c);
	return(putchar('\n'));
}

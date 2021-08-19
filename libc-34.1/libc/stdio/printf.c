/*
 * Copyright (c) 1987, NeXT, INC.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)printf.c	1.0 (NeXT) July 10, 1987"
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>
#include	<stdarg.h>

printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_doprnt(fmt, ap, stdout);
	va_end(ap);
	return(ferror(stdout)? EOF: 0);
}

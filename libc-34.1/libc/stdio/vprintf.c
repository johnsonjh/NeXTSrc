/*
 * Copyright (c) 1988, NeXT, INC.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)vprintf.c	1.0 (NeXT) July 10, 1988"
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>
#include	<stdarg.h>

vprintf(const char *fmt, va_list ap)
{

	_doprnt(fmt, ap, stdout);
	return(ferror(stdout)? EOF: 0);
}

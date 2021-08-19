/*
 * Copyright (c) 1988 NeXT, INC
 */
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)vsprintf.c	1.0 (NeXT) July 12, 1988"
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>
#include	<stdarg.h>
#if	NeXT
#include	<math.h>
#endif	NeXT

char *bsd_vsprintf(char *str, const char *fmt, va_list ap)
{
	FILE _strbuf;

	_strbuf._flag = _IOWRT+_IOSTRG;
	_strbuf._ptr = str;
#if	NeXT
	_strbuf._cnt = MAXINT;
#else
	_strbuf._cnt = 32767;
#endif	NeXT
	_doprnt(fmt, ap, &_strbuf);
	putc('\0', &_strbuf);
	return (str);
}

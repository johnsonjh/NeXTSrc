/*
 * Copyright (c) 1987 NeXT, INC
 */
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)bsd_sprintf.c	1.0 (NeXT) July 10, 1987"
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>
#include	<stdarg.h>
#if	NeXT
#include	<math.h>
#endif	NeXT

char * 
bsd_sprintf(char *str, const char *fmt, ...)
{
	FILE _strbuf;
	va_list ap;

	va_start(ap, fmt);
	_strbuf._flag = _IOWRT+_IOSTRG;
	_strbuf._ptr = str;
#if	NeXT
	_strbuf._cnt = MAXINT;
#else
	_strbuf._cnt = 32767;
#endif	NeXT
	_doprnt(fmt, ap, &_strbuf);
	putc('\0', &_strbuf);
	va_end(ap);
	return(str);
}

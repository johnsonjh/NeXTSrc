#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)scanf.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>
#include	<stdarg.h>

int
scanf(const char *fmt, ...)
{
	va_list ap;
	int ret;
		
	va_start(ap, fmt);
	ret = _doscan(stdin, fmt, ap);
	va_end(ap);
	return ret;
}

int
fscanf(FILE *iop, const char *fmt, ...)
{
	va_list ap;
	int ret;
	
	va_start(ap, fmt);
	ret = _doscan(iop, fmt, ap);
	va_end(ap);
	return ret;
}

int
sscanf(register const char *str, const char *fmt, ...)
{
	FILE _strbuf;
	va_list ap;
	int ret;
	
	va_start(ap, fmt);
	_strbuf._flag = _IOREAD|_IOSTRG;
	_strbuf._ptr = _strbuf._base = (char *)str;
	_strbuf._cnt = 0;
	while (*str++)
		_strbuf._cnt++;
	_strbuf._bufsiz = _strbuf._cnt;
	ret = _doscan(&_strbuf, fmt, ap);
	va_end(ap);
	return ret;
}

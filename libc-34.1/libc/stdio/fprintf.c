/*
 * Copyright (c) 1987 NeXT, INC.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)fprintf.c	1.0 (NeXT) July 10, 1987"
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>
#include	<stdarg.h>

fprintf(register FILE *iop, const char *fmt, ...)
{
	char localbuf[BUFSIZ];
	va_list ap;

	va_start(ap, fmt);
	if (iop->_flag & _IONBF) {
		iop->_flag &= ~_IONBF;
		iop->_ptr = iop->_base = localbuf;
		iop->_bufsiz = BUFSIZ;
		_doprnt(fmt, ap, iop);
		fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = 0;
		iop->_cnt = 0;
	} else
		_doprnt(fmt, ap, iop);
	va_end(ap);
	return(ferror(iop)? EOF: 0);
}

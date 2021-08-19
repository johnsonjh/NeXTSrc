/*
 * Copyright (c) 1988 NeXT, INC.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)vfprintf.c	1.0 (NeXT) July 12, 1988"
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>
#include	<stdarg.h>

vfprintf(register FILE *iop, const char *fmt, va_list ap)
{
	char localbuf[BUFSIZ];

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
	return(ferror(iop)? EOF: 0);
}

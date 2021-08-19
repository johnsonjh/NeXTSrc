/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)fread.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#include	<stdio.h>
#include	<string.h>

size_t fread(register void *ptr, size_t size, size_t count, register FILE *iop)
{
	register int s;
	int c;

	s = size * count;
	while (s > 0) {
		if (iop->_cnt < s) {
			if (iop->_cnt > 0) {
				memcpy(ptr, iop->_ptr, iop->_cnt);
				((char *)ptr) += iop->_cnt;
				s -= iop->_cnt;
			}
			/*
			 * filbuf clobbers _cnt & _ptr,
			 * so don't waste time setting them.
			 */
			if ((c = _filbuf(iop)) == EOF)
				break;
			*((char *)ptr)++ = c;
			s--;
		}
		if (iop->_cnt >= s) {
			memcpy(ptr, iop->_ptr, s);
			iop->_ptr += s;
			iop->_cnt -= s;
			return (count);
		}
	}
	return (size != 0 ? count - ((s + size - 1) / size) : 0);
}

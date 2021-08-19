/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <stdio.h>
#include <stdlib.h>

#undef setvbuf
int setvbuf(FILE *iop, char *buf, int mode, size_t size) {
	if (iop->_base != NULL && iop->_flag&_IOMYBUF)
		free(iop->_base);
	iop->_flag &= ~(_IOMYBUF|_IONBF|_IOLBF);
	if ((mode != _IONBF) && (buf == NULL))
		buf = malloc(size);
	if ((iop->_base = buf) == NULL) {
		iop->_flag |= _IONBF;
		iop->_bufsiz = 0;
	} else {
		iop->_ptr = iop->_base;
		iop->_bufsiz = size;
	}
	iop->_cnt = 0;
	if (mode == _IOLBF) {
		iop->_flag |= _IOLBF|_IOMYBUF;
	}
	return 0;
}


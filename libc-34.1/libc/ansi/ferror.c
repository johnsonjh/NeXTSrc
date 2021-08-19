/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <stdio.h>

#undef ferror
int ferror(FILE *p) {
	return (((p)->_flag&_IOERR)!=0);
}


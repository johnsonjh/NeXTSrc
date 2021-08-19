/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <stdio.h>

#undef feof
int feof(FILE *p) {
	return (((p)->_flag&_IOEOF)!=0);
}


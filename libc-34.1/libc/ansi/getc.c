/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <stdio.h>

#undef getc
int getc(FILE *p) {
	return (--(p)->_cnt>=0? (int)(*(unsigned char *)(p)->_ptr++):_filbuf(p));
}


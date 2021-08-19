/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <stdio.h>

#undef fgetpos
int fgetpos(FILE *stream, fpos_t *pos) {
	if ((*pos = ftell(stream)) == -1) return -1;
	return 0;
}


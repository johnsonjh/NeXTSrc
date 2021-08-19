/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <stdio.h>

#undef fsetpos
int fsetpos(FILE *stream, const fpos_t *pos) {
	return fseek(stream, *pos, SEEK_SET);
}


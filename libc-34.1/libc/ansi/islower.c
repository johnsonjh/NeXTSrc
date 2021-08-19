/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#include <ctype.h>

#undef islower
int islower(int c) { return ((int)((_ctype_+1)[c]&_L)); }


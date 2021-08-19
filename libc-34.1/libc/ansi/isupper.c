/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#include <ctype.h>

#undef isupper
int isupper(int c) { return ((int)((_ctype_+1)[c]&_U)); }


/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#include <ctype.h>

#undef tolower
int tolower(int c) { return isupper(c) ? _tolower(c) : c; }


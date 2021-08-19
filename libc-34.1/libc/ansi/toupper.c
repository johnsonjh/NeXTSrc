/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#include <ctype.h>

#undef toupper
int toupper(int c) { return islower(c) ? _toupper(c) : c; }



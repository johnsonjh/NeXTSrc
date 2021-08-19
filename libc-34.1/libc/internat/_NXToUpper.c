/*
 * Copyright 1990, NeXT, Inc.
 */

#include <NXCType.h>

_NXToUpper(c)
	unsigned int c;
{
	return ((unsigned int )(_NX_ULTable_)[c]);
}

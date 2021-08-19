/*
 * Copyright 1990, NeXT, Inc.
 */

#include	<NXCType.h>

_NXToLower(c)
	unsigned int c;
{
	return ((unsigned int )(_NX_ULTable_)[c]);
}

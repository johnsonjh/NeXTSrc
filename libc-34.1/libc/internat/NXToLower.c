/*
 * Copyright 1990, NeXT, Inc.
 */

#include <NXCType.h>

NXToLower(c)
	unsigned int c;
{
	return (NXIsUpper(c) ? ((unsigned int )(_NX_ULTable_)[c]) : c);
}

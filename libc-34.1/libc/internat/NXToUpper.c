/*
 * Copyright 1990, NeXT, Inc.
 */

#include <NXCType.h>

NXToUpper(c)
	unsigned int c;
{
	return (NXIsLower(c) ? ((unsigned int )(_NX_ULTable_)[c]) : c);
}

/*
 * Copyright 1990, NeXT, Inc.
 */

#include	<NXCType.h>

NXIsAscii(c)
	unsigned int c;
{
	return (c <= 0177);
}

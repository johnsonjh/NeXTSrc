/*
 * Network number to ascii conversion
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>

char *
nettoa(net)
	unsigned net;
{
	static char buf[10];
	unsigned char f1, f2, f3;

	f1 = net & 0xff;
	net >>= 8;
	f2 = net & 0xff;
	net >>= 8;
	f3 = net & 0xff;
	if (f3 != 0) {
		sprintf(buf, "%u.%u.%u", f3, f2, f1);
	} else if (f2 != 0) {
		sprintf(buf, "%u.%u", f2, f1);
	} else {
		sprintf(buf, "%u", f1);
	}
	return (buf);
}


/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Sys5 compat routine: rstrtok
 * Leo 15Sep88 frmo strtok
 */

#import <string.h>

char *
#if	NeXT
rstrtok(register char *s, register const char *sep)
#else
rstrtok(register char *s, sep)
	register char *s, *sep;
#endif	NeXT
{
	register char *p;
	register c;
	static char *lasts,*origS;

	if (s == 0)
		s = lasts;
	else
	{
		origS = s;
		s += strlen(s)-1;
	}
	if (s == 0)
		return (0);

	/* Advance pointer s past all separator characters */
	while (s >= origS) {
		c = *s;
		if (!strchr(sep, c))
			break;
		*s = '\0';
		s--;
	}
	/* s now points at rightmost non-separator character or is left of origS */

	/* If the whole string was separators, we're through */
	if (s < origS) {
		lasts = origS = 0;
		return (0);
	}

	/* Move pointer left until a separator character found or string ends */
	for (p = s; p > origS; )
	{
		c = *--p;
		if (strchr(sep, c))
			break;
	}
	/* p now points at leftmost separator or is left of origS */

	if (p <= origS)
		lasts = 0;
	else {
		/* Make lasts point at the first character to examine next time */
		lasts = p-1;
	}
	return (p+1);
}

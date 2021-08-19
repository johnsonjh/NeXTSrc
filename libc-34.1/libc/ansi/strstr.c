/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <string.h>

#undef strstr
char *strstr(const char *s1, const char *s2) {
      register char c1;
      register const char c2 = *s2;

      while ((c1 = *s1++) != '\0') {
	     if (c1 == c2) {
	            register const char *p1, *p2;

	            p1 = s1;
	            p2 = &s2[1];
		    while (*p1++ == (c1 = *p2++) && c1)
			continue;
		    if (c1 == '\0') return ((char *)s1) - 1;
	     }
      }
      return NULL;
}

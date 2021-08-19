#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

static unsigned long int
strtol_or_ul(register const char *nptr, char **endptr, int base, int sign)
{
	register unsigned long int n;
	register char c;
	register const char *p;
	register int empty = 1;
	register unsigned long int d;

	n = 0;
	p = nptr;
	c = *p++;
	while (isspace(c)) {
	    c = *p++;
	}
	if (sign) {
	    switch (c) {
	    case '-': sign = -1;
	    case '+': c = *p++;
	    }
	}
	if (base==0) {
	    if (c=='0') {
	        c = *p++;
		empty = 0;
		if (c=='x') {
		    base = 16;
		    c = *p++;
		} else {
		    base = 8;
		}
	    } else {
	        base = 10;
	    }
	} else if (base==16) {
	    if (c=='0') {
	        c = *p++;
		if (tolower(c) == 'x') {
		    if (isxdigit(*p)) {
		       c = *p++;
		       empty = 0;
		    }
		} else {
		    empty = 0;
		}
	    }
	}
	while (((d = (c - '0')) < base) ||
	       ((d = (tolower(c) - 'a' + 10)) < base)) {
	    d = n*base + d;
	    if (d < n) {
		n = ULONG_MAX;
		errno = ERANGE;
	        break;
	    }
	    n = d;
	    empty = 0;
	    c = *p++;
	}
	if (endptr != NULL) {
	    *endptr = empty ? (char *) nptr : (char *) (p-1);
	}
	if (sign) {
	    if (sign < 0) {
	        if (n>((unsigned long int)(-LONG_MIN))) {
		     n = (unsigned long int)LONG_MIN;
		     errno = ERANGE;
		} else {
		     n = (unsigned long int)(-(long int)n);
		}
	    } else {
		if (n>LONG_MAX) {
		     n = LONG_MAX;
		     errno = ERANGE;
		}
	    }
	}
	return n;
}

long int
strtol(register const char *nptr, char **endptr, int base) {
	return (long int) strtol_or_ul(nptr, endptr, base, 1);
}

unsigned long int
strtoul(register const char *nptr, char **endptr, int base) {
	return strtol_or_ul(nptr, endptr, base, 0);
}

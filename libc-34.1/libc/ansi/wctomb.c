#include <stddef.h>
#include <stdlib.h>

#undef wctomb
int
wctomb(char *s, wchar_t wchar) {
	if (s==NULL) {
		return 0;
	} else {
		*s = (char) wchar;
		return 1;
	}
}

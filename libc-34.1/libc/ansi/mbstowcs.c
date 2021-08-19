#include <stddef.h>
#include <stdlib.h>

#undef mbstowcs
size_t
mbstowcs(wchar_t *pwcs, const char *s, size_t n) {
	size_t c = 0;

	while (n != c) {
		if ((*pwcs++ = (wchar_t) *s++) == 0) {
	break;
		}
		c++;
	}
	return c;
}

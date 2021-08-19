#include <stddef.h>
#include <stdlib.h>

#undef wcstombs
size_t
wcstombs(char *s, const wchar_t *pwcs, size_t n) {
	size_t c = 0;

	while (n != c) {
		if ((*s++ = (char) *pwcs++) == 0) {
	break;
		}
		c++;
	}
	return c;	
}

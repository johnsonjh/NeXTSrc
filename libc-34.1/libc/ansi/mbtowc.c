#include <stddef.h>
#include <stdlib.h>

#undef mbtowc
int
mbtowc(wchar_t *pwc, const char *s, size_t n) {
	if (s==NULL) {
		return 0;
	} else {
		if (n==0) {
		        return -1;
		} else {
			if (pwc != NULL) {
				*pwc = (wchar_t)*s;
			}
			return (*s != '\0');
		}
	}
}

#include <stddef.h>
#include <stdlib.h>

#undef mblen
int
mblen(const char *s, size_t n) {
	if (s==NULL) {
		return 0;
	} else {
		if (n==0) {
			return -1;
		} else {
			return (*s != '\0');
		}
	}
}

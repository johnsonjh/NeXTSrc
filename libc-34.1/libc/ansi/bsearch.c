#include <stdlib.h>

#undef bsearch
void *
bsearch(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
    int l = 0;
    int u = nmemb - 1;
    int m;
    void *mp;
    int r;

    while (l <= u) {
	m = (l + u) / 2;
	mp = (void *)(((char *)base) + (m * size));
	if ((r = (*compar) (key, mp)) == 0) {
	    return mp;
	} else if (r < 0) {
	    u = m - 1;
	} else {
	    l = m + 1;
	}
    }
    return NULL;
}

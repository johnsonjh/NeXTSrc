#include <stdlib.h>

#undef div
div_t
div(int numer, int denom) {
	div_t result;
	
	result.quot = numer / denom;
	result.rem = numer % denom;
	return result;
}

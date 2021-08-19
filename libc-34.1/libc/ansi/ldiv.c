#include <stdlib.h>

#undef ldiv
ldiv_t
ldiv(long int numer, long int denom) {
	ldiv_t result;
	
	result.quot = numer / denom;
	result.rem = numer % denom;
	return result;
}

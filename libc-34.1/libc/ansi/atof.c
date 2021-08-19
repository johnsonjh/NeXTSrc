#include <stdlib.h>

#undef atof
double
atof(const char *nptr) {
    return strtod(nptr, NULL);
}

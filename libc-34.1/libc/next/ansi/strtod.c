#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <math.h>

#undef strtod
double
strtod(char *str, char **endptr) {
	extern double atof();
	
	if (endptr != NULL) {
		*endptr = str;
		while (**endptr && isspace(**endptr) == 0)
			(*endptr)++;
	}
	return (atof (str));
}

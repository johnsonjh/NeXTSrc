#include <string.h>
#include <errno.h>

char *strerror(int errnum) {
	return errnum < sys_nerr ? sys_errlist[errnum] : "Unknown error";
}

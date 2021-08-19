#ifdef SHLIB
#include "shlib.h"
#endif

#include <stdio.h>
#include "internal.h"

int
dbError(dbErrorType errno, char *fmt, ...)
/*
** Print a diagnostic message to the FILE 'dbMsgs' ('stdout' by default).
** 'fmt' and 'args' are used as in 'printf()'.
** If the global variable 'dbErrors' is 0 (default), 'dbError' does nothing.
*/
{
	va_list	ap;

	dbErrorNo = errno;
	va_start(ap, fmt);
	if (dbErrors)
	{
		fprintf(dbMsgs, "dbError: %d, ", errno);
		vfprintf(dbMsgs, fmt, ap);
	}
	
	va_end(ap);
	return FAILURE;
}

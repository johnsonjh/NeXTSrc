#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mach.h>
extern char *mach_error_string(kern_return_t);
#include "errors.h"

long errors = 0;	/* number of calls to error() */

/*
 * Print the error message and return to the caller after setting the error
 * indication.
 */
void
error(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
	va_end(ap);
	errors++;
}

/*
 * Print the error message along with the system error message and return to
 * the caller after setting the error indication.
 */
void
perror_error(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, " (%s)\n", strerror(errno));
	va_end(ap);
	errors++;
}

/*
 * Print the fatal error message and cleanup and exit.
 */
void
fatal(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
	va_end(ap);
	cleanup(0);
}

/*
 * Print the fatal error message along with the system error message and cleanup
 * and exit.
 */
void
perror_fatal(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, " (%s)\n", strerror(errno));
	va_end(ap);
	cleanup(0);
}

/*
 * Print the fatal error message along with the mach error string, and cleanup
 * and exit.
 */
void
mach_fatal(
kern_return_t r,
char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, " (%s)\n", mach_error_string(r));
	va_end(ap);
	cleanup(0);
}

/*****************************************************
 * ABSTRACT:
 *   String utility programs to allocate and copy
 *   or concatenate, free and return constant strings.
 *
 *	$Header: string.c,v 1.1 89/06/08 14:20:18 mmeyer Locked $
 *
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 15-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Declare and initialize charNULL here for strNull def in string.h
 *
 * 27-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 ***************************************************/

#define	EXPORT_BOOLEAN
#if	NeXT
#include <sys/boolean.h>
#else	NeXT
#include <mach/boolean.h>
#endif	NeXT
#include <sys/types.h>
#include "error.h"
#include "alloc.h"
#include "string.h"

char	charNULL = 0;

string_t
strmake(string)
    char *string;
{
    register string_t saved;

    saved = malloc((u_int) (strlen(string) + 1));
    if (saved == strNULL)
	fatal("strmake('%s'): %s", string, unix_error_string(errno));
    return strcpy(saved, string);
}

string_t
strconcat(left, right)
    string_t left, right;
{
    register string_t saved;

    saved = malloc((u_int) (strlen(left) + strlen(right) + 1));
    if (saved == strNULL)
	fatal("strconcat('%s', '%s'): %s",
	      left, right, unix_error_string(errno));
    return strcat(strcpy(saved, left), right);
}

void
strfree(string)
    string_t string;
{
    free(string);
}

char *
strbool(bool)
    boolean_t bool;
{
    if (bool)
	return "TRUE";
    else
	return "FALSE";
}

char *
strstring(string)
    string_t string;
{
    if (string == strNULL)
	return "NULL";
    else
	return string;
}

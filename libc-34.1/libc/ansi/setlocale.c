/* Copyright (c) 1990 NeXT, Inc. - 3 August 90 John Anderson */

#include <locale.h>
#include <memory.h>
#include <stddef.h>
#include <string.h>
#include "strftimeprivate.h"

static const char *currentLocale = "C";

#undef setlocale
char *setlocale(int category, const char *locale)
{
    const char	*oldLocale;

    if (locale != NULL) {
	oldLocale = currentLocale;
	if (strcmp(locale, "") == 0)
	    currentLocale = "";
	else if (strcmp(locale, "C") == 0)
	    currentLocale = "C";
	else return NULL;
	if (currentLocale != oldLocale) {
		/*
		 * Clear the string pointer cache so the next time we 
		 * call strftime we'll pick up the new strings.
		 */
		_strftime_clear_cache();
	}
    }
    return (char *)currentLocale;
}

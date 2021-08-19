#include "dbUtil.h"

char *
_dbBaseName(char *s)
/*
 * Return pointer to the base file name in path 's'
 */
{
	char *p = rindex(s, '/');
	return p ? p + 1 : s;
}

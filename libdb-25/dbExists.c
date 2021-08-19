#ifdef SHLIB
#include "shlib.h"
#endif

#include "internal.h"

int
dbExists(register char *name)
/*
** True if the database 'name' exists (ie, the files "name.[DL]" exist).
*/
{
	return access(_dbLeafFile(name), 0) == 0 
		&& access(_dbDirFile(name), 0) == 0;
}

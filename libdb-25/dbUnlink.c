#ifdef SHLIB
#include "shlib.h"
#endif

#include "internal.h"

int
dbUnlink(char *name)
/*
** Unlink (remove) files associated with the Database 'name' (ie, "name.[DL]").
*/
{
	dbErrorNo = dbErrorNoError;
	return (unlink(_dbLeafFile(name)) || unlink(_dbDirFile(name))) ? 
		dbError(errno + dbErrorSystem, 
			"cannot remove database '%s'\n", name) : SUCCESS;
}

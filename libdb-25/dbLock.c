#ifdef SHLIB
#include "shlib.h"
#endif

/*
** file-locking routines to manage concurrent writes.
** (ie, to regulate writing into the database by concurrent processes)
** these use 4.2 file locking calls.
** sufficient to only worry about locking the leaf file.
*/

#include <sys/file.h>
#include "internal.h"

int
dbLock(register Database *db)
/*
** Lock 'db', by locking up the leaf file.
** Should probably be NFSified.
*/
{
	return flock(db->L, LOCK_EX | LOCK_NB) == 0;
}

int
dbUnlock(register Database *db)
/*
** Unlock 'db'.
*/
{
	(void) flock(db->L, LOCK_UN);
	return SUCCESS;
}

int
dbWaitLock(register Database *db, register int count, unsigned pause)
/*
** Try to lock 'db'.
** Make 'count' attempts (trying forever if 'count' < 0),
** and wait for 'pause' seconds after each unsuccessful try.
** Return 1 if 'db' is successfully locked, otherwise return 0.
**
** Typical way to achieve a lock on a database:
** to try to lock it at most 5 times, pausing 30 seconds between attempts:
*/
{
	dbErrorNo = dbErrorNoError;
	if (count < 0)
		return (! flock(db->L, LOCK_EX)) ? SUCCESS : 
			dbError(errno + dbErrorSystem, 
				"cannot lock database '%s'\n", db->name);

	for (; count; sleep(pause))
		if (dbLock(db))
			return SUCCESS;
		else
			count = (count < 1) ? 0 : count - 1;

	return FAILURE;
}

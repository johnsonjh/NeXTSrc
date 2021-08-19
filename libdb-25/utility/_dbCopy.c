#include <stdio.h>
#include "dbUtil.h"

int
_dbCopy(Database *dbFrom, int count, Database **dbTo)
{
	char *s, *c;
	register int i;
	Data d;
	ushort maxLen = 0;

	for (i = 0; i < count; i++)
		if (dbTo[i]->d.llen > maxLen)
			maxLen = dbTo[i]->d.llen;

	if (! (s = malloc(maxLen)))
	{
		mallocError:
		dbError(dbErrorNoMemory, 
			"cannot allocate buffer to copy '%s'\n", dbFrom->name);
		return FAILURE;
	}

	d.k.s = strcpy(s, "");
	if (! (c = malloc(maxLen)))
	{
		free(s);
		goto mallocError;
	}
	
	d.c.s = strcpy(c, "");
	dbFirst(dbFrom, (Data *) 0);
	for (; dbGet(dbFrom, &d); dbNext(dbFrom, (Data *) 0))
		for (i = 0; i < count; i++)
			if (! dbStore(dbTo[i], &d))
				break;

	free(s);
	free(c);
	return (! dbErrorNo);
}


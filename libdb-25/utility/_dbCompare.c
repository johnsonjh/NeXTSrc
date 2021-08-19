#include <stdio.h>
#include "dbUtil.h"

int
_dbCompare(Database *dbFrom, int count, Database **dbTo, int verbose)
{
	register int i, *errorCount;
	Data d1, d2;
	char *s1, *c1, *c2;
	int errorTotal = 0;
	int maxLen = 0;

	if (! (errorCount = (int *) malloc(count * sizeof(int))))
		mallocError: return dbError(dbErrorNoMemory, 
			"not enough memory to compare '%s'\n", dbFrom->name);

	for (i = 0; i < count; i++)
	{
		errorCount[i] = 0;
		if (dbTo[i]->d.llen > maxLen)
			maxLen = dbTo[i]->d.llen;
	}

	if (! (s1 = malloc(maxLen)))
	{
		mallocError_s1: free(errorCount);
		goto mallocError;
	}

	d2.k.s = d1.k.s = strcpy(s1, "");
	if (! (c1 = malloc(maxLen)))
	{
		mallocError_c1: free(s1);
		goto mallocError_s1;
	}
	
	d1.c.s = strcpy(c1, "");
	if (! (c2 = malloc(maxLen)))
	{
		free(c1);
		goto mallocError_c1;
	}
	
	d2.c.s = strcpy(c2, "");
	dbFirst(dbFrom, (Data *) 0);
	for (; dbGet(dbFrom, &d1); dbNext(dbFrom, (Data *) 0))
		for (i = 0; i < count; i++)
			if (! dbFind(dbTo[i], &d1) || ! dbGet(dbTo[i], &d2))
			{
				++errorTotal;
				++errorCount[i];
				if (verbose)
				{
					fprintf(dbMsgs, 
			"(%d) record %d of leaf %ld not found in '%s'\n", 
				errorCount[i], dbFrom->recno, 
					dbFrom->cleaf->lp, dbTo[i]->name);
					if (verbose > 1)
					{
						fputc('\n', dbMsgs);
						_dbCatenate(dbMsgs, &d1);
						fputc('\n', dbMsgs);
					}
				}
			} else
			if (d1.c.n != d2.c.n || bcmp(d1.c.s, d2.c.s, d1.c.n))
			{
				++errorTotal;
				++errorCount[i];
				if (verbose)
				{
					fprintf(dbMsgs, 
			"(%d) record %d of leaf %ld different in '%s'\n", 
				errorCount[i], dbFrom->recno, 
					dbFrom->cleaf->lp, dbTo[i]->name);
					if (verbose > 1)
					{
						fputc('\n', dbMsgs);
						_dbCatenate(dbMsgs, &d1);
						fputc('\n', dbMsgs);
						_dbCatenate(dbMsgs, &d2);
						fputc('\n', dbMsgs);
					}
				}
			}

	if (verbose)
	{
		fputc('\n', dbMsgs);
		for (i = 0; i < count; i++)
			if (errorCount[i])
				fprintf(dbMsgs, 
			"%d records missing or different for database '%s'\n", 
						errorCount[i], dbTo[i]->name);
			else
				fprintf(dbMsgs, 
				"databases '%s' and '%s' are identical\n", 
						dbFrom->name, dbTo[i]->name);
	}

	return (! (dbErrorNo = errorTotal));
}


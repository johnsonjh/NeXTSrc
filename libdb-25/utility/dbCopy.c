#include <stdio.h>
#include "dbUtil.h"

void
main(int argc, char **argv)
{
	register int i;
	register int j;
	Database *dbFrom, *dbTo[MAXOPEN + 1];
	int verbose = 0;
	int exitVal = 0;

	dbErrors = 1;
	if (argc < 3)
	{
		usageError: fprintf(stderr, 
		"usage: %s [-s size] from-database to-database ...\n", 
						_dbBaseName(argv[0]));
		exit(-1);
	}

	if (! strcmp(argv[i = 1], "-s"))
	{
		dbLeafSize = atoi(argv[++i]);
		if (argc - ++i < 2)
			goto usageError;
	}

	if (! (dbFrom = dbInit(argv[i++])))
		exit(dbErrorNo);

	dbSetCache(dbFrom, 2, 1);
	for (j = 0; i < argc && j < MAXOPEN + 1; i++, j++)
		if (! (dbTo[j] = dbOpen(argv[i])))
			exitVal = dbErrorNo;

	if (! exitVal && ! _dbCopy(dbFrom, j, dbTo))
		exitVal =  dbErrorNo;
		
	for (i = 0; i < j; i++)
		if (! dbClose(dbTo[i]))
			exitVal = dbErrorNo;

	exit(dbClose(dbFrom) ? exitVal : dbErrorNo);
}


#include <stdio.h>
#include "internal.h"

extern int hexFlag = 1;

void
main(int argc, char **argv)
{
	register int i, j;
	Database *dbFrom, *dbTo[MAXOPEN + 1];
	int verbose = 0, exitVal = 0;

	if (argc < 3)
	{
		errorUsage:
		fprintf(stderr, 
			"usage: %s [-v][-V] from-database to-database ...\n", 
							_dbBaseName(argv[0]));
		exit(-1);
	}

	for (i = 1; i < argc; i++)
	{
		if (! strcmp(argv[i], "-v"))
			verbose = 1;
		else
		if (! strcmp(argv[i], "-V"))
			verbose = 2;
		else
			break;
	}

	if (argc - i < 2)
		goto errorUsage;

	dbErrors = 1;
	if (! (dbFrom = dbInit(argv[i++])))
		exit(dbErrorNo);

	for (j = 0; i < argc && j < MAXOPEN + 1; i++, j++)
		if (! (dbTo[j] = dbInit(argv[i])))
			exitVal = dbErrorNo;

	dbErrors = 0;
	if (! exitVal && ! _dbCompare(dbFrom, j, dbTo, verbose)) 
		exitVal = dbErrorNo;

	dbErrors = 1;
	for (i = 0; i < j; i++)
		if (! dbClose(dbTo[i]))
			exitVal = dbErrorNo;

	exit(dbClose(dbFrom) ? exitVal : dbErrorNo);
}


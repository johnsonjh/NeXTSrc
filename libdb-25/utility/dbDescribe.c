#include <stdio.h>
#include "dbUtil.h"

/*
** this is a lobotomized version of dbReport for external distribution
*/

void
main(int argc, char **argv)
{
	register int i;
	Database *db;
	int exitVal = 0;

	dbErrors = 1;
	if (argc < 2)
	{
		errorUsage: fprintf(stderr, "usage: %s databases ...\n", 
							_dbBaseName(argv[0]));
		exit(-1);
	}

	for (i = 1; i < argc && ! exitVal; i++)
	{
		if (! (db = dbInit(argv[i])))
			exitVal = dbErrorNo;

		if (! exitVal && ! _dbReport(db, 0))
			exitVal = dbErrorNo;
		
		if (! dbClose(db))
			exitVal = dbErrorNo;
	}

	exit(exitVal);
}



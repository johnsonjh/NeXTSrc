#include <stdio.h>
#include "dbUtil.h"

extern int	hexFlag = 0;	/* used by _dbCatenate */

void
main(int argc, char **argv)
/*
** dbcat databases...
** 'cat' contents of databases to 'stdout'.
** assumes key/content pairs are strings and prints them,
** separated by a spargce and a newline.
*/
{
	register int i = 1;
	Database *db;

	dbErrors = 1;
	if (argc < 2)
	{
		errorUsage:
		fprintf(stderr, "usage: %s [-x] databases ...\n", 
							_dbBaseName(argv[0]));
		exit(-1);
	}

	if (! strcmp(argv[1], "-x"))
	{
		hexFlag = 1;
		if (++i >= argc)
			goto errorUsage;
	}

	for (; i < argc; i++)
	{
		if (! (db = dbInit(argv[i])))
			exit(dbErrorNo);

		dbCat(stdout, db, _dbCatenate);
		if (! dbClose(db))
			exit(dbErrorNo);
	}

	exit(0);
}



#include <stdio.h>
#include "internal.h"

void
main(int argc, char **argv)
{
	register int i;
	Database *db;

	if (argc < 2)
	{
		errorUsage:
		fprintf(stderr, "usage: %s databases ...\n", 
							_dbBaseName(argv[0]));
		exit(-1);
	}

	for (i = 1; i < argc; i++)
	{
		if (! (db = dbInit(argv[i])))
			exit(dbErrorNo);

		if (dbExpand(db) + dbClose(db) < 2)
			exit(dbErrorNo);
	}

	exit(0);
}



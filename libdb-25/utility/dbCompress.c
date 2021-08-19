#include <stdio.h>
#include "internal.h"

void
main(int argc, char **argv)
{
	register int i = 1;
	Database *db;
	int packFlag = 0;

	if (argc < 2)
	{
		errorUsage:
		fprintf(stderr, "usage: %s databases ...\n", 
							_dbBaseName(argv[0]));
		exit(-1);
	}

	for (; i < argc; i++)
	{
		if (! (db = dbInit(argv[i])))
			exit(dbErrorNo);

		if (dbCompress(db) + dbClose(db) < 2)
			exit(dbErrorNo);
	}

	exit(0);
}



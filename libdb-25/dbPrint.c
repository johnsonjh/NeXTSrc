#ifdef SHLIB
#include "shlib.h"
#endif

#include "internal.h"

int
dbPrint(Database *db, char *k, char *c)
/*
** Store a Data pair (k, c) in Database 'db'.
** Return 1 if the store succeeded, 0 if not.
*/
{
	Data d;

	d.k.s = k;
	d.k.n = strlen(k) + 1;
	d.c.s = c;
	d.c.n = strlen(c) + 1;
	return dbStore(db, &d);
}

int
dbScan(Database *db, char *k, char *c)
/*
** Given key 'k', fetch its contents from 'db' and put them in 'c'.
** Return 1 if successful, 0 if not.
*/
{
	Data d;
	
	d.k.s = k;
	d.k.n = strlen(k)+1;
	d.c.s = c;
	d.c.n = 0;
	return dbFetch(db, &d);
}

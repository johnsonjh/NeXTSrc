#ifdef SHLIB
#include "shlib.h"
#endif

#include <stdio.h>
#include "internal.h"

void
dbPrintKC(FILE *f, register Data *d)
/*
** Default function to print a 'key/content' data pair on stream 'f', 
** separated by " \t".  Both data are assumed to be strings.
*/
{	
	if (d->k.n)
		fprintf(f, "%s", d->k.s);
		
	if (d->c.n)
		fprintf(f, " \t%s", d->c.s);
		
	if (d->k.n || d->c.n)
		fprintf(f, "\n");
}

void
dbCat(FILE *f, register Database *db, register void (*pfunc)())
/*
** Catenate (print) the contents of 'db' on FILE 'f',
** calling the function 'pfunc(f, data)' to print each 
** 'key/content' pair in the database.
** If 'pfunc' is null, 'dbPrintKC()' is used by default.
*/
{
	Data d;
	char k[LEAFSIZE], c[LEAFSIZE];

	if (!pfunc)
		pfunc = dbPrintKC;
		
	d.k.s = k;
	d.c.s = c;
	if (dbFirst(db, &d))
		do {
			if (dbGet(db, &d))
				(*pfunc)(f, &d);

		} while (dbNext(db,&d));
}

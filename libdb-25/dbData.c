#ifdef SHLIB
#include "shlib.h"
#endif

#include "internal.h"

Data
dbData(Datum k, Datum c)
/*
** Construct and return a 'Data' pair with key 'k' and content 'c'.
*/
{ 
	Data d;
	 
	d.k = k; 
	d.c = c; 
	return d; 
}

Datum
StrDatum(char *s)
/*
** Construct and return a 'Datum' from string 's'.
*/
{ 
	Datum d;
 
	d.s = s; 
	d.n = strlen(s) + 1; 
	return d;
}

Data
StrData(char *k, char *c)
/*
** Construct and return a 'Data' pair from strings 'k' and 'c'.
*/
{
	return dbData(StrDatum(k), StrDatum(c));
}

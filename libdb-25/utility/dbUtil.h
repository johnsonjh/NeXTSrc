/*
** header file for libdb utility library, libdbUtil
*/

#ifndef	dbUtil
#define	dbUtil

#if ! defined(DEBUG)
#define	DEBUG
#endif

#import "internal.h"

/* function prototypes */
extern void _dbCatenate(FILE *fp, Data *d);
extern int 
_dbCompare(Database *dbFrom, int count, Database **dbTo, int verbose);
extern int _dbCopy(Database *dbFrom, int count, Database **dbTo);
extern int _dbReport(Database *db, int verbose);
extern char *_dbBaseName(char *s);
extern int _dbHistogramD(Database *db, int verbose);
extern int _dbHistogramL(Database *db, int verbose);
extern void _dbPrintD(dhead *d, FILE *fp, int verbose);


#endif


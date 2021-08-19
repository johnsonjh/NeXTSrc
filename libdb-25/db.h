/*
** User header file for binary extendible hashing package.
** Hides implementation details.
**
** !!!NOTICE!!!
**
** This file and the functions it prototypes will not be supported
** after release 2.0.
**
** Use the BTree package instead of this library 
** (see /usr/lib/libbtree.a and /NextLibrary/Documentation/Next/RelNotes)
*/
#ifndef _DB_INCLUDED_
#define _DB_INCLUDED_

#include <stdio.h>

	/* datum is a string 's' of 'n' bytes */
typedef struct {
	char *s;
	unsigned short n;
} Datum;

	/* data passing buffer (key/content pair) */
typedef struct {
	Datum k, c;
} Data;

	/* enumeration of error codes reported */
typedef enum {
	dbErrorNoError, 	/* no error: the normal state of affairs */
	dbErrorLockFailed, 	/* a lock could not be obtained for database */
	dbErrorReadOnly, 	/* write or delete on read only database */
	dbErrorNoKey, 		/* the specified key was not found */
	dbErrorNoRoom, 		/* database cannot accept the new record */
	dbErrorNoMemory, 	/* sufficient memory is not available */
	dbErrorTooLarge, 	/* the new record is too large to insert */
	dbErrorTooDeep, 	/* the directory cannot expand further */
	dbErrorInternal, 	/* an internal inconsistency was found */
	dbErrorSystem = 100	/* base value added to system variable errno */
} dbErrorType; 
	

#ifndef _INTERNAL_H_ /* internal code has it's own version */
typedef struct {
	char *name;	/* database name; no extension */
	int flag;	/* directory flags; see defines below */
	int D;		/* file descriptor for directory file */
	int L;		/* file descriptor for leaf file */
} Database;
#endif

/* function prototypes */
extern int dbStore(Database *db, Data *d);
extern int dbFetch(Database *db, Data *d);
extern int dbDelete(Database *db, Data *d);
extern int dbFind(Database *db, Data *d);
extern int dbGet(Database *db, Data *d);
extern int dbSetKey(Database *db, Data *d);
extern int dbFirst(Database *db, Data *d);
extern int dbNext(Database *db, Data *d);
extern int dbFlush(Database *db);
extern int dbCompress(Database *db);
extern int dbExpand(Database *db);
extern void dbSetCache(Database *db, unsigned short limit, int private);
extern int dbUnlink(char *name);
extern int dbExists(char *name);
extern Database *dbOpen(char *name);
extern Data dbData(Datum k, Datum c);
extern Datum StrDatum(char *s);
extern Data StrData(char *k, char *c);
extern int dbPrint(Database *db, char *k, char *c);
extern int dbScan(Database *db, char *k, char *c);
extern int dbWaitLock(Database *db, int count, unsigned int pause);
extern int dbUnlock(Database *db);
extern int dbLock(Database *db);
extern Database *dbInit(char *name);
extern int dbCreate(char *name);
extern int dbClose(Database *db);
extern int dbError(dbErrorType errno, char *fmt, ...);

#ifdef NULL
extern void dbPrintKC(FILE *f, Data *d);
extern void dbCat(FILE *f, Database *db, void (*pfunc)(FILE *, Data *));
#endif

/* global variables */
extern char *dbDirExtension;	/* default directory file extension */
extern char *dbLeafExtension;	/* default leaf file extension */
extern int dbErrors;		/* if true, errors generate printed messages */
extern dbErrorType dbErrorNo;	/* contains last error code reported */
extern int dbMaxLeafBuf;	/* maximum number of buffers in common cache */
extern int dbLeafSize;		/* leaf size used when database created */
extern int dbMode, dbFlags;	/* flags and mode for dbOpen */
extern FILE *dbMsgs;		/* where to write diagnostic messages */
extern unsigned int *dbSizeTable;	/* pointer to null-terminated table */
					/* of rounding points for leaf sizes */

#define LEAFSIZE 4088		/* default leaf size (block size) */
				/* provides for 0x001fffff leaves */

/* flag bits that may be set by the caller */
#define dbFlagBuffered (1 << 0)		/* output buffered, dbFlush() writes */

/* additional, read only flag bits set internally */
#define	dbFlagReadOnly (1 << 8)		/* if set, writes and deletes fail */
#define dbFlagCompressed (1 << 9)	/* database is compressed, read only */
#define dbFlagPrivate (1 << 10)		/* if set, private caching enabled */

/* the following are for dbm compatibility */
#define PBLKSIZ LEAFSIZE
#define DBM Database

typedef struct { char *dptr; int dsize; } datum;

extern int dbminit(char *file);
extern int dbmclose(void);
extern datum fetch(datum key);
extern int store(datum key, datum content);
extern int delete(datum key);
extern datum firstkey(void);
extern datum nextkey(datum key);
extern DBM *dbm_open(char *file, int flags, int mode);
extern int dbm_close(DBM *db);
extern datum dbm_fetch(DBM *db, datum key);
extern int dbm_store(DBM *db, datum key, datum content);
extern int dbm_delete(DBM *db, datum key);
extern datum dbm_firstkey(DBM *db);
extern datum dbm_nextkey(DBM *db, datum key);
extern int dbm_error(void);
extern int dbm_clearerr(void);

#endif


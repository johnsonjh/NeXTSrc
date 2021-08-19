/* internal definitions for binary extendible hashing package */

#define _INTERNAL_H_

#include <stdlib.h>
#include <libc.h>

/*
 * The rightmost two byte of the magic number -- the first 4-byte
 * value in the directory header -- indicate whether or not the
 * database was written by a big or little-endian machine
 * and with what kind of floating point format.
 */

#define ENDIAN	0	/* 0 for little, 1 for big */
#define IEEE	1	/* 0 for non-IEEE float format, 1 for true */
#define BYTESEX	(ENDIAN<<8)|IEEE
#define MAGIC	(('m'+'h')<<16)|BYTESEX	/* magic number for directory header */

/* 
 * the maximum depth, MAX_DEPTH, is non-portable.  It must be chosen
 * such that (2^MAX_DEPTH)*SL can be expressed in an unsigned long integer.
 * On the NeXT 68030 machine, a long is 32 bits so SL is 4;
 * 	the maximum directory size is therefore 0x80000000, or (2^29)*4  
 */
#define MAX_DEPTH	29

#define SL		sizeof(long)
#define SS		sizeof(short)

#define SDIR		sizeof(dhead)
#define SLHD		sizeof(lhead)
#define	MINOVERHEAD	(SLHD + (3 * SS)) /* amount of overhead in a leaf */

#define _MaxLeafBuf	128	/* maximum number of internal leaves */
#define _LeafHash	(_MaxLeafBuf + 13)
#define	dbMinLeafBuf	2	/* minimum no. of leaves needed for split */

				/* in-core directory, block-cached */
#define DBLOCK	4096		/* directory block size, number of bytes */
#define LPDB 	(DBLOCK/SL)	/* how many longs in such a block */
#define MAXOPEN	30		/* maximum number of open databases */

/* how to copy data into a Data buffer */
typedef enum	{
	dbCopyLength, 		/* copy length of key and length of data */
	dbCopyRecord, 		/* copy lengths and content of key and data */
	dbCopyPointer, 		/* copy lengths and pointer to key and data */
	dbCopyKeyOnly		/* copy length and content of key */
} dbCopyType;

/* leaf is sorted list of Data */
typedef struct	{	/* leaf header (in .L file) */
	u_short	flag;	/* leaf flag word */
	u_short	ldepth;	/* hash prefix length for this record */
	u_short	lsize;	/* size of this leaf when compressed */
	u_short	lrcnt;	/* how many records in the leaf */
	u_short	reserved;
	u_short	ldata;	/* byte count to beginning of data */
} lhead;		/* next (on disk) is array of record pointers */
			/* followed by the 'lrcnt' records */

/* a leaf in core */
typedef struct Leaf	{
	u_short	flag;	/* leaf descriptor flags */
	u_long	lp;	/* leaf address on disk */
	union	{		/* map buffer into leaf */
		char	*ulbuf;	/* start of a whole buffer */
		lhead	*ulhd;	/* leaf header always first */
	} leafp;
	u_short	*list;	/* pointer to list of rec pointers */
struct Leaf	*lnext;	/* open leaves chained on in-core hash table */
			/* free leaves chained on free chain */
} Leaf;

#define lhd	leafp.ulhd
#define lbuf	leafp.ulbuf

/* directory header in .D file */
typedef struct	{
	u_long	magic;	/* magic number */
	u_short	flag;	/* directory flag word */
	u_short	depth;	/* hash prefix length; 2^depth = number of entries */
	u_short	reserved;
	u_short	llen;	/* leaf size in bytes */
	u_long	dlcnt;	/* number of leaves in database */
} dhead;

#if defined(PROFILE) || defined(DEBUG)

/* leaf fetching statistics */
typedef struct bstat	{
	u_long	fetch; 	/* number of calls to _dbFetchLeaf */
	u_long	cleaf; 	/* number of current leaves reused */
	u_long	nopen; 	/* number of open leaves reused */
} bstat;

/* leaf accquisition statistics */
typedef struct astat	{
	u_long	total; 	/* number of calls to _dbLeafDescriptor */
	u_long	alloc;	/* allocations made by current database */
	u_long	cfree; 	/* free leaves reused; current database */
	u_long	ccycle; /* open leaves cycled; current database */
	u_long	sfree; 	/* free leaves reused; smaller leaf databases */
	u_long	scycle; /* open leaves cycled; smaller leaf databases */
	u_long	lfree; 	/* free leaves reused; larger leaf databases */
	u_long	lcycle; /* open leaves cycled; larger leaf databases */
} astat;

#endif

/* open database descriptor */
typedef struct Database	{
	char	*name;	/* name of database; no extension */
	int	flag;	/* directory flags; see defines below */
	int	D;	/* file descriptor for directory */
	int	L;	/* file descriptor for leaf file */
	u_short	cache;	/* cache limit for this database */
	u_short	alloc;	/* number of leaves allocated by this database */
	u_short	nopen;	/* number of open leaves in cache */
	u_short 	recno;	/* current record within the current leaf */
	dhead	d;	/* disk resident directory header */
	long	clidx;	/* the directory index of the current leaf */
	Leaf	*cleaf;	/* pointer to the current leaf */
	Leaf	*free;	/* list of free leaves for this database */
	Leaf	*table[_LeafHash];	/* open leaf hash table */
	u_long	*dir;	/* in core copy of directory */
struct Database	*dnext; /* chain of open dbs for cache sharing */
#if defined(PROFILE) || defined(DEBUG)
	astat	dastat;	/* leaf fetching statistics */
	bstat	dbstat;	/* leaf accquisition statistics */
#endif
} Database;

/* file name construction macros */
#define BSIZE 200	/* DO NOT INCREASE BSIZE W/O CHECKING dbGlobals.c */
#define	_dbExtFile(s, t)	\
	strncat(strncat(strcpy(_dbstrbuf, ""), s, BSIZE - 1), t, BSIZE - 1)
#define _dbDirFile(s)	_dbExtFile(s, dbDirExtension)
#define _dbLeafFile(s)	_dbExtFile(s, dbLeafExtension)
#define _dbLockFile(s)	_dbExtFile(s, dbLockExtension)

/* return code definitions */
#define SUCCESS		1
#define FAILURE		0

/* boolean value definitions */
#define	TRUE		1
#define	FALSE		0

/* flag bits used in disk resident leaf header */
#define	dbFlagCoalesceable	(1 << 0)	/* leaf is coalesceable */

/* additional flag bits used in leaf descriptor */
#define	dbFlagLeafOpen		(1 << 0)	/* leaf is in cache */
#define	dbFlagLeafDirty		(1 << 1)	/* leaf modified */
#define	dbFlagLeafMatch		(1 << 2)	/* leaf contains current key */

/* private flag bits used in directory descriptor - see db.h for public bits */
#define	dbFlagDirDirty (1 << 11)	/* directory modified, not written */

/* macros to move 16 bits into and out of char[2] */
#define _dbSetLen(x,y)	(*((u_short *) (x)) = ((u_short) y), \
					((u_char *) (x)) += SS)
#define _dbGetLen(y,x)	(((u_short) (y)) = *((u_short *) (x)), \
					((u_char *) (x)) += SS)

/* (might need bytewise versions on some machines) */
#define x_dbSetLen(x,y)	(*((u_char *) (x))++ = (u_char) (((u_short) y) >> 8), \
				*((u_char *) (x))++ = ((u_char) y))
#define x_dbGetLen(y,x)	(((u_short) y) = (u_short) (*((u_char *) (x))++) << 8, \
			 ((u_short) y) |= (u_short) (*((u_char *) (x))++))

/* functions made macros for speed */
	
#define _dbAllocLeaf(l)	(((l) = (Leaf *) calloc(1, sizeof(Leaf))) \
			? (((l)->lbuf = calloc(1, db->d.llen)) \
			? (l) : (free(l), ((l) = (Leaf *) 0))) : (l))

#define _dbLeafAddress(db, idx)	((db)->dir[(idx)])

#define	_dbFreeToHeap(l)	(free((l)->lbuf), free(l))

#define _dbFreeToList(db, l)	(((l)->lnext = (db)->free), ((db)->free = (l)))

#define	_dbGetFreeLeaf(db, l)	((db)->free ? (((l) = (db)->free), \
				((db)->free = (l)->lnext), (l)) : (Leaf *) 0)

#define	_dbSwapLeaf(l, tl)	(((l)->lbuf = (tl)->lbuf), \
				((tl)->lbuf = ((char *) (l)->list) - SLHD), \
				((l)->list = (tl)->list), \
				((tl)->list = (u_short *) ((tl)->lbuf + SLHD)))

#define	__dbHashLeaf(db, lp)	(((lp) / (db)->d.llen) % _LeafHash)

#define _dbHashLeaf(db, lp)	((db)->table[__dbHashLeaf(db, lp)])

#define _dbOpenLeaf(db, l)	(++(db)->nopen, \
			((l)->flag |= dbFlagLeafOpen), \
		((l)->lnext = (db)->table[__dbHashLeaf(db, (l)->lp)]), \
			((db)->table[__dbHashLeaf(db, (l)->lp)] = (l)))

#define _dbReadLeaf(db, l)	\
	(((l) && ((l)->lp == lseek((db)->L, (l)->lp, 0))) && \
	(read((db)->L, (l)->lbuf, (db)->d.llen) >= \
		((l)->lhd->lsize ? (l)->lhd->lsize : (db)->d.llen)))

#define	_dbVerifyWrite(fd, buf, len)	(write(fd, buf, len) == len)
			
#define _dbFlushLeaf(db, l)  	((void) lseek((db)->L, (l)->lp, 0), \
		_dbVerifyWrite((db)->L, (char *) (l)->lbuf, (db)->d.llen) ? \
		(((l)->flag &= ~dbFlagLeafDirty), SUCCESS) : FAILURE)

#define _dbWriteLeaf(db, l)	(((db)->flag & dbFlagBuffered) ? \
				((l)->flag |= dbFlagLeafDirty) : \
						_dbFlushLeaf(db, l))

#define	_dbNewBuffer(db, l, p)	(((p) = malloc((db)->d.llen)) ? \
				(free((l)->lbuf), ((l)->lbuf = (p)), SUCCESS) \
							: FAILURE)

#define	_dbSetUpUserFlag(db)	((dbErrorNo = dbErrorNoError), \
			((db)->flag = ((db)->d.flag & ~((1 << 8) - 1)) \
					| ((db)->flag & ((1 << 8) - 1))))

#define _dbWriteDir(db)		(((db)->flag & dbFlagBuffered) ? \
				((db)->d.flag |= dbFlagDirDirty) : \
						_dbFlushDir(db))

#define _dbCreateFile(s)	(open(s, O_CREAT | O_TRUNC | O_RDWR, \
					dbMode & 0666))

/* externs */

extern char _dbstrbuf[];
extern int _dbLeafCount;	/* not static: used by dbClose */
extern Database *_DB;		/* not used previously -- commandeered for 
				chain of open dbs for buffer sharing */

#include "db.h"

/* internal functions are usually static, visible in debuggable library */

#ifdef SHLIB

#define visibility static

#else

#define	visibility extern

#endif

visibility int _dbDoubleDir(Database *db);
visibility int _dbReadDir(Database *db);
visibility int _dbFlushDir(Database *db);
visibility unsigned long _dbHashKey(Data *d, u_short depth);
visibility int 
_dbCopyData(Database *db, Leaf *l, u_short k, Data *b, dbCopyType mode);
visibility int _dbSearchLeaf(Leaf *l, Data *d);
visibility int _dbPackLeaf(Database *db, Leaf *l);
visibility int _dbFreeLeaf(Database *db, Leaf *l, int freeFlag);
visibility Leaf *_dbCloseLeaf(Database *db, Leaf *l);
visibility Leaf *_dbCycleLeaf(Database *db);
visibility Leaf *_dbLeafDescriptor(Database *db);
visibility Leaf *_dbNewLeaf(Database *db);
visibility Leaf *_dbFetchLeaf(Database *db, long clidx);
visibility int _dbRemoveRecord(Database *db, Leaf *l, u_short recno);
visibility int _dbPutRecord(Database *db, Leaf *l, u_short recno, Data *d);
visibility int _dbSplitLeaf(Database *db);
visibility int
_dbFindRecord(Database *db, Data *d, long clidx, dbCopyType mode);
visibility int _dbGetRecord(Database *db, Data *d, dbCopyType mode);


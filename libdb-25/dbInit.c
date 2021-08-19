#ifdef SHLIB
#include "shlib.h"
#endif

#include "internal.h"
#include <sys/file.h>

#define strsave(s) strcpy(malloc(strlen(s) + 1), s)

visibility int
_dbReadDir(Database *db)
/* read directory file */
{
	int nl;
	register long i, j, k;
	register u_long *p, *q;
	long bufdir[LPDB];

        (void) lseek(db->D, SDIR, 0);
	p = db->dir;
	q = p + (1 << db->d.depth);
	while (nl = read(db->D, (char *) bufdir, sizeof(bufdir)) / SL)
	{
		for (i = 0; i < nl;)
			if (bufdir[i] & 1L)
			{
				k = bufdir[i++] & ~1L;
				if (i >= nl)
					return dbError(dbErrorInternal, 
					"directory of database '%s' corrupt\n", 
							_dbDirFile(db->name));

				if (p + bufdir[i] > q)
					goto errorTooBig;
					
				for (j = bufdir[i++]; j > 0; j--)
					*p++ = k;
			} else
			if (p < q)
				*p++ = bufdir[i++];
			else
				errorTooBig:
				return dbError(dbErrorInternal, 
				"directory of database '%s' over sized\n", 
							_dbDirFile(db->name));
	}

	return (p < q) ? dbError(dbErrorInternal, 
		"directory of database '%s' under sized\n", 
				_dbDirFile(db->name)) : SUCCESS;
}

static int
_dbOpenFile(Database *db, char *s) 
/* open 's', first try read/write, then try readonly. */
{
	int fd, flags = 0;

	if (dbFlags == -1)
	{
		if (! (db->d.flag & (dbFlagCompressed | dbFlagReadOnly)))
			flags |= O_RDWR;
	} else
	{
		flags |= dbFlags;
		if (db->d.flag & (dbFlagCompressed | dbFlagReadOnly))
			flags &= ~(O_WRONLY | O_RDWR);
		else
		if (! (flags & (O_WRONLY | O_RDWR)))
			db->d.flag |= dbFlagReadOnly;
	}

	if ((fd = open(s, flags, 0)) >= 0)
		return fd;

	if (flags & (O_WRONLY | O_RDWR))
	{
		flags &= ~(O_WRONLY | O_RDWR);
		db->d.flag |= dbFlagReadOnly;
		if ((fd = open(s, flags, 0)) < 0)
			(void) dbError(errno + dbErrorSystem, 
					"cannot open file '%s'\n", s);
	}

	return fd;
}

Database *
dbInit(char *name) 
/*
** Get database 'name' ready for access.
** The directory and leaf files must already exist
** (they can be created using 'dbCreate').
** Returns the open database if successful, otherwise returns NULL.
*/
{
	register Database *db, *pdb, *tdb;
	dbErrorType saveErrorNo;
	
	dbErrorNo = dbErrorNoError;
	db = (Database *) calloc(1, sizeof(Database));
	if (! db)
	{
		(void) dbError(dbErrorNoMemory, 
		"cannot allocate descriptor for database '%s'\n", name);
		return (Database *) 0;
	}

	db->D = db->L = -1;
	if ((db->D = _dbOpenFile(db, _dbDirFile(name))) < 0)
	{
		errorReturn:
		saveErrorNo = dbErrorNo;
		dbClose(db);
		dbErrorNo = saveErrorNo;
		return (Database *) 0;
	}

	db->name = strsave(name);
	if (read(db->D, (char *) &(db->d), SDIR) != SDIR)
	{
		(void) dbError(errno + dbErrorSystem, 
		"cannot read directory header for database '%s'\n", name);
		goto errorReturn;
	}
	
	if ((db->L = _dbOpenFile(db, _dbLeafFile(name))) < 0)
		goto errorReturn;

	db->flag = (db->d.flag |= dbFlagBuffered);
	db->dir = (u_long *) calloc((unsigned) 1 << db->d.depth, SL);
	if (! db->dir)
	{
		(void) dbError(dbErrorNoMemory, 
		"cannot allocate directory buffer for database '%s'\n", name);
		goto errorReturn;
	}

	if (! _dbReadDir(db))
		goto errorReturn;

	pdb = (Database *) 0;
	for (tdb = _DB; tdb; pdb = tdb, tdb = tdb->dnext)
		if (tdb->d.llen >= db->d.llen)
			break;

	dbLeafSize = db->d.llen;
	return (pdb) ? (db->dnext = pdb->dnext, pdb->dnext = db) 
					: (db->dnext = _DB, _DB = db);
}

#define countArray(x)	(sizeof(x)/sizeof(x[0]))

/* this table tunes leaf sizes to the documented magic sizes for malloc */
static const unsigned short _dbSizeTable[] =	{
	252, 340, 508, 680, 1020, 1360, 2044, 2724, 4088, 8192
};

int
dbCreate(char *name)
/*
** Create the files "name.[DL]" for a new database.
** Files will be zeroed (overwritten) if they already exist.
** The default leaf size, 'dbLeafSize' will be 'LEAFSIZE' (1024 bytes).
*/
{
	struct 	{
		dhead d;	/* header template for new directory */
		long zero;	/* initial leaf pointer is always zero */
	} dir;
	lhead *lh;
	register int i, fd;
	int writeSucceeded;

	if ((fd = _dbCreateFile(_dbDirFile(name))) < 0)
		return dbError(errno + dbErrorSystem, 
			"cannot create directory for database '%s'\n", name);

	bzero(&dir, sizeof(dir));	
	dir.d.dlcnt = 1;
	dir.d.magic = MAGIC;
	for (i = 0; i < countArray(_dbSizeTable); i++)
		if (_dbSizeTable[i] > dbLeafSize)
		{
			dbLeafSize = _dbSizeTable[i];
			goto sizeAdjusted;
		}

	dbLeafSize = (dbLeafSize + 8191) & ~8191;
	sizeAdjusted:
	dir.d.llen = (1 + dbLeafSize) & ~1;

	writeSucceeded = _dbVerifyWrite(fd, (char *) &dir, sizeof(dir));
	if (close(fd) || ! writeSucceeded)
	{
		(void) dbError(errno + dbErrorSystem, 
			"cannot write directory header for '%s'\n", name);
		errorReturn:
		(void) unlink(_dbDirFile(name));
		return FAILURE;
	}

	if ((fd = _dbCreateFile(_dbLeafFile(name))) < 0)
	{
		(void) dbError(errno + dbErrorSystem, 
			"cannot create leaf file for '%s'\n", name);
		goto errorReturn;
	}
	
	if (! (lh = (lhead *) calloc(1, dir.d.llen)))
	{
		(void) dbError(dbErrorNoMemory, 
			"cannot allocate leaf buffer for '%s'\n", name);
		errorReturn2:
		(void) unlink(_dbLeafFile(name));
		goto errorReturn;
	}

	lh->ldata = dir.d.llen;
	writeSucceeded = _dbVerifyWrite(fd, (char *) lh, dir.d.llen);
	free(lh);
	if (close(fd) || ! writeSucceeded)
	{
		(void) dbError(errno + dbErrorSystem, 
			"cannot write initial leaf for '%s'\n", name);
		goto errorReturn2;
	}

	return SUCCESS;
}

Database *
dbOpen(char *name)
/*
** Open the database 'name' for access, creating the files if necessary.
** The open database is returned, or NULL if there was some failure.
*/
{
	Database *db;
	int savedbErrors = dbErrors;

	dbErrors = 0;
	db = dbInit(name);
	dbErrors = savedbErrors;
	return ((db) ? db : (dbCreate(name) ? dbInit(name) : (Database *) 0));
}



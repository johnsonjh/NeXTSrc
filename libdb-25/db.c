/*
** Copyright 1987, NeXT Inc.
**
** Extendible hash database interface routines
** M. J. Hawley
** Bug fixes and performance tuning, J. Greenfield 04/24/89
**
*/

/* this routine is documented with mktemp(), but is not declared in libc.h */
int mkstemp(char *template);

#ifdef SHLIB
#include "shlib.h"
#endif 

#include "internal.h"

static char *
_dbBaseName(char *s)
/*
** Return pointer to the base file name in path 's'
*/
{
	char *p = rindex(s, '/');
	return p ? p + 1 : s;
}

visibility int
_dbDoubleDir(Database *db)
/* Double the directory: entry at n goes to 2n, 2n+1 */
{
	register long k;
	register u_long *dir;

	if (db->d.depth >= MAX_DEPTH)
		return dbError(dbErrorTooDeep, 
			"directory of '%s' at maximum depth\n", db->name);

	dir = (u_long *) malloc(((unsigned) 1 << (db->d.depth + 1)) * SL);
	if (! dir)
		return dbError(dbErrorNoMemory, 
			"cannot reallocate directory for '%s'\n", db->name);

	db->clidx <<= 1;
	for (k = (1 << db->d.depth) - 1; k >= 0; k--)
		dir[(k << 1) + 1] = dir[k << 1] = db->dir[k];

	free((char *) db->dir);
	db->dir = dir;
	return ++db->d.depth;
}

visibility int
_dbFlushDir(Database *db)
/* write directory file */
{
	dhead d;
	u_long bufdir[LPDB];
	register long i;
	register u_long *p, *q, j, k;

	(void) lseek(db->D, 0L, 0);
	d = db->d;
	d.flag = (db->d.flag & dbFlagCompressed);
	if (! _dbVerifyWrite(db->D, (char *) &d, sizeof(d)))
		errorReturn: return dbError(errno + dbErrorSystem, 
			"cannot write directory for '%s'\n", db->name);

	p = db->dir;
	q = p + (1 << db->d.depth);
	for (i = 0; p < q; )
	{
		if (i >= LPDB)
		{
			i = 0;
			if (! _dbVerifyWrite(db->D, (char *) bufdir, DBLOCK))
				goto errorReturn;
		}

		bufdir[i++] = k = *p++;
		if (i < LPDB)
		{
			for (j = 1; p < q; j++, p++)
				if (*p != k)
					break;

			if (j > 1)
			{
				bufdir[i++] = j;
				bufdir[i - 2] |= 1;
			}
		}
	}
		
	if (! _dbVerifyWrite(db->D, (char *) bufdir, SL * i))
		goto errorReturn;

	db->d.flag &= ~dbFlagDirDirty;
	return SUCCESS;
}

/* hash function, mercilessly plundered from ken's dbm... */
static const char hstab[]={61,57,53,49,45,41,37,33,29,25,21,17,13,9,5,1};
/* hstab[]={055,043,036,054,063,014,004,005,010,064,077,000,035,027,025,071};*/
static const long hltab[64] = {
	06100151277L, 06106161736L, 06452611562L, 05001724107L,
	02614772546L, 04120731531L, 04665262210L, 07347467531L,
	06735253126L, 06042345173L, 03072226605L, 01464164730L,
	03247435524L, 07652510057L, 01546775256L, 05714532133L,
	06173260402L, 07517101630L, 02431460343L, 01743245566L,
	00261675137L, 02433103631L, 03421772437L, 04447707466L,
	04435620103L, 03757017115L, 03641531772L, 06767633246L,
	02673230344L, 00260612216L, 04133454451L, 00615531516L,
	06137717526L, 02574116560L, 02304023373L, 07061702261L,
	05153031405L, 05322056705L, 07401116734L, 06552375715L,
	06165233473L, 05311063631L, 01212221723L, 01052267235L,
	06000615237L, 01075222665L, 06330216006L, 04402355630L,
	01451177262L, 02000133436L, 06025467062L, 07121076461L,
	03123433522L, 01010635225L, 01716177066L, 05161746527L,
	01736635071L, 06243505026L, 03637211610L, 01756474365L,
	04723077174L, 03642763134L, 05750130273L, 03655541561L,
};

visibility unsigned long
_dbHashKey(Data *d, u_short depth)
/*
** Return leftmost 'depth' bits of hashed data.
** max 32 bits of precision (length of unsigned long, limit of disk addressing)
*/
{
	register short i;
	register u_short f, hi = 0;
	register long lo = 0L;
	
	for(i = d->k.n - 1; i >= 0; i--)
	{
		f = d->k.s[i];
		lo += hltab[(hi += hstab[f & 017]) & 63];
		lo += hltab[(hi += hstab[(f >> 4) & 017]) & 63];
	}
	
	return (lo >> (32 - depth)) & ((unsigned)(1 << depth) - 1);
}

visibility int
_dbCopyData(Database *db, Leaf *l, u_short recno, Data *d, dbCopyType mode)
/*
** copy data from postion 'recno' in 'l' to data buffer 'd' in mode 'mode'.
*/
{
	register char *S;

#if defined(DEBUG)
	if (recno >= l->lhd->lrcnt)
		return dbError(dbErrorInternal, 
			"record %d in leaf %ld of '%s' invalid\n", 
						recno, l->lp, db->name);
#endif

	S = l->lbuf + l->list[recno];
	_dbGetLen(d->k.n, S);
	if (mode == dbCopyPointer)
		d->k.s = S;
	else
	if (mode == dbCopyKeyOnly || mode == dbCopyRecord)
		bcopy(S, d->k.s, d->k.n);
	
	S += d->k.n;
	_dbGetLen(d->c.n, S);
	if (mode == dbCopyPointer)
		d->c.s = S;
	else
	if (mode == dbCopyRecord)
		bcopy(S, d->c.s, d->c.n);
		
	return SUCCESS;
}

visibility int
_dbSearchLeaf(Leaf *l, Data *d)
/*
** Binary search for 'd' in 'l'; return position where 'd' belongs 
** if 'd' is found, sets dbFlagLeafMatch in 'l->flag', otherwise clears it.
*/
{
	register char *s1, *s2;
	register u_short n, low, high, mid;
	register short cmp;

	l->flag &= ~dbFlagLeafMatch;
	low = 0;
	high = l->lhd->lrcnt;
	cmp = -1;
	while (low < high)
	{
		mid = (low + high) >> 1;
		s1 = l->lbuf + l->list[mid];
		_dbGetLen(n, s1);
		s2 = d->k.s;
		if ((cmp = n - d->k.n) > 0)
			n = d->k.n;

		for(; n; n--)
			if (*s1++ != *s2++)
			{
				cmp = *--s1 - *--s2;
				break;
			}
			
		if (cmp < 0)
			low = mid + 1;
		else if (cmp > 0)
			high = mid;
		else
		{
			l->flag |= dbFlagLeafMatch;
			return mid;
		}
	}

	return low;
}

visibility int
_dbPackLeaf(Database *db, Leaf *l)
/*
** Coalesce data in 'l', return number of bytes recouped.
*/
{
	register Leaf *tl;
	register char *p, *t, *r;
	register u_short n;
	register int i, s;

	p = l->lbuf;
	for (i = s = 0; i < l->lhd->lrcnt; i++, s += t - r)
	{
		r = t = p + l->list[i];
		_dbGetLen(n, t); t += n; _dbGetLen(n, t); t += n;
	}

	s = db->d.llen - l->lhd->ldata - s;
	if (s > 0)
	{
		if (! (tl = _dbNewLeaf(db)))
			return 0;

		_dbSwapLeaf(l, tl);
		l->lhd->flag = tl->lhd->flag;
		l->lhd->ldepth = tl->lhd->ldepth;
		l->lhd->lrcnt = tl->lhd->lrcnt;
		l->lhd->ldata = s + tl->lhd->ldata;
		p = l->lbuf + l->lhd->ldata;
		for (i = 0; i < l->lhd->lrcnt; i++)
		{
			l->list[i] = (u_short) (p - l->lbuf);
			r = t = tl->lbuf + tl->list[i];
			_dbGetLen(n, t); t += n; _dbGetLen(n, t); t += n;
			bcopy(r, p, n = t - r); p += n;
		}

		(void) _dbFreeLeaf(db, tl, FALSE);
	} else
		s = 0;

	l->lhd->flag &= ~dbFlagCoalesceable;
	return s;
}

visibility int
_dbFreeLeaf(Database * db, Leaf *l, int force)
{
#if defined(DEBUG)
	if (l->flag & dbFlagLeafOpen)
	{
		dbError(dbErrorInternal, 
			"illegal free: leaf %ld in '%s'\n", l->lp, db->name);
		return FAILURE;
	}
#endif

	if (! l)
		return FAILURE;

	if (db->d.flag & dbFlagPrivate)
	{
		if (force || db->alloc > db->cache)
		{
			db->alloc--;
			_dbFreeToHeap(l);
		} else
			_dbFreeToList(db, l);
	} else
	{
		if (force || _dbLeafCount > dbMaxLeafBuf)
		{
			_dbLeafCount--;
			_dbFreeToHeap(l);
		} else
			_dbFreeToList(db, l);
	}

	return SUCCESS;
}

visibility Leaf *
_dbCloseLeaf(Database *db, Leaf *l)
/*
** remove a leaf from the hash table
*/
{
	register Leaf *tl;
	register u_short hash;

#if defined(DEBUG)
	if (! (l->flag & dbFlagLeafOpen))
	{
		(void) dbError(dbErrorInternal, 
			"illegal close: leaf %ld in '%s'\n", l->lp, db->name);
		return (Leaf *) 0;
	}
#endif

	if ((l->flag & dbFlagLeafDirty) && ! _dbFlushLeaf(db, l))
		return (Leaf *) 0;

	if (db->cleaf == l)
		db->cleaf = (Leaf *) 0;
		
	hash = __dbHashLeaf(db, l->lp);
	if (l == db->table[hash])
	{
		db->table[hash] = l->lnext;
		goto foundLeaf;
	}

	for (tl = db->table[hash]; tl->lnext; tl = tl->lnext)
		if (tl->lnext == l)
		{
			tl->lnext = l->lnext;
			goto foundLeaf;
		}

#if defined(DEBUG)
	(void) dbError(dbErrorInternal, 
		"leaf %ld not found for '%s'\n", l->lp, db->name);
#endif
	foundLeaf:
	l->flag &= ~dbFlagLeafOpen;
	if (db->nopen > 0)
		--db->nopen;
#if defined(DEBUG)
	else
		(void) dbError(dbErrorInternal, 
			"open count too small in '%s'\n", db->name);
#endif

	return l;
}

visibility int
_dbFlushTable(Database *db, int force)
{
	register Leaf *l;
	register short i;

	if (db->nopen > 0)
		for (i = 0; i < _LeafHash; ++i)
			while (l = db->table[i])
				if (! _dbFreeLeaf(db, 
						_dbCloseLeaf(db, l), force))
					return FAILURE;

	if (force)
		while (_dbGetFreeLeaf(db, l))
			if (! _dbFreeLeaf(db, l, force))
				return FAILURE;

	return SUCCESS;
}

visibility Leaf *
_dbCycleLeaf(Database *db)
/*
** cycle a buffer that can hold a leaf from the hash table of database 'db'.
*/
{
	register Leaf *l;
	register long i, j, location;

	location = random() % _LeafHash;
	i = j = location;
	while (j >= 0 || i < _LeafHash)
		if ((i < _LeafHash && (l = db->table[i++])) 
				|| (j >= 0 && (l = db->table[j--])))
			if (l != db->cleaf || (l = l->lnext))
				return _dbCloseLeaf(db, l);

	return (Leaf *) 0;
}

visibility Leaf *
_dbLeafDescriptor(Database *db)
{
	register char *p;
	register Database *tdb;
	register Leaf *l;

#if defined(DEBUG)
	++db->dastat.total;
#endif
	if (_dbGetFreeLeaf(db, l))
	{
#if defined(DEBUG)
		++db->dastat.cfree;
#endif
		return l;
	}

	if (db->d.flag & dbFlagPrivate)
	{
		if (db->alloc < db->cache || db->alloc < dbMinLeafBuf)
			if (_dbAllocLeaf(l))
			{
				++db->alloc;
				return l;
			}
	} else
	if (_dbLeafCount < dbMaxLeafBuf || _dbLeafCount < dbMinLeafBuf)
		if (_dbAllocLeaf(l))
		{
			++_dbLeafCount;
#if defined(DEBUG)
			++db->dastat.alloc;
#endif
			return l;
		}

	if ((db->nopen > (db->cleaf ? 1 : 0)) && (l = _dbCycleLeaf(db)))
	{
#if defined(DEBUG)
		++db->dastat.ccycle;
#endif
		return l;
	}

	if (db->d.flag & dbFlagPrivate)
		return (Leaf *) 0;

	for (tdb = db->dnext; tdb; tdb = tdb->dnext)
		if (! (tdb->d.flag & dbFlagPrivate))
		{
			if (_dbGetFreeLeaf(tdb, l))
			{
#if defined(DEBUG)
				++db->dastat.lfree;
#endif
				return l;
			}

			if ((tdb->nopen > (tdb->cleaf ? 1 : 0)) 
					&& (l = _dbCycleLeaf(tdb)))
			{
#if defined(DEBUG)
				++db->dastat.lcycle;
#endif
				return l;
			}
		}

	for (tdb = _DB; tdb && db != tdb; tdb = tdb->dnext)
		if (! (tdb->d.flag & dbFlagPrivate))
		{
			if (_dbGetFreeLeaf(tdb, l))
				return _dbNewBuffer(db, l, p) ? 
#if defined(DEBUG)
				(++db->dastat.sfree, l)
#else
				(l)
#endif
			: ((void) _dbFreeLeaf(tdb, l, FALSE), (Leaf *) 0);
			
			if ((tdb->nopen > (tdb->cleaf ? 1 : 0)) 
					&& (l = _dbCycleLeaf(tdb)))
				return _dbNewBuffer(db, l, p) ?
#if defined(DEBUG)
				(++db->dastat.scycle, l)
#else
				(l)
#endif
			: ((void) _dbFreeLeaf(tdb, l, FALSE), (Leaf *) 0);
		}

	return (Leaf *) 0;
}

visibility Leaf * 
_dbNewLeaf(Database *db)
/*
** get a buffer that can hold a leaf.
*/
{
	Leaf *l;
	
	if (l = _dbLeafDescriptor(db))
	{
		l->flag = l->lp = 0;
		l->list = (u_short *)(l->lbuf + SLHD);
		l->lnext = (Leaf *) 0;
		bzero(l->lbuf, db->d.llen);
		l->lhd->ldata = db->d.llen;
	} else
		dbError(dbErrorNoMemory, 
			"cannot obtain leaf buffer for '%s'\n", db->name);
		
	return l;
}


visibility Leaf * 
_dbFetchLeaf(Database *db, long clidx)
/*
** Retrieve leaf at entry 'clidx' in 'db'; return pointer to in-core copy.
** Some optimizations here to quickly locate cached leaves.
*/
{
	register u_long lp;
	register Leaf *l;

#if defined(DEBUG)
	++db->dbstat.fetch;
	if (clidx >= (1 << db->d.depth))
	{
		(void) dbError(dbErrorInternal, 
			"index %ld too large for '%s'\n", db->name);
		return (Leaf *) 0;
	}
#endif

	db->clidx = clidx;
	lp = _dbLeafAddress(db, clidx);
	if (db->nopen > 0)
	{
		if (db->cleaf && lp == db->cleaf->lp)
		{
#if defined(DEBUG)
			++db->dbstat.cleaf;
#endif
			return db->cleaf;
		}

		for (l = _dbHashLeaf(db, lp); l; l = l->lnext)
			if (l->lp == lp)
			{
#if defined(DEBUG)
				++db->dbstat.nopen;
#endif
				return db->cleaf = l;
			}
	}

	if (l = _dbNewLeaf(db))
	{
		l->lp = lp;
		if (_dbReadLeaf(db, l))
			return db->cleaf = _dbOpenLeaf(db, l);

		(void) dbError(errno + dbErrorSystem, 
			"cannot read leaf %ld for '%s'\n", lp, db->name);
		(void) _dbFreeLeaf(db, l, FALSE);
	}

	return db->cleaf = (Leaf *) 0;
}


visibility int
_dbRemoveRecord(Database *db, Leaf *l, u_short recno)
/*
** Remove data at 'recno' from 'l' -- mark leaf as "coalescable".
*/
{
#if defined(DEBUG)
	if (recno >= l->lhd->lrcnt)
		return dbError(dbErrorInternal, 
			"record %d in leaf %ld of '%s' invalid\n", 
						recno, l->lp, db->name);
#endif

	l->lhd->lrcnt--;
	l->lhd->flag |= dbFlagCoalesceable;
	if (recno < l->lhd->lrcnt)
		bcopy(l->list + recno + 1, l->list + recno, 
					SS * (l->lhd->lrcnt - recno));

	return SUCCESS;
}

/*
** dbDelete is an entry point into this library
*/

int
dbDelete(Database *db, Data *d)
/*
** Remove 'd' from Database 'db' -- only the key part of 'd' need be specified.
*/
{
	int returnVal;
	register Leaf *l;

	_dbSetUpUserFlag(db);
	if (db->d.flag & (dbFlagReadOnly | dbFlagCompressed))
		return dbError(dbErrorReadOnly, 
			"database '%s' compressed or read only\n", db->name);

	if (! (l = _dbFetchLeaf(db, _dbHashKey(d, db->d.depth)))) 
		return FAILURE;

	db->recno = _dbSearchLeaf(l, d);
	returnVal = (l->flag & dbFlagLeafMatch) ? 
		(_dbRemoveRecord(db, l, db->recno) && _dbWriteLeaf(db, l)) 
			: dbError(dbErrorNoKey, 
				"key not found in database '%s'\n", db->name);

	return ((db->d.flag & dbFlagPrivate) && (db->cache < 1)) ? 
		(_dbFreeLeaf(db, _dbCloseLeaf(db, l), FALSE) ? 
					returnVal : FAILURE) : returnVal;
}

visibility int
_dbPutRecord(Database *db, Leaf *l, u_short recno, Data *d)
/*
** Copy 'd' into 'l' at 'recno' if there's room.
** Return 0 if not, true if copied.
** If the copy fails at first, try packing the data in the leaf to
** see if enough storage can be recouped to accommodate the key.
*/
{
	register char *D;
	register short n, at, limit;

 	n = d->k.n + SS + d->c.n + SS;
	limit = SLHD + (l->lhd->lrcnt + 1) * SS;
  	do {
		at = l->lhd->ldata - n;
		if (at >= limit)
		{
			if (recno < l->lhd->lrcnt)
				bcopy(l->list + recno, l->list + recno + 1, 
						SS * (l->lhd->lrcnt - recno));

			D = l->lbuf + at;
			_dbSetLen(D, d->k.n); bcopy(d->k.s, D, d->k.n);
			D += d->k.n;
			_dbSetLen(D, d->c.n); bcopy(d->c.s, D, d->c.n);
			l->lhd->lrcnt++;
			return l->lhd->ldata = l->list[recno] = at;
		}
		
	} while ((l->lhd->flag & dbFlagCoalesceable) && _dbPackLeaf(db, l));
	
	return FAILURE;
}

visibility int
_dbSplitLeaf(Database *db)
/* Split leaf at 'db->cleaf' into two */
{
	int returnVal;
	register long i, mid, diff;
	Data td;
	register Leaf *l, *nl;

	if (! (l = db->cleaf) || (l->lp != _dbLeafAddress(db, db->clidx)))
		return dbError(dbErrorInternal, 
			"current leaf invalid in database '%s'\n", db->name);

	while (l->lhd->ldepth >= db->d.depth)
		if (! _dbDoubleDir(db))
			return FAILURE;

	if (! (nl = _dbNewLeaf(db)))
		return FAILURE;

	nl->lp = db->d.dlcnt * db->d.llen;
	_dbOpenLeaf(db, nl);
	diff = 1 << (db->d.depth - l->lhd->ldepth);
	mid = (db->clidx & ~(diff - 1)) + (diff >> 1);
	for(i = 0; i < l->lhd->lrcnt; i++)
	{
		_dbCopyData(db, l, i, &td, dbCopyPointer);
		if (mid <= _dbHashKey(&td, db->d.depth))
			continue;

		_dbRemoveRecord(db, l, i--);
		if (! _dbPutRecord(db, nl, nl->lhd->lrcnt, &td))
		{
			(void) dbError(dbErrorInternal, 
				"rehash failed in database '%s'\n", db->name);
			goto splitError;
		}
	}

	nl->lhd->ldepth = ++l->lhd->ldepth;
	if (_dbWriteLeaf(db, nl) && _dbWriteLeaf(db, l))
	{
		++db->d.dlcnt;
		for(i = db->clidx & ~(diff - 1); i < mid; i++)
			db->dir[i] = nl->lp;
		
		returnVal = _dbWriteDir(db);
		return ((db->d.flag & dbFlagPrivate) && (db->cache < 2)) ? 
			(_dbFreeLeaf(db, _dbCloseLeaf(db, 
				(db->clidx < mid) ? l : nl), FALSE) ? 
					returnVal : FAILURE) : returnVal;
	}

	(void) dbError(errno + dbErrorSystem, 
		"cannot flush leaves in database '%s'\n", db->name);
	splitError:
	(void) _dbFreeLeaf(db, _dbCloseLeaf(db, l), FALSE);
	(void) _dbFreeLeaf(db, _dbCloseLeaf(db, nl), FALSE);
	return FAILURE;
}

/*
** dbStore is an entry point into this library
*/

int
dbStore(Database *db, Data *d)
/*
** Store the key/content data pair 'd' in Database 'db'.
*/
{
	int returnVal;
	register Leaf *l;

	_dbSetUpUserFlag(db);
	if (db->d.flag & (dbFlagReadOnly | dbFlagCompressed))
		return dbError(dbErrorReadOnly, 
			"database '%s' compressed or read only\n", db->name);
		
	if (d->k.n + d->c.n + MINOVERHEAD > db->d.llen)
		return dbError(dbErrorTooLarge, 
			"record size %d too large for '%s'\n", 
					d->k.n + d->c.n, db->name);

	do
	{
		if (! (l = _dbFetchLeaf(db, _dbHashKey(d, db->d.depth))))
			return FAILURE;

		db->recno = _dbSearchLeaf(l, d);
		if (l->flag & dbFlagLeafMatch)
			_dbRemoveRecord(db, l, db->recno);

		if (_dbPutRecord(db, l, db->recno, d))
		{
			returnVal = _dbWriteLeaf(db, l);
			return ((db->d.flag & dbFlagPrivate) 
						&& (db->cache < 1)) ? 
				(_dbFreeLeaf(db, _dbCloseLeaf(db, l), FALSE) ? 
					returnVal : FAILURE) : returnVal;
		}

	} while (_dbSplitLeaf(db));

	return FAILURE;
}

visibility int
_dbFindRecord(Database *db, Data *d, long clidx, dbCopyType mode)
/* 
** Retrieve record specified by 'd->k', setting 'db->cleaf' and 'db->recno'
*/
{
	register Leaf *l;

	_dbSetUpUserFlag(db);
	if (! (l = _dbFetchLeaf(db, clidx)))
		return FAILURE;

	db->recno = _dbSearchLeaf(l, d);
	return (l->flag & dbFlagLeafMatch) ? 
		_dbCopyData(db, l, db->recno, d, mode) : 
			dbError(dbErrorNoKey, 
				"key not found in database '%s'\n", db->name);
}

/*
** dbFetch is an entry point to this library
*/

int
dbFetch(Database *db, Data *d)
/* 
** Fetch data by key:
** with key 'd->k' known, attempt to fill in the content part ('d->c').
*/
{
	int returnVal =  _dbFindRecord(db, d, 
		_dbHashKey(d, db->d.depth), dbCopyRecord);

	return ((db->d.flag & dbFlagPrivate) && (db->cache < 1)) ? 
		(_dbFreeLeaf(db, _dbCloseLeaf(db, db->cleaf), FALSE) ? 
					returnVal : FAILURE) : returnVal;
}

/*
** dbFind is an entry point to this library
*/

int
dbFind(Database *db, Data *d)
/* 
** True if 'd->k' is in 'db' -- only the key part of 'd' need be specified
** (use 'dbGet()' to read the contents).
*/
{
	return _dbFindRecord(db, d, _dbHashKey(d, db->d.depth), dbCopyLength);
}

visibility int
_dbGetRecord(Database *db, Data *d, dbCopyType mode)
/* 
** copy data from record at 'd->cleaf' and 'db->recno' in specified mode
*/
{
	int returnVal;

	_dbSetUpUserFlag(db);
	returnVal = (db->cleaf == (Leaf *) 0) ? FAILURE : 
		((d == (Data *) 0) ? SUCCESS : 
			_dbCopyData(db, db->cleaf, db->recno, d, mode));

	return ((db->d.flag & dbFlagPrivate) && (db->cache < 1)) ? 
		(_dbFreeLeaf(db, _dbCloseLeaf(db, db->cleaf), FALSE) ? 
					returnVal : FAILURE) : returnVal;
}

/*
** dbSetKey is an entry point to this library
*/

int
dbSetKey(Database *db, Data *d)
/*
** Get the current record and put the key and length of content in 'd'.
** Do this after calling 'dbFirst()', or 'dbNext()' to get the Key.
*/
{
	return _dbGetRecord(db, d, dbCopyKeyOnly);
}

/*
** dbGet is an entry point to this library
*/

int
dbGet(Database *db, Data *d)
/*
** Get the current record and put it in 'd'.
** Do this after calling 'dbFirst()', 'dbNext()' or 'dbFind()' to get the data.
*/
{
	return _dbGetRecord(db, d, dbCopyRecord);
}

/*
** all routines below are the entry points to this library
*/

int
dbFirst(Database *db, Data *d)
/*
 * Set 'd' to the first record in the Database 'db'.
 * Sizes of 'd' are returned in 'd'.
 */
{
	_dbSetUpUserFlag(db);
	db->recno = 0;
	return _dbFetchLeaf(db, 0L) && ((d == (Data *) 0) ? SUCCESS : 
		_dbCopyData(db, db->cleaf, db->recno, d, dbCopyLength));
}

int
dbNext(Database *db, Data *d)
/* 
** Set 'd' to the next record in Database 'db'.
** The order is uninteresting (it depends on a hash function).
** Fails if there are no more records. See 'dbFirst()'.
*/
{
	register long i, k;
	register Leaf *nl, *cl;

	_dbSetUpUserFlag(db);
	if (! (cl = db->cleaf) && ! (cl = _dbFetchLeaf(db, db->clidx)))
		return FAILURE;

	if (++db->recno < cl->lhd->lrcnt)
		goto foundNext;

	db->recno = 0;
	for (i = 1 + db->clidx, k = 1 << db->d.depth; i < k; i++)
		if (cl->lp != _dbLeafAddress(db, i))
		{
			if (! (nl = _dbFetchLeaf(db, i)))
				return FAILURE;

			if (! _dbFreeLeaf(db, _dbCloseLeaf(db, cl), FALSE))
				return FAILURE;

			if ((cl = nl)->lhd->lrcnt > 0)
				foundNext: return (d == (Data *) 0) ? 
					SUCCESS : _dbCopyData(db, cl, 
						db->recno, d, dbCopyLength);
		}

	db->clidx = 0;
	db->cleaf = (Leaf *) 0;
	return FAILURE;
}

int
dbFlush(Database *db)
/*
 * 'dbFlush(db)' writes any unwritten blocks to disk,
 * (Set 'db->flag &= ~dbFlagBuffered' after calling 'dbInit()' or 'dbOpen()'
 * to make writes synchronous).
 */
{
	register short i;
	register Leaf *l;

	_dbSetUpUserFlag(db);
	if (db->d.flag & (dbFlagReadOnly | dbFlagCompressed))
		return dbError(dbErrorReadOnly, 
			"database '%s' compressed or read only\n", db->name);

	if ((db->d.flag & dbFlagDirDirty) && ! _dbFlushDir(db))
		return FAILURE;

	if (db->nopen > 0)
		for (i = 0; i < _LeafHash; ++i)
			for (l = db->table[i]; l; l = l->lnext)
		if ((l->flag & dbFlagLeafDirty) && ! _dbFlushLeaf(db, l))
			return FAILURE;

	return SUCCESS;
}

int
dbCompress(Database *db)
{
	char template[257];
	int fd, returnVal;
	Data td;
	register long k;
	register u_long i, j, eof, diff;
	register Leaf *l;

	_dbSetUpUserFlag(db);
	if (db->d.flag & (dbFlagReadOnly | dbFlagCompressed))
		return dbError(dbErrorReadOnly, 
			"database '%s' compressed or read only\n", db->name);

	_dbFlushTable(db, FALSE);
	if (! (l = _dbNewLeaf(db)))
		return FAILURE;

	sprintf(template, "%.247s.C.XXXXXX", _dbBaseName(db->name));
	if ((fd = mkstemp(template)) < 0)
	{
		(void) dbError(errno + dbErrorSystem, 
			"cannot create compression file for '%s'\n", db->name);
		errorReturn:
		(void) _dbFreeLeaf(db, l, FALSE);
		return FAILURE;
	}

	eof = lseek(db->L, 0L, 2);
	(void) lseek(db->L, 0L, 0);
	for(i = j = 0; i < eof; i += db->d.llen, j += l->lhd->lsize)
	{
		if (read(db->L, l->lbuf, db->d.llen) < 0)
		{
			(void) dbError(errno + dbErrorSystem, 
				"leaf read failed for '%s'\n", db->name);
			goto errorReturn;
		}

		if (l->lhd->flag & dbFlagCoalesceable)
			(void) _dbPackLeaf(db, l);

		k = db->d.llen - l->lhd->ldata;
		l->lhd->lsize = (1 + k + SLHD + l->lhd->lrcnt * SS) & ~1;
		if (l->lhd->lrcnt)
		{
			diff = db->d.llen - l->lhd->lsize;
			bcopy(l->lbuf + l->lhd->ldata, 
				l->lbuf + l->lhd->ldata - diff, k);
			l->lhd->ldata -= diff;
			for (k = 0; k < l->lhd->lrcnt; k++)
				l->list[k] -= diff;

			diff = db->d.depth - l->lhd->ldepth;
			_dbCopyData(db, l, 0, &td, dbCopyPointer);
			k = _dbHashKey(&td, l->lhd->ldepth) << diff;
			for (diff = k + (1 << diff); k < diff; k++)
				db->dir[k] = j;
		}

		if (write(fd, l->lbuf, l->lhd->lsize) != l->lhd->lsize)
		{
			(void) dbError(errno + dbErrorSystem, 
				"leaf write failed for '%s'\n", db->name);
			goto errorReturn;
		}
	}

	(void) _dbFreeLeaf(db, l, FALSE);
	returnVal = (close(db->L) || rename(template, _dbLeafFile(db->name))
		|| fchmod(fd, dbMode & 0666)) ? dbError(errno + dbErrorSystem, 
			"cannot replace leaf file for '%s'\n", db->name) : 1; 

	db->L = fd;
	db->flag = (db->d.flag |= dbFlagCompressed);
	return _dbWriteDir(db) ? returnVal : FAILURE;
}

int
dbExpand(Database *db)
{
	char template[257];
	int fd, returnVal;
	Data td;
	register Leaf *l;
	register long k;
	register u_long i, j, eof, len, diff;
	register char *r, *t;

	_dbSetUpUserFlag(db);
	if (db->d.flag & dbFlagReadOnly)
		return dbError(dbErrorReadOnly, 
			"database '%s' is read only\n", db->name);

	if (! (db->d.flag & dbFlagCompressed))
		return dbError(dbErrorReadOnly, 
			"database '%s' is not compressed\n", db->name);

	_dbFlushTable(db, FALSE);
	if (! (l = _dbNewLeaf(db)))
		return FAILURE;

	sprintf(template, "%.247s.E.XXXXXX", db->name);
	if ((fd = mkstemp(template)) < 0)
	{
		dbError(errno + dbErrorSystem, 
			"cannot create expansion file for %s\n", 
						_dbBaseName(db->name));
		errorReturn:
		(void) _dbFreeLeaf(db, l, FALSE);
		return FAILURE;
	}

	eof = lseek(db->L, 0L, 2);
	(void) lseek(db->L, 0L, 0);
	for(i = j = 0; i < eof; i += len, j += db->d.llen)
	{
		l->lp = i;
		if (! _dbReadLeaf(db, l))
		{
			(void) dbError(errno + dbErrorSystem, 
				"leaf read failed for '%s'\n", db->name);
			goto errorReturn;
		}

		len = l->lhd->lsize - (SLHD + SS * l->lhd->lrcnt) 
						- (l->lhd->ldata & 1);
		if (len < 0)
		{
			len = 0;
			for (k = 0; k < l->lhd->lrcnt; k++)
			{
				r = t = l->lbuf + l->list[k];
				_dbGetLen(diff, t); t += diff;
				_dbGetLen(diff, t); t += diff;
				len += t - r;
			}

			l->lhd->lsize = 
				(1 + len + SLHD + SS * l->lhd->lrcnt) & ~1;
		}

		if (l->lhd->lrcnt)
		{
			diff = db->d.llen - l->lhd->lsize;
			bcopy(l->lbuf + l->lhd->ldata, 
				l->lbuf + l->lhd->ldata + diff, len);
			bzero(l->lbuf + l->lhd->ldata, diff);
			l->lhd->ldata += diff;
			for (k = 0; k < l->lhd->lrcnt; k++)
				l->list[k] += diff;

			diff = db->d.depth - l->lhd->ldepth;
			_dbCopyData(db, l, 0, &td, dbCopyPointer);
			k = _dbHashKey(&td, l->lhd->ldepth) << diff;
			for (diff = k + (1 << diff); k < diff; k++)
				db->dir[k] = j;
		}

		len = l->lhd->lsize;
		l->lhd->lsize = 0;
		if (write(fd, l->lbuf, db->d.llen) != db->d.llen)
		{
			(void) dbError(errno + dbErrorSystem, 
				"leaf write failed for '%s'\n", db->name);
			goto errorReturn;
		}
	}

	(void) _dbFreeLeaf(db, l, FALSE);
	returnVal = (close(db->L) || rename(template, _dbLeafFile(db->name)) 
		|| fchmod(fd, dbMode & 0666)) ? dbError(errno + dbErrorSystem, 
			"cannot replace leaf file for '%s'\n", db->name) : 1; 
			
	db->L = fd;
	db->flag = (db->d.flag &= ~dbFlagCompressed);
	return _dbWriteDir(db) ? returnVal : FAILURE;
}

void
dbSetCache(Database *db, u_short limit, int private)
{
	_dbSetUpUserFlag(db);
	if (private)
	{
		_dbFlushTable(db, TRUE);
		db->cache = limit;
		db->flag = (db->d.flag |= dbFlagPrivate);
	} else
	if (db->d.flag & dbFlagPrivate)
	{
		_dbFlushTable(db, TRUE);
		db->flag = (db->d.flag &= ~dbFlagPrivate);
	} else
		_dbFlushTable(db, FALSE);
}

int
dbClose(Database *db)
/*
** Close the directory and leaf files for database 'db', 
** freeing up memory used by 'db', and flushing any unwritten data to disk.
*/
{
	int returnVal = 0;
	register Database *tdb;

	if (! db)
		return FAILURE;
	
	_dbSetUpUserFlag(db);
	if (db == _DB)
		_DB = db->dnext;
	else
		for (tdb = _DB; tdb; tdb = tdb->dnext)
			if (tdb->dnext == db)
			{
				tdb->dnext = db->dnext;
				break;
			}

	if (db->L >= 0)
	{
		_dbFlushTable(db, TRUE);
		if (close(db->L) < 0)
			returnVal = errno;
	}

	if (db->D >= 0)
	{
		if ((db->d.flag & dbFlagDirDirty) && ! _dbFlushDir(db))
			returnVal = dbErrorNo;

		if (close(db->D) < 0)
			returnVal = errno;
	}

	if (db->name)
	{
		_dbExtFile(db->name, "");
		free(db->name);
	} else
		_dbExtFile("UNNAMED", "");
		
	if (db->dir)
		free((char *) db->dir);

	free((char *) db);
	return (returnVal) ? dbError(returnVal + dbErrorSystem, 
		"cannot close database '%s'\n", _dbstrbuf) : SUCCESS;
}



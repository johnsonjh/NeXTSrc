#include <stdio.h>
#include "dbUtil.h"

int
_dbReport(Database *db, int verbose)
{
	register Leaf *l;
	register long i, k, diff, lp, oldlp = -1L;
	float usage, totload = 0.0, totrec = 0.0;
	FILE *fp;

	fp = (dbMsgs) ? dbMsgs : stderr;
	fprintf(fp, "\nDatabase '%s' --\n", db->name);
	_dbPrintD(&db->d, fp, verbose);
	k = 1 << db->d.depth;
	if (verbose)
		fprintf(fp, "\n");
	
	for(i = 0; i < k; i++)
	{
		if ((lp = _dbLeafAddress(db, i)) == oldlp)
			continue;
			
		oldlp = lp;
		if (! (l = _dbFetchLeaf(db, i)))
			return FAILURE;

		if (l->lhd->flag & dbFlagCoalesceable)
			(void) _dbPackLeaf(db, l);

		usage = 100.0 * (float) (SLHD + l->lhd->lrcnt * SS + 
			db->d.llen - l->lhd->ldata) / (float) db->d.llen;
		if (verbose)
		{
			if (diff = db->d.depth - l->lhd->ldepth)
				fprintf(fp, "\tentries %ld-%ld", 
						i, i + (1 << diff) - 1);
			else
				fprintf(fp, "\tentry   %ld", i);

			fprintf(fp, 
		"=%ld\n\t\tdepth=%2d, count=%4d, usage=%.2f%%\n", 
		    		l->lp, l->lhd->ldepth, l->lhd->lrcnt, usage);
		}

		totload += usage;
		totrec += l->lhd->lrcnt;
	}

	if (verbose)
		fprintf(fp, "\n");
	
	fprintf(fp, 
	"\ttotal records=%.0f, %.2f per leaf; mean leaf usage=%.2f%%\n", 
			totrec, totrec / db->d.dlcnt, totload / db->d.dlcnt);
	return SUCCESS;
}

// visibility Leaf * 
static Leaf *
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


static int
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

static Leaf * 
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

static int
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

static Leaf *
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
static Leaf *
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
static Leaf *
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


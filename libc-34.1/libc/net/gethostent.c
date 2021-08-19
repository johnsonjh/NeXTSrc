/* @(#)gethostent.c	1.3 87/08/27 3.2/4.3NFSSRC */
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)gethostent.c	1.6 88/07/27 4.0NFSSRC; from	5.3 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#if	NeXT
#include <stdlib.h>
#endif	NeXT
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <rpcsvc/ypclnt.h>
#include <ndbm.h>

/*
 * Internet version.
 */
static struct hostdata {
	FILE	*hostf;
	DBM	*host_db;	/* set by gethostbyname(), gethostbyaddr() */
	char	*current;
	int	currentlen;
	int	stayopen;
#define	MAXALIASES	35
	char	*host_aliases[MAXALIASES];
	char	*host_addrs[2];
#define	MAXADDRSIZE	14
	char	hostaddr[MAXADDRSIZE];
	char	*addr_list[2];
	char	line[BUFSIZ+1];
	struct	hostent host;
	char	*domain;
	char	*map;
	int	usingyellow;
} *hostdata, *_hostdata();
#define	MAXADDRSIZE	14

static	struct hostent *interpret();
#if	NeXT
struct hostent *_old_gethostent();
static int yellowup();
#else
struct	hostent *gethostent();
#endif	NeXT
char	*inet_ntoa();
static char *any();

static	char *HOSTDB = "/etc/hosts";

static struct hostdata *
_hostdata()
{
	register struct hostdata *d = hostdata;

	if (d == 0) {
		d = (struct hostdata *)calloc(1, sizeof (struct hostdata));
		hostdata = d;
		d->host_addrs[0] = d->hostaddr;
		d->host_addrs[1] = NULL;
	}
	return (d);
}

#if 	NeXT
extern int h_errno;
#else
int h_errno;
#endif	NeXT

static struct hostent *
fetchhost(key)
	datum key;
{
	register struct hostdata *d = _hostdata();
	register char *cp, *tp, **ap;
	int naliases;

	if (d == 0) 
		return ((struct hostent*)NULL);
	if (key.dptr == 0)
		return ((struct hostent *)NULL);
	key = dbm_fetch(d->host_db, key);
	if (key.dptr == 0)
		return ((struct hostent *)NULL);
	cp = key.dptr;
	tp = d->line;
	d->host.h_name = tp;
	while (*tp++ = *cp++)
		;
	bcopy(cp, (char *)&naliases, sizeof(int)); cp += sizeof (int);
	for (ap = d->host_aliases; naliases > 0; naliases--) {
		*ap++ = tp;
		while (*tp++ = *cp++)
			;
	}
	*ap = (char *)NULL;
	d->host.h_aliases = d->host_aliases;
	bcopy(cp, (char *)&d->host.h_addrtype, sizeof (int));
	cp += sizeof (int);
	bcopy(cp, (char *)&d->host.h_length, sizeof (int));
	cp += sizeof (int);
	d->host.h_addr_list = d->host_addrs;
	d->host.h_addr = tp;
	bcopy(cp, tp, d->host.h_length);
        return (&d->host);
}

struct hostent *
#if	NeXT
_old_gethostbyname(name)
#else
gethostbyname(name)
#endif	NeXT
	register char *name;
{
	register struct hostdata *d = _hostdata();
	register struct hostent *p;
	register char **cp;
	char *val = NULL;
	char *lowname;          /* local copy of name in lower case */
	datum key;
	int vallen;

	if (d == 0) {
		h_errno = NO_RECOVERY;
		return ((struct hostent*)NULL);
	}

	lowname = (char *)malloc(strlen(name)+1);
	for (val = lowname; *name; name++)
		*val++ = isupper(*name) ? tolower(*name) : *name;
	*val = '\0';

	d->map = "hosts.byname";
#if	NeXT
	_old_sethostent(0);
#else
	sethostent(0);
#endif	NeXT
	if (!d->usingyellow) {
	    if ((d->host_db == (DBM *)NULL)
	      && ((d->host_db = dbm_open(HOSTDB, O_RDONLY)) == (DBM *)NULL)) {
#if	NeXT
		while (p = _old_gethostent()) {
#else
		while (p = gethostent()) {
#endif	NeXT
	/* XXX - should be stricmp() (case insensitive compares) */
			if (strcmp(p->h_name, lowname) == 0)
				break;
			for (cp = p->h_aliases; cp != 0 && *cp != 0; cp++)
				if (strcmp(*cp, lowname) == 0)
					goto found;
		}
	    } else {
		key.dptr = lowname;
		key.dsize = strlen(lowname);
		p = fetchhost(key);
	    }
	} else {
		if (yp_match(d->domain, d->map,
		    lowname, val - lowname, &val, &vallen))
			p = NULL;
		else {
			p = interpret(val, vallen);
			free(val);
		}
	}
found:
	if (p == NULL)
		h_errno = HOST_NOT_FOUND;
#if	NeXT
	_old_endhostent();
#else
	endhostent();
#endif	NeXT
	free(lowname);        
	return (p);
}

struct hostent *
#if	NeXT
_old_gethostbyaddr(addr, len, type)
#else
gethostbyaddr(addr, len, type)
#endif	NeXT
	char *addr;
	register int len, type;
{
	register struct hostdata *d = _hostdata();
	register struct hostent *p;
	datum key;
	char *adrstr, *val = NULL;
	int vallen;

	if (d == 0) {
		h_errno = NO_RECOVERY;
		return ((struct hostent*)NULL);
	}
	d->map = "hosts.byaddr";
#if	NeXT
	_old_sethostent(0);
#else
	sethostent(0);
#endif	NeXT
	if (!d->usingyellow) {
	    if ((d->host_db == (DBM *)NULL)
		    && ((d->host_db = dbm_open(HOSTDB, O_RDONLY)) 
		    == (DBM *)NULL)) {
#if	NeXT
		while (p = _old_gethostent()) {
#else
		while (p = gethostent()) {
#endif	NeXT
			if (p->h_addrtype != type || p->h_length != len)
				continue;
			if (bcmp(p->h_addr_list[0], addr, len) == 0)
				break;
		}
	    } else {
	        key.dptr = addr;
	        key.dsize = len;
	        p = fetchhost(key);
	    }
	} else {
		adrstr = inet_ntoa(*(struct in_addr *)addr);
		if (yp_match(d->domain, d->map,
		    adrstr, strlen(adrstr), &val, &vallen))
			p = NULL;
		else {
			p = interpret(val, vallen);
			free(val);
		}
	}
	if (p == NULL)
	    h_errno = HOST_NOT_FOUND;
#if	NeXT
	_old_endhostent();
#else
	endhostent();
#endif	NeXT
	return (p);
}

#if	NeXT
_old_sethostent(f)
#else
sethostent(f)
#endif	NeXT
	int f;
{
	register struct hostdata *d = _hostdata();

	if (d == 0)
		return;
	if (d->hostf == NULL)
		d->hostf = fopen(HOSTDB, "r");
	else
		rewind(d->hostf);
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
	if (d->map)
		yellowup();	/* recompute whether yellow pages are up */
}

#if	NeXT
_old_endhostent()
#else
endhostent()
#endif	NeXT
{
	register struct hostdata *d = _hostdata();

	if (d == 0)
		return;
	if (d->current && !d->stayopen) {
		free(d->current);
		d->current = NULL;
	}
	if (d->hostf && !d->stayopen) {
		fclose(d->hostf);
		d->hostf = NULL;
	}
	if (!d->usingyellow && !d->stayopen)
		if (d->host_db) {
			dbm_close(d->host_db);
			d->host_db = (DBM *)NULL;
		}
}

struct hostent *
#if	NeXT
_old_gethostent()
#else
gethostent()
#endif	NeXT
{
	register struct hostdata *d = _hostdata();
	struct hostent *hp;
	char *key = NULL, *val = NULL;
	int keylen, vallen;

	if (d == 0)
		return ((struct hostent*)NULL);
	d->map = "hosts.byaddr";
	yellowup();
	if (!d->usingyellow) {
		if (d->hostf == NULL && (d->hostf = fopen(HOSTDB, "r")) == NULL)
			return (NULL);
	        if (fgets(d->line, BUFSIZ, d->hostf) == NULL)
			return (NULL);
		return (interpret(d->line, strlen(d->line)));
	}
	if (d->current == NULL) {
		if (yp_first(d->domain, d->map,
		    &key, &keylen, &val, &vallen))
			return (NULL);
	} else {
		if (yp_next(d->domain, d->map,
		    d->current, d->currentlen, &key, &keylen, &val, &vallen))
			return (NULL);
	}
	if (d->current)
		free(d->current);
	d->current = key;
	d->currentlen = keylen;
	hp = interpret(val, vallen);
	free(val);
	return (hp);
}

static struct hostent *
interpret(val, len)
char	*val;
int	len;
{
	register struct hostdata *d = _hostdata();
	char *p;
	register char *cp, **q;

	if (d == 0)
		return (0);
	strncpy(d->line, val, len);
	p = d->line;
	d->line[len] = '\n';
	if (*p == '#')
		return (gethostent());
	cp = any(p, "#\n");
	if (cp == NULL)
		return (gethostent());
	*cp = '\0';
	cp = any(p, " \t");
	if (cp == NULL)
		return (gethostent());
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	d->addr_list[0] = d->hostaddr;
	d->addr_list[1] = NULL;
	d->host.h_addr_list = d->addr_list;
	*((u_long *)d->host.h_addr) = inet_addr(p);
	d->host.h_length = sizeof (u_long);
	d->host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	d->host.h_name = cp;
	q = d->host.h_aliases = d->host_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &(d->host_aliases[MAXALIASES - 1]))
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&d->host);
}

#if	NeXT
_old_sethostfile(file)
#else
sethostfile(file)
#endif	NeXT
	char *file;
{
	HOSTDB = file;
}

static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

/* 
 * check to see if yellow pages are up, and store that fact in usingyellow.
 */
static
yellowup()
{
	register struct hostdata *d = _hostdata();

        if (d->domain == NULL)
                d->usingyellow = usingypmap(&d->domain, d->map);
}

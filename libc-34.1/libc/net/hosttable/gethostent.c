/* @(#)gethostent.c	1.3 87/08/27 3.2/4.3NFSSRC */
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)gethostent.c	5.3 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netdb.h>
#include <rpcsvc/ypclnt.h>
#include <ndbm.h>
#include <ctype.h>

/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14

static char domain[256];
static char *current = NULL;	/* current entry, analogous to hostf */
static int currentlen;
static struct hostent *interpret();
struct hostent *_old_gethostent();
char *inet_ntoa();
static FILE *hostf = NULL;
static int usingyellow;			/* are yellow pages up? */

static char	*_host_file = "/etc/hosts";
static int	_host_stayopen;
static DBM	*_host_db;			/* set by gethostbyname(), gethostbyaddr() */

static char *any();
static yellowup();

extern int h_errno;

static struct hostent *
fetchhost(key)
	datum key;
{
	static struct hostent host;
	static char *host_aliases[MAXALIASES];
	static char hostbuf[BUFSIZ+1];
	static char *host_addrs[2];
	register char *cp, *tp, **ap;
	int naliases;

	if (key.dptr == 0)
		return ((struct hostent *)NULL);
	key = dbm_fetch(_host_db, key);
	if (key.dptr == 0)
	return ((struct hostent *)NULL);
	cp = key.dptr;
	tp = hostbuf;
	host.h_name = tp;
	while (*tp++ = *cp++)
		;
	bcopy(cp, (char *)&naliases, sizeof(int)); cp += sizeof (int);
	for (ap = host_aliases; naliases > 0; naliases--) {
		*ap++ = tp;
		while (*tp++ = *cp++)
			;
	}
	*ap = (char *)NULL;
	host.h_aliases = host_aliases;
	bcopy(cp, (char *)&host.h_addrtype, sizeof (int));
	cp += sizeof (int);
	bcopy(cp, (char *)&host.h_length, sizeof (int));
	cp += sizeof (int);
	host.h_addr_list = host_addrs;
	host.h_addr = tp;
	bcopy(cp, tp, host.h_length);
        return (&host);
}

struct hostent *
_old_gethostbyaddr(addr, length, type)
	char *addr;
	register int length;
	register int type;
{
	register struct hostent *hp;
	datum key;
	int reason;
	char *adrstr, *val;
	int vallen;

    yellowup(0);
    if (!usingyellow) {
	if ((_host_db == (DBM *)NULL)
	  && ((_host_db = dbm_open(_host_file, O_RDONLY)) == (DBM *)NULL)) {
		_old_sethostent(_host_stayopen);
		while (hp = _old_gethostent()) {
			if (hp->h_addrtype == type && hp->h_length == length
			    && bcmp(hp->h_addr, addr, length) == 0)
				break;
		}
		if (!_host_stayopen)
			_old_endhostent();
		if ( hp == NULL)
			h_errno = HOST_NOT_FOUND;
		return (hp);
	}
	key.dptr = addr;
	key.dsize = length;
	hp = fetchhost(key);
	if (!_host_stayopen) {
		dbm_close(_host_db);
		_host_db = (DBM *)NULL;
	}
	if ( hp == NULL)
		h_errno = HOST_NOT_FOUND;
        return (hp);
    }
    else {
		_old_sethostent(_host_stayopen);
	    adrstr = (char *) inet_ntoa(*(int *)addr);
    	if (reason = yp_match(domain, "hosts.byaddr",
    	    adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
    		fprintf(stderr, "reason yp_first failed is %d\n",
    		    reason);
#endif
    		hp = NULL;
    	    }
    	else {
    		hp = interpret(val, vallen);
    		free(val);
    	}
    }
    _old_endhostent();
    return (hp);
}

struct hostent *
_old_gethostbyname(nam)
	register char *nam;
{
	register struct hostent *hp;
	register char **cp;
	datum key;
	char lowname[128];
	register char *lp = lowname;
	int reason;
	char *val = nam;            /* squirrel away copy of nam */
	int vallen;

	while (*nam)
		if (isupper(*nam))
			*lp++ = tolower(*nam++);
		else
			*lp++ = *nam++;
	*lp = '\0';
	nam = val;

    yellowup(0);
    if (!usingyellow) {
	if ((_host_db == (DBM *)NULL)
	  && ((_host_db = dbm_open(_host_file, O_RDONLY)) == (DBM *)NULL)) {
		_old_sethostent(_host_stayopen);
		while (hp = _old_gethostent()) {
			if (strcmp(hp->h_name, lowname) == 0)
				break;
			for (cp = hp->h_aliases; cp != 0 && *cp != 0; cp++)
				if (strcmp(*cp, lowname) == 0)
					goto found;
		}
	found:
		if (!_host_stayopen)
			_old_endhostent();
		return (hp);
	}
        key.dptr = lowname;
        key.dsize = strlen(lowname);
	hp = fetchhost(key);
	if (!_host_stayopen) {
		dbm_close(_host_db);
		_host_db = (DBM *)NULL;
	}
	if ( hp == NULL)
		h_errno = HOST_NOT_FOUND;
        return (hp);
    }
    else {
    	_old_sethostent(_host_stayopen);
    	if (reason = yp_match(domain, "hosts.byname",
			nam, strlen(nam), &val, &vallen)) {
#ifdef DEBUG
    		fprintf(stderr, "reason yp_first failed is %d\n",
    		    reason);
#endif
    		hp = NULL;
    	    }
    	else {
    		hp = interpret(val, vallen);
    		free(val);
    	}
    }
	goto found;
}

_old_sethostent(f)
	int f;
{
	if (hostf != NULL)
		rewind(hostf);
	if (current)
		free(current);
	current = NULL;
	_host_stayopen |= f;
	yellowup(1);	/* recompute whether yellow pages are up */
}

_old_endhostent()
{
	if (current) {
		free(current);
		current = NULL;
	}
	if (hostf) {
		fclose(hostf);
		hostf = NULL;
	}
	if (!usingyellow)
		if (_host_db) {
			dbm_close(_host_db);
			_host_db = (DBM *)NULL;
		}
	_host_stayopen = 0;
}

struct hostent *
_old_gethostent()
{
	struct hostent *hp;
	int reason;
	char *key, *val;
	int keylen, vallen;
	static char line1[BUFSIZ+1];

	yellowup(0);
	if (!usingyellow) {
		if (hostf == NULL && (hostf = fopen(_host_file, "r" )) == NULL)
			return (NULL);
	        if (fgets(line1, BUFSIZ, hostf) == NULL)
			return (NULL);
		return interpret(line1, strlen(line1));
	}
	if (current == NULL) {
		if (reason =  yp_first(domain, "hosts.byaddr",
		    &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n",
			    reason);
#endif
			return NULL;
		    }
	}
	else {
		if (reason = yp_next(domain, "hosts.byaddr",
		    current, currentlen, &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_next failed is %d\n",
			    reason);
#endif
			return NULL;
		}
	}
	if (current)
		free(current);
	current = key;
	currentlen = keylen;
	hp = interpret(val, vallen);
	free(val);
	return (hp);
}

static struct hostent *
interpret(val, len)
{
	static char *host_aliases[MAXALIASES];
	static char hostaddr[MAXADDRSIZE];
	static struct hostent host;
	static char line[BUFSIZ+1];
	static char *host_addrs[] = {
		hostaddr,
		NULL
	};
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	if (*p == '#')
		return (_old_gethostent());
	cp = any(p, "#\n");
	if (cp == NULL)
		return (_old_gethostent());
	*cp = '\0';
	cp = any(p, " \t");
	if (cp == NULL)
		return (_old_gethostent());
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	host.h_addr_list = host_addrs;
	*((u_long *)host.h_addr) = inet_addr(p);
	host.h_length = sizeof (u_long);
	host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	host.h_name = cp;
	q = host.h_aliases = host_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &host_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&host);
}

_old_sethostfile(file)
	char *file;
{
	_host_file = file;
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
 * The check is performed once at startup and thereafter if flag is set
 */
static
yellowup(flag)
{
	static int firsttime = 1;
	char *key, *val;
	int keylen, vallen;

	if (firsttime || flag) {
		firsttime = 0;
		if (domain[0] == 0) {
			if (getdomainname(domain, sizeof(domain)) < 0) {
				domain[0] = '\0';   /* missing system call implies that */
				usingyellow = 0;    /*   we're not usingyellow */
				return;
			}
		}
		usingyellow = !yp_bind(domain);
	}	
}


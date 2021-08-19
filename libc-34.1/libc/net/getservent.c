/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
/* 
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getservent.c	1.3 88/03/31 4.0NFSSRC; from	5.3 (Berkeley) 5/19/86"; /* and from 1.19 88/02/08 SMI Copyr 1984 Sun Micro */
#endif LIBC_SCCS and not lint

#if	NeXT
#include <stdlib.h>
#endif	NeXT
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <rpcsvc/ypclnt.h>

/*
 * Internet version.
 */
static struct servdata {
	FILE	*servf;
	char	*current;
	int	currentlen;
	int	stayopen;
#define MAXALIASES	35
	char	*serv_aliases[MAXALIASES];
	struct	servent serv;
	char	line[BUFSIZ+1];
	char	*domain;
	int	usingyellow;
} *servdata = { 0 }, *_servdata();

#if	NeXT
static void yellowup(void);
#endif	NeXT
static struct servent *interpret();
struct servent *_old_getservent();
char *inet_ntoa();
#if	NeXT
#else
char *malloc();
#endif	NeXT
static char *any();

static char SERVDB[] = "/etc/services";
static char YPMAP[] = "services.byname";

static struct servdata *
_servdata()
{
	register struct servdata *d = servdata;

	if (d == 0) {
		d = (struct servdata *)calloc(1, sizeof (struct servdata));
		servdata = d;
	}
	return (d);
}

struct servent *
_old_getservbyport(svc_port, proto)
	int svc_port;
	char *proto;
{
	register struct servdata *d = _servdata();
	register struct servent *p = NULL;
	register u_short port = svc_port;

	if (d == 0)
		return (0);
	/*
	 * first try to do very fast yp lookup.
	 */
	yellowup();
	if (d->usingyellow && (proto != 0)) {
		char portstr[12];
		char *key, *val;
		int keylen, vallen;
 
		sprintf(portstr, "%d", ntohs(port));
		keylen = strlen(portstr) + 1 + strlen(proto);
		key = malloc(keylen + 1);
		sprintf(key, "%s/%s", portstr, proto);
		if (!yp_match(d->domain, YPMAP, key, keylen,
		    &val, &vallen)) {
			p = interpret(val, vallen);
			free(val);
		}
		free(key);
		return(p);
	}
	_old_setservent(0);
	while (p = _old_getservent()) {
		if (p->s_port != port)
			continue;
		if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			break;
	}
	_old_endservent();
	return (p);
}

struct servent *
_old_getservbyname(name, proto)
	register char *name, *proto;
{
	register struct servdata *d = _servdata();
	register struct servent *p;
	register char **cp;

	if (d == 0)
		return (0);
	_old_setservent(0);
	while (p = _old_getservent()) {
		if (strcmp(name, p->s_name) == 0)
			goto gotname;
		for (cp = p->s_aliases; *cp; cp++)
			if (strcmp(name, *cp) == 0)
				goto gotname;
		continue;
gotname:
		if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			break;
	}
	_old_endservent();
	return (p);
}

_old_setservent(f)
	int f;
{
	register struct servdata *d = _servdata();

	if (d == 0)
		return;
	if (d->servf == NULL)
		d->servf = fopen(SERVDB, "r");
	else
		rewind(d->servf);
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
	yellowup();	/* recompute whether yellow pages are up */
}

_old_endservent()
{
	register struct servdata *d = _servdata();

	if (d == 0)
		return;
	if (d->current && !d->stayopen) {
		free(d->current);
		d->current = NULL;
	}
	if (d->servf && !d->stayopen) {
		fclose(d->servf);
		d->servf = NULL;
	}
}

struct servent *
_old_getservent()
{
	register struct servdata *d = _servdata();
	int reason;
	char *key = NULL, *val = NULL;
	int keylen, vallen;
	struct servent *sp;

	if (d == 0)
		return NULL;
	yellowup();
	if (!d->usingyellow) {
		if (d->servf == NULL && (d->servf = fopen(SERVDB, "r")) == NULL)
			return (NULL);
		if (fgets(d->line, BUFSIZ, d->servf) == NULL)
			return (NULL);
		return interpret(d->line, strlen(d->line));
	}
	if (d->current == NULL) {
		if (reason =  yp_first(d->domain, YPMAP,
		    &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_first failed is %d\n",
			    reason);
#endif
			return NULL;
		    }
	}
	else {
		if (reason = yp_next(d->domain, YPMAP,
		    d->current, d->currentlen, &key, &keylen, &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_next failed is %d\n",
			    reason);
#endif
			return NULL;
		}
	}
	if (d->current)
		free(d->current);
	d->current = key;
	d->currentlen = keylen;
	sp = interpret(val, vallen);
	free(val);
	return (sp);
}

static struct servent *
interpret(val, len)
{
	register struct servdata *d = _servdata();
	char *p;
	register char *cp, **q;

	if (d == 0)
		return (0);
	strncpy(d->line, val, len);
	p = d->line;
	d->line[len] = '\n';
	if (*p == '#')
		return (_old_getservent());
	cp = any(p, "#\n");
	if (cp == NULL)
		return (_old_getservent());
	*cp = '\0';
	d->serv.s_name = p;
	p = any(p, " \t");
	if (p == NULL)
		return (_old_getservent());
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	cp = any(p, ",/");
	if (cp == NULL)
		return (_old_getservent());
	*cp++ = '\0';
	d->serv.s_port = htons((u_short)atoi(p));
	d->serv.s_proto = cp;
	q = d->serv.s_aliases = d->serv_aliases;
	cp = any(cp, " \t");
	if (cp != NULL)
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &(d->serv_aliases[MAXALIASES - 1]))
			*q++ = cp;
		cp = any(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&d->serv);
}

/*
 * check to see if yellow pages are up, and store that fact in d->usingyellow.
 */
#if	NeXT
static void yellowup(void)
#else
static
yellowup()
#endif	NeXT
{
	register struct servdata *d = _servdata();

	if (d->domain == NULL) {
		d->usingyellow = usingypmap(&d->domain, YPMAP);
	}
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

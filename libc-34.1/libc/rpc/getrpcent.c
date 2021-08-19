#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = 	"@(#)getrpcent.c	1.1 88/05/10 4.0NFSSRC SMI"; /* @(#) from SUN 1.12   */
#endif

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#if	NeXT
#include <stdlib.h>
#endif	NeXT
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <rpcsvc/ypclnt.h>

/*
 * Internet version.
 */
#if	NeXT
static struct rpcdata {
#else
struct rpcdata {
#endif	NeXT
	FILE	*rpcf;
	char	*current;
	int	currentlen;
	int	stayopen;
#define	MAXALIASES	35
	char	*rpc_aliases[MAXALIASES];
	struct	rpcent rpc;
	char	line[BUFSIZ+1];
	char	*domain;
	int	usingyellow;
} *rpcdata, *_rpcdata();

static	struct rpcent *interpret();
struct	hostent *_old_gethostent();
char	*inet_ntoa();
static	char *strpbrk();
#if	NeXT
struct rpcent *_old_getrpcbynumber(int number);
struct rpcent *_old_getrpcbyname(char *name);
int _old_setrpcent(int f);
void _old_endrpcent(void);
struct rpcent *_old_getrpcent(void);
static void yellowup(void);
static struct rpcent *interpret(char * val, int len);
#endif	NeXT

static char RPCDB[] = "/etc/rpc";

static char *YPMAP = "rpc.bynumber";

static struct rpcdata *
_rpcdata()
{
	register struct rpcdata *d = rpcdata;

	if (d == 0) {
		d = (struct rpcdata *)calloc(1, sizeof (struct rpcdata));
		rpcdata = d;
	}
	return (d);
}

struct rpcent *
_old_getrpcbynumber(number)
	register int number;
{
	register struct rpcdata *d = _rpcdata();
	register struct rpcent *p;
	int reason;
	char adrstr[16], *val = NULL;
	int vallen;

	if (d == 0)
		return (0);
	_old_setrpcent(0);
	if (!d->usingyellow) {
		while (p = _old_getrpcent()) {
			if (p->r_number == number)
				break;
		}
	}
	else {
		sprintf(adrstr, "%d", number);
		if (reason = yp_match(d->domain, YPMAP,
		    adrstr, strlen(adrstr), &val, &vallen)) {
#ifdef DEBUG
			fprintf(stderr, "reason yp_match failed is %d\n",
			    reason);
#endif
			p = NULL;
		    }
		else {
			p = interpret(val, vallen);
			free(val);
		}
	}
	_old_endrpcent();
	return (p);
}

struct rpcent *
_old_getrpcbyname(name)
	char *name;
{
	struct rpcent *rpc;
	char **rp;

	_old_setrpcent(0);
	while(rpc = _old_getrpcent()) {
		if (strcmp(rpc->r_name, name) == 0)
			return (rpc);
		for (rp = rpc->r_aliases; *rp != NULL; rp++) {
			if (strcmp(*rp, name) == 0)
				return (rpc);
		}
	}
	_old_endrpcent();
	return (NULL);
}

_old_setrpcent(f)
	int f;
{
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;
	if (d->rpcf == NULL)
		d->rpcf = fopen(RPCDB, "r");
	else
		rewind(d->rpcf);
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
	yellowup();	/* recompute whether yellow pages are up */
}

void _old_endrpcent(void)
{
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;
	if (d->current && !d->stayopen) {
		free(d->current);
		d->current = NULL;
	}
	if (d->rpcf && !d->stayopen) {
		fclose(d->rpcf);
		d->rpcf = NULL;
	}
}

struct rpcent *
_old_getrpcent()
{
	struct rpcent *hp;
	int reason;
	char *key = NULL, *val = NULL;
	int keylen, vallen;
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return (0);
	yellowup();
	if (!d->usingyellow) {
		if (d->rpcf == NULL && (d->rpcf = fopen(RPCDB, "r")) == NULL)
			return (NULL);
	        if (fgets(d->line, BUFSIZ, d->rpcf) == NULL)
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
	hp = interpret(val, vallen);
	free(val);
	return (hp);
}

static struct rpcent *
interpret(char *val, int len)
{
	register struct rpcdata *d = _rpcdata();
	char *p;
	register char *cp, **q;

	if (d == 0)
		return (0);
	strncpy(d->line, val, len);
	p = d->line;
	d->line[len] = '\n';
	if (*p == '#')
		return (getrpcent());
	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		return (getrpcent());
	*cp = '\0';
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		return (getrpcent());
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	d->rpc.r_name = d->line;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	d->rpc.r_number = atoi(cp);
	q = d->rpc.r_aliases = d->rpc_aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL) 
		*cp++ = '\0';
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &(d->rpc_aliases[MAXALIASES - 1]))
			*q++ = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&d->rpc);
}

/* 
 * check to see if yellow pages are up, and store that fact in d->usingyellow.
 */
static void yellowup(void)
{
	register struct rpcdata *d = _rpcdata();

	if (d == 0)
		return;
	if (d->domain == NULL) {
		d->usingyellow = usingypmap(&d->domain, YPMAP);
	}
}

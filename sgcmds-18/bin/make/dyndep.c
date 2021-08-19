/*
 * Dynamicdep() checks each dependency by calling runtime().
 * Runtime() determines if a dependent line contains "$@"
 * or "$(@F)" or "$(@D)". If so, it makes a new dependent line
 * and insert it into the dependency chain of the input name, p.
 * Here, "$@" gets translated to p->namep. That is
 * the current name on the left of the colon in the
 * makefile.  Thus,
 *	xyz:	s.$@.c
 * translates into
 *	xyz:	s.xyz.c
 *
 * Also, "$(@F)" translates to the same thing without a prededing
 * directory path (if one exists).
 * Note, to enter "$@" on a dependency line in a makefile
 * "$$@" must be typed. This is because `make' expands
 * macros upon reading them.
 *
 * Now, also does '%' and "` ... `" style dynamic dependencies.
 */

#include "defs.h"

FSTATIC char var[11][MAXPATHLEN+1];  /* XXX */
FSTATIC int used[11];

static struct lineblock *copyline();
static struct depblock *execsub();


dynamicdep(p)
	register struct nameblock *p;
{
	register struct lineblock *lp;
	register struct nameblock *q;
	int i;

	p->rundep = 1;

	for (i = 0; i < 11; i++)
		used[i] = 0;

	for (lp = p->linep; lp != 0; lp = lp->nxtlineblock)
		if (lp->shp != 0)
			goto pass1;

	for (q = firstname; q != 0; q = q->nxtnameblock) {
		if (q->linep == 0 || index(q->namep, '%') == 0
		|| unify(p->namep, "", q->namep) == 0)
			continue;
		if (q->septype != ALLDEPS)
			fatal1("%s rule can only use a single ':'", q->namep);
		if (dbgflag) {
			printf("unify(%s, %s):\n", p->namep, q->namep);
			for (i = 0; i < 11; i++) {
				if (used[i] == 0)
					continue;
				if (i == 0)
					printf("\t%% : %s\n", var[i]);
				else
					printf("\t%%%d: %s\n", i-1, var[i]);
			}
		}
		p->septype = ALLDEPS;
		if (p->linep == 0)
			p->linep = copyline(q->linep);
		else {
			lp = p->linep;
			while (lp->nxtlineblock != 0)
				lp = lp->nxtlineblock;
			lp->nxtlineblock = copyline(q->linep);
		}
		doruntime(0, p);
		break;
	}

pass1:
	doruntime(1, p);
	doruntime(2, p);
}


doruntime(pass, p)
	int pass;
	struct nameblock *p;
{
	register struct lineblock *lp, *nlp;
	struct lineblock *backlp = 0;

	for (lp = p->linep; lp != 0; lp = lp->nxtlineblock) {
		if ((nlp = runtime(lp, pass)) != 0)
			if (backlp)
				backlp->nxtlineblock = nlp;
			else
				p->linep = nlp;
		backlp = (nlp == 0) ? lp : nlp;
	}
}


struct lineblock *
runtime(lp, pass)
	register struct lineblock *lp;
	int pass;
{
	register struct depblock *q, *nq;
	register struct shblock *s, *ns;
	register char *pc;
	struct lineblock *nlp;
	char buf[OUTMAX];  /* XXX */

	switch (pass) {
	case 0:
		for (q = lp->depp; q != 0; q = q->nxtdepblock)
			if (q->depname && index(q->depname->namep, '%'))
				break;
		if (q)
			break;
		for (s = lp->shp; s != 0; s = s->nxtshblock)
			if (index(s->shbp, '%'))
				break;
		if (s == 0)
			return 0;
		break;
	case 1:
		for (q = lp->depp; q != 0; q = q->nxtdepblock)
			if (q->depname && index(q->depname->namep, '$'))
				break;
		if (q == 0)
			return 0;
		break;
	case 2:
		for (q = lp->depp; q != 0; q = q->nxtdepblock)
			if (q->depname && amatch(q->depname->namep, "`*`"))
				break;
		if (q == 0)
			return 0;
		break;
	default:
		fatal("Unknown pass in dyndep");
	}

	nlp = ALLOC(lineblock);
	nlp->nxtlineblock = lp->nxtlineblock;

	switch (pass) {
	case 0:
		nq = nlp->depp = (lp->depp ? ALLOC(depblock) : 0);
		for (q = lp->depp; q != 0; q = q->nxtdepblock) {
			if (q->depname && index(pc = q->depname->namep, '%')) {
				instant(pc, buf);
				nq->depname = makename(buf);
			} else
				nq->depname = q->depname;
			nq = nq->nxtdepblock = (q->nxtdepblock ?
					ALLOC(depblock) : 0);
		}
		break;
	case 1:
		nq = nlp->depp = (lp->depp ? ALLOC(depblock) : 0);
		for (q = lp->depp; q != 0; q = q->nxtdepblock) {
			if (q->depname && index(pc = q->depname->namep, '$')) {
				(void) subst(pc, buf);
				nq->depname = makename(buf);
			} else
				nq->depname = q->depname;
			nq = nq->nxtdepblock = (q->nxtdepblock ?
					ALLOC(depblock) : 0);
		}
		break;
	case 2:
		nq = nlp->depp = (lp->depp ? ALLOC(depblock) : 0);
		for (q = lp->depp; q != 0; q = q->nxtdepblock) {
			if (q->depname && amatch(q->depname->namep, "`*`"))
				nq = execsub(nq, q->depname->namep);
			else
				nq->depname = q->depname;
			nq = nq->nxtdepblock = (q->nxtdepblock ?
					ALLOC(depblock) : 0);
		}
		break;
	}

	switch (pass) {
	case 0:
		ns = nlp->shp = (lp->shp ? ALLOC(shblock) : 0);
		for (s = lp->shp; s != 0; s = s->nxtshblock) {
			instant(s->shbp, buf);
			ns->shbp = copys(buf);
			ns = ns->nxtshblock = (s->nxtshblock ?
					ALLOC(shblock) : 0);
		}
		break;
	case 1:
	case 2:
		nlp->shp = lp->shp;
		break;
	}

	return nlp;
}


/*
 * find a substitution of h|p that matches s.
 */
int
unify(s, h, p)
	char *s;
	char *h;
	char *p;
{
	char *q, *r;
	int vn;

	if (*h != 0)
		return *s == *h && unify(s + 1, h + 1, p);
	if (*p == 0)
		return *s == 0;
	if (*p != '%')
		return *s == *p && unify(s + 1, h, p + 1);
	if (used[vn = isdigit(*++p) ? *p++ - '0' + 1 : 0])
		return unify(s, var[vn], p);
	for (q = s, r = var[vn]; *r = *q++; r++)
		;
	for (used[vn] = 1; q > s; *--r = 0)
		if (unify(--q, h, p))
			return 1;
	used[vn] = 0;
	return 0;
}


/*
 * copy string a into b, substituting for % variables
 */
instant(a, b)
	register char *a, *b;
{
	register char *c;
	register int vn;

	if (a == 0) {
		*b = 0;
		return;
	}
	while (*a) {
		if (*a != '%') {
			if (*a == '\\' && *(a+1) == '%')
				a++;
			*b++ = *a++;
			continue;
		}
		if (used[vn = isdigit(*++a) ? *a++ - '0' + 1 : 0] == 0)
			fatal("Reference to uninstantiated variable");
		for (c = var[vn]; *c; *b++ = *c++)
			;
	}
	*b = 0;
	return;
}


static struct depblock *
execsub(q, comm)
	register struct depblock *q;
	char *comm;
{
	register char *p;
	register FILE *f;
	register int c;
	int ign, nopr;
	char cmd[OUTMAX];
	char temp[OUTMAX];
	char buf[BUFSIZ];
	extern FILE *pfopen();

	q->depname = 0;
	if (*(p = comm++) != '`')
		fatal("execsub: doesn't start with `");
	while (*++p)
		;
	if (p == comm || *--p != '`')
		fatal("execsub: doesn't end with `");
	*p = 0;
	(void) subst(comm, temp);
	*p = '`';
#ifdef notdef
	/*
	 * Attempt to be smart about finding command
	 * arguments.  Probably not worth it.
	 */
	for (p = temp; *p; p++) {
		if (isspace(*p))
			continue;
		for (comm = p; *p && !isspace(*p); p++) {
			if (*p != '\'' && *p != '"' && *p != '`')
				continue;
			c = *p; 
			while (*++p && *p != c)
					;
		}
		c = *p;
		*p = 0;
		if (metas(comm) == 0 && *comm != '-')
			(void) rcsco(comm);
		if ((*p = c) == 0)
			break;
	}
#endif
	fixname(temp, cmd);
	ign = ignerr;
	nopr = NO;
	for (p = cmd; *p == '-' || *p == '@'; ++p)
		if (*p == '-')
			ign = YES;
		else
			nopr = YES;
	if (*p == 0)
		return q;
	if (!silflag && (!nopr || noexflag))
		printf("%s%s\n", (noexflag ? "" : prompt), p);
	if (noexflag)
		return q;
	if ((f = pfopen(p, ign)) == NULL)
		return q;
	setbuf(f, buf);

	for (;;) {
		do
			c = getc(f);
		while (c != EOF && isspace(c));
		if (c == EOF)
			break;
		p = temp;
		for (*p++ = c; (c = getc(f)) != EOF && !isspace(c); *p++ = c)
			;
		*p = 0;
		if (q->depname)
			q = q->nxtdepblock = ALLOC(depblock);
		q->depname = makename(temp);
		if (c == EOF)
			break;
	}

	(void) pfclose(f, ign);
	q->nxtdepblock = 0;
	return q;
}


static struct shblock *
copysh(sp)
	struct shblock *sp;
{
	register struct shblock *nsp;

	if (sp == 0)
		return 0;
	nsp = ALLOC(shblock);
	nsp->shbp = copys(sp->shbp);
	nsp->nxtshblock = copysh(sp->nxtshblock);
	return nsp;
}


static struct depblock *
copydep(dp)
	struct depblock *dp;
{
	register struct depblock *ndp;

	if (dp == 0)
		return 0;
	ndp = ALLOC(depblock);
	ndp->depname = dp->depname;
	ndp->nxtdepblock = copydep(dp->nxtdepblock);
	return ndp;
}


static struct lineblock *
copyline(lp)
	struct lineblock *lp;
{
	register struct lineblock *nlp;

	if (lp == 0)
		return 0;
	nlp = ALLOC(lineblock);
	nlp->depp = copydep(lp->depp);
	nlp->shp = copysh(lp->shp);
	nlp->nxtlineblock = copyline(lp->nxtlineblock);
	return nlp;
}

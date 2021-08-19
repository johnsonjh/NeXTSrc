#include "defs.h"


/*
 * BASIC PROCEDURE.  RECURSIVE.
 *
 * p->done = 0   don't know what to do yet
 * p->done = 1   file in process of being updated
 * p->done = 2   file already exists in current state
 * p->done = 3   file make failed
 */
doname(p, reclevel, tval, ochain)
	register struct nameblock *p;
	int reclevel;
	time_t *tval;
	struct chain **ochain;
{
	int errstat;
	int okdel1;
	int didwork;
	time_t td, td1, tdep, ptime, ptime1, prestime();
	register struct depblock *q;
	struct depblock *qtemp, *srchdir(), *suffp, *suffp1;
	struct nameblock *p1, *p2;
	struct shblock *implcom, *explcom;
	register struct lineblock *lp;
	struct lineblock *lp1, *lp2;
	char sourcename[MAXPATHLEN+1], prefix[MAXPATHLEN+1], temp[MAXPATHLEN+1];
	char concsuff[20];
	char *pnamep, *p1namep, *cp;
	char *mkqlist();
	struct chain *achain, *qchain, *appendq();
	int found, onetime;
	extern char *ctime(), *strcpy();
	struct chain *cochain;
	char *setimpl;			/* set implicit vars */
#ifdef NeXT_MOD
	char *targetname;
#endif NeXT_MOD
	static char abuf[QBUFMAX];

	if (p == 0) {
		*tval = 0;
		return 0;
	}

#ifdef NeXT_MOD
	if (dbgflag) {
		if (p->archp)
			printf("doname(%s(%s), %d)\n", p->archp, p->namep,
			    reclevel);
		else
			printf("doname(%s, %d)\n", p->namep, reclevel);
	}
#else NeXT_MOD
	if (dbgflag)
		printf("doname(%s, %d)\n", p->namep ,reclevel);
#endif NeXT_MOD

	if (p->done) {
		/*
		 * if we want to check-out RCS files, and we have previously
		 * determined that we can, then append it to the previous 
		 * level's cochain.
		 */
		if (p->RCSnamep)
			*ochain = appendq(*ochain, p->namep);
		*tval = p->modtime;
		return p->done == 3;
	}

	cochain = 0;
	errstat = 0;
	tdep = 0;
	implcom = 0;
	explcom = 0;
	ptime = exists(p, ochain); 
	ptime1 = 0;
	didwork = NO;
	p->done = 1;	/* avoid infinite loops */
#ifdef NeXT_MOD
	targetname = p->archp ? p->archp : p->namep;
#endif NeXT_MOD
	achain = 0;
	qchain = 0;

	/*
	 * Perform runtime dependency translations.
	 */
	if (p->rundep == 0) {
#ifdef	NeXT_MOD
		setvar("@", targetname);
#else	NeXT_MOD
		setvar("@", p->namep);
#endif	NeXT_MOD
		dynamicdep(p);
		setvar("@", (char *) 0);
	}

	/*
	 * Expand any names that have embedded metacharaters. Must be
	 * done after dynamic dependencies because the dyndep symbols
	 * ($(*D)) may contain shell meta characters.
	 */
	for (lp = p->linep; lp; lp = lp->nxtlineblock)
		for (q = lp->depp; q; q = qtemp) {
			qtemp = q->nxtdepblock;
			expand(q);
		}

	/*
	 * make sure all dependents are up to date
	 */
	for (lp = p->linep; lp; lp = lp->nxtlineblock) {
		td = 0;
		for (q = lp->depp; q; q = q->nxtdepblock) {
			if (q->depname == 0)
				continue;
			errstat += doname(q->depname, reclevel+1, &td1, &cochain);
			if (dbgflag)
				printf("TIME(%s)=%s", q->depname->namep, ctime(&td1)+4);
			if (td1 > td)
				td = td1;
			achain = appendq(achain, q->depname->namep);
			if (ptime < td1)
				qchain = appendq(qchain, q->depname->namep);
		}
		if (p->septype != SOMEDEPS) {
			if (lp->shp != 0) {
				if (explcom)
					fprintf(stderr, "Too many command lines for `%s'\n", p->namep);
				else
					explcom = lp->shp;
			}
			if (td > tdep)
				tdep = td;
			continue;
		}
		if (lp->shp != 0 && (ptime < td || (ptime == 0 && td == 0) || lp->depp == 0)) {
			okdel1 = okdel;
			okdel = NO;
			if (!questflag) {
				if (cochain)
					co(cochain);
#ifdef	NeXT_MOD
				setvar("@", targetname);
				if (p->archp)
					setvar("%", p->namep);
#else	NeXT_MOD
				setvar("@", p->namep);
#endif	NeXT_MOD
				setvar(">", strcpy(abuf, mkqlist(achain)));
				setvar("?", mkqlist(qchain));
				errstat += docom(lp->shp);
				setvar("@", (char *) 0);
			}
			achain = 0;
			qchain = 0;
			cochain = 0;
			okdel = okdel1;
			ptime1 = prestime();
			didwork = YES;
		}
	}

	found = 0; 
	onetime = 0;

	/*
	 * Look for implicit dependents, using suffix rules
	 */
	setimpl = 0;
	for (lp = sufflist; lp; lp = lp->nxtlineblock)
	    for (suffp = lp->depp; suffp; suffp = suffp->nxtdepblock) {
		if (suffp->depname == 0)
			continue;
		pnamep = suffp->depname->namep;
		if (!suffix(p->namep, pnamep, prefix))
			continue;
#ifdef NeXT_MOD
		if (p->archp)
			pnamep = ".a";
#endif NeXT_MOD
		found = 1;
searchdir:
		(void) srchdir(concat(prefix, "*", temp), NO, (struct depblock *) 0);
		if (coflag)
			srchRCS(temp);
		srchmachine(temp);
		for (lp1 = sufflist; lp1; lp1 = lp1->nxtlineblock)
		    for (suffp1 = lp1->depp; suffp1; suffp1 = suffp1->nxtdepblock) {
			if (suffp1->depname == 0)
				continue;
			/*
			 * only use a single suffix if it really has rules
			 */
			if (onetime == 1) {
				struct lineblock *lbp = suffp1->depname->linep;
				for (; lbp; lbp = lbp->nxtlineblock)
					if (lbp->depp || lbp->shp)
						break;
				if (lbp == 0)
					continue;
			}
			p1namep = suffp1->depname->namep;
			p1 = srchname(concat(p1namep, pnamep, concsuff));
			if (p1 == 0)
				continue;
			p2 = srchname(concat(prefix, p1namep, sourcename));
			if (p2 == 0)
				continue;
			errstat += doname(p2, reclevel+1, &td, &cochain);
			if (dbgflag)
				printf("TIME(%s)=%s", p2->namep, ctime(&td)+4);
			if (td > tdep)
				tdep = td;
			achain = appendq(achain, p2->namep);
			if (ptime < td)
				qchain = appendq(qchain, p2->namep);
			setimpl = copys((p2->alias ? p2->alias : p2->namep));
			for (lp2 = p1->linep; lp2; lp2 = lp2->nxtlineblock)
				if (implcom = lp2->shp)
					break;
			goto endloop;
		}
		if (onetime == 1)
			/*
			 * quit search for single suffix rule
			 */
			goto endloop;
		cp = rindex(prefix, '/');
		if (cp++ == 0)
			cp = prefix;
		setvar("*", cp);
	}
endloop:

	/*
	 * look for a single suffix type rule.
	 * only possible if no explicit dependents and no shell rules
	 * are found, and nothing has been done so far. (previously, `make'
	 * would exit with 'Don't know how to make ...' message.
	 */
	if (found == 0 && onetime == 0 && (p->linep == 0
	|| (p->linep->depp == 0 && p->linep->shp == 0))) {
		onetime = 1;
		if (dbgflag)
			printf("Looking for Single suffix rule.\n");
		(void) concat(p->namep, "", prefix);
		pnamep = "";
		goto searchdir;
	}

	if (dbgflag && cochain)
		printf("CO(%s): %s\n", p->namep, mkqlist(cochain));
	if (p->RCSnamep && (explcom || implcom)) {
		if (!keepgoing)
			fatal1("%s has both an RCS file and rules", p->namep);
		errstat++;
		printf("%s has both an RCS file and rules\n", p->namep);
	}

	if (errstat == 0 && (ptime < tdep || (ptime == 0 && tdep == 0))) {
		ptime = (tdep > 0 ? tdep : prestime());
		if (cochain)
			co(cochain);
		if (setimpl) {
			setvar("*", prefix);
			setvar("<", setimpl);
		}
#ifdef	NeXT_MOD
		setvar("@", targetname);
		if (p->archp)
			setvar("%", p->namep);
#else	NeXT_MOD
		setvar("@", p->namep);
#endif	NeXT_MOD
		setvar(">", strcpy(abuf, mkqlist(achain)));
		setvar("?", mkqlist(qchain));
		if (explcom)
			errstat += docom(explcom);
		else if (implcom)
			errstat += docom(implcom);
		else if (p->septype == 0) {
			if (p1 = srchname(".DEFAULT")) {
				setvar("<", p->alias ? p->alias : p->namep);
				for (lp2 = p1->linep; lp2; lp2 = lp2->nxtlineblock)
					if (implcom = lp2->shp) {
						errstat += docom(implcom);
						break;
					}
			} else if (keepgoing) {
				printf("Don't know how to make %s\n", p->namep);
				++errstat;
			} else
				fatal1(" Don't know how to make %s", p->namep);
		}
		setvar("@", (char *) 0);
		if (explcom || implcom)
			if(noexflag || (ptime = exists(p, (struct chain **) 0)) == 0)
				ptime = prestime();
	}
	
	else if (errstat != 0 && reclevel == 0)
		printf("`%s' not remade because of errors\n", p->namep);

	else if (!questflag && reclevel == 0 && didwork == NO)
		printf("`%s' is up to date.\n", p->namep);

	if (questflag && reclevel == 0)
		quit(ndocoms > 0 ? -1 : 0);
	p->done = (errstat ? 3 : 2);
	if (ptime1 > ptime)
		ptime = ptime1;
	p->modtime = ptime;
	*tval = ptime;
	return errstat;
}


docom(q)
	struct shblock *q;
{
#ifdef NeXT_MOD
	char *s;
	int exok;
#else
	char *s, *exok;
#endif NeXT_MOD
	struct varblock *varptr();
	int ign, nopr;
	char string[OUTMAX];
	char string2[OUTMAX];
	char *sindex();

	++ndocoms;
	if (questflag)
		return 0;

	if (touchflag) {
		s = varptr("@")->varval;
		if (!silflag)
			printf("touch(%s)\n", s);
		if (!noexflag)
			touch(YES, s);
		return 0;
	}
	for (; q; q = q->nxtshblock) {
#ifdef NeXT_MOD
		exok = (sindex(q->shbp, "$(MAKE)") != NULL)
		    || (sindex(q->shbp, "${MAKE}") != NULL);
#else
		exok = sindex(q->shbp, "$(MAKE)");
#endif NeXT_MOD
		(void) subst(q->shbp, string2);
		fixname(string2, string);
		ign = ignerr;
		nopr = NO;
		for (s = string; *s == '-' || *s == '@'; ++s)
			if (*s == '-')
				ign = YES;
			else
				nopr = YES;
		if (docom1(s, ign, nopr, exok) && !ign)
			return 1;
	}
	return 0;
}


docom1(comstring, nohalt, noprint, exok)
	register char *comstring;
	int nohalt, noprint;
#ifdef NeXT_MOD
	int exok;
#else
	char *exok;
#endif NeXT_MOD
{
	if (comstring[0] == '\0')
		return 0;

	if (!silflag && (!noprint || noexflag))
		printf("%s%s\n", (noexflag ? "" : prompt), comstring);

	if (noexflag && exok == 0)
		return 0;

	return dosys(comstring, nohalt);
}


/*
 * If there are any Shell meta characters in the name,
 * expand into a list, after searching directory
 */
expand(q)
	register struct depblock *q;
{
	register char *s;
	char *s1;
	struct depblock *p, *srchdir();

	if (q->depname == 0)
		return;
	s1 = s = q->depname->namep;
	for (;;) {
		switch (*s++) {
		case '\0':
			return;
		case '*':
		case '?':
		case '[':
			if (p = srchdir(s1 , YES, q->nxtdepblock)) {
				q->nxtdepblock = p;
				q->depname = 0;
			}
			return;
		}
	}
}

#include "defs.h"

FSTATIC struct nameblock *hashtab[HASHSIZE];
FSTATIC int lasthash	= 0;
FSTATIC int nhashed	= 0;
FSTATIC int inenvir	= 0;

extern char *dftrans();
extern char *straightrans();

/*
 * simple linear hash.
 */
hashloc(s)
	char *s;
{
	register int i;
	register int hashval;
	register char *t;

	hashval = 0;

	for (t = s; *t != '\0'; ++t)
		hashval += (*t << 8) | *(t + 1);

	lasthash = hashval %= HASHSIZE;

	for (i = hashval;
		hashtab[i] != 0 && (hashval != hashtab[i]->hashval
				|| unequal(s, hashtab[i]->namep));
			i = (i+1) % HASHSIZE)
		;

	return i;
}


struct nameblock *
srchname(s)
	char *s;
{
	return hashtab[hashloc(s)];
}


struct nameblock *
makename(s)
	char *s;
{
	/*
	 * make a fresh copy of the string s
	 */
	register int i;
	register struct nameblock *p;
	extern char *copys();

	if (hashtab[i = hashloc(s)])
		return hashtab[i];

	if (nhashed++ > HASHSIZE-8)
		fatal("Hash table overflow");

	p = ALLOC(nameblock);
	p->nxtnameblock = firstname;
	p->namep = copys(s);
	p->alias = 0;
	p->linep = 0;
	p->done = 0;
	p->septype = 0;
	p->rundep = 0;
	p->RCSnamep = 0;
	p->hashval = lasthash;
	p->modtime = 0;

	firstname = p;
	if (mainname == 0)
		if (s[0] != '.' || hasslash(s))
			mainname = p;

	hashtab[i] = p;

	return p;
}


hasslash(s)
	register char *s;
{
	for (; *s; ++s)
		if (*s == '/')
			return YES;
	return NO;
}


char *
copys(s)
	register char *s;
{
	char *calloc();
	register char *t, *t0;

	if ((t = t0 = calloc((unsigned) strlen(s) + 1, sizeof(char))) == 0)
		fatal("out of memory");
	while (*t++ = *s++)
		;
	return t0;
}


/*
 * c = concatenation of a and b
 */
char *
concat(a, b, c)
	register char *a, *b;
	char *c;
{
	register char *t;

	t = c;
	while (*t = *a++)
		t++;
	while (*t++ = *b++)
		;
	return c;
}


/*
 * is b the suffix of a?  if so, set p = prefix
 */
suffix(a, b, p)
	register char *a, *b, *p;
{
	char *a0,*b0;

	a0 = a;
	b0 = b;

	while (*a++)
		;
	while (*b++)
		;

	if ((a-a0) < (b-b0))
		return 0;

	while (b > b0)
		if (*--a != *--b)
			return 0;

	while (a0 < a)
		*p++ = *a0++;
	*p = '\0';

	return 1;
}


char *
ckalloc(n)
	register int n;
{
	char *calloc();
	register char *p;

	if (p = calloc(1, (unsigned) n))
		return p;
	fatal("out of memory");
#ifdef lint
	return ckalloc(n);  /* use and return */
#endif
}

#ifdef NeXT_MOD
char recursestring[] = "Infinitely recursive macro? Check for substitutions \
like\n\tLIBS = -lNeXT_s -lsys_s $(LIBS)";
#endif NeXT_MOD
/*
 * copy string a into b, substituting for arguments
 */
char *
subst(a, b)
	register char *a, *b;
{
	static depth = 0;
	register char *s;
	char vbuf[QBUFMAX], vname[QBUFMAX];
	register char closer;
	extern char *cshtrans();
	extern char *colontrans();

	if (a == 0) {
		*b = 0;
		return b;
	}
#ifdef NeXT_MOD
	if (++depth > 25)  {
		(void) fflush(stdout);
		fprintf(stderr, "Make: %s\nStop.\n", recursestring);
		quit(1);
	}
#else
	if (++depth > 100)
		fatal("infinitely recursive macro?");
#endif NeXT_MOD
	while (*a) {
		if (*a != '$')
			*b++ = *a++;
		else if (*++a == '\0' || *a == '$')
			*b++ = *a++;
		else {
			s = vbuf;
			if (*a=='(' || *a=='{') {
				closer = (*a=='(' ? ')' : '}');
				++a;
				while (*a == ' ')
					++a;
				while (*a != ' ' && *a != closer && *a != '\0')
					*s++ = *a++;
				while (*a != closer && *a != '\0')
					++a;
				if (*a == closer)
					++a;
			} else
				*s++ = *a++;
			*s = '\0';
			(void) subst(vbuf, vname);
			if (amatch(vname, "[@*<%][DF]"))
				b = dftrans(b, vname);
			else if (amatch(vname, "*:[ehrtEHRT]"))
				b = cshtrans(b, vname);
			else if (amatch(vname, "*:*=*"))
				b = colontrans(b, vname);
			else
				b = straightrans(b, vname);
		}
	}
	*b = '\0';
	--depth;
	return b;
}


/*
 * copy s into t, return the location of the next free character in s
 */
char *
copstr(s, t)
	register char *s, *t;
{
	if (t == 0)
		return s;
	while (*t)
		*s++= *t++;
	*s = 0;
	return s;
}


struct varblock *
srchvar(v)
	register char *v;
{
	register struct varblock *vp;
	register int n;

	vp = firstvar;
	while (vp) {
		if ((n = strcmp(v, vp->varname)) == 0)
			return vp;
		vp = (n < 0) ? vp->lftvarblock : vp->rgtvarblock;
	}
	return 0;
}


/*
 * Translate the $(name:*=*) type things.
 */
char *
colontrans(b, vname)
	register char *b;
	char *vname;
{
	register int i;
	register char *ps1;
	int fromlen;
	char from[128], to[128];  /* XXX */
	char ttemp[QBUFMAX];
	char tmp;
	char *psave, *pcolon;

	/*
	 * Mark off the name (up to colon), the from expression
	 * (up to '='), and the to expresion (up to null).
	 */
	for (ps1 = vname; *ps1 != ':'; ps1++)
		;
	pcolon = ps1;
	*pcolon = 0;
	i = 0;
	while (*++ps1 != '=')
		from[i++] = *ps1;
	from[i] = 0;
	fromlen = i;
	i = 0;
	while (*++ps1)
		to[i++] = *ps1;
	to[i] = 0;

	/*
	 * Now, translate.
	 */
	*(ps1 = ttemp) = 0;
	if (amatch(vname, "[@*<%][DF]"))
		(void) dftrans(ps1, vname);
	else
		(void) straightrans(ps1, vname);
	while (*ps1 == ' ' || *ps1 == '\t')
		*b++ = *ps1++;
	while (*ps1) {
		psave = ps1;
		while (*ps1 && *ps1 != ' ' && *ps1 != '\t')
			ps1++;
		if (fromlen <= ps1-psave
		&& strncmp(ps1-fromlen, from, fromlen) == 0) {
			tmp = *(ps1 - fromlen);
			*(ps1 - fromlen) = 0;
			b = copstr(b, psave);
			*(ps1 - fromlen) = tmp;
			b = copstr(b, to);
		} else {
			tmp = *ps1;
			*ps1 = 0;
			b = copstr(b, psave);
			*ps1 = tmp;
		}
		while (*ps1 == ' ' || *ps1 == '\t')
			*b++ = *ps1++;
	}
	*pcolon = ':';
	return b;
}


/*
 * Do csh-style $(name:[ehrt]) translations.
 */
char *
cshtrans(b, vname)
	register char *b;
	char *vname;
{
	register char *p1, *p2;
	char c, sep, *pcolon, *psave;
	char tmp, ttemp[QBUFMAX];

	for (p1 = vname; *p1 != ':'; p1++)
		;
	*(pcolon = p1) = 0;
	c = *++p1;
	if (isupper(c))
		c = tolower(c);
	if (c != 'h' && c != 't') {
		sep = '.';
		c = (c == 'r') ? 'h' : 't';
	} else
		sep = '/';

	*(p1 = ttemp) = 0;
	if (amatch(vname, "[@*<%][DF]"))
		(void) dftrans(p1, vname);
	else
		(void) straightrans(p1, vname);

	while (*p1 == ' ' || *p1 == '\t')
		*b++ = *p1++;
	while (*p1) {
		psave = p1;
		p2 = 0;
		while (*p1 && *p1 != ' ' && *p1 != '\t') {
			if (*p1 == sep)
				p2 = p1;
			p1++;
		}
		tmp = *p1;
		*p1 = 0;
		if (p2 == 0) {
			if (c != 't' || sep != '.')
				b = copstr(b, psave);
		} else {
			if (c == 'h') {
				*p2 = 0;
				b = copstr(b, psave);
				*p2 = sep;
			} else
				b = copstr(b, p2 + 1);
		}
		*p1 = tmp;
		while (*p1 == ' ' || *p1 == '\t')
			*b++ = *p1++;
	}

	*pcolon = ':';
	return b;
}


/*
 * Do the $(@D) type translations.
 */
char *
dftrans(b, vname)
	register char *b;
	char *vname;
{
	char c, c1;
	char *p1, *p2;
	struct varblock *vbp;
#ifdef	NeXT_MOD
	char nb[QBUFMAX];
#endif	NeXT_MOD

	c1 = vname[1];
	vname[1] = 0;
	vbp = srchvar(vname);
	if (vbp != 0 && vbp->varval != 0) {
#ifdef	NeXT_MOD
		subst(vbp->varval, nb);
		for (p1 = p2 = nb; *p1; p1++)
			if (*p1 == '/')
				p2 = p1;
#else	NeXT_MOD
		for (p1 = p2 = vbp->varval; *p1; p1++)
			if (*p1 == '/')
				p2 = p1;
#endif	NeXT_MOD
		if (*p2 == '/') {
			if (c1 == 'D') {
				if (p2 == vbp->varval)
					p2++;
				c = *p2;
				*p2 = 0;
				b = copstr(b,vbp->varval);
				*p2 = c;
			} else
				b = copstr(b, p2+1);
		} else {
			if (c1 == 'D')
				b = copstr(b, ".");
			else
				b = copstr(b, p2);
		}
	}
	vname[1] = c1;
	return b;
}


/*
 * Standard translation, nothing fancy.
 */
char *
straightrans(b, vname)
	register char *b;
	char *vname;
{
	register struct varblock *vbp;

	vbp = srchvar(vname);
	if (vbp != 0 && vbp->varval != 0) {
		b = subst(vbp->varval, b);
		vbp->used = YES;
	}
	return b;
}


setvar(v, s)
	char *v, *s;
{
	register struct varblock *p;
	extern struct varblock *varptr();

	p = varptr(v);
	if (p->noreset == 0) {
		p->varval = s;
		p->noreset = inarglist;
		if (p->used && unequal(v, "@") && unequal(v, "*")
#ifdef NeXT_MOD
		&& unequal(v, "%")
#endif NeXT_MOD
		&& unequal(v, "<") && unequal(v, "?") && unequal(v, ">"))
			fprintf(stderr, "Warning: %s changed after being used\n", v);
	}
}


/*
 * look for arguments with equal signs but not colons
 */
eqsign(a)
	char *a;
{
	register char *s, *t;
	register char c;

	while (*a == ' ')
		++a;
	for (s = a; *s != '\0' && *s != ':'; ++s)
		if (*s == '=') {
			for (t = a; *t != '=' && *t != ' ' && *t != '\t'; ++t )
				;
			c = *t;
			*t = '\0';
			for (++s; *s == ' ' || *s == '\t'; ++s)
				;
			if (inarglist != 0 && inenvir == 0)
				setenv(a, s, 1);
			setvar(a, copys(s));
			*t = c;
			return YES;
		}

	return NO;
}


struct varblock *
varptr(v)
	register char *v;
{
	register struct varblock *vp, *vlp;
	register int n;

	vlp = 0;
	vp = firstvar;
	while (vp) {
		if ((n = strcmp(v, vp->varname)) == 0)
			return vp;
		vlp = vp;
		vp = (n < 0) ? vp->lftvarblock : vp->rgtvarblock;
	}

	vp = ALLOC(varblock);
	vp->lftvarblock = 0;
	vp->rgtvarblock = 0;
	vp->varname = copys(v);
	vp->varval = 0;

	if (vlp == 0)
		firstvar = vp;
	else if (n < 0)
		vlp->lftvarblock = vp;
	else
		vlp->rgtvarblock = vp;

	return vp;
}


fatal1(s, t)
	char *s;
	char *t;
{
	char buf[BUFSIZ];

	(void) sprintf(buf, s, t);
	fatal(buf);
}


fatal(s)
	char *s;
{
	(void) fflush(stdout);
	if (s)
		fprintf(stderr, "Make: %s.  Stop.\n", s);
	else
		fprintf(stderr, "\nStop.\n");
#ifdef unix
	quit(1);
#endif
#ifdef gcos
	quit(0);
#endif
}


yyerror(s)
	char *s;
{
	char buf[256];
	extern int yylineno;

	(void) sprintf(buf, "%%s, line %d: %s", yylineno, s);
	fatal1(buf, curfname);
}


struct chain *
appendq(head, tail)
	struct chain *head;
	char *tail;
{
	register struct chain *p, *q;

	p = ALLOC(chain);
	p->datap = tail;
	p->nextp = 0;

	if (head) {
		for (q = head; q->nextp; q = q->nextp)
			;
		q->nextp = p;
		return head;
	}
	return p;
}


char *
mkqlist(p)
	struct chain *p;
{
	register char *qbufp, *s;
	static char qbuf[QBUFMAX];

	if (p == 0) {
		*qbuf = 0;
		return qbuf;
	}
	for (qbufp = qbuf; p; p = p->nextp) {
		s = p->datap;
		if (qbufp+strlen(s) > &qbuf[QBUFMAX-3]) {
			fprintf(stderr, "$? list too long\n");
			break;
		}
		while (*s)
			*qbufp++ = *s++;
		*qbufp++ = ' ';
	}
	*--qbufp = '\0';
	return qbuf;
}


/*
 * Called in main
 * If a string like "CC=" occurs then CC is not put in environment.
 * This is because there is no good way to remove a variable
 * from the environment within the shell.
 */
readenv()
{
	register char **ea, *p;
	extern char **environ;

	inenvir	= 1;
	for (ea = environ; *ea; ea++) {
		for (p = *ea; *p && *p != '='; p++)
			;
		if (*p == '=' && *++p != 0)
			(void) eqsign(*ea);
	}
	inenvir	= 0;
}


/*
 * add the .EXPORT dependencies to the environment
 */
export(p)
	struct nameblock *p;
{
	register struct lineblock *lp;
	register struct depblock *q;
	register char *v;

	for (lp = p->linep; lp; lp = lp->nxtlineblock)
		for (q = lp->depp; q; q = q->nxtdepblock)
			if (v = varptr(q->depname->namep)->varval)
				setenv(q->depname->namep, v, 1);
}


/*
 * hook for catching make as it exits
 */
onexit()
{
	struct nameblock *p;
	register struct lineblock *lp;
	register struct shblock *sp;
	static int once = 0;

	if (once)
		return;
	once = 1;
	if ((p = srchname(".EXIT")) == 0)
		return;
	sp = 0;
	for (lp = p->linep; lp; lp = lp->nxtlineblock)
		if (sp = lp->shp)
			break;
	if (sp == 0)
		return;
	(void) docom(sp);	
}


quit(val)
	int val;
{
	onexit();
	touchflag = 0;		/* don't try to touch anything */
	rm();			/* clean up co'ed files */
	exit(val);
}


/*
 * ncat copies n chars of string b at a, returning a pointer to the
 * null terminating a.  This way, it can be used to daisy-chain.
 * If n < 0, all chars of b will be copied
 */
char *
ncat(a, b, n)
	register char *a, *b;
	register int n;
{
	if (n < 0) {
		while (*a++ = *b++)
			;
		return --a;
	} 
	while (n--)
		if ((*a++ = *b++) == '\0')
			return --a;
	*a = '\0';
	return a;
}


char *
sindex(big, small)
	char *big, *small;
{
	register char *bp, *bp1, *sp;
	register char c = *small++;

	if (c == 0)
		return 0;
	for (bp = big; *bp; bp++)
		if (*bp == c) {
			for (sp = small, bp1 = bp+1; *sp && *sp == *bp1++; sp++)
				;
			if (*sp == 0)
				return bp;
		}
	return 0;
}

setenv(n, v, p)
	char	*n;
	char	*v;
	int	p;
{
	putenv(n, v);
}

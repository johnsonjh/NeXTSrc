/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)regex.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint
/*
 * routines to do regular expression matching
 *
 * Entry points:
 *
 *	recmp(pattern,target)
 *		char *pattern, *target;
 *	 ... like strcmp(), but returns 0 if the regular expression
 *		'pattern' is found in 'target', 1 if not.
 *		The compiled pattern is cached for efficiency
 *		during iterative calls.
 *
 *	struct regex *
 *	re_compile(s)
 *		returns a pointer to a regular expression buffer
 *		for the given string.
 *
 *	re_comp(s)
 *		char *s;
 *	 ... returns 0 if the string s was compiled successfully,
 *		     a pointer to an error message otherwise.
 *	     If passed 0 or a null string returns without changing
 *           the currently compiled re (see note 11 below).
 *
 *	re_exec(s)
 *		char *s;
 *	 ... returns 1 if the string s matches the last compiled regular
 *		       expression, 
 *		     0 if the string s failed to match the last compiled
 *		       regular expression, and
 *		    -1 if the compiled regular expression was invalid 
 *		       (indicating an internal error).
 *
 *	re_match(s,r)
 *		char *s;
 *		struct regex *r;
 *	 ... same as 're_exec()', passing the expression 'r'.
 *
 * The strings passed to both re_comp and re_exec may have trailing or
 * embedded newline characters; they are terminated by nulls.
 *
 * The identity of the author of these routines is lost in antiquity;
 * this is essentially the same as the re code in the original V6 ed.
 *
 * The regular expressions recognized are described below. This description
 * is essentially the same as that for ed.
 *
 *	A regular expression specifies a set of strings of characters.
 *	A member of this set of strings is said to be matched by
 *	the regular expression.  In the following specification for
 *	regular expressions the word `character' means any character but NUL.
 *
 *	1.  Any character except a special character matches itself.
 *	    Special characters are the regular expression delimiter plus
 *	    \ [ . and sometimes ^ * $.
 *	2.  A . matches any character.
 *	3.  A \ followed by any character except a digit or ( )
 *	    matches that character.
 *	4.  A nonempty string s bracketed [s] (or [^s]) matches any
 *	    character in (or not in) s. In s, \ has no special meaning,
 *	    and ] may only appear as the first letter. A substring 
 *	    a-b, with a and b in ascending ASCII order, stands for
 *	    the inclusive range of ASCII characters.
 *	5.  A regular expression of form 1-4 followed by * matches a
 *	    sequence of 0 or more matches of the regular expression.
 *	6.  A regular expression, x, of form 1-8, bracketed \(x\)
 *	    matches what x matches.
 *	7.  A \ followed by a digit n matches a copy of the string that the
 *	    bracketed regular expression beginning with the nth \( matched.
 *	8.  A regular expression of form 1-8, x, followed by a regular
 *	    expression of form 1-7, y matches a match for x followed by
 *	    a match for y, with the x match being as long as possible
 *	    while still permitting a y match.
 *	9.  A regular expression of form 1-8 preceded by ^ (or followed
 *	    by $), is constrained to matches that begin at the left
 *	    (or end at the right) end of a line.
 *	10. A regular expression of form 1-9 picks out the longest among
 *	    the leftmost matches in a line.
 *	11. An empty regular expression stands for a copy of the last
 *	    regular expression encountered.
 */

/*
 * constants for re's
 */
#define	CBRA	1
#define	CCHR	2
#define	CDOT	4
#define	CCL	6
#define	NCCL	8
#define	CDOL	10
#define	CEOF	11
#define	CKET	12
#define	CBACK	18

#define	CSTAR	01

#if	NeXT
#include <stdlib.h>
#endif	NeXT
#include <regex.h>

static struct regex R;
static	int advance();

static struct regex *
re_alloc(){
	return (struct regex *)calloc(1, sizeof(struct regex));

}

#include <ctype.h>

re_fold(a,b)
	char *a, *b;
/*
 * Copy 'b' into 'a', expanding characters to match both upper and lower case.
 *  e.g., 'foo' -> '[fF][oO][oO]'
 */
{
	while (*b)
		if (isalpha(*b)){
			char c;
			c = isupper(*b)? tolower(*b) : *b;
			*a++ = '[';
			*a++ = c;
			*a++ = toupper(c);
			*a++ = ']';
			b++;
		}	
		else *a++ = *b++;
	*a = '\0';
}

/*
 * compile the regular expression argument into a dfa
 */
char *
re_comp(sp)
	register char	*sp;
{
	register int	c;
	register char	*ep = R.expbuf;
	int	cclcnt, numbra = 0;
	char	*lastep = 0;
	char	bracket[NBRA];
	char	*bracketp = &bracket[0];
	static	char	*retoolong = "Regular expression too long";

#define	comerr(msg) {R.expbuf[0] = 0; numbra = 0; return(msg); }

	if (sp == 0 || *sp == '\0') {
		if (*ep == 0)
			return("No previous regular expression");
		return(0);
	}
	if (*sp == '^') {
		R.circf = 1;
		sp++;
	}
	else
		R.circf = 0;
	for (;;) {
		if (ep >= &(R.expbuf[ESIZE]))
			comerr(retoolong);
		if ((c = *sp++) == '\0') {
			if (bracketp != bracket)
				comerr("unmatched \\(");
			*ep++ = CEOF;
			*ep++ = 0;
			return(0);
		}
		if (c != '*')
			lastep = ep;
		switch (c) {

		case '.':
			*ep++ = CDOT;
			continue;

		case '*':
			if (lastep == 0 || *lastep == CBRA || *lastep == CKET)
				goto defchar;
			*lastep |= CSTAR;
			continue;

		case '$':
			if (*sp != '\0')
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			*ep++ = CCL;
			*ep++ = 0;
			cclcnt = 1;
			if ((c = *sp++) == '^') {
				c = *sp++;
				ep[-2] = NCCL;
			}
			do {
				if (c == '\0')
					comerr("missing ]");
				if (c == '-' && ep [-1] != 0) {
					if ((c = *sp++) == ']') {
						*ep++ = '-';
						cclcnt++;
						break;
					}
					while (ep[-1] < c) {
						*ep = ep[-1] + 1;
						ep++;
						cclcnt++;
						if (ep >= &(R.expbuf[ESIZE]))
							comerr(retoolong);
					}
				}
				*ep++ = c;
				cclcnt++;
				if (ep >= &(R.expbuf[ESIZE]))
					comerr(retoolong);
			} while ((c = *sp++) != ']');
			lastep[1] = cclcnt;
			continue;

		case '\\':
			if ((c = *sp++) == '(') {
				if (numbra >= NBRA)
					comerr("too many \\(\\) pairs");
				*bracketp++ = numbra;
				*ep++ = CBRA;
				*ep++ = numbra++;
				continue;
			}
			if (c == ')') {
				if (bracketp <= bracket)
					comerr("unmatched \\)");
				*ep++ = CKET;
				*ep++ = *--bracketp;
				continue;
			}
			if (c >= '1' && c < ('1' + NBRA)) {
				*ep++ = CBACK;
				*ep++ = c - '1';
				continue;
			}
			*ep++ = CCHR;
			*ep++ = c;
			continue;

		defchar:
		default:
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
}

struct regex *
re_compile(s,fold)
	char *s;
	int fold;
/*
 * Compile and return a buffer for the regular expression 's'.
 * If 'fold' is true mixed-case will match.
 */
{
	char t[1024];
	char *p;
	struct regex *r;
	if (fold) re_fold(t,s), p = re_comp(t);
	else p = re_comp(s);
	if (p) return r;
	r = re_alloc();
	if (!r) return r;
	R.start = R.end = (char *)0;
	bcopy(&R,r,sizeof(struct regex));
	return r;
}

/* 
 * match the argument string against the compiled re
 */
int
re_match(p1,r)
	register char	*p1;
	struct regex *r;
{
	register char	*p2 = r->expbuf;
	register int	c;
	int	rv;

	for (c = 0; c < NBRA; c++) {
		r->braslist[c] = 0;
		r->braelist[c] = 0;
	}
	r->start = p1;
	if (r->circf)
		return((advance(p1, p2, r)));
	/*
	 * fast check for first character
	 */
	if (*p2 == CCHR) {
		c = p2[1];
		do {
			if (*p1 != c)
				continue;
			r->start = p1;
			if (rv = advance(p1, p2, r))
				return(rv);
		} while (*p1++);
		return(0);
	}
	/*
	 * regular algorithm
	 */
	do {
		r->start = p1;
		if (rv = advance(p1, p2, r))
			return(rv);
	}
	while (*p1++);
	return(0);
}

re_exec(s)
	char *s;
{
	return re_match(s,&R);
}

/* 
 * try to match the next thing in the dfa
 */
static	int
advance(lp, ep, r)
	register char	*lp, *ep;
	struct regex *r;
{
	register char	*curlp;
	int	ct, i;
	int	rv;
#define Return(n) {r->end = lp; return n;}

	for (;;)
		switch (*ep++) {
		case CCHR: if (*ep++ == *lp++) continue; Return(0);
		case CDOT: if (*lp++) continue; Return(0);
		case CDOL: if (*lp == '\0') continue; Return(0);
		case CEOF: Return(1);
		case CCL:
			if (cclass(ep, *lp++, 1)) {
				ep += *ep;
				continue;
			}
			Return(0);

		case NCCL:
			if (cclass(ep, *lp++, 0)) {
				ep += *ep;
				continue;
			}
			Return(0);

		case CBRA:
			r->braslist[*ep++] = lp;
			continue;

		case CKET:
			r->braelist[*ep++] = lp;
			continue;

		case CBACK:
			if (r->braelist[i = *ep++] == 0)
				return(-1);
			if (backref(i, lp, r)) {
				lp += r->braelist[i] - r->braslist[i];
				continue;
			}
			Return(0);

		case CBACK|CSTAR:
			if (r->braelist[i = *ep++] == 0)
				return(-1);
			curlp = lp;
			ct = r->braelist[i] - r->braslist[i];
			while (backref(i, lp, r))
				lp += ct;
			while (lp >= curlp) {
				if (rv = advance(lp, ep, r))
					return(rv);
				lp -= ct;
			}
			continue;

		case CDOT|CSTAR:
			curlp = lp;
			while (*lp++)
				;
			goto star;

		case CCHR|CSTAR:
			curlp = lp;
			while (*lp++ == *ep)
				;
			ep++;
			goto star;

		case CCL|CSTAR:
		case NCCL|CSTAR:
			curlp = lp;
			while (cclass(ep, *lp++, ep[-1] == (CCL|CSTAR)))
				;
			ep += *ep;
			goto star;

		star:
			do {
				lp--;
				if (rv = advance(lp, ep, r))
					return(rv);
			} while (lp > curlp);
			Return(0);

		default:
			Return(-1);
		}
}

backref(i, lp, r)
	register int	i;
	register char	*lp;
	struct regex *r;
{
	register char	*bp;

	bp = r->braslist[i];
	while (*bp++ == *lp++)
		if (bp >= r->braelist[i])
			return(1);
	return(0);
}

int
cclass(set, c, af)
	register char	*set, c;
	int	af;
{
	register int	n;

	if (c == 0)
		return(0);
	n = *set++;
	while (--n)
		if (*set++ == c)
			return(af);
	return(! af);
}

recmp(pattern,target)
	char *pattern, *target;
/*
 * like 'strcmp()', but takes a regular expression 'pattern'
 * and matches it against 'target', returning 0 if there is a match,
 * 1 if not, and -1 if there was an error in compilation.
 */
{
	static char prev[256], *prevp;
	if (prevp != pattern || strcmp(pattern,prev)){
		prevp = pattern;
		strcpy(prev,pattern);
		if (re_comp(pattern))
			return -1;
	}
	return re_exec(target)==1? 0 : 1;
}

/*
main(){
	char s[120], t[120];
	struct regex *r;
	gets(s);
	r = re_compile(s,0);
	while (gets(s))
		printf("%d\n", re_match(s,r));
}
*/

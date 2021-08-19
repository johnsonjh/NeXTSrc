/*	@(#)test.c	1.8	*/
/*
 *      test expression
 *      [ expression ]
 */

#include	"defs.h"
#include <sys/types.h>
#include <sys/stat.h>

int	ap, ac;
char	**av;

test(argn, com)
char	*com[];
int	argn;
{
	ac = argn;
	av = com;
	ap = 1;
	if (eq(com[0],lbstr))			/* DAG -- made strings sharable */
	{
		if (!eq(com[--ac], rbstr))	/* DAG */
			failed(btest, rbmiss);	/* DAG */
	}
	com[ac] = 0;
	if (ac <= 1)
		return(1);
	return(exp() ? 0 : 1);
}

char *
nxtarg(mt)
{
	if (ap >= ac)
	{
		if (mt)
		{
			ap++;
			return(0);
		}
		failed(btest, argexp);		/* DAG */
	}
	return(av[ap++]);
}

exp()
{
	int	p1;
	char	*p2;

	p1 = e1();
	p2 = nxtarg(1);
	if (p2 != 0)
	{
		if (eq(p2, dasho))			/* DAG */
			return(p1 | exp());

		if (eq(p2, rbstr) && !eq(p2, rpstr))	/* DAG */
			failed(btest, synmsg);		/* DAG */
	}
	ap--;
	return(p1);
}

e1()
{
	int	p1;
	char	*p2;

	p1 = e2();
	p2 = nxtarg(1);

	if ((p2 != 0) && eq(p2, dasha))		/* DAG */
		return(p1 & e1());
	ap--;
	return(p1);
}

e2()
{
	if (eq(nxtarg(0), bang))		/* DAG */
		return(!e3());
	ap--;
	return(e3());
}

e3()
{
	int	p1;
	register char	*a;
	char	*p2;
	long	atol();
	long	int1, int2;

	a = nxtarg(0);
	if (eq(a, lpstr))			/* DAG */
	{
		p1 = exp();
		if (!eq(nxtarg(0), rpstr))	/* DAG */
			failed(btest, parexp);	/* DAG */
		return(p1);
	}
	p2 = nxtarg(1);
	ap--;
	if ((p2 == 0) || (!eq(p2, eqlstr) && !eq(p2, neqstr)))	/* DAG */
	{
		if (eq(a, dashr))				/* DAG */
			return(tio(nxtarg(0), 4));
		if (eq(a, dashw))				/* DAG */
			return(tio(nxtarg(0), 2));
		if (eq(a, dashx))				/* DAG */
			return(tio(nxtarg(0), 1));
		if (eq(a, dashd))				/* DAG */
			return(filtyp(nxtarg(0), S_IFDIR));
		if (eq(a, dashc))				/* DAG */
			return(filtyp(nxtarg(0), S_IFCHR));
		if (eq(a, dashb))				/* DAG */
			return(filtyp(nxtarg(0), S_IFBLK));
		if (eq(a, dashf))				/* DAG */
			return(filtyp(nxtarg(0), S_IFREG));
		if (eq(a, dashu))				/* DAG */
			return(ftype(nxtarg(0), S_ISUID));
		if (eq(a, dashg))				/* DAG */
			return(ftype(nxtarg(0), S_ISGID));
#ifdef NeXT_MOD
		if (eq(a, dashh))				/* MIKE */
			return(filtyp(nxtarg(0), S_IFLNK));
#endif NeXT_MOD
		if (eq(a, dashk))				/* DAG */
			return(ftype(nxtarg(0), S_ISVTX));
		if (eq(a, dashp))				/* DAG */
#if defined(BERKELEY) && !defined(pyr) && !defined(sun) && !defined(NeXT)
#define S_IFIFO	S_IFSOCK	/* fifo - map to socket on 4.2BSD */
#endif
			return(filtyp(nxtarg(0),S_IFIFO));
   		if (eq(a, dashs))				/* DAG */
			return(fsizep(nxtarg(0)));
		if (eq(a, dasht))				/* DAG */
		{
			if (ap >= ac)		/* no args */
				return(isatty(1));
			else if (eq((a = nxtarg(0)), dasha) || eq(a, dasho))	/* DAG */
			{
				ap--;
				return(isatty(1));
			}
			else
				return(isatty(atoi(a)));
		}
		if (eq(a, dashn))				/* DAG */
			return(!eq(nxtarg(0), nullstr));	/* DAG */
		if (eq(a, dashz))				/* DAG */
			return(eq(nxtarg(0), nullstr));		/* DAG */
	}

	p2 = nxtarg(1);
	if (p2 == 0)
		return(!eq(a, nullstr));	/* DAG */
	if (eq(p2, dasha) || eq(p2, dasho))	/* DAG */
	{
		ap--;
		return(!eq(a, nullstr));	/* DAG */
	}
	if (eq(p2, eqlstr))			/* DAG */
		return(eq(nxtarg(0), a));
	if (eq(p2, neqstr))			/* DAG */
		return(!eq(nxtarg(0), a));
	int1 = atol(a);
	int2 = atol(nxtarg(0));
	if (eq(p2, dasheq))			/* DAG */
		return(int1 == int2);
	if (eq(p2, dashne))			/* DAG */
		return(int1 != int2);
	if (eq(p2, dashgt))			/* DAG */
		return(int1 > int2);
	if (eq(p2, dashlt))			/* DAG */
		return(int1 < int2);
	if (eq(p2, dashge))			/* DAG */
		return(int1 >= int2);
	if (eq(p2, dashle))			/* DAG */
		return(int1 <= int2);

	bfailed(btest, badop, p2);
/* NOTREACHED */
#ifdef gould
	return 0;	/* DAG -- added */
#endif
}

tio(a, f)
char	*a;
int	f;
{
	if (access(a, f) == 0)
		return(1);
	else
		return(0);
}

ftype(f, field)
char	*f;
int	field;
{
	struct stat statb;

	if (stat(f, &statb) < 0)
		return(0);
	if ((statb.st_mode & field) == field)
		return(1);
	return(0);
}

filtyp(f,field)
char	*f;
int field;
{
	struct stat statb;

#ifdef NeXT_MOD
	if (field == S_IFLNK) {
		if (lstat(f, &statb) < 0)
			return(0);
	} else {
		if (stat(f, &statb) < 0)
			return(0);
	}
#else !NeXT_MOD
	if (stat(f, &statb) < 0)
		return(0);
#endif !NeXT_MOD
	if ((statb.st_mode & S_IFMT) == field)
		return(1);
	else
		return(0);
}



fsizep(f)
char	*f;
{
	struct stat statb;

	if (stat(f, &statb) < 0)
		return(0);
	return(statb.st_size > 0);
}

/*
 * fake diagnostics to continue to look like original
 * test(1) diagnostics
 */
bfailed(s1, s2, s3) 
char	*s1;
char	*s2;
char	*s3;
{
	prp();
	prs(s1);
	if (s2)
	{
		prs(colon);
		prs(s2);
		prs(s3);
	}
	newline();
	exitsh(ERROR);
}

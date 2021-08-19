/*	@(#)args.c	1.4	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"

static struct dolnod *copyargs();
static void freedolh();	/* DAG -- bug fix (no value returned) */
extern struct dolnod *freeargs();
static struct dolnod *dolh;

#if BRL
#if JOBS
char	flagadr[17];
#else
char	flagadr[16];
#endif
#else	/* !BRL */
#if JOBS
char	flagadr[15];
#else
char	flagadr[14];
#endif
#endif

char	flagchar[] =
{
	'x',
	'n', 
	'v', 
	't', 
	STDFLG, 
	'i', 
	'e', 
	'r', 
	'k', 
	'u', 
	'h',
	'f',
	'a',
#if BRL
	'I',
	'E',
#endif
#if JOBS
	'J',
#endif
	 0
};

long	flagval[]  =
{
	execpr,	
	noexec,	
	readpr,	
	oneflg,	
	stdflg,	
	intflg,	
	errflg,	
	rshflg,	
	keyflg,	
	setflg,	
	hashflg,
	nofngflg,
	exportflg,
#if BRL
	infoflg,
	noeotflg,
#endif
#if JOBS
	jobflg,
#endif
	  0
};

/* ========	option handling	======== */


options(argc,argv)
	char	**argv;
	int	argc;
{
	register char *cp;
	register char **argp = argv;
	register char *flagc;
	char	*flagp;

	if (argc > 1 && *argp[1] == '-')
	{
		/*
		 * if first argument is "--" then options are not
		 * to be changed. Fix for problems getting 
		 * $1 starting with a "-"
		 */

		cp = argp[1];
		if (cp[1] == '-')
		{
			argp[1] = argp[0];
			argc--;
			return(argc);
		}
		if (cp[1] == '\0')
			flags &= ~(execpr|readpr);

		/*
		 * Step along 'flagchar[]' looking for matches.
		 * 'sicr' are not legal with 'set' command.
		 */

		while (*++cp)
		{
			flagc = flagchar;
			while (*flagc && *flagc != *cp)
				flagc++;
			if (*cp == *flagc)
			{
				if (eq(argv[0], setstr) && any(*cp, sicrstr))	/* DAG -- made strings sharable */
					failed(argv[1], badopt);
				else
				{
#if JOBS
					if (*cp == 'J')
						j_init();
#endif
					flags |= flagval[flagc-flagchar];
					if (flags & errflg)
						Eflag = eflag = errflg;
				}
			}
			else if (*cp == 'c' && argc > 2 && comdiv == 0)
			{
				comdiv = argp[2];
				argp[1] = argp[0];
				argp++;
				argc--;
			}
			else
				failed(argv[1],badopt);
		}
		argp[1] = argp[0];
		argc--;
	}
	else if (argc > 1 && *argp[1] == '+')	/* unset flags x, k, t, n, v, e, u */
						/* or any added BRL/JOBS flags */
	{
		cp = argp[1];
		while (*++cp)
		{
			flagc = flagchar;
			while (*flagc && *flagc != *cp)
				flagc++;
			/*
			 * step through flags
			 */
			if (!any(*cp, sicrstr) && *cp == *flagc)	/* DAG -- made string sharable */
			{
				/*
				 * only turn off if already on
				 */
				if ((flags & flagval[flagc-flagchar]))
				{
#if JOBS
					if (*cp != 'J' || !j_finish(FALSE))
#endif
					    flags &= ~(flagval[flagc-flagchar]);
					if (*cp == 'e')
						Eflag = eflag = 0;
				}
			}
		}
		argp[1] = argp[0];
		argc--;
	}
	/*
	 * set up $-
	 */
	flagp = flagadr;
	if (flags)
	{
		flagc = flagchar;
		while (*flagc)
		{
			if (flags & flagval[flagc-flagchar])
				*flagp++ = *flagc;
			flagc++;
		}
	}
	*flagp = 0;
	return(argc);
}

/*
 * sets up positional parameters
 */
setargs(argi)
	char	*argi[];
{
	register char **argp = argi;	/* count args */
	register int argn = 0;

	while (Rcheat(*argp++) != ENDARGS)
		argn++;
	/*
	 * free old ones unless on for loop chain
	 */
	freedolh();
	dolh = copyargs(argi, argn);
	dolc = argn - 1;
}


static void	/* DAG -- bug fix (no value returned) */
freedolh()
{
	register char **argp;
	register struct dolnod *argblk;

	if (argblk = dolh)
	{
		if ((--argblk->doluse) == 0)
		{
			for (argp = argblk->dolarg; Rcheat(*argp) != ENDARGS; argp++)
				free(*argp);
			free(argblk);
		}
	}
}

struct dolnod *
freeargs(blk)
	struct dolnod *blk;
{
	register char **argp;
	register struct dolnod *argr = 0;
	register struct dolnod *argblk;
	int cnt;

	if (argblk = blk)
	{
		argr = argblk->dolnxt;
		cnt  = --argblk->doluse;

		if (argblk == dolh)
		{
			if (cnt == 1)
				return(argr);
			else
				return(argblk);
		}
		else
		{			
			if (cnt == 0)
			{
				for (argp = argblk->dolarg; Rcheat(*argp) != ENDARGS; argp++)
					free(*argp);
				free(argblk);
			}
		}
	}
	return(argr);
}

static struct dolnod *
copyargs(from, n)
	char	*from[];
{
	register struct dolnod *np = (struct dolnod *)alloc(sizeof(char**) * n + 3 * BYTESPERWORD);
	register char **fp = from;
	register char **pp;

	np->doluse = 1;	/* use count */
	pp = np->dolarg;
	dolv = pp;
	
	while (n--)
		*pp++ = make(*fp++);
	*pp++ = ENDARGS;
	return(np);
}


struct dolnod *
clean_args(blk)
	struct dolnod *blk;
{
	register char **argp;
	register struct dolnod *argr = 0;
	register struct dolnod *argblk;

	if (argblk = blk)
	{
		argr = argblk->dolnxt;

		if (argblk == dolh)
			argblk->doluse = 1;
		else
		{
			for (argp = argblk->dolarg; Rcheat(*argp) != ENDARGS; argp++)
				free(*argp);
			free(argblk);
		}
	}
	return(argr);
}

clearup()
{
	/*
	 * force `for' $* lists to go away
	 */
	while (argfor = clean_args(argfor))
		;
	/*
	 * clean up io files
	 */
	while (pop())
		;

	/*
	 * clean up tmp files
	*/
	while (poptemp())
		;
}

struct dolnod *
useargs()
{
	if (dolh)
	{
		if (dolh->doluse++ == 1)
		{
			dolh->dolnxt = argfor;
			argfor = dolh;
		}
	}
	return(dolh);
}

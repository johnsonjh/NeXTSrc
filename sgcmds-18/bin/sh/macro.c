/*	@(#)macro.c	1.5	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"

static char	quote;	/* used locally */
static char	quoted;	/* used locally */

static getch(char endch);
static comsubst(void);
static flush(int ot);


static void	/* DAG -- bug fix (no value returned) */
copyto(register char endch)
{
	register char	c;

	while ((c = getch(endch)) != endch && c)
		pushstak(c | quote);
	zerostak();
	if (c != endch)
		error(badsub);
}

static
skipto(register char endch)
{
	/*
	 * skip chars up to }
	 */
	register char	c;

	while ((c = readc()) && c != endch)
	{
		switch (c)
		{
		case SQUOTE:
			skipto(SQUOTE);
			break;

		case DQUOTE:
			skipto(DQUOTE);
			break;

		case DOLLAR:
			if (readc() == BRACE)
				skipto('}');
		}
	}
	if (c != endch)
		error(badsub);
}

static
getch(char endch)
{
	register char	d;

retry:
	d = readc();
	if (!subchar(d))
		return(d);
#if JOBS
	if (d == PERCENT && (flags&jobflg))
	{
		register int	c;

		peekc = (c = readc()) | MARK;
		if (digchar(c) || c == PERCENT)
		{
			register char	*v;

			if (v = j_macro())	/* %number or %% handled */
				while (c = *v++)
					pushstak(c | quote);
			/* else expands to nothingness */
			goto retry;
		}
	}
	else
#endif
	    if (d == DOLLAR)
	    {
		register int	c;

		if ((c = readc(), dolchar(c)))
		{
			struct namnod *n = (struct namnod *)NIL;
			int		dolg = 0;
			BOOL		bra;
			BOOL		nulflg;
			register char	*argp, *v;
			char		idb[2];
			char		*id = idb;

			if (bra = (c == BRACE))
				c = readc();
			if (letter(c))
			{
				argp = (char *)relstak();
				while (alphanum(c))
				{
					pushstak(c);
					c = readc();
				}
				zerostak();
				n = lookup(absstak(argp));
				setstak(argp);
				if (n->namflg & N_FUNCTN)
					error(badsub);
				v = n->namval;
				id = n->namid;
				peekc = c | MARK;
			}
			else if (digchar(c))
			{
				*id = c;
				idb[1] = 0;
				if (astchar(c))
				{
					dolg = 1;
					c = '1';
				}
				c -= '0';
				v = ((c == 0) ? cmdadr : (c <= dolc) ? dolv[c] : (char *)(dolg = 0));
			}
			else if (c == '$')
				v = pidadr;
			else if (c == '!')
				v = pcsadr;
			else if (c == '#')
			{
				itos(dolc);
				v = numbuf;
			}
			else if (c == '?')
			{
				itos(retval);
				v = numbuf;
			}
			else if (c == '-')
				v = flagadr;
			else if (bra)
				error(badsub);
			else
				goto retry;
			c = readc();
			if (c == ':' && bra)	/* null and unset fix */
			{
				nulflg = 1;
				c = readc();
			}
			else
				nulflg = 0;
			if (!defchar(c) && bra)
				error(badsub);
			argp = 0;
			if (bra)
			{
				if (c != '}')
				{
					argp = (char *)relstak();
					if ((v == 0 || (nulflg && *v == 0)) ^ (setchar(c)))
						copyto('}');
					else
						skipto('}');
					argp = absstak(argp);
				}
			}
			else
			{
				peekc = c | MARK;
				c = 0;
			}
			if (v && (!nulflg || *v))
			{
				char tmp = (*id == '*' ? SP | quote : SP);

				if (c != '+')
				{
					for (;;)
					{
						if (*v == 0 && quote)
							pushstak(QUOTE);
						else
						{
							while (c = *v++)
								pushstak(c | quote);
						}

						if (dolg == 0 || (++dolg > dolc))
							break;
						else
						{
							v = dolv[dolg];
							pushstak(tmp);
						}
					}
				}
			}
			else if (argp)
			{
				if (c == '?')
					failed(id, *argp ? argp : badparam);
				else if (c == '=')
				{
					if (n)
					{
						trim(argp);
						assign(n, argp);
					}
					else
						error(badsub);
				}
			}
			else if (flags & setflg)
				failed(id, unset);
			goto retry;
		}
		else
			peekc = c | MARK;
	    }
	else if (d == endch)
		return(d);
	else if (d == SQUOTE)
	{
		comsubst();
		goto retry;
	}
	else if (d == DQUOTE)
	{
		quoted++;
		quote ^= QUOTE;
		goto retry;
	}
	return(d);
}

char *
macro(as)
char	*as;
{
	/*
	 * Strip "" and do $ substitution
	 * Leaves result on top of stack
	 */
	register BOOL	savqu = quoted;
	register char	savq = quote;
	struct filehdr	fb;

	push(&fb);
	estabf(as);
	usestak();
	quote = 0;
	quoted = 0;
	copyto(0);
	pop();
	if (quoted && (stakbot == staktop))
		pushstak(QUOTE);
/*
 * above is the fix for *'.c' bug
 */
	quote = savq;
	quoted = savqu;
	return(fixstak());
}

static
comsubst(void)
{
	/*
	 * command substn
	 */
	struct fileblk	cb;
	register char	d;
	register char *savptr = fixstak();

	usestak();
	while ((d = readc()) != SQUOTE && d)
		pushstak(d);
	{
		register char	*argc;

		trim(argc = fixstak());
		push(&cb);
		estabf(argc);
	}
#if JOBS
	set_wfence();
#endif
	{
		register struct trenod *t = makefork(FPOU, cmd(EOFSYM, MTFLG | NLFLG));
		int		pv[2];

		/*
		 * this is done like this so that the pipe
		 * is open only when needed
		 */
		chkpipe(pv);
		initf(pv[INPIPE]);
		execute(t, 0, (int)(flags & errflg), 0, pv);
		close(pv[OTPIPE]);
	}
	tdystak(savptr);
	staktop = movstr(savptr, stakbot);
	while (d = readc()) {
		if (staktop >= brkend) {
			setbrk(brkincr);
		}
		pushstak(d | quote);
	}
	await(0, 0);
	while (stakbot != staktop)
	{
		if ((*--staktop & STRIP) != NL)
		{
			++staktop;
			break;
		}
	}
	pop();
}

#define CPYSIZ	512

subst(in, ot)
int	in, ot;
{
	register char	c;
	struct fileblk	fb;
	register int	count = CPYSIZ;

	push(&fb);
	initf(in);
	/*
	 * DQUOTE used to stop it from quoting
	 */
	while (c = (getch(DQUOTE) & STRIP))
	{
		pushstak(c);
		if (--count == 0)
		{
			flush(ot);
			count = CPYSIZ;
		}
	}
	flush(ot);
	pop();
}

static
flush(int ot)
{
	write(ot, stakbot, staktop - stakbot);
	if (flags & execpr)
		write(output, stakbot, staktop - stakbot);
	staktop = stakbot;
}

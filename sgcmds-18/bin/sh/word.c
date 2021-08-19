/*	@(#)word.c	1.4	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"

static readb(void);

/* ========	character handling for command lines	========*/


word()
{
	register char	c, d;
	struct argnod	*arg = (struct argnod *)locstak();
	register char	*argp = arg->argval;
	int		alpha = 1;
#ifdef TILDE_SUB
	char		*save;
#endif

	wdnum = 0;
	wdset = 0;

#if BRL || JOBS
	catcheof = TRUE;
#endif
	while (1)
	{
		while (c = nextc(0), space(c))		/* skipc() */
			;

		if (c == COMCHAR)
		{
			while ((c = readc()) != NL && c != EOF);
			peekc = c;
		}
		else
		{
			break;	/* out of comment - white space loop */
		}
	}
#ifdef TILDE_SUB
	save = argp;	/* save start of word */
#endif
	if (!eofmeta(c))
	{
		do
		{
			if (c == LITERAL)
			{
				*argp++ = (DQUOTE);
				while ((c = readc()) && c != LITERAL)
				{
					*argp++ = (c | QUOTE);
					if (c == NL)
						chkpr();
				}
				*argp++ = (DQUOTE);
			}
			else
			{
				*argp++ = (c);
				if (c == '=')
					wdset |= alpha;
				if (!alphanum(c))
					alpha = 0;
				if (qotchar(c))
				{
					d = c;
					while ((*argp++ = (c = nextc(d))) && c != d)
					{
						if (c == NL)
							chkpr();
					}
				}
#ifdef TILDE_SUB
				/*
				 * check for ~login_name
				 */
				else if ( c == SQUIGGLE && validtilde( save, argp ) )
					{
					char	*home, *name;

					name = argp;
					while ( (c = nextc( 0 )) != '/'
					     && !eofmeta( c )
					      )
						*name++ = c;
					peekc = c;
					*name = '\0';
					home = homedir( argp );
					if ( *home )
						movstr( home, --argp );
					argp += length( argp ) - 1;
					}
#endif

			}
		} while ((c = nextc(0), !eofmeta(c)));
		argp = endstak(argp);
		if (!letter(arg->argval[0]))
			wdset = 0;

		peekn = c | MARK;
		if (arg->argval[1] == 0 && 
		    (d = arg->argval[0], digit(d)) && 
		    (c == '>' || c == '<'))
		{
			word();
			wdnum = d - '0';
		}
		else		/*check for reserved words*/
		{
			if (reserv == FALSE || (wdval = syslook(arg->argval,reserved, no_reserved)) == 0)
			{
				wdarg = arg;
				wdval = 0;
			}
		}
	}
	else if (dipchar(c))
	{
		if ((d = nextc(0)) == c)
		{
			wdval = c | SYMREP;
			if (c == '<')
			{
				if ((d = nextc(0)) == '-')
					wdnum |= IOSTRIP;
				else
					peekn = d | MARK;
			}
		}
		else
		{
			peekn = d | MARK;
			wdval = c;
		}
	}
	else
	{
		if ((wdval = c) == EOF)
			wdval = EOFSYM;
		if (iopend && eolchar(c))
		{
			copy(iopend);
			iopend = 0;
		}
	}
#if BRL || JOBS
	catcheof = FALSE;
#endif
	reserv = FALSE;
	return(wdval);
}

skipc()
{
	register char c;

	while (c = nextc(0), space(c))
		;
	return(c);
}

nextc(quote)
char	quote;
{
	register char	c, d;

retry:
	if ((d = readc()) == ESCAPE)
	{
		if ((c = readc()) == NL)
		{
			chkpr();
			goto retry;
		}
		else if (quote && c != quote && !escchar(c))
			peekc = c | MARK;
		else
			d = c | QUOTE;
	}
	return(d);
}

readc()
{
#if BRL || JOBS
	static int	eofcount = 0;	/* to break endless catcheof loop */
#endif
	register char	c;
	register int	len;
	register struct fileblk *f;

	if (peekn)
	{
		peekc = peekn;
		peekn = 0;
	}
	if (peekc)
	{
		c = peekc;
		peekc = 0;
		return(c);
	}
	f = standin;
retry:
	if (f && f->fnxt != f->fend)
	{
		if ((c = *f->fnxt++) == 0)
		{
			if (f->feval)
			{
				if (estabf(*f->feval++))
					c = EOF;
				else
					c = SP;
			}
			else
				goto retry;	/* = c = readc(); */
		}
		if (flags & readpr && standin->fstak == 0)
			prc(c);
		if (c == NL)
			f->flin++;
	}
	else if (f->feof || f->fdes < 0)
	{
		c = EOF;
		f->feof++;
	}
	else if ((len = readb()) <= 0)
	{
#if BRL || JOBS
		if (catcheof
#if JOBS
		 && (flags&(ttyflg|prompt|dotflg)) == (ttyflg|prompt)
		 && ((flags&noeotflg) || j_finish(FALSE))
#else	/* BRL && !JOBS */
		 && (flags&(ttyflg|prompt|noeotflg|dotflg)) == (ttyflg|prompt|noeotflg)
#endif
		 && ++eofcount < 10)	/* in case terminal is disconnected */
		{
#if JOBS
			if ((flags&(ttyflg|prompt|noeotflg))
			 == (ttyflg|prompt|noeotflg))
#endif
			    prs(noeot);
#if JOBS
			/* else "there are stopped jobs" was printed */
#endif
			c = NL;
		}
		else
		{
#endif	/* BRL || JOBS */
		    close(f->fdes);
		    f->fdes = -1;
		    c = EOF;
		    f->feof++;
#if JOBS
		    j_finish(TRUE);
#endif
#if BRL || JOBS
		}
#endif
	}
	else
	{
		f->fend = (f->fnxt = f->fbuf) + len;
#if BRL || JOBS
		eofcount = 0;
#endif
		goto retry;
	}
	return(c);
}

static
readb(void)
{
	register struct fileblk *f = standin;
	register int	len;

	do
	{
		if (trapnote & SIGSET)
		{
			newline();
			sigchk();
		}
		else if ((trapnote & TRAPSET) && (rwait > 0))
		{
			newline();
			chktrap();
			clearup();
		}
	} while ((len = read(f->fdes, f->fbuf, f->fsiz)) < 0 && trapnote);
	return(len);
}

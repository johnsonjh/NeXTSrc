/*	@(#)error.c	1.4	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


/* ========	error handling	======== */

failed(s1, s2)
char	*s1, *s2;
{
	prp();
	prs_cntl(s1);
	if (s2)
	{
		prs(colon);
		prs(s2);
	}
	newline();
	exitsh(ERROR);
}

error(s)
char	*s;
{
	failed(s, NIL);
}

exitsh(xno)
int	xno;
{
	/*
	 * Arrive here from `FATAL' errors
	 *  a) exit command,
	 *  b) default trap,
	 *  c) fault with no trap set.
	 *
	 * Action is to return to command level or exit.
	 */
	exitval = xno;
	flags |= eflag;
	if ((flags & (forked | errflg | ttyflg)) != ttyflg)
		done();
	else
	{
		clearup();
		restore(0);
		clear_buff();
		execbrk = breakcnt = funcnt = 0;
		longjmp(errshell, 1);
	}
}

void	/* DAG */
done()
{
	register char	*t;

	if (t = trapcom[0])
	{
		trapcom[0] = 0;
#if BRL && pdp11
		if (!loginsh)		/* no exit trap on login shell */
#endif
		    execexp(t, 0);
		free(t);
	}
	else
		chktrap();

	rmtemp(0);
	rmfunctmp();

#ifdef ACCT
	doacct();
#endif

#ifdef JOBS
	if (flags & jobflg)	/* Never leave process groups in funny state */
		j_really_reset_pg();
#endif

#if BRL && pdp11
	if (loginsh && (flags&ttyflg) && (!(flags&forked) || (flags&intflg)))
	{
		extern char	*simple();

		signal(SIGHUP, SIG_IGN);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGKILL, SIG_IGN);
		/* perhaps readc() closed fd0 on EOF; quota can't cope: */
		dup(2);
		execl(quota, simple(quota), quomsg, argv0, (char *)0);
		prs(quofail);
		exit(99);
	}
#endif
	exit(exitval);
}

rmtemp(base)
struct ionod	*base;
{
	while (iotemp > base)
	{
		unlink(iotemp->ioname);
		free(iotemp->iolink);
		iotemp = iotemp->iolst;
	}
}

rmfunctmp()
{
	while (fiotemp)
	{
		unlink(fiotemp->ioname);
		fiotemp = fiotemp->iolst;
	}
}

/* DAG -- added for changes in xec.c; not as drastic as failed(): */
failure(s1, s2)
char	*s1, *s2;
{
	prp();
	prs_cntl(s1);
	if (s2)
	{
		prs(colon);
		prs(s2);
	}
	newline();

	if (flags & errflg)
		exitsh(ERROR);

	flags |= eflag;
	exitval = ERROR;
	exitset();
}


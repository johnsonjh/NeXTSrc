/*	@(#)fault.c	1.5	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"

extern void	done();	/* DAG */

char	*trapcom[MAXTRAP];
BOOL	trapflg[MAXTRAP] =
{
	0,
	0,	/* hangup */
	0,	/* interrupt */
	0,	/* quit */
	0,	/* illegal instr */
	0,	/* trace trap */
	0,	/* IOT */
	0,	/* EMT */
	0,	/* float pt. exp */
	0,	/* kill */
	0, 	/* bus error */
	0,	/* memory faults */
	0,	/* bad sys call */
	0,	/* bad pipe call */
	0,	/* alarm */
	0, 	/* software termination */
	0,	/* unassigned */
	0,	/* unassigned */
	0,	/* death of child (if not BERKELEY) */
	0,	/* power fail */
};

void 	(*(sigval[MAXTRAP]))() =	/* DAG -- make sure there are MAXTRAP */
{
	0,
#if BRL
	fault,
#else
	done,
#endif
	fault,
	fault,
	done,
	done,
	done,
	done,
	done,
	0,
	done,
	done,
	done,
	done,
	fault,
	fault,
	done,
#if JOBS
	0,	/* SIGSTOP */
#else
	done,
#endif
	done,
	done,
#if defined(BERKELEY) || defined(BRL) && !defined(pdp11)
	done,
	done,
	done,
	done,
	done,
	done,
	done,
	done,
	done,
	done,
	done,
	done
#endif
};

/* ========	fault handling routines	   ======== */


void	/* DAG */
fault(sig,code,scp)
register int	sig;
int code;
struct sigcontext *scp;
{
	register int	flag;

#if	!defined(BERKELEY)
	signal(sig, (int (*)())fault);
#endif
	if (sig == SIGSEGV)
	{
		if (setbrk(brkincr) == -1) {
			error(nospace);
		}
	}
	else if (sig == SIGALRM)
	{
		if (flags & waiting)
		{
#if BRL
#if pdp11
			prs(quomsg = timout);	/* for user and quota */
#else
			prs(timout);	/* for user */
#endif
			newline();
#endif
			done();
		}
	}
#if BRL
	else if (loginsh && sig == SIGHUP)
	{
#if pdp11
		quomsg = hangup;	/* for quota */
#endif
		done();
	}
	else if (loginsh && sig == SIGTERM)
	{
#if pdp11
		prs(quomsg = terminate);/* for user and quota */
#else
		prs(terminate);	/* for user */
#endif
		newline();
		done();
	}
#endif
	else
	{
		flag = (trapcom[sig] ? TRAPSET : SIGSET);
		trapnote |= flag;
		trapflg[sig] |= flag;
		if (sig == SIGINT)
			wasintr++;
	}
}

stdsigs()
{
	setsig(SIGHUP);
	setsig(SIGINT);
	ignsig(SIGQUIT);
	setsig(SIGILL);
	setsig(SIGTRAP);
	setsig(SIGIOT);
	setsig(SIGEMT);
	setsig(SIGFPE);
	setsig(SIGBUS);
	signal(SIGSEGV, (int (*)())fault);
	setsig(SIGSYS);
	setsig(SIGPIPE);
	setsig(SIGALRM);
	setsig(SIGTERM);
	setsig(SIGUSR1);
	setsig(SIGUSR2);
#ifndef JOBS
	setsig(SIGSTOP);
#endif
}

ignsig(n)
{
	register int	s, i;

	if ((i = n) == SIGSEGV)
	{
		clrsig(i);
		failed(badtrap, no11trap);	/* DAG -- made string sharable */
	}
	else if ((s = (signal(i, SIG_IGN) == SIG_IGN)) == 0)
	{
		trapflg[i] |= SIGMOD;
	}
	return(s);
}

getsig(n)
{
	register int	i;

	if (trapflg[i = n] & SIGMOD || ignsig(i) == 0) {
#if	BERKELEY
		struct sigvec vec = {(int (*)())fault, 0, SV_INTERRUPT};
		sigvec(i, &vec, (struct sigvec *)0);
#else
		signal(i, (int (*)())fault);
#endif
	}
}


setsig(n)
{
	register int	i;

	if (ignsig(i = n) == 0) {
#ifndef	BERKELEY
		struct sigvec vec;
		vec.sv_handler = (int (*)())sigval[i];
		vec.sv_mask = 0;
		vec.sv_flags = SV_INTERRUPT;
		sigvec(i, &vec, (struct sigvec *)0);
#else
		signal(i, (int (*)())sigval[i]);
#endif
	}
}

oldsigs()
{
	register int	i;
	register char	*t;

	i = MAXTRAP;
	while (i--)
	{
		t = trapcom[i];
		if (t == 0 || *t)
			clrsig(i);
		trapflg[i] = 0;
	}
	trapnote = 0;
}

clrsig(i)
int	i;
{
	free(trapcom[i]);
	trapcom[i] = 0;
	if (trapflg[i] & SIGMOD)
	{
#if	BERKELEY
		struct sigvec vec;
		vec.sv_handler = (int (*)())sigval[i];
		vec.sv_mask = 0;
		vec.sv_flags = SV_INTERRUPT;
		sigvec(i, &vec, (struct sigvec *)0);
#else
		signal(i, (int (*)())sigval[i]);
#endif
		trapflg[i] &= ~SIGMOD;
	}
}

/*
 * check for traps
 */
chktrap()
{
	register int	i = MAXTRAP;
	register char	*t;

	trapnote &= ~TRAPSET;
	while (--i)
	{
		if (trapflg[i] & TRAPSET)
		{
			trapflg[i] &= ~TRAPSET;
			if (t = trapcom[i])
			{
				int	savxit = exitval;

				execexp(t, 0);
				exitval = savxit;
				exitset();
			}
		}
	}
}

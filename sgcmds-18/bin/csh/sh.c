/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)sh.c	5.3 (Berkeley) 3/29/86";
#endif

#include "sh.h"
#include <sys/ioctl.h>

/*
 * C Shell
 *
 * Bill Joy, UC Berkeley, California, USA
 * October 1978, May 1980
 *
 * Jim Kulp, IIASA, Laxenburg, Austria
 * April 1980
 **********************************************************************
 * HISTORY
 * 07-May-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Split srccat calls into srccatdef and srccathome.  Added code to
 *	use CSHHOME in preference to HOME when reading profiles.
 *
 * 11-Apr-86  CMU Software (cmu) at Carnegie-Mellon University
 *	Changed code around srccat calls so that system profiles are
 *	always taken, even with -f specified.
 *
 * 09-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Reorganize way that CSPROFILES are implemented by srccat().
 *
 * 08-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Initialize "editmode" shell variable from environment if defined
 *	there.
 *
 * 04-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created CS (CMU CS Dept.) version of this shell from
 *	"newcsh" on CMU 4.1 UNIX systems.  Features include those added
 *	by Ken Greer, Steve Shafer, Duane Williams, Doug Phillips,
 *	and Glenn Marcy within the last five years.  The features are:
 *		command-line editing in style of vi or emacs
 *		command-line filename expansion
 *		setpath command and bind-to-keys command
 *		autologout code
 *	CMU changes to standard (sh.*.c, sh.*.h) files are enclosed
 *	in conditional compilation based on CMUCS symbol; new CMU
 *	files do not begin with "sh." in the filename.
 *
 **********************************************************************
 */

#if	CMUCS && !NeXT_MOD
#define CSPROFILES "/usr/cs/lib/profiles"
#define DEFAULT_AUTOLOGOUT	"0"	/* no autologout default */
#endif	CMUCS

#if	CMUCS && NeXT_MOD
#define CSPROFILES "/etc"
#define DEFAULT_AUTOLOGOUT	"0"	/* no autologout default */
#endif	CMUCS

#if	CMUCS && !NeXT_MOD
char	*pathlist[] =	{ ".", "/usr/cs/bin", "/usr/ucb", "/bin", "/usr/bin", 0 };
#else	CMUCS
char	*pathlist[] =	{ ".", "/usr/ucb", "/bin", "/usr/bin", 0 };
#endif	CMUCS
char	*dumphist[] =	{ "history", "-h", 0, 0 };
char	*loadhist[] =	{ "source", "-h", "~/.history", 0 };
char	HIST = '!';
char	HISTSUB = '^';
bool	nofile;
volatile bool	reenter;
bool	nverbose;
bool	nexececho;
bool	quitit = 0;
bool	fast;
bool	batch;
bool	prompt = 1;
bool	enterhist = 0;
#if	CMUCS
bool	warnpath = 0;
#endif	CMUCS

#if	BSD42
extern	int getegid(), getgid();
extern	int geteuid(), getuid();
#else	BSD43
extern	gid_t getegid(), getgid();
extern	uid_t geteuid(), getuid();
#endif	BSD42

#if	CMUCS
auto_logout ()
{
    printf ("auto-logout\n");
    close (SHIN);
    child++;
    goodbye ();
}
#endif	CMUCS

main(c, av)
	int c;
	char **av;
{
	register char **v, *cp;
	register int f;
	struct sigvec osv;
#ifdef NeXT_MOD
	extern int atexit (void (*func)(void));
	void untty();

	atexit (untty);
#endif NeXT_MOD

	settimes();			/* Immed. estab. timing base */
	v = av;
	if (eq(v[0], "a.out"))		/* A.out's are quittable */
		quitit = 1;
	uid = getuid();
	loginsh = **v == '-' && c == 1;
	if (loginsh)
		(void) time(&chktim);

	/*
	 * Move the descriptors to safe places.
	 * The variable didfds is 0 while we have only FSH* to work with.
	 * When didfds is true, we have 0,1,2 and prefer to use these.
	 */
	initdesc();

	/*
	 * Initialize the shell variables.
	 * ARGV and PROMPT are initialized later.
	 * STATUS is also munged in several places.
	 * CHILD is munged when forking/waiting
	 */

#if	CMUCS
	set("autologout", DEFAULT_AUTOLOGOUT);
	(void) signal(SIGALRM, auto_logout);
#endif	CMUCS
	set("status", "0");
	dinit(cp = getenv("HOME"));	/* dinit thinks that HOME == cwd in a
					 * login shell */
	if (cp == NOSTR)
		fast++;			/* No home -> can't read scripts */
	else
		set("home", savestr(cp));
	/*
	 * Grab other useful things from the environment.
	 * Should we grab everything??
	 */
	if ((cp = getenv("USER")) != NOSTR)
		set("user", savestr(cp));
	if ((cp = getenv("TERM")) != NOSTR)
		set("term", savestr(cp));
	/*
	 * Re-initialize path if set in environment
	 */
	if ((cp = getenv("PATH")) == NOSTR)
		set1("path", saveblk(pathlist), &shvhed);
	else
		importpath(cp);
#if	CMUCS
	if ((cp = getenv("EDITMODE")) == NOSTR)
		cp = "dumb";
	set("editmode", savestr(cp));
	if ((cp = getenv("SHELL")) != NOSTR)
		set("shell", savestr(cp));
	else
#endif	CMUCS
	set("shell", SHELLPATH);

	doldol = putn(getpid());		/* For $$ */
	shtemp = strspl("/tmp/sh", doldol);	/* For << */

	/*
	 * Record the interrupt states from the parent process.
	 * If the parent is non-interruptible our hand must be forced
	 * or we (and our children) won't be either.
	 * Our children inherit termination from our parent.
	 * We catch it only if we are the login shell.
	 */
		/* parents interruptibility */
	(void) sigvec(SIGINT, (struct sigvec *)0, &osv);
	parintr = osv.sv_handler;
		/* parents terminability */
	(void) sigvec(SIGTERM, (struct sigvec *)0, &osv);
	parterm = osv.sv_handler;
	if (loginsh) {
		(void) signal(SIGHUP, phup);	/* exit processing on HUP */
		(void) signal(SIGXCPU, phup);	/* ...and on XCPU */
		(void) signal(SIGXFSZ, phup);	/* ...and on XFSZ */
	}

	/*
	 * Process the arguments.
	 *
	 * Note that processing of -v/-x is actually delayed till after
	 * script processing.
	 */
	c--, v++;
	while (c > 0 && (cp = v[0])[0] == '-' && *++cp != '\0' && !batch) {
		do switch (*cp++) {

		case 'b':		/* -b	Next arg is input file */
			batch++;
			break;

		case 'c':		/* -c	Command input from arg */
			if (c == 1)
				exit(0);
			c--, v++;
			arginp = v[0];
			prompt = 0;
			nofile++;
			break;

		case 'e':		/* -e	Exit on any error */
			exiterr++;
			break;

		case 'f':		/* -f	Fast start */
			fast++;
			break;

		case 'i':		/* -i	Interactive, even if !intty */
			intact++;
			nofile++;
			break;

		case 'n':		/* -n	Don't execute */
			noexec++;
			break;

		case 'q':		/* -q	(Undoc'd) ... die on quit */
			quitit = 1;
			break;

		case 's':		/* -s	Read from std input */
			nofile++;
			break;

		case 't':		/* -t	Read one line from input */
			onelflg = 2;
			prompt = 0;
			nofile++;
			break;

		case 'v':		/* -v	Echo hist expanded input */
			nverbose = 1;			/* ... later */
			break;

		case 'x':		/* -x	Echo just before execution */
			nexececho = 1;			/* ... later */
			break;

		case 'V':		/* -V	Echo hist expanded input */
			setNS("verbose");		/* NOW! */
			break;

		case 'X':		/* -X	Echo just before execution */
			setNS("echo");			/* NOW! */
			break;

		} while (*cp);
		v++, c--;
	}

	if (quitit)			/* With all due haste, for debugging */
		(void) signal(SIGQUIT, SIG_DFL);

	/*
	 * Unless prevented by -c, -i, -s, or -t, if there
	 * are remaining arguments the first of them is the name
	 * of a shell file from which to read commands.
	 */
	if (nofile == 0 && c > 0) {
		nofile = open(v[0], 0);
		if (nofile < 0) {
			child++;		/* So this ... */
			Perror(v[0]);		/* ... doesn't return */
		}
		file = v[0];
		SHIN = dmove(nofile, FSHIN);	/* Replace FSHIN */
		(void) ioctl(SHIN, FIOCLEX, (char *)0);
		prompt = 0;
		c--, v++;
	}
	if (!batch && (uid != geteuid() || getgid() != getegid())) {
		errno = EACCES;
		child++;			/* So this ... */
		Perror("csh");			/* ... doesn't return */
	}
	/*
	 * Consider input a tty if it really is or we are interactive.
	 */
	intty = intact || isatty(SHIN);
	/*
	 * Decide whether we should play with signals or not.
	 * If we are explicitly told (via -i, or -) or we are a login
	 * shell (arg0 starts with -) or the input and output are both
	 * the ttys("csh", or "csh</dev/ttyx>/dev/ttyx")
	 * Note that in only the login shell is it likely that parent
	 * may have set signals to be ignored
	 */
	if (loginsh || intact || intty && isatty(SHOUT))
		setintr = 1;
#if	CMUCS
	if (setintr) {
		if (!eq(value("editmode"), "none"))
			cmdedit = 1;
	}
#endif	CMUCS
#ifdef TELL
	settell();
#endif
	/*
	 * Save the remaining arguments in argv.
	 */
	setq("argv", v, &shvhed);

	/*
	 * Set up the prompt.
	 */
	if (prompt)
		set("prompt", uid == 0 ? "# " : "% ");

	/*
	 * If we are an interactive shell, then start fiddling
	 * with the signals; this is a tricky game.
	 */
	shpgrp = getpgrp(0);
	opgrp = tpgrp = -1;
	oldisc = -1;
	if (setintr) {
		**av = '-';
		if (!quitit)		/* Wary! */
			(void) signal(SIGQUIT, SIG_IGN);
		(void) signal(SIGINT, pintr);
		(void) sigblock(sigmask(SIGINT));
		(void) signal(SIGTERM, SIG_IGN);
		if (quitit == 0 && arginp == 0) {
			(void) signal(SIGTSTP, SIG_IGN);
			(void) signal(SIGTTIN, SIG_IGN);
			(void) signal(SIGTTOU, SIG_IGN);
			/*
			 * Wait till in foreground, in case someone
			 * stupidly runs
			 *	csh &
			 * dont want to try to grab away the tty.
			 */
			if (isatty(FSHDIAG))
				f = FSHDIAG;
			else if (isatty(FSHOUT))
				f = FSHOUT;
			else if (isatty(OLDSTD))
				f = OLDSTD;
			else
				f = -1;
retry:
			if (ioctl(f, TIOCGPGRP, (char *)&tpgrp) == 0 &&
			    tpgrp != -1) {
				int ldisc;
				if (tpgrp != shpgrp) {
					int (*old)() = signal(SIGTTIN, SIG_DFL);
					(void) kill(0, SIGTTIN);
					(void) signal(SIGTTIN, old);
					goto retry;
				}
				if (ioctl(f, TIOCGETD, (char *)&oldisc) != 0) 
					goto notty;
				if (oldisc != NTTYDISC) {
#ifdef DEBUG
					printf("Switching to new tty driver...\n");
#endif DEBUG
					ldisc = NTTYDISC;
					(void) ioctl(f, TIOCSETD,
						(char *)&ldisc);
				} else
					oldisc = -1;
				opgrp = shpgrp;
				shpgrp = getpid();
				tpgrp = shpgrp;
				(void) ioctl(f, TIOCSPGRP, (char *)&shpgrp);
				(void) setpgrp(0, shpgrp);
				(void) ioctl(dcopy(f, FSHTTY), FIOCLEX,
					(char *)0);
			} else {
notty:
  printf("Warning: no access to tty; thus no job control in this shell...\n");
				tpgrp = -1;
#if	CMUCS
				cmdedit = 0;
#endif	CMUCS
			}
		}
	}
	if (setintr == 0 && parintr == SIG_DFL)
		setintr++;
	(void) signal(SIGCHLD, pchild);	/* while signals not ready */

	/*
	 * Set an exit here in case of an interrupt or error reading
	 * the shell start-up scripts.
	 */
	setexit();
	haderr = 0;		/* In case second time through */
#if	CMUCS
	if (reenter == 0) {
		reenter++;
		srccatdef("/cshrc.std");
		if (!fast) {
			/* Will have value("home") here because set fast if don't */
			srccathome("/.cshrc");
			if (!arginp && !onelflg)
				dohash();
		}
		if (loginsh) {
			srccatdef("/login.std");
			if (!fast)
				srccathome("/.login");
		}
		dosource(loadhist);
	}
#else	CMUCS
	if (!fast && reenter == 0) {
		reenter++;
		/* Will have value("home") here because set fast if don't */
		srccat(value("home"), "/.cshrc");
		if (!fast && !arginp && !onelflg && !havhash)
			dohash();
		if (loginsh) {
			srccat(value("home"), "/.login");
		}
		dosource(loadhist);
	}
#endif	CMUCS

	/*
	 * Now are ready for the -v and -x flags
	 */
	if (nverbose)
		setNS("verbose");
	if (nexececho)
		setNS("echo");

	/*
	 * All the rest of the world is inside this call.
	 * The argument to process indicates whether it should
	 * catch "error unwinds".  Thus if we are a interactive shell
	 * our call here will never return by being blown past on an error.
	 */
	process(setintr);

	/*
	 * Mop-up.
	 */
	if (loginsh) {
		printf("logout\n");
		(void) close(SHIN);
		child++;
		goodbye();
	}
	rechist();
	exitstat();
}

untty()
{

	if (tpgrp > 0) {
		(void) setpgrp(0, opgrp);
		(void) ioctl(FSHTTY, TIOCSPGRP, (char *)&opgrp);
		if (oldisc != -1 && oldisc != NTTYDISC) {
#ifdef DEBUG
			printf("\nReverting to old tty driver...\n");
#endif DEBUG
			(void) ioctl(FSHTTY, TIOCSETD, (char *)&oldisc);
		}
	}
}

importpath(cp)
	char *cp;
{
	register int i = 0;
	register char *dp;
	register char **pv;
	int c;
	static char dot[2] = {'.', 0};

	for (dp = cp; *dp; dp++)
		if (*dp == ':')
			i++;
	/*
	 * i+2 where i is the number of colons in the path.
	 * There are i+1 directories in the path plus we need
	 * room for a zero terminator.
	 */
	pv = (char **) calloc((unsigned) (i + 2), sizeof (char **));
	dp = cp;
	i = 0;
	if (*dp)
	for (;;) {
		if ((c = *dp) == ':' || c == 0) {
			*dp = 0;
			pv[i++] = savestr(*cp ? cp : dot);
			if (c) {
				cp = dp + 1;
				*dp = ':';
			} else
				break;
		}
		dp++;
	}
	pv[i] = 0;
	set1("path", pv, &shvhed);
}

#if	CMUCS
/*
 * Source a system default profile.
 */
srccatdef(dp)
	char *dp;
{
	register char *ep = strspl(CSPROFILES, dp);
	register int unit = dmove(open(ep, 0), -1);

	(void) ioctl(unit, FIOCLEX, (char *)0);
	xfree(ep);
	srcunit(unit, 0, 0);
}

/*
 * Source a profile from home (or cshhome) directory.
 */
srccathome(pp)
	char *pp;
{
	register char *hp, *ep;
	register int unit;

	if (hp = getenv("CSHHOME"))
		ep = strspl(hp, pp);
	else if (!adrof("home"))
		return;
	else
		ep = strspl(value("home"), pp);
	unit = dmove(open(ep, 0), -1);
	(void) ioctl(unit, FIOCLEX, (char *)0);
	xfree(ep);
	if (loginsh)
		warnpath = 1;
	srcunit(unit, hp ? 0 : 1, 0);
	if (loginsh)
		warnpath = 0;
}

pathwarn(p)
char *p;
{
	if (!warnpath)
		return;
	if (eq(p, "path"))
		printf("Please use the setpath command instead of \"set path = ...\".\n");
	else
		printf("Please use the setpath command instead of \"setenv %s ...\".\n", p);
}
#else	CMUCS
/*
 * Source to the file which is the catenation of the argument names.
 */
srccat(cp, dp)
	char *cp, *dp;
{
	register char *ep = strspl(cp, dp);
	register int unit = dmove(open(ep, 0), -1);

	(void) ioctl(unit, FIOCLEX, (char *)0);
	xfree(ep);
#ifdef INGRES
	srcunit(unit, 0, 0);
#else
	srcunit(unit, 1, 0);
#endif
}
#endif	CMUCS

/*
 * Source to a unit.  If onlyown it must be our file or our group or
 * we don't chance it.	This occurs on ".cshrc"s and the like.
 */
volatile int	insource;
static srcunit(unit, onlyown, hflg)
	register int unit;
	bool onlyown;
	bool hflg;
{
	/* We have to push down a lot of state here */
	/* All this could go into a structure */
	int oSHIN = -1, oldintty = intty;
	struct whyle *oldwhyl = whyles;
	char *ogointr = gointr, *oarginp = arginp;
	char *oevalp = evalp, **oevalvec = evalvec;
	int oonelflg = onelflg;
	bool oenterhist = enterhist;
	char OHIST = HIST;
#ifdef TELL
	bool otell = cantell;
#endif
	struct Bin saveB;

	/* The (few) real local variables */
	jmp_buf oldexit;
	volatile int reenter;
	int omask;

	if (unit < 0)
		return;
	if (didfds)
		donefds();
	if (onlyown) {
		struct stat stb;

		if (fstat(unit, &stb) < 0 ||
		    (stb.st_uid != uid && stb.st_gid != getgid())) {
			(void) close(unit);
			return;
		}
	}

	/*
	 * There is a critical section here while we are pushing down the
	 * input stream since we have stuff in different structures.
	 * If we weren't careful an interrupt could corrupt SHIN's Bin
	 * structure and kill the shell.
	 *
	 * We could avoid the critical region by grouping all the stuff
	 * in a single structure and pointing at it to move it all at
	 * once.  This is less efficient globally on many variable references
	 * however.
	 */
	insource = 1;
	getexit(oldexit);
	reenter = 0;
	if (setintr)
		omask = sigblock(sigmask(SIGINT));
	setexit();
	reenter++;
	if (reenter == 1) {
		/* Setup the new values of the state stuff saved above */
		copy((char *)&saveB, (char *)&B, sizeof saveB);
		fbuf = (char **) 0;
		fseekp = feobp = fblocks = 0;
		oSHIN = SHIN, SHIN = unit, arginp = 0, onelflg = 0;
		intty = isatty(SHIN), whyles = 0, gointr = 0;
		evalvec = 0; evalp = 0;
		enterhist = hflg;
		if (enterhist)
			HIST = '\0';
		/*
		 * Now if we are allowing commands to be interrupted,
		 * we let ourselves be interrupted.
		 */
		if (setintr)
			(void) sigsetmask(omask);
#ifdef TELL
		settell();
#endif
		process(1);		/* 0 -> blow away on errors */
	}
	if (setintr)
		(void) sigsetmask(omask);
	if (oSHIN >= 0) {
		register int i;

		/* We made it to the new state... free up its storage */
		/* This code could get run twice but xfree doesn't care */
		for (i = 0; i < fblocks; i++)
			xfree(fbuf[i]);
		xfree((char *)fbuf);

		/* Reset input arena */
		copy((char *)&B, (char *)&saveB, sizeof B);

		(void) close(SHIN), SHIN = oSHIN;
		arginp = oarginp, onelflg = oonelflg;
		evalp = oevalp, evalvec = oevalvec;
		intty = oldintty, whyles = oldwhyl, gointr = ogointr;
		if (enterhist)
			HIST = OHIST;
		enterhist = oenterhist;
#ifdef TELL
		cantell = otell;
#endif
	}

	resexit(oldexit);
	/*
	 * If process reset() (effectively an unwind) then
	 * we must also unwind.
	 */
	if (reenter >= 2)
		error(NOSTR);
	insource = 0;
}

rechist()
{
	char buf[BUFSIZ];
	int fp, ftmp, oldidfds;

	if (!fast) {
		if (value("savehist")[0] == '\0')
			return;
		(void) strcpy(buf, value("home"));
		(void) strcat(buf, "/.history");
		fp = creat(buf, 0666);
		if (fp == -1)
			return;
		oldidfds = didfds;
		didfds = 0;
		ftmp = SHOUT;
		SHOUT = fp;
		(void) strcpy(buf, value("savehist"));
		dumphist[2] = buf;
		dohist(dumphist);
		(void) close(fp);
		SHOUT = ftmp;
		didfds = oldidfds;
	}
}

goodbye()
{
	if (loginsh) {
		(void) signal(SIGQUIT, SIG_IGN);
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGTERM, SIG_IGN);
		setintr = 0;		/* No interrupts after "logout" */
#if	CMUCS
		srccatdef("/logout.std");
		srccathome("/.logout");
#else	CMUCS
		if (adrof("home"))
			srccat(value("home"), "/.logout");
#endif	CMUCS
	}
#if	CMUCS
	if (!fast && cmdedit)
		SaveMacros();
#endif	CMUCS
	rechist();
	exitstat();
}

exitstat()
{

#ifdef PROF
	monitor(0);
#endif
	/*
	 * Note that if STATUS is corrupted (i.e. getn bombs)
	 * then error will exit directly because we poke child here.
	 * Otherwise we might continue unwarrantedly (sic).
	 */
	child++;
	exit(getn(value("status")));
}

/*
 * in the event of a HUP we want to save the history
 */
phup()
{
	rechist();
	exit(1);
}

char	*jobargv[2] = { "jobs", 0 };
/*
 * Catch an interrupt, e.g. during lexical input.
 * If we are an interactive shell, we reset the interrupt catch
 * immediately.  In any case we drain the shell output,
 * and finally go through the normal error mechanism, which
 * gets a chance to make the shell go away.
 */
pintr()
{
	pintr1(1);
}

pintr1(wantnl)
	bool wantnl;
{
	register char **v;
	int omask;

	omask = sigblock(0);
	if (setintr) {
		(void) sigsetmask(omask & ~sigmask(SIGINT));
		if (pjobs) {
			pjobs = 0;
			printf("\n");
			dojobs(jobargv);
			bferr("Interrupted");
		}
	}
	(void) sigsetmask(omask & ~sigmask(SIGCHLD));
	draino();

	/*
	 * If we have an active "onintr" then we search for the label.
	 * Note that if one does "onintr -" then we shan't be interruptible
	 * so we needn't worry about that here.
	 */
	if (gointr) {
		search(ZGOTO, 0, gointr);
		timflg = 0;
		if (v = pargv)
			pargv = 0, blkfree(v);
		if (v = gargv)
			gargv = 0, blkfree(v);
		reset();
	} else if (intty && wantnl)
		printf("\n");		/* Some like this, others don't */
	error(NOSTR);
}

/*
 * Process is the main driving routine for the shell.
 * It runs all command processing, except for those within { ... }
 * in expressions (which is run by a routine evalav in sh.exp.c which
 * is a stripped down process), and `...` evaluation which is run
 * also by a subset of this code in sh.glob.c in the routine backeval.
 *
 * The code here is a little strange because part of it is interruptible
 * and hence freeing of structures appears to occur when none is necessary
 * if this is ignored.
 *
 * Note that if catch is not set then we will unwind on any error.
 * If an end-of-file occurs, we return.
 */
process(catch)
	bool catch;
{
	jmp_buf osetexit;
	register struct command *t;

	getexit(osetexit);
	for (;;) {
#if	CMUCS
		register char *cp;
		volatile int sawhist;
		volatile int edithist = (adrof("edithist") != 0);
#endif	CMUCS
		pendjob();
		paraml.next = paraml.prev = &paraml;
		paraml.word = "";
		t = 0;
		setexit();
		justpr = enterhist;	/* execute if not entering history */

		/*
		 * Interruptible during interactive reads
		 */
		if (setintr)
			(void) sigsetmask(sigblock(0) & ~sigmask(SIGINT));

		/*
		 * For the sake of reset()
		 */
		freelex(&paraml), freesyn(t), t = 0;

		if (haderr) {
			if (!catch) {
				/* unwind */
				doneinp = 0;
				resexit(osetexit);
				reset();
			}
			haderr = 0;
			/*
			 * Every error is eventually caught here or
			 * the shell dies.  It is at this
			 * point that we clean up any left-over open
			 * files, by closing all but a fixed number
			 * of pre-defined files.  Thus routines don't
			 * have to worry about leaving files open due
			 * to deeper errors... they will get closed here.
			 */
			closem();
			continue;
		}
		if (doneinp) {
			doneinp = 0;
			break;
		}
		if (chkstop)
			chkstop--;
		if (neednote)
			pnote();
		if (intty && prompt && evalvec == 0) {
			mailchk();
			/*
			 * If we are at the end of the input buffer
			 * then we are going to read fresh stuff.
			 * Otherwise, we are rereading input and don't
			 * need or want to prompt.
			 */
			if (fseekp == feobp)
				printprompt();
#if	CMUCS
			if (cp = value("autologout"))
				alarm(atoi(cp) * 60);	/* Autologout ON */
#endif	CMUCS
		}
		err = 0;

		/*
		 * Echo not only on VERBOSE, but also with history expansion.
		 * If there is a lexical error then we forego history echo.
		 */
#if	CMUCS
		/* Also push back input if history went off in
		 * interactive mode.
		 */
		if ((sawhist=lex(&paraml)) && !err && intty ||
		    adrof("verbose")) {
			haderr = 1;
			if ((justpr || edithist) && sawhist && cmdedit)
			    prpushlex(&paraml);
			else
			    prlex(&paraml);
			haderr = 0;
		}
#else	CMUCS
		if (lex(&paraml) && !err && intty ||
		    adrof("verbose")) {
			haderr = 1;
			prlex(&paraml);
			haderr = 0;
		}
#endif	CMUCS
#if	CMUCS
		alarm(0);				/* Autologout OFF */
#endif	CMUCS

		/*
		 * The parser may lose space if interrupted.
		 */
		if (setintr)
			(void) sigblock(sigmask(SIGINT));

		/*
		 * Save input text on the history list if 
		 * reading in old history, or it
		 * is from the terminal at the top level and not
		 * in a loop.
		 */
#if	CMUCS
		/* ... unless we have just pushed the command back.
		 */
		if (!((justpr || edithist) && sawhist && cmdedit))
#endif	CMUCS
		if (enterhist || catch && intty && !whyles)
			savehist(&paraml);

		/*
		 * Print lexical error messages, except when sourcing
		 * history lists.
		 */
		if (!enterhist && err)
			error(err);

		/*
		 * If had a history command :p modifier then
		 * this is as far as we should go
		 */
#if	CMUCS
		/* ... or if we had an interactive history expand
		 */
		if (justpr || (edithist && sawhist && cmdedit))
			reset();
#else	CMUCS
		if (justpr)
			reset();
#endif	CMUCS

		alias(&paraml);

		/*
		 * Parse the words of the input into a parse tree.
		 */
		t = syntax(paraml.next, &paraml, 0);
		if (err)
			error(err);

		/*
		 * Execute the parse tree
		 */
		execute(t, tpgrp);

		/*
		 * Made it!
		 */
		freelex(&paraml), freesyn(t);
	}
	resexit(osetexit);
}

dosource(t)
	register char **t;
{
	register char *f;
	register int u;
	bool hflg = 0;
	char buf[BUFSIZ];

	t++;
	if (*t && eq(*t, "-h")) {
		/*
		 * Check for null arg to -h.  MAM  March 28, 1989
		 */
 		if (*++t == NOSTR)
 			bferr("No operand for -h flag");
  		hflg++;
	}
	if (*t == NULL)  {
	        printf ("source: No operand for -h flag\n");
		return;
	}
	(void) strcpy(buf, *t);
	f = globone(buf);
	u = dmove(open(f, 0), -1);
	xfree(f);
	/*
	 * Just print out an error using perror() when an open of a
	 * source'd file fails instead of not letting the login
	 * session continue by calling Perror().
	 *	MAM	March 29, 1989
	 */
	 if (u < 0 && !hflg)  {
		perror(f);
		flush();
	}
	else  {
		(void) ioctl(u, FIOCLEX, (char *)0);
		srcunit(u, 0, hflg);
	}
	return;
}

/*
 * Check for mail.
 * If we are a login shell, then we don't want to tell
 * about any mail file unless its been modified
 * after the time we started.
 * This prevents us from telling the user things he already
 * knows, since the login program insists on saying
 * "You have mail."
 */
mailchk()
{
	register struct varent *v;
	register char **vp;
	time_t t;
	int intvl, cnt;
	struct stat stb;
	bool new;

	v = adrof("mail");
	if (v == 0)
		return;
	(void) time(&t);
	vp = v->vec;
	cnt = blklen(vp);
	intvl = (cnt && number(*vp)) ? (--cnt, getn(*vp++)) : MAILINTVL;
	if (intvl < 1)
		intvl = 1;
	if (chktim + intvl > t)
		return;
	for (; *vp; vp++) {
		if (stat(*vp, &stb) < 0)
			continue;
		new = stb.st_mtime > time0.tv_sec;
		if (stb.st_size == 0 || stb.st_atime > stb.st_mtime ||
		    (stb.st_atime < chktim && stb.st_mtime < chktim) ||
		    loginsh && !new)
			continue;
		if (cnt == 1)
			printf("You have %smail.\n", new ? "new " : "");
		else
			printf("%s in %s.\n", new ? "New mail" : "Mail", *vp);
	}
	chktim = t;
}

#include <pwd.h>
/*
 * Extract a home directory from the password file
 * The argument points to a buffer where the name of the
 * user whose home directory is sought is currently.
 * We write the home directory of the user back there.
 */
gethdir(home)
	char *home;
{
	register struct passwd *pp = getpwnam(home);

	if (pp == 0)
		return (1);
	(void) strcpy(home, pp->pw_dir);
	return (0);
}

/*
 * Move the initial descriptors to their eventual
 * resting places, closin all other units.
 */
initdesc()
{

	didfds = 0;			/* 0, 1, 2 aren't set up */
	(void) ioctl(SHIN = dcopy(0, FSHIN), FIOCLEX, (char *)0);
	(void) ioctl(SHOUT = dcopy(1, FSHOUT), FIOCLEX, (char *)0);
	(void) ioctl(SHDIAG = dcopy(2, FSHDIAG), FIOCLEX, (char *)0);
	(void) ioctl(OLDSTD = dcopy(SHIN, FOLDSTD), FIOCLEX, (char *)0);
	closem();
}

#ifdef PROF
done(i)
#else
#ifdef NeXT_MOD
brain_damaged_exit(i)
#else
exit(i)
#endif NeXT_MOD
#endif PROF
	int i;
{

	untty();
	_exit(i);
}

printprompt()
{
	register char *cp;

	if (!whyles) {
		for (cp = value("prompt"); *cp; cp++)
			if (*cp == HIST)
				printf("%d", eventno + 1);
			else {
				if (*cp == '\\' && cp[1] == HIST)
					cp++;
				putchar(*cp | QUOTE);
			}
	} else
		/* 
		 * Prompt for forward reading loop
		 * body content.
		 */
		printf("? ");
	flush();
}

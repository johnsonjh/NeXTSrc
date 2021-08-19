/*	@(#)service.c	1.11	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<errno.h>
#if BERKELEY
extern int	errno;
#else
#define	wait3	_wait3
#endif
#if defined(BERKELEY) || defined(BRL) && !defined(pdp11)
#include	"/usr/include/sys/wait.h"
#endif

#define ARGMK	01

extern int	gsort();
extern char	*sysmsg[];
extern short topfd;

static int split(register char *s);	/* blank interpretation routine */


/*
 * service routines for `execute'
 */
initio(iop, save)
	struct ionod	*iop;
	int		save;
{
	extern long	lseek();	/* DAG -- bug fix (was missing) */
	register char	*ion;
	register int	iof, fd;
	int		ioufd;
	short	lastfd;

	lastfd = topfd;
	while (iop)
	{
		iof = iop->iofile;
		ion = mactrim(iop->ioname);
		ioufd = iof & IOUFD;

		if (*ion && (flags&noexec) == 0)
		{
			if (save)
			{
				fdmap[topfd].org_fd = ioufd;
				fdmap[topfd++].dup_fd = savefd(ioufd);
			}

			if (iof & IODOC)
			{
				struct tempblk tb;

				subst(chkopen(ion), (fd = tmpfil(&tb)));

				poptemp();	/* pushed in tmpfil() --
						   bug fix for problem with
						   in-line scripts
						*/

				fd = chkopen(tmpout);
				unlink(tmpout);
			}
			else if (iof & IOMOV)
			{
				if (eq(minus, ion))
				{
					fd = -1;
					close(ioufd);
				}
				else if ((fd = stoi(ion)) >= USERIO)
					failed(ion, badfile);
				else
					fd = dup(fd);
			}
			else if (iof & IORDW)	/* DAG -- added (due to Brian Horn) */
			{
				if ((fd = open(ion, 2)) < 0)
					failed(ion, badopen);
			}
			else if ((iof & IOPUT) == 0)
				fd = chkopen(ion);
			else if (flags & rshflg)
				failed(ion, restricted);
			else if (iof & IOAPP && (fd = open(ion, 1)) >= 0)
				lseek(fd, 0L, 2);
			else
				fd = create(ion);
			if (fd >= 0)
				rename(fd, ioufd);
		}

		iop = iop->ionxt;
	}
	return(lastfd);
}

char *
simple(s)
char	*s;
{
	char	*sname;

	sname = s;
	while (1)
	{
		if (any('/', sname))
			while (*sname++ != '/')
				;
		else
			return(sname);
	}
}

char *
getpath(s)
	char	*s;
{
	register char	*path;

	if (any('/', s) || any(('/' | QUOTE), s))
	{
		if (flags & rshflg)
		{
			failed(s, restricted);
			/*NOTREACHED*/	/* DAG -- keep lint happy */
		}
		else
			return(nullstr);
	}
	else if ((path = pathnod.namval) == 0)
		return(defpath);
	else
		return(cpystak(path));
#ifdef gould
	return 0;	/* DAG -- added */
#endif
}

pathopen(path, name)
register char *path, *name;
{
	register int	f;

	do
	{
		path = catpath(path, name);
	} while ((f = open(curstak(), 0)) < 0 && path);
	return(f);
}

char *
catpath(path, name)
register char	*path;
char	*name;
{
	/*
	 * leaves result on top of stack
	 */
	register char	*scanp = path;
	register char	*argp = locstak();
#ifdef TILDE_SUB
	char		*save = argp;
	char		*cp;
#endif

	while (*scanp && *scanp != COLON)
		*argp++ = *scanp++;
#ifdef TILDE_SUB
	/* try a tilde expansion */
	*argp = '\0';
	if ( *save == SQUIGGLE && (cp = homedir( save + 1 )) != nullstr )
		{
		movstr( cp, save );
		argp = save + length( save ) - 1;
		}
#endif
	if (scanp != path)
		*argp++ = '/';
	if (*scanp == COLON)
		scanp++;
	path = (*scanp ? scanp : 0);
	scanp = name;
	while ((*argp++ = *scanp++))
		;
	return(path);
}

char *
nextpath(path)
	register char	*path;
{
	register char	*scanp = path;

	while (*scanp && *scanp != COLON)
		scanp++;

	if (*scanp == COLON)
		scanp++;

	return(*scanp ? scanp : 0);
}

static char	*xecmsg;
static char	**xecenv;

int	execa(at, pos)
	char	*at[];
	short pos;
{
	register char	*path;
	register char	**t = at;
	int		cnt;

	if ((flags & noexec) == 0)
	{
		xecmsg = notfound;
		path = getpath(*t);
		xecenv = setenv();

		if (pos > 0)
		{
			cnt = 1;
			while (cnt != pos)
			{
				++cnt;
				path = nextpath(path);
			}
			execs(path, t);
			path = getpath(*t);
		}
		while (path = execs(path,t))
			;
		failed(*t, xecmsg);
	}
}

static char *
execs(ap, t)
char	*ap;
register char	*t[];
{
	register char *p, *prefix;

	prefix = catpath(ap, t[0]);
	trim(p = curstak());
	sigchk();
	
	execve(p, &t[0] ,xecenv);
#if BRL && pdp11
	/* The close-on-exec emulation has closed INIO and OTIO;
	   this once was a problem, but seems to be fine with Release 2.0. */
#endif
	switch (errno)
	{
	case ENOEXEC:		/* could be a shell script */
		funcnt = 0;
		flags = 0;
		*flagadr = 0;
		comdiv = 0;
		ioset = 0;
		clearup();	/* remove open files and for loop junk */
		if (input)
			close(input);
		input = chkopen(p);
#if BRL
		/* don't try to interpret directories etc. */
		{
#include	<sys/types.h>
#include	<sys/stat.h>
			struct stat	sbuf;

			if (fstat(input, &sbuf) == 0
			 && (sbuf.st_mode&S_IFMT) != S_IFREG)
			{
				close(input);
				goto def;	/* badexec unless other found */
			}
		}
#endif
	
#ifdef ACCT
		preacct(p);	/* reset accounting */
#endif

		/*
		 * set up new args
		 */
		
		setargs(t);

#if BRL
		/* change argv[0] to aid ps utility */
		{
			register int	n;

			p = argv0;
			prefix = *t++;
			for (n = length(p); --n; )
			{
				if (prefix && *prefix)
					*p++ = *prefix++;
				else
				{
					*p++ = ' ';
					if (prefix = *t)
						t++;
				}
			}
		}
#endif
		longjmp(subshell, 1);

	case ENOMEM:
		failed(p, toobig);

	case E2BIG:
		failed(p, arglist);

	case ETXTBSY:
		failed(p, txtbsy);

	default:
#if BRL
    def:
#endif
		xecmsg = badexec;
	case ENOENT:
		return(prefix);
	}
}


/*
 * for processes to be waited for
 */
#define MAXP 20
static int	pwlist[MAXP];
static int	pwc;

#if JOBS
static int	*wf_pwlist;
static int	wf_pwc;

void
set_wfence()
{
	wf_pwlist = &pwlist[pwc];
	wf_pwc = 0;
}

BOOL
unpost(pcsid)
	int	pcsid;
{
	register int	*pw = pwlist;

	if (pcsid)
	{
		while (pw <= &pwlist[pwc])
		{
			if (pcsid == *pw)
			{
				if (pw >= wf_pwlist)
					wf_pwc--;
				else
					wf_pwlist--;
				while (pw <= &pwlist[pwc])
				{
					*pw = pw[1];
					pw++;
				}
				pw[pwc] = 0;
				pwc--;
				return TRUE;
			}
			pw++;
		}
		return FALSE;
	}
	else
		return TRUE;
}
#endif

postclr()
{
	register int	*pw = pwlist;

	while (pw <= &pwlist[pwc])
	{
#if JOBS
		j_child_clear(*pw);
#endif
		*pw++ = 0;
	}
	pwc = 0;
}

post(pcsid)
int	pcsid;
{
	register int	*pw = pwlist;

	if (pcsid)
	{
		while (*pw)
#ifdef JOBS
			if (pcsid == *pw)
				return;
			else
#endif
			    pw++;
		if (pwc >= MAXP - 1)
			pw--;
		else
		{
			pwc++;
#if JOBS
			wf_pwc++;
#endif
		}
		*pw = pcsid;
	}
}

await(i, bckg)
int	i, bckg;
{
#if BRL
#if pdp11
#include	<sys/types.h>
#include	<BRL/lwtimes.h>
	struct lwtimes	lw;
#else
#include	"/usr/include/sys/time.h"
#include	"/usr/include/sys/resource.h"
	struct rusage	ru;
#endif
#endif
	int	rc = 0, wx = 0;
#ifdef NeXT_MOD
	union wait w;
#else
	int	w;
#endif NeXT_MOD
	int	ipwc = pwc;
#if JOBS
	BOOL	update_only = i == -2;

	if (update_only)
		i = -1;
	if ((flags&jobflg) == 0 || i != -1)
#endif
	    post(i);
#if JOBS
	while (pwc || (flags&jobflg))
#else
	while (pwc)
#endif
	{
		register int	p;
		register int	sig;
		int		w_hi;
		int	found = 0;

		{
#ifndef JOBS
			register int	*pw = pwlist;
#endif

#if BRL
#if pdp11
			p = _waittimes(&w, &lw, sizeof lw);
#else
#if JOBS
			if (i == 0 && (flags&jobflg) && wf_pwc == 0)
				break;
			if ((pwc == 0 && (flags&jobflg)) || update_only)
			{
				if ((p = wait3(&w, WUNTRACED|WNOHANG, &ru)) == 0)
				{
					unpost(i);
					break;
				}
				if (pwc == 0 && p == -1)
					break;
			}
			else
#endif
			    p = wait3(&w, WUNTRACED, &ru);
#endif
#else	/* !BRL */
#if JOBS
			/* need job control hook here */
#endif
			p = wait(&w);
#endif
			if (wasintr)
			{
				wasintr = 0;
				if (bckg)
				{
					break;
				}
			}
#if JOBS
			if (unpost(p))
				found++;
#else
			while (pw <= &pwlist[ipwc])
			{
				if (*pw == p)
				{
					*pw = 0;
					pwc--;
					found++;
				}
				else
					pw++;
			}
#endif
		}
		if (p == -1)
		{
			if (bckg)
			{
#if JOBS
				j_child_clear(i);
				unpost(i);
#else
				register int *pw = pwlist;

				while (pw <= &pwlist[ipwc] && i != *pw)
					pw++;
				if (i == *pw)
				{
					*pw = 0;
					pwc--;
				}
#endif
			}
			continue;
		}
#ifdef NeXT_MOD
		w_hi = (w.w_status >> 8) & LOBYTE;
		if (sig = w.w_status & 0177)
#else
		w_hi = (w >> 8) & LOBYTE;
		if (sig = w & 0177)
#endif NeXT_MOD
		{
			if (sig == 0177)	/* ptrace! return */
			{
				sig = w_hi;
#if JOBS
				if ((flags&jobflg) &&
				    (sig == SIGSTOP || sig == SIGTSTP ||
				     sig == SIGTTOU || sig == SIGTTIN))
				{
					j_child_stop(p, sig);
					goto j_bypass;
				}
#endif
				prs(ptrcolon);	/* DAG -- made string sharable */
			}
			if (sysmsg[sig])
			{
				if (i != p || (flags & prompt) == 0)
				{
					prp();
					prn(p);
					blank();
				}
				prs(sysmsg[sig]);
#ifdef NeXT_MOD
				if (w.w_status & 0200)
#else
				if (w & 0200)
#endif NeXT_MOD
					prs(coredump);
			}
			newline();
		}
#if BRL
		if (flags&infoflg)
		{
			register int	k;
			long		l;

			prc('[');
#if pdp11
			l = lw.lw_utime + lw.lw_stime + lw.lw_cutime + lw.lw_cstime;
			prn((int)(l / 60));	/* integral seconds */
			k = (int)(l % 60) * 100 / 60;	/* hundredths */
#else
			l = ru.ru_utime.tv_sec * 1000000L + ru.ru_utime.tv_usec
			  + ru.ru_stime.tv_sec * 1000000L + ru.ru_stime.tv_usec;
			prn((int)(l / 1000000));	/* integral seconds */
			k = (int)(l % 1000000L) / 1000;	/* thousandths */
#endif
			prc('.');
#ifndef pdp11
			if (k < 100)
				prc('0');
#endif
			if (k < 10)
				prc('0');
			prn(k);
			blank();
#if pdp11
			prn((int)lw.lw_bread + (int)lw.lw_cbread);
#else
			prn((int)ru.ru_inblock);
#endif
			blank();
#if pdp11
			prn((int)lw.lw_bwrite + (int)lw.lw_cbwrite);
			if (k = lw.lw_pages[0] + lw.lw_pages[1]
			      + lw.lw_pages[2] + lw.lw_pages[3])
			{
				blank();
				prn(k);
			}
#else
			prn((int)ru.ru_oublock);
			blank();
			prn((int)ru.ru_minflt);
			blank();
			prn((int)ru.ru_majflt);
#endif
			prc(']');
			newline();
		}
#endif	/* BRL */
#if JOBS
		j_child_die(p);
    j_bypass:
#endif
		if (rc == 0 && found != 0)
			rc = (sig ? sig | SIGFLG : w_hi);
#ifdef NeXT_MOD
		wx |= w.w_status;
#else
		wx |= w;
#endif NeXT_MOD
		if (p == i)
		{
			break;
		}
	}
	if (wx && flags & errflg)
		exitsh(rc);
	flags |= eflag;
	exitval = rc;
	exitset();
}

BOOL		nosubst;

trim(at)
char	*at;
{
	register char	*p;
	register char 	*ptr;
	register char	c;
	register char	q = 0;

	if (p = at)
	{
		ptr = p;
		while (c = *p++)
		{
			if (*ptr = c & STRIP)
				++ptr;
			q |= c;
		}

		*ptr = 0;
	}
	nosubst = q & QUOTE;
}

char *
mactrim(s)
char	*s;
{
	register char	*t = macro(s);

	trim(t);
	return(t);
}

char **
scan(argn)
int	argn;
{
	register struct argnod *argp = (struct argnod *)(Rcheat(gchain) & ~ARGMK);
	register char **comargn, **comargm;

	comargn = (char **)getstak(BYTESPERWORD * argn + BYTESPERWORD);
	comargm = comargn += argn;
	*comargn = ENDARGS;
	while (argp)
	{
		*--comargn = argp->argval;

		trim(*comargn);
		argp = argp->argnxt;

		if (argp == 0 || Rcheat(argp) & ARGMK)
		{
			gsort(comargn, comargm);
			comargm = comargn;
		}
		/* Lcheat(argp) &= ~ARGMK; */
		argp = (struct argnod *)(Rcheat(argp) & ~ARGMK);
	}
	return(comargn);
}

static int
gsort(from, to)
char	*from[], *to[];
{
	int	k, m, n;
	register int	i, j;

	if ((n = to - from) <= 1)
		return;
	for (j = 1; j <= n; j *= 2)
		;
	for (m = 2 * j - 1; m /= 2; )
	{
		k = n - m;
		for (j = 0; j < k; j++)
		{
			for (i = j; i >= 0; i -= m)
			{
				register char **fromi;

				fromi = &from[i];
				if (cf(fromi[m], fromi[0]) > 0)
				{
					break;
				}
				else
				{
					char *s;

					s = fromi[m];
					fromi[m] = fromi[0];
					fromi[0] = s;
				}
			}
		}
	}
}

/*
 * Argument list generation
 */
getarg(ac)
struct comnod	*ac;
{
	register struct argnod	*argp;
	register int		count = 0;
	register struct comnod	*c;

	if (c = ac)
	{
		argp = c->comarg;
		while (argp)
		{
			count += split(macro(argp->argval));
			argp = argp->argnxt;
		}
	}
	return(count);
}

static int
split(register char *s)		/* blank interpretation routine */
{
	register char	*argp;
	register int	c;
	int		count = 0;

	for (;;)
	{
		sigchk();
		argp = locstak() + BYTESPERWORD;
		while ((c = *s++, !any(c, ifsnod.namval) && c))
			*argp++ = c;
		if (argp == staktop + BYTESPERWORD)
		{
			if (c)
			{
				continue;
			}
			else
			{
				return(count);
			}
		}
		else if (c == 0)
			s--;
		/*
		 * file name generation
		 */

		argp = endstak(argp);

		if ((flags & nofngflg) == 0 && 
			(c = expand(((struct argnod *)argp)->argval, 0)))
			count += c;
		else
		{
			makearg(argp);
			count++;
		}
		gchain = (struct argnod *)((int)gchain | ARGMK);
	}
}

#ifdef ACCT
#include	<sys/types.h>
#include	"acctdef.h"
#include	<sys/acct.h>
#include 	<sys/times.h>

struct acct sabuf;
struct tms buffer;
extern long times();
static long before;
static int shaccton;	/* 0 implies do not write record on exit
			   1 implies write acct record on exit
			*/


/*
 *	suspend accounting until turned on by preacct()
 */

suspacct()
{
	shaccton = 0;
}

preacct(cmdadr)
	char *cmdadr;
{
	char *simple();

	if (acctnod.namval && *acctnod.namval)
	{
		sabuf.ac_btime = time((long *)0);
		before = times(&buffer);
		sabuf.ac_uid = getuid();
		sabuf.ac_gid = getgid();
		movstrn(simple(cmdadr), sabuf.ac_comm, sizeof(sabuf.ac_comm));
		shaccton = 1;
	}
}

#include	<fcntl.h>

doacct()
{
	int fd;
	long int after;

	if (shaccton)
	{
		after = times(&buffer);
		sabuf.ac_utime = compress(buffer.tms_utime + buffer.tms_cutime);
		sabuf.ac_stime = compress(buffer.tms_stime + buffer.tms_cstime);
		sabuf.ac_etime = compress(after - before);

		if ((fd = open(acctnod.namval, O_WRONLY | O_APPEND | O_CREAT, 0666)) != -1)
		{
			write(fd, &sabuf, sizeof(sabuf));
			close(fd);
		}
	}
}

/*
 *	Produce a pseudo-floating point representation
 *	with 3 bits base-8 exponent, 13 bits fraction
 */

compress(t)
	register time_t t;
{
	register exp = 0;
	register rund = 0;

	while (t >= 8192)
	{
		exp++;
		rund = t & 04;
		t >>= 3;
	}

	if (rund)
	{
		t++;
		if (t >= 8192)
		{
			t >>= 3;
			exp++;
		}
	}

	return((exp << 13) + t);
}
#endif

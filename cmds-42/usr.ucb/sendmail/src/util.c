/*
**  Sendmail
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1983 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
*/

#ifndef lint
static char	SccsId[] = "@(#)util.c	5.8 (Berkeley) 12/17/85";
#endif not lint

# include <stdio.h>
#ifdef NeXT_MOD
# include <mach.h>
# include <sys/table.h>
#undef TRUE
#undef FALSE
#endif
# include <sys/types.h>
# include <sys/stat.h>
# include <sysexits.h>
# include <errno.h>
# include <ctype.h>
# include "sendmail.h"

#define YES	1
#define NO	0

static readtimeout();

/*
**  STRIPQUOTES -- Strip quotes & quote bits from a string.
**
**	Runs through a string and strips off unquoted quote
**	characters and quote bits.  This is done in place.
**
**	Parameters:
**		s -- the string to strip.
**		qf -- if set, remove actual `` " '' characters
**			as well as the quote bits.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Called By:
**		deliver
*/

stripquotes(s, qf)
	char *s;
	bool qf;
{
	register char *p;
	register char *q;
	register char c;

	if (s == NULL)
		return;

	for (p = q = s; (c = *p++) != '\0'; )
	{
		if (c != '"' || !qf)
			*q++ = c & 0177;
	}
	*q = '\0';
}
/*
**  QSTRLEN -- give me the string length assuming 0200 bits add a char
**
**	Parameters:
**		s -- the string to measure.
**
**	Reurns:
**		The length of s, including space for backslash escapes.
**
**	Side Effects:
**		none.
*/

qstrlen(s)
	register char *s;
{
	register int l = 0;
	register char c;

	while ((c = *s++) != '\0')
	{
		if (bitset(0200, c))
			l++;
		l++;
	}
	return (l);
}
/*
**  CAPITALIZE -- return a copy of a string, properly capitalized.
**
**	Parameters:
**		s -- the string to capitalize.
**
**	Returns:
**		a pointer to a properly capitalized string.
**
**	Side Effects:
**		none.
*/

char *
capitalize(s)
	register char *s;
{
	static char buf[50];
	register char *p;

	p = buf;

	for (;;)
	{
		while (!isalpha(*s) && *s != '\0')
			*p++ = *s++;
		if (*s == '\0')
			break;
		*p++ = toupper(*s++);
		while (isalpha(*s))
			*p++ = *s++;
	}

	*p = '\0';
	return (buf);
}
/*
**  XALLOC -- Allocate memory and bitch wildly on failure.
**
**	THIS IS A CLUDGE.  This should be made to give a proper
**	error -- but after all, what can we do?
**
**	Parameters:
**		sz -- size of area to allocate.
**
**	Returns:
**		pointer to data region.
**
**	Side Effects:
**		Memory is allocated.
*/

char *
xalloc(sz)
	register int sz;
{
	register char *p;
#if	NeXT
	p = calloc (1, sz);
#else
	extern char *malloc();

	p = malloc((unsigned) sz);
#endif	NeXT
	if (p == NULL)
	{
		syserr("Out of memory!!");
		abort();
		/* exit(EX_UNAVAILABLE); */
	}
	return (p);
}
/*
**  COPYPLIST -- copy list of pointers.
**
**	This routine is the equivalent of newstr for lists of
**	pointers.
**
**	Parameters:
**		list -- list of pointers to copy.
**			Must be NULL terminated.
**		copycont -- if TRUE, copy the contents of the vector
**			(which must be a string) also.
**
**	Returns:
**		a copy of 'list'.
**
**	Side Effects:
**		none.
*/

char **
copyplist(list, copycont)
	char **list;
	bool copycont;
{
	register char **vp;
	register char **newvp;

	for (vp = list; *vp != NULL; vp++)
		continue;

	vp++;

	newvp = (char **) xalloc((int) (vp - list) * sizeof *vp);
	bcopy((char *) list, (char *) newvp, (int) (vp - list) * sizeof *vp);

	if (copycont)
	{
		for (vp = newvp; *vp != NULL; vp++)
			*vp = newstr(*vp);
	}

	return (newvp);
}
/*
**  PRINTAV -- print argument vector.
**
**	Parameters:
**		av -- argument vector.
**
**	Returns:
**		none.
**
**	Side Effects:
**		prints av.
*/

printav(av)
	register char **av;
{
	while (av && *av != NULL)
	{
		if (tTd(0, 44))
			printf("\n\t%08x=", *av);
		else
			(void) putchar(' ');
		xputs(*av++);
	}
	(void) putchar('\n');
}
/*
**  LOWER -- turn letter into lower case.
**
**	Parameters:
**		c -- character to turn into lower case.
**
**	Returns:
**		c, in lower case.
**
**	Side Effects:
**		none.
*/

char
lower(c)
	register char c;
{
	if (isascii(c) && isupper(c))
		c = c - 'A' + 'a';
	return (c);
}
/*
**  XPUTS -- put string doing control escapes.
**
**	Parameters:
**		s -- string to put.
**
**	Returns:
**		none.
**
**	Side Effects:
**		output to stdout
*/

xputs(s)
	register char *s;
{
	register char c;

	if (s == NULL)
	{
		printf("<null>");
		return;
	}
	(void) putchar('"');
	while ((c = *s++) != '\0')
	{
		if (!isascii(c))
		{
			(void) putchar('\\');
			c &= 0177;
		}
		if (c < 040 || c >= 0177)
		{
			(void) putchar('^');
			c ^= 0100;
		}
		(void) putchar(c);
	}
	(void) putchar('"');
	(void) fflush(stdout);
}
/*
**  MAKELOWER -- Translate a line into lower case
**
**	Parameters:
**		p -- the string to translate.  If NULL, return is
**			immediate.
**
**	Returns:
**		none.
**
**	Side Effects:
**		String pointed to by p is translated to lower case.
**
**	Called By:
**		parse
*/

makelower(p)
	register char *p;
{
	register char c;

	if (p == NULL)
		return;
	for (; (c = *p) != '\0'; p++)
		if (isascii(c) && isupper(c))
			*p = c - 'A' + 'a';
}
/*
**  SAMEWORD -- return TRUE if the words are the same
**
**	Ignores case.
**
**	Parameters:
**		a, b -- the words to compare.
**
**	Returns:
**		TRUE if a & b match exactly (modulo case)
**		FALSE otherwise.
**
**	Side Effects:
**		none.
*/

bool
sameword(a, b)
	register char *a, *b;
{
	char ca, cb;

	do
	{
		ca = *a++;
		cb = *b++;
		if (isascii(ca) && isupper(ca))
			ca = ca - 'A' + 'a';
		if (isascii(cb) && isupper(cb))
			cb = cb - 'A' + 'a';
	} while (ca != '\0' && ca == cb);
	return (ca == cb);
}
/*
**  BUILDFNAME -- build full name from gecos style entry.
**
**	This routine interprets the strange entry that would appear
**	in the GECOS field of the password file.
**
**	Parameters:
**		p -- name to build.
**		login -- the login name of this user (for &).
**		buf -- place to put the result.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

buildfname(p, login, buf)
	register char *p;
	char *login;
	char *buf;
{
	register char *bp = buf;

	if (*p == '*')
		p++;
	while (*p != '\0' && *p != ',' && *p != ';' && *p != '%')
	{
		if (*p == '&')
		{
			(void) strcpy(bp, login);
			*bp = toupper(*bp);
			while (*bp != '\0')
				bp++;
			p++;
		}
		else
			*bp++ = *p++;
	}
	*bp = '\0';
}
/*
**  SAFEFILE -- return true if a file exists and is safe for a user.
**
**	Parameters:
**		fn -- filename to check.
**		uid -- uid to compare against.
**		mode -- mode bits that must match.
**
**	Returns:
**		TRUE if fn exists, is owned by uid, and matches mode.
**		FALSE otherwise.
**
**	Side Effects:
**		none.
*/

bool
safefile(fn, uid, mode)
	char *fn;
	int uid;
	int mode;
{
	struct stat stbuf;

	if (stat(fn, &stbuf) >= 0 && stbuf.st_uid == uid &&
	    (stbuf.st_mode & mode) == mode)
		return (TRUE);
	errno = 0;
	return (FALSE);
}
/*
**  FIXCRLF -- fix <CR><LF> in line.
**
**	Looks for the <CR><LF> combination and turns it into the
**	UNIX canonical <NL> character.  It only takes one line,
**	i.e., it is assumed that the first <NL> found is the end
**	of the line.
**
**	Parameters:
**		line -- the line to fix.
**		stripnl -- if true, strip the newline also.
**
**	Returns:
**		none.
**
**	Side Effects:
**		line is changed in place.
*/

fixcrlf(line, stripnl)
	char *line;
	bool stripnl;
{
	register char *p;

	p = index(line, '\n');
	if (p == NULL)
		return;
	if (p[-1] == '\r')
		p--;
	if (!stripnl)
		*p++ = '\n';
	*p = '\0';
}
/*
**  DFOPEN -- determined file open
**
**	This routine has the semantics of fopen, except that it will
**	keep trying a few times to make this happen.  The idea is that
**	on very loaded systems, we may run out of resources (inodes,
**	whatever), so this tries to get around it.
*/

FILE *
dfopen(filename, mode)
	char *filename;
	char *mode;
{
	register int tries;
	register FILE *fp;

	for (tries = 0; tries < 10; tries++)
	{
#ifdef NeXT_MOD
		sendmail_sleep((unsigned) (10 * tries));
#else
		sleep((unsigned) (10 * tries));
#endif NeXT_MOD
		errno = 0;
		fp = fopen(filename, mode);
		if (fp != NULL)
			break;
		if (errno != ENFILE && errno != EINTR)
			break;
	}
	errno = 0;
	return (fp);
}
/*
**  PUTLINE -- put a line like fputs obeying SMTP conventions
**
**	This routine always guarantees outputing a newline (or CRLF,
**	as appropriate) at the end of the string.
**
**	Parameters:
**		l -- line to put.
**		fp -- file to put it onto.
**		m -- the mailer used to control output.
**
**	Returns:
**		none
**
**	Side Effects:
**		output of l to fp.
*/

# define SMTPLINELIM	990	/* maximum line length */

putline(l, fp, m)
	register char *l;
	FILE *fp;
	MAILER *m;
{
	register char *p;
	char svchar;
	register int linelen;

	/* strip out 0200 bits -- these can look like TELNET protocol */
	if (bitnset(M_LIMITS, m->m_flags))
	{
		p = l;
		while ((*p++ &= ~0200) != 0)
			continue;
	}

	do
	{
		/* find the end of the line */
		p = index(l, '\n');
		if (p == NULL)
			p = &l[strlen(l)];
		
		/* check for line overflow */
		while ((p - l) > SMTPLINELIM && bitnset(M_LIMITS, m->m_flags))
		{
			register char *q = &l[SMTPLINELIM - 1];
		
			svchar = *q;
			*q = '\0';
			if (l[0] == '.' && bitnset(M_XDOT, m->m_flags))
				(void) putc('.', fp);
			fputs(l, fp);
			(void) putc('!', fp);
			fputs(m->m_eol, fp);
			*q = svchar;
			l = q;
		}
		
		/* output last part */
#ifdef	__STDC__
		/*
		 * Literal strings aren't writable.
		 */
		linelen = p - l;
#if	NeXT_MOD
		/*
		 * Remove redundant CR characters from the end of lines
		 */
		while (linelen != 0 && l[linelen-1] == '\r')
			linelen--;
#endif	NeXT_MOD
		if (l[0] == '.' && bitnset(M_XDOT, m->m_flags))
			(void) putc('.', fp);
		fwrite(l, sizeof(char), linelen, fp);
		fputs(m->m_eol, fp);
#else	__STDC__
		svchar = *p;
		*p = '\0';
#if	NeXT_MOD
		  /*
		   * Remove redundant CR characters from the end of lines
		   */
		while (p != l && p[-1] == '\r')
			*--p = '\0';
#endif	NeXT_MOD
		if (l[0] == '.' && bitnset(M_XDOT, m->m_flags))
			(void) putc('.', fp);
		fputs(l, fp);
		fputs(m->m_eol, fp);
		*p = svchar;
#endif	__STDC__
		l = p;
		if (*l == '\n')
			l++;
	} while (l[0] != '\0');
}
/*
**  XUNLINK -- unlink a file, doing logging as appropriate.
**
**	Parameters:
**		f -- name of file to unlink.
**
**	Returns:
**		none.
**
**	Side Effects:
**		f is unlinked.
*/

xunlink(f)
	char *f;
{
	register int i;

# ifdef LOG
	if (LogLevel > 20)
		syslog(LOG_DEBUG, "%s: unlink %s\n", CurEnv->e_id, f);
# endif LOG

	i = unlink(f);
# ifdef LOG
	if (i < 0 && LogLevel > 21)
		syslog(LOG_DEBUG, "%s: unlink-fail %d", f, errno);
# endif LOG
}
/*
**  SFGETS -- "safe" fgets -- times out and ignores random interrupts.
**
**	Parameters:
**		buf -- place to put the input line.
**		siz -- size of buf.
**		fp -- file to read from.
**
**	Returns:
**		NULL on error (including timeout).  This will also leave
**			buf containing a null string.
**		buf otherwise.
**
**	Side Effects:
**		none. (not NeXT_MOD)
**		Errno is set if an error occurs. (NeXT_MOD)
*/

static jmp_buf	CtxReadTimeout;

#ifndef ETIMEDOUT
#define ETIMEDOUT	EINTR
#endif

char *
sfgets(buf, siz, fp)
	char *buf;
	int siz;
	FILE *fp;
{
	register EVENT *ev = NULL;
	register char *p;
	extern readtimeout();
#if	NeXT_MOD
	extern int errno;
#endif	NeXT_MOD

	/* set the timeout */
	if (ReadTimeout != 0)
	{
		if (setjmp(CtxReadTimeout) != 0)
		{
#if	NeXT_MOD
			extern char *RealHostName;

			errno = ETIMEDOUT;
			if (RealHostName) 
				syserr("net hang reading from %s",
					RealHostName);
			else
				syserr("input hang");
			errno = ETIMEDOUT; /* syserr() resets errno */
#else	NeXT_MOD
			errno = ETIMEDOUT;
			syserr("net timeout");
#endif	NeXT_MOD
			buf[0] = '\0';
			return (NULL);
		}
		ev = setevent((time_t) ReadTimeout, readtimeout, 0);
	}

	/* try to read */
	p = NULL;
	while (p == NULL && !feof(fp) && !ferror(fp))
	{
		errno = 0;
		p = fgets(buf, siz, fp);
		if (errno == EINTR)
			clearerr(fp);
	}

	/* clear the event if it has not sprung */
	clrevent(ev);

	/* clean up the books and exit */
	LineNumber++;
	if (p == NULL)
	{
		buf[0] = '\0';
		return (NULL);
	}
	for (p = buf; *p != '\0'; p++)
		*p &= ~0200;
	return (buf);
}

static
readtimeout()
{
	longjmp(CtxReadTimeout, 1);
}
/*
**  FGETFOLDED -- like fgets, but know about folded lines.
**
**	Parameters:
**		buf -- place to put result.
**		n -- bytes available.
**		f -- file to read from.
**
**	Returns:
**		buf on success, NULL on error or EOF.
**
**	Side Effects:
**		buf gets lines from f, with continuation lines (lines
**		with leading white space) appended.  CRLF's are mapped
**		into single newlines.  Any trailing NL is stripped.
*/

char *
fgetfolded(buf, n, f)
	char *buf;
	register int n;
	FILE *f;
{
	register char *p = buf;
	register int i;

	n--;
	while ((i = getc(f)) != EOF)
	{
		if (i == '\r')
		{
			i = getc(f);
			if (i != '\n')
			{
				if (i != EOF)
					(void) ungetc(i, f);
				i = '\r';
			}
		}
		if (--n > 0)
			*p++ = i;
		if (i == '\n')
		{
			LineNumber++;
			i = getc(f);
			if (i != EOF)
				(void) ungetc(i, f);
			if (i != ' ' && i != '\t')
			{
				*--p = '\0';
				return (buf);
			}
		}
	}
	return (NULL);
}
/*
**  CURTIME -- return current time.
**
**	Parameters:
**		none.
**
**	Returns:
**		the current time.
**
**	Side Effects:
**		none.
*/

time_t
curtime()
{
	auto time_t t;

	(void) time(&t);
	return (t);
}
/*
**  ATOBOOL -- convert a string representation to boolean.
**
**	Defaults to "TRUE"
**
**	Parameters:
**		s -- string to convert.  Takes "tTyY" as true,
**			others as false.
**
**	Returns:
**		A boolean representation of the string.
**
**	Side Effects:
**		none.
*/

bool
atobool(s)
	register char *s;
{
	if (*s == '\0' || index("tTyY", *s) != NULL)
		return (TRUE);
	return (FALSE);
}
/*
**  ATOOCT -- convert a string representation to octal.
**
**	Parameters:
**		s -- string to convert.
**
**	Returns:
**		An integer representing the string interpreted as an
**		octal number.
**
**	Side Effects:
**		none.
*/

atooct(s)
	register char *s;
{
	register int i = 0;

	while (*s >= '0' && *s <= '7')
		i = (i << 3) | (*s++ - '0');
	return (i);
}
/*
**  WAITFOR -- wait for a particular process id.
**
**	Parameters:
**		pid -- process id to wait for.
**
**	Returns:
**		status of pid.
**		-1 if pid never shows up.
**
**	Side Effects:
**		none.
*/

waitfor(pid)
	int pid;
{
	auto int st;
	int i;

	do
	{
		errno = 0;
		i = wait(&st);
	} while ((i >= 0 || errno == EINTR) && i != pid);
	if (i < 0)
		st = -1;
	return (st);
}
/*
**  BITINTERSECT -- tell if two bitmaps intersect
**
**	Parameters:
**		a, b -- the bitmaps in question
**
**	Returns:
**		TRUE if they have a non-null intersection
**		FALSE otherwise
**
**	Side Effects:
**		none.
*/

bool
bitintersect(a, b)
	BITMAP a;
	BITMAP b;
{
	int i;

	for (i = BITMAPBYTES / sizeof (int); --i >= 0; )
		if ((a[i] & b[i]) != 0)
			return (TRUE);
	return (FALSE);
}
/*
**  BITZEROP -- tell if a bitmap is all zero
**
**	Parameters:
**		map -- the bit map to check
**
**	Returns:
**		TRUE if map is all zero.
**		FALSE if there are any bits set in map.
**
**	Side Effects:
**		none.
*/

bool
bitzerop(map)
	BITMAP map;
{
	int i;

	for (i = BITMAPBYTES / sizeof (int); --i >= 0; )
		if (map[i] != 0)
			return (FALSE);
	return (TRUE);
}

#ifdef NeXT_MOD
/*
** check_system_proc_limit - Garth Snyder Sep 6, 1990
**
** This is a hack which will prevent sendmail from forking so many procs
** that the kernel panics.  It checks to see if the number of
** processes running on the system is greater than SystemProcLimit,
** and does not return until that is no longer true.
**
** Currently, this routine is only called before accepting an 
** internet connection.  Forked children may run wild to their
** hearts' content.  This is not optimal, but sendmail has 
** forks and waits in so many places that it is difficult to make
** this check each time a fork is done (because deadlocks can 
** occur when processes wait on each other to exit before continuing).
**
** By default, SystemProcLimit is 0 and this check is disabled. The
** value may be set with the option code p in the sendmail.cf file
** or on the sendmail command line.
**
** The process-counting code is mostly stolen from ps.  There's 
** probably a better way to do this...
*/

#define MACH_20

check_system_proc_limit()

{
#ifdef MACH_20
    processor_set_name_t  *psets;
    kern_return_t	  ret;
    processor_set_t	  p;
    task_t		  *tasks;
    unsigned int	  pcount,tcount;
    int			  i,j,total_proc;
    static unsigned char  first_time = YES;
    static host_priv_t	  host;
#else MACH_20
    register int i,j;
    long	nproc;
#define    NPROC    16        /* I don't know why, I just copy code -kh*/
    struct tbl_procinfo proc[NPROC];
    struct tbl_procinfo *mproc;
    int total_proc;
#endif

    if (SystemProcLimit == 0)		/* Proc counting disabled */
    {
	return(0);
    }

#ifdef MACH_20
    if (first_time == YES)
    {
    	first_time = NO;
	host = host_priv_self();
    	if (host == PORT_NULL) {
	    fprintf(stderr,"Insufficient privileges.\n");
	    exit(0);
    	}
    }
    
    while (1) {
	ret = host_processor_sets(host, &psets, &pcount);
	if (ret != KERN_SUCCESS) {
		mach_error("host_processor_sets", ret);
		exit(0);
	}
	
	total_proc = 0;
	
	for (i = 0; i < pcount; i++) {
		ret = host_processor_set_priv(host, psets[i], &p);
		if (ret != KERN_SUCCESS) {
			/* Compatibility hack */
			p = psets[i];
		}
		
		ret = processor_set_tasks(p, &tasks, &tcount);
		if (ret != KERN_SUCCESS) {
			mach_error("processor_set_tasks", ret);
			exit(0);       
		}
		total_proc += tcount;
		
		/* Relinquish all the task ports we just got. */

		for (j = 0; j < tcount; j++)
		{
			if (tasks[j] != task_self())
			{
				ret = port_deallocate(task_self(), tasks[j]);
				if (ret != KERN_SUCCESS)
				{
					/* Do nothing, assume the task has died. */
					/* mach_error("port_deallocate",ret); */
					/* exit(1); */
				}
			}
		}
		
		ret = vm_deallocate(task_self(), (vm_address_t) tasks,
			sizeof(task_t *)*tcount);
		if (ret != KERN_SUCCESS)
		{
			mach_error("vm_deallocate",ret);
			exit(1);
		}

		/* And relinquish the processor set for those tasks. */
		
		if (port_deallocate(task_self(),psets[i]) != 
			KERN_SUCCESS)
		{
			mach_error("port_deallocate",ret);
			exit(1);
		} 
	}
	
	/* Now release the array used to store processor sets. */

	ret = vm_deallocate(task_self(), (vm_address_t) psets,
		sizeof(int *)*pcount);
	if (ret != KERN_SUCCESS)
	{
		mach_error("vm_deallocate",ret);
		exit(1);
	}

	/* Now total_proc holds the number of processes. */
	
# ifdef DEBUG
	if (tTd(15, 1))
		printf("Process count %s: %d processes\n",
			(total_proc >= SystemProcLimit) ? "FAILED" :
			"PASSED",total_proc);
# endif DEBUG

	if (total_proc < SystemProcLimit)
	{
	    break;
	}
	
	sendmail_sleep(5);
    }
#else MACH_20
    nproc = table(TBL_PROCINFO, 0, (char *)0, 32767, 0);

    while (1) {
	total_proc = 0;
	for (i=0; i < nproc; i += NPROC) {
	    j = nproc - i;
	    if (j > NPROC)
		j = NPROC;
	    j = table(TBL_PROCINFO, i, (char *)proc, NPROC, sizeof(proc[0]));
	    for (j = j - 1; j >= 0; j--) {
    
		mproc = &proc[j];
    
		if ((mproc->pi_status == PI_EMPTY) || (mproc->pi_pid == 0))
		    continue;
#undef	NPROC  
		total_proc++;
	    }
	}

	/* Now total_proc holds the number of processes. */
	
# ifdef DEBUG
	if (tTd(15, 1))
		printf("Process count %s: %d processes\n",
			(total_proc >= SystemProcLimit) ? "FAILED" :
			"PASSED",total_proc);
# endif DEBUG

	if (total_proc < SystemProcLimit)
	{
	    break;
	}
	
	sendmail_sleep(5);
    }
#endif MACH_20
}
#endif

/*
**  Sendmail
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1983 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
*/


# include "sendmail.h"
# include <pwd.h>
# include <sys/stat.h>
# include <sys/dir.h>
# include <signal.h>
# include <errno.h>

# ifndef QUEUE
# ifndef lint
static char	SccsId[] = "@(#)queue.c	5.21 (Berkeley) 4/17/86	(no queueing)";
# endif not lint
# else QUEUE

# ifndef lint
static char	SccsId[] = "@(#)queue.c	5.21 (Berkeley) 4/17/86";
# endif not lint

/*
**  Work queue.
*/

struct work
{
	char		*w_name;	/* name of control file */
	long		w_pri;		/* priority of message, see below */
	time_t		w_ctime;	/* creation time of message */
	struct work	*w_next;	/* next in queue */
};

typedef struct work	WORK;

WORK	*WorkQ;			/* queue of things to be done */

#if	NeXT_MOD
static dowork();
static readqf();
#endif	NeXT_MOD
/*
**  QUEUEUP -- queue a message up for future transmission.
**
**	Parameters:
**		e -- the envelope to queue up.
**		queueall -- if TRUE, queue all addresses, rather than
**			just those with the QQUEUEUP flag set.
**		announce -- if TRUE, tell when you are queueing up.
**
**	Returns:
**		none.
**
**	Side Effects:
**		The current request are saved in a control file.
*/

queueup(e, queueall, announce)
	register ENVELOPE *e;
	bool queueall;
	bool announce;
{
	char *tf;
	char *qf;
	char buf[MAXLINE];
	register FILE *tfp;
	register HDR *h;
	register ADDRESS *q;
	MAILER nullmailer;

	/*
	**  Create control file.
	*/

	tf = newstr(queuename(e, 't'));
	tfp = fopen(tf, "w");
	if (tfp == NULL)
	{
		syserr("queueup: cannot create temp file %s", tf);
		return;
	}
	(void) chmod(tf, FileMode);

# ifdef DEBUG
	if (tTd(40, 1))
		printf("queueing %s\n", e->e_id);
# endif DEBUG

	/*
	**  If there is no data file yet, create one.
	*/

	if (e->e_df == NULL)
	{
		register FILE *dfp;
		extern putbody();

		e->e_df = newstr(queuename(e, 'd'));
		dfp = fopen(e->e_df, "w");
		if (dfp == NULL)
		{
			syserr("queueup: cannot create %s", e->e_df);
			(void) fclose(tfp);
			return;
		}
		(void) chmod(e->e_df, FileMode);
		(*e->e_putbody)(dfp, ProgMailer, e);
		(void) fclose(dfp);
		e->e_putbody = putbody;
	}

	/*
	**  Output future work requests.
	**	Priority and creation time should be first, since
	**	they are required by orderq.
	*/

	/* output message priority */
	fprintf(tfp, "P%ld\n", e->e_msgpriority);

	/* output creation time */
	fprintf(tfp, "T%ld\n", e->e_ctime);

	/* output name of data file */
	fprintf(tfp, "D%s\n", e->e_df);

	/* message from envelope, if it exists */
	if (e->e_message != NULL)
		fprintf(tfp, "M%s\n", e->e_message);

	/* output name of sender */
	fprintf(tfp, "S%s\n", e->e_from.q_paddr);

#if	NeXT_MOD
#else	NeXT_MOD
	/* output list of recipient addresses */
	for (q = e->e_sendqueue; q != NULL; q = q->q_next)
	{
		if (queueall ? !bitset(QDONTSEND, q->q_flags) :
			       bitset(QQUEUEUP, q->q_flags))
		{
			char *ctluser, *getctluser();
			bool badctladdr();
			if (badctladdr(q))	/* sanity/safety */
				continue;
			if ((ctluser = getctluser(q)) == NULL)
				fprintf(tfp, "R%s\n", q->q_paddr);
			else
				fprintf(tfp, "R(%s) %s\n", ctluser, q->q_paddr);
			if (announce)
			{
				e->e_to = q->q_paddr;
				message(Arpa_Info, "queued");
				if (LogLevel > 4)
					logdelivery("queued");
				e->e_to = NULL;
			}
#ifdef DEBUG
			if (tTd(40, 1))
			{
				printf("queueing ");
				printaddr(q, FALSE);
			}
#endif DEBUG
		}
	}
#endif	NeXT_MOD

	/* output list of error recipients */
	for (q = e->e_errorqueue; q != NULL; q = q->q_next)
	{
		if (!bitset(QDONTSEND, q->q_flags))
		{
			char *ctluser, *getctluser();
			bool badctladdr();
			if (badctladdr(q))	/* sanity/safety */
				continue;
			if ((ctluser = getctluser(q)) == NULL)
				fprintf(tfp, "E%s\n", q->q_paddr);
			else
				fprintf(tfp, "E(%s) %s\n", ctluser, q->q_paddr);
		}
	}

	/*
	**  Output headers for this message.
	**	Expand macros completely here.  Queue run will deal with
	**	everything as absolute headers.
	**		All headers that must be relative to the recipient
	**		can be cracked later.
	**	We set up a "null mailer" -- i.e., a mailer that will have
	**	no effect on the addresses as they are output.
	*/

	bzero((char *) &nullmailer, sizeof nullmailer);
	nullmailer.m_r_rwset = nullmailer.m_s_rwset = -1;
	nullmailer.m_eol = "\n";
#if	NeXT_MOD
	nullmailer.m_argvsize = 10000;
#endif	NeXT_MOD

	define('g', "\001f", e);
	for (h = e->e_header; h != NULL; h = h->h_link)
	{
		extern bool bitzerop();

#if	NeXT_MOD
		if (bitset(H_ERRSTO,h->h_flags) && 
		    bitset(H_DEFAULT,h->h_flags))
		      errors_to(h, e);
#endif	NeXT_MOD
		/* don't output null headers */
		if (h->h_value == NULL || h->h_value[0] == '\0')
			continue;

		/* don't output resent headers on non-resent messages */
		if (bitset(H_RESENT, h->h_flags) && !bitset(EF_RESENT, e->e_flags))
			continue;

		/* output this header */
		fprintf(tfp, "H");

		/* if conditional, output the set of conditions */
		if (!bitzerop(h->h_mflags) && bitset(H_CHECK|H_ACHECK, h->h_flags))
		{
			int j;

			(void) putc('?', tfp);
			for (j = '\0'; j <= '\177'; j++)
				if (bitnset(j, h->h_mflags))
					(void) putc(j, tfp);
			(void) putc('?', tfp);
		}

		/* output the header: expand macros, convert addresses */
		if (bitset(H_DEFAULT, h->h_flags))
		{
			(void) expand(h->h_value, buf, &buf[sizeof buf], e);
			fprintf(tfp, "%s: %s\n", h->h_field, buf);
		}
		else if (bitset(H_FROM|H_RCPT, h->h_flags))
		{
			commaize(h, h->h_value, tfp, bitset(EF_OLDSTYLE, e->e_flags),
				 &nullmailer);
		}
		else
			fprintf(tfp, "%s: %s\n", h->h_field, h->h_value);
	}

#if	NeXT_MOD
	/* output list of recipient addresses */
	for (q = e->e_sendqueue; q != NULL; q = q->q_next)
	{
		if (queueall ? !bitset(QDONTSEND, q->q_flags) :
			       bitset(QQUEUEUP, q->q_flags))
		{
			fprintf(tfp, "R%s\n", q->q_paddr);
			if (announce)
			{
				e->e_to = q->q_paddr;
				message(Arpa_Info, "queued");
				if (LogLevel > 4)
					logdelivery("queued");
				e->e_to = NULL;
			}
#ifdef DEBUG
			if (tTd(40, 1))
			{
				printf("queueing ");
				printaddr(q, FALSE);
			}
#endif DEBUG
		}
	}
#endif	NeXT_MOD

	/*
	**  Clean up.
	*/

	(void) fclose(tfp);
	qf = queuename(e, 'q');
	if (tf != NULL)
	{
#if	NeXT_MOD
		holdsigs();
		if (rename(tf, qf) < 0) 
		{
			syserr("cannot rename(%s, %s), df=%s", tf, qf, e->e_df);
			(void) unlink(tf);
		}
		rlsesigs();
#else	NeXT_MOD
		(void) unlink(qf);
		if (rename(tf, qf) < 0)
			syserr("cannot unlink(%s, %s), df=%s", tf, qf, e->e_df);
		errno = 0;
#endif	NeXT_MOD
	}

# ifdef LOG
	/* save log info */
	if (LogLevel > 15)
		syslog(LOG_DEBUG, "%s: queueup, qf=%s, df=%s\n", e->e_id, qf, e->e_df);
# endif LOG
}

#if	NeXT_MOD
/*
**  checkpoint -- rewrite the qf file to reflect the updated queue of
**	recipients.  WARNING: This function assumes that the recipient
**	lines are all at the END of the queue files, after all other lines.
**		"others" points to the list of recipients that have NOT been
**			processed yet.  We save any already processed 
**			recipients that have been marked for queuing, plus
**			all the non-processed recipients except those that
**			have already been delivered to.
**/
checkpoint(e,others)
    ENVELOPE *e;
    ADDRESS *others;
  {
    register ADDRESS *to;
    register FILE *qfp;
    char buf[MAXFIELD];
    extern char *fgetfolded();
    long position;

    if (others==NULL) return;
    qfp = fopen(queuename(e,'q'),"r+");
    if (qfp==NULL) return;

    while (fgetfolded(buf, sizeof buf, qfp) != NULL)
      {
	if (buf[0]=='R') break;
        position = ftell(qfp);
      }
    if (feof(qfp) || ferror(qfp))
      {
        /*
	 * got an error reading the queue file - give up.
	 */
	 fclose(qfp);
	 return;
      }
    fseek(qfp,position,0);
    for (to = e->e_sendqueue; to != NULL && to != others; to = to->q_next)
	if (bitset(QQUEUEUP, to->q_flags) )
	        fprintf(qfp, "R%s\n", to->q_paddr);
    if (to != NULL)
        for (; to != NULL; to = to->q_next)
	    if (!bitset(QDONTSEND, to->q_flags) || 
	         bitset(QQUEUEUP, to->q_flags) )
		    fprintf(qfp, "R%s\n", to->q_paddr);
    ftruncate(fileno(qfp), ftell(qfp));
    fclose(qfp);  
    e->e_flags |= EF_INQUEUE;
  }
#endif	NeXT_MOD
/*
**  RUNQUEUE -- run the jobs in the queue.
**
**	Gets the stuff out of the queue in some presumably logical
**	order and processes them.
**
**	Parameters:
**		forkflag -- TRUE if the queue scanning should be done in
**			a child process.  We double-fork so it is not our
**			child and we don't have to clean up after it.
**
**	Returns:
**		none.
**
**	Side Effects:
**		runs things in the mail queue.
*/

runqueue(forkflag)
	bool forkflag;
{
	extern bool shouldqueue();

	/*
	**  If no work will ever be selected, don't even bother reading
	**  the queue.
	*/

	if (shouldqueue(-100000000L))
	{
		if (Verbose)
			printf("Skipping queue run -- load average too high\n");

		if (forkflag)
			return;
		finis();
	}

	/*
	**  See if we want to go off and do other useful work.
	*/

	if (forkflag)
	{
		int pid;

		pid = dofork();
		if (pid != 0)
		{
			extern reapchild();

			/* parent -- pick up intermediate zombie */
#ifndef SIGCHLD
			(void) waitfor(pid);
#else SIGCHLD
			(void) signal(SIGCHLD, reapchild);
#endif SIGCHLD
			if (QueueIntvl != 0)
				(void) setevent(QueueIntvl, runqueue, TRUE);
			return;
		}
		/* child -- double fork */
#ifndef SIGCHLD
		if (fork() != 0)
			exit(EX_OK);
#else SIGCHLD
		(void) signal(SIGCHLD, SIG_DFL);
#endif SIGCHLD
	}

	setproctitle("running queue");

# ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "runqueue %s, pid=%d", QueueDir, getpid());
# endif LOG

	/*
	**  Release any resources used by the daemon code.
	*/

# ifdef DAEMON
	clrdaemon();
# endif DAEMON

#if	NeXT_MOD
#else	NeXT_MOD
	/*
	**  Make sure the alias database is open.
	*/

	initaliases(AliasFile, FALSE);
#endif	NeXT_MOD

	/*
	**  Start making passes through the queue.
	**	First, read and sort the entire queue.
	**	Then, process the work in that order.
	**		But if you take too long, start over.
	*/

	/* order the existing work requests */
	(void) orderq(FALSE);

	/* process them once at a time */
	while (WorkQ != NULL)
	{
		WORK *w = WorkQ;

		WorkQ = WorkQ->w_next;
		dowork(w);
		free(w->w_name);
		free((char *) w);
	}
#if	NeXT_MOD
	/*
	 * Don't let dropenvelope() trash locked queue files!
	 */
	CurEnv->e_id = NULL;
#endif	NeXT_MOD
	finis();
}
/*
**  ORDERQ -- order the work queue.
**
**	Parameters:
**		doall -- if set, include everything in the queue (even
**			the jobs that cannot be run because the load
**			average is too high).  Otherwise, exclude those
**			jobs.
**
**	Returns:
**		The number of request in the queue (not necessarily
**		the number of requests in WorkQ however).
**
**	Side Effects:
**		Sets WorkQ to the queue of available work, in order.
*/

# define NEED_P		001
# define NEED_T		002
#if	NeXT_MOD
# define NEED_R		004
#endif	NeXT_MOD


orderq(doall)
	bool doall;
{
	register struct direct *d;
	register WORK *w;
	DIR *f;
	register int i;
	WORK wlist[QUEUESIZE+1];
	int wn = -1;
	extern workcmpf();
#if	NeXT_MOD
	extern int	OnlyRunId;		/* main.c */
	extern char	*OnlyRunRecip;		/* main.c */
#endif	NeXT_MOD

	/* clear out old WorkQ */
	for (w = WorkQ; w != NULL; )
	{
		register WORK *nw = w->w_next;

		WorkQ = nw;
		free(w->w_name);
		free((char *) w);
		w = nw;
	}

	/* open the queue directory */
	f = opendir(".");
	if (f == NULL)
	{
		syserr("orderq: cannot open \"%s\" as \".\"", QueueDir);
		return (0);
	}

	/*
	**  Read the work directory.
	*/

	while ((d = readdir(f)) != NULL)
	{
		FILE *cf;
		char lbuf[MAXNAME];

		/* is this an interesting entry? */
		if (d->d_name[0] != 'q' || d->d_name[1] != 'f')
			continue;

#if	NeXT_MOD
		/*
		** If we're only interested in a particular job, check
		** for that one.
		*/
		if (OnlyRunId) {
			if (OnlyRunId != atoi(&d->d_name[4])) {
				continue;
			}
		}
#endif	NeXT_MOD

		/* yes -- open control file (if not too many files) */
		if (++wn >= QUEUESIZE)
			continue;
		cf = fopen(d->d_name, "r");
		if (cf == NULL)
		{
			/* this may be some random person sending hir msgs */
			/* syserr("orderq: cannot open %s", cbuf); */
#ifdef DEBUG
			if (tTd(41, 2))
				printf("orderq: cannot open %s (%d)\n",
					d->d_name, errno);
#endif DEBUG
			errno = 0;
			wn--;
			continue;
		}
		w = &wlist[wn];
		w->w_name = newstr(d->d_name);

		/* make sure jobs in creation don't clog queue */
		w->w_pri = 0x7fffffff;
		w->w_ctime = 0;

		/* extract useful information */
		i = NEED_P | NEED_T;
#if	NeXT_MOD
		if (OnlyRunRecip != 0)
			i |= NEED_R;
#endif	NeXT_MOD
		while (i != 0 && fgets(lbuf, sizeof lbuf, cf) != NULL)
		{
			extern long atol();

			switch (lbuf[0])
			{
			  case 'P':
				w->w_pri = atol(&lbuf[1]);
				i &= ~NEED_P;
				break;

			  case 'T':
				w->w_ctime = atol(&lbuf[1]);
				i &= ~NEED_T;
				break;
#if	NeXT_MOD
			  case 'R':
			        if (i |= NEED_R)
			    {
				register char *sp;
				for (sp = &lbuf[1]; *sp; sp++)
				{
					if (*sp == OnlyRunRecip[0] && 
					    strncmp(sp, OnlyRunRecip,
						strlen(OnlyRunRecip)) == 0) {
							i &= ~NEED_R;
							break;
					}
				}
			    }
#endif	NeXT_MOD
			}
		}
		(void) fclose(cf);

#if	NeXT_MOD
		/* If recip name didn't match, don't take this queue entry */
		if ( (shouldqueue(wlist[wn].w_pri) && !doall) ||
		       (i & NEED_R)!= 0)
#else	NeXT_MOD
		if (!doall && shouldqueue(w->w_pri))
#endif	NeXT_MOD
		{
			/* don't even bother sorting this job in */
			wn--;
		}
	}
	(void) closedir(f);
	wn++;

	/*
	**  Sort the work directory.
	*/

	qsort((char *) wlist, min(wn, QUEUESIZE), sizeof *wlist, workcmpf);

	/*
	**  Convert the work list into canonical form.
	**	Should be turning it into a list of envelopes here perhaps.
	*/

	WorkQ = NULL;
	for (i = min(wn, QUEUESIZE); --i >= 0; )
	{
		w = (WORK *) xalloc(sizeof *w);
		w->w_name = wlist[i].w_name;
		w->w_pri = wlist[i].w_pri;
		w->w_ctime = wlist[i].w_ctime;
		w->w_next = WorkQ;
		WorkQ = w;
	}

# ifdef DEBUG
	if (tTd(40, 1))
	{
		for (w = WorkQ; w != NULL; w = w->w_next)
			printf("%32s: pri=%ld\n", w->w_name, w->w_pri);
	}
# endif DEBUG

	return (wn);
}
/*
**  WORKCMPF -- compare function for ordering work.
**
**	Parameters:
**		a -- the first argument.
**		b -- the second argument.
**
**	Returns:
**		-1 if a < b
**		 0 if a == b
**		+1 if a > b
**
**	Side Effects:
**		none.
*/

workcmpf(a, b)
	register WORK *a;
	register WORK *b;
{
	long pa = a->w_pri + a->w_ctime;
	long pb = b->w_pri + b->w_ctime;

	if (pa == pb)
		return (0);
	else if (pa > pb)
		return (1);
	else
		return (-1);
}
/*
**  DOWORK -- do a work request.
**
**	Parameters:
**		w -- the work request to be satisfied.
**
**	Returns:
**		none.
**
**	Side Effects:
**		The work request is satisfied if possible.
*/

static
dowork(w)
	register WORK *w;
{
#if	NeXT_MOD
	ENVELOPE *newenvelope();
	extern ENVELOPE BlankEnvelope;
#else	NeXT_MOD
	register int i;
	extern bool shouldqueue();
#endif	NeXT_MO

# ifdef DEBUG
	if (tTd(40, 1))
		printf("dowork: %s pri %ld\n", w->w_name, w->w_pri);
# endif DEBUG

	/*
	**  Ignore jobs that are too expensive for the moment.
	*/

	if (shouldqueue(w->w_pri))
	{
		if (Verbose)
			printf("\nSkipping %s\n", w->w_name + 2);
		return;
	}

#if	NeXT_MOD
#else	NeXT_MOD
	/*
	**  Fork for work.
	*/

	if (ForkQueueRuns)
	{
		i = fork();
		if (i < 0)
		{
			syserr("dowork: cannot fork");
			return;
		}
	}
	else
	{
		i = 0;
	}

	if (i == 0)
	{
#endif	NeXT_MOD
		/*
		**  CHILD except for NeXT_NFS which in order to use the
		**	cache, delays forking here.
		**
		**	Lock the control file to avoid duplicate deliveries.
		**		Then run the file as though we had just read it.
		**	We save an idea of the temporary name so we
		**		can recover on interrupt.
		*/

		/* set basic modes, etc. */
		(void) alarm(0);
#if	NeXT_MOD
		closexscript(CurEnv);
		CurEnv = newenvelope(CurEnv);
		CurEnv->e_flags = BlankEnvelope.e_flags;
#else	NeXT_MOD
		clearenvelope(CurEnv, FALSE);
#endif	NeXT_MOD
		QueueRun = TRUE;
		ErrorMode = EM_MAIL;
		CurEnv->e_id = &w->w_name[2];

#if	NeXT_MOD
		setproctitle(CurEnv->e_id, 0);	/* Set process name for ps */
#endif	NeXT_MOD

# ifdef LOG
		if (LogLevel > 11)
			syslog(LOG_DEBUG, "%s: dowork, pid=%d", CurEnv->e_id,
			       getpid());
# endif LOG

		/* don't use the headers from sendmail.cf... */
		CurEnv->e_header = NULL;

		/* lock the control file during processing */
		if (link(w->w_name, queuename(CurEnv, 'l')) < 0)
		{
			/* being processed by another queuer */
# ifdef LOG
			if (LogLevel > 4)
				syslog(LOG_DEBUG, "%s: locked", CurEnv->e_id);
# endif LOG
#if	NeXT_MOD
#else	NeXT_MOD
			if (ForkQueueRuns)
				exit(EX_OK);
			else
#endif	NeXT_MOD
				return;
		}

		/* do basic system initialization */
		initsys();

		/* read the queue control file */
		readqf(CurEnv, TRUE);
		CurEnv->e_flags |= EF_INQUEUE;
		eatheader(CurEnv);

		/* do the delivery */
		if (!bitset(EF_FATALERRS, CurEnv->e_flags))
			sendall(CurEnv, SM_DELIVER);

#if	NeXT_MOD
	dropenvelope(CurEnv);
#else	NeXT_MOD
		/* finish up and exit */
		if (ForkQueueRuns)
			finis();
		else
			dropenvelope(CurEnv);
	}
	else
	{
		/*
		**  Parent -- pick up results.
		*/

		errno = 0;
		(void) waitfor(i);
	}
#endif	NeXT_NFS
}
/*
**  READQF -- read queue file and set up environment.
**
**	Parameters:
**		e -- the envelope of the job to run.
**		full -- if set, read in all information.  Otherwise just
**			read in info needed for a queue print.
**
**	Returns:
**		none.
**
**	Side Effects:
**		cf is read and created as the current job, as though
**		we had been invoked by argument.
*/

static
readqf(e, full)
	register ENVELOPE *e;
	bool full;
{
	char *qf;
	register FILE *qfp;
	char buf[MAXFIELD];
	extern char *fgetfolded(), *setctluser();
	extern long atol();

	/*
	**  Read and process the file.
	*/

	qf = queuename(e, 'q');
	qfp = fopen(qf, "r");
	if (qfp == NULL)
	{
		syserr("readqf: no control file %s", qf);
		return;
	}
	FileName = qf;
	LineNumber = 0;
	if (Verbose && full)
		printf("\nRunning %s\n", e->e_id);
	while (fgetfolded(buf, sizeof buf, qfp) != NULL)
	{
# ifdef DEBUG
		if (tTd(40, 4))
			printf("+++++ %s\n", buf);
# endif DEBUG
		switch (buf[0])
		{
		  case 'R':		/* specify recipient */
			sendtolist(setctluser(&buf[1]), (ADDRESS *) NULL,
			           &e->e_sendqueue);
			clrctluser();
			break;

		  case 'E':		/* specify error recipient */
			sendtolist(setctluser(&buf[1]), (ADDRESS *) NULL,
			           &e->e_errorqueue);
			clrctluser();
			break;

		  case 'H':		/* header */
			if (full)
				(void) chompheader(&buf[1], FALSE);
			break;

		  case 'M':		/* message */
			e->e_message = newstr(&buf[1]);
			break;

		  case 'S':		/* sender */
			setsender(newstr(&buf[1]));
			break;

		  case 'D':		/* data file name */
			if (!full)
				break;
			e->e_df = newstr(&buf[1]);
			e->e_dfp = fopen(e->e_df, "r");
			if (e->e_dfp == NULL)
				syserr("readqf: cannot open %s", e->e_df);
			break;

		  case 'T':		/* init time */
			e->e_ctime = atol(&buf[1]);
			break;

		  case 'P':		/* message priority */
			e->e_msgpriority = atol(&buf[1]) + WkTimeFact;
			break;

		  case '\0':		/* blank line; ignore */
			break;

		  default:
			syserr("readqf(%s:%d): bad line \"%s\"", e->e_id,
				LineNumber, buf);
			break;
		}
	}

	(void) fclose(qfp);
	FileName = NULL;

	/*
	**  If we haven't read any lines, this queue file is empty.
	**  Arrange to remove it without referencing any null pointers.
	*/

	if (LineNumber == 0)
	{
		errno = 0;
		e->e_flags |= EF_CLRQUEUE | EF_FATALERRS | EF_RESPONSE;
	}
}
/*
**  PRINTQUEUE -- print out a representation of the mail queue
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Prints a listing of the mail queue on the standard output.
*/

printqueue()
{
	register WORK *w;
	FILE *f;
	int nrequests;
	char buf[MAXLINE];

	/*
	**  Read and order the queue.
	*/

	nrequests = orderq(TRUE);

	/*
	**  Print the work list that we have read.
	*/

	/* first see if there is anything */
	if (nrequests <= 0)
	{
		printf("Mail queue is empty\n");
		return;
	}

	printf("\t\tMail Queue (%d request%s", nrequests, nrequests == 1 ? "" : "s");
	if (nrequests > QUEUESIZE)
		printf(", only %d printed", QUEUESIZE);
	if (Verbose)
		printf(")\n--QID-- --Size-- -Priority- ---Q-Time--- -----------Sender/Recipient-----------\n");
	else
#ifdef	NeXT_MOD
! 		printf(")\n--QID-- -Size- ----Q-Time----- ------------Sender/Recipient------------\n");
#else	NeXT_MOD
		printf(")\n--QID-- --Size-- -----Q-Time----- ------------Sender/Recipient------------\n");
#endif	NeXT_MOD
	for (w = WorkQ; w != NULL; w = w->w_next)
	{
		struct stat st;
		auto time_t submittime = 0;
		long dfsize = -1;
		char lf[20];
		char message[MAXLINE];
		extern bool shouldqueue();

		f = fopen(w->w_name, "r");
#if	NeXT_MOD
#else	NeXT_MOD
		if (f == NULL)
		{
			errno = 0;
			continue;
		}
#endif	NeXT_MOD
		printf("%7s", w->w_name + 2);
		(void) strcpy(lf, w->w_name);
		lf[0] = 'l';
		if (stat(lf, &st) >= 0)
			printf("*");
#if	NeXT_MOD
		else
			printf(" ");
		if (shouldqueue(w->w_pri))
#else	NeXT_MOD
		else if (shouldqueue(w->w_pri))
#endif	NeXT_MOD
			printf("X");
		else
			printf(" ");
		errno = 0;
#if	NeXT_MOD
		if (f == NULL)
		{
			printf(" (finished)\n");
			errno = 0;
			continue;
		}
#endif	NeXT_MOD
		message[0] = '\0';
		while (fgets(buf, sizeof buf, f) != NULL)
		{
			fixcrlf(buf, TRUE);
			switch (buf[0])
			{
			  case 'M':	/* error message */
				(void) strcpy(message, &buf[1]);
				break;

			  case 'S':	/* sender name */
				if (Verbose)
#if	NeXT_MOD
					printf("%5ld %10ld %.12s %.38s", dfsize,
#else	NeXT_MOD
					printf("%8ld %10ld %.12s %.38s", dfsize,
#endif	NeXT_MOD
					    w->w_pri, ctime(&submittime) + 4,
					    &buf[1]);
				else
#if	NeXT_MOD
					printf("%5ld %.16s %.45s", dfsize,
#else	NeXT_MOD
					printf("%8ld %.16s %.45s", dfsize,
#endif	NeXT_MOD
					    ctime(&submittime), &buf[1]);
				if (message[0] != '\0')
#if	NeXT_MOD
					printf("\n%57s", message);
#else	NeXT_MOD
					printf("\n\t\t (%.60s)", message);
#endif	NeXT_MOD
				break;

			  case 'R':	/* recipient name */
				if (Verbose)
					printf("\n\t\t\t\t\t %.38s", &buf[1]);
				else
					printf("\n\t\t\t\t  %.45s", &buf[1]);
				break;

			  case 'T':	/* creation time */
				submittime = atol(&buf[1]);
				break;

			  case 'D':	/* data file name */
				if (stat(&buf[1], &st) >= 0)
					dfsize = st.st_size;
				break;
			}
		}
		if (submittime == (time_t) 0)
			printf(" (no control file)");
		printf("\n");
		(void) fclose(f);
	}
}

# endif QUEUE
/*
**  QUEUENAME -- build a file name in the queue directory for this envelope.
**
**	Assigns an id code if one does not already exist.
**	This code is very careful to avoid trashing existing files
**	under any circumstances.
**		We first create an nf file that is only used when
**		assigning an id.  This file is always empty, so that
**		we can never accidently truncate an lf file.
**
**	Parameters:
**		e -- envelope to build it in/from.
**		type -- the file type, used as the first character
**			of the file name.
**
**	Returns:
**		a pointer to the new file name (in a static buffer).
**
**	Side Effects:
**		Will create the lf and qf files if no id code is
**		already assigned.  This will cause the envelope
**		to be modified.
*/

char *
queuename(e, type)
	register ENVELOPE *e;
	char type;
{
	static char buf[MAXNAME];
	static int pid = -1;
	char c1 = 'A';
	char c2 = 'A';

	if (e->e_id == NULL)
	{
		char qf[20];
		char nf[20];
		char lf[20];

		/* find a unique id */
		if (pid != getpid())
		{
			/* new process -- start back at "AA" */
			pid = getpid();
			c1 = 'A';
			c2 = 'A' - 1;
		}
		(void) sprintf(qf, "qfAA%05d", pid);
		(void) strcpy(lf, qf);
		lf[0] = 'l';
		(void) strcpy(nf, qf);
		nf[0] = 'n';

		while (c1 < '~' || c2 < 'Z')
		{
			int i;

			if (c2 >= 'Z')
			{
				c1++;
				c2 = 'A' - 1;
			}
			lf[2] = nf[2] = qf[2] = c1;
			lf[3] = nf[3] = qf[3] = ++c2;
# ifdef DEBUG
			if (tTd(7, 20))
				printf("queuename: trying \"%s\"\n", nf);
# endif DEBUG

# ifdef QUEUE
# if NeXT_MOD
			/*
			 * Access doesn't work for super-user,
			 * better to use stat to see if the file
			 * exists.
			 */
			{
				struct stat st;
				int i, j, k, l;
				if (   (i=stat(lf, &st)) != -1 || (j=errno) != ENOENT
				    || (k=stat(qf, &st)) != -1 || (l=errno) != ENOENT)
				    	continue;
			}
# else NeXT_MOD
			if (access(lf, 0) >= 0 || access(qf, 0) >= 0)
				continue;
# endif NeXT_MOD
			errno = 0;
			i = creat(nf, FileMode);
			if (i < 0)
			{
				(void) unlink(nf);	/* kernel bug */
				continue;
			}
			(void) close(i);
			i = link(nf, lf);
			(void) unlink(nf);
			if (i < 0)
				continue;
			if (link(lf, qf) >= 0)
				break;
			(void) unlink(lf);
# else QUEUE
			if (close(creat(qf, FileMode)) >= 0)
				break;
# endif QUEUE
		}
#if	NeXT_MOD
		/*
		 * The next test used to check for c1 >= '~', but there's
		 * no reason to go thru 26**2 permutations; 52 should do.
		 */
		if (c1 >= 'B' && c2 >= 'Z')
		{
			if (type != '\0') {
			     syserr("queuename: Cannot create \"%s\" in \"%s\"",
				    qf, QueueDir);
			}
#else	NeXT_MOD
		if (c1 >= '~' && c2 >= 'Z')
		{
			syserr("queuename: Cannot create \"%s\" in \"%s\"",
				qf, QueueDir);
#endif	NeXT_MOD
			exit(EX_OSERR);
		}
		e->e_id = newstr(&qf[2]);
		define('i', e->e_id, e);
# ifdef DEBUG
		if (tTd(7, 1))
			printf("queuename: assigned id %s, env=%x\n", e->e_id, e);
# ifdef LOG
		if (LogLevel > 16)
			syslog(LOG_DEBUG, "%s: assigned id", e->e_id);
# endif LOG
# endif DEBUG
	}

	if (type == '\0')
		return (NULL);
	(void) sprintf(buf, "%cf%s", type, e->e_id);
# ifdef DEBUG
	if (tTd(7, 2))
		printf("queuename: %s\n", buf);
# endif DEBUG
	return (buf);
}
/*
**  UNLOCKQUEUE -- unlock the queue entry for a specified envelope
**
**	Parameters:
**		e -- the envelope to unlock.
**
**	Returns:
**		none
**
**	Side Effects:
**		unlocks the queue for `e'.
*/

unlockqueue(e)
	ENVELOPE *e;
{
#if	NeXT_MOD
	if (e->e_id == NULL) return;
#endif	NeXT_MOD
	/* remove the transcript */
#ifdef DEBUG
# ifdef LOG
	if (LogLevel > 19)
		syslog(LOG_DEBUG, "%s: unlock", e->e_id);
# endif LOG
	if (!tTd(51, 4))
#endif DEBUG
		xunlink(queuename(e, 'x'));

# ifdef QUEUE
	/* last but not least, remove the lock */
	xunlink(queuename(e, 'l'));
# endif QUEUE
}
/*
**  GETCTLUSER -- return controlling user if mailing to prog or file
**
**	Check for a "|" or "/" at the beginning of the address.  If
**	found, return a controlling username.
**
**	Parameters:
**		a - the address to check out
**
**	Returns:
**		Either NULL, if we werent mailing to a program or file,
**		or a controlling user name (possibly in getpwuid's
**		static buffer).
**
**	Side Effects:
**		none.
**
**	Discussion:
**		Bugfix:  Mail to a file or program is occasionally
**		delivered with an inappropriate uid/gid.  Problem
**		arises usually because of a .forward file and the
**		fact the message got queued instead of immediately
**		delivered.  The proper Control Address info (and it's
**		uid/gid info) is usually lost irretrievably when a
**		program or file recipient is written into the queuefile.
**
**		It may be better to emit a controlling address in all
**		cases where a "ctladdr" can be found.  The use of a
**		new "C" record was considered, but would result in
**		non-backward-compatible queue files.
**
**	Larry Parmelee 9-Mar-89			Jeff Forys 25-Feb-90
**	parmelee@cs.cornell.edu			forys@cs.utah.edu
*/
char *
getctluser(a)
	ADDRESS *a;
{
	extern ADDRESS *getctladdr();
	struct passwd *pw;
	char *retstr, buf[MAXNAME];
	/* get unquoted user for file, program or user.name check */
	(void) strncpy(buf, a->q_paddr, MAXNAME);
	buf[MAXNAME-1] = '\0';
	stripquotes(buf, TRUE);
	if (buf[0] != '|' && buf[0] != '/')
		return((char *)NULL);
	a = getctladdr(a);		/* find controlling address */
	if (a != NULL && a->q_uid != 0 && (pw = getpwuid(a->q_uid)) != NULL)
		retstr = pw->pw_name;
	else				/* use default user */
		retstr = DefUser;
	if (tTd(40, 5))
		printf("Set controlling user for `%s' to `%s'\n",
		       (a == NULL)? "<null>": a->q_paddr, retstr);
	return(retstr);
}
static char CtlUser[MAXNAME];
/*
**  SETCTLUSER - sets `CtlUser' to controlling user if appropriate
**  CLRCTLUSER - clears controlling user (no params, nothing returned)
**
**	These routines manipulate `CtlUser'.
**
**	Parameters:
**		str  - string checked for a controlling user
**
**	Returns:
**		If `str' is of the form "(user) anything", `CtlUser'
**		is set to "user", and "anything" is returned, otherwise
**		`str' is returned.
**
**	Side Effects:
**		setctluser() may chop up `str' into two strings.
**		`CtlUser' is changed.
**
**	Larry Parmelee 9-Mar-89			Jeff Forys 25-Feb-90
**	parmelee@cs.cornell.edu			forys@cs.utah.edu
*/
char *
setctluser(str)
register char *str;
{
	register char *ectl;
	int depth = 0, bslash = 0;
	if (*str != '(')	/* no controlling user */
		return(str);
	/*
	**  Look thru the string for the matching right paren,
	**  being careful about nesting and backslash escapes.
	**  This is done to take care of any "weird" user names
	**  which could conceivably show up between our paren.
	*/
	for (ectl = str; *ectl; ectl++) {
		if (bslash == 0 && *ectl == '\\') {	/* escape */
			bslash = 1;
			continue;
		}
		if (bslash) {				/* escaped char */
			bslash = 0;
			continue;
		}
		if (*ectl == '(')			/* left paren */
			depth++;
		else if (*ectl == ')') {		/* right paren */
			if (--depth == 0)
				break;
		}
	}
	if (depth)		/* no controlling user */
		return(str);
	/* we have a controlling user */
	*ectl++ = '\0';
	(void) strncpy(CtlUser, ++str, MAXNAME);
	CtlUser[MAXNAME-1] = '\0';
	return((*ectl == ' ')? ++ectl: ectl);
}
clrctluser()
{
	*CtlUser = '\0';
}
/*
**  SETCTLADDR -- create a controlling address
**
**	If global variable `CtlUser' is set and we are given a valid
**	address, make that address a controlling address; change the
**	`q_uid', `q_gid', and `q_ruser' fields and set QGOODUID.
**
**	Parameters:
**		a - address for which control uid/gid info may apply
**
**	Returns:
**		None.
**
**	Side Effects:
**		Fills in uid/gid fields in address and sets QGOODUID
**		flag if appropriate.
**
**	Larry Parmelee 9-Mar-89			Jeff Forys 25-Feb-90
**	parmelee@cs.cornell.edu			forys@cs.utah.edu
*/
setctladdr(a)
	ADDRESS *a;
{
	struct passwd *pw;
	if (*CtlUser == '\0' || a == NULL || a->q_ruser)
		return;
	if ((pw = getpwnam(CtlUser)) != NULL) {
		if (a->q_home)
			free(a->q_home);
		a->q_home = newstr(pw->pw_dir);
		a->q_uid = pw->pw_uid;
		a->q_gid = pw->pw_gid;
		a->q_ruser = newstr(CtlUser);
	} else {
		a->q_uid = DefUid;
		a->q_gid = DefGid;
		a->q_ruser = newstr(DefUser);
	}
	a->q_flags |= QGOODUID;		/* flag as a "ctladdr"  */
	if (tTd(40, 5))
		printf("Restored controlling user for `%s' to `%s'\n",
		       (a == NULL)? "<null>": a->q_paddr, a->q_ruser);
}
/*
**  BADCTLADDR - ensure that an address does not look like "(user) xxx".
**	This is only a sanity check and is probably unnecessary, however
**	being wrong could lead to another security hole so we do it!
**
**	Parameters:
**		addr  - address to be checked
**
**	Returns:
**		0 if the address is valid, 1 otherwise.
**
**	Side Effects:
**		If `addr->q_paddr' looks like "(user) xxx", `addr->q_flags'
**		gets set to QBADADDR and `addr->q_paddr' gets chopped up.
*/
bool
badctladdr(addr)
ADDRESS *addr;
{
	char *setctluser();
	char *opaddr = addr->q_paddr;
	if (setctluser(addr->q_paddr) != opaddr) {
		usrerr("Bad format for qf command: %s %s",
		       CtlUser, addr->q_paddr);
		clrctluser();
		addr->q_flags |= QBADADDR;
		return(TRUE);
	}
	return(FALSE);
}

/*
 *  JOBS.C -- job control for Bourne shell
 *
 *  created by Ron Natalie, BRL
 *  slight changes by Doug Gwyn
 */

#ifdef JOBS

#include "defs.h"
#include "sym.h"

#if BERKELEY	/* native /bin/sh */
#include <sys/ioctl.h>
#else	/* /usr/5bin/sh */
#include	<sys/_ioctl.h>
#define	ioctl	_ioctl
#define	killpg	_killpg
#define	setpgrp	_setpgrp
#define	TIOCSETD	_IOW( 't', 1, int )
#define	TIOCSPGRP	_IOW( 't', 118, int )
#define	NTTYDISC	2
#endif

#define	NJCH	30
#define	JCOMSIZE 50
static struct	j_child  {
	int	j_pgid;
	int	j_status;
	int	j_info;
	char	j_com[JCOMSIZE];
	int	j_jobnum;
} j_children[NJCH];

static int	j_number = 1;
static int	j_current = 0;

#define	JEMPTY	0
#define	JALIVE	1
#define	JSTOP	2
#define	JBG	3

BOOL		j_son_of_jobs = FALSE;
int		j_default_pg = 0;
int		j_original_pg = 0;
static int	j_last_pgrp = 0;
static int	j_do(), j_getnumber(), j_stuff();
static void	j_backoff(), j_print_ent(), j_setcommand();
	
j_child_post(p, bg, pin, t)
	int		p;
	int		bg;
	register int	pin;
	struct trenod	***t;
{
	register struct j_child *j = j_children;

	if((flags & jobflg) == 0)
		return;

	if(!pin)  {
		setpgrp(p, p);
		j_last_pgrp = p;
		if(!bg)  {
			ioctl(1, TIOCSPGRP, &p);
			setpgrp(0, p);
		}
	}
	else
		setpgrp(p, j_last_pgrp);

	for(j=j_children; j < &j_children[NJCH]; j++)  {
		if(pin && j->j_pgid == j_last_pgrp)  {
			j_setcommand(j, t);
			return;
		}

		if(!pin &&j->j_status == JEMPTY)  {
			j->j_com[0] = 0;
			j->j_status = bg ? JBG : JALIVE;
			j->j_pgid = p;
			j_setcommand(j, t);
			if(bg)  {
				post(p);
				j->j_jobnum = j_getnumber();
				j_print_ent(j);
			}
			else
				j->j_jobnum = 0;
			return;
		}
	}
	prn(p);
	prs(cjpostr);			/* DAG -- made strings sharable */
}

j_child_clear(p)
	register int	p;
{
	register struct j_child *j = j_children;

	if(p == 0 || p == -1)
		return;

	for(; j < &j_children[NJCH]; j++)
		if(j->j_status == JALIVE && j->j_pgid == p)  {
			j->j_status = JEMPTY;
			if(j->j_jobnum && j->j_jobnum == j_current)
				j_backoff();
			break;
		}
}

j_child_stop(p, sig)
	register int	p;
	int		sig;
{
	register struct j_child *j = j_children; 

	for(; j < &j_children[NJCH]; j++)
		if((j->j_status == JALIVE || j->j_status == JBG) && j->j_pgid == p)  {
			j->j_status = JSTOP;
			j->j_info = sig;
			if(j->j_jobnum == 0)
				j->j_jobnum = j_getnumber();
			j_current = j->j_jobnum;
			prc(NL);
			j_print_ent(j);
			fault(SIGSTOP);
			break;
		}
}

j_child_die(p)
	register int	p;
{
	register struct j_child *j = j_children; 

	if(p == 0 || p == -1)
		return;

	for(; j < &j_children[NJCH]; j++)
		if( j->j_status != JEMPTY && j->j_pgid == p)  {
			j->j_status = JEMPTY;
			if(j->j_jobnum && j->j_jobnum == j_current)
				j_backoff();
			break;
		}
}

j_print()
{
	register struct j_child *j = j_children; 

	if((flags & jobflg) == 0)  {
		prs(jcoffstr);		/* DAG */
		return;
	}

	await(-2, 1);
	for(; j < &j_children[NJCH]; j++)
		j_print_ent(j);
}

static void
j_print_ent(j)
	register struct j_child *j;
{
	if(j->j_status == JEMPTY)
		return;

	if(j->j_jobnum == 0) {
		prs(jpanstr);		/* DAG */
		prn(j->j_pgid);
		prc(NL);
	}
	prc('[');
	prn(j->j_jobnum);
	prs(rsqbrk);			/* DAG */
	if(j_current == j->j_jobnum)
		prs(execpmsg);		/* DAG */
	else
		prs(spspstr);		/* DAG */
	prn(j->j_pgid);
	prc(' ');

	switch(j->j_status)  {
	case JALIVE:
		prs(fgdstr);		/* DAG */
		break;
	case JSTOP:
		prs(stpdstr);		/* DAG */
		switch(j->j_info)  {
		case SIGTSTP:
			prs(lotspstr);	/* DAG */
			break;
		case SIGSTOP:
			prs(psgpstr);	/* DAG */
			break;
		case SIGTTIN:
			prs(ptinstr);	/* DAG */
			break;
		case SIGTTOU:
			prs(ptoustr);	/* DAG */
			break;
		}
		break;
	case JBG:
		prs(bgdstr);		/* DAG */
		break;
	}
	prc(' ');
	prs(j->j_com);
	prc(NL);
}

j_resume(cp, bg)
	char	*cp;
	BOOL	bg;
{
	register struct j_child *j = j_children; 
	int	p;
	
	if((flags & jobflg) == 0)  {
		prs(jcoffstr);		/* DAG */
		return;
	}

	if(cp)  {
		p = atoi(cp);
		if(p == 0)  {
			prs(jinvstr);	/* DAG */
			return;
		}
	}
	else
		p = 0;
		
	await(-2, 1);
	if(p == 0 && j_current == 0)  {
		prs(ncjstr);		/* DAG */
		return;
	}

	for(; j < &j_children[NJCH]; j++)
		if(j->j_status != JEMPTY)
			if(
			   (p != 0 && j->j_pgid == p) ||
			   (p == 0 && j->j_jobnum == j_current)
			)  {
				p = j->j_pgid;
				if(!bg)  {
					ioctl(1, TIOCSPGRP, &p);
					setpgrp(0, p);
				}
				j->j_status = bg ? JBG : JALIVE;
				j_print_ent(j);
				if(killpg(p, SIGCONT) == -1)  {
					j->j_status = JEMPTY;
					break;
				}
				if(bg)
					post(p);
				else
					await(p, 0);
				j_reset_pg();
				return;
			}
	prn(p);
	prs(nstpstr);			/* DAG */
}

char	*
j_macro()
{
	static char	digbuf[40];
	register char	*c;
	register int	i;
	register struct j_child *j = j_children; 

	c = digbuf;
	*c++ = '%';

	for(;;) {
		*c = readc();
		if( c==(digbuf+1) && *c == '%')  {
			i = j_current;
			break;
		}
		if(!digchar(*c))  {
			peekc = *c | MARK;
			*c = 0;
			i = stoi(digbuf+1);
			break;
		}
		c++;
	}

	if(i != 0)
		for(; j < &j_children[NJCH]; j++)
			if(j->j_status != JEMPTY && j->j_jobnum == i)  {
				itos(j->j_pgid);
				movstr(numbuf, digbuf);	/* DAG */
				break;
			}

	return digbuf;
}

j_reset_pg()
{
	if((flags & jobflg) == 0)
		return;
	ioctl(0, TIOCSPGRP, &j_default_pg);
	setpgrp(0, j_default_pg);
}

j_really_reset_pg()
{
	ioctl(0, TIOCSPGRP, &j_original_pg);
	setpgrp(0, j_original_pg);
}


#include "ctype.h"
extern BOOL trapflg[];

j_init()
{
	static int	ldisc = NTTYDISC;	/* BERKELEY ioctl brain damage */

	if(flags & jobflg)
		return;
	j_reset_pg();
	trapflg[SIGTTIN] = SIGMOD | 1;
	trapflg[SIGTTOU] = SIGMOD | 1;
	trapflg[SIGTSTP] = SIGMOD | 1;
	trapflg[SIGSTOP] = SIGMOD | 1;
	ignsig(SIGTSTP);
	ignsig(SIGSTOP);	/*  Just to make sure  */
	(void)ioctl( 0, TIOCSETD, &ldisc );	/* DAG -- require "new tty" handler */
/*	flags |= jobflg;	*/
}

BOOL
j_finish(force)
	BOOL	force;
{
	register struct j_child *j = j_children; 

	if((flags & jobflg) == 0)
		return FALSE;

	await(-2, 1);
	for(; j < &j_children[NJCH]; j++)
		if(j->j_status == JSTOP )
			if(force)  {
				killpg(j->j_pgid, SIGHUP);
				killpg(j->j_pgid, SIGCONT);
			}
			else  {
				prs(tasjstr);	/* DAG */
				return TRUE;
			}
	if(force)  {
		await(-2, 1);		
		return FALSE;
	}
	trapflg[SIGTTIN] = SIGMOD;
	trapflg[SIGTTOU] = SIGMOD;
	trapflg[SIGTSTP] = SIGMOD;
	trapflg[SIGSTOP] = SIGMOD;
	flags &= ~jobflg;
	j_really_reset_pg();
	return FALSE;
}


static int	j_numbers = 0;

static int
j_getnumber()
{
	register struct j_child *j = j_children; 
	
	for(; j < &j_children[NJCH]; j++)
		if(j->j_status != JEMPTY && j->j_jobnum)
			return j_numbers++;
	j_numbers = 2;
	return 1;
}

static void
j_backoff()
{
	register struct j_child *j = j_children; 
	
	j_current = 0;
	for(; j < &j_children[NJCH]; j++)
		if(j->j_status != JEMPTY && j->j_jobnum)
			if(j->j_jobnum > j_current)
				j_current = j->j_jobnum;
}

static	int	jcleft;
static  char	*jcp;

static void
j_setcommand(j, t)
	register struct j_child	*j;
	struct trenod		*t;
{
	jcleft = strlen(j->j_com);
	jcp = j->j_com + jcleft;
	jcleft = JCOMSIZE - 1  - jcleft;

	if(j->j_com[0] == '\0' || !j_stuff(pipestr))	/* DAG */
		j_do(t);
}

static int
j_do_chain(a)
	register struct argnod *a;
{
	while(a)  {
		if(j_stuff(a->argval))
			return 1;
		a = a->argnxt;
		if(a)
			j_stuff(spcstr);	/* DAG */
	}
	return 0;
}

#define IOGET 0
static int
j_do_redir(t)
	register struct ionod	*t;
{
	register int	iof;	/* DAG -- added for speed */
	register int	i;

	while(t)  {
		if(t->ioname)  {
			if(j_stuff(spcstr))	/* DAG */
				return 1;
			iof = t->iofile;
			i = iof & IOUFD;
			if(
			    ((iof&IOPUT) && (i != 1)) ||
			    (((iof&IOPUT)==0) && (i!= 0))
			)  {
				itos(i);
				if(j_stuff(numbuf))
					return 1;
			}
			switch(iof & (IODOC|IOPUT|IOMOV|IOAPP))  {
			case IOGET:
				if(j_stuff(rdinstr))	/* DAG */
					return 1;
				break;
			case IOPUT:
				if(j_stuff(readmsg))	/* DAG */
					return 1;
				break;
			case IOAPP|IOPUT:
				if(j_stuff(appdstr))	/* DAG */
					return 1;
				break;
			case IODOC:
				if(j_stuff(inlnstr))	/* DAG */
					return 1;
				break;
			case IOMOV|IOPUT:
				if(j_stuff(toastr))	/* DAG */
					return 1;
				break; 
			case IOMOV|IOGET:
				if(j_stuff(fromastr))	/* DAG */
					return 1;
				break;
			}
			if(j_stuff(t->ioname))
				return 1;
		}
		t = t->ionxt;
	}
	return 0;
}
				
				
static int
j_do(t)	
	register struct trenod *t;
{
	int	type;

	if (t == (struct trenod *)0)	/* DAG -- added safety check */
		return 0;

	type = t->tretyp & COMMSK;
	switch(type)  {
	case	TFND:	/* added by DAG for System V Release 2 shell */
		return j_stuff(fndptr(t)->fndnam)
		    || j_stuff(sfnstr)	/* DAG */
		    || j_do(fndptr(t)->fndval)
		    || j_stuff(efnstr);	/* DAG */

	case	TCOM:
		if(comptr(t)->comset)  {
			if(j_do_chain(comptr(t)->comset)
			|| j_stuff(spcstr))
				return 1;			
		}
		return j_do_chain(comptr(t)->comarg)
		    || j_do_redir(comptr(t)->comio);

	case	TLST:
	case	TAND:
	case	TORF:
	case	TFIL:	/* DAG -- merged */
		if(j_do(lstptr(t)->lstlef))
			return 1;
		switch(type)  {
		case TLST:
			if(j_stuff(semspstr))	/* DAG */
				return 1;
			break;
		case TAND:
			if(j_stuff(andstr))	/* DAG */
				return 1;
			break;
		case TORF:
			if(j_stuff(orstr))	/* DAG */
				return 1;
			break;
		case TFIL:
			if(j_stuff(pipestr))	/* DAG */
				return 1;
			break;
		}
		return j_do(lstptr(t)->lstrit);

	case	TFORK:
		return j_do(forkptr(t)->forktre)
		    || j_do_redir(forkptr(t)->forkio)
		    || (forkptr(t)->forktyp & FAMP) && j_stuff(amperstr);	/* DAG */

	case	TPAR:
		return j_stuff(lpnstr)	/* DAG */
		    || j_do(parptr(t)->partre)
		    || j_stuff(rpnstr);	/* DAG */

	case	TFOR:
	case	TWH:
	case	TUN:
	{
		struct trenod *c;

		switch(type)  {
		case TFOR:
			if(j_stuff(forstr)	/* DAG */
			|| j_stuff(forptr(t)->fornam))
				return 1;
			if(forptr(t)->forlst)  {
				if(j_stuff(insstr)	/* DAG */
				|| j_do_chain(forptr(t)->forlst->comarg))
					return 1;
			}
			c = forptr(t)->fortre;
			break;

		case TWH:
			if(j_stuff(whilestr)	/* DAG */
			|| j_do(whptr(t)->whtre))
				return 1;
			c = whptr(t)->dotre;
			break;

		case TUN:
			if(j_stuff(untilstr)	/* DAG */
			|| j_do(whptr(t)->whtre))
				return 1;
			c = whptr(t)->dotre;
			break;
		}
		return j_stuff(sdostr)	/* DAG */
		    || j_do(c)
		    || j_stuff(sdonstr);	/* DAG */
	}
	case	TIF:
		if(j_stuff(ifstr)	/* DAG */
		|| j_do(ifptr(t)->iftre)
		|| j_stuff(sthnstr)	/* DAG */
		|| j_do(ifptr(t)->thtre))
			return 1;
		if(ifptr(t)->eltre)  {
			if(j_stuff(selsstr)	/* DAG */
			|| j_do(ifptr(t)->eltre))
				return 1;
		}
		return j_stuff(sfistr);	/* DAG -- bug fix (was "; done") */

	case	TSW:
		return j_stuff(casestr)	/* DAG */
		    || j_stuff(swptr(t)->swarg)
		    || j_stuff(iesacstr);	/* DAG */

	default:
/*		printf("sh bug: j_do--unknown type %d\n", type);	*/
		return 0;
	}
}

static int
j_stuff(f)
	char	*f;
{
	register int	i;
	register int	runover;

	i = strlen(f);
	runover = i > jcleft;
	if(runover)
		i = jcleft;
	strncpy(jcp, f, i);
	jcleft -= i;
	jcp += i;
	*jcp = 0;
	if(runover)  {
		jcp[-1] = '.';
		jcp[-2] = '.';
		jcp[-3] = '.';
	}
	return runover;
}
#endif JOBS

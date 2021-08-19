/*	@(#)expand.c	1.4	*/
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#if defined(BRL) && !defined(BERKELEY)
#include	<dir.h>
#else
#include	<sys/dir.h>
#endif

#if BERKELEY || BRL
#define	getdir(dirf)	readdir(dirf)

#define	entry		e->d_name
#else
#define MAXDIR	64
struct direct		*getdir();

static struct direct	dirbuf[MAXDIR];
static int		nxtdir = -1;
static int		maxdir = 0;
static char		entry[DIRSIZ+1];
#endif

/*
 * globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */
extern int	addg();


expand(as, rcnt)
	char	*as;
{
	int	count;
#if BERKELEY || BRL
	DIR	*dirf;
#else
	int	dirf;
#endif
	BOOL	dir = 0;
	char	*rescan = 0;
	register char	*s, *cs;
	struct argnod	*schain = gchain;
	struct stat statb;
	BOOL	slash;

	if (trapnote & SIGSET)
		return(0);
	s = cs = as;

	/*
	 * check for meta chars
	 */
	{
		register BOOL open;

		slash = 0;
		open = 0;
		do
		{
			switch (*cs++)
			{
			case 0:
				if (rcnt && slash)
					break;
				else
					return(0);

			case '/':
				slash++;
				open = 0;
				continue;

			case '[':
				open++;
				continue;

			case ']':
				if (open == 0)
					continue;

			case '?':
			case '*':
				if (rcnt > slash)
					continue;
				else
					cs--;
				break;


			default:
				continue;
			}
			break;
		} while (TRUE);
	}

	for (;;)
	{
		if (cs == s)
		{
			s = nullstr;
			break;
		}
		else if (*--cs == '/')
		{
			*cs = 0;
			if (s == cs)
				s = slstr;	/* DAG -- made string sharable */
			break;
		}
	}

#if BERKELEY || BRL
	if (dirf = opendir(*s ? s : pdstr))
#else
	/* DAG -- bug fix next line due to Mike Wescott (was >) */
	if ((dirf = open(*s ? s : pdstr, 0)) >= 0)	/* DAG -- made string sharable */
#endif
	{
#if BERKELEY || BRL
		if (fstat(dirf->dd_fd, &statb) != -1 &&
#else
		if (fstat(dirf, &statb) != -1 &&
#endif
		    (statb.st_mode & S_IFMT) == S_IFDIR)
			dir++;
		else
#if BERKELEY || BRL
			closedir(dirf);
#else
			close(dirf);
#endif
	}

	count = 0;
	if (*cs == 0)
		*cs++ = 0200;
	if (dir)		/* check for rescan */
	{
		register char *rs;
		struct direct *e;

		rs = cs;
		do
		{
			if (*rs == '/')
			{
				rescan = rs;
				*rs = 0;
				gchain = 0;
			}
		} while (*rs++);

		while ((e = getdir(dirf)) && (trapnote & SIGSET) == 0)
		{
#if !defined(BERKELEY) && !defined(BRL)
			*(movstrn(e->d_name, entry, DIRSIZ)) = 0;
#endif

			if (entry[0] == '.' && *cs != '.')
#ifndef BOURNE
				continue;
#else
			{
				if (entry[1] == 0)
					continue;
				if (entry[1] == '.' && entry[2] == 0)
					continue;
			}
#endif

			if (gmatch(entry, cs))
			{
				addg(s, entry, rescan);
				count++;
			}
		}
#if BERKELEY || BRL
		closedir(dirf);
#else
		close(dirf);
#endif

		if (rescan)
		{
			register struct argnod	*rchain;

			rchain = gchain;
			gchain = schain;
			if (count)
			{
				count = 0;
				while (rchain)
				{
					count += expand(rchain->argval, slash + 1);
					rchain = rchain->argnxt;
				}
			}
			*rescan = '/';
		}
	}

	{
		register char	c;

		s = as;
		while (c = *s)
			*s++ = (c & STRIP ? c : '/');
	}
	return(count);
}


#if !defined(BERKELEY) && !defined(BRL)
reset_dir()
{
	nxtdir = -1;
	maxdir = 0;
}


/*
 * read next directory entry
 * and ignore inode == 0
 *
 */

struct direct *
getdir(dirf)
{
	for (;;)
	{
		if (++nxtdir == maxdir)
		{
			int r;

			r = read(dirf, dirbuf, sizeof(dirbuf)) / sizeof(struct direct);
			if (maxdir = r)
				nxtdir = 0;
			else
			{
				nxtdir = -1;
				return(0);
			}
		}

		/* nxtdir is next available entry */
		if (dirbuf[nxtdir].d_ino == 0)
			continue;


		return(&dirbuf[nxtdir]);
	}
}
#endif

gmatch(s, p)
register char	*s, *p;
{
	register int	scc;
	char		c;

	if (scc = *s++)
	{
		if ((scc &= STRIP) == 0)
			scc=0200;
	}
	switch (c = *p++)
	{
	case '[':
		{
			BOOL ok;
			int lc;
			int notflag = 0;

			ok = 0;
			lc = 077777;
			if (*p == '!')
			{
				notflag = 1;
				p++;
			}
			while (c = *p++)
			{
				if (c == ']')
					return(ok ? gmatch(s, p) : 0);
				else if (c == MINUS)
				{
					if (notflag)
					{
						if (scc < lc || scc > *(p++))
							ok++;
						else
							return(0);
					}
					else
					{
						if (lc <= scc && scc <= (*p++))
							ok++;
					}
				}
				else
				{
					lc = c & STRIP;
					if (notflag)
					{
						if (scc && scc != lc)
							ok++;
						else
							return(0);
					}
					else
					{
						if (scc == lc)
							ok++;
					}
				}
			}
			return(0);
		}

	default:
		if ((c & STRIP) != scc)
			return(0);

	case '?':
		return(scc ? gmatch(s, p) : 0);

	case '*':
		while (*p == '*')
			p++;

		if (*p == 0)
			return(1);
		--s;
		while (*s)
		{
			if (gmatch(s++, p))
				return(1);
		}
		return(0);

	case 0:
		return(scc == 0);
	}
}

static int
addg(as1, as2, as3)
char	*as1, *as2, *as3;
{
	register char	*s1, *s2;
	register int	c;

	s2 = locstak() + BYTESPERWORD;
	s1 = as1;
	while (c = *s1++)
	{
		if ((c &= STRIP) == 0)
		{
			*s2++ = '/';
			break;
		}
		*s2++ = c;
	}
	s1 = as2;
	while (*s2 = *s1++)
		s2++;
	if (s1 = as3)
	{
		*s2++ = '/';
		while (*s2++ = *++s1);
	}
	makearg(endstak(s2));
}

makearg(args)
	register struct argnod *args;
{
	args->argnxt = gchain;
	gchain = args;
}

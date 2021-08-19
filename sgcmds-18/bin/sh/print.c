/*	@(#)print.c	1.5	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#if BERKELEY
#define	HZ	60
#else
#include	<sys/param.h>
#endif

#define		BUFLEN		256

static char	buffer[BUFLEN + 2];	/* DAG -- added slop space for prs_cntl */
static int	index = 0;
char		numbuf[12];

extern void	prc_buff();
extern void	prs_buff();
extern void	prn_buff();
extern void	prs_cntl();
extern void	prn_buff();

/*
 * printing and io conversion
 */
prp()
{
	if ((flags & prompt) == 0 && cmdadr)
	{
		prs_cntl(cmdadr);
		prs(colon);
	}
}

prs(as)
char	*as;
{
	register char	*s;

	if (s = as)
		write(output, s, length(s) - 1);
}

prc(c)
char	c;
{
	if (c)
		write(output, &c, 1);
}

prt(t)
long	t;
{
	register int hr, min, sec;

	t += HZ / 2;
	t /= HZ;
	sec = t % HZ;
	t /= HZ;
	min = t % HZ;

	if (hr = t / HZ)
	{
		prn_buff(hr);
		prc_buff('h');
	}

#if BRL
	if (min)	/* for consistency */
#endif
	{
	    prn_buff(min);
	    prc_buff('m');
	}
	prn_buff(sec);
	prc_buff('s');
}

prn(n)
	int	n;
{
	itos(n);

	prs(numbuf);
}

itos(n)
{
	register char *abuf;
	register unsigned a, i;
	int pr, d;

	abuf = numbuf;

	pr = FALSE;
	a = n;
	for (i = 10000; i != 1; i /= 10)
	{
		if ((pr |= (d = a / i)))
			*abuf++ = d + '0';
		a %= i;
	}
	*abuf++ = a + '0';
	*abuf++ = 0;
}

#if BRL
static int
stob(icp, base)	/* DAG -- new function to support octal as well as decimal */
char	*icp;
int	base;	/* 8 or 10 */
#else
stoi(icp)
char	*icp;
#define	base	10
#endif
{
	register char	*cp = icp;
	register int	r = 0;
	register char	c;

	while ((c = *cp, digit(c)) && c && r >= 0)
	{
		r = r * base + c - '0';	/* DAG -- changed 10 to base */
		cp++;
	}
	if (r < 0 || cp == icp)
		failed(icp, badnum);
		/*NOTREACHED*/
	else
		return(r);
#ifdef gould
	return 0;	/* DAG -- added */
#endif
}

#if BRL
int
stoi(icp)
	char	*icp;
{
	return(stob(icp, 10));		/* DAG -- change to support octals */
}

int
stoo(icp)
	char	*icp;
{
	return(stob(icp, 8));
}
#endif

prl(n)
long n;
{
	int i;

	i = 11;
	while (n > 0 && --i >= 0)
	{
		numbuf[i] = n % 10 + '0';
		n /= 10;
	}
	numbuf[11] = '\0';
	prs_buff(&numbuf[i]);
}

void
flushb()
{
	if (index)
	{
		buffer[index] = '\0';
		write(1, buffer, length(buffer) - 1);
		index = 0;
	}
}

void
prc_buff(c)
	char c;
{
	if (c)
	{
		if (index + 1 >= BUFLEN)
			flushb();

		buffer[index++] = c;
	}
	else
	{
		flushb();
		write(1, &c, 1);
	}
}

void
prs_buff(s)
	char *s;
{
	register int len = length(s) - 1;

	if (index + len >= BUFLEN)
		flushb();

	if (len >= BUFLEN)
		write(1, s, len);
	else
	{
		movstr(s, &buffer[index]);
		index += len;
	}
}


clear_buff()
{
	index = 0;
}


void
prs_cntl(s)
	char *s;
{
	register char *ptr = buffer;
	register char c;

	while (*s != '\0') 
	{
		c = (*s & 0177) ;
		
		/* translate a control character into a printable sequence */

		if (c < '\040') 
		{	/* assumes ASCII char */
			*ptr++ = '^';
			*ptr++ = (c + 0100);	/* assumes ASCII char */
		}
		else if (c == 0177) 
		{	/* '\0177' does not work */
			*ptr++ = '^';
			*ptr++ = '?';
		}
		else 
		{	/* printable character */
			*ptr++ = c;
		}

		++s;
	}

	*ptr = '\0';
	prs(buffer);
}


void
prn_buff(n)
	int	n;
{
	itos(n);

	prs_buff(numbuf);
}

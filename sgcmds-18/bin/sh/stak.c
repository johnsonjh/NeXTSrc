/*	@(#)stak.c	1.4	*/
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"

/* ========	storage allocation	======== */

#if	MACH
initstak()
{
	/*
	 * Initialize the stack locations.  These never
	 * have to move.
	 */
	vm_allocate(task_self(), &stakbas, 1024*1024*4, TRUE);
	staktop = stakbot = stakbas;
}
#endif	MACH

char *
getstak(asize)			/* allocate requested stack */
int	asize;
{
	register char	*oldstak;
	register int	size;

	size = round(asize, BYTESPERWORD);
	oldstak = stakbot;
	staktop = stakbot += size;
	return(oldstak);
}

/*
 * set up stack for local use
 * should be followed by `endstak'
 */
char *
locstak()
{
#if	MACH
	/*
	 * On MACH systems the stack and storage allocator are kept
	 * in separate virutal memory regions.  No provision for expansion
	 * is needed.
	 */
#else	MACH
	if (brkend - stakbot < BRKINCR)
	{
		if (setbrk(brkincr) == -1)
			error(nostack);
		if (brkincr < BRKMAX)
			brkincr += 256;
	}
#endif	MACH
	return(stakbot);
}

char *
savstak()
{
	assert(staktop == stakbot);
	return(stakbot);
}

char *
endstak(argp)		/* tidy up after `locstak' */
register char	*argp;
{
	register char	*oldstak;

	*argp++ = 0;
	oldstak = stakbot;
	stakbot = staktop = (char *)round(argp, BYTESPERWORD);
	return(oldstak);
}

tdystak(x)		/* try to bring stack back to x */
register char	*x;
{
#ifndef	MACH
	while ((char *)(stakbsy) > (char *)(x))
	{
		free(stakbsy);
		stakbsy = stakbsy->word;
	}
#endif	MACH
	staktop = stakbot = max((char *)(x), (char *)(stakbas));
	rmtemp(x);
}

stakchk()
{
#ifndef	MACH
	if ((brkend - stakbas) > BRKINCR + BRKINCR)
		setbrk(-BRKINCR);
#endif	MACH
}

char *
cpystak(x)
char	*x;
{
	return(endstak(movstr(x, locstak())));
}

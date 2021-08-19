/*	@(#)blok.c	1.4	*/
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */
#if	MACH
	/*
	 * Mach doesn't need to use this allocator.
	 */

/*
 * Shell will call free with zero, or values that weren't actually
 * allocated, check for this here.
 */
myfree(s)
	char *s;
{
#if 0	/* 1.5 Bug fix */
	extern end;
	if (s && s > (char *)&end)
#else
	extern unsigned long get_end ();
	static char *end_addr = 0;
	
	if (end_addr == 0)
	  end_addr = (char *) get_end ();
	
	if (s && s > end_addr)
#endif
		free(s);
}

#else	MACH
#include	"defs.h"

/*
 *	storage allocator
 *	(circular first fit strategy)
 */

#define BUSY 01
#define busy(x)	(Rcheat((x)->word) & BUSY)

unsigned	brkincr = BRKINCR;
struct blk *blokp;			/*current search pointer*/
struct blk *bloktop;			/* top of arena (last blok) */

char		*brkbegin;
char		*setbrk();

char *
alloc(nbytes)
	unsigned nbytes;
{
	register unsigned rbytes = round(nbytes+BYTESPERWORD, BYTESPERWORD);

	for (;;)
	{
		int	c = 0;
		register struct blk *p = blokp;
		register struct blk *q;

		do
		{
			if (!busy(p))
			{
				while (!busy(q = p->word))
					p->word = q->word;
				if ((char *)q - (char *)p >= rbytes)
				{
					blokp = (struct blk *)((char *)p + rbytes);
					if (q > blokp)
						blokp->word = p->word;
					p->word = (struct blk *)(Rcheat(blokp) | BUSY);
					return((char *)(p + 1));
				}
			}
			q = p;
			p = (struct blk *)(Rcheat(p->word) & ~BUSY);
		} while (p > q || (c++) == 0);
		addblok(rbytes);
	}
}

addblok(reqd)
	unsigned reqd;
{
	if (stakbot == 0)
	{
		brkbegin = setbrk(3 * BRKINCR);
		bloktop = (struct blk *)brkbegin;
	}

	if (stakbas != staktop)
	{
		register char *rndstak;
		register struct blk *blokstak;

		pushstak(0);
		rndstak = (char *)round(staktop, BYTESPERWORD);
		blokstak = (struct blk *)(stakbas) - 1;
		blokstak->word = stakbsy;
		stakbsy = blokstak;
		bloktop->word = (struct blk *)(Rcheat(rndstak) | BUSY);
		bloktop = (struct blk *)(rndstak);
	}

	reqd += brkincr;
	reqd &= ~(brkincr - 1);
	blokp = bloktop;
	bloktop = bloktop->word = (struct blk *)(Rcheat(bloktop) + reqd);
	if ((char *) bloktop >= brkend) {
		if ((int)setbrk(brkincr) < 0) {
			error(nospace);
		}
	}
	bloktop->word = (struct blk *)(brkbegin + 1);
	{
		register char *stakadr = (char *)(bloktop + 2);

		if (stakbot != staktop)
			staktop = movstr(stakbot, stakadr);
		else
			staktop = stakadr;

		stakbas = stakbot = stakadr;
	}
}

free(ap)
	struct blk *ap;
{
	register struct blk *p;
#if gould
/*
 *	On VAX, <bloktop skips things on stack, doesn't on Gould
 *	where stack is below text.
 */
	extern int _csrt();

	if ((p = ap) && p < bloktop && p > (struct blk *)_csrt)
#else
	if ((p = ap) && p < bloktop)
#endif
	{
#ifdef DEBUG
		chkbptr(p);
#endif
		--p;
		p->word = (struct blk *)(Rcheat(p->word) & ~BUSY);
	}


}


#ifdef DEBUG

chkbptr(ptr)
	struct blk *ptr;
{
	int	exf = 0;
	register struct blk *p = (struct blk *)brkbegin;
	register struct blk *q;
	int	us = 0, un = 0;

	for (;;)
	{
		q = (struct blk *)(Rcheat(p->word) & ~BUSY);

		if (p+1 == ptr)
			exf++;

		if (q < (struct blk *)brkbegin || q > bloktop)
			abort(3);

		if (p == bloktop)
			break;

		if (busy(p))
			us += q - p;
		else
			un += q - p;

		if (p >= q)
			abort(4);

		p = q;
	}
	if (exf == 0)
		abort(1);
}


chkmem()
{
	register struct blk *p = (struct blk *)brkbegin;
	register struct blk *q;
	int	us = 0, un = 0;

	for (;;)
	{
		q = (struct blk *)(Rcheat(p->word) & ~BUSY);

		if (q < (struct blk *)brkbegin || q > bloktop)
			abort(3);

		if (p == bloktop)
			break;

		if (busy(p))
			us += q - p;
		else
			un += q - p;

		if (p >= q)
			abort(4);

		p = q;
	}

	prs("un/used/avail ");
	prn(un);
	blank();
	prn(us);
	blank();
	prn((char *)bloktop - brkbegin - (un + us));
	newline();

}

#endif
#endif	MACH

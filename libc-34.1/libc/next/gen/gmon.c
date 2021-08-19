/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*
 * History
 *  2-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Changed include of kern/mach.h to kern/mach_interface.h
 *
 *  1-May-90  Matthew Self (mself) at NeXT
 *	Added prototypes, and added casts to remove all warnings.
 *	Made all private data static.
 *	vm_deallocate old data defore vm_allocate'ing new data.
 *	Added new functions monoutput and monreset.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)gmon.c	5.2 (Berkeley) 6/21/85";
#endif

#include <libc.h>
#import <sys/loader.h>
extern int profil (char *buff, int bufsiz, int offset, int scale);
extern const struct section *getsectbyname (char *segname, char *sectname);
#include "monitor.h"
#include <mach_init.h>			/* for task_self */
#include <kern/mach_interface.h>	/* for vm_allocate, vm_offset_t */
#include <sys/types.h>			/* for caddr_t */
#ifdef DEBUG
#include <stdio.h>
#endif DEBUG

#include "gmon.h"

    /*
     *	_froms is actually a bunch of unsigned shorts indexing _tos
     */
static char		_profiling = -1;	/* tas loc for NeXT */
static unsigned short	*_froms;
static struct tostruct	*_tos = 0;
static long		_tolimit = 0;
static char		*_s_lowpc = 0;
static char		*_s_highpc = 0;
static unsigned long	_s_textsize = 0;

static int	ssiz;
static char	*sbuf = 0;
static int	s_scale;
    /* see profil(2) where this is describe (incorrectly) */
#define		SCALE_1_TO_1	0x10000L

#define	MSG "No space for monitor buffer(s)\n"

void moninit (void)
{
  const struct section *section = getsectbyname ("__TEXT", "__text");
  
  monstartup ((char *) section->addr, (char *) (section->addr + section->size));
}

void monstartup (char *lowpc, char *highpc)
{
    int			monsize;
    caddr_t		*buffer;
    kern_return_t	ret;

    moncontrol (0);

	/*
	 *	round lowpc and highpc to multiples of the density we're using
	 *	so the rest of the scaling (here and in gprof) stays in ints.
	 */
    lowpc = (char *) ROUNDDOWN((unsigned)lowpc, HISTFRACTION*sizeof(HISTCOUNTER));
    _s_lowpc = lowpc;
    highpc = (char *) ROUNDUP((unsigned)highpc, HISTFRACTION*sizeof(HISTCOUNTER));
    _s_highpc = highpc;
    _s_textsize = highpc - lowpc;
    monsize = (_s_textsize / HISTFRACTION) + sizeof(struct phdr);

    if (sbuf)
      vm_deallocate (task_self (),
		     (vm_address_t) sbuf,
		     (vm_size_t) monsize);
    ret =  vm_allocate (task_self (),
			(vm_address_t *) &buffer,
			(vm_size_t) monsize,
			TRUE);
    if (ret != KERN_SUCCESS)  {
	write( 2 , MSG , sizeof(MSG) );
	return;
    }
    if (_froms)
      vm_deallocate (task_self (),
		     (vm_address_t) _froms,
		     (vm_size_t) (_s_textsize / HASHFRACTION));
    ret =  vm_allocate (task_self (),
			(vm_address_t *) &_froms,
			(vm_size_t) (_s_textsize / HASHFRACTION),
			TRUE);
    if (ret != KERN_SUCCESS)  {
	write( 2 , MSG , sizeof(MSG) );
	_froms = 0;
	return;
    }
    _tolimit = _s_textsize * ARCDENSITY / 100;
    if ( _tolimit < MINARCS ) {
	_tolimit = MINARCS;
    } else if ( _tolimit > 65534 ) {
	_tolimit = 65534;
    }
    if (_tos)
      vm_deallocate (task_self (),
		     (vm_address_t) _tos,
		     (vm_size_t) (_tolimit * sizeof(struct tostruct)));
    ret =  vm_allocate (task_self (), 
    			(vm_address_t *) &_tos,
			(vm_size_t) (_tolimit * sizeof(struct tostruct)),
			TRUE);
    if (ret != KERN_SUCCESS)  {
	write( 2 , MSG , sizeof(MSG) );
	_froms = 0;
	_tos = 0;
	return;
    }
    _tos[0].link = 0;
    monitor( lowpc , highpc , (char *) buffer , monsize , _tolimit );
}

void monreset (void)
{
    moncontrol (0);
    bzero (sbuf, ssiz);
    bzero (_froms, _s_textsize / HASHFRACTION);
    bzero (_tos, _tolimit * sizeof (struct tostruct));
    ( (struct phdr *) sbuf ) -> lpc = _s_lowpc;
    ( (struct phdr *) sbuf ) -> hpc = _s_highpc;
    ( (struct phdr *) sbuf ) -> ncnt = ssiz;
}

void monoutput (const char *filename)
{
    int			fd;
    int			fromindex;
    int			endfrom;
    char		*frompc;
    int			toindex;
    struct rawarc	rawarc;

    moncontrol (0);
    fd = creat( filename , 0666 );
    if ( fd < 0 ) {
	perror( "mcount: gmon.out" );
	return;
    }
#ifdef DEBUG
	fprintf( stderr , "[mcleanup] sbuf 0x%x ssiz %d\n" , sbuf , ssiz );
#endif DEBUG
    write( fd , sbuf , ssiz );
    endfrom = _s_textsize / (HASHFRACTION * sizeof(*_froms));
    for ( fromindex = 0 ; fromindex < endfrom ; fromindex++ ) {
	if ( _froms[fromindex] == 0 ) {
	    continue;
	}
	frompc = _s_lowpc + (fromindex * HASHFRACTION * sizeof(*_froms));
	for (toindex=_froms[fromindex];
	    toindex!=0;
	    toindex=_tos[toindex].link) {
#ifdef DEBUG
		fprintf( stderr ,
		    "[mcleanup] frompc 0x%x selfpc 0x%x count %d\n" ,
		    frompc , _tos[toindex].selfpc , _tos[toindex].count );
#endif DEBUG
	    rawarc.raw_frompc = (unsigned long) frompc;
	    rawarc.raw_selfpc = (unsigned long) _tos[toindex].selfpc;
	    rawarc.raw_count = _tos[toindex].count;
	    write( fd , &rawarc , sizeof rawarc );
	}
    }
    close( fd );
}

/* nfunc is not used; available for compatability only. */

void monitor (char *lowpc, char *highpc, char *buf, int bufsiz, int cntsiz)
{
    register o;

    moncontrol (0);
    if ( lowpc == 0 ) {
	moncontrol(0);
	monoutput ("gmon.out");
	return;
    }
    sbuf = buf;
    ssiz = bufsiz;
    ( (struct phdr *) buf ) -> lpc = lowpc;
    ( (struct phdr *) buf ) -> hpc = highpc;
    ( (struct phdr *) buf ) -> ncnt = ssiz;
    bufsiz -= sizeof(struct phdr);
    if ( bufsiz <= 0 )
	return;
    o = highpc - lowpc;
    if( bufsiz < o )
	s_scale = ( (float) bufsiz / o ) * SCALE_1_TO_1;
    else
	s_scale = SCALE_1_TO_1;
    moncontrol(1);
}

/*
 * Control profiling
 *	profiling is what mcount checks to see if
 *	all the data structures are ready.
 */
void moncontrol (int mode)
{
    if (mode) {
	/* start */
	profil(sbuf + sizeof(struct phdr), ssiz - sizeof(struct phdr),
		(int) _s_lowpc, s_scale);
	_profiling = 0;
    } else {
	/* stop */
	profil((char *)0, 0, 0, 0);
	_profiling = -1;
    }
}

void mcount (void)
{
	register char			*selfpc;
	register unsigned short		*frompcindex;
	register struct tostruct	*top;
	register struct tostruct	*prevtop;
	register long			toindex;

	/*
	 *	check that we are profiling
	 *	and that we aren't recursively invoked.
	 */
	if (_profiling) {
		goto out;
	}
	_profiling++;

	/*
	 *	find the return address for mcount,
	 *	and the return address for mcount's caller.
	 */
	asm("	movl a6@,%0" : "=a" (selfpc));	/* Get parents frame pointer */
	asm("	movl %1@(4),%0" : "=a" (frompcindex) : "a" (selfpc));
	asm("	movl a6@(4),%0" : "=a" (selfpc));

	/*
	 *	check that frompcindex is a reasonable pc value.
	 *	for example:	signal catchers get called from the stack,
	 *			not from text space.  too bad.
	 */
	frompcindex = (unsigned short *)((long)frompcindex - (long)_s_lowpc);
	if ((unsigned long)frompcindex > _s_textsize) {
		frompcindex = 0;
	}
	frompcindex =
	    &_froms[((long)frompcindex) / (HASHFRACTION * sizeof(*_froms))];
	toindex = *frompcindex;
	if (toindex == 0) {
		/*
		 *	first time traversing this arc
		 */
		toindex = ++_tos[0].link;
		if (toindex >= _tolimit) {
			goto overflow;
		}
		*frompcindex = toindex;
		top = &_tos[toindex];
		top->selfpc = selfpc;
		top->count = 1;
		top->link = 0;
		goto done;
	}
	top = &_tos[toindex];
	if (top->selfpc == selfpc) {
		/*
		 *	arc at front of chain; usual case.
		 */
		top->count++;
		goto done;
	}
	/*
	 *	have to go looking down chain for it.
	 *	top points to what we are looking at,
	 *	prevtop points to previous top.
	 *	we know it is not at the head of the chain.
	 */
	for (; /* goto done */; ) {
		if (top->link == 0) {
			/*
			 *	top is end of the chain and none of the chain
			 *	had top->selfpc == selfpc.
			 *	so we allocate a new tostruct
			 *	and link it to the head of the chain.
			 */
			toindex = ++_tos[0].link;
			if (toindex >= _tolimit) {
				goto overflow;
			}
			top = &_tos[toindex];
			top->selfpc = selfpc;
			top->count = 1;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}
		/*
		 *	otherwise, check the next arc on the chain.
		 */
		prevtop = top;
		top = &_tos[top->link];
		if (top->selfpc == selfpc) {
			/*
			 *	there it is.
			 *	increment its count
			 *	move it to the head of the chain.
			 */
			top->count++;
			toindex = prevtop->link;
			prevtop->link = top->link;
			top->link = *frompcindex;
			*frompcindex = toindex;
			goto done;
		}

	}
done:
	_profiling--;
	/* and fall through */
out:
#ifdef SAVE_A1
	asm(" movl %0,a1" : /* no outputs */ : "a" (save_a1));
#endif
	return;

overflow:
	_profiling++; /* halt further profiling */
#   define	TOLIMIT	"mcount: tos overflow\n"
	write(2, TOLIMIT, sizeof(TOLIMIT));
	goto out;
}

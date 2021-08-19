/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)gprof.h	5.1 (Berkeley) 6/4/85
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <a.out.h>
#include "gmon.h"

#if vax
#   include "vax.h"
#endif
#if NeXT
#    include "next.h"
#endif

#include <sys/loader.h>
#include "mach_o.h"


    /*
     *	who am i, for error messages.
     */
char	*whoami;

    /*
     * booleans
     */
typedef int	bool;
#define	FALSE	0
#define	TRUE	1

    /*
     *	ticks per second
     */
long	hz;

#ifdef	NeXT_MOD
#define	UNIT short		/* unit of profiling */
#else	NeXT_MOD
typedef	short UNIT;		/* unit of profiling */
#endif	NeXT_MOD
char	*a_outname;
#define	A_OUTNAME		"a.out"

char	*gmonname;
#define	GMONNAME		"gmon.out"
#define	GMONSUM			"gmon.sum"
	
    /*
     *	blurbs on the flat and graph profiles.
     */
#define	FLAT_BLURB	"/usr/lib/gprof.flat"
#define	CALLG_BLURB	"/usr/lib/gprof.callg"

    /*
     *	a constructed arc,
     *	    with pointers to the namelist entry of the parent and the child,
     *	    a count of how many times this arc was traversed,
     *	    and pointers to the next parent of this child and
     *		the next child of this parent.
     */
struct arcstruct {
    struct nl		*arc_parentp;	/* pointer to parent's nl entry */
    struct nl		*arc_childp;	/* pointer to child's nl entry */
    long		arc_count;	/* how calls from parent to child */
    double		arc_time;	/* time inherited along arc */
    double		arc_childtime;	/* childtime inherited along arc */
    struct arcstruct	*arc_parentlist; /* parents-of-this-child list */
    struct arcstruct	*arc_childlist;	/* children-of-this-parent list */
};
typedef struct arcstruct	arctype;

    /*
     * The symbol table;
     * for each external in the specified file we gather
     * its address, the number of calls and compute its share of cpu time.
     */
struct nl {
    char		*name;		/* the name */
    unsigned long	value;		/* the pc entry point */
    unsigned long	svalue;		/* entry point aligned to histograms */
    double		time;		/* ticks in this routine */
    double		childtime;	/* cumulative ticks in children */
    long		ncall;		/* how many times called */
    long		selfcalls;	/* how many calls to self */
    double		propfraction;	/* what % of time propagates */
    double		propself;	/* how much self time propagates */
    double		propchild;	/* how much child time propagates */
    bool		printflag;	/* should this be printed? */
    int			index;		/* index in the graph list */
    int			toporder;	/* graph call chain top-sort order */
    int			cycleno;	/* internal number of cycle on */
    struct nl		*cyclehead;	/* pointer to head of cycle */
    struct nl		*cnext;		/* pointer to next member of cycle */
    arctype		*parents;	/* list of caller arcs */
    arctype		*children;	/* list of callee arcs */
};
typedef struct nl	nltype;

nltype	*nl;			/* the whole namelist */
nltype	*npe;			/* the virtual end of the namelist */
int	nname;			/* the number of function names */

int n_files;
struct file { 
  unsigned long firstpc, lastpc; 
  char *name, *what_name; } 
*files;

    /*
     *	flag which marks a nl entry as topologically ``busy''
     *	flag which marks a nl entry as topologically ``not_numbered''
     */
#define	DFN_BUSY	-1
#define	DFN_NAN		0

    /* 
     *	namelist entries for cycle headers.
     *	the number of discovered cycles.
     */
nltype	*cyclenl;		/* cycle header namelist */
int	ncycle;			/* number of cycles discovered */

    /*
     * The header on the gmon.out file.
     * gmon.out consists of one of these headers,
     * and then an array of ncnt samples
     * representing the discretized program counter values.
     *	this should be a struct phdr, but since everything is done
     *	as UNITs, this is in UNITs too.
     */
struct hdr {
    UNIT	*lowpc;
    UNIT	*highpc;
    int	ncnt;
};

struct hdr	h;

int	debug;

    /*
     * Each discretized pc sample has
     * a count of the number of samples in its range
     */
unsigned UNIT	*samples;

unsigned long	s_lowpc;	/* lowpc from the profile file */
unsigned long	s_highpc;	/* highpc from the profile file */
unsigned lowpc, highpc;		/* range profiled, in UNIT's */
unsigned sampbytes;		/* number of bytes of samples */
int	nsamples;		/* number of samples */
double	actime;			/* accumulated time thus far for putprofline */
double	totime;			/* total time for all routines */
double	printtime;		/* total of time being printed */
double	scale;			/* scale factor converting samples to pc
				   values: each sample covers scale bytes */
char	*strtab;		/* string table in core */
off_t	ssiz;			/* size of the string table */
struct	exec xbuf;		/* exec header of a.out */
unsigned char	*textspace;		/* text space of a.out in core */

    /*
     *	option flags, from a to z.
     */
bool	aflag;				/* suppress static functions */
bool	bflag;				/* blurbs, too */
bool	cflag;				/* discovered call graph, too */
bool	dflag;				/* debugging options */
bool	eflag;				/* specific functions excluded */
bool	Eflag;				/* functions excluded with time */
bool	fflag;				/* specific functions requested */
bool	Fflag;				/* functions requested with time */
bool	sflag;				/* sum multiple gmon.out files */
bool	Sflag;				/* produce order file for scatter loading */
bool	zflag;				/* zero time/called functions, too */

    /*
     *	structure for various string lists
     */
struct stringlist {
    struct stringlist	*next;
    char		*string;
};
struct stringlist	*elist;
struct stringlist	*Elist;
struct stringlist	*flist;
struct stringlist	*Flist;

    /*
     *	function declarations
     */
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		addarc();
int		arccmp();
arctype		*arclookup();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		asgnsamples();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		printblurb();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		cyclelink();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		dfn();
bool		dfn_busy();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		dfn_findcycle();
bool		dfn_numbered();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		dfn_post_visit();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		dfn_pre_visit();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		dfn_self_cycle();
nltype		**doarcs();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		done();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		findcalls();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		flatprofheader();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		flatprofline();
bool		funcsymbol();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		getnfile();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		getpfile();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		getstrtab();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		getsymtab();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		gettextspace();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		gprofheader();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		gprofline();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		main();
unsigned long	max();
int		membercmp();
unsigned long	min();
nltype		*nllookup();
FILE		*openpfile();
long		operandlength();
char		*operandname();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		printchildren();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		printcycle();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		printgprof();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		printmembers();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		printname();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		printparents();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		printprof();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		readsamples();
unsigned long	reladdr();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		sortchildren();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		sortmembers();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		sortparents();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		tally();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		timecmp();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		topcmp();
int		totalcmp();
#ifdef	NeXT_MOD
int
#endif	NeXT_MOD
		valcmp();

#define	LESSTHAN	-1
#define	EQUALTO		0
#define	GREATERTHAN	1

#define	DFNDEBUG	1
#define	CYCLEDEBUG	2
#define	ARCDEBUG	4
#define	TALLYDEBUG	8
#define	TIMEDEBUG	16
#define	SAMPLEDEBUG	32
#define	AOUTDEBUG	64
#define	CALLSDEBUG	128
#define	LOOKUPDEBUG	256
#define	PROPDEBUG	512
#define	ANYDEBUG	1024

/* Mach-O format?  if false, a.out */
bool mach_o;

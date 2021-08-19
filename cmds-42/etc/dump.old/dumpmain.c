/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)dumpmain.c	5.4 (Berkeley) 5/28/86";
#endif not lint

#include "dump.h"

/*
 * TODO
 * 1.   Dump should have some file that it consults that contain
 *	inode numbers, names of files and directories, and names of users
 *	and/or groups to *NOT* dump to OD/tape.  This feature would come 
 *	in especially handy to not dump /private/vm/swapfile, 
 *	/usr/man/cat?, /NextLibrary, /NextApps.
 *
 * 2.	Rewrite dump/restore to be media and machine independent.  
 *	Would be nice to dump all the machines on the network to 
 *	a few servers that have Exabyte tape drives with an 
 *	easy-to-use application front end.
 *
 *		Morris Meyer, 8/21/89
 */


int	notify = 0;	/* notify operator flag */
int	blockswritten = 0;	/* number of blocks written on current tape */
int	tapeno = 0;	/* current tape number */
int	density = 0;	/* density in bytes/0.1" */
int	ntrec = NTREC;	/* # tape blocks in each tape record */
int	cartridge = 0;	/* Assume non-cartridge tape */
#ifdef NeXT_MOD
int	optical = 0;	/* 
			 * Assume dumping to magtape still.  OD's still 
			 * not economical to do backups to.  Maybe a year
			 * from now.  MAM 8/14/89
			 */
int	diskfd	= -1;	/* 
			 * File descriptor for DKIOCGLABEL.
			 */
struct disktab *dtab, disktab;
struct disktab *getdiskbyname (char *name);
char	*opticalsizestr;
int	opticalsize = 0;
#endif NeXT_MOD		
#ifdef RDUMP
char	*host;
#endif
int	anydskipped;	/* set true in mark() if any directories are skipped */
			/* this lets us avoid map pass 2 in some cases */

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char		*arg;
	int		bflag = 0, i;
	float		fetapes;
	register	struct	fstab	*dt;

#ifdef NeXT_MOD
	dtab = &disktab;
#endif NeXT_MOD
	time(&(spcl.c_date));

	tsize = 0;	/* Default later, based on 'c' option for cart tapes */
	tape = TAPE;
	disk = DISK;
	increm = NINCREM;
	temp = TEMP;
	if (TP_BSIZE / DEV_BSIZE == 0 || TP_BSIZE % DEV_BSIZE != 0) {
		msg("TP_BSIZE must be a multiple of DEV_BSIZE\n");
		dumpabort();
	}
	incno = '9';
	uflag = 0;
	arg = "u";
	if(argc > 1) {
		argv++;
		argc--;
		arg = *argv;
		if (*arg == '-')
			argc++;
	}
	while(*arg)
	switch (*arg++) {
	case 'w':
		lastdump('w');		/* tell us only what has to be done */
		exit(0);
		break;
	case 'W':			/* what to do */
		lastdump('W');		/* tell us the current state of what has been done */
		exit(0);		/* do nothing else */
		break;

	case 'f':			/* output file */
		if(argc > 1) {
			argv++;
			argc--;
			tape = *argv;
		}
		break;

	case 'd':			/* density, in bits per inch */
		if (argc > 1) {
			argv++;
			argc--;
			density = atoi(*argv) / 10;
			if (density >= 625 && !bflag)
				ntrec = HIGHDENSITYTREC;
		}
		break;

	case 's':			/* tape size, feet */
		if(argc > 1) {
			argv++;
			argc--;
			tsize = atol(*argv);
			tsize *= 12L*10L;
		}
		break;

	case 'b':			/* blocks per tape write */
		if(argc > 1) {
			argv++;
			argc--;
			bflag++;
			ntrec = atol(*argv);
		}
		break;

	case 'c':			/* Tape is cart. not 9-track */
		cartridge++;
		break;
#ifdef NeXT_MOD
	case 'o':			/* Tape is optical not 9-track */
		optical++;
		cartridge = 0;		/* Kludge */
		/*
		 * Optimum write size on the OD is a multiple of 8k.
		 * Since TP_BSIZE is 1024, when move ntrec from 10 downto 8.
		 */
		ntrec = 8;		
		tape = "/dev/rod0a";
		break;
	case 'O':
		optical++;
		cartridge = 0;		/* Kludge */
		/*
		 * Optimum write size on the OD is a multiple of 8k.
		 * Since TP_BSIZE is 1024, when move ntrec from 10 downto 8.
		 */
		ntrec = 8;		
		if(argc > 1) {
			argv++;
			argc--;
			/* Convert from meg to optical records. */
			opticalsize = atol(*argv) * 1024;
		}
		break;
#endif NeXT_MOD
	case '0':			/* dump level */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		incno = arg[-1];
		break;

	case 'u':			/* update /etc/dumpdates */
		uflag++;
		break;

	case 'n':			/* notify operators */
		notify++;
		break;

	default:
		fprintf(stderr, "bad key '%c%'\n", arg[-1]);
		Exit(X_ABORT);
	}
	if(argc > 1) {
		argv++;
		argc--;
		disk = *argv;
	}
	if (strcmp(tape, "-") == 0) {
		pipeout++;
		tape = "standard output";
	}
#ifdef NeXT_MOD
	if (optical && cartridge) {
msg("Choose either \'o\' flag for optical or \'c\' flag for cartridge\n");
		dumpabort();
	}
	if (optical && density) {
		msg("Optical flag specified: Do not give tape density\n");
		dumpabort();
	}
	if (opticalsize == 0)  {
		/*
		 * SECTORS/DISK = NCYL/DISK * NTRACK/CYL * NSECT/TRACK
		 */
		opticalsize = (1029 * 15 * 16) - 3500;
	}
#endif NeXT_MOD

	/*
	 * Determine how to default tape size and density
	 *
	 *         	density				tape size
	 * 9-track	1600 bpi (160 bytes/.1")	2300 ft.
	 * 9-track	6250 bpi (625 bytes/.1")	2300 ft.
 	 * cartridge	8000 bpi (100 bytes/.1")	1700 ft. (450*4 - slop)
	 */
#ifdef NeXT_MOD
	/*
	 * "tsize" is the number of "density" amounts that fit on the tape.
	 * Since we are doing this to an optical disk, I am using optical
	 * disk tracks as the "density" amount and number of tracks as
	 * "tsize".  Until someone invests the time to do a multi-media
	 * (no pun intended) dump/restore, this abortion will have to suffice.
	 * 	MAM  8/14/89
	 */
	if (density == 0)
		if (cartridge)
			density = 100;
		else if (optical)
			density = DEV_BSIZE;
		else 
			density = 160;	/* Default to 9-track, 1600bpi */
	if (tsize == 0)
		if (cartridge)
			tsize = 1700L*120L;
		else if (optical)
			tsize = opticalsize;
		else
			tsize = 2300L*120L;  /* Default to 9-track, 2300ft */
#else
	if (density == 0)
		density = cartridge ? 100 : 160;
	if (tsize == 0)
 		tsize = cartridge ? 1700L*120L : 2300L*120L;
#endif NeXT_MOD
#ifdef RDUMP
	{ char *index();
	  host = tape;
	  tape = index(host, ':');
	  if (tape == 0) {
#ifdef NeXT_MOD
		msg("need keyletter ``f'' and device ``host:%s''\n",
			(optical ? "optical" : "tape"));
#else
		msg("need keyletter ``f'' and device ``host:tape''\n");
#endif NeXT_MOD
		exit(1);
	  }
	  *tape++ = 0;
	  if (rmthost(host) == 0)
		exit(X_ABORT);
	}
	setuid(getuid());	/* rmthost() is the only reason to be setuid */
#endif
	if (signal(SIGHUP, sighup) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTRAP, sigtrap) == SIG_IGN)
		signal(SIGTRAP, SIG_IGN);
#ifndef DEBUG
	if (signal(SIGFPE, sigfpe) == SIG_IGN)
		signal(SIGFPE, SIG_IGN);
#endif DEBUG
	if (signal(SIGBUS, sigbus) == SIG_IGN)
		signal(SIGBUS, SIG_IGN);
	if (signal(SIGSEGV, sigsegv) == SIG_IGN)
		signal(SIGSEGV, SIG_IGN);
	if (signal(SIGTERM, sigterm) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
	

	if (signal(SIGINT, interrupt) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	set_operators();	/* /etc/group snarfed */
	getfstab();		/* /etc/fstab snarfed */
	/*
	 *	disk can be either the full special file name,
	 *	the suffix of the special file name,
	 *	the special name missing the leading '/',
	 *	the file system name with or without the leading '/'.
	 */
	dt = fstabsearch(disk);
	if (dt != 0)
		disk = rawname(dt->fs_spec);
	getitime();		/* /etc/dumpdates snarfed */

	msg("Date of this level %c dump: %s\n", incno, prdate(spcl.c_date));
 	msg("Date of last level %c dump: %s\n",
		lastincno, prdate(spcl.c_ddate));
	msg("Dumping %s ", disk);
	if (dt != 0)
		msgtail("(%s) ", dt->fs_file);
#ifdef RDUMP
	msgtail("to %s on host %s\n", tape, host);
#else
	msgtail("to %s\n", tape);
#endif

	fi = open(disk, 0);
	if (fi < 0) {
		msg("Cannot open %s\n", disk);
		Exit(X_ABORT);
	}
	esize = 0;
	sblock = (struct fs *)buf;
	sync();
	bread(SBLOCK, sblock, SBSIZE);
	if (sblock->fs_magic != FS_MAGIC) {
		msg("bad sblock magic number\n");
		dumpabort();
	}
	msiz = roundup(howmany(sblock->fs_ipg * sblock->fs_ncg, NBBY),
		TP_BSIZE);
#ifdef DEBUG
	fprintf (stderr, "msiz %d\n", msiz);
#endif DEBUG
	clrmap = (char *)calloc(msiz, sizeof(char));
	dirmap = (char *)calloc(msiz, sizeof(char));
	nodmap = (char *)calloc(msiz, sizeof(char));

	anydskipped = 0;
	msg("mapping (Pass I) [regular files]\n");
	pass(mark, (char *)NULL);		/* mark updates esize */

	if (anydskipped) {
		do {
			msg("mapping (Pass II) [directories]\n");
			nadded = 0;
			pass(add, dirmap);
		} while(nadded);
	} else				/* keep the operators happy */
		msg("mapping (Pass II) [directories]\n");

	bmapest(clrmap);
	bmapest(nodmap);

	if (cartridge) {
		/* Estimate number of tapes, assuming streaming stops at
		   the end of each block written, and not in mid-block.
		   Assume no erroneous blocks; this can be compensated for
		   with an artificially low tape size. */
		fetapes = 
		(	  esize		/* blocks */
			* TP_BSIZE	/* bytes/block */
			* (1.0/density)	/* 0.1" / byte */
		  +
			  esize		/* blocks */
			* (1.0/ntrec)	/* streaming-stops per block */
			* 15.48		/* 0.1" / streaming-stop */
		) * (1.0 / tsize );	/* tape / 0.1" */
#ifdef NeXT_MOD
	}
	else if (optical)  {
		/*
		 * Estimate the number of optical disks, mapping
		 * esize number of TP_BSIZE (1024) blocks onto the
		 * number of optical disk tracks at dl_secsize/track.
		 * dl_secsize should be 1024, as should TP_BSIZE, and
		 * fetapes should == esize / tsize, but the extra math
 		 * is there just in case.
		 *	MAM 8/14/89
		 */
		fetapes = (float)(esize * TP_BSIZE) /	
				(float)(tsize  * density);
#ifdef DEBUG
		fprintf (stderr, "fetapes %f esize %d tsize %d\n", 
			fetapes, esize, tsize);
#endif DEBUG
#endif NeXT_MOD
	} else {
		/* Estimate number of tapes, for old fashioned 9-track tape */
		int tenthsperirg = (density == 625) ? 3 : 7;
		fetapes =
		(	  esize		/* blocks */
			* TP_BSIZE	/* bytes / block */
			* (1.0/density)	/* 0.1" / byte */
		  +
			  esize		/* blocks */
			* (1.0/ntrec)	/* IRG's / block */
			* tenthsperirg	/* 0.1" / IRG */
		) * (1.0 / tsize );	/* tape / 0.1" */
	}
	etapes = fetapes;		/* truncating assignment */
	etapes++;
	/* count the nodemap on each additional tape */
	for (i = 1; i < etapes; i++)
		bmapest(nodmap);
	esize += i + 10;	/* headers + 10 trailer blocks */
#ifdef NeXT_MOD
	msg("estimated %ld %s blocks on %3.2f %s(s).\n", esize,
		(optical ? "optical disk" : "tape"), fetapes,
		(optical ? "optical disk" : "tape"));

#else
	msg("estimated %ld tape blocks on %3.2f tape(s).\n", esize, fetapes);
#endif NeXT_MOD

	alloctape();			/* Allocate tape buffer */

	otape();			/* bitmap is the first to tape write */
	time(&(tstart_writing));
	bitmap(clrmap, TS_CLRI);

	msg("dumping (Pass III) [directories]\n");
 	pass(dirdump, dirmap);

	msg("dumping (Pass IV) [regular files]\n");
	pass(dump, nodmap);

	spcl.c_type = TS_END;
#ifndef RDUMP
	for(i=0; i<ntrec; i++)
		spclrec();
#endif
#ifdef NeXT_MOD
	msg("DUMP: %ld %s blocks on %d %s(s)\n", spcl.c_tapea, 
		(optical ? "optical disk" : "tape"), 
		spcl.c_volume, 
		(optical ? "optical disk" : "tape"));
#else
	msg("DUMP: %ld tape blocks on %d tape(s)\n", spcl.c_tapea, spcl.c_volume);
#endif NeXT_MOD
	msg("DUMP IS DONE\n");

	putitime();
#ifndef RDUMP
	if (!pipeout) {
		close(to);
#ifdef NeXT_MOD
		tape_rewind();
#else
		rewind();
#endif NeXT_MOD
	}
#else
	tflush(1);
#ifdef NeXT_MOD
	tape_rewind();
#else
	rewind();
#endif NeXT_MOD
#endif
	broadcast("DUMP IS DONE!\7\7\n");
	Exit(X_FINOK);
}

int	sighup(){	msg("SIGHUP()  try rewriting\n"); sigAbort();}
int	sigtrap(){	msg("SIGTRAP()  try rewriting\n"); sigAbort();}
int	sigfpe(){	msg("SIGFPE()  try rewriting\n"); sigAbort();}
int	sigbus(){	msg("SIGBUS()  try rewriting\n"); sigAbort();}
int	sigsegv(){	msg("SIGSEGV()  ABORTING!\n"); abort();}
int	sigalrm(){	msg("SIGALRM()  try rewriting\n"); sigAbort();}
int	sigterm(){	msg("SIGTERM()  try rewriting\n"); sigAbort();}

sigAbort()
{
	if (pipeout) {
		msg("Unknown signal, cannot recover\n");
		dumpabort();
	}
	msg("Rewriting attempted as response to unknown signal.\n");
	fflush(stderr);
	fflush(stdout);
	close_rewind();
	exit(X_REWRITE);
}

char *rawname(cp)
	char *cp;
{
	static char rawbuf[32];
	char *rindex();
	char *dp = rindex(cp, '/');

	if (dp == 0)
		return (0);
	*dp = 0;
	strcpy(rawbuf, cp);
	*dp = '/';
	strcat(rawbuf, "/r");
	strcat(rawbuf, dp+1);
	return (rawbuf);
}

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

/*{
Shar puts readable text files together in a package
from which they are easy to extract.  The original version
was a shell script posted to the net, shown below:
	#Date: Mon Oct 18 11:08:34 1982
	#From: decvax!microsof!uw-beave!jim (James Gosling at CMU)
	AR=$1
	shift
	for i do
		echo a - $i
		echo "echo x - $i" >>$AR
		echo "cat >$i <<'!Funky!Stuff!'" >>$AR
		cat $i >>$AR
		echo "!Funky!Stuff!" >>$AR
	done
I rewrote this version in C to provide better diagnostics
and to run faster.  The major difference is that my version
does not affect any files because it prints to the standard
output.  Mine also has several options.

Gary Perlman/Wang Institute/Tyngsboro, MA/01879/(617) 649-9731

Many enhancements motivated by Michael Thompson.

Directory archiving motivated by Derek Zahn @ wisconsin
	His version had some problems, so I wrote a general
	routine for traversing a directory hierarchy.  It
	allows marching through a directory on old and new
	UNIX systems.
}*/

/* COMMANDS */
#define	EXTRACT "#! /bin/sh"     /* magic exec string at shar file start */
#define	PATH    "/bin:$PATH"     /* search path for programs */
#define	CAT     "cat";           /* /bin/cat */
#define	SED     "sed 's/^%s//'"  /* /bin/sed removes Prefix from lines */
#define	MKDIR   "mkdir"          /* make a new dirctory */
#define	CHMOD   "chmod +x"       /* change file protection (for executables) */
#define	CHDIR   "cd"             /* change current directory */
#define	TEST    "test"           /* /bin/test files */
#define	WC_C    "wc -c <"        /* counts chars in file */
#define	ECHO    "echo shar"      /* echo a message to extractor */

main (argc, argv) char **argv;	
	{
	int 	shar ();
	int 	optind;
	if ((optind = initial (argc, argv)) < 0)
		exit (1);
	if (header (argc, argv, optind))
		exit (2);
	while (optind < argc)
		traverse (argv[optind++], shar);
	footer ();
	exit (0);
	}

/*			OPTIONS			*/
typedef	int	lgl;
#define	true	((lgl) 1)
#define	false	((lgl) 0)
int 	Lastchar;   /* the last character printed */
int 	Ctrlcount;  /* how many bad control characters are in file */

#define	USAGE "[-abcsv] [-p prefix] [-d delim] files > archive"
#define	OPTSTRING "abcsvp:d:"

lgl 	Verbose = false;       /* provide append/extract feedback */
lgl 	Basename = false;      /* extract into basenames */
lgl 	Count = false;         /* count characters to check transfer */
lgl 	Silent = false;        /* turn off all verbosity */
char	*Delim = "SHAR_EOF";   /* put after each file */
char	Filter[100] = CAT;     /* used to extract archived files */
char	*Prefix = NULL;        /* line prefix to avoid funny chars */

int /* returns the index of the first operand file */
initial (argc, argv) char **argv;
	{
	int 	errflg = 0;
	extern	int 	optind;
	extern	char	*optarg;
	int 	C;
	while ((C = getopt (argc, argv, OPTSTRING)) != EOF)
		switch (C)
			{
			case 'v': Verbose = true; break;
			case 'c': Count = true; break;
			case 'b': Basename = true; break;
			case 'd': Delim = optarg; break;
			case 's': /* silent running */
				Silent = true;
				Verbose = false;
				Count = false;
				Prefix = NULL;
				break;
			case 'a': /* all the options */
				Verbose = true;
				Count = true;
				Basename = true;
				/* fall through to set prefix */
				optarg = "	X";
			case 'p': (void) sprintf (Filter, SED, Prefix = optarg); break;
			default: errflg++;
			}
	if (errflg || optind == argc)
		{
		if (optind == argc)
			fprintf (stderr, "shar: No input files\n");
		fprintf (stderr, "USAGE: shar %s\n", USAGE);
		return (-1);
		}
	return (optind);
	}

header (argc, argv, optind)
char	**argv;
	{
	int 	i;
	lgl 	problems = false;
	long	clock;
	char	*ctime ();
	char	*getenv ();
	char	*NAME = getenv ("NAME");
	char	*ORG = getenv ("ORGANIZATION");
	for (i = optind; i < argc; i++)
		if (access (argv[i], 4)) /* check read permission */
			{
			fprintf (stderr, "shar: Can't read '%s'\n", argv[i]);
			problems++;
			}
	if (problems) return (problems);
	/*	I have given up on putting a cut line in the archive.
		Too many people complained about having to remove it.
		puts ("-----cut here-----cut here-----cut here-----cut here-----");
	*/
	puts (EXTRACT);
	puts ("# This is a shell archive, meaning:");
	printf ("# 1. Remove everything above the %s line.\n", EXTRACT);
	puts ("# 2. Save the resulting text in a file.");
	puts ("# 3. Execute the file with /bin/sh (not csh) to create the files:");
	for (i = optind; i < argc; i++)
		printf ("#\t%s\n", argv[i]);
	(void) time (&clock);
	printf ("# This archive created: %s", ctime (&clock));
	if (NAME)
		printf ("# By:\t%s (%s)\n", NAME, ORG ? ORG : "");
	printf ("export PATH; PATH=%s\n", PATH);
	return (0);
	}

footer ()
	{
	puts ("#\tEnd of shell archive");
	puts ("exit 0");
	}

archive (input, output)
char	*input, *output;
	{
	char	buf[BUFSIZ];
	FILE	*ioptr;
	if (ioptr = fopen (input, "r"))
		{
		if (Count == true)
			{
			Ctrlcount = 0;    /* no bad control characters so far */
			Lastchar = '\n';  /* simulate line start */
			}
		printf ("%s << \\%s > '%s'\n", Filter, Delim, output);
		if (Prefix)
			{
			while (fgets (buf, BUFSIZ, ioptr))
				{
				if (Prefix) outline (Prefix);
				outline (buf);
				}
			}
		else copyout (ioptr);
		/* thanks to H. Morrow Long (ittvax!long) for the next fix */
		if (Lastchar != '\n') /* incomplete last line */
			putchar ('\n');   /* Delim MUST begin new line! */
		puts (Delim);
		if (Count == true && Lastchar != '\n')
			printf ("%s: a missing newline was added to \"'%s'\"\n", ECHO, input);
		if (Count == true && Ctrlcount)
			printf ("%s: %d control character%s may be missing from \"'%s'\"\n",
				ECHO, Ctrlcount, Ctrlcount > 1 ? "s" : "", input);
		(void) fclose (ioptr);
		return (0);
		}
	else
		{
		fprintf (stderr, "shar: Can't open '%s'\n", input);
		return (1);
		}
	}

/*
	Copyout copies its ioptr almost as fast as possible
	except that it has to keep track of the last character
	printed.  If the last character is not a newline, then
	shar has to add one so that the end of file delimiter
	is recognized by the shell.  This checking costs about
	a 10% difference in user time.  Otherwise, it is about
	as fast as cat.  It also might count control characters.
*/
#define	badctrl(c) (iscntrl (c) && !isspace (c))
copyout (ioptr)
register	FILE	*ioptr;
	{
	register	int 	C;
	register	int 	L;
	register	count;
	count = Count;
	while ((C = getc (ioptr)) != EOF)
		{
		if (count == true && badctrl (C)) Ctrlcount++;
		L = putchar (C);
		}
	Lastchar = L;
	}

outline (s)
register	char	*s;
	{
	if (*s)
		{
		while (*s)
			{
			if (Count == true && badctrl (*s)) Ctrlcount++;
			putchar (*s++);
			}
		Lastchar = *(s-1);
		}
	}

#define	FSIZE     statbuf.st_size
shar (file, type, pos)
char	*file;     /* file or directory to be processed */
int 	type;      /* either 'f' for file or 'd' for directory */
int 	pos;       /* 0 going in to a file or dir, 1 going out */
	{
	struct	stat	statbuf;
	char	*basefile = file;
	if (!strcmp (file, ".")) return;
	if (stat (file, &statbuf)) FSIZE = 0;
	if (Basename == true)
		{
		while (*basefile) basefile++; /* go to end of name */
		while (basefile > file && *(basefile-1) != '/') basefile--;
		}
	if (pos == 0) /* before the file starts */
		{
		if (type == 'd')
			{
			printf ("if %s ! -d '%s'\n", TEST, basefile);
			printf ("then\n");
			if (Verbose == true)
				printf ("	%s: creating directory \"'%s'\"\n", ECHO, basefile);
			printf ("	%s '%s'\n", MKDIR, basefile);
			printf ("fi\n");
			if (Verbose == true)
				printf ("%s: entering directory \"'%s'\"\n", ECHO, basefile);
			printf ("%s '%s'\n", CHDIR, basefile);
			}
		else /* type == 'f' */
			{
			if (Verbose == true)
				printf ("%s: extracting \"'%s'\" '(%d character%s)'\n",
					ECHO, basefile, FSIZE, FSIZE > 1 ? "s" : "");
			if (Silent == false) /* this solution by G|ran Uddeborg */
				{
				printf ("if %s -f '%s'\n", TEST, basefile);
				puts ("then");
				printf ("	%s: will not over-write existing file \"'%s'\"\n",
					ECHO, basefile);
				puts ("else");
				}
			if (archive (file, basefile)) exit (-1);
			}
		}
	else /* pos == 1, after the file is archived */
		{
		if (type == 'd')
			{
			if (Verbose == true)
				printf ("%s: done with directory \"'%s'\"\n", ECHO, basefile);
			printf ("%s ..\n", CHDIR);
			}
		else /* type == 'f' (plain file) */
			{
			if (Count == true)
				{
				printf ("if %s %d -ne \"`%s '%s'`\"\n",
					TEST, FSIZE, WC_C, basefile);
				puts ("then");
				printf ("	%s: error transmitting \"'%s'\" ", ECHO, basefile);
				printf ("'(should have been %d character%s)'\n",
					FSIZE, FSIZE > 1 ? "s" : "");
				puts ("fi");
				}
			if (access (file, 1) == 0) /* executable -> chmod +x */
				printf ("%s '%s'\n", CHMOD, basefile);
			if (Silent == false)
				{
				puts ("fi # end of overwriting check");
				}
			}
		}
	}

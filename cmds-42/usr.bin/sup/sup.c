/*
 *	sup -- Software Upgrade Protocol client process
 *
 *	Usage:  sup [ flags ] [ supfile ] [ collection ... ]
 *
 *	The only required argument to sup is the name of a supfile.  It
 *	must either be given explicitly on the command line, or the -s
 *	flag must be specified.  If the -s flag is given, the system
 *	supfile will be used and a supfile command argument should not be
 *	specified.  The list of collections is optional and if specified
 *	will be the only collections upgraded.  The following flags affect
 *	all collections specified.
 *
 *	-s	"system upgrade" flag
 *			As described above.
 *
 *	-t	"upgrade time" flag
 *			When this flag is given, Sup will print the time
 *			that each collection was last upgraded, rather than
 *			performing actual upgrades.
 *
 *	-N	"debug network" flag
 *			Sup will trace messages sent and received that
 *			implement the Sup network protocol.
 *
 *	-P	"debug ports" flag
 *	    		Sup will use a set of non-privileged network
 *			ports reserved for debugging purposes.
 *
 *	The remaining flags affect all collections unless an explicit list
 *	of collections are given with the flags.  Multiple flags may be
 *	specified together that affect the same collections.  For the sake
 *	of convience, any flags that always affect all collections can be
 *	specified with flags that affect only some collections.  For
 *	example, "sup -sde=coll1,coll2" would perform a system upgrade,
 *	and the first two collections would allow both file deletions and
 *	command executions.  Note that this is not the same command as
 *	"sup -sde=coll1 coll2", which would perform a system upgrade of
 *	just the coll2 collection and would ignore the flags given for the
 *	coll1 collection.
 *
 *	-a	"all files" flag
 *			All files in the collection will be copied from
 *			the repository, regardless of their status on the
 *			current machine.  Because of this, it is a very
 *			expensive operation and should only be done for
 *			small collections if data corruption is suspected
 *			and been confirmed.  In most cases, the -o flag
 *			should be sufficient.
 *
 *	-b	"backup files" flag
 *			If the -b flag if given, or the "backup" supfile
 *			option is specified, the contents of regular files
 *			on the local system will be saved before they are
 *			overwritten with new data.  The data will be saved
 *			in a subdirectory called "BACKUP" in the directory
 *			containing the original version of the file, in a
 *			file with the same non-directory part of the file
 *			name.  The files to backup are specified by the
 *			list file on the repository.
 *
 *	-B	"don't backup files" flag
 *			The -B flag overrides and disables the -b flag and
 *			the "backup" supfile option.
 *
 *	-d	"delete files" flag
 *			Files that are no longer in the collection on the
 *			repository will be deleted if present on the local
 *			machine.  This may also be specified in a supfile
 *			with the "delete" option.
 *
 *	-D	"don't delete files" flag
 *			The -D flag overrides and disables the -d flag and
 *			the "delete" supfile option.
 *
 *	-e	"execute files" flag
 *			Sup will execute commands sent from the repository
 *			that should be run when a file is upgraded.  If
 *			the -e flag is omitted, Sup will print a message
 *			that specifies the command to execute.  This may
 *			also be specified in a supfile with the "execute"
 *			option.
 *
 *	-E	"don't execute files" flag
 *			The -E flag overrides and disables the -e flag and
 *			the "execute" supfile option.
 *
 *	-f	"file listing" flag
 *			A "list-only" upgrade will be performed.  Messages
 *			will be printed that indicate what would happen if
 *			an actual upgrade were done.
 *
 *	-l	"local upgrade" flag
 *			Normally, Sup will not upgrade a collection if the
 *			repository is on the same machine.  This allows
 *			users to run upgrades on all machines without
 *			having to make special checks for the repository
 *			machine.  If the -l flag is specified, collections
 *			will be upgraded even if the repository is local.
 *
 *	-m	"mail" flag
 *			Normally, Sup used standard output for messages.
 *			If the -m flag if given, Sup will send mail to the
 *			user running Sup, or a user specified with the
 *			"notify" supfile option, that contains messages
 *			printed by Sup.
 *
 *	-o	"old files" flag
 *			Sup will normally only upgrade files that have
 *			changed on the repository since the last time an
 *			upgrade was performed.  The -o flag, or the "old"
 *			supfile option, will cause Sup to check all files
 *			in the collection for changes instead of just the
 *			new ones.
 *
 *	-O	"not old files" flag
 *			The -O flag overrides and disables the -o flag and
 *			the "old" supfile option.
 *
 *	-v	"verbose" flag
 *			Normally, Sup will only print messages if there
 *			are problems.  This flag causes Sup to also print
 *			messages during normal progress showing what Sup
 *			is doing.
 *
 **********************************************************************
 * HISTORY
 * 19-Sep-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	Changed default supfile name for system collections when -t
 *	is specified to use FILESUPTDEFAULT; added missing new-line
 *	in retry message.
 *
 * 21-Jun-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Missed a caller to a routine which had an extra argument added
 *	to it last edit. [V5.17]
 *
 * 07-Jun-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed getcoll() so that fatal errors are checked immediately
 *	instead of after sleeping for a little while.  Changed all
 *	rm -rf commands to rmdir since the Mach folks keep deleting
 *	their root and /usr directory trees.  Reversed the order of
 *	delete commands to that directories will possibly empty so
 *	that the rmdir's work. [V5.16]
 *
 * 30-May-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed temporary file names to #n.sup format. [V5.15]
 *
 * 19-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Moved PGMVERSION to supvers.c module. [V5.14]
 *
 * 06-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added check for file type before unlink when receiving a
 *	symbolic link.  Now runs "rm -rf" if the file type is a
 *	directory. [V5.13]
 *
 * 03-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed small bug in signon that didn't retry connections if an
 *	error occured on the first attempt to connect. [V5.12]
 *
 * 26-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	New command interface.  Added -bBDEO flags and "delete",
 *	"execute" and "old" supfile options.  Changed -d to work
 *	correctly without implying -o. [V5.11]
 *
 * 21-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fix incorrect check for supfile changing.  Flush output buffers
 *	before restart. [V5.10]
 *
 * 17-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Add call to requestend() after connection errors are retried to
 *	free file descriptors. [V5.9]
 *
 * 15-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fix SERIOUS merge error from previous edit.  Added notify
 *	when execute command fails. [V5.8]
 *
 * 11-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed ugconvert to clear setuid/setgid bits if it doesn't use
 *	the user and group specified by the remote system.  Changed
 *	execute code to invalidate collection if execute command returns
 *	with a non-zero exit status.  Added support for execv() of
 *	original arguments of supfile is upgraded sucessfully.  Changed
 *	copyfile to always use a temp file if possible. [V5.7]
 *
 * 04-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added support for fileserver busy messages and new nameserver
 *	protocol to support multiple repositories per collection.
 *	Added code to lock collections with lock files. [V5.6]
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Major rewrite for protocol version 4. [V4.5]
 *
 * 12-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed to check for DIFFERENT mtime (again). [V3.4]
 *
 * 08-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Replaced [ug]convert routines with ugconvert routine so that an
 *	appropriate group will be used if the default user is used.
 *	Changed switch parsing to allow multiple switches to be specified
 *	at the same time. [V3.3]
 *
 * 04-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added test to request a new copy of an old file that already
 *	exists if the mtime is different. [V3.2]
 *
 * 24-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added -l switch to enable upgrades from local repositories.
 *
 * 03-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Minor change in order -t prints so that columns line up.
 *
 * 22-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to implement retry flag and pass this on to request().
 *
 * 22-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Merged 4.1 and 4.2 versions together.
 *
 * 04-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for 4.2 BSD.
 *
 **********************************************************************
 */

#include <libc.h>
#include <netdb.h>
#include <signal.h>
#include <setjmp.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <c.h>
#include "sup.h"
#define MSGNAME
#define MSGFILE
#include "supmsg.h"

extern int errno;
extern int PGMVERSION;

/*******************************************
 ***    D A T A   S T R U C T U R E S    ***
 *******************************************/

struct collstruct {			/* one per collection to be upgraded */
	char *Cname;			/* collection name */
	TREE *Chost;			/* attempted host for collection */
	TREE *Chtree;			/* possible hosts for collection */
	char *Cbase;			/* local base directory */
	char *Chbase;			/* remote base directory */
	char *Cprefix;			/* local collection pathname prefix */
	char *Cnotify;			/* user to notify of status */
	char *Clogin;			/* remote login name */
	char *Cpswd;			/* remote password */
	char *Ccrypt;			/* data encryption key */
	int Cflags;			/* collection flags */
	int Cnogood;			/* upgrade no good, "when" unchanged */
	int Clockfd;			/* >= 0 if collection is locked */
	struct collstruct *Cnext;	/* next collection */
};
typedef struct collstruct COLLECTION;

#define CFALL		00001
#define CFBACKUP	00002
#define CFDELETE	00004
#define CFEXECUTE	00010
#define CFLIST		00020
#define CFLOCAL		00040
#define CFMAIL		00100
#define CFOLD		00200
#define CFVERBOSE	00400

typedef enum {				/* supfile options */
	OHOST, OBASE, OHOSTBASE, OPREFIX,
	ONOTIFY, OLOGIN, OPASSWORD, OCRYPT,
	OBACKUP, ODELETE, OEXECUTE, OOLD,
	ONOEXEC, ONODELETE, ONOOLD
} OPTION;

struct option {
	char *op_name;
	OPTION op_enum;
} options[] = {
	"host",		OHOST,
	"base",		OBASE,
	"hostbase",	OHOSTBASE,
	"prefix",	OPREFIX,
	"notify",	ONOTIFY,
	"login",	OLOGIN,
	"password",	OPASSWORD,
	"crypt",	OCRYPT,
	"backup",	OBACKUP,
	"delete",	ODELETE,
	"execute",	OEXECUTE,
	"old",		OOLD,
	"noexec",	ONOEXEC,
	"nodelete",	ONODELETE,
	"noold",	ONOOLD
};

struct liststruct {		/* uid and gid lists */
	char *Lname;		/* name */
	int Lnumber;		/* uid or gid */
	struct liststruct *Lnext;
};
typedef struct liststruct LIST;

/*************************
 ***	M A C R O S    ***
 *************************/

#define HASHBITS	4
#define HASHSIZE	(1<<HASHBITS)
#define HASHMASK	(HASHSIZE-1)
#define LISTSIZE	(HASHSIZE*HASHSIZE)

#define vnotify	if (thisC->Cflags&CFVERBOSE)  notify

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

char program[] = "SUP";			/* program name for SCM messages */
int progpid = -1;			/* and process id */

static LIST *uidL[LISTSIZE];		/* uid and gid lists */
static LIST *gidL[LISTSIZE];

COLLECTION *firstC,*thisC;		/* collection list pointer */
TREE *lastT;				/* last filenames in collection */
jmp_buf sjbuf;				/* jump location for network errors */

int sysflag;				/* system upgrade flag */
int timeflag;				/* print times flag */
extern int scmdebug;			/* SCM debugging flag */
int portdebug;				/* network debugging ports */

/*************************************
 ***    M A I N   R O U T I N E    ***
 *************************************/

main (argc,argv)
int argc;
char **argv;
{
	char *init ();
	char *progname,*supfname;
	int restart,sfdev,sfino,sfmtime;
	struct stat sbuf;

	/* initialize global variables */
	pgmversion = PGMVERSION;	/* export version number */
	server = FALSE;			/* export that we're not a server */
	collname = NULL;		/* no current collection yet */
	errstr = NULL;			/* and no errors yet */
	sjbuf[0] = 0;			/* clear setjmp buffer */
	progname = salloc (argv[0]);

	supfname = init (argc,argv);
	restart = -1;			/* don't make restart checks */
	if (*progname == '/' && *supfname == '/') {
		if (stat (supfname,&sbuf) < 0)
			printf ("SUP: Can't stat supfile %s\n",supfname);
		else {
			sfdev = sbuf.st_dev;
			sfino = sbuf.st_ino;
			sfmtime = sbuf.st_mtime;
			restart = 0;
		}
	}
	if (timeflag) {
		for (thisC = firstC; thisC; thisC = thisC->Cnext)
			prtime ();
	} else {
		signal (SIGPIPE,SIG_IGN); /* ignore network pipe signals */
		getnams ();		/* find unknown repositories */
		for (thisC = firstC; thisC; thisC = thisC->Cnext) {
			getcoll ();	/* upgrade each collection */
			if (restart == 0) {
				if (stat (supfname,&sbuf) < 0)
					printf ("SUP Can't stat supfile %s\n",supfname);
				else if (sfmtime != sbuf.st_mtime ||
					 sfino != sbuf.st_ino ||
					 sfdev != sbuf.st_dev) {
					restart = 1;
					break;
				}
			}
		}
		endpwent ();		/* close /etc/passwd */
		endgrent ();		/* close /etc/group */
		Lfree (uidL);
		Lfree (gidL);
		if (restart == 1) {
			int fd;
			printf ("SUP Restarting %s with new supfile %s\n",
				progname,supfname);
			fflush (stdout);
			fflush (stderr);
			for (fd = getdtablesize (); fd > 3; fd--)
				(void) close (fd);
			execv (progname,argv);
			quit (1,"SUP: Restart failed\n");
		}
	}
	while (thisC = firstC) {
		firstC = firstC->Cnext;
		free (thisC->Cname);
		Tfree (&thisC->Chtree);
		free (thisC->Cbase);
		if (thisC->Chbase)  free (thisC->Chbase);
		if (thisC->Cprefix)  free (thisC->Cprefix);
		if (thisC->Cnotify)  free (thisC->Cnotify);
		if (thisC->Clogin)  free (thisC->Clogin);
		if (thisC->Cpswd)  free (thisC->Cpswd);
		if (thisC->Ccrypt)  free (thisC->Ccrypt);
		free ((char *)thisC);
	}
	exit (0);
}

/*************************************************
 ***    P R I N T   U P D A T E   T I M E S    ***
 *************************************************/

prtime ()
{
	char buf[STRINGLENGTH];
	int twhen,f;

	if (chdir (thisC->Cbase) < 0) {
		printf ("Can't change to base directory %s for collection %s\n",
			thisC->Cbase,thisC->Cname);
		fflush (stdout);
	}
	sprintf (buf,FILEWHEN,thisC->Cname);
	f = open (buf,O_RDONLY,0);
	if (f >= 0) {
		if (read(f,(char *)&twhen,sizeof(int)) != sizeof(int))
			twhen = 0;
		close (f);
	} else
		twhen = 0;
	strcpy (buf,ctime (&twhen));
	buf[strlen(buf)-1] = '\0';
	printf ("Last update occurred at %s for collection %s\n",
		buf,thisC->Cname);
	fflush (stdout);
}

/*****************************************
 ***    I N I T I A L I Z A T I O N    ***
 *****************************************/
/* Set up collection list from supfile.  Check all fields except
 * hostname to be sure they make sense.
 */

#define Toflags	Tflags
#define Taflags	Tmode
#define Twant	Tuid
#define Tcount	Tgid

doswitch (argp,collTp,oflagsp,aflagsp)
char *argp;
register TREE **collTp;
int *oflagsp,*aflagsp;
{
	register TREE *t;
	register char *coll;
	register int oflags,aflags;

	oflags = aflags = 0;
	for (;;) {
		switch (*argp) {
		default:
			printf ("SUP: Invalid flag '%c' ignored\n",*argp);
			break;
		case '\0':
		case '=':
			if (*argp++ == '\0' || *argp == '\0') {
				*oflagsp |= oflags;
				*oflagsp &= ~aflags;
				*aflagsp |= aflags;
				*aflagsp &= ~oflags;
				return;
			}
			do {
				coll = nxtarg (&argp,", \t");
				t = Tinsert (collTp,coll,TRUE);
				t->Toflags |= oflags;
				t->Toflags &= ~aflags;
				t->Taflags |= aflags;
				t->Taflags &= ~oflags;
				argp = skipover (argp,", \t");
			} while (*argp);
			return;
		case 's':
			sysflag = TRUE;
			break;
		case 't':
			timeflag = TRUE;
			break;
		case 'N':
			scmdebug++;
			break;
		case 'P':
			portdebug = TRUE;
			break;
		case 'a':
			oflags |= CFALL;
			break;
		case 'b':
			oflags |= CFBACKUP;
			aflags &= ~CFBACKUP;
			break;
		case 'B':
			oflags &= ~CFBACKUP;
			aflags |= CFBACKUP;
			break;
		case 'd':
			oflags |= CFDELETE;
			aflags &= ~CFDELETE;
			break;
		case 'D':
			oflags &= ~CFDELETE;
			aflags |= CFDELETE;
			break;
		case 'e':
			oflags |= CFEXECUTE;
			aflags &= ~CFEXECUTE;
			break;
		case 'E':
			oflags &= ~CFEXECUTE;
			aflags |= CFEXECUTE;
			break;
		case 'f':
			oflags |= CFLIST;
			break;
		case 'l':
			oflags |= CFLOCAL;
			break;
		case 'm':
			oflags |= CFMAIL;
			break;
		case 'o':
			oflags |= CFOLD;
			aflags &= ~CFOLD;
			break;
		case 'O':
			oflags &= ~CFOLD;
			aflags |= CFOLD;
			break;
		case 'v':
			oflags |= CFVERBOSE;
			break;
		}
		argp++;
	}
}

char *init (argc,argv)
int argc;
char **argv;
{
	char buf[STRINGLENGTH],*p;
	char username[STRINGLENGTH];
	register char *supfname,*q,*arg;
	register COLLECTION *c,*lastC;
	register FILE *f;
	register OPTION option;
	register int opno,bogus;
	register struct passwd *pw;
	register TREE *t;
	TREE *collT;			/* collections we are interested in */
	int timenow;			/* startup time */
	int checkcoll ();
	int oflags,aflags;
	int cwant;

	sysflag = FALSE;		/* not system upgrade */
	timeflag = FALSE;		/* don't print times */
	scmdebug = 0;			/* level zero, no SCM debugging */
	portdebug = FALSE;		/* no debugging ports */

	collT = NULL;
	oflags = aflags = 0;
	while (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		doswitch (&argv[1][1],&collT,&oflags,&aflags);
		--argc;
		argv++;
	}
	if (argc == 1 && !sysflag)
		quit (1,"SUP: Need either -s or supfile\n");
	if (sysflag)
		supfname = salloc (sprintf (buf,!timeflag?FILESUPDEFAULT:FILESUPTDEFAULT,DEFDIR));
	else {
		supfname = salloc (argv[1]);
		if (strcmp (supfname,"-") == 0)
			*supfname = '\0';
		--argc;
		argv++;
	}
	cwant = argc > 1;
	while (argc > 1) {
		t = Tinsert (&collT,argv[1],TRUE);
		t->Twant = TRUE;
		--argc;
		argv++;
	}
	if ((p = getlogin()) || ((pw = getpwuid (getuid())) && (p = pw->pw_name)))
		strcpy (username,p);
	else
		*username = '\0';
	bzero ((char *)uidL, sizeof (uidL));
	bzero ((char *)gidL, sizeof (gidL));
	if (*supfname) {
		f = fopen (supfname,"r");
		if (f == NULL)
			quit (1,"SUP: Can't open supfile %s\n",supfname);
	} else
		f = stdin;
	firstC = NULL;
	lastC = NULL;
	bogus = FALSE;
	while (p = fgets (buf,STRINGLENGTH,f)) {
		q = index (p,'\n');
		if (q)  *q = '\0';
		if (index ("#;:",*p))  continue;
		arg = nxtarg (&p," \t");
		if (*arg == '\0') {
			printf ("SUP: Missing collection name in supfile\n");
			bogus = TRUE;
			continue;
		}
		if (cwant) {
			register TREE *t;
			if ((t = Tsearch (collT,arg)) == NULL)
				continue;
			t->Tcount++;
		}
		c = (COLLECTION *) malloc (sizeof(COLLECTION));
		if (firstC == NULL)  firstC = c;
		if (lastC != NULL) lastC->Cnext = c;
		lastC = c;
		c->Cnext = NULL;
		c->Cname = salloc (arg);
		c->Chost = NULL;
		c->Chtree = NULL;
		c->Cbase = NULL;
		c->Chbase = NULL;
		c->Cprefix = NULL;
		c->Cnotify = NULL;
		c->Clogin = NULL;
		c->Cpswd = NULL;
		c->Ccrypt = NULL;
		c->Cflags = 0;
		c->Cnogood = FALSE;
		c->Clockfd = -1;
		p = skipover (p," \t");
		while (*(arg=nxtarg(&p," \t="))) {
			for (opno = 0; opno < sizeofA(options); opno++)
				if (strcmp (arg,options[opno].op_name) == 0)
					break;
			if (opno == sizeofA(options)) {
				printf ("SUP: Invalid option %s for %s in supfile\n",
					arg,c->Cname);
				bogus = TRUE;
				continue;
			}
			option = options[opno].op_enum;
			switch (option) {
			case OHOST:
				passdelim (&p,'=');
				do {
					arg = nxtarg (&p,", \t");
					(void) Tinsert (&c->Chtree,arg,FALSE);
					arg = p;
					q = skipover (p," \t");
					if (*q++ == ',')  p = q;
				} while (arg != p);
				break;
			case OBASE:
				passdelim (&p,'=');
				arg = nxtarg (&p," \t");
				c->Cbase = salloc (arg);
				break;
			case OHOSTBASE:
				passdelim (&p,'=');
				arg = nxtarg (&p," \t");
				c->Chbase = salloc (arg);
				break;
			case OPREFIX:
				passdelim (&p,'=');
				arg = nxtarg (&p," \t");
				c->Cprefix = salloc (arg);
				break;
			case ONOTIFY:
				passdelim (&p,'=');
				arg = nxtarg (&p," \t");
				c->Cnotify = salloc (arg);
				break;
			case OLOGIN:
				passdelim (&p,'=');
				arg = nxtarg (&p," \t");
				c->Clogin = salloc (arg);
				break;
			case OPASSWORD:
				passdelim (&p,'=');
				arg = nxtarg (&p," \t");
				c->Cpswd = salloc (arg);
				break;
			case OCRYPT:
				passdelim (&p,'=');
				arg = nxtarg (&p," \t");
				c->Ccrypt = salloc (arg);
				break;
			case OBACKUP:
				c->Cflags |= CFBACKUP;
				break;
			case ODELETE:
				c->Cflags |= CFDELETE;
				break;
			case OEXECUTE:
				c->Cflags |= CFEXECUTE;
				break;
			case OOLD:
				c->Cflags |= CFOLD;
				break;
			case ONOEXEC:
			case ONODELETE:
			case ONOOLD:
				printf("SUP: Ignored obsolete keyword %s\n",arg);
				break;
			}
		}
		c->Cflags |= oflags;
		c->Cflags &= ~aflags;
		if (t = Tsearch (collT,c->Cname)) {
			c->Cflags |= t->Toflags;
			c->Cflags &= ~t->Taflags;
		}
		if ((c->Cflags&CFMAIL) && c->Cnotify == NULL) {
			if (*username == '\0')
				printf ("SUP: No notification will be given... user unknown\n");
			else
				c->Cnotify = salloc (username);
		}
		if (c->Cbase == NULL) {
			sprintf (buf,FILEBASEDEFAULT,c->Cname);
			c->Cbase = salloc (buf);
		}
	}
	if (bogus)  quit (1,"SUP: Aborted due to supfile errors\n");
	if (f != stdin)  fclose (f);
	if (cwant)  (void) Tprocess (collT,checkcoll);
	Tfree (&collT);
	if (firstC == NULL)  quit (1,"SUP: No collections to upgrade\n");
	timenow = time ((int *)NULL);
	if (*supfname == '\0')
		p = "standard input";
	else if (sysflag)
		p = "system software";
	else
		p = sprintf (buf,"file %s",supfname);
	printf ("SUP %d.%d (%s) for %s at %s",PROTOVERSION,PGMVERSION,scmversion,
		p,ctime (&timenow));
	return (salloc (supfname));
}

checkcoll (t)
register TREE *t;
{
	if (!t->Twant)  return (SCMOK);
	if (t->Tcount == 0)
		printf ("SUP: Collection %s not found\n",t->Tname);
	if (t->Tcount > 1)
		printf ("SUP: Collection %s found more than once\n",t->Tname);
	return (SCMOK);
}

passdelim (ptr,delim)		/* skip over delimiter */
char **ptr,delim;
{
	extern char _argbreak;
	*ptr = skipover (*ptr, " \t");
	if (_argbreak != delim && **ptr == delim) {
		(*ptr)++;
		*ptr = skipover (*ptr, " \t");
	}
}

/*****************************************
 ***    G E T   H O S T   N A M E S    ***
 *****************************************/
/* For each collection that doesn't have a host name specified, ask
 * the name server for the name of the host for that collection.
 * There are several name servers to try to connect to.
 * It's a fatal error if a collection has no host name.
 */

getnams ()
{
	register COLLECTION *c;
	register FILE *f;
	register int x;
	char buf[STRINGLENGTH];
	char *p,*q;
	int foundns = FALSE;

	/* find out if any collections need a host name */
	for (c = firstC; c && c->Chtree; c = c->Cnext);
	if (c == NULL)  return;
	sprintf (buf,FILEHOSTS,DEFDIR);
	f = fopen (buf,"r");
	if (f == NULL)  quit (1,"SUP: Can't open %s\n",buf);
	while (p = fgets (buf,STRINGLENGTH,f)) {
		/* try each specified name-server host while needed */
		q = index (p,'\n');
		if (q)  *q = '\0';
		if (index ("#;:",*p))  continue;
		if (portdebug)	x = request (DEBUGNPORT,p,FALSE);
		else		x = request (NAMEPORT,p,FALSE);
		if (x != SCMOK) {
			printf ("SUP: Can't connect to name server @ %s\n",p);
			continue;
		}
		foundns = TRUE;
		x = msgnsignon ();	/* signon to name server */
		if (x != SCMOK) {
			goaway ("Error sending signon request to name server");
			(void) requestend ();
			continue;
		}
		x = msgnsignonack ();	/* receive signon ack from name server */
		if (x != SCMOK) {
			goaway ("Error reading signon reply from name server");
			(void) requestend ();
			continue;
		}
		printf ("SUP Nameserver %d.%d (%s) %d @ %s\n",
			protver,pgmver,scmver,nspid,remotehost());
		free (scmver);
		scmver = NULL;
		do {
			collname = c->Cname;
			x = msgncoll (); /* request repository host for collection */
			if (x != SCMOK) {
				goaway ("Error sending name server request");
				break;
			}
			hostT = NULL;
			x = msgnhost (); /* read name server reply */
			if (x != SCMOK) {
				goaway ("Error receiving name server reply");
				break;
			}
			if (hostT == NULL)
				printf ("SUP: Collection %s unknown\n",c->Cname);
			else
				c->Chtree = hostT;
			while ((c = c->Cnext) && c->Chtree);
		} while (c);
		if (x == SCMOK)  goaway ((char *)NULL);
		(void) requestend ();
		for (c = firstC; c && c->Chtree != NULL; c = c->Cnext);
		if (c == NULL)  break;
	}
	fclose (f);
	if (c == NULL)  return;
	if (!foundns)  quit (1,"SUP: No name server available\n");
	do {
		printf("SUP: Host for collection %s not found\n",c->Cname);
		while ((c = c->Cnext) && c->Chtree);
	} while (c);
	quit (1,"SUP: Hosts not found for all collections\n");
}

/*************************************************
 ***    U P G R A D E   C O L L E C T I O N    ***
 *************************************************/

/* The next two routines define the fsm to support multiple fileservers
 * per collection.
 */
getonehost (t,state)
register TREE *t;
int *state;
{
	if (t->Tflags != *state)
		return (SCMOK);
	if (*state != 0 && t->Tmode == SCMEOF) {
		t->Tflags = 0;
		return (SCMOK);
	}
	if (*state == 2)
		t->Tflags--;
	else
		t->Tflags++;
	thisC->Chost = t;
	return (SCMEOF);
}

TREE *getcollhost (backoff,state,nhostsp)
int *backoff,*state,*nhostsp;
{
	static int laststate = 0;
	static int nhosts = 0;
	int sltime;
	struct timeval tt;

	if (*state != laststate) {
		*nhostsp = nhosts;
		laststate = *state;
		nhosts = 0;
	}
	if (Tprocess (thisC->Chtree,getonehost,*state) == SCMEOF) {
		if (*state != 0 && nhosts == 0) {
			sltime = *backoff * 30;
			if (gettimeofday(&tt,(struct timezone *)NULL) >= 0)
				sltime += (tt.tv_usec >> 8) % sltime;
			vnotify ("SUP Will retry in %d seconds\n",sltime);
			sleep (sltime);
			if (*backoff < 32) *backoff <<= 1;
		}
		nhosts++;
		return (thisC->Chost);
	}
	if (nhosts == 0)
		return (NULL);
	if (*state == 2)
		(*state)--;
	else
		(*state)++;
	return (getcollhost (backoff,state,nhostsp));
}

/*  Upgrade a collection from the file server on the appropriate
 *  host machine.
 */

getcoll ()
{
	register TREE *t;
	register int x;
	int backoff,state,nhosts;

	collname = thisC->Cname;
	lastT = NULL;
	backoff = 2;
	state = 0;
	nhosts = 0;
	for (;;) {
		if ((t = getcollhost (&backoff,&state,&nhosts)) == NULL) {
			finishup (SCMEOF);
			notify ((char *)NULL);
			return;
		}
		t->Tmode = SCMEOF;
		if (!setjmp (sjbuf) && !signon (t,nhosts) && !setup (t))
			break;
		(void) requestend ();
	}
	if (setjmp (sjbuf))
		x = SCMERR;
	else {
		login ();
		listfiles ();
		recvfiles ();
		x = SCMOK;
	}
	if (thisC->Clockfd >= 0) {
		close (thisC->Clockfd);
		thisC->Clockfd = -1;
	}
	finishup (x);
	notify ((char *)NULL);
}

/***  Sign on to file server ***/

int signon (t,nhosts)
register TREE *t;
int nhosts;
{
	register int x;

	if ((thisC->Cflags&CFLOCAL) == 0 && thishost (thisC->Chost->Tname)) {
		vnotify ("SUP: Skipping local collection %s\n",collname);
		t->Tmode = SCMEOF;
		return (TRUE);
	}
	if (portdebug)	x = request (DEBUGFPORT,thisC->Chost->Tname,
					nhosts == 1 ? TRUE : FALSE);
	else		x = request (FILEPORT,thisC->Chost->Tname,
					nhosts == 1 ? TRUE : FALSE);
	if (x != SCMOK) {
		notify ("SUP: Can't connect to host %s\n", thisC->Chost->Tname);
		t->Tmode = nhosts ? SCMEOF : SCMOK;
		return (TRUE);
	}
	x = msgfsignon ();	/* signon to file server */
	if (x != SCMOK)  goaway ("Error sending signon request to file server");
	x = msgfsignonack ();	/* receive signon ack from file server */
	if (x != SCMOK)  goaway ("Error reading signon reply from file server");
	vnotify ("SUP Fileserver %d.%d (%s) %d @ %s\n",
		protver,pgmver,scmver,fspid,remotehost());
	free (scmver);
	scmver = NULL;
	if (protver < 4) {
		sjbuf[0] = 0;
		goaway ("Fileserver sup protocol version is obsolete.");
		notify ("SUP: This version of sup can only communicate with a fileserver using at least\n");
		notify ("SUP: version 4 of the sup network protocol.  You should either run a newer\n");
		notify ("SUP: version of the sup fileserver or find an older version of sup.\n");
		t->Tmode = SCMEOF;
		return (TRUE);
	}
	return (FALSE);
}

/***  Tell file server what to connect to ***/

setup (t)
register TREE *t;
{
	char buf[STRINGLENGTH];
	register int f,x;
	struct stat sbuf;

	if (chdir (thisC->Cbase) < 0)
		goaway ("Can't change to base directory %s",thisC->Cbase);
	if (stat ("sup",&sbuf) < 0) {
		mkdir ("sup",0755);
		if (stat("sup",&sbuf) < 0)
			goaway ("Can't create directory %s/sup",thisC->Cbase);
		vnotify ("SUP Created directory %s/sup\n",thisC->Cbase);
	}
	if (thisC->Cprefix && chdir (thisC->Cprefix) < 0)
		goaway ("Can't change to %s from base directory %s",thisC->Cprefix,thisC->Cbase);
	if (stat (".",&sbuf) < 0)
		goaway ("Can't stat %s directory %s",thisC->Cprefix ? "prefix" : "base",
			thisC->Cprefix ? thisC->Cprefix : thisC->Cbase);
	if (thisC->Cprefix)  chdir (thisC->Cbase);
	/* read time of last upgrade from when file */
	sprintf (buf,FILEWHEN,collname);
	f = open (buf,O_RDONLY,0);
	if (f >= 0) {
		if (read(f,(char *)&lasttime,sizeof(int)) != sizeof(int))
			lasttime = 0;
		close (f);
	} else
		lasttime = 0;
	/* setup for msgfsetup */
	basedir = thisC->Chbase;
	basedev = sbuf.st_dev;
	baseino = sbuf.st_ino;
	listonly = (thisC->Cflags&CFLIST);
	newonly = ((thisC->Cflags&(CFALL|CFDELETE|CFOLD)) == 0);
	x = msgfsetup ();
	if (x != SCMOK)  goaway ("Error sending setup request to file server");
	x = msgfsetupack ();
	if (x != SCMOK)  goaway ("Error reading setup reply from file server");
	if (setupack != FSETUPOK)
		switch (setupack) {
		case FSETUPSAME:
			goaway ("Attempt to upgrade from same host into same directory");
		case FSETUPHOST:
			goaway ("This host has no permission to access %s",collname);
		case FSETUPOLD:
			goaway ("This version of SUP is not supported by the fileserver");
		case FSETUPBUSY:
			vnotify ("SUP Fileserver is currently busy\n");
			t->Tmode = SCMOK;
			return (TRUE);
		default:
			goaway ("Unrecognized file server setup status %d",setupack);
		}
	/* Test encryption */
	(void) netcrypt (thisC->Ccrypt);
	crypttest = CRYPTTEST;
	x = msgfcrypt ();
	if (x != SCMOK)  goaway ("Error sending encryption test request to file server");
	x = msgfcryptok ();
	if (x == SCMEOF)  goaway ("Data encryption test failed");
	if (x != SCMOK)  goaway ("Error reading encryption test reply from file server");
	return (FALSE);
}

/***  Tell file server what account to use ***/

int login ()
{
	char buf[STRINGLENGTH];
	register int f,x;

	/* lock collection if desired */
	sprintf (buf,FILELOCK,collname);
	f = open (buf,O_RDONLY,0);
	if (f >= 0) {
		if (flock (f,(LOCK_EX|LOCK_NB)) < 0) {
			if (errno != EWOULDBLOCK)
				goaway ("Can't lock collection %s",collname);
			if (flock (f,(LOCK_SH|LOCK_NB)) < 0) {
				close (f);
				if (errno == EWOULDBLOCK)
					goaway ("Collection %s is locked by another sup",collname);
				goaway ("Can't lock collection %s",collname);
			}
			vnotify ("SUP Waiting for exclusive access lock\n");
			if (flock (f,LOCK_EX) < 0) {
				close (f);
				goaway ("Can't lock collection %s",collname);
			}
		}
		thisC->Clockfd = f;
		vnotify ("SUP Locked collection %s for exclusive access\n",collname);
	}
	logcrypt = CRYPTTEST;
	loguser = thisC->Clogin;
	logpswd = thisC->Cpswd;
	(void) netcrypt (PSWDCRYPT);	/* encrypt password data */
	x = msgflogin ();
	(void) netcrypt ((char *)NULL);	/* turn off encryption */
	if (x != SCMOK)  goaway ("Error sending login request to file server");
	x = msgflogack ();
	if (x != SCMOK)  goaway ("Error reading login reply from file server");
	if (logack == FLOGNG) {
		notify ("SUP: %s\n",logerr);
		free (logerr);
		logerr = NULL;
		goaway ("Improper login to %s account",
			thisC->Clogin ? thisC->Clogin : "default");
	}
	(void) netcrypt (thisC->Ccrypt);	/* restore encryption */
}

/*
 *  send list of files that we are not interested in.  receive list of
 *  files that are on the repository that could be upgraded.  Find the
 *  ones that we need.  Receive the list of files that the server could
 *  not access.  Delete any files that have been upgraded in the past
 *  which are no longer on the repository.
 */

int listfiles ()
{
	int needone(), denyone(), deleteone();
	char buf[STRINGLENGTH];
	register char *p,*q;
	register FILE *f;
	register int x;

	sprintf (buf,FILELAST,collname);
	f = fopen (buf,"r");
	if (f) {
		while (p = fgets (buf,STRINGLENGTH,f)) {
			if (q = index (p,'\n'))  *q = '\0';
			if (index ("#;:",*p))  continue;
			(void) Tinsert (&lastT,p,FALSE);
		}
		fclose (f);
	}
	refuseT = NULL;
	sprintf (buf,FILEREFUSE,collname);
	f = fopen (buf,"r");
	if (f) {
		while (p = fgets (buf,STRINGLENGTH,f)) {
			if (q = index (p,'\n'))  *q = '\0';
			if (index ("#;:",*p))  continue;
			(void) Tinsert (&refuseT,p,FALSE);
		}
		fclose (f);
	}
	vnotify ("SUP Requesting changes since %s",ctime (&lasttime));
	x = msgfrefuse ();
	if (x != SCMOK)  goaway ("Error sending refuse list to file server");
	listT = NULL;
	x = msgflist ();
	if (x != SCMOK)  goaway ("Error reading file list from file server");
	if (thisC->Cprefix)  chdir (thisC->Cprefix);
	needT = NULL;
	(void) Tprocess (listT,needone);
	Tfree (&listT);
	x = msgfneed ();
	if (x != SCMOK)  goaway ("Error sending needed files list to file server");
	Tfree (&needT);
	denyT = NULL;
	x = msgfdeny ();
	if (x != SCMOK)  goaway ("Error reading denied files list from file server");
	if (thisC->Cflags&CFVERBOSE)
		(void) Tprocess (denyT,denyone);
	Tfree (&denyT);
	if (thisC->Cflags&(CFALL|CFDELETE|CFOLD))
		(void) Trprocess (lastT,deleteone);
	Tfree (&refuseT);
}

needone (t)
register TREE *t;
{
	register TREE *newt;
	register int exists,update;
	struct stat sbuf;

	newt = Tinsert (&lastT,t->Tname,TRUE);
	newt->Tflags |= FUPDATE;
	update = FALSE;
	if ((thisC->Cflags&CFALL) == 0) {
		if ((t->Tflags&FNEW) == 0 && (thisC->Cflags&CFOLD) == 0)
			return (SCMOK);
		if ((t->Tmode&S_IFMT) == S_IFLNK)
			exists = (lstat (t->Tname,&sbuf) >= 0);
		else
			exists = (stat (t->Tname,&sbuf) >= 0);
		update = (exists && ((t->Tmode&S_IFMT) != S_IFREG ||
				     sbuf.st_mtime == t->Tmtime));
		if ((t->Tflags&FNEW) == 0 && update)
			return (SCMOK);
	}
	newt = Tinsert (&needT,t->Tname,TRUE);
	if (update && (t->Tmode&S_IFMT) == S_IFREG)
		newt->Tflags |= FUPDATE;
	return (SCMOK);
}

denyone (t)
TREE *t;
{
	vnotify ("SUP Access denied to %s\n",t->Tname);
	return (SCMOK);
}

deleteone (t)
TREE *t;
{
	struct stat sbuf;
	register int x;

	if (t->Tflags&FUPDATE)		/* in current upgrade list */
		return (SCMOK);
	if (lstat(t->Tname,&sbuf) < 0)	/* doesn't exist */
		return (SCMOK);
	/* is it a symbolic link ? */
	if ((sbuf.st_mode & S_IFMT) == S_IFLNK) {
		if (Tlookup (refuseT,t->Tname)) {
			vnotify ("SUP Would not delete symbolic link %s\n",t->Tname);
			return (SCMOK);
		}
		if (thisC->Cflags&CFLIST) {
			vnotify ("SUP Would delete symbolic link %s\n",t->Tname);
			return (SCMOK);
		}
		if ((thisC->Cflags&CFDELETE) == 0) {
			notify ("SUP Please delete symbolic link %s\n",t->Tname);
			t->Tflags |= FUPDATE;
			return (SCMOK);
		}
		x = unlink (t->Tname);
		if (x < 0) {
			notify ("SUP: Unable to delete symbolic link %s\n",
				t->Tname);
			t->Tflags |= FUPDATE;
			return (SCMOK);
		}
		vnotify ("SUP Deleted symbolic link %s\n",t->Tname);
		return (SCMOK);
	}
	/* is it a directory ? */
	if ((sbuf.st_mode & S_IFMT) == S_IFDIR) {
		if (Tlookup (refuseT,t->Tname)) {
			vnotify ("SUP Would not delete directory %s\n",t->Tname);
			return (SCMOK);
		}
		if (thisC->Cflags&CFLIST) {
			vnotify ("SUP Would delete directory %s\n",t->Tname);
			return (SCMOK);
		}
		if ((thisC->Cflags&CFDELETE) == 0) {
			notify ("SUP Please delete directory %s\n",t->Tname);
			t->Tflags |= FUPDATE;
			return (SCMOK);
		}
		runp ("rmdir","rmdir",t->Tname,0);
		if (lstat(t->Tname,&sbuf) >= 0) {
			notify ("SUP: Unable to delete directory %s\n",
				t->Tname);
			t->Tflags |= FUPDATE;
			return (SCMOK);
		}
		vnotify ("SUP Deleted directory %s\n",t->Tname);
		return (SCMOK);
	}
	/* it is a file */
	if (Tlookup (refuseT,t->Tname)) {
		vnotify ("SUP Would not delete file %s\n",t->Tname);
		return (SCMOK);
	}
	if (thisC->Cflags&CFLIST) {
		vnotify ("SUP Would delete file %s\n",t->Tname);
		return (SCMOK);
	}
	if ((thisC->Cflags&CFDELETE) == 0) {
		notify ("SUP Please delete file %s\n",t->Tname);
		t->Tflags |= FUPDATE;
		return (SCMOK);
	}
	x = unlink (t->Tname);
	if (x < 0) {
		notify ("SUP: Unable to delete file %s\n",t->Tname);
		t->Tflags |= FUPDATE;
		return (SCMOK);
	}
	vnotify ("SUP Deleted file %s\n",t->Tname);
	return (SCMOK);
}

/***************************************
 ***    R E C E I V E   F I L E S    ***
 ***************************************/

/* Note for these routines, return code SCMOK generally means
 * NETWORK communication is OK; it does not mean that the current
 * file was correctly received and stored.  If a file gets messed
 * up, too bad, just print a message and go on to the next one;
 * but if the network gets messed up, the whole sup program loses
 * badly and best just stop the program as soon as possible.
 */

recvfiles ()
{
	register int x;
	int recvone ();
	int recvmore;

	recvmore = TRUE;
	upgradeT = NULL;
	do {
		x = msgfsend ();
		if (x != SCMOK)  goaway ("Error sending receive file request to file server");
		(void) Tinsert (&upgradeT,(char *)NULL,FALSE);
		x = msgfrecv (recvone,&recvmore);
		if (x != SCMOK)  goaway ("Error receiving file from file server");
		Tfree (&upgradeT);
	} while (recvmore);
}

recvone (t,recvmore)
register TREE *t;
int **recvmore;
{
	struct timeval tbuf[2];
	register int x;
	int linkone (),execone ();

	/* check for end of file list */
	if (t == NULL) {
		*(*recvmore) = FALSE;
		return (SCMOK);
	}
	/* check for failed access at fileserver */
	if (t->Tmode == 0) {
		notify ("SUP: File server unable to transfer file %s\n",t->Tname);
		thisC->Cnogood = TRUE;
		return (SCMOK);
	}
	/* make file mode specific changes */
	switch (t->Tmode&S_IFMT) {
	case S_IFDIR:
		x = recvdir (t);
		break;
	case S_IFLNK:
		x = recvsym (t);
		break;
	case S_IFREG:
		x = recvreg (t);
		break;
	default:
		goaway ("Unknown file type %o\n",t->Tmode&S_IFMT);
	}
	if (x) {
		thisC->Cnogood = TRUE;
		return (SCMOK);
	}
	if ((thisC->Cflags&CFLIST) == 0 && (t->Tmode&S_IFMT) != S_IFLNK) {
		if ((t->Tflags&FNOACCT) == 0) {
			/* convert user and group names to local ids */
			ugconvert (t->Tuser,t->Tgroup,&t->Tuid,&t->Tgid,&t->Tmode);
			chown (t->Tname,t->Tuid,t->Tgid);
			chmod (t->Tname,t->Tmode&S_IMODE);
		}
		tbuf[0].tv_sec = time((int *)NULL);  tbuf[0].tv_usec = 0;
		tbuf[1].tv_sec = t->Tmtime;  tbuf[1].tv_usec = 0;
		utimes (t->Tname,tbuf);
	}
	if ((t->Tmode&S_IFMT) == S_IFREG)
		(void) Tprocess (t->Tlink,linkone,t->Tname);
	(void) Tprocess (t->Texec,execone);
	return (SCMOK);
}

int recvdir (t)				/* receive directory from network */
register TREE *t;
{
	register int new;
	struct stat sbuf;
	new = (stat(t->Tname,&sbuf) < 0);
	if (thisC->Cflags&CFLIST) {
		vnotify ("SUP Would %s directory %s\n",
			new ? "create" : "update",t->Tname);
		return (FALSE);
	}
	if (new) {
		if (establishdir (t->Tname)) /* can't make directory */
			return (TRUE);
		mkdir (t->Tname,0755);
		if (stat (t->Tname,&sbuf) < 0) {
			notify ("SUP: Can't create directory %s\n",t->Tname);
			return (TRUE);
		}
	}
	vnotify ("SUP %s directory %s\n",new ? "Created" : "Updated",t->Tname);
	return (FALSE);
}

int recvsym (t)				/* receive symbolic link */
register TREE *t;
{
	struct stat sbuf;
	register char *linkname;

	if (t->Tlink == NULL || t->Tlink->Tname == NULL) {
		notify ("SUP: Missing linkname for symbolic link %s\n",t->Tname);
		return (TRUE);
	}
	linkname = t->Tlink->Tname;
	if (thisC->Cflags&CFLIST) {
		vnotify ("SUP Would create symbolic link %s to %s\n",
			t->Tname,linkname);
		return (FALSE);
	}
	if (lstat(t->Tname,&sbuf) >= 0) {
		if ((sbuf.st_mode&S_IFMT) == S_IFDIR)
			runp ("rmdir","rmdir",t->Tname,0);
		else
			unlink (t->Tname);
		if (lstat(t->Tname,&sbuf) >= 0) {
			notify ("SUP: Unable to %s %s for symbolic link\n",
				((sbuf.st_mode&S_IFMT) == S_IFDIR) ?
				"delete directory" : "delete",t->Tname);
			return (SCMOK);
		}
	}
	if (symlink (linkname,t->Tname) < 0 ||
	    lstat(t->Tname,&sbuf) < 0) {
		notify ("SUP: Unable to create symbolic link %s\n",t->Tname);
		return (TRUE);
	}
	vnotify ("SUP Created symbolic link %s to %s\n",
		t->Tname,linkname);
	return (FALSE);
}

int recvreg (t)				/* receive file from network */
register TREE *t;
{
	register FILE *fin,*fout;
	char dirpart[STRINGLENGTH],filepart[STRINGLENGTH];
	char filename[STRINGLENGTH],buf[STRINGLENGTH];
	struct stat sbuf;
	register int new,x;

	new = (stat(t->Tname,&sbuf) < 0);
	if ((thisC->Cflags&CFLIST) || (t->Tflags&FUPDATE)) {
		if (thisC->Cflags&CFLIST) {
			vnotify ("SUP Would %s file %s\n",
				(t->Tflags&FUPDATE) ? "update" : "receive",
				t->Tname);
		} else {
			vnotify ("SUP Updating file %s\n",t->Tname);
		}
		return (FALSE);
	}
	vnotify ("SUP Receiving file %s\n",t->Tname);
	if (new && establishdir (t->Tname)) { /* can't make directory */
		x = readskip ();	/* skip over file */
		if (x != SCMOK)  goaway ("Can't skip file transfer");
		return (TRUE);		/* mark upgrade as nogood */
	}
	if (new || (t->Tmode&S_IFMT) != S_IFREG || !(t->Tflags&FBACKUP) || (thisC->Cflags&CFBACKUP) == 0)
		return (copyfile (t->Tname,(char *)NULL));
	fin = fopen (t->Tname,"r");	/* create backup */
	if (fin == NULL) {
		x = readskip ();	/* skip over file */
		if (x != SCMOK)  goaway ("Can't skip file transfer");
		notify ("SUP: Can't open %s to create backup\n",t->Tname);
		return (TRUE);		/* mark upgrade as nogood */
	}
	path (t->Tname,dirpart,filepart);
	sprintf (filename,FILEBACKUP,dirpart,filepart);
	fout = fopen (filename,"w");
	if (fout == NULL) {
		sprintf (buf,FILEBKDIR,dirpart);
		mkdir (buf,0755);
		fout = fopen (filename,"w");
	}
	if (fout == NULL) {
		x = readskip ();	/* skip over file */
		if (x != SCMOK)  goaway ("Can't skip file transfer");
		notify ("SUP: Can't create %s for backup\n",filename);
		fclose (fin);
		return (TRUE);
	}
	ffilecopy (fin,fout);
	fclose (fin);
	fclose (fout);
	vnotify ("SUP Backup of %s created\n", t->Tname);
	return (copyfile (t->Tname,(char *)NULL));
}

linkone (t,fname)			/* link to file already received */
register TREE *t;
register char **fname;
{
	char lnkdir[STRINGLENGTH],buf[STRINGLENGTH];
	struct stat fbuf,lnkbuf;
	struct timeval tbuf[2];
	register char *lnkname = t->Tname;

	if (thisC->Cflags&CFLIST) {
		vnotify ("SUP Would link %s to %s\n",lnkname,*fname);
		return (SCMOK);
	}
	if (stat(*fname,&fbuf) < 0) {	/* source file */
		notify ("SUP: Can't link %s to missing file %s\n",lnkname,*fname);
		thisC->Cnogood = TRUE;
		return (SCMOK);
	}
	path (lnkname,lnkdir,buf);
	if (stat(lnkdir,&lnkbuf) < 0) {	/* destination directory */
		if (establishdir (lnkname)) {
			thisC->Cnogood = TRUE;
			return (SCMOK);
		}
		stat (lnkdir,&lnkbuf);	/* (guaranteed to work) */
	}
	if (fbuf.st_dev != lnkbuf.st_dev) /* cross-device */
		notify ("SUP: Copying %s to %s to avoid cross-device link\n",*fname,lnkname);
	else {
		unlink (lnkname);	/* OK, do link */
		if (link(*fname,lnkname) < 0)	/* didn't work */
			notify ("SUP: Copying %s to %s due to link failure\n",*fname,lnkname);
		else {
			vnotify ("SUP Linked %s to %s\n",lnkname,*fname);
			return (SCMOK);
		}
	}
	if (copyfile (lnkname,*fname)) {
		thisC->Cnogood = TRUE;
		return (SCMOK);
	}
	chown (lnkname,fbuf.st_uid,fbuf.st_gid);
	chmod (lnkname,(fbuf.st_mode&S_IMODE));
	tbuf[0].tv_sec = time((int *)NULL);  tbuf[0].tv_usec = 0;
	tbuf[1].tv_sec = fbuf.st_mtime;  tbuf[1].tv_usec = 0;
	utimes (lnkname,tbuf);
	return (SCMOK);
}

execone (t)			/* execute command for file */
register TREE *t;
{
	register int x;

	if (thisC->Cflags&CFLIST) {
		vnotify ("SUP Would execute %s\n",t->Tname);
		return (SCMOK);
	}
	if ((thisC->Cflags&CFEXECUTE) == 0) {
		notify ("SUP Please execute %s\n",t->Tname);
		return (SCMOK);
	}
	vnotify ("SUP Executing %s\n",t->Tname);
	if ((x = system (t->Tname)) != 0) {
		notify ("SUP Execute command returned failure status %.1o, signal %d\n",
			(x>>8)&0377,x&0377);
		thisC->Cnogood = TRUE;
	}
	return (SCMOK);
}

int establishdir (fname)
char *fname;
{
	char dpart[STRINGLENGTH],fpart[STRINGLENGTH];
	path (fname,dpart,fpart);
	return (estabd (fname,dpart));
}

int estabd (fname,dname)
char *fname,*dname;
{
	char dpart[STRINGLENGTH],fpart[STRINGLENGTH];
	struct stat sbuf;
	register int x;

	if (stat (dname,&sbuf) >= 0)  return (FALSE); /* exists */
	path (dname,dpart,fpart);
	if (strcmp (fpart,".") == 0) {		/* dname is / or . */
		notify ("SUP: Can't create directory %s for %s\n",dname,fname);
		return (TRUE);
	}
	x = estabd (fname,dpart);
	if (x)  return (TRUE);
	mkdir (dname,0755);
	if (stat (dname,&sbuf) < 0) {		/* didn't work */
		notify ("SUP: Can't create directory %s for %s\n",dname,fname);
		return (TRUE);
	}
	vnotify ("SUP Created directory %s for %s\n",dname,fname);
	return (FALSE);
}

int copyfile (to,from)
char *to;
char *from;		/* 0 if reading from network */
{
	register int fromf,tof,istemp,canlink,x;
	char dpart[STRINGLENGTH],fpart[STRINGLENGTH];
	char tname[STRINGLENGTH];
	struct stat sbuf;
	static int thispid = 0;		/* process id # */

	if (from) {			/* reading file */
		fromf = open (from,O_RDONLY,0);
		if (fromf < 0) {
			notify ("SUP: Can't open %s to copy to %s: %s\n",
				from,to,errmsg (-1));
			return (TRUE);
		}
	} else				/* reading network */
		fromf = -1;
	istemp = TRUE;			/* try to create temp file */
	if (stat (to,&sbuf) < 0)	/* if new file */
		sbuf.st_nlink = 1;	/* then like one link */
	canlink = (sbuf.st_nlink == 1);	/* don't try to link/unlink temp */
	lockout (TRUE);			/* block interrupts */
	if (thispid == 0)  thispid = getpid ();
	/* Now try hard to find a temp file name.  Try VERY hard. */
	for (;;) {
	/* try destination directory */
		path (to,dpart,fpart);
		sprintf (tname,"%s/#%d.sup",dpart,thispid);
		tof = open (tname,(O_WRONLY|O_CREAT|O_TRUNC),0600);
		if (tof >= 0)  break;
	/* try sup directory */
		if (thisC->Cprefix)  chdir (thisC->Cbase);
		sprintf (tname,"sup/#%d.sup",thispid);
		tof = open (tname,(O_WRONLY|O_CREAT|O_TRUNC),0600);
		if (tof >= 0) {
			if (thisC->Cprefix)  chdir (thisC->Cprefix);
			break;
		}
	/* try base directory */
		sprintf (tname,"#%d.sup",thispid);
		tof = open (tname,(O_WRONLY|O_CREAT|O_TRUNC),0600);
		if (thisC->Cprefix)  chdir (thisC->Cprefix);
		if (tof >= 0)  break;
	/* try /usr/tmp */
		sprintf (tname,"/usr/tmp/#%d.sup",thispid);
		tof = open (tname,(O_WRONLY|O_CREAT|O_TRUNC),0600);
		if (tof >= 0)  break;
	/* try /tmp */
		sprintf (tname,"/tmp/#%d.sup",thispid);
		tof = open (tname,(O_WRONLY|O_CREAT|O_TRUNC),0600);
		if (tof >= 0)  break;
		istemp = FALSE;
	/* give up: try to create output file */
		tof = open (to,(O_WRONLY|O_CREAT|O_TRUNC),0600);
		if (tof >= 0)  break;
	/* no luck */
		notify ("SUP: Can't create %s or temp file for it\n",to);
		lockout (FALSE);
		if (fromf >= 0)
			close (fromf);
		else {
			x = readskip ();
			if (x != SCMOK)  goaway ("Can't skip file transfer");
		}
		return (TRUE);
	}
	if (fromf >= 0) {		/* read file */
		x = filecopy (fromf,tof);
		close (fromf);
		close (tof);
		if (x < 0) {
			notify ("SUP: Error in copying %s to %s\n",from,to);
			if (istemp)  unlink (tname);
			lockout (FALSE);
			return (TRUE);
		}
	} else {			/* read network */
		x = readfile (tof);
		close (tof);
		if (x != SCMOK) {
			if (istemp)  unlink (tname);
			lockout (FALSE);
			goaway ("Error in receiving %s\n",to);
		}
	}
	if (!istemp) {			/* no temp file used */
		lockout (FALSE);
		return (FALSE);
	}
	/* copy somehow to destination */
	if (canlink) {			/* link/unlink allowed */
		x = movefile (tname,to);	/* this tries link first */
		unlink (tname);
		lockout (FALSE);
		if (x < 0) {
			notify ("SUP: Error in moving temp file to %s\n",to);
			return (TRUE);
		}
		return (FALSE);
	}
	fromf = open (tname,O_RDONLY,0);
	if (fromf < 0) {
		notify ("SUP: Error in moving temp file to %s: %s\n",
			to,errmsg (-1));
		unlink (tname);
		lockout (FALSE);
		return (TRUE);
	}
	tof = open (to,(O_WRONLY|O_CREAT|O_TRUNC),0600);
	if (tof < 0) {
		close (fromf);
		notify ("SUP: Can't create %s from temp file: %s\n",
			to,errmsg (-1));
		unlink (tname);
		lockout (FALSE);
		return (TRUE);
	}
	x = filecopy (fromf,tof);
	close (fromf);
	close (tof);
	unlink (tname);
	lockout (FALSE);
	if (x < 0) {
		notify ("SUP: Error in storing data in %s\n",to);
		return (TRUE);
	}
	return (FALSE);
}

/***  Finish connection with file server ***/

finishup (x)
int x;
{
	char tname[STRINGLENGTH],fname[STRINGLENGTH];
	int tloc;
	FILE *finishfile;		/* record of all filenames */
	int f,finishone();

	sjbuf[0] = 0;			/* once here, no more longjmp */
	(void) netcrypt ((char *)NULL);
	/* done with server */
	if (x == SCMOK)  goaway ((char *)NULL);
	(void) requestend ();
	tloc = time ((int *)NULL);
	if (x != SCMOK) {
		notify ("SUP Upgrade of %s aborted at %s",collname,ctime (&tloc));
		Tfree (&lastT);
		return;
	}
	if (thisC->Cnogood) {
		notify ("SUP Upgrade of %s completed with errors at %s",collname,ctime (&tloc));
		notify ("SUP Upgrade time will not be updated\n");
		Tfree (&lastT);
		return;
	}
	if (thisC->Cprefix)  chdir (thisC->Cbase);
	vnotify ("SUP Upgrade of %s completed at %s",collname,ctime (&tloc));
	if (thisC->Cflags&CFLIST) {
		Tfree (&lastT);
		return;
	}
	sprintf (fname,FILEWHEN,collname);
	if (establishdir (fname)) {
		notify ("SUP: Can't create directory for upgrade timestamp\n");
		Tfree (&lastT);
		return;
	}
	f = open (fname,(O_WRONLY|O_CREAT|O_TRUNC),0644);
	if (f < 0)
		notify ("SUP: Can't record current time in %s: %s\n",
			fname,errmsg (-1));
	else {
		write (f,(char *)&scantime,sizeof(int));
		close (f);
	}
	sprintf (tname,FILELASTTEMP,collname);
	finishfile = fopen (tname,"w");
	if (finishfile == NULL) {
		notify ("SUP: Can't record list of all files in %s\n",tname);
		Tfree (&lastT);
		return;
	}
	(void) Tprocess (lastT,finishone,finishfile);
	fclose (finishfile);
	sprintf (fname,FILELAST,collname);
	if (rename (tname,fname) < 0)
		notify ("SUP: Can't change %s to %s\n",tname,fname);
	unlink (tname);
	Tfree (&lastT);
}

finishone (t,finishfile)
TREE *t;
FILE **finishfile;
{
	if ((thisC->Cflags&CFDELETE) == 0 || (t->Tflags&FUPDATE))
		fprintf (*finishfile,"%s\n",t->Tname);
	return (SCMOK);
}

/***************************************
 ***    L I S T   R O U T I N E S    ***
 ***************************************/

static
int Lhash (name)
char *name;
{
	/* Hash function is:  HASHSIZE * (strlen mod HASHSIZE)
	 *		      +          (char   mod HASHSIZE)
	 * where "char" is last character of name (if name is non-null).
	 */

	register int len;
	register char c;
	len = strlen (name);
	if (len > 0)	c = name[len-1];
	else		c = 0;
	return (((len&HASHMASK)<<HASHBITS)|(((int)c)&HASHMASK));
}

static
Lfree (table)
LIST **table;
{
	register LIST *l;
	register int i;
	for (i = 0; i < LISTSIZE; i++)
		while (l = table[i]) {
			table[i] = l->Lnext;
			if (l->Lname)  free (l->Lname);
			free ((char *)l);
		}
}

static
Linsert (table,name,number)
LIST **table;
char *name;
int number;
{
	register LIST *l;
	register int lno;
	lno = Lhash (name);
	l = (LIST *) malloc (sizeof(LIST));
	l->Lname = name;
	l->Lnumber = number;
	l->Lnext = table[lno];
	table[lno] = l;
}

static
LIST *Llookup (table,name)
LIST **table;
char *name;
{
	register int lno;
	register LIST *l;
	lno = Lhash (name);
	for (l = table[lno]; l && strcmp(l->Lname,name) != 0; l = l->Lnext);
	return (l);
}

ugconvert (uname,gname,uid,gid,mode)
char *uname,*gname;
int *uid,*gid,*mode;
{
	register LIST *u,*g;
	register struct passwd *pw;
	register struct group *gr;
	struct stat sbuf;
	static int defuid = -1;
	static int defgid;

	if (u = Llookup (uidL,uname))
		*uid = u->Lnumber;
	else if (pw = getpwnam (uname)) {
		Linsert (uidL,salloc(uname),pw->pw_uid);
		*uid = pw->pw_uid;
	}
	if (u || pw) {
		if (g = Llookup (gidL,gname)) {
			*gid = g->Lnumber;
			return;
		}
		if (gr = getgrnam (gname)) {
			Linsert (gidL,salloc(gname),gr->gr_gid);
			*gid = gr->gr_gid;
			return;
		}
		if (pw == NULL)
			pw = getpwnam (uname);
		*mode &= ~S_ISGID;
		*gid = pw->pw_gid;
		return;
	}
	*mode &= ~(S_ISUID|S_ISGID);
	if (defuid >= 0) {
		*uid = defuid;
		*gid = defgid;
		return;
	}
	if (stat (".",&sbuf) < 0) {
		*uid = defuid = getuid ();
		*gid = defgid = getgid ();
		return;
	}
	*uid = defuid = sbuf.st_uid;
	*gid = defgid = sbuf.st_gid;
}


/*********************************************
 ***    U T I L I T Y   R O U T I N E S    ***
 *********************************************/

/* VARARGS1 */
notify (fmt,args)		/* record error message */
char *fmt;
{
	char buf[STRINGLENGTH];
	int tloc;
	static FILE *noteF = NULL;	/* mail program on pipe */

	if (fmt == NULL) {
		if (noteF && noteF != stdout)
			pclose (noteF);
		noteF = NULL;
		return;
	}
	if (noteF == NULL) {
		if ((thisC->Cflags&CFMAIL) && thisC->Cnotify) {
			sprintf (buf,"mail -s \"SUP Upgrade of %s\" %s >/dev/null",
				collname,thisC->Cnotify);
			noteF = popen (buf,"w");
			if (noteF == NULL) {
				fprintf (stderr,"Can't send mail to %s for %s\n",
					thisC->Cnotify,collname);
				fflush (stderr);
				noteF = stdout;
			}
		} else
			noteF = stdout;
		tloc = time ((int *)NULL);
		fprintf (noteF,"SUP Upgrade of %s at %s",collname,ctime (&tloc));
		fflush (noteF);
	}
	_doprnt(fmt,&args,noteF);
	fflush (noteF);
}

#define mask(sig) (1<<((sig)-1))

lockout (on)		/* lock out interrupts */
int on;
{
	register int x;
	static int lockmask;
	if (on) {
		x = mask (SIGHUP) | mask (SIGINT) |
		    mask (SIGQUIT) | mask (SIGTERM);
		lockmask = sigblock (x);
	}
	else {
		sigsetmask (lockmask);
	}
}

int thishost (host)
register char *host;
{
	register struct hostent *h;
	static char myhost[STRINGLENGTH];
	if (*myhost == '\0') {
		gethostname (myhost,STRINGLENGTH);
		h = gethostbyname (myhost);
		if (h == NULL) quit (1,"SUP: Can't find my host entry\n");
		strcpy (myhost,h->h_name);
	}
	h = gethostbyname (host);
	if (h == NULL) return (0);
	return (strcmp (myhost,h->h_name) == 0);
}

/* VARARGS1 */
goaway (fmt,args)
register char *fmt;
{
	struct _iobuf _strbuf;
	char buf[STRINGLENGTH];

	(void) netcrypt ((char *)NULL);
	if (fmt) {
		_strbuf._flag = _IOWRT+_IOSTRG;
		_strbuf._ptr = buf;
		_strbuf._cnt = sizeofS(buf);
		_doprnt(fmt, &args, &_strbuf);
		putc('\0', &_strbuf);
		goawayreason = buf;
	} else
		goawayreason = NULL;
	(void) msggoaway ();
	if (fmt)  notify ("SUP: %s\n",buf);
	if (sjbuf[0])  longjmp (sjbuf,TRUE);
}

/* supfilesrv -- SUP File Server
 *
 * Usage:  supfilesrv [-l] [-q] [-P] [-N]
 *	-l	"live" -- don't fork daemon
 *	-q	"quiet" -- don't print log message for each connection
 *	-P	"debug ports" -- use debugging network ports
 *	-N	"debug network" -- print debugging messages for network i/o
 *
 **********************************************************************
 * HISTORY
 * 04-Aug-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to increment scmdebug as more -N flags are
 *	added. [V5.7]
 *
 * 25-May-86  Jonathan J. Chew (jjc) at Carnegie-Mellon University
 *	Renamed local variable in main program from "sigmask" to
 *	"signalmask" to avoid name conflict with 4.3BSD identifier.
 *	Conditionally compile in calls to CMU routines, "setaid" and
 *	"logaccess". [V5.6]
 *
 * 21-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed supfilesrv to use the crypt file owner and group for
 *	access purposes, rather than the directory containing the crypt
 *	file. [V5.5]
 *
 * 07-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to keep logfiles in repository collection directory.
 *	Added code for locking collections. [V5.4]
 *
 * 05-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to support new FSETUPBUSY return.  Now accepts all
 *	connections and tells any clients after the 8th that the
 *	fileserver is busy.  New clients will retry again later. [V5.3]
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Major rewrite for protocol version 4. [V4.2]
 *
 * 12-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed close of crypt file to use file pointer as argument
 *	instead of string pointer.
 *
 * 24-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Allow "!hostname" lines and comments in collection "host" file.
 *
 * 13-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Don't use access() on symbolic links since they may not point to
 *	an existing file.
 *
 * 22-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to restrict file server availability to when it has
 *	less than or equal to eight children.
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
#include <sys/param.h>
#include <c.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <pwd.h>
#include <grp.h>
#include <acc.h>
#include <sys/wait.h>
#include <sys/ttyloc.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <access.h>
#include "sup.h"
#define MSGFILE
#include "supmsg.h"

extern int errno;

#define PGMVERSION 7

/*************************
 ***    M A C R O S    ***
 *************************/

#define HASHBITS 8
#define HASHSIZE (1<<HASHBITS)
#define HASHMASK (HASHSIZE-1)
#define HASHFUNC(x,y) ((x)&HASHMASK)

/*******************************************
 ***    D A T A   S T R U C T U R E S    ***
 *******************************************/

struct hashstruct {			/* hash table for number lists */
	int Hnum1;			/* numeric keys */
	int Hnum2;
	char *Hname;			/* string value */
	TREE *Htree;			/* TREE value */
	struct hashstruct *Hnext;
};
typedef struct hashstruct HASH;

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

char program[] = "supfilesrv";		/* program name for SCM messages */
int progpid = -1;			/* and process id */

jmp_buf sjbuf;				/* jump location for network errors */

int live;				/* -l flag */
int quiet;				/* -q flag */
int dbgportsq;				/* -P flag */
extern int scmdebug;			/* -N flag */

int nchildren;				/* number of children that exist */
char *prefix;				/* collection pathname prefix */
char *cryptkey;				/* encryption key if non-null */
int lockfd;				/* descriptor of lock file */

/* global variables for scan functions */
int trace = FALSE;			/* directory scan trace */

HASH *uidH[HASHSIZE];			/* for uid and gid lookup */
HASH *gidH[HASHSIZE];
HASH *inodeH[HASHSIZE];			/* for inode lookup for linked file check */

char *fmttime ();			/* time format routine */

/*************************************
 ***    M A I N   R O U T I N E    ***
 *************************************/

main (argc,argv)
int argc;
char **argv;
{
	register int x,pid,signalmask;
	struct sigvec chldvec,ignvec,oldvec;
	int chldsig ();

	/* initialize global variables */
	pgmversion = PGMVERSION;	/* export version number */
	server = TRUE;			/* export that we're not a server */
	collname = NULL;		/* no current collection yet */
	errstr = NULL;			/* and no errors yet */

	init (argc,argv);		/* process arguments */
	fflush (stdout);
	fflush (stderr);
	if (live) {
		x = service ();
		if (x != SCMOK)  quit (1,"supfilesrv: Can't connect to network\n");
		answer ();
		(void) serviceend ();
		exit (0);
	}
	ignvec.sv_handler = SIG_IGN;
	ignvec.sv_onstack = 0;
	ignvec.sv_mask = 0;
	sigvec (SIGHUP,&ignvec,&oldvec);
	sigvec (SIGINT,&ignvec,&oldvec);
	sigvec (SIGPIPE,&ignvec,&oldvec);
	chldvec.sv_handler = chldsig;
	chldvec.sv_mask = 0;
	chldvec.sv_onstack = 0;
	sigvec (SIGCHLD,&chldvec,&oldvec);
	nchildren = 0;
	close (2);
	dup (1);			/* redirect error output */
	for (;;) {
		x = service ();
		if (x != SCMOK)
			quit (1,"supfilesrv: Error in establishing network connection\n");
		signalmask = sigblock(1<<(SIGCHLD-1));
		if ((pid = fork()) == 0) { /* server process */
			(void) serviceprep ();
			answer ();
			(void) serviceend ();
			exit (0);
		}
		(void) servicekill ();	/* parent */
		if (pid > 0) nchildren++;
		sigsetmask(signalmask);
	}
}

/*
 * Child status signal handler
 */

chldsig()
{
	union wait w;

	while (wait3(&w.w_status, WNOHANG, 0) > 0) {
		if (nchildren) nchildren--;
	}
}

/*****************************************
 ***    I N I T I A L I Z A T I O N    ***
 *****************************************/

init (argc,argv)
int argc;
char **argv;
{
	long tloc;
	register int i;
	register int x;

	live = FALSE;
	quiet = FALSE;
	dbgportsq = FALSE;
	scmdebug = 0;
	while (argc > 1 && argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 'l':
			live = TRUE;
			break;
		case 'q':
			quiet = TRUE;
			break;
		case 'P':
			dbgportsq = TRUE;
			break;
		case 'N':
			scmdebug++;
			break;
		default:
			fprintf (stderr,"supfilesrv: Invalid flag %s ignored\n",argv[1]);
			fflush (stderr);
		}
		--argc;
		argv++;
	}
	if (argc != 1)  quit (1,"Usage: supfilesrv [-l] [-q] [-P] [-N]\n");
	if (dbgportsq)
		x = servicesetup (DEBUGFPORT);
	else
		x = servicesetup (FILEPORT);
	if (x != SCMOK)  quit (1,"supfilesrv: Error in network setup\n");
	for (i = 0; i < HASHSIZE; i++)
		uidH[i] = gidH[i] = inodeH[i] = NULL;
	tloc = time ((int *)NULL);
	printf ("SUP File Server Version %d.%d (%s) starting at %s\n",
		PROTOVERSION,PGMVERSION,scmversion,fmttime (tloc));
	fflush (stdout);
}

/*****************************************
 ***    A N S W E R   R E Q U E S T    ***
 *****************************************/

answer ()
{
	int starttime;

	progpid = fspid = getpid ();
	collname = NULL;
	basedir = NULL;
	prefix = NULL;
	lockfd = -1;
	starttime = time ((int *)NULL);
	if (!setjmp (sjbuf)) {
		signon ();
		setup ();
		login ();
		listfiles ();
		sendfiles ();
		finishup (starttime);
	}
	if (collname)  free (collname);
	if (basedir)  free (basedir);
	if (prefix)  free (prefix);
	if (lockfd >= 0)  close (lockfd);
	endpwent ();
	endgrent ();
	endacent ();
	Hfree (uidH);
	Hfree (gidH);
	Hfree (inodeH);
}

/*****************************************
 ***    S I G N   O N   C L I E N T    ***
 *****************************************/

signon ()
{
	register int x;

	x = msgfsignon ();
	if (x != SCMOK)  goaway ("Error reading signon request from client");
	x = msgfsignonack ();
	if (x != SCMOK)  goaway ("Error sending signon reply to client");
	if (!quiet) {
		printf ("supfilesrv %d: SUP %d.%d (%s) @ %s\n",
			fspid,protver,pgmver,scmver,remotehost());
		fflush (stdout);
	}
	free (scmver);
	scmver = NULL;
}

/*****************************************************************
 ***    E X C H A N G E   S E T U P   I N F O R M A T I O N    ***
 *****************************************************************/

setup ()
{
	register int x;
	long tnow;
	char *p,*q;
	char buf[STRINGLENGTH];
	register FILE *f;
	struct stat sbuf;
	
	x = msgfsetup ();
	if (x != SCMOK)  goaway ("Error reading setup request from client");
	if (protver < 3) {
		setupack = FSETUPOLD;
		(void) msgfsetupack ();
		goaway ("Sup client too obsolete for fileserver to understand");
	}
	if (basedir == NULL || *basedir == '\0') {
		basedir = NULL;
		sprintf (buf,FILEDIRS,DEFDIR);
		f = fopen (buf,"r");
		if (f) {
			while (p = fgets (buf,STRINGLENGTH,f)) {
				q = index (p,'\n');
				if (q)  *q = 0;
				if (index ("#;:",*p))  continue;
				q = nxtarg (&p," \t=");
				if (strcmp(q,collname) == 0) {
					basedir = skipover(p," \t=");
					basedir = salloc (basedir);
					break;
				}
			}
			fclose (f);
		}
		if (basedir == NULL) {
			sprintf (buf,FILEBASEDEFAULT,collname);
			basedir = salloc(buf);
		}
	}
	if (chdir(basedir) < 0)  goaway ("Can't chdir to base directory %s",basedir);
	sprintf (buf,FILEPREFIX,collname);
	f = fopen (buf,"r");
	if (f) {
		while (p = fgets (buf,STRINGLENGTH,f)) {
			q = index (p,'\n');
			if (q)  *q = 0;
			if (index ("#;:",*p))  continue;
			prefix = salloc(p);
			if (chdir (prefix) < 0)
				goaway ("Can't chdir to %s from base directory %s",
					prefix,basedir);
			break;
		}
		fclose (f);
	}
	if (samehost()) {
		if (stat (".",&sbuf) < 0)  goaway ("Can't stat base/prefix directory");
		if (sbuf.st_dev == basedev && sbuf.st_ino == baseino) {
			setupack = FSETUPSAME;
			(void) msgfsetupack ();
			goaway ("Attempt to upgrade to same directory on same host");
		}
	}
	if (prefix)  chdir (basedir);

	/* check host access file */
	cryptkey = NULL;
	sprintf (buf,FILEFOREIGN,collname);
	f = fopen (buf,"r");
	if (f) {
		int hostok = FALSE;
		while (p = fgets (buf,STRINGLENGTH,f)) {
			int not;
			q = index (p,'\n');
			if (q)  *q = 0;
			if (index ("#;:",*p))  continue;
			q = nxtarg (&p," \t");
			if ((not = (*q == '!')) && *++q == '\0')
				q = nxtarg (&p," \t");
			if (strcmp(q,LOCALSYSTEMS) == 0)
				hostok = (not == (localhost() == 0));
			else
				hostok = (not == (matchhost(folddown(q,q)) == 0));
			if (hostok) {
				if (*p)  cryptkey = salloc (p);
				break;
			}
		}
		fclose (f);
		if (!hostok) {
			setupack = FSETUPHOST;
			(void) msgfsetupack ();
			goaway ("Host not on access list for %s",collname);
		}
	}
	if (nchildren >= MAXCHILDREN) {
		setupack = FSETUPBUSY;
		(void) msgfsetupack ();
		goaway ("Sup client told to try again later");
	}
	sprintf (buf,FILELOCK,collname);
	x = open (buf,O_RDONLY,0);
	if (x >= 0) {
		if (flock (x,(LOCK_SH|LOCK_NB)) < 0) {
			close (x);
			if (errno != EWOULDBLOCK)
				goaway ("Can't lock collection %s",collname);
			setupack = FSETUPBUSY;
			(void) msgfsetupack ();
			goaway ("Sup client told to wait for lock");
		}
		lockfd = x;
	}
	setupack = FSETUPOK;
	x = msgfsetupack ();
	if (x != SCMOK)  goaway ("Error sending setup reply to client");

	/** Test data encryption **/
	if (cryptkey == NULL) {
		sprintf (buf,FILECRYPT,collname);
		f = fopen (buf,"r");
		if (f) {
			if (p = fgets (buf,STRINGLENGTH,f)) {
				if (q = index (p,'\n'))  *q = '\0';
				if (*p)  cryptkey = salloc (buf);
			}
			fclose (f);
		}
	}
	(void) netcrypt (cryptkey);
	x = msgfcrypt ();
	if (x != SCMOK)  goaway ("Error reading encryption test request from client");
	(void) netcrypt ((char *)NULL);
	if (strcmp(crypttest,CRYPTTEST) != 0)
		goaway ("Client not encrypting data properly for %s",collname);
	free (crypttest);
	crypttest = NULL;
	x = msgfcryptok ();
	if (x != SCMOK)  goaway ("Error sending encryption test reply to client");
	tnow = time ((int *)NULL);
	if (!quiet) {
		printf ("supfilesrv %d: %s %s at %s\n",fspid,
			(listonly ? "Listing files for" : "Upgrading"),
			collname,fmttime(tnow));
		fflush (stdout);
	}
}

/***************************************************************
 ***    C O N N E C T   T O   P R O P E R   A C C O U N T    ***
 ***************************************************************/

login ()
{
	char *changeuid ();
	register int x,fileuid,filegid;
	struct stat sbuf;
	char buf[STRINGLENGTH];

	(void) netcrypt (PSWDCRYPT);	/* encrypt acct name and password */
	x = msgflogin ();
	(void) netcrypt ((char *)NULL); /* turn off encryption */
	if (x != SCMOK)  goaway ("Error reading login request from client");
	if (strcmp(logcrypt,CRYPTTEST) != 0) {
		logack = FLOGNG;
		logerr = "Improper login encryption";
		(void) msgflogack ();
		goaway ("Client not encrypting login information properly");
	}
	free (logcrypt);
	logcrypt = NULL;
	if (loguser == NULL) {
		if (cryptkey) {
			sprintf (buf,FILECRYPT,collname);
			if (stat (buf,&sbuf) == 0) {
				fileuid = sbuf.st_uid;
				filegid = sbuf.st_gid;
				loguser = NULL;
			} else
				loguser = salloc (DEFUSER);
		} else
			loguser = salloc (DEFUSER);
	}
	if ((logerr = changeuid (loguser,logpswd,fileuid,filegid)) != NULL) {
		logack = FLOGNG;
		(void) msgflogack ();
		goaway ("Client denied login access");
	}
	if (loguser)  free (loguser);
	if (logpswd)  free (logpswd);
	logack = FLOGOK;
	x = msgflogack ();
	if (x != SCMOK)  goaway ("Error sending login reply to client");
	(void) netcrypt (cryptkey);	/* restore desired encryption */
	free (cryptkey);
	cryptkey = NULL;
}

/*****************************************
 ***    M A K E   N A M E   L I S T    ***
 *****************************************/

listfiles ()
{
	int denyone();
	register int x;

	refuseT = NULL;
	x = msgfrefuse ();
	if (x != SCMOK)  goaway ("Error reading refuse list from client");
	getscan ();
	Tfree (&refuseT);
	x = msgflist ();
	if (x != SCMOK)  goaway ("Error sending file list to client");
	if (prefix)  chdir (prefix);
	needT = NULL;
	x = msgfneed ();
	if (x != SCMOK)  goaway ("Error reading needed files list from client");
	denyT = NULL;
	(void) Tprocess (needT,denyone);
	Tfree (&needT);
	x = msgfdeny ();
	if (x != SCMOK)  goaway ("Error sending denied files list to client");
	Tfree (&denyT);
}

denyone (t)
register TREE *t;
{
	register char *name = t->Tname;
	register int update = (t->Tflags&FUPDATE) != 0;
	struct stat sbuf;
	register TREE *tlink;
	TREE *linkcheck ();
	char slinkname[STRINGLENGTH];
	register int x;

	t = Tsearch (listT,name);
	if (t == NULL) {
		(void) Tinsert (&denyT,name,FALSE);
		return (SCMOK);
	}
	if ((t->Tmode&S_IFMT) == S_IFLNK) {
		if (lstat(name,&sbuf) < 0) {
			(void) Tinsert (&denyT,name,FALSE);
			return (SCMOK);
		}
	} else {
		if (stat(name,&sbuf) < 0) {
			(void) Tinsert (&denyT,name,FALSE);
			return (SCMOK);
		}
	}
	if ((sbuf.st_mode&S_IFMT) != (t->Tmode&S_IFMT)) {
		(void) Tinsert (&denyT,name,FALSE);
		return (SCMOK);
	}
	switch (t->Tmode&S_IFMT) {
	case S_IFLNK:
		if ((x = readlink (name,slinkname,STRINGLENGTH)) <= 0) {
			(void) Tinsert (&denyT,name,FALSE);
			return (SCMOK);
		}
		slinkname[x] = '\0';
		(void) Tinsert (&t->Tlink,slinkname,FALSE);
		break;
	case S_IFREG:
		if (sbuf.st_nlink > 1 &&
		    (tlink = linkcheck (t,(int)sbuf.st_dev,(int)sbuf.st_ino))) {
			(void) Tinsert (&tlink->Tlink,name,FALSE);
			return (SCMOK);
		}
		if (update)  t->Tflags |= FUPDATE;
	case S_IFDIR:
		t->Tuid = sbuf.st_uid;
		t->Tgid = sbuf.st_gid;
		break;
	default:
		(void) Tinsert (&denyT,name,FALSE);
		return (SCMOK);
	}
	t->Tflags |= FNEEDED;
	return (SCMOK);
}

/*********************************
 ***    S E N D   F I L E S    ***
 *********************************/

sendfiles ()
{
	int sendone(),senddir(),sendfile();
	register int x;

	(void) Tprocess (listT,sendone); /* send all files */
	(void) Trprocess (listT,senddir); /* send directories in reverse order */
	x = msgfsend ();
	if (x != SCMOK)  goaway ("Error reading receive file request from client");
	upgradeT = NULL;
	x = msgfrecv (sendfile);
	if (x != SCMOK)  goaway ("Error sending file to client");
	if (protver < 4)  execfiles ();
	Tfree (&listT);
}

sendone (t)
TREE *t;
{
	register int x,fd;
	char *uconvert(),*gconvert();
	int sendfile ();

	if ((t->Tflags&FNEEDED) == 0)	/* only send needed files */
		return (SCMOK);
	if ((t->Tmode&S_IFMT) == S_IFDIR) /* send no directories this pass */
		return (SCMOK);
	x = msgfsend ();
	if (x != SCMOK)  goaway ("Error reading receive file request from client");
	upgradeT = t;			/* upgrade file pointer */
	fd = -1;			/* no open file */
	if ((t->Tmode&S_IFMT) == S_IFREG) {
		if (!listonly && (t->Tflags&FUPDATE) == 0) {
			fd = open (t->Tname,O_RDONLY,0);
			if (fd < 0)  t->Tmode = 0;
		}
		if (t->Tmode) {
			t->Tuser = salloc (uconvert (t->Tuid));
			t->Tgroup = salloc (gconvert (t->Tgid));
		}
	}
	msgfrecv (sendfile,fd);
	if (x != SCMOK)  goaway ("Error sending file to client");
	return (SCMOK);
}

senddir (t)
TREE *t;
{
	register int x;
	char *uconvert(),*gconvert();
	int sendfile ();

	if ((t->Tflags&FNEEDED) == 0)	/* only send needed files */
		return (SCMOK);
	if ((t->Tmode&S_IFMT) != S_IFDIR) /* send only directories this pass */
		return (SCMOK);
	x = msgfsend ();
	if (x != SCMOK)  goaway ("Error reading receive file request from client");
	upgradeT = t;			/* upgrade file pointer */
	t->Tuser = salloc (uconvert (t->Tuid));
	t->Tgroup = salloc (gconvert (t->Tgid));
	msgfrecv (sendfile,0);
	if (x != SCMOK)  goaway ("Error sending file to client");
	return (SCMOK);
}

sendfile (t,fd)
register TREE *t;
int *fd;
{
	register int x;
	if ((t->Tmode&S_IFMT) != S_IFREG || listonly || (t->Tflags&FUPDATE))
		return (SCMOK);
	x = writefile (*fd);
	if (x != SCMOK)  goaway ("Error sending file to client");
	close (*fd);
	return (SCMOK);
}

/***********************************************************
 ***    S E N D   E X E C U T E   F I L E   N A M E S    ***
 ***********************************************************/

execfiles ()
{
	register int x;
	int execone ();

	x = readmnull (MSGFEXECQ);
	if (x != SCMOK)  goaway ("Client not requesting execute file");
	x = writemsg (MSGFEXECNAMES);
	if (x != SCMOK)  goaway ("Error sending execute file reply");
	x = Tprocess (listT,execone);
	if (x != SCMOK)  goaway ("Error sending execute file");
	x = writemend ();
	if (x != SCMOK)  goaway ("Error sending execute file end");
}

execone (t)
register TREE *t;
{
	int execsub ();

	if (t->Tflags&FCOMPAT)  return (Tprocess (t->Texec,execsub));
	return (SCMOK);
}

execsub (t)
register TREE *t;
{
	return (writestring (t->Tname));
}

/*****************************************
 ***    E N D   C O N N E C T I O N    ***
 *****************************************/

finishup (starttime)
int starttime;
{
	char buf[STRINGLENGTH];
	register FILE *f;
	struct stat sbuf;
	int finishtime;

	(void) netcrypt ((char *)NULL);
	goawayreason = NULL;
	(void) msggoaway ();
	if (prefix)  chdir (basedir);
	sprintf (buf,FILELOGFILE,collname);
	if (stat (buf,&sbuf) >= 0 && (f = fopen (buf,"a"))) {
		finishtime = time ((int *)NULL);
		fprintf (f,"%s ",fmttime (lasttime));
		fprintf (f,"%s ",fmttime (starttime));
		fprintf (f,"%s ",fmttime (finishtime));
		fprintf (f,"%s\n",remotehost ());
		fclose (f);
	}
}

/***************************************************
 ***    H A S H   T A B L E   R O U T I N E S    ***
 ***************************************************/

Hfree (table)
HASH **table;
{
	register HASH *h;
	register int i;
	for (i = 0; i < HASHSIZE; i++)
		while (h = table[i]) {
			table[i] = h->Hnext;
			if (h->Hname)  free (h->Hname);
			free ((char *)h);
		}
}

HASH *Hlookup (table,num1,num2)
HASH **table;
int num1,num2;
{
	register HASH *h;
	register int hno;
	hno = HASHFUNC(num1,num2);
	for (h = table[hno]; h && (h->Hnum1 != num1 || h->Hnum2 != num2); h = h->Hnext);
	return (h);
}

Hinsert (table,num1,num2,name,tree)
HASH **table;
int num1,num2;
char *name;
TREE *tree;
{
	register HASH *h;
	register int hno;
	hno = HASHFUNC(num1,num2);
	h = (HASH *) malloc (sizeof(HASH));
	h->Hnum1 = num1;
	h->Hnum2 = num2;
	h->Hname = name;
	h->Htree = tree;
	h->Hnext = table[hno];
	table[hno] = h;
}

/*********************************************
 ***    U T I L I T Y   R O U T I N E S    ***
 *********************************************/

TREE *linkcheck (t,d,i)
TREE *t;
int d,i;			/* inode # and device # */
{
	register HASH *h;
	h = Hlookup (inodeH,i,d);
	if (h)  return (h->Htree);
	Hinsert (inodeH,i,d,(char *)NULL,t);
	return ((TREE *)NULL);
}

char *uconvert (uid)
int uid;
{
	register struct passwd *pw;
	register char *p;
	register HASH *u;
	u = Hlookup (uidH,uid,0);
	if (u)  return (u->Hname);
	pw = getpwuid (uid);
	if (pw == NULL)  return ("");
	p = salloc (pw->pw_name);
	Hinsert (uidH,uid,0,p,(TREE*)NULL);
	return (p);
}

char *gconvert (gid)
int gid;
{
	register struct group *gr;
	register char *p;
	register HASH *g;
	g = Hlookup (gidH,gid,0);
	if (g)  return (g->Hname);
	gr = getgrgid (gid);
	if (gr == NULL)  return ("");
	p = salloc (gr->gr_name);
	Hinsert (gidH,gid,0,p,(TREE *)NULL);
	return (p);
}

char *changeuid (namep,passwordp,fileuid,filegid)
char *namep,*passwordp;
int fileuid,filegid;
{
	char *okpassword ();
	char *group,*account,*pswdp;
	struct passwd *pwd;
	struct group *grp;
	struct account *acc;
	struct ttyloc tlc;
	register int status = ACCESS_CODE_OK;
	char nbuf[STRINGLENGTH];
	static char errbuf[STRINGLENGTH];
	int *grps;
	char *p;

	if (namep == NULL) {
		pwd = getpwuid (fileuid);
		if (pwd == NULL)
			return (sprintf (errbuf,"Reason:  Unknown user id %d",fileuid));
		grp = getgrgid (filegid);
		if (grp)  group = strcpy (nbuf,grp->gr_name);
		else  group = NULL;
		account = NULL;
		pswdp = NULL;
	} else {
		strcpy (nbuf,namep);
		account = group = index (nbuf,',');
		if (group != NULL) {
			*group++ = '\0';
			account = index (group,',');
			if (account != NULL) {
				*account++ = '\0';
				if (*account == '\0')  account = NULL;
			}
			if (*group == '\0')  group = NULL;
		}
		pwd = getpwnam (nbuf);
		if (pwd == NULL)
			return (sprintf (errbuf,"Reason:  Unknown user %s",nbuf));
		if (strcmp (nbuf,DEFUSER) == 0)
			pswdp = NULL;
		else
			pswdp = passwordp ? passwordp : "";
	}
	if (getuid ()) {
		if (getuid () == pwd->pw_uid)
			return (NULL);
		if (strcmp (pwd->pw_name,DEFUSER) == 0)
			return (NULL);
		fprintf (stderr,"supfilesrv %d: File server not superuser\n",fspid);
		fflush (stderr);
		return ("Reason:  fileserver is not running privileged");
	}
	tlc.tlc_hostid = TLC_UNKHOST;
	tlc.tlc_ttyid = TLC_UNKTTY;
	if (okaccess(pwd->pw_name,ACCESS_TYPE_SU,0,-1,tlc) != 1)
		status = ACCESS_CODE_DENIED;
	else {
		grp = NULL;
		acc = NULL;
		status = oklogin(pwd->pw_name,group,&account,pswdp,&pwd,&grp,&acc,&grps);
		if (status == ACCESS_CODE_OK) {
			if ((p = okpassword(pswdp,pwd->pw_name,pwd->pw_gecos)) != NULL)
				status = ACCESS_CODE_INSECUREPWD;
		}
	}
	switch (status) {
	case ACCESS_CODE_INSECUREPWD:
		p = sprintf (errbuf,"Reason:  %s",p);
		break;
	case ACCESS_CODE_OK:
		break;
	case ACCESS_CODE_DENIED:
		p = "Reason:  Access denied";
		break;
	case ACCESS_CODE_NOUSER:
		p = errbuf;
		break;
	case ACCESS_CODE_BADPASSWORD:
		p = "Reason:  Invalid password";
		break;
	case ACCESS_CODE_ACCEXPIRED:
		p = "Reason:  Account expired";
		break;
	case ACCESS_CODE_GRPEXPIRED:
		p = "Reason:  Group expired";
		break;
	case ACCESS_CODE_ACCNOTVALID:
		p = "Reason:  Invalid account";
		break;
	case ACCESS_CODE_MANYDEFACC:
		p = "Reason:  User has more than one default account";
		break;
	case ACCESS_CODE_NOACCFORGRP:
		p = "Reason:  No account for group";
		break;
	case ACCESS_CODE_NOGRPFORACC:
		p = "Reason:  No group for account";
		break;
	case ACCESS_CODE_NOGRPDEFACC:
		p = "Reason:  No group for default account";
		break;
	case ACCESS_CODE_NOTGRPMEMB:
		p = "Reason:  Not member of group";
		break;
	case ACCESS_CODE_NOTDEFMEMB:
		p = "Reason:  Not member of default group";
		break;
	case ACCESS_CODE_OOPS:
		p = "Reason:  Internal error";
		break;
	default:
		p = sprintf (errbuf,"Reason:  Status %d",status);
		break;
	}
	if (pwd == NULL)
		return (p);
	if (status != ACCESS_CODE_OK) {
		fprintf (stderr,"supfilesrv %d: Login failure for %s\n",fspid,pwd->pw_name);
		fprintf (stderr,"supfilesrv %d: %s\n",fspid,p);
		fflush (stderr);
#ifdef	CMUCS
		logaccess (pwd->pw_name,ACCESS_TYPE_SUP,status,0,-1,tlc);
#endif	CMUCS
		return (p);
	}
	if (setgroups (grps[0], &grps[1]) < 0)
		perror ("setgroups");
#ifdef	CMUCS
	if (acc && setaid (acc->ac_aid) < 0)
		perror ("setaid");
#endif	CMUCS
	if (setgid (grp->gr_gid) < 0)
		perror ("setgid");
	if (setuid (pwd->pw_uid) < 0)
		perror ("setuid");
	return (NULL);
}

/* VARARGS1 */
goaway (fmt,args)
char *fmt;
{
	struct _iobuf _strbuf;
	char buf[STRINGLENGTH];

	(void) netcrypt ((char *)NULL);
	_strbuf._flag = _IOWRT+_IOSTRG;
	_strbuf._ptr = buf;
	_strbuf._cnt = sizeofS(buf);
	_doprnt(fmt, &args, &_strbuf);
	putc('\0', &_strbuf);
	goawayreason = buf;
	(void) msggoaway ();
	fprintf (stderr,"supfilesrv %d: %s\n",fspid,buf);
	fflush (stderr);
	longjmp (sjbuf,TRUE);
}

char *fmttime (time)
int time;
{
	static char buf[STRINGLENGTH];
	int len;

	strcpy (buf,ctime (&time));
	len = strlen(buf+4)-6;
	strncpy (buf,buf+4,len);
	buf[len] = '\0';
	return (buf);
}

/* supnamesrv -- SUP Name Server
 *
 * Usage:  supnamesrv [-l] [-q] [-P] [-N]
 *	-l	"live" -- don't fork daemon
 *	-q	"quiet" -- don't print log message for each connection
 *	-P	"debug ports" -- use debugging network ports
 *	-N	"debug network" -- print network i/o debug output
 *
 **********************************************************************
 * HISTORY
 * 04-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed name server protocol to support multiple repositories
 *	per collection. [V5.3]
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Minor rewrite for protocol version 4. [V4.2]
 *
 * 13-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added comments to collection host file.
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
#include <c.h>
#include <signal.h>
#include "sup.h"
#define MSGNAME
#include "supmsg.h"

#define PGMVERSION 3

/*******************************************
 ***    D A T A   S T R U C T U R E S    ***
 *******************************************/

struct namestruct {			/* name list */
	char *Ncoll;			/* collection name */
	TREE *Nhost;			/* host name tree */
	struct namestruct *Nnext;	/* next */
};
typedef struct namestruct NAME;

/*************************
 ***    M A C R O S    ***
 *************************/

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

char program[] = "supnamesrv";		/* program name for SCM messages */
int progpid = -1;			/* and process id */

NAME *firstN;				/* head of name list */

int live;				/* -l switch */
int quiet;				/* -q switch */
int dbgportsq;				/* -P switch */
extern int scmdebug;			/* -N switch */

/*************************************
 ***    M A I N   R O U T I N E    ***
 *************************************/

main (argc,argv)
int argc;
char **argv;
{
	register int x;
	struct sigvec ignvec,oldvec;

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
		if (x != SCMOK)  quit (1,"supnamesrv: can't connect to network\n");
		answer ();
		(void) serviceend ();
		exit (0);
	}
	ignvec.sv_handler = SIG_IGN;
	ignvec.sv_onstack = 0;
	ignvec.sv_mask = 0;
	sigvec (SIGHUP,&ignvec,&oldvec);
	sigvec (SIGINT,&ignvec,&oldvec);
	close (2);
	dup (1);			/* redirect error output */
	for (;;) {
		x = service ();
		if (x != SCMOK)
			quit (1,"supnamesrv: error in establishing network connection\n");
		if (dfork() == 0) {	/* server process */
			(void) serviceprep ();
			answer ();
			(void) serviceend ();
			exit (0);
		}
		(void) servicekill ();	/* parent */
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
	int x;

	live = FALSE;
	quiet = FALSE;
	dbgportsq = FALSE;
	scmdebug = FALSE;

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
			scmdebug = TRUE;
			break;
		default:
			quit (1,"Invalid flag %s\n",argv[1]);
		}
		--argc;
		argv++;
	}
	if (argc != 1)
		quit (1,"Usage:  supnamesrv [-l] [-q] [-P] [-N]\n");
	if (dbgportsq)
		x = servicesetup (DEBUGNPORT);
	else
		x = servicesetup (NAMEPORT);
	if (x != SCMOK)
		quit (1,"supnamesrv: error in network setup\n");
	tloc = time ((int *)NULL);
	printf ("SUP Name Server Version %d.%d (%s) starting at %s",
		PROTOVERSION,PGMVERSION,scmversion,ctime(&tloc));
}

/*****************************************
 ***    A N S W E R   R E Q U E S T    ***
 *****************************************/

answer ()
{
	register NAME *n;
	register int x;
	long tloc;

	progpid = nspid = getpid ();
	makeNlist ();			/* make name list */
	x = msgnsignon ();
	if (x != SCMOK)  goaway ("error reading signon request from client");
	x = msgnsignonack ();
	if (x != SCMOK)  goaway ("error sending signon reply to client");
	if (!quiet) {
		tloc = time ((int *)NULL);
		printf ("supnamesrv %d: SUP %d.%d (%s) @ %s %s",
			nspid,protver,pgmver,scmver,remotehost(),ctime(&tloc));
	}
	free (scmver);
	scmver = NULL;
	for (;;) {			/* for each collection */
		x = msgncoll ();
		if (x != SCMOK) {
			if (x == SCMEOF)  break;
			goaway ("error reading collection name from client");
		}
		for (n = firstN; n && strcmp(collname,n->Ncoll) != 0; n = n->Nnext);
		free (collname);
		collname = NULL;
		hostT = n ? n->Nhost : NULL;
		x = msgnhost ();
		if (x != SCMOK)  goaway ("error sending host name to client");
	}
	while (n = firstN) {
		firstN = firstN->Nnext;
		free (n->Ncoll);
		Tfree (&n->Nhost);
		free ((char *)n);
	}
}

/*****************************************
 ***    M A K E   N A M E   L I S T    ***
 *****************************************/

makeNlist ()
{
	register FILE *f;
	register NAME *n;
	char buf[STRINGLENGTH],*p;
	register char *q;
	NAME *lastN;

	firstN = lastN = NULL;
	sprintf (buf,FILENCOLLS,DEFDIR);
	f = fopen (buf,"r");
	if (f == NULL)  goaway ("can't open %s",buf);
	while (p = fgets(buf,STRINGLENGTH,f)) {
		if (q = index (p,'\n'))  *q = '\0';
		if (index ("#;:",*p))  continue;
		q = nxtarg (&p,"= \t");
		p = skipover (p," \t");
		if (*p == '=')  p++;
		p = skipover (p," \t");
		if (*p == '\0')  goaway ("error in collection/host file");
		n = (NAME *) malloc (sizeof(NAME));
		if (firstN == NULL)  firstN = n;
		if (lastN != NULL)  lastN->Nnext = n;
		lastN = n;
		n->Nnext = NULL;
		n->Ncoll = salloc (q);
		n->Nhost = NULL;
		do {
			q = nxtarg (&p,", \t");
			p = skipover (p," \t");
			if (*p == ',')  p++;
			p = skipover (p," \t");
			(void) Tinsert (&n->Nhost,q,FALSE);
		} while (*p != '\0');
	}
	fclose (f);
}

/* VARARGS1 */
goaway (fmt,args)
char *fmt;
{
	struct _iobuf _strbuf;
	char buf[STRINGLENGTH];

	_strbuf._flag = _IOWRT+_IOSTRG;
	_strbuf._ptr = buf;
	_strbuf._cnt = sizeofS(buf);
	_doprnt(fmt, &args, &_strbuf);
	putc('\0', &_strbuf);
	goawayreason = buf;
	(void) msggoaway ();
	quit (1,"supnamesrv %d: %s\n",nspid,buf);
}

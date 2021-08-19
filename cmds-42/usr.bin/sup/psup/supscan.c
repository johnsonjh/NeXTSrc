/* supscan -- SUP Scan File Builder
 *
 * Usage:  supscan [ -[vs] ] [ collection ] [ basedir ]
 *	-v	"verbose" -- print messages as you go
 *	-s	"system" -- perform scan for system supfile
 *	collection	-- name of the desired collection if not -s
 *	basedir		-- name of the base directory, if not
 *				the default or recorded in coll.dir
 *
 **********************************************************************
 * HISTORY
 * 05-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed collection setup errors to be non-fatal. [V5.3]
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Moved most of the scanning code to scan.c. [V4.2]
 *
 * 02-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added "-s" option.
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
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include "sup.h"

#define PGMVERSION 3

/*******************************************
 ***    D A T A   S T R U C T U R E S    ***
 *******************************************/

struct collstruct {			/* one per collection to be upgraded */
	char *Cname;			/* collection name */
	char *Cbase;			/* local base directory */
	char *Cprefix;			/* local collection pathname prefix */
	struct collstruct *Cnext;	/* next collection */
};
typedef struct collstruct COLLECTION;

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

int trace;				/* -v flag */
int sflag;				/* -s flag */

COLLECTION *firstC;			/* collection list pointer */
char *collname;				/* collection name */
char *basedir;				/* base directory name */
char *prefix;				/* collection pathname prefix */
int lasttime = 0;			/* time of last upgrade */
int scantime;				/* time of this scan */
int newonly = FALSE;			/* new files only */

TREE *listT;				/* list of all files specified by <coll>.list */
TREE *refuseT = NULL;			/* list of all files specified by <coll>.list */

/*************************************
 ***    M A I N   R O U T I N E    ***
 *************************************/

main (argc,argv)
int argc;
char **argv;
{
	register COLLECTION *c;

	init (argc,argv);		/* process arguments */
	for (c = firstC; c; c = c->Cnext) {
		collname = c->Cname;
		basedir = c->Cbase;
		prefix = c->Cprefix;
		chdir (basedir);
		scantime = time ((int *)NULL);
		printf ("SUP Scan for %s starting at %s",collname,ctime (&scantime));
		fflush (stdout);
		makescan ();		/* record names in scan file */
		scantime = time ((int *)NULL);
		printf ("SUP Scan for %s completed at %s",collname,ctime (&scantime));
		fflush (stdout);
	}
	while (c = firstC) {
		firstC = firstC->Cnext;
		free (c->Cname);
		free (c->Cbase);
		if (c->Cprefix)  free (c->Cprefix);
		free ((char *)c);
	}
	exit (0);
}

/*****************************************
 ***    I N I T I A L I Z A T I O N    ***
 *****************************************/

init (argc,argv)
int argc;
char **argv;
{
	char buf[STRINGLENGTH],*p,*q;
	FILE *f;
	COLLECTION **c, *getcoll();

	trace = FALSE;
	sflag = FALSE;
	while (argc > 1 && argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 'v':
			trace = TRUE;
			break;
		case 's':
			sflag = TRUE;
			break;
		default:
			fprintf (stderr,"supscan: Invalid flag %s ignored\n",argv[1]);
			fflush (stderr);
		}
		--argc;
		argv++;
	}
	if (sflag) {
		if (argc != 1)
			quit (1,"Usage:  supscan [ -[vs] ] [ collection ] [ basedir ]\n");
		firstC = NULL;
		c = &firstC;
		sprintf (buf,FILENCOLLS,DEFDIR);
		if (f = fopen (buf,"r")) {
			while ((p = fgets (buf,STRINGLENGTH,f)) != NULL) {
				q = index (p,'\n');
				if (q)  *q = 0;
				if (index ("#;:",*p))  continue;
				collname = nxtarg (&p," \t=");
				p = skipover (p," \t=");
				if (!localhost (p))  continue;
				*c = getcoll(salloc (collname), (char *)NULL);
				if (*c)  c = &((*c)->Cnext);
			}
			fclose (f);
		}
		return;
	}
	if (argc < 2 || argc > 3)
		quit (1,"Usage:  supscan [ -[vs] ] [ collection ] [ basedir ]\n");
	firstC = getcoll(salloc (argv[1]), argc > 2 ? salloc (argv[2]) : (char *)NULL);
}

COLLECTION *
getcoll(collname, basedir)
register char *collname, *basedir;
{
	char buf[STRINGLENGTH],*p,*q;
	FILE *f;
	COLLECTION *c;

	if (basedir == NULL) {
		sprintf (buf,FILEDIRS,DEFDIR);
		if (f = fopen (buf,"r")) {
			while (p = fgets (buf,STRINGLENGTH,f)) {
				q = index (p,'\n');
				if (q)  *q = 0;
				if (index ("#;:",*p))  continue;
				q = nxtarg (&p," \t=");
				if (strcmp (q,collname) == 0) {
					p = skipover (p," \t=");
					basedir = salloc (p);
					break;
				}
			}
			fclose (f);
		}
		if (basedir == NULL) {
			sprintf (buf,FILEBASEDEFAULT,collname);
			basedir = salloc (buf);
		}
	}
	if (chdir(basedir) < 0) {
		fprintf (stderr,"supscan:  Can't chdir to base directory %s for %s\n",
			basedir,collname);
		return (NULL);
	}
	prefix = NULL;
	sprintf (buf,FILEPREFIX,collname);
	if (f = fopen (buf,"r")) {
		while (p = fgets (buf,STRINGLENGTH,f)) {
			q = index (p,'\n');
			if (q) *q = 0;
			if (index ("#;:",*p))  continue;
			prefix = salloc (p);
			if (chdir(prefix) < 0) {
				fprintf (stderr,"supscan: can't chdir to %s from base directory %s for %s\n",
					prefix,basedir,collname);
				return (NULL);
			}
			break;
		}
		fclose (f);
	}
	if ((c = (COLLECTION *) malloc (sizeof(COLLECTION))) == NULL)
		quit (1,"supscan: can't malloc collection structure\n");
	c->Cname = collname;
	c->Cbase = basedir;
	c->Cprefix = prefix;
	c->Cnext = NULL;
	return (c);
}

/* VARARGS1 */
goaway (fmt,args)
char *fmt;
{
	_doprnt(fmt, &args, stderr);
	putc ('\n',stderr);
	fflush (stderr);
	exit (1);
}

int localhost (host)
register char *host;
{
	register struct hostent *h;
	static char myhost[STRINGLENGTH];
	if (*myhost == '\0') {
		gethostname (myhost,STRINGLENGTH);
		h = gethostbyname (myhost);
		if (h == NULL) quit (1,"supscan: can't find my host entry\n");
		strcpy (myhost,h->h_name);
	}
	h = gethostbyname (host);
	if (h == NULL) return (0);
	return (strcmp (myhost,h->h_name) == 0);
}

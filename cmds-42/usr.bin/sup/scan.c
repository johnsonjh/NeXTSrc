/*
 * scan.c - sup list file scanner
 *
 **********************************************************************
 * HISTORY
 * 21-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added check for newonly upgrade when lasttime is the same as
 *	scantime.  This will save us the trouble of parsing the scanfile
 *	when the client has successfully received everything in the
 *	scanfile already.
 *
 * 16-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Clear Texec pointers in execT so that Tfree of execT will not
 *	free command trees associated with files in listT.
 *
 * 06-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to omit scanned files from list if we want new files
 *	only and they are old.
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Major rewrite for protocol version 4.  Added version numbers to
 *	scan file.  Also added mode of file in addition to flags.
 *	Execute commands are now immediately after file information.
 *
 * 13-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added comments to list file format.
 *
 * 08-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to implement omitany.  Currently doesn't know about
 *	{a,b,c} patterns.
 *
 * 07-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#include <libc.h>
#include <c.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include "sup.h"

/*************************
 ***    M A C R O S    ***
 *************************/

#define SPECNUMBER 1000
	/* number of filenames produced by a single spec in the list file */

/*******************************************
 ***    D A T A   S T R U C T U R E S    ***
 *******************************************/

typedef enum {			/* <collection>/list file lines */
	LUPGRADE,	LOMIT,		LBACKUP,	LEXECUTE,
	LINCLUDE,	LNOACCT,	LOMITANY,	LALWAYS,
	LSYMLINK
} LISTTYPE;

static char *ltname[] = {
	"upgrade",	"omit",		"backup",	"execute",
	"include",	"noaccount",	"omitany",	"always",
	"symlink",
	0
};

#define FALWAYS		FUPDATE

/* <coll>/list file lines */
static TREE *upgT;			/* files to upgrade */
static TREE *flagsT;			/* special flags for files: BACKUP NOACCT */
static TREE *omitT;			/* recursize file omition list */
static TREE *omanyT;			/* non-recursize file omition list */
static TREE *symT;			/* symbolic links to quote */
static TREE *execT;			/* execution files matching <coll>/list */

/*************************
 ***    E X T E R N    ***
 *************************/

extern char _argbreak;			/* break character from nxtarg */

extern TREE *listT;			/* list of all files matching <coll>/list */
extern TREE *refuseT;			/* list of files refused in <coll>/list */

extern char *collname;			/* collection name */
extern char *basedir;			/* base directory name */
extern char *prefix;			/* collection pathname prefix */
extern int lasttime;			/* time of last upgrade */
extern int scantime;			/* time of this scan */
extern int trace;			/* trace directories */
extern int newonly;			/* new files only */

/*************************************************
 ***    L I S T   S C A N   R O U T I N E S    ***
 *************************************************/

makescan ()
{
	listT = NULL;
	doscan ();			/* read list file and scan disk */
	makescanfile ();		/* record names in scan file */
	Tfree (&listT);			/* free file list tree */
}

getscan ()
{
	listT = NULL;
	if (!getscanfile()) {		/* check for pre-scanned file list */
		scantime = time ((int *)NULL);
		doscan ();		/* read list file and scan disk */
	}
}

static
doscan ()
{
	char buf[STRINGLENGTH];
	int listone ();

	upgT = NULL;
	flagsT = NULL;
	omitT = NULL;
	omanyT = NULL;
	execT = NULL;
	symT = NULL;
	sprintf (buf,FILELIST,collname);
	readlistfile (buf);		/* get contents of list file */
	(void) Tprocess (upgT,listone); /* build list of files specified */
	if (prefix)  chdir (basedir);
	Tfree (&upgT);
	Tfree (&flagsT);
	Tfree (&omitT);
	Tfree (&omanyT);
	Tfree (&execT);
	Tfree (&symT);
}

static
readlistfile (fname)
char *fname;
{
	char buf[STRINGLENGTH],*p;
	register char *q,*r;
	register FILE *f;
	register int ltn,n,i,flags;
	register TREE **t;
	register LISTTYPE lt;
	char *speclist[SPECNUMBER];

	f = fopen (fname,"r");
	if (f == NULL)  goaway ("Can't read list file %s",fname);
	if (prefix)  chdir (prefix);
	while (p = fgets (buf,STRINGLENGTH,f)) {
		if (q = index (p,'\n'))  *q = '\0';
		if (index ("#;:",*p))  continue;
		q = nxtarg (&p," \t");
		if (*q == '\0') continue;
		for (ltn = 0; ltname[ltn] && strcmp(q,ltname[ltn]) != 0; ltn++);
		if (ltname[ltn] == NULL)
			goaway ("Invalid list file keyword %s",q);
		lt = (LISTTYPE) ltn;
		flags = 0;
		switch (lt) {
		case LUPGRADE:
			t = &upgT;
			break;
		case LBACKUP:
			t = &flagsT;
			flags = FBACKUP;
			break;
		case LNOACCT:
			t = &flagsT;
			flags = FNOACCT;
			break;
		case LSYMLINK:
			t = &symT;
			break;
		case LOMIT:
			t = &omitT;
			break;
		case LOMITANY:
			t = &omanyT;
			break;
		case LALWAYS:
			t = &upgT;
			flags = FALWAYS;
			break;
		case LINCLUDE:
			while (*(q=nxtarg(&p," \t"))) {
				if (prefix)  chdir (basedir);
				n = expand (q,speclist,SPECNUMBER);
				for (i = 0; i < n && i < SPECNUMBER; i++) {
					readlistfile (speclist[i]);
					if (prefix)  chdir (basedir);
					free (speclist[i]);
				}
				if (prefix)  chdir (prefix);
			}
			continue;
		case LEXECUTE:
			r = p = q = skipover (p," \t");
			do {
				q = p = skipto (p," \t(");
				p = skipover (p," \t");
			} while (*p != '(' && *p != '\0');
			if (*p++ == '(') {
				*q = '\0';
				do {
					q = nxtarg (&p," \t)");
					if (*q == 0)
						_argbreak = ')';
					else
						expTinsert (q,&execT,0,r);
				} while (_argbreak != ')');
				continue;
			}
			/* fall through */
		default:
			goaway ("Error in handling list file keyword %d",ltn);
		}
		while (*(q=nxtarg(&p," \t"))) {
			if (lt == LOMITANY)
				(void) Tinsert (t,q,FALSE);
			else
				expTinsert (q,t,flags,(char *)NULL);
		}
	}
	fclose (f);
}

static
expTinsert (p,t,flags,exec)
char *p;
TREE **t;
int flags;
char *exec;
{
	register int n, i;
	register TREE *newt;
	char *speclist[SPECNUMBER];
	char buf[STRINGLENGTH];

	n = expand (p,speclist,SPECNUMBER);
	for (i = 0; i < n && i < SPECNUMBER; i++) {
		newt = Tinsert (t,speclist[i],TRUE);
		newt->Tflags |= flags;
		if (exec) {
			sprintf (buf,exec,speclist[i]);
			(void) Tinsert (&newt->Texec,buf,FALSE);
		}
		free (speclist[i]);
	}
}

static
listone (t)		/* expand and add one name from upgrade list */
TREE *t;
{
	listentry(t->Tname,t->Tname,(char *)NULL,(t->Tflags&FALWAYS) != 0);
	return (SCMOK);
}

static
listentry(name,fullname,updir,always)
register char *name, *fullname, *updir;
int always;
{
	struct stat statbuf;
	int link = 0;
	int omitanyone ();

	if (Tlookup (refuseT,fullname))  return;
	if (!always) {
		if (Tsearch (omitT,fullname))  return;
		if (Tprocess (omanyT,omitanyone,fullname) != SCMOK)
			return;
	}
	if (lstat(name,&statbuf) < 0)
		return;
	if ((statbuf.st_mode&S_IFMT) == S_IFLNK) {
		if (Tsearch (symT,fullname)) {
			listname (fullname,&statbuf);
			return;
		}
		if (updir) link++;
		if (stat(name,&statbuf) < 0) return;
	}
	if ((statbuf.st_mode&S_IFMT) == S_IFDIR) {
		if (access(name,05) < 0) return;
		if (chdir(name) < 0) return;
		listname (fullname,&statbuf);
		if (trace) {
			printf ("Scanning directory %s\n",fullname);
			fflush (stdout);
		}
		listdir (fullname,always);
		if (updir == 0 || link) {
			chdir (basedir);
			if (prefix) chdir (prefix);
			if (updir && *updir) chdir (updir);
		} else
			chdir ("..");
		return;
	}
	if (access(name,04) < 0) return;
	listname (fullname,&statbuf);
}

static
listname (name,st)
register char *name;
register struct stat *st;
{
	register TREE *t,*ts;
	register int new;

	new = st->st_ctime > lasttime;
	if (newonly && !new)  return;
	t = Tinsert (&listT,name,FALSE);
	if (t == NULL)  return;
	t->Tmode = st->st_mode;
	t->Tctime = st->st_ctime;
	t->Tmtime = st->st_mtime;
	if (new)  t->Tflags |= FNEW;
	if (ts = Tsearch (flagsT,name))
		t->Tflags |= ts->Tflags;
	if (ts = Tsearch (execT,name)) {
		t->Texec = ts->Texec;
		ts->Texec = NULL;
	}
}

static
listdir (name,always)		/* expand directory */
char *name;
int always;
{
	struct direct *dentry;
	register DIR *dirp;
	char ename[STRINGLENGTH],newname[STRINGLENGTH],filename[STRINGLENGTH];
	register char *p,*newp;
	register int i;

	dirp = opendir (".");
	if (dirp == 0) return;	/* unreadable: probably protected */

	p = name;		/* punt leading ./ and trailing / */
	newp = newname;
	if (p[0] == '.' && p[1] == '/') {
		p += 2;
		while (*p == '/') p++;
	}
	while (*newp++ = *p++) ;	/* copy string */
	--newp;				/* trailing null */
	while (newp > newname && newp[-1] == '/') --newp; /* trailing / */
	*newp = 0;
	if (strcmp (newname,".") == 0) newname[0] = 0;	/* "." ==> "" */

	while (dentry=readdir(dirp)) {
		if (dentry->d_ino == 0) continue;
		if (strcmp(dentry->d_name,".") == 0) continue;
		if (strcmp(dentry->d_name,"..") == 0) continue;
		for (i=0; i<=MAXNAMLEN && dentry->d_name[i]; i++)
			ename[i] = dentry->d_name[i];
		ename[i] = 0;
		if (*newname)
			sprintf (filename,"%s/%s",newname,ename);
		else
			strcpy (filename,ename);
		listentry(ename,filename,newname,always);
	}
	closedir (dirp);
}

static
omitanyone (t,filename)
TREE *t;
char **filename;
{
	if (anyglob (t->Tname,*filename))
		return (SCMERR);
	return (SCMOK);
}

static
anyglob (pattern,match)
char *pattern,*match;
{
	register char *p,*m;
	register char *pb,*pe;

	p = pattern; 
	m = match;
	while (*m && *p == *m ) { 
		p++; 
		m++; 
	}
	if (*p == '\0' && *m == '\0')
		return (TRUE);
	switch (*p++) {
	case '*':
		for (;;) {
			if (*p == '\0')
				return (TRUE);
			if (*m == '\0')
				return (*p == '\0');
			if (anyglob (p,++m))
				return (TRUE);
		}
	case '?':
		return (anyglob (p,++m));
	case '[':
		pb = p;
		while (*(++p) != ']')
			if (*p == '\0')
				return (FALSE);
		pe = p;
		for (p = pb + 1; p != pe; p++) {
			switch (*p) {
			case '-':
				if (p == pb && *m == '-') {
					p = pe + 1;
					return (anyglob (p,++m));
				}
				if (p == pb)
					continue;
				if ((p + 1) == pe)
					return (FALSE);
				if (*m > *(p - 1) &&
				    *m <= *(p + 1)) {
					p = pe + 1;
					return (anyglob (p,++m));
				}
				continue;
			default:
				if (*m == *p) {
					p = pe + 1;
					return (anyglob (p,++m));
				}
			}
		}
		return (FALSE);
	default:
		return (FALSE);
	}
}

/*****************************************
 ***    R E A D   S C A N   F I L E    ***
 *****************************************/

static
int getscanfile ()
{
	char buf[STRINGLENGTH];
	struct stat sbuf;
	register FILE *f;
	TREE ts;
	register char *p,*q;
	register TREE *t = NULL;
	register notwanted;

	sprintf (buf,FILESCAN,collname);
	if (stat(buf,&sbuf) < 0)
		return (FALSE);
	if ((f = fopen (buf,"r")) == NULL)
		return (FALSE);
	if ((p = fgets (buf,STRINGLENGTH,f)) == NULL) {
		fclose (f);
		return (FALSE);
	}
	if (q = index (p,'\n'))  *q = '\0';
	if (*p++ != 'V') {
		fclose (f);
		return (FALSE);
	}
	if (atoi (p) != SCANVERSION) {
		fclose (f);
		return (FALSE);
	}
	scantime = sbuf.st_mtime;	/* upgrade time is time of supscan,
					 * i.e. time of creation of scanfile */
	if (newonly && scantime == lasttime) {
		fclose (f);
		return (TRUE);
	}
	notwanted = FALSE;
	while (p = fgets (buf,STRINGLENGTH,f)) {
		q = index (p,'\n');
		if (q)  *q = 0;
		ts.Tflags = 0;
		if (*p == 'X') {
			if (notwanted)  continue;
			p++;
			if (t == NULL)  goaway ("scanfile format inconsistant");
			(void) Tinsert (&t->Texec,p,FALSE);
			continue;
		}
		notwanted = FALSE;
		if (*p == 'B') {
			p++;
			ts.Tflags |= FBACKUP;
		}
		if (*p == 'N') {
			p++;
			ts.Tflags |= FNOACCT;
		}
		if ((q = index (p,' ')) == NULL)
			goaway ("scanfile format inconsistant");
		*q++ = '\0';
		ts.Tmode = atoo (p);
		p = q;
		if ((q = index (p,' ')) == NULL)
			goaway ("scanfile format inconsistant");
		*q++ = '\0';
		ts.Tctime = atoi (p);
		p = q;
		if (ts.Tctime > lasttime)
			ts.Tflags |= FNEW;
		else if (newonly) {
			notwanted = TRUE;
			continue;
		}
		if ((q = index (p,' ')) == NULL)
			goaway ("scanfile format inconsistant");
		*q++ = 0;
		ts.Tmtime = atoi (p);
		if (Tlookup (refuseT,q)) {
			notwanted = TRUE;
			continue;
		}
		t = Tinsert (&listT,q,TRUE);
		t->Tmode = ts.Tmode;
		t->Tflags = ts.Tflags;
		t->Tctime = ts.Tctime;
		t->Tmtime = ts.Tmtime;
	}
	fclose (f);
	return (TRUE);
}

/*******************************************
 ***    W R I T E   S C A N   F I L E    ***
 *******************************************/

static makescanfile ()
{
	char tname[STRINGLENGTH],fname[STRINGLENGTH];
	struct timeval tbuf[2];
	FILE *scanfile;			/* output file for scanned file list */
	int recordone ();

	sprintf (tname,FILESCANTEMP,collname);
	scanfile = fopen (tname,"w");
	if (scanfile == NULL)
		goaway ("Can't write scan file temp %s for %s",tname,collname);
	fprintf (scanfile,"V%d\n",SCANVERSION);
	(void) Tprocess (listT,recordone,scanfile);
	fclose (scanfile);
	sprintf (fname,FILESCAN,collname);
	if (rename (tname,fname) < 0)
		goaway ("Can't change %s to %s",tname,fname);
	unlink (tname);
	tbuf[0].tv_sec = time((int *)NULL);  tbuf[0].tv_usec = 0;
	tbuf[1].tv_sec = scantime;  tbuf[1].tv_usec = 0;
	utimes (fname,tbuf);
}

static
recordone (t,scanfile)
TREE *t;
FILE **scanfile;
{
	int recordexec ();

	if (t->Tflags&FBACKUP)  fprintf (*scanfile,"B");
	if (t->Tflags&FNOACCT)  fprintf (*scanfile,"N");
	fprintf (*scanfile,"%o %d %d %s\n",
		t->Tmode,t->Tctime,t->Tmtime,t->Tname);
	(void) Tprocess (t->Texec,recordexec,*scanfile);
	return (SCMOK);
}

static
recordexec (t,scanfile)
TREE *t;
FILE **scanfile;
{
	fprintf(*scanfile,"X%s\n",t->Tname);
	return (SCMOK);
}

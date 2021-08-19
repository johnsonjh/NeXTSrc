/*
 *  setpath --- smart interface for setting path variables
 *
 *  usage:	setpath(paths, cmds, localsyspath, dosuffix, printerrors)
 *		char **paths, **cmds, *localsyspath;
 *		int dosuffix, printerrors;
 *
 *  The 'paths' argument is a list of pointers to path lists of the
 *  form "name=value" where name is the name of the path and value
 *  is a colon separated list of directories.  There can never be
 *  more than MAXDIRS (64) directories in a path.
 *
 *  The 'cmds' argument may be a sequence of any of the following:
 *	-i newpath			insert newpath before localsyspath
 *	-ia oldpath newpath		insert newpath after oldpath
 *	-ib oldpath newpath		insert newpath before oldpath
 *	-i# newpath			insert newpath at position #
 *	-d oldpath			delete oldpath
 *	-d#				delete path at position #
 *	-c oldpath newpath		change oldpath to newpath
 *	-c# newpath			change position # to newpath
 *
 *  The "-i newpath" command is equivilent to "-ib 'localsyspath' newpath".
 *
 *  If 'dosuffix' is true, the appropriate suffix will be added to
 *  all command operands for any system path in 'paths'.
 *
 *  Both of the 'paths' and 'cmds' lists are terminated by a NULL
 *  entry.
 *
 *  if 'printerrors' is true, setpath will printf error diagnostics.
 *
 *  WARNING !!!: Under no circumstances should anyone change this
 *  module without fully understanding the impact on the C shell.
 *  The C shell has it's own malloc and printf routines and this
 *  module was carefully written taking that into account.  Do not
 *  use any stdio routines from this module except printf.
 *
 **********************************************************************
 * HISTORY
 * 06-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created from old setpath program for the shell.
 *
 **********************************************************************
 */

#include <libc.h>
#include <sys/param.h>
#include <setjmp.h>

#define MAXDIRS 64		/* max directories on a path */
#define NULL 0
#define isdigit(c)	((c) >= '0' && (c) <= '9')

static int npaths;		/* # pathlist arguments */

static struct pelem {
    struct pelem *pnext;	/* pointer to next path */
    char *pname;		/* name of pathlist */
    char *psuf;			/* suffix for pathlist */
    int pdirs;			/* # directories on each pathlist */
    char *pdir[MAXDIRS];	/* directory names for each pathlist */
} *pathhead = NULL;

static struct {
    char *name;
    char *suffix;
} syspath[] = {
    "PATH",  "/bin",
    "CPATH", "/include",
    "LPATH", "/lib",
    "MPATH", "/man",
    "EPATH", "/maclib",
    0, 0
};

static int sflag;
static int eflag;
static jmp_buf jmpbuf;

#define INVALID { \
	if (eflag) printf("setpath: invalid command '%s'.\n", cmd); \
	freepaths(); \
	return(-1); \
}

#define TOOFEW { \
	if (eflag) printf("setpath: insufficient arguments to command '%s'.\n", cmd); \
	freepaths(); \
	return(-1); \
}

setpath(paths, cmds, localsyspath, dosuffix, printerrors)
register char **paths, **cmds, *localsyspath;
int dosuffix, printerrors;
{
    register char *cmd, *cmd1, *cmd2;
    register int ncmd;

    sflag = dosuffix;
    eflag = printerrors;
    if (initpaths(paths) < 0)
	return(-1);
    if (npaths == 0)
	return(0);
    if (setjmp(jmpbuf) != 0)
	return(-1);
    for (ncmd = 0; cmd = cmds[ncmd]; ncmd++) {
	if (cmd[0] != '-')
	    INVALID;
	cmd1 = cmds[ncmd+1];
	cmd2 = cmds[ncmd+2];
	switch (cmd[1]) {
	case 'i':
	    if (cmd[2] == '\0') {
		ncmd++;
		if (cmd1 == NULL) TOOFEW;
		icmd(cmd1, localsyspath);
	    } else if (isdigit(cmd[2])) {
		ncmd++;
		if (cmd1 == NULL) TOOFEW;
		incmd(cmd1, atoi(cmd+2));
	    } else if (cmd[3] != '\0' || (cmd[2] != 'a' && cmd[2] != 'b')) {
		INVALID;
	    } else {
		ncmd += 2;
		if (cmd1 == NULL || cmd2 == NULL) TOOFEW;
		if (cmd[2] == 'a')
		    iacmd(cmd1, cmd2);
		else
		    ibcmd(cmd1, cmd2);
	    }
	    break;
	case 'd':
	    if (cmd[2] == '\0') {
		ncmd++;
		if (cmd1 == NULL) TOOFEW;
		dcmd(cmd1);
	    } else if (isdigit(cmd[2]))
		dncmd(atoi(cmd+2));
	    else {
		INVALID;
	    }
	    break;
	case 'c':
	    if (cmd[2] == '\0') {
		ncmd += 2;
		if (cmd1 == NULL || cmd2 == NULL) TOOFEW;
		ccmd(cmd1, cmd2);
	    } else if (isdigit(cmd[2])) {
		ncmd++;
		if (cmd1 == NULL) TOOFEW;
		cncmd(cmd1, atoi(cmd+2));
	    } else {
		INVALID;
	    }
	    break;
	default:
	    INVALID;
	}
    }
    if (savepaths(paths) < 0)
	return(-1);
    freepaths();
    return(0);
}

/* static */
initpaths(paths)
register char **paths;
{
    register char *path, *value, *p, *q;
    register int i, done;
    register struct pelem *pe, *pathend;

    freepaths();
    for (npaths = 0; path = paths[npaths]; npaths++) {
	value = index(path, '=');
	if (value == NULL) {
	    if (eflag)
		printf("setpath: value missing in path '%s'\n", path);
	    freepaths();
	    return(-1);
	}
	*value++ = '\0';
	pe = (struct pelem *)malloc((unsigned)(sizeof(struct pelem)));
	if (pe == NULL) {
	    if (eflag)
		printf("setpath: not enough core\n");
	    freepaths();
	    return(-1);
	}
	bzero(pe, sizeof(struct pelem));
	if (pathhead == NULL)
	    pathhead = pathend = pe;
	else {
	    pathend->pnext = pe;
	    pathend = pe;
	}
	p = salloc(path);
	if (p == NULL) {
	    if (eflag)
		printf("setpath: not enough core\n");
	    freepaths();
	    return(-1);
	}
	pe->pname = p;
	pe->psuf = "";
	for (i = 0; syspath[i].name; i++)
	    if (strcmp(pe->pname, syspath[i].name) == 0) {
		pe->psuf = syspath[i].suffix;
		break;
	    }
	q = value;
	for (;;) {
	    q = skipto(p = q, ":");
	    done = (*q == '\0');
	    if (!done)
		*q++ = '\0';
	    p = salloc(p);
	    if (p == NULL) {
		if (eflag)
		    printf("setpath: not enough core\n");
		freepaths();
		return(-1);
	    }
	    pe->pdir[pe->pdirs] = p;
	    pe->pdirs++;
	    if (done)
		break;
	}
    }
    return(0);
}

/* static */
savepaths(paths)
register char **paths;
{
    register char *p, *q;
    register int npath, i, len;
    register struct pelem *pe;

    for (npath = 0, pe = pathhead; pe; npath++, pe = pe->pnext) {
	len = strlen(pe->pname) + 1;
	if (pe->pdirs == 0)
	    len++;
	else for (i = 0; i < pe->pdirs; i++)
	    len += strlen(pe->pdir[i]) + 1;
	p = malloc((unsigned)len);
	if (p == NULL) {
	    if (eflag)
		printf("setpath: not enough core\n");
	    freepaths();
	    return(-1);
	}
	paths[npath] = p;
	for (q = pe->pname; *p = *q; p++, q++);
	*p++ = '=';
	if (pe->pdirs != 0) {
	    for (i = 0; i < pe->pdirs; i++) {
		for (q = pe->pdir[i]; *p = *q; p++, q++);
		*p++ = ':';
	    }
	    p--;
	}
	*p = '\0';
    }
    return(0);
}

/* static */
freepaths()
{
    register char *p;
    register int i;
    register struct pelem *pe;

    if (npaths == 0 || pathhead == NULL)
	return;
    while (pe = pathhead) {
	if (pe->pname) {
	    for (i = 0; i < pe->pdirs; i++) {
		if (pe->pdir[i] == NULL)
		    continue;
		p = pe->pdir[i];
		pe->pdir[i] = NULL;
		free(p);
	    }
	    pe->pdirs = 0;
	    p = pe->pname;
	    pe->pname = NULL;
	    free(p);
	}
	pathhead = pe->pnext;
	free((char *)pe);
    }
    npaths = 0;
}

/***********************************************
 ***    I N S E R T   A   P A T H N A M E    ***
 ***********************************************/

icmd(path, localsyspath)	/* insert path before localsyspath */
char *path, *localsyspath;
{
    register int n;
    register char *new;
    register struct pelem *pe;
    char newbuf[MAXPATHLEN+1];

    for (pe = pathhead; pe; pe = pe->pnext) {
	if (sflag)
	    new = localsyspath;
	else {
	    new = newbuf;
	    strcpy(new, localsyspath);
	    strcat(new, pe->psuf);
	}
	n = locate(pe, new);
	if (n >= 0)
	    insert(pe, n, path);
	else
	    insert(pe, 0, path);
    }
}

iacmd(inpath, path)		/* insert path after inpath */
char *inpath, *path;
{
    register int n;
    register struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	n = locate(pe, inpath);
	if (n >= 0)
	    insert(pe, n + 1, path);
	else
	    printf("setpath: %s not found in %s\n",
		    inpath, pe->pname);
    }
}

ibcmd(inpath, path)		/* insert path before inpath */
char *inpath, *path;
{
    register int n;
    register struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	n = locate(pe, inpath);
	if (n >= 0)
	    insert(pe, n, path);
	else
	    printf("setpath: %s not found in %s\n",
		    inpath, pe->pname);
	}
}

incmd(path, n)			/* insert path at position n */
char *path;
int n;
{
    register struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext)
	insert(pe, n, path);
}

insert(pe, loc, key)
register struct pelem *pe;
register int loc;
register char *key;
{
    register int i;
    register char *new;
    char newbuf[2000];

    if (sflag) {		/* add suffix */
	new = newbuf;
	strcpy(new, key);
	strcat(new, pe->psuf);
    } else
	new = key;
    new = salloc(new);
    if (new == NULL) {
	if (eflag)
	    printf("setpath: not enough core\n");
	freepaths();
	longjmp(jmpbuf, -1);
    }
    for (i = pe->pdirs; i > loc; --i)
	pe->pdir[i] = pe->pdir[i-1];
    if (loc > pe->pdirs)
	loc = pe->pdirs;
    pe->pdir[loc] = new;
    pe->pdirs++;
}

/***********************************************
 ***    D E L E T E   A   P A T H N A M E    ***
 ***********************************************/

dcmd(path)			/* delete path */
char *path;
{
    register int n;
    register struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	n = locate(pe, path);
	if (n >= 0)
	    delete(pe, n);
	else
	    printf("setpath: %s not found in %s\n",
		    path, pe->pname);
    }
}

dncmd(n)			/* delete at position n */
int n;
{
    register struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	if (n < pe->pdirs)
	    delete(pe, n);
	else
	    printf("setpath: %d not valid position in %s\n",
		    n, pe->pname);
    }
}

delete(pe, n)
register struct pelem *pe;
int n;
{
    register int d;

    free(pe->pdir[n]);
    for (d = n; d < pe->pdirs - 1; d++)
	pe->pdir[d] = pe->pdir[d+1];
    --pe->pdirs;
}

/***********************************************
 ***    C H A N G E   A   P A T H N A M E    ***
 ***********************************************/

ccmd(inpath, path)		/* change inpath to path */
char *inpath, *path;
{
    register int n;
    register struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	n = locate(pe, inpath);
	if (n >= 0)
	    change(pe, n, path);
	else
	    printf("setpath: %s not found in %s\n",
		    inpath, pe->pname);
    }
}

int cncmd(path, n)		/* change at position n to path */
char *path;
int n;
{
    register struct pelem *pe;

    for (pe = pathhead; pe; pe = pe->pnext) {
	if (n < pe->pdirs)
	    change(pe, n, path);
	else
	    printf("setpath: %d not valid position in %s\n",
		    n, pe->pname);
    }
}

change(pe, loc, key)
register struct pelem *pe;
register int loc;
register char *key;
{
    register char *new;
    char newbuf[MAXPATHLEN+1];

    if (sflag) {		/* append suffix */
	new = newbuf;
	strcpy(new, key);
	strcat(new, pe->psuf);
    } else
	new = key;
    new = salloc(new);
    if (new == NULL) {
	if (eflag)
	    printf("setpath: not enough core\n");
	freepaths();
	longjmp(jmpbuf, -1);
    }
    free(pe->pdir[loc]);
    pe->pdir[loc] = new;
}

/***************************************
 ***    F I N D   P A T H N A M E    ***
 ***************************************/

int locate(pe, key)
register struct pelem *pe;
register char *key;
{
    register int i;
    register char *realkey;
    char keybuf[MAXPATHLEN+1];

    if (sflag) {
	realkey = keybuf;
	strcpy(realkey, key);
	strcat(realkey, pe->psuf);
    } else
	realkey = key;
    for (i = 0; i < pe->pdirs; i++)
	if (strcmp(pe->pdir[i], realkey) == 0)
	    break;
    return((i < pe->pdirs) ? i : -1);
}

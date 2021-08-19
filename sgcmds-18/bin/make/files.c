/*
 * UNIX DEPENDENT PROCEDURES
 */

#include "defs.h"
#include <errno.h>
#include <fcntl.h>
#ifdef CMUCS
#include <sys/features.h>
#endif

#define DIRHASHSIZE	509

extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];
extern time_t time();

static struct dirhdr *scan_dir();
static umatch();

FSTATIC char VPATH[] = "VPATH";
FSTATIC DIR *cdirf = 0;
FSTATIC struct dirblock cdblock;
FSTATIC struct dirblock *freedb = 0;
FSTATIC struct dirhdr *dirhashtab[DIRHASHSIZE];
FSTATIC dhashed = 0;
FSTATIC nopdir = 0;
FSTATIC maxdir = 0;

#define nextent(d)	(((d) == &cdblock) ? cdnext() : (d)->nxtdirblock)


static int
dhashloc(nam)
	register char *nam;
{
	register char *p;
	register int i;
	
	i = 0;
	for (p = nam; *p; p++)
		i += (*p << 8) | *(p + 1);
	i %= DIRHASHSIZE;

	while (dirhashtab[i] && unequal(nam, dirhashtab[i]->dirn))
		i = (i + 1) % DIRHASHSIZE;

	return i;
}


struct dirblock *
finddir(nam)
	register char *nam;
{
	register struct dirhdr *dh;
	register int loc;
	struct stat sbuf;
	extern struct dirblock *cdnext();
	extern struct dirhdr *scan_dir();

	if (*nam == 0 || (*nam == '.' && (*(nam+1) == 0
	|| (*(nam+1) == '/' && *(nam+2) == 0)))) {
		if (cdirf != 0)
			rewinddir(cdirf);
		else if ((cdirf = opendir(nam)) == 0)
			return 0;
		(void) fcntl(cdirf->dd_fd, F_SETFD, 1);
		return cdnext();
	}

	if (dh = dirhashtab[loc = dhashloc(nam)]) {
		if (stat(nam, &sbuf) == -1)
			return 0;
		if (sbuf.st_mtime != dh->mtime)
			dirhashtab[loc] = scan_dir(dh, nam, &sbuf);
		return dh->nlist;
	}

	if ((dh = scan_dir((struct dirhdr *)0, nam, &sbuf)) == 0)
		return 0;

	if (dhashed++ > DIRHASHSIZE-8)
		fatal("Directory hash table overflow");
	dirhashtab[loc] = dh;
	return dh->nlist;
}


static struct dirhdr *
scan_dir(dh, nam, s)
	register struct dirhdr *dh;
	char *nam;
	struct stat *s;
{
	register struct dirblock *db, *odb;
	register DIR *dirf;
	register struct direct *dptr;

	if (dh == 0) {
		if (stat(nam, s) == -1)
			return 0;
		dh = ALLOC(dirhdr);
		dh->nlist = 0;
		dh->dirn = copys(nam);
		dh->dirfc = 0;
	} else if (db = dh->nlist) {
		while (db->nxtdirblock)
			db = db->nxtdirblock;
		db->nxtdirblock = freedb;
		freedb = dh->nlist;
		dh->nlist = 0;
	}
	dh->mtime = s->st_mtime;

	if (dh->dirfc == 0) {
		errno = 0;
		if ((dirf = opendir(nam)) == 0) {
			if (errno != ENOENT)
				fatal((errno < sys_nerr) ? sys_errlist[errno]
							: "opendir error");
			if (db = dh->nlist) {
				db = dh->nlist;
				while (db->nxtdirblock)
					db = db->nxtdirblock;
				db->nxtdirblock = freedb;
				freedb = dh->nlist;
				dh->nlist = 0;
			}
			return dh;
		}
		if (maxdir == 0) {
			maxdir = getdtablesize() - 8;
			if (maxdir < 12)
				fatal("NOFILE < 20");
		}
		if (nopdir < maxdir) {
			nopdir++;
			dh->nxtopendir = firstod;
			firstod = dh;
			dh->dirfc = dirf;
			(void) fcntl(dirf->dd_fd, F_SETFD, 1);
		}
	} else {
		dirf = dh->dirfc;
		rewinddir(dirf);
	}

	odb = 0;
	while ((dptr = readdir(dirf)) != 0) {
		if (freedb) {
			db = freedb;
			freedb = db->nxtdirblock;
		} else
			db = ALLOC(dirblock);
		db->nxtdirblock = odb;
		db->fname = copys(dptr->d_name);
		odb = db;
	}
	dh->nlist = odb;

	if (dh->dirfc == 0)
		closedir(dirf);
	return dh;
}


struct dirblock *
cdnext()
{
	register struct direct *dptr;

	if ((dptr = readdir(cdirf)) == 0)
		return 0;
	cdblock.fname = dptr->d_name;
	return &cdblock;
}


FSTATIC char vpath[INMAX];  /* XXX */
FSTATIC char machine[INMAX];  /* XXX */

setvpath()
{
	register struct varblock *cp;
	extern struct varblock *srchvar();

	if ((cp = srchvar(VPATH)) == 0)
		*vpath = 0;
	else
		(void) subst(cp->varval, vpath);

	/*
	 * The following should probably be CPUTYPE
	 */
	if (cp = srchvar("MACHINE"))
		(void) subst(cp->varval, machine);
	if (*machine == 0)
		(void) strcpy(machine, "machine");
}


time_t 
exists(pname, ch)
	struct nameblock *pname;
	register struct chain **ch;
{
	register char *p, *filename;
	time_t ptime;
	struct stat buf;
	char fname[MAXPATHLEN+1], temp[MAXPATHLEN+1];
	extern time_t lookarch();
	extern char *canon(), *execat();

	filename = pname->namep;

	if (pname->archp) {
		ptime = lookarch(pname);
#ifdef	NeXT_MOD
		return ptime;
#else	NeXT_MOD
		if (stat(filename, &buf) == -1)
			return ptime;
		return (ptime > buf.st_mtime) ? ptime : buf.st_mtime;
#endif	NeXT_MOD
	}

	if (*filename == '.' && *(filename+1) == '/')
		pname->alias = filename + 2;
	else if (machdep) {
		mksubdir(filename, machine, temp);
		if (stat(temp, &buf) == 0) {
			pname->alias = copys(canon(temp));
			return buf.st_mtime;
		}
		if (coflag && ch && getrcs(temp, pname, &buf, ch))
			return buf.st_mtime;
	}
	if (stat(filename, &buf) == 0)
		return buf.st_mtime;
	if (coflag && ch && getrcs(filename, pname, &buf, ch))
		return buf.st_mtime;

	if (*filename == '/'
	|| (*filename == '.' && *(filename+1) == '/')
	|| *(p = vpath) == 0)
		return 0;

	do {
		p = execat(p, filename, fname);
		if (!unequal(filename, fname))
			continue;
		if (machdep) {
			mksubdir(fname, machine, temp);
			if (stat(temp, &buf) == 0) {
				pname->alias = copys(canon(temp));
				return buf.st_mtime;
			}
			if (coflag && ch && getrcs(temp, pname, &buf, ch))
				return buf.st_mtime;
		}
		if (stat(fname, &buf) == 0) {
			pname->alias = copys(canon(fname));
			return buf.st_mtime;
		}
		if (coflag && ch && getrcs(fname, pname, &buf, ch))
			return buf.st_mtime;
	} while (p);

	return 0;
}


time_t
prestime()
{
	time_t tvec;

	(void) time(&tvec);
	return tvec;
}


int
prevpat(pat)
	register char *pat;
{
	register struct pattern *patp, *lpatp;
	register int n;

	lpatp = 0;
	patp = firstpat;
	while (patp) {
		if ((n = strcmp(pat, patp->patval)) == 0)
			return 1;
		lpatp = patp;
		patp = (n < 0) ? patp->lftpattern : patp->rgtpattern;
	}

	patp = ALLOC(pattern);
	patp->lftpattern = 0;
	patp->rgtpattern = 0;
	patp->patval = copys(pat);

	if (lpatp == 0)
		firstpat = patp;
	else if (n < 0)
		lpatp->lftpattern = patp;
	else
		lpatp->rgtpattern = patp;

	return 0;
}


struct depblock *
srchdir(pat, mkchain, nextdbl)
	char *pat;		/* pattern to be matched in directory */
	int mkchain;		/* nonzero if results to be remembered */
	struct depblock *nextdbl; /* final value for chain */
{
	register char *endir, *dirpref, *filepat, *RCSpref;
	register char *p;
	register struct depblock *db;
	struct depblock *thisdbl;
	char vpref[MAXPATHLEN+1], pth[MAXPATHLEN+1];
	char temp[MAXPATHLEN+1], temp2[MAXPATHLEN+1];
	extern struct depblock *srchpref();
	extern char *execat();

	if (mkchain == NO && prevpat(pat))
		return 0;

	/*
	 * dirpref == directory part of "pat" with trailing '/'
	 * RCSpref == dirpref sans any trailing RCSdir or $(MACHINE)
	 * filepat == file part of "pat"
	 */
	if ((endir = rindex(pat, '/')) == 0) {
		dirpref = RCSpref = "";
		filepat = pat;
	} else {
		*endir = 0;
		dirpref = RCSpref = concat(pat, "/", pth);
		if (coflag && mkchain == NO) {
			if (!unequal(pat, RCSdir))
				RCSpref = "";
			else if (suffix(pat, concat("/", RCSdir, temp), temp))
				(void) ncat(RCSpref = temp, "/", -1);
		}
		if (machdep && mkchain == NO
		&& (p = rindex(RCSpref, '/')) != 0) {
			if (*(p+1) != 0)
				fatal("Cannot happen in srchdir");
			*p = 0;
			if (!unequal(RCSpref, machine))
				RCSpref = "";
			else if (suffix(RCSpref, concat("/", machine, temp2), temp2))
				(void) ncat(RCSpref = temp2, "/", -1);
			*p = '/';
		}
		filepat = endir + 1;
	}

	if (thisdbl = srchpref("", dirpref, RCSpref, filepat, mkchain, nextdbl))
		nextdbl = thisdbl;

	if (*pat == '/'
	|| (*pat == '.' && *(pat+1) == '/')
	|| *(p = vpath) == 0)
		goto out;

	/*
	 * vpref == VPATH component + '/' if non-null
	 */
	do {
		p = execat(p, "", vpref);
		if (*vpref == 0)
			continue;
		if (db = srchpref(vpref, dirpref, RCSpref, filepat, mkchain, nextdbl))
			nextdbl = thisdbl = db;
	} while(p);

out:
	if (endir != 0)
		*endir = '/';

	return thisdbl;
}


struct depblock *
srchpref(vpref, dpref, rcspref, fpat, mkchain, nextdbl)
	char *vpref, *dpref, *rcspref;
	register char *fpat;
	int mkchain;
	struct depblock *nextdbl;
{
	register struct dirblock *d;
	register char *p1, *p2;
	char *dirname;
	struct nameblock *q;
	struct depblock *thisdbl;
	char fullname[MAXPATHLEN+1], temp[MAXPATHLEN+1];
	static char nbuf[MAXNAMLEN + 1];
	static char *nbufend = &nbuf[MAXNAMLEN];

	thisdbl = 0;
	dirname = concat(vpref, dpref, temp);

	for (d = finddir(dirname); d; d = nextent(d)) {
		p1 = d->fname;
		p2 = nbuf;
		while (p2 < nbufend && (*p2++ = *p1++) != 0)
			;
		if (!amatch(nbuf, fpat))
			continue;
		q = makename(concat(dirname, nbuf, fullname));
		if (mkchain) {
			thisdbl = ALLOC(depblock);
			thisdbl->nxtdepblock = nextdbl;
			thisdbl->depname = q;
			nextdbl = thisdbl;
		}
		/*
		 * Strip any VPATH prefix and any "RCS/" suffix
		 * from the directory name, and any ",v" suffix
		 * from the file name so that implicit rules
		 * can find the corresponding files.
		 */
		(void) suffix(nbuf, RCSsuf, nbuf);
		(void) makename(concat(rcspref, nbuf, fullname));
	}

	return thisdbl;
}


/*
 * stolen from glob through find
 */
int
amatch(s, p)
	register char *s, *p;
{
	register int cc, scc, c;
	int k, lc;

again:
	scc = *s;
	lc = 077777;
	switch (c = *p) {
	case '[':
		k = 0;
		while (cc = *++p) {
			switch (cc) {
			case ']':
				if (k == 0)
					return 0;
				++s, ++p;
				goto again;
			case '-':
				k |= (lc <= scc) & (scc <= (cc=p[1]));
			}
			if (scc == (lc = cc))
				k++;
		}
		return 0;

	case '?':
		if (scc == 0)
			return 0;
		++s, ++p;
		goto again;

	case '*':
		return umatch(s, ++p);

	case 0:
		return scc == 0;

	case '\\':
		if (index("[?*\\", *(p+1)))
			c = *++p;

	default:
		if (c != scc)
			return 0;
		++s, ++p;
		goto again;
	}
}


static
umatch(s, p)
	char *s, *p;
{
	if (*p == 0)
		return 1;
	while (*s)
		if (amatch(s++, p))
			return 1;
	return 0;
}


#ifdef METERFILE
#include <pwd.h>
int meteron	= 0;	/* default: metering off */
extern char *ctime();

meter(file)
	char *file;
{
	time_t tvec;
	char *p;
	FILE * mout;
	struct passwd *pwd;

	if (file == 0 || meteron == 0)
		return;

	pwd = getpwuid(getuid());

	(void) time(&tvec);

	if ((mout = fopen(file, "a")) != NULL) {
		p = ctime(&tvec);
		p[16] = '\0';
		fprintf(mout, "User %s, %s\n", pwd->pw_name, p+4);
		(void) fclose(mout);
	}
}
#endif


/*
 * look inside archives for notations a(b) and a((b)):
 *	a(b)	is file member b in archive a
 *	a((b))	is entry point _b in object archive a
 *
 * This stuff needs to be rewritten to handle different archive types
 * dynamically.  It should also know how to use "ranlib" info if present.
 */

#ifndef MULTIMAX
#include <ar.h>
#ifdef ENTRYPT_DEP
/*
 * NOTE: This is commented out because we currently don't support this.
 * Code is also modified in gram.y
 */
#include <a.out.h>
#else
#include <nlist.h>
#endif ENTRYPT_DEP

FSTATIC struct ar_hdr arhead;
FSTATIC long strtab;
FSTATIC long arflen;
FSTATIC char arfname[sizeof(arhead.ar_name)];
FSTATIC FILE *arfd = NULL;
FSTATIC long arpos, arlen;
#ifdef ENTRYPT_DEP
FSTATIC struct exec objhead;
#endif ENTRYPT_DEP
FSTATIC struct nlist objentry;

#ifdef ASCARCH
extern long atol();
#endif

time_t
lookarch(pname)
	register struct nameblock *pname;
{
	long nsym;
	extern char *getonam();

	if (dbgflag)
		printf("lookarch(%s)\n", pname->archp);
	if (openarch(pname->archp) == -1)
		return 0;

	while (getarch()) {
		if (!pname->objarch) {
#ifdef	NeXT_MOD
			if (eqstr(arfname, pname->namep, sizeof(arfname)-1))
#else	NeXT_MOD
			if (eqstr(arfname, pname->namep, sizeof(arfname)))
#endif	NeXT_MOD
#ifdef ASCARCH
				return atol(arhead.ar_date);  /* XXX */
#else
				return arhead.ar_date;
#endif
			continue;
		}
#ifdef ENTRYPT_DEP
		if (getobj() == 0)
			continue;
		for (nsym = objhead.a_syms / sizeof(objentry); nsym; --nsym) {
			(void) fread((char *) &objentry, sizeof(objentry), 1, arfd);
			objentry.n_un.n_name = getonam(objentry.n_un.n_strx);
			if (dbgflag)
				printf("foundsym(%s)\n", objentry.n_un.n_name);
			if ((objentry.n_type & N_EXT)
			&& ((objentry.n_type & ~N_EXT) || objentry.n_value)
			&& !unequal(objentry.n_un.n_name, pname->namep))
#ifdef ASCARCH
				return atol(arhead.ar_date);  /* XXX */
#else
				return arhead.ar_date;
#endif
		}
#endif ENTRYPT_DEP
	}

	return 0;
}


openarch(f)
	register char *f;
{
#ifdef ASCARCH
	char magic[SARMAG];
#else
	int word;
#endif
	struct stat buf;
	static char lastf[MAXPATHLEN+1];
	static char arbuf[BUFSIZ];

	if (arfd == NULL || unequal(f, lastf)) {
		if (arfd != NULL)
			(void) fclose(arfd);
		if (dbgflag)
			printf("openarch(%s)\n", f);
		if ((arfd = fopen(f, "r")) == NULL)
			return -1;
		(void) fcntl(fileno(arfd), F_SETFD, 1);
		setbuf(arfd, arbuf);
		(void) strcpy(lastf, f);
		(void) fstat(fileno(arfd), &buf);
		arlen = buf.st_size;
#ifdef ASCARCH
		(void) fread(magic, SARMAG, 1, arfd);
		arpos = SARMAG;
		if (!eqstr(magic, ARMAG, SARMAG))
#else
		(void) fread((char *) &word, sizeof(word), 1, arfd);
		arpos = sizeof(word);
		if (word != ARMAG)
#endif
			fatal1("%s is not an archive", f);
	} else {
#ifdef ASCARCH
		(void) fseek(arfd, arpos = SARMAG, 0);
#else
		(void) fseek(arfd, arpos = sizeof(word), 0);
#endif
	}
	arflen = 0;
	return 0;
}

#ifdef	NeXT_MOD
purgearch()
{
	if (arfd != NULL) {
		fclose(arfd);
		arfd = NULL;
	}
}
#endif	NeXT_MOD

getarch()
{
	char *p;

	/*
	 * round archived file length up to even
	 */
	arpos += (arflen + 1) & ~1L;
	if (arpos >= arlen)
		return 0;
	(void) fseek(arfd, arpos, 0);
	(void) fread((char *) &arhead, sizeof(arhead), 1, arfd);
	arpos += sizeof(arhead);
#ifdef ASCARCH
	arflen = atol(arhead.ar_size);
#else
	arflen = arhead.ar_size;
#endif
	(void) strncpy(arfname, arhead.ar_name, sizeof(arfname));
	for (p = arfname + sizeof(arfname); *--p == ' '; *p = 0)
		;
	return 1;
}


#ifdef ENTRYPT_DEP
getobj()
{
	long skip;

	(void) fread((char *) &objhead, sizeof(objhead), 1, arfd);
	if (N_BADMAG(objhead)) {
		if (dbgflag)
			printf("skipping(%.*s)\n", sizeof(arfname), arfname);
		return 0;
	}
	skip = objhead.a_text + objhead.a_data;
#if defined(vax) || defined(sun) || defined(ibm032) || defined(ibm370) || defined(balance) || defined(NeXT)
	skip += objhead.a_trsize + objhead.a_drsize;
#else
	if (!objhead.a_flag)
		skip *= 2;
#endif
	(void) fseek(arfd, skip, 1);
	strtab = ftell(arfd) + objhead.a_syms;
	return 1;
}
#endif ENTRYPT_DEP


eqstr(a, b, n)
	register char *a, *b;
	int n;
{
	register int i;

	for (i = 0; i < n && (*a || *b); ++i)
		if (*a++ != *b++)
			return NO;
	return YES;
}


char *
getonam(p)
	long p;
{
	long at;
	register int c;
	register char *s;
	static char space[MAXNAMLEN + 1];

	at = ftell(arfd);
	if (fseek(arfd, strtab + p, 0) == -1) {
		(void) sprintf(space, "%.*s", sizeof(arfname), arfname);
		fatal1("%s is a corrupt object module\n", space);
	}
	s = space;
	while ((c = fgetc(arfd)) != EOF && c != 0 && s < space + MAXNAMLEN)
		*s++ = c;
	*s = 0;
	(void) fseek(arfd, at, 0);
	return space;
}
#else /* MULTIMAX */
/*
 * On MULTIMAX, have to deal with coff format archives -- what a mess!!
 */

#include <pwd.h>
#include <ar.h>

/* 
 * BSD flag seems to refer to use of BSD random library format. (ranlib)
 * UMAX never uses it, so turn it off.
 */

#ifdef  BSD
#undef  BSD
#endif  BSD
#define BSD     0

#if BSD
#include <ranlib.h>
#endif

/*
 * This routine was under the BSD symbol for some reason.  It is
 * needed to access fields in archive file structures.
 */
/*
 * static char ID[] = "@(#) sgetl.c: 1.1 1/8/82";
 *
 * The intent here is to provide a means to make the value of
 * bytes in an io-buffer correspond to the value of a long
 * in the memory while doing the io a `long' at a time.
 * Files written and read in this way are machine-independent.
 */
/*#include <values.h>  This file defines BITSPERBYTE, it's system 5 !@! */
#define BITSPERBYTE     8

long
sgetl(buffer)
register char *buffer;
{
        register long w = 0;
        register int i = BITSPERBYTE * sizeof(long);

        while ((i -= BITSPERBYTE) >= 0)
                w |= (long) ((unsigned char) *buffer++) << i;
        return (w);
}


#define equaln          !strncmp
#define equal           !strcmp

/*
 * For 6.0, create a make which can understand all three archive
 * formats.  This is kind of tricky, and <ar.h> isn't any help.
 * Note that there is no sgetl() and sputl() on the pdp11, so
 * make cannot handle anything but the one format there.
 */
char    archname[64];           /* archive file to be opened */

int     ar_type;        /* to distiguish which archive format we have */
#define ARpdp   1
#define AR5     2
#define ARport  3

long    first_ar_mem;   /* where first archive member header is at */
long    sym_begin;      /* where the symbol lookup starts */
long    num_symbols;    /* the number of symbols available */
long    sym_size;       /* length of symbol directory file */

/*
 * Defines for all the different archive formats.  See next comment
 * block for justification for not using <ar.h>'s versions.
 */
#define ARpdpMAG        0177545         /* old format (short) magic number */

#define AR5MAG          "<ar>"          /* 5.0 format magic string */
#define SAR5MAG         4               /* 5.0 format magic string length */

#define ARportMAG       "!<arch>\n"     /* Port. (6.0) magic string */
#define SARportMAG      8               /* Port. (6.0) magic string length */
#define ARFportMAG      "`\n"           /* Port. (6.0) end of header string */


/*
 * These are the archive file headers for the three formats.  Note
 * that it really doesn't matter if these structures are defined
 * here.  They are correct as of the respective archive format
 * releases.  If the archive format is changed, then since backwards
 * compatability is the desired behavior, a new structure is added
 * to the list.
 */
struct          /* pdp11 -- old archive format */
{
        char    ar_name[14];    /* '\0' terminated */
        long    ar_date;        /* native machine bit representation */
        char    ar_uid;         /*      "       */
        char    ar_gid;         /*      "       */
        int     ar_mode;        /*      "       */
        long    ar_size;        /*      "       */
} ar_pdp;

struct          /* pdp11 a.out header */
{
        short           a_magic;
        unsigned        a_text;
        unsigned        a_data;
        unsigned        a_bss;
        unsigned        a_syms;         /* length of symbol table */
        unsigned        a_entry;
        char            a_unused;
        char            a_hitext;
        char            a_flag;
        char            a_stamp;
} arobj_pdp;

struct          /* pdp11 a.out symbol table entry */
{
        char            n_name[8];      /* null-terminated name */
        int             n_type;
        unsigned        n_value;
} ars_pdp;

struct          /* UNIX 5.0 archive header format: vax family; 3b family */
{
        char    ar_magic[SAR5MAG];      /* AR5MAG */
        char    ar_name[16];            /* ' ' terminated */
        char    ar_date[4];             /* sgetl() accessed */
        char    ar_syms[4];             /* sgetl() accessed */
} arh_5;

struct          /* UNIX 5.0 archive symbol format: vax family; 3b family */
{
        char    sym_name[8];    /* ' ' terminated */
        char    sym_ptr[4];     /* sgetl() accessed */
} ars_5;

struct          /* UNIX 5.0 archive member format: vax family; 3b family */
{
        char    arf_name[16];   /* ' ' terminated */
        char    arf_date[4];    /* sgetl() accessed */
        char    arf_uid[4];     /*      "       */
        char    arf_gid[4];     /*      "       */
        char    arf_mode[4];    /*      "       */
        char    arf_size[4];    /*      "       */
} arf_5;

struct          /* Portable (6.0) archive format: vax family; 3b family */
{
        char    ar_name[16];    /* '/' terminated */
        char    ar_date[12];    /* left-adjusted; decimal ascii; blank filled */
        char    ar_uid[6];      /*      "       */
        char    ar_gid[6];      /*      "       */
        char    ar_mode[8];     /* left-adjusted; octal ascii; blank filled */
        char    ar_size[10];    /* left-adjusted; decimal ascii; blank filled */
        char    ar_fmag[2];     /* special end-of-header string (ARFportMAG) */
} ar_port;


time_t          afilescan();
time_t          entryscan();
time_t          pdpentrys();
FILE            *arfd;
char            BADAR[] = "BAD ARCHIVE";

#define ARNLEN  15              /* max number of characters in name */


time_t
lookarch(pname)
	register struct nameblock *pname;
{
        time_t date = 0L;

	(void) strcpy(archname, pname->archp); /* for error messages */

        if(openarch(archname) == -1)
                return(0L);
        if(pname->objarch)
                date = entryscan(pname->namep);   /* fatals if not found */
        else
                date = afilescan(pname->namep);
        clarch();
        return(date);
}


time_t
afilescan(name)         /* return date for named archive member file */
char *name;
{
        int     len = strlen(name);
        long    ptr;

        if (fseek(arfd, first_ar_mem, 0) != 0)
        {
        seek_error:;
                fatal1("Seek error on archive file %s", archname);
        }
        /*
        * Hunt for the named file in each different type of
        * archive format.
        */
        switch (ar_type)
        {
        case ARpdp:
                for (;;)
                {
                        if (fread((char *)&ar_pdp,
                                sizeof(ar_pdp), 1, arfd) != 1)
                        {
                                if (feof(arfd))
                                        return (0L);
                                break;
                        }
                        if (equaln(ar_pdp.ar_name, name, ARNLEN))       /* DAG -- bug fix (all must match) */
                                return (ar_pdp.ar_date);
                        ptr = ar_pdp.ar_size;
                        ptr += (ptr & 01);
                        if (fseek(arfd, ptr, 1) != 0)
                                goto seek_error;
                }
                break;
        case AR5:
                for (;;)
                {
                register char   *cp;

                        if (fread((char *)&arf_5, sizeof(arf_5), 1, arfd) != 1)
                        {
                                if (feof(arfd))
                                        return (0L);
                                break;
                        }
                        cp = &arf_5.arf_name[ARNLEN];   /* DAG -- bug fix (was fhead.ar_name); pad with NULs */
                        *cp = '\0';
                        while (cp > arf_5.arf_name && *--cp == ' ')     /* DAG -- bug fix (was fhead.arname) */
                                *cp = '\0';
                        if (equal(arf_5.arf_name, name))        /* DAG -- bug fix (all must match ) */
                                return (sgetl(arf_5.arf_date));
                        ptr = sgetl(arf_5.arf_size);
                        ptr += (ptr & 01);
                        if (fseek(arfd, ptr, 1) != 0)
                                goto seek_error;
                }
                break;
        case ARport:
                for (;;)
                {
                register char   *cp;

                        if (fread((char *)&ar_port, sizeof(ar_port), 1,
                                arfd) != 1 || !equaln(ar_port.ar_fmag,
                                ARFportMAG, sizeof(ar_port.ar_fmag)))
                        {
                                if (feof(arfd))
                                        return (0L);
                                break;
                        }
                        cp = &ar_port.ar_name[ARNLEN];  /* DAG -- bug fix (was fhead); pad with NULs */
                        *cp = '\0';
                        while (cp > ar_port.ar_name && (*--cp == ' ' || *cp == '/'))    /* DAG -- bug fix (was fhead) */
                                *cp = '\0';
                        if (equal(ar_port.ar_name, name))       /* DAG -- bug fix (all must match) */
                        {
                                long date;

                                if (sscanf(ar_port.ar_date, "%ld", &date) != 1)
                                {
                                        fatal1("Bad date field for %.14s in %s",
                                                name, archname);
                                }
                                return (date);
                        }
                        if (sscanf(ar_port.ar_size, "%ld", &ptr) != 1)
                        {
                                fatal1("Bad size field for %.14s in archive %s",
                                        name, archname);
                        }
                        ptr += (ptr & 01);
                        if (fseek(arfd, ptr, 1) != 0)
                                goto seek_error;
                }
                break;
        }
        /*
         * Only here if fread() [or equaln()] failed and not at EOF
         */
        fatal1("Read error on archive %s", archname);
        /*NOTREACHED*/
}


time_t
entryscan(name)         /* return date of member containing global var named */
char *name;
{
        /*
        * Hunt through each different archive format for the named
        * symbol.  Note that the old archive format does not support
        * this convention since there is no symbol directory to
        * scan through for all defined global variables.
        */
        if (ar_type == ARpdp)
                return (pdpentrys(name));
        if (sym_begin == 0L || num_symbols == 0L)
        {
        no_such_sym:;
                fatal1("Cannot find symbol %s in archive %s", name, archname);
        }
        if (fseek(arfd, sym_begin, 0) != 0)
        {
        seek_error:;
                fatal1("Seek error on archive file %s", archname);
        }
        if (ar_type == AR5)
        {
                register int i;
                int len = strlen(name) + 1;     /* DAG -- bug fix (added 1) */

                if (len > 8)
                        len = 8;
                for (i = 0; i < num_symbols; i++)
                {
                        if (fread((char *)&ars_5, sizeof(ars_5), 1, arfd) != 1)
                        {
                        read_error:;
                                fatal1("Read error on archive %s", archname);
                        }
                        if (equaln(ars_5.sym_name, name, len))
                        {
                                if (fseek(arfd, sgetl(ars_5.sym_ptr), 0) != 0)
                                        goto seek_error;
                                if (fread((char *)&arf_5,
                                        sizeof(arf_5), 1, arfd) != 1)
                                {
                                        goto read_error;
                                }
                                return (sgetl(arf_5.arf_date));
                        }
                }
        }
        else    /* ar_type == ARport */
        {
                extern char *malloc();
                int strtablen;
#if BSD
                register struct ranlib *offs;
#else
                register char *offs;    /* offsets table */
#endif
                register char *syms;    /* string table */
#if BSD
                register struct ranlib *offend; /* end of offsets table */
#else
                register char *strend;  /* end of string table */
#endif
                char *strbeg;

                /*
                * Format of the symbol directory for this format is
#if BSD
                * as follows:   [sputl()d number_of_symbols * sizeof(struct ranlib)]
#else
                * as follows:   [sputl()d number_of_symbols]
#endif
                *               [sputl()d first_symbol_offset]
#if BSD
                *               [sputl()d first_string_table_offset]
#endif
                *                       ...
                *               [sputl()d number_of_symbols'_offset]
#if BSD
                *               [sputl()d last_string_table_offset]
#endif
                *               [null_terminated_string_table_of_symbols]
                */
#if BSD
                /* have already read the num_symbols info */
                if ((offs = (struct ranlib *)malloc(num_symbols * sizeof(struct ranlib))) == 0)
#else
                if ((offs = malloc(num_symbols * sizeof(long))) == 0)
#endif
                {
                        fatal1("Cannot alloc offset table for archive %s",
                                archname);
                }
#if BSD
                if (fread(offs, sizeof(struct ranlib), num_symbols, arfd)
                                                                != num_symbols)
#else
                if (fread(offs, sizeof(long), num_symbols, arfd) != num_symbols)
#endif
                        goto read_error;
#if BSD
                strtablen = sym_size -
                        (sizeof(long) + num_symbols * sizeof(struct ranlib));
#else
                strtablen = sym_size - ((num_symbols + 1) * sizeof(long));
#endif
                if ((syms = malloc(strtablen)) == 0)
                {
                        fatal1("Cannot alloc string table for archive %s",
                                archname);
                }
                if (fread(syms, sizeof(char), strtablen, arfd) != strtablen)
                        goto read_error;
#if BSD
                offend = &offs[num_symbols];
#else
                strend = &syms[strtablen];
#endif
                strbeg = syms;
#if BSD
                while (offs < offend )
#else
                while (syms < strend)
#endif
                {
#if BSD
                        if (strcmp(&syms[offs->ran_un.ran_strx], name) == 0)
#else
                        if (equal(syms, name))
#endif
                        {
                                long ptr, date;

#if BSD
                                ptr = offs->ran_off;
#else
                                ptr = sgetl(offs);
#endif
                                if (fseek(arfd, ptr, 0) != 0)
                                        goto seek_error;
                                if (fread((char *)&ar_port, sizeof(ar_port), 1,
                                        arfd) != 1 || !equaln(ar_port.ar_fmag,
                                        ARFportMAG, sizeof(ar_port.ar_fmag)))
                                {
                                        goto read_error;
                                }
                                if (sscanf(ar_port.ar_date, "%ld", &date) != 1)
                                {
                                        fatal1("Bad date for %.14s, archive %s",
                                                ar_port.ar_name, archname);
                                }
                                free(strbeg);
                                return (date);
                        }
#if BSD
                        offs += sizeof(struct ranlib);
#else
                        syms += strlen(syms) + 1;
                        offs += sizeof(long);
#endif
                }
                free(strbeg);
        }
        goto no_such_sym;
}


time_t
pdpentrys(name)
        char *name;
{
        long    skip;
        long    last;
        int     len;
        register int i;

        fatal("Cannot do global variable search in pdp11 or old object file.");
        /*NOTREACHED*/
}


openarch(f)
register char *f;
{
        unsigned short  mag_pdp;                /* old archive format */
        char            mag_5[SAR5MAG];         /* 5.0 archive format */
        char            mag_port[SARportMAG];   /* port (6.0) archive format*/

        arfd = fopen(f, "r");
        if (arfd == NULL)
                return(-1);
        /*
         * More code for three archive formats.  Read in just enough to
         * distinguish the three types and set ar_type.  Then if it is
         * one of the newer archive formats, gather more info.
         */
        if (fread((char *)&mag_pdp, sizeof(mag_pdp), 1, arfd) != 1)
                return (-1);
        if (mag_pdp == (unsigned short)ARpdpMAG)
        {
                ar_type = ARpdp;
                first_ar_mem = ftell(arfd);
                sym_begin = num_symbols = sym_size = 0L;
                return (0);
        }
        if (fseek(arfd, 0L, 0) != 0 || fread(mag_5, SAR5MAG, 1, arfd) != 1)
                return (-1);
        if (equaln(mag_5, AR5MAG, SAR5MAG))
        {
                ar_type = AR5;
                /*
                * Must read in header to set necessary info
                */
                if (fseek(arfd, 0L, 0) != 0 ||
                        fread((char *)&arh_5, sizeof(arh_5), 1, arfd) != 1)
                {
                        return (-1);
                }
                sym_begin = ftell(arfd);
                num_symbols = sgetl(arh_5.ar_syms);
                first_ar_mem = sym_begin + sizeof(ars_5) * num_symbols;
                sym_size = 0L;
                return (0);
        }
        if (fseek(arfd, 0L, 0) != 0 ||
                fread(mag_port, SARportMAG, 1, arfd) != 1)
        {
                return (-1);
        }
        if (equaln(mag_port, ARportMAG, SARportMAG))
        {
                ar_type = ARport;
                /*
                * Must read in first member header to find out
                * if there is a symbol directory
                */
                if (fread((char *)&ar_port, sizeof(ar_port), 1, arfd) != 1 ||
                        !equaln(ARFportMAG, ar_port.ar_fmag,
                        sizeof(ar_port.ar_fmag)))
                {
                        return (-1);
                }
#if BSD
                if (equaln(ar_port.ar_name,"__.SYMDEF       ",16))
#else
                if (ar_port.ar_name[0] == '/')
#endif
                {
                        char s[4];

                        if (sscanf(ar_port.ar_size, "%ld", &sym_size) != 1)
                                return (-1);
                        sym_size += (sym_size & 01);    /* round up */
                        if (fread(s, sizeof(s), 1, arfd) != 1)
                                return (-1);
#if BSD
                        num_symbols = sgetl(s) / sizeof(struct ranlib);
#else
                        num_symbols = sgetl(s);
#endif
                        sym_begin = ftell(arfd);
                        first_ar_mem = sym_begin + sym_size - sizeof(s);
                }
                else    /* there is no symbol directory */
                {
                        sym_size = num_symbols = sym_begin = 0L;
                        first_ar_mem = ftell(arfd) - sizeof(ar_port);
                }
                return (0);
        }
        fatal1("%s is not an archive", f);
        /*NOTREACHED*/
}


clarch()
{
        if (arfd != NULL)
                (void) fclose(arfd);
}
#endif /* MULTIMAX */


char *
execat(s1, s2, si)
	register char *s1, *s2;
	char *si;
{
	register char *s;

	s = si;
	while (*s1 && *s1 != ':')
		*s++ = *s1++;
	if (si != s)
		*s++ = '/';
	while (*s2)
		*s++ = *s2++;
	*s = '\0';
	return *s1 ? ++s1 : 0;
}


/*
 * copy s to d, changing file names to file aliases
 */
fixname(s, d)
	char *s, *d;
{
	register char *r, *q;
	struct nameblock *pn;
	char name[MAXPATHLEN+1];

	while (*s) {
		if (isspace(*s) || (funny[*s] & META))
			*d++ = *s++;
		else {
			r = name;
			while (*s) {
				if (isspace(*s) || (funny[*s] & META))
					break; 
				*r++ = *s++;
			}
			*r = '\0';

			if (((pn = srchname(name)) != 0) && (pn->alias))
				q = pn->alias;
			else
				q = name;

			while (*q)
				*d++ = *q++;
		}
	}
	*d = '\0';
}

struct chain *appendq();

/*
 * Try to find an RCS file corresponding to 'fn', and set the modified
 *	time of the RCS file in the stat buffer.
 * If ch is non-null, then append it on the chain for later check-out.
 */
int
getrcs(fn, p, sb, ch)
	char *fn;
	register struct nameblock *p;
	struct stat *sb;
	struct chain **ch;
{
	char temp[MAXPATHLEN+1];
	register char *tail;
	register char *s;
	int headlen;
	extern char *canon();

	if ((tail = rindex(fn, '/')) == 0) {
		headlen = 0;
		tail = fn;
	} else
		headlen = ++tail - fn;
	s = ncat(temp, fn, headlen);
	s = ncat(s, RCSdir, -1);
	*s++ = '/';
	s = ncat(s, tail, -1);
	(void) ncat(s, RCSsuf, -1);
	if (stat(temp, sb) == -1)
		if (stat(concat(fn, RCSsuf, temp), sb) == -1)
			return 0;
	p->RCSnamep = copys(canon(temp));
	if (ch)
		*ch = appendq(*ch, p->namep);
	rcstime(temp, sb);
	return 1;
}


/*
 * Try to find the modification time of a particular version.
 */
rcstime(rcsf, sb)
	char *rcsf;
	struct stat *sb;
{
	register FILE *f;
	register char *p;
	int ign, nopr;
	char cmd[OUTMAX], buf[OUTMAX];
	char iobuf[BUFSIZ];
	long t;
	extern FILE *pfopen();
	extern long atol();

	if (rcstime_cmd == 0)
		return;
	setvar("<", rcsf);
	(void) subst(rcstime_cmd->shbp, buf);
	fixname(buf, cmd);
	ign = ignerr;
	nopr = NO;
	for (p = cmd; *p == '-' || *p == '@'; ++p)
		if (*p == '-')
			ign = YES;
		else
			nopr = YES;
	if (*p == 0)
		return;
	if (!silflag && !nopr)
		printf("%s%s\n", (noexflag ? "" : prompt), p);
	if (noexflag)
		return;
	if ((f = pfopen(p, ign)) == NULL)
		return;
	setbuf(f, iobuf);
	if (fgets(buf, sizeof(buf), f) != 0
	&& (t = atol(buf)) != 0)
		sb->st_mtime = t;
	(void) pfclose(f, ign);
}


/*
 * really check-out a Make description file.
 */
struct nameblock *
rcsco(descfile)
	char *descfile;
{
	struct nameblock *omainname, *p;
	struct chain *ormchain;
	int onoexflag;
	struct chain *ch;

	omainname = mainname;
	p = makename(descfile);
	mainname = omainname;
	ch = 0;
	if (exists(p, &ch) && ch) {
		ormchain = rmchain;
		rmchain = deschain;
		onoexflag = noexflag;
		noexflag = NO;
		co(ch);
		noexflag = onoexflag;;
		deschain = rmchain;
		rmchain = ormchain;
	}
	return p;
}


/*
 * Try to check-out the files specified in ch, if they do not
 *	already exist.  If rmflag is true, mark successful attempts
 *	for automatic deletion.
 * Try to make the modified time of the file the same as that of the
 *	RCS file.
 */
co(ch)
	register struct chain *ch;
{
	register struct nameblock *p;
	register char *file;
	char *RCSfile;
	struct stat sbuf;
	int i;
	long tv[2];

	for (; ch; ch = ch->nextp) {
		if ((file = ch->datap) == 0
		|| (p = srchname(file)) == 0
		|| p->RCSnamep == 0)
			continue;
		if (p->alias)
			file = p->alias;
		if (stat(file, &sbuf) == 0)
			continue;	/* don't do it again */
		RCSfile = p->RCSnamep;
		p->RCSnamep = 0;
		setvar("@", file);
		setvar("<", RCSfile);
		i = docom(co_cmd);
		setvar("@", (char *) 0);
		if (i)
			continue;	/* docom() failed */
		/*
		 * since we succeeded, mark it for later deletion
		 */
		if (rmflag && !isprecious(file))
			rmchain = appendq(rmchain, file);
		/*
		 * try to set modified time on file
		 */
		tv[0] = prestime();
		tv[1] = p->modtime;
		(void) utime(file, tv);
	}
}


/*
 * delete the files listed in rmchain
 */
rm()
{
	struct nameblock *p;
	register struct lineblock *lp;
	register struct shblock *sp;
	int onoexflag;
	static int once = 0;
	char *mkqlist();

	if (once)  /* only if we should */
		return;
	once = 1;
	if ((p = srchname(".CLEANUP")) == 0)
		return;
	sp = 0;
	for (lp = p->linep; lp; lp = lp->nxtlineblock)
		if (sp = lp->shp)
			break;
	if (sp == 0)			/* no or NULL .CLEANUP ? */
		return;
	if (rmchain) {
		setvar("?", mkqlist(rmchain));
		(void) docom(sp);	
	}
	if (deschain) {
		setvar("?", mkqlist(deschain));
		onoexflag = noexflag;
		noexflag = NO;
		(void) docom(sp);	
		noexflag = onoexflag;
	}
}


mksubdir(path, subdir, temp)
	char *path;
	char *subdir;
	char *temp;
{
	register char *tail, *s;
	register int headlen;

	if ((tail = rindex(path, '/')) == 0) {
		s = ncat(temp, subdir, -1);
		*s++ = '/';
		(void) ncat(s, path, -1);
	} else {
		headlen = ++tail - path;
		s = ncat(temp, path, headlen);
		s = ncat(s, subdir, -1);
		*s++ = '/';
		(void) ncat(s, tail, -1);
	} 
}


/*
 * Do the srchdir for RCS files.  For
 * a pattern a/b, it searches a/RCS/b.
 */
srchRCS(pat)
	char *pat;
{
	char temp[MAXPATHLEN+1];

	mksubdir(pat, RCSdir, temp);
	(void) srchdir(temp, NO, (struct depblock *) 0);
}


/*
 * Do the srchdir for machine specific files.  For
 * a pattern a/b, it searches a/$(MACHINE)/b.
 */
srchmachine(pat)
	char *pat;
{
	char temp[MAXPATHLEN+1];

	if (machdep == NO || (*pat == '.' && *(pat+1) == '/'))
		return;
	mksubdir(pat, machine, temp);
	(void) srchdir(temp, NO, (struct depblock *) 0);
	if (coflag)
		srchRCS(temp);
}


extern char *getenv(), *getwd();

FSTATIC char OBJECTS[] = "OBJECTDIR";
FSTATIC char SOURCES[] = "SOURCEDIR";
FSTATIC char MAKECWD[] = "MAKECWD";
FSTATIC char MAKEPSD[] = "MAKEPSD";
#ifdef CMUCS
FSTATIC char CPATH[] = "CPATH";
#else
FSTATIC char CFLAGS[] = "CFLAGS";
#endif
FSTATIC char MAKECONF[] = "MAKECONF";
FSTATIC char *fixedv;
FSTATIC char *newsd, *oldsd;


#ifdef CMUCS
/*
 * all the superroot code is undoubtedly CMU specific as well
 */
#define superroot(d)	(strncmp(d, "/../", 4) == 0)
FSTATIC char superpre[] = "/../.LOCALROOT";
#else
#define superroot(d)	(0)
FSTATIC char superpre[] = "/SUPERROOT";
#endif


makemove(cf)
	char *cf;
{
	char *owd, *psd, cwd[MAXPATHLEN+1];

	if (getwd(cwd) == 0 || *cwd != '/')
		fatal1("makemove: %s", cwd);
	if ((owd = getenv(MAKECWD)) == 0 || (psd = getenv(MAKEPSD)) == 0)
		plainmake(cwd, cf);
	else
		movedmake(cwd, owd, psd);
}


/*
 * find and read the configuration file and move
 * to the object tree fix VPATH if required
 */
plainmake(cwd, cf)
	char *cwd, *cf;
{
	register char *d, *p, *q;
	struct nameblock *pname, *omainname;
	struct chain *ch;
	char path[MAXPATHLEN+1];

	fixedv = 0;
	*vpath = 0;
	omainname = mainname;
	pname = makename(cf);
	mainname = omainname;
	ch = 0;
	if (exists(pname, &ch)) {
		confmove(cwd, pname, (char *) 0, (char *) 0);
		return;
	}
	d = cwd;
	while (*++d)
		;
	while (*(d-1) == '/') {
		if (--d == cwd) {
			confmove(cwd, (struct nameblock *) 0,
					(char *) 0, (char *) 0);
			return;
		}
		*d = 0;
	}
	while (*(d-1) != '/')
		--d;
	q = strcpy(path + MAXPATHLEN - strlen(cf), cf);
	*--q = '/';  p = q;  *--q = '.';  *--q = '.';
	for (;;) {
		*p = '/';
		pname->namep = q;
		if (exists(pname, &ch)) {
			pname->namep = copys(q);
			*p = 0;
			confmove(cwd, pname, q, d);
			return;
		}
		*p = 0;
		while (*(d-1) == '/')
			if (--d == cwd) {
				confmove(cwd, (struct nameblock *) 0,
						(char *) 0, (char *) 0);
				return;
			}
		while (*(d-1) != '/')
			--d;
		if (q < path + 3)
			fatal("plainmake: path too long");
		*--q = '/';  *--q = '.';  *--q = '.';
	}
}


/*
 * we may have changed directories, so we fix VPATH accordingly
 */
movedmake(cwd, owd, psd)
	char *cwd, *owd, *psd;
{
	register char *p, *q, *r, *s;
	struct nameblock *pname, *omainname;
	struct chain *ch;
	char *cf, dir[MAXPATHLEN+1];
	extern char *rdircat(), *fixpath(), *canon();

	if (*owd != '/' || *psd == 0)
		fatal("bad environment (recurmake)");
	if ((cf = getenv(MAKECONF)) != 0) {
		*vpath = 0;
		omainname = mainname;
		pname = makename(cf);
		mainname = omainname;
		ch = 0;
		if (exists(pname, &ch))
			readconf((char *) 0, pname);
	}
	if (!superroot(owd) && superroot(cwd)) {
		for (p = owd; *p; p++)
			;
		do
			*(p + sizeof(superpre) - 1) = *p;
		while (p-- > owd);
		for (p = owd, q = superpre; *q; *p++ = *q++)
			;
	}
	for (p = r = owd, q = s = cwd; *r && *r == *s; r++, s++)
		if (*r == '/') {
			while (*(r+1) == '/')
				r++;
			while (*(s+1) == '/')
				s++;
			p = r + 1;
			q = s + 1;
		}
	if (*r == 0 && *s == 0) {
		p = r;
		q = s;
	} else if (*r == '/' && *s == 0) {
		p = r + 1;
		q = s;
	} else if (*r == 0 && *s == '/') {
		p = r;
		q = s + 1;
	}
	r = dir;
	s = psd;
	if (*s != '/') {
		r = rdircat(q, p, r);
		if (r != dir && *(r-1) != '/')
			*r++ = '/';
	}
	do
		*r++ = *s++;
	while (*s);
	if (*(r-1) != '/' && (*p || *q))
		*r++ = '/';
	r = rdircat(p, q, r);
	newsd = copys(canon(dir));
	oldsd = copys(canon(psd));
	setenv(MAKECWD, cwd, 1);
	setenv(MAKEPSD, newsd, 1);
	fixedv = fixpath(VPATH, newsd, oldsd);
}


/*
 * if this is a "moved make" we previously fixed VPATH, but the
 * user may have reset VPATH in the description file, so we fix
 * it again if necessary, and arrange for include files to be
 * found too
 */
postmove()
{
	register struct varblock *sp;
	extern struct varblock *srchvar();
	extern char *fixpath();

	if (fixedv == 0)
		return;
	if ((sp = srchvar(VPATH)) == 0)
		fatal("cannot happen (postmove)");
	if (sp->varval != fixedv)
		(void) fixpath(VPATH, newsd, oldsd);
#ifdef CMUCS
	(void) fixpath(CPATH, newsd, oldsd);
#else
	addincdir(newsd);
#endif
}


confmove(cwd, p, pre, suf)
	char *cwd;
	struct nameblock *p;
	char *pre, *suf;
{
	register char *objb;
	char obj[MAXPATHLEN+1], src[MAXPATHLEN+1];
	extern char *fixvar();

	if (p)
		readconf(cwd, p);
	objb = fixvar(OBJECTS, obj, pre, suf);
	(void) fixvar(SOURCES, src, pre, suf);
	if (*obj || *src)
		reldir(cwd, obj, src, objb);
}


readconf(cwd, p)
	char *cwd;
	struct nameblock *p;
{
	register FILE *f;
	register char *cf;
	char buf[INMAX];  /* XXX */
	static char cmd[] = "$(CO) -q -p $<";
	extern char *canon();
	extern FILE *pfopen();

	cf = p->alias ? p->alias : p->namep;
	if (dbgflag)
		printf("using %s\n", cf);
	if (cwd) {
		if (*cf != '/')
			(void) strcat(strcat(strcpy(buf, cwd), "/"), cf);
		else
			(void) strcpy(buf, cf);
		setenv(MAKECONF, canon(buf), 1);
	}
	if (p->RCSnamep) {
		setvar("<", p->RCSnamep);
		(void) subst(cmd, buf);
		if (!silflag)
			printf("%s%s\n", (noexflag ? "" : prompt), buf);
		if ((f = pfopen(buf, 0)) == NULL) {
			perror(buf);
			return;
		}
		curfname = p->namep;
		rdd1(f);
		(void) pfclose(f, 0);
	} else {
		if ((f = fopen(cf, "r")) == NULL) {
			perror(cf);
			return;
		}
		curfname = cf;
		rdd1(f);
		(void) fclose(f);
	}
}


char *
fixvar(name, buf, pre, suf)
	char *name, *buf, *pre, *suf;
{
	register struct varblock *vp;
	register char *p, *q, *b;
	char value[INMAX];  /* XXX */
	extern struct varblock *srchvar();
	extern char *canon();

	*(p = buf) = 0;
	if ((vp = srchvar(name)) == 0)
		return p;
	(void) subst(vp->varval, value);
	if (dbgflag)
		printf("%s=%s\n", name, value);
	if ((q = pre) && *value != '/') {
		while (*p = *q++)
			p++;
		*p++ = '/';
	}
	for (q = value; *p = *q++; p++)
		;
	b = p;
	if (q = suf) {
		*p++ = '/';
		while (*p = *q++)
			p++;
	}
	(void) canon(buf);
	if (dbgflag)
		printf("%s=%s\n", name, buf);
	return b;
}


/*
 * find a name for the source directory after a
 * chdir(obj) and use it to adjust search paths
 */
reldir(cwd, obj, src, objb)
	char *cwd, *obj, *src, *objb;
{
	register char *h, *t;
	register char *q, *r;
	char dir[MAXPATHLEN+1];

	if (*src == '/') {
		makechdir(obj, src, objb);
		return;
	}
	r = cwd;
	while (*++r)
		;
	while (--r != cwd && *r == '/')
		;
	for (h = src; *h; h++)
		;
	q = dir + MAXPATHLEN;
	for (*q = 0; h != src; *--q = *--h)
		;
	if (*obj == '/' || superroot(cwd)) {
		if (*q)
			*--q = '/';
		for (r++; r != cwd; *--q = *--r)
			;
		makechdir(obj, q, objb);
		return;
	}
	for (h = t = obj; *h; h = t) {
		while (*t && *t != '/')
			t++;
		if (t != h + 1 || *h != '.') {
			if (*q)
				*--q = '/';
			if (t == h + 2 && *h == '.' && *(h + 1) == '.') {
				if (r == cwd) {
					if (*q != '/')
						*--q = '/';
					break;
				}
				while (*r != '/')
					*--q = *r--;
				while (r != cwd && *r == '/')
					--r;
			} else {
				for (*++r = '/'; h != t; *++r = *h++)
					;
				*--q = '.';
				*--q = '.';
			}
		}
		while (*t == '/')
			t++;
	}
	if (*q)
		makechdir(obj, q, objb);
}


makechdir(obj, dir, objb)
	char *obj;
	char *dir;
	char *objb;
{
	char cwd[MAXPATHLEN+1];
	extern char *fixpath(), *canon();

	if (*obj) {
		makepath(obj, dir, objb);
		if (!silflag)
			printf("%scd %s\n", (noexflag ? "" : prompt), obj);
		if (chdir(obj) == -1) {
			(void) sprintf(cwd, "%%s: %s", (errno < sys_nerr) ?
					sys_errlist[errno] : "chdir error");
			fatal1(cwd, obj);
		}
	}
	if (getwd(cwd) == 0 || *cwd != '/')
		fatal1("makechdir: %s", cwd);
	newsd = copys(canon(dir));
	oldsd = 0;
	setenv(MAKECWD, cwd, 1);
	setenv(MAKEPSD, newsd, 1);
	fixedv = fixpath(VPATH, newsd, oldsd);
}


makepath(dir, src, dirb)
	char *dir, *src, *dirb;
{
	register char *p, *q, *r, *s;
	register char c, c1;
	char buf[MAXPATHLEN+1];
	struct stat sbuf;
	extern char *canon();

	p = buf;
	if (*src != '/') {
		for (q = dir; *q; *p++ = *q++)
			;
		*p++ = '/';
	}
	for (q = src; *p = *q; p++, q++)
		;
	src = canon(buf);
	for (p = dir; *p; p++)
		;
	for (q = src; *q; q++)
		;
	r = p;
	s = q;
	while (p > dir && q > src && *--p == *--q) {
		if (*p != '/')
			continue;
		r = p + 1;
		s = q + 1;
		while (p > dir && *(p - 1) == '/')
			--p;
		while (q > src && *(q - 1) == '/')
			--q;
	}
	for (q = dir; *q == '/'; q++)
		;
	while (*q) {
		while (*++q && *q != '/')
			;
		c = *q;
		*q = 0;
		if (q > r)
			while (*s && *s != '/')
				s++;
		if (stat(dir, &sbuf) == -1) {
			if (q <= dirb)
				fatal1("No such directory: %s", dir);
			sbuf.st_mode = 0777;
			if (q > r) {
				c1 = *s;
				*s = 0;
				if (stat(src, &sbuf) != -1 && dbgflag)
					printf("using mode of %s\n", src);
				*s = c1;
			}
			if (!silflag)
				printf("%smkdir %s\n", (noexflag ? "" : prompt), dir);
			if (mkdir(dir, (int) sbuf.st_mode) == -1)
				fatal1("Couldn't make directory: %s", dir);
		} else if ((sbuf.st_mode & S_IFMT) != S_IFDIR)
			fatal1("Not a directory: %s", dir);
		if (q > r)
			while (*s == '/')
				s++;
		*q = c;
		while (*q == '/')
			q++;
	}
}


/*
 * c = rev(a) | '/' | b
 */
char *
rdircat(a, b, c)
	char *a, *b;
	register char *c;
{
	register char *p;

	for (p = a; *p; *c++ = '.', *c++ = '.', *c++ = '/') {
		if (*p == '/'
		|| (*p == '.' && (*++p == 0 || *p == '/'
		|| (*p == '.' && (*++p == 0 || *p == '/')))))
			fatal1("bad directory: %s", a);
		while (*p && *p++ != '/')
			;
	}
	if (*a && *b == 0)
		*--c = 0;
	else
		for (p = b; *c = *p++; c++)
			;
	return c;
}


/*
 * add dir to each relative PATH component
 * while removing references to old
 */
char *
fixpath(path, dir, old)
	char *path, *dir, *old;
{
	register struct varblock *cp;
	register char *p, *q, *h, *t, c;
	char *np, *ap, *bp;
	char oldpath[INMAX], newpath[INMAX];  /* XXX */
	extern struct varblock *varptr();
	extern char *canon();

	cp = varptr(path);
	t = oldpath + 1;
	(void) subst(cp->varval, t);
	if (dbgflag)
		printf("%s=%s\n", path, t);
	if (path == VPATH && *t && *t != ':')
		*--t = ':';
	np = p = newpath;
	*p++ = ':';
	for (q = dir; *q; *p++ = *q++)
		;
	ap = p + 1;
	for (h = t;; h = t) {
		while (*t && *t != ':')
			t++;
		c = *t;
		*t = 0;
		if (old == 0 || strcmp(h, old)) {
			*p++ = ':';
			bp = 0;
			if (*h != '/') {
				for (q = h; q != t; *p++ = *q++)
					;
				*p++ = ':';
				bp = p;
				for (q = dir; *q; *p++ = *q++)
					;
				if (h != t)
					*p++ = '/';
				else
					bp = 0, np = ap;
			}
			while (h != t)
				*p++ = *h++;
			if (bp)
				for (*p = 0, p = canon(bp); *p; p++)
					;
		}
		*t = c;
		if (*t++ == 0)
			break;
	}
	*p = 0;
	if (path == VPATH)
		np++;
	cp->varval = copys(np);
	if (dbgflag)
		printf("%s=%s\n", path, cp->varval);
	if (getenv(path))
		setenv(path, cp->varval, 1);
	return cp->varval;
}


char *
canon(f)
	char *f;
{
	register char *p, *q, *r;
	register char **sp, **tp;
	char *stack[MAXPATHLEN >> 1];

	if (dbgflag)
		printf("%s -> ", f);
	p = q = f;
	sp = tp = stack + (MAXPATHLEN >> 1);
	if (*q == '/') {
		p++;
		while (*++q == '/')
			;
	}
	while (*(r = q)) {
		while (*++q && *q != '/')
			;
		if (q != r+1 || *r != '.') {
			if (q != r+2 || *r != '.' || *(r+1) != '.')
				*--sp = r;
			else if (sp != tp)
				sp++;
			else
				*p++ = '.', *p++ = '.', *q && (*p++ = '/');
		}
		while (*q == '/')
			q++;
	}
	while (tp > sp)
		for (q = *--tp; *q; )
			if ((*p++ = *q++) == '/')
				break;
	if (p > f+1 && *(p-1) == '/')
		--p;
	*p = 0;
	if (dbgflag)
		printf("%s\n", f);
	return f;
}


#ifndef CMUCS
addincdir(dir)
	char *dir;
{
	register struct varblock *cp;
	char buf[INMAX];  /* XXX */
	extern struct varblock *varptr();

	if ((cp = varptr(CFLAGS))->varval == 0)
		cp->varval = "";
	(void) sprintf(buf, "-I. -I%s%s%s", dir, *cp->varval ? " " : "", cp->varval);
	cp->varval = copys(buf);
}
#endif

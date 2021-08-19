#define	NeXT_NFS
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)ranlib.c	5.3 (Berkeley) 1/22/86";
#endif not lint

/*
 * ranlib - create table of contents for archive; string table version
 */
#include <sys/types.h>
#include <ar.h>
#include <ranlib.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/loader.h>
#include <sys/stat.h>

#define	OARMAG 0177545
struct	ar_hdr	archdr;
long	arsize;
struct	exec	exp;
FILE	*fi, *fo;
long	off, oldoff;
long	atol(), ftell();
#define TABSZ	3000
int	tnum;
#define	STRTABSZ	30000
int	tssiz;
char	*strtab;
int	ssiz;
int	new;
char	*tempnm;
#define TEMP_SUFFIX ".ranlib"
char	firstname[17];
off_t	offdelta;
off_t	startoff;
void	stash();
char *malloc();
char *allocate();

/*
 * table segment definitions
 */
void	segclean();
struct	tabsegment {
	struct tabsegment	*pnext;
	unsigned		nelem;
	struct ranlib		tab[TABSZ];
} tabbase, *ptabseg;
struct	strsegment {
	struct strsegment	*pnext;
	unsigned		nelem;
	char			stab[STRTABSZ];
} strbase, *pstrseg;

static int ranlib_qsort(const struct ranlib *ran1, const struct ranlib *ran2);
static long round(long v, unsigned long r);
static int check_sort_ranlibs(struct ranlib *sort_ranlibs, int nranlibs,
			      char *libname);

main(argc, argv)
char **argv;
{
	char cmdbuf[BUFSIZ];
	/* magbuf must be an int array so it is aligned on an int-ish
	   boundary, so that we may access its first word as an int! */
	int magbuf[(SARMAG+sizeof(int))/sizeof(int)];
	register int just_touch = 0;
	register struct tabsegment *ptab;
	register struct strsegment *pstr;
	int sort = 1, good_sort, commons = 0;
	struct ranlib *sort_ranlibs;
	int nsort_ranlibs;
	char arbuf[sizeof(struct ar_hdr)+1];
	struct stat stbuf;
	int tm;
	char *buf;
	int bufsize;

	if(argc == 1){
		fprintf(stderr, "Usage: ranlib archive\n");
		exit(1);
	}

	/* check for the "-t" flag" */
	--argc;
	argv++;
	while(*argv[0] == '-'){
		if (strcmp(*argv, "-t") == 0)
			just_touch++;
		else if (strcmp(*argv, "-s") == 0)
			sort = 1;
		else if (strcmp(*argv, "-a") == 0)
			sort = 0;
		else if (strcmp(*argv, "-c") == 0)
			commons = 1;
		else{
			fprintf(stderr, "ranlib: unknown flag %s\n", *argv);
			exit(1);
		}
		argc--;
		argv++;
	}

	while(argc--) {
		fi = fopen(*argv,"r");
		if (fi == NULL) {
			fprintf(stderr, "ranlib: cannot open %s\n", *argv);
			argv++;
			continue;
		}
		off = SARMAG;
		fread((char *)magbuf, 1, SARMAG, fi);
		if (strncmp((char *)magbuf, ARMAG, SARMAG)) {
			if (magbuf[0] == OARMAG)
				fprintf(stderr, "old format ");
			else
				fprintf(stderr, "not an ");
			fprintf(stderr, "archive: %s\n", *argv);
			fclose(fi);
			argv++;
			continue;
		}
		if (just_touch) {
			register int	len;

			fseek(fi, (long) SARMAG, 0);
			if (fread(cmdbuf, sizeof archdr.ar_name, 1, fi) != 1) {
				fprintf(stderr, "malformed archive: %s\n",
					*argv);
				fclose(fi);
				argv++;
				continue;
			}
			if (bcmp(cmdbuf, SYMDEF, sizeof(SYMDEF) - 1) != 0){
				fprintf(stderr, "no symbol table: %s\n", *argv);
				fclose(fi);
				argv++;
				continue;
			}
			fclose(fi);
			fixdate(*argv);
			fclose(fi);
			argv++;
			continue;
		}
		fseek(fi, 0L, 0);
		new = tssiz = tnum = 0;
		segclean();
		if (nextel(fi) == 0) {
			fclose(fi);
			argv++;
			continue;
		}
		do {
			long o;
			register n;
			struct nlist sym;

			fread((char *)&exp, 1, sizeof(struct exec), fi);
			if (!N_BADMAG(exp)){
				if (!strncmp(SYMDEF, archdr.ar_name,
				    sizeof(SYMDEF) - 1))
					continue;
				if (exp.a_syms == 0) {
					fprintf(stderr, "ranlib: warning: %s(%s): no symbol table\n", *argv, archdr.ar_name);
					continue;
				}
				o = N_STROFF(exp) - sizeof (struct exec);
				if (ftell(fi)+o+sizeof(ssiz) >= off) {
					fprintf(stderr, "ranlib: warning: %s(%s): old format .o file\n", *argv, archdr.ar_name);
					continue;
				}
				fseek(fi, o, 1);
				fread((char *)&ssiz, 1, sizeof (ssiz), fi);
				if (ssiz < sizeof ssiz){
					/* sanity check */
					fprintf(stderr, "ranlib: warning: %s(%s): mangled string table\n", *argv, archdr.ar_name);
					continue;
				}
				strtab = (char *)allocate(ssiz);
				fread(strtab+sizeof(ssiz), ssiz - sizeof(ssiz),
				      1, fi);
				fseek(fi, -(exp.a_syms+ssiz), 1);
				n = exp.a_syms / sizeof(struct nlist);
			}
			else if (*((long *)&exp) == MH_MAGIC){
				long i, start_off;
				struct mach_header mh;
				struct load_command *load_commands, *lcp;
				struct symtab_command *stp;

				fseek(fi, -(sizeof(struct exec)), 1);
				start_off = ftell(fi);

				fread((char *)&mh, 1,
				      sizeof(struct mach_header), fi);
				load_commands = (struct load_command *)
						allocate(mh.sizeofcmds);
				fread((char *)load_commands, 1, mh.sizeofcmds,
				      fi);
				stp = NULL;
				lcp = load_commands;
				for (i = 0; i < mh.ncmds; i++){
				    if(stp == NULL && lcp->cmd == LC_SYMTAB)
					stp = (struct symtab_command *)lcp;
				    lcp = (struct load_command *)
						((char *)lcp + lcp->cmdsize);
				}
				if (stp == NULL || stp->nsyms == 0){
				    fprintf(stderr, "ranlib: warning: %s(%s): "
					    "no symbol table\n", *argv,
					    archdr.ar_name);
				    continue;
				}
				if (stp->symoff + stp->nsyms *
				    sizeof(struct nlist) > off - start_off){
				    fprintf(stderr, "ranlib: warning: %s(%s): "
					    "bad symbol table offset or size\n",
					    *argv, archdr.ar_name);
				    continue;
				}
				if (stp->strsize != 0 && stp->stroff +
				    stp->strsize > off - start_off){
				    fprintf(stderr, "ranlib: warning: %s(%s): "
					    "bad string table offset or size\n",
					    *argv, archdr.ar_name);
				    continue;
				}

				fseek(fi, start_off + stp->stroff, 0);
				strtab = (char *)allocate(stp->strsize);
				fread(strtab, stp->strsize, 1, fi);

				fseek(fi, start_off + stp->symoff, 0);
				n = stp->nsyms;
				free(load_commands);
			} else
				continue;
			while (--n >= 0) {
				fread((char *)&sym, 1, sizeof(sym), fi);
				if (sym.n_un.n_strx == 0)
					continue;
				sym.n_un.n_name = strtab + sym.n_un.n_strx;
				if ((sym.n_type&N_EXT)==0)
					continue;
				switch (sym.n_type&N_TYPE) {

				case N_UNDF:
					if (sym.n_value != 0 && commons)
						stash(&sym);
					continue;

				default:
					stash(&sym);
					continue;
				}
			}
			free(strtab);
		} while(nextel(fi));

		new = fixsize();
		if(sort) {
		    sort_ranlibs = (struct ranlib *)
				   allocate(tnum * sizeof (struct ranlib));
		    ptab = &tabbase;
		    nsort_ranlibs = 0;
		    do {
			    memcpy(sort_ranlibs + nsort_ranlibs,
				   ptab->tab,
				   ptab->nelem * sizeof(struct ranlib));
			    nsort_ranlibs += ptab->nelem;
		    } while (ptab = ptab->pnext);
		    qsort(sort_ranlibs, nsort_ranlibs, sizeof(struct ranlib),
			  (int (*)(const void *, const void *))ranlib_qsort);
		    good_sort = check_sort_ranlibs(sort_ranlibs, nsort_ranlibs,
						   *argv);
		    if(!good_sort)
			    free(sort_ranlibs);
		}

		tempnm = allocate(strlen(*argv) + sizeof(TEMP_SUFFIX));
		strcpy(tempnm, *argv);
		strcat(tempnm, TEMP_SUFFIX);
		fo = fopen(tempnm, "w");
		if(fo == NULL) {
			fprintf(stderr, "can't create temporary\n");
			exit(1);
		}

		fstat(fileno(fo), &stbuf);
		tm = time((long *)NULL);

		fwrite(ARMAG, SARMAG, sizeof(char), fo);
		sprintf(arbuf, "%-*s%-*ld%-*u%-*u%-*o%-*ld%-*s",
		   sizeof(archdr.ar_name),
		       (sort && good_sort) ? SYMDEF_SORTED : SYMDEF,
		   sizeof(archdr.ar_date),
		       stbuf.st_mtime,
		   sizeof(archdr.ar_uid),
		       (u_short)stbuf.st_uid,
		   sizeof(archdr.ar_gid),
		       (u_short)stbuf.st_gid,
		   sizeof(archdr.ar_mode),
		       stbuf.st_mode,
		   sizeof(archdr.ar_size),
		       sizeof(tnum) + tnum * sizeof(struct ranlib) +
		       sizeof(tssiz) + tssiz,
		   sizeof(archdr.ar_fmag),
		       ARFMAG);
		fwrite(arbuf, sizeof(struct ar_hdr), sizeof(char), fo);

		tnum *= sizeof (struct ranlib);
		fwrite(&tnum, 1, sizeof (tnum), fo);
		tnum /= sizeof (struct ranlib);
		if(sort == 0 || good_sort == 0){
			ptab = &tabbase;
			do {
				fwrite((char *)ptab->tab, ptab->nelem,
				    sizeof(struct ranlib), fo);
			} while (ptab = ptab->pnext);
		} else {
			fwrite((char *)sort_ranlibs, nsort_ranlibs,
			       sizeof(struct ranlib), fo);
			free(sort_ranlibs);
		}
		fwrite(&tssiz, 1, sizeof (tssiz), fo);
		pstr = &strbase;
		do {
			fwrite(pstr->stab, pstr->nelem, 1, fo);
			tssiz -= pstr->nelem;
		} while (pstr = pstr->pnext);
		/* pad with nulls */
		while (tssiz--) putc('\0', fo);

		fstat(fileno(fi), &stbuf);
		bufsize = stbuf.st_size - startoff;
		buf = allocate(bufsize);
		fseek(fi, startoff, 0);
		fread(buf, bufsize, sizeof(char), fi);
		fwrite(buf, bufsize, sizeof(char), fo);
		free(buf);

		fclose(fi);
		fclose(fo);

		unlink(*argv);
		if(rename(tempnm, *argv) == -1){
			fprintf(stderr, "ranlib: can't rename temporary"
				" file from: %s to: %s\n", tempnm, *argv);
			exit(1);
		}
		fixdate(*argv);
		free(tempnm);
		argv++;
	}
	exit(0);
}

/*
 * Function for qsort() for comparing ranlib structures by name.
 */
static
int
ranlib_qsort(
const struct ranlib *ran1,
const struct ranlib *ran2)
{
	int i;
	register struct strsegment *pstr;
	char *name1, *name2;

	i = 0;
	pstr = &strbase;
	while(ran1->ran_un.ran_strx >= i + pstr->nelem){
		i += pstr->nelem;
		pstr = pstr->pnext;
	}
	name1 = pstr->stab + (ran1->ran_un.ran_strx - i);

	i = 0;
	pstr = &strbase;
	while(ran2->ran_un.ran_strx >= i + pstr->nelem){
		i += pstr->nelem;
		pstr = pstr->pnext;
	}
	name2 = pstr->stab + (ran2->ran_un.ran_strx - i);

	return(strcmp(name1, name2));
}

static
int
check_sort_ranlibs(
struct ranlib *sort_ranlibs,
int nranlibs,
char *libname)
{
	int i, j, r, multiple_defs, val;
	struct ar_hdr archdr;
	register struct strsegment *pstr;
	char *name1, *name2;

	multiple_defs = 0;
	for(i = 0; i < nranlibs - 1; i++){
		j = 0;
		pstr = &strbase;
		while(sort_ranlibs[i].ran_un.ran_strx >= j + pstr->nelem){
			j += pstr->nelem;
			pstr = pstr->pnext;
		}
		name1 = pstr->stab + (sort_ranlibs[i].ran_un.ran_strx - j);

		j = 0;
		pstr = &strbase;
		while(sort_ranlibs[i + 1].ran_un.ran_strx >= j + pstr->nelem){
			j += pstr->nelem;
			pstr = pstr->pnext;
		}
		name2 = pstr->stab + (sort_ranlibs[i + 1].ran_un.ran_strx - j);

		if((val = strcmp(name1, name2)) == 0){
			if(multiple_defs == 0){
				fprintf(stderr, "ranlib: same symbol defined "
					"in more than one member in: %s (table "
					"of contents will not be sorted)\n",
					libname);
				multiple_defs = 1;
			}
			if(sort_ranlibs[i].ran_off != -1){
				fseek(fi, sort_ranlibs[i].ran_off - offdelta,0);
				r = fread((char *)&archdr, 1,
					  sizeof(struct ar_hdr), fi);
				if (r == sizeof(struct ar_hdr)){
					fprintf(stderr, "symbol: %s defined "
						"in: %0.16s\n", name1,
						archdr.ar_name);
				}
				sort_ranlibs[i].ran_off = -1;
			}
			if(sort_ranlibs[i + 1].ran_off != -1){
				fseek(fi, sort_ranlibs[i + 1].ran_off-offdelta,
				      0);
				r = fread((char *)&archdr, 1,
					  sizeof(struct ar_hdr), fi);
				if (r == sizeof(struct ar_hdr)){
					fprintf(stderr, "symbol: %s defined "
						"in: %0.16s\n", name2,
						archdr.ar_name);
				}
				sort_ranlibs[i + 1].ran_off = -1;
			}
		}
		else if(val > 0){
		    fprintf(stderr, "qsort() failed, entries %d (%s) and %d "
			    "(%s) not in increasing order\n", i, name1, i + 1,
			    name2);
		}
	}
	return(multiple_defs == 0);
}

nextel(af)
FILE *af;
{
	register r;
	register char *cp;

	oldoff = off;
	fseek(af, off, 0);
	r = fread((char *)&archdr, 1, sizeof(struct ar_hdr), af);
	if (r != sizeof(struct ar_hdr))
		return(0);
	for (cp=archdr.ar_name; cp < & archdr.ar_name[sizeof(archdr.ar_name)]; cp++)
		if (*cp == ' ')
			*cp = '\0';
	arsize = atol(archdr.ar_size);
	if (arsize & 1)
		arsize++;
	off = ftell(af) + arsize;
	return(1);
}

void
stash(s)
	struct nlist *s;
{
	register char *cp;
	register char *strtab;
	register strsiz;
	register struct ranlib *tab;
	register tabsiz;
	register len, i;

	len = strlen(s->n_un.n_name) + 1;
	if (ptabseg->nelem >= TABSZ) {
		/* allocate a new symbol table segment */
		ptabseg = ptabseg->pnext =
		    (struct tabsegment *) allocate(sizeof(struct tabsegment));
		ptabseg->pnext = NULL;
		ptabseg->nelem = 0;
	}
	tabsiz = ptabseg->nelem;
	tab = ptabseg->tab;

	if (pstrseg->nelem + len >= STRTABSZ) {
		while(pstrseg->nelem < STRTABSZ){
		    pstrseg->stab[pstrseg->nelem] = '\0';
		    pstrseg->nelem++;
		    tssiz++;
		}
		/* allocate a new string table segment */
		pstrseg = pstrseg->pnext =
		    (struct strsegment *) allocate(sizeof(struct strsegment));
		pstrseg->pnext = NULL;
		pstrseg->nelem = 0;
	}
	strsiz = pstrseg->nelem;
	strtab = pstrseg->stab;

	tab[tabsiz].ran_un.ran_strx = tssiz;
	tab[tabsiz].ran_off = oldoff;
redo:
	for (cp = s->n_un.n_name; strtab[strsiz++] = *cp++;)
		if (strsiz >= STRTABSZ) {
			/* allocate a new string table segment */
			pstrseg = pstrseg->pnext =
			    (struct strsegment *) allocate(sizeof(struct strsegment));
			pstrseg->pnext = NULL;
			strsiz = pstrseg->nelem = 0;
			strtab = pstrseg->stab;
			goto redo;
		}

	tssiz += strsiz - pstrseg->nelem; /* length of the string */
	pstrseg->nelem = strsiz;
	tnum++;
	ptabseg->nelem++;
}

char *
allocate(
int size)
{
	char *p;

	if((p = malloc(size)) == NULL){
		fprintf(stderr, "ranlib: ran out of memory\n");
		exit(1);
	}
	return(p);
}

/* free segments */
void
segclean()
{
	register struct tabsegment *ptab;
	register struct strsegment *pstr;

	/*
	 * symbol table
	 *
	 * The first entry is static.
	 */
	ptabseg = &tabbase;
	ptab = ptabseg->pnext;
	while (ptabseg = ptab) {
		ptab = ptabseg->pnext;
		free((char *)ptabseg);
	}
	ptabseg = &tabbase;
	ptabseg->pnext = NULL;
	ptabseg->nelem = 0;

	/*
	 * string table
	 *
	 * The first entry is static.
	 */
	pstrseg = &strbase;
	pstr = pstrseg->pnext;
	while (pstrseg = pstr) {
		pstr = pstrseg->pnext;
		free((char *)pstrseg);
	}
	pstrseg = &strbase;
	pstrseg->pnext = NULL;
	pstrseg->nelem = 0;
}

fixsize()
{
	int i;
	register struct tabsegment *ptab;

	tssiz = round(tssiz, sizeof(long));
	offdelta = sizeof(archdr) + sizeof (tnum) + tnum * sizeof(struct ranlib) +
	    sizeof (tssiz) + tssiz;
	off = SARMAG;
	nextel(fi);
	if(strncmp(archdr.ar_name, SYMDEF, sizeof(SYMDEF) - 1) == 0) {
		new = 0;
		offdelta -= sizeof(archdr) + arsize;
		startoff = SARMAG + sizeof(archdr) + arsize;
	} else {
		new = 1;
		startoff = SARMAG;
	}
	strncpy(firstname, archdr.ar_name, sizeof(archdr.ar_name));
	for(i = 0; i < sizeof(archdr.ar_name) ; i++){
		if(firstname[i] == ' ')
			firstname[i] = '\0';
	}
	ptab = &tabbase;
	do {
		for (i = 0; i < ptab->nelem; i++)
			ptab->tab[i].ran_off += offdelta;
	} while (ptab = ptab->pnext);
	return(new);
}

/*
 * Round v to a multiple of r.
 */
static
long
round(
long v,
unsigned long r)
{
	r--;
	v += r;
	v &= ~(long)r;
	return(v);
}

/* patch time */
fixdate(s)
	char *s;
{
	long time();
	char buf[24];
	int fd;
#ifdef	NeXT_NFS
	/*  vars for setting time across the net */
	int tm;
	struct stat stat;
#endif	NeXT_NFS

	fd = open(s, 1);
	if(fd < 0) {
		fprintf(stderr, "ranlib: can't reopen %s\n", s);
		return;
	}
#ifdef	NeXT_NFS
	/*  stat file descriptor and set time appropiately */
	if (fstat(fd, &stat) < 0) {
		fprintf(stderr, "ranlib: can't stat %s\n", s);
		return;
        }
	tm = time((long *)NULL);
	tm = (tm > stat.st_mtime) ? tm : stat.st_mtime;


	sprintf(buf, "%-*ld", sizeof(archdr.ar_date), tm+5);
#else	NeXT_NFS
	sprintf(buf, "%-*ld", sizeof(archdr.ar_date), time((long *)NULL)+5);
#endif	NeXT_NFS
	lseek(fd, (long)SARMAG + ((char *)archdr.ar_date-(char *)&archdr), 0);
	write(fd, buf, sizeof(archdr.ar_date));
#define OS_BUG
#ifdef OS_BUG
	fsync(fd);
#endif
	close(fd);
}

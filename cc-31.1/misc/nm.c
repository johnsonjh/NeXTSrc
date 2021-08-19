#ifndef lint
static	char sccsid[] = "@(#)nm.c 4.7 5/19/86";
#endif
/*
 * nm - print name list; VAX string table version
 */
#include <sys/types.h>
#include <ar.h>
#include <stdio.h>
#include <ctype.h>
#include <a.out.h>
#include <stab.h>
#include <sys/stat.h>
#include <sys/loader.h>
#include "mlist.h"
#include <stdlib.h>
#include <string.h>

struct mlist *ml, *mp;
long mlist_cnt;

static long find(const long *value, const struct mlist *mp);

int	aflg, gflg, nflg, oflg, pflg, uflg, mflg, xflg, sflg, jflg, lflg;
int	rflg = 1, zflg = 1;
char	**xargv;
int	archive;
struct	ar_hdr	archdr;
long	arsize;
char	*membername;
union {
	char	mag_armag[SARMAG+1];
	struct	exec mag_exp;
	struct	mach_header mag_mh;
} mag_un;
#define	OARMAG	0177545
FILE	*fi;
off_t	off, next_off;
off_t	ftell();
char	*strp;
char	*stab();
off_t	strsiz;
int	compare();
int	narg;
int	errs;

long text_nsect, data_nsect, bss_nsect;
long nsect, sect_addr, sect_size, sect_start_symbol;
char *segname, *sectname;

struct nlist *getsyms();

main(argc, argv)
char **argv;
{

	if (--argc>0 && argv[1][0]=='-' && argv[1][1]!=0) {
		argv++;
		while (*++*argv) switch (**argv) {

		case 'n':
			nflg++;
			continue;
		case 'g':
			gflg++;
			continue;
		case 'u':
			uflg++;
			continue;
		case 'r':
			rflg = -1;
			continue;
		case 'p':
			pflg++;
			continue;
		case 'o':
			oflg++;
			continue;
		case 'a':
			aflg++;
			continue;
		case 'm':
			mflg++;
			continue;
		case 'x':
			xflg++;
			continue;
		case 'z':
			zflg = 0;
			continue;
		case 's':
			sflg++;
			continue;
		case 'j':
			jflg++;
			continue;
		case 'l':
			lflg++;
			continue;
		default:
			fprintf(stderr, "nm: invalid argument -%c\n",
			    *argv[0]);
			exit(2);
		}
		argc--;
	}
	if(xflg && zflg)
		zflg = 0;
	if(sflg){
		if(argc < 2){
			fprintf(stderr, "nm: missing arguments to -s\n");
			exit(2);
		}
		segname = *++argv;
		sectname = *++argv;
		argc -= 2;
	}
	if (argc == 0) {
		argc = 1;
		argv[1] = "a.out";
	}
	narg = argc;
	xargv = argv;
	while (argc--) {
		++xargv;
		text_nsect = -1;
		data_nsect = -1;
		bss_nsect = -1;
		nsect = -1;
		sect_start_symbol = 0;
		namelist();
	}
	exit(errs);
}

namelist()
{
    char *p;
    int len;

	membername = NULL;
	/*
	 * Look for a file name of the form "archive(member)" which is
	 * to mean a member in that archive (the member name must be at
	 * least one character long to be recognized as this form).
	 */
	len = strlen(*xargv);
	if((*xargv)[len-1] == ')'){
	    p = strrchr(*xargv, '(');
	    if(p != NULL && (p - *xargv) > 1){
		membername = p+1;
		*p = '\0';
		(*xargv)[len-1] = '\0';
	    }
	}

	archive = 0;
	fi = fopen(*xargv, "r");
	if (fi == NULL) {
		error(0, "cannot open");
		return;
	}
	fread((char *)&mag_un, 1, sizeof(mag_un), fi);
	if (mag_un.mag_exp.a_magic == OARMAG) {
		error(0, "old archive");
		goto out;
	}
	if (strncmp(mag_un.mag_armag, ARMAG, SARMAG)==0)
		archive++;
	else if (N_BADMAG(mag_un.mag_exp) &&
		 mag_un.mag_mh.magic != MH_MAGIC) {
		error(0, "bad format");
		goto out;
	}
	fseek(fi, 0L, 0);
	if (archive) {
		next_off = SARMAG;
		nextel(fi);
		if (membername == NULL && narg > 1)
			printf("\n%s:\n", *xargv);
	}
	else
		off = 0;
	do {
		off_t o;
		int i, j, k, n, c;
		struct nlist *symp = NULL;
		struct nlist sym;
		struct stat stb;

		if (archive == 0)
			fstat(fileno(fi), &stb);
		fread((char *)&mag_un.mag_exp, 1, sizeof(struct exec), fi);
		if (mag_un.mag_mh.magic == MH_MAGIC) {

		    struct load_command *load_commands, *lcp;
		    struct symtab_command *stp;
		    struct segment_command *sgp;
		    struct section *sp, **sections;
		    long nsects;

		    fseek(fi, off, 0);
		    fread((char *)&mag_un.mag_mh, 1,
			  sizeof(struct mach_header), fi);
		    load_commands = (struct load_command *)
				    malloc(mag_un.mag_mh.sizeofcmds);
		    if(load_commands == NULL)
			error(1, "out of memory");
		    fread((char *)load_commands, 1,
			  mag_un.mag_mh.sizeofcmds, fi);
		    nsects = 0;
		    sections = NULL;
		    stp = NULL;
		    lcp = load_commands;
		    for (i = 0; i < mag_un.mag_mh.ncmds; i++){
			if(stp == NULL && lcp->cmd == LC_SYMTAB){
			    stp = (struct symtab_command *)lcp;
			}
			else if(lcp->cmd == LC_SEGMENT){
			    sgp = (struct segment_command *)lcp;
			    nsects += sgp->nsects;
			}
			lcp = (struct load_command *)
				    ((char *)lcp + lcp->cmdsize);
		    }
		    if (stp == NULL || stp->nsyms == 0){
			error(0, "no name list");
			continue;
		    }
		    if (stp->stroff == 0){
			error(0, "no string table or truncated file");
			continue;
		    }
		    if(nsects > 0){
			sections = (struct section **)
				   malloc(sizeof(struct section *) * nsects);
			k = 0;
			lcp = load_commands;
			for (i = 0; i < mag_un.mag_mh.ncmds; i++){
			    if(lcp->cmd == LC_SEGMENT){
				sgp = (struct segment_command *)lcp;
				sp = (struct section *)
				 ((char *)sgp + sizeof(struct segment_command));
				for(j = 0; j < sgp->nsects; j++){
				    if(strcmp((sp + j)->sectname,
					      SECT_TEXT) == 0 &&
				       strcmp((sp + j)->segname,
					      SEG_TEXT) == 0)
					text_nsect = k + 1;
				    else if(strcmp((sp + j)->sectname,
					      SECT_DATA) == 0 &&
				       strcmp((sp + j)->segname,
					      SEG_DATA) == 0)
					data_nsect = k + 1;
				    else if(strcmp((sp + j)->sectname,
					      SECT_BSS) == 0 &&
				       strcmp((sp + j)->segname,
					      SEG_DATA) == 0)
					bss_nsect = k + 1;
				    if(segname != NULL && sectname !=NULL){
					if(strncmp((sp + j)->sectname, sectname,
						   sizeof(sp->sectname)) == 0 &&
				           strncmp((sp + j)->segname, segname,
						   sizeof(sp->segname)) == 0){
					    nsect = k + 1;
					    sect_addr = (sp + j)->addr;
					    sect_size = (sp + j)->size;
					}
				    }
				    sections[k++] = sp + j;
				}
			    }
			    lcp = (struct load_command *)
				  ((char *)lcp + lcp->cmdsize);
			}
		    }
		    fseek(fi, off + stp->symoff, 0);
		    symp = getsyms(stp->nsyms, &i);

		    fseek(fi, off + stp->stroff, 0);
		    strsiz = stp->strsize;
		    strp = (char *)malloc(strsiz);
		    if(strp == NULL)
			error(1, "ran out of memory");
		    if(fread(strp, strsiz, 1, fi) != 1)
			error(1, "error reading string table");
		    if(xflg == 0){
			for(j = 0; j < i; j++){
			    if(symp[j].n_un.n_strx == 0)
				symp[j].n_un.n_name = "";
			    else if(symp[j].n_un.n_strx < 0 ||
				    symp[j].n_un.n_strx > strsiz)
				symp[j].n_un.n_name = "bad string index";
			    else
				symp[j].n_un.n_name =
						symp[j].n_un.n_strx + strp;
			    if((symp[j].n_type & N_TYPE) == N_INDR){
				if(symp[j].n_value == 0)
				    symp[j].n_value = (long)"";
				else if(symp[j].n_value < 0 ||
				        symp[j].n_value > strsiz)
				    symp[j].n_value = (long)"bad string index";
				else
				    symp[j].n_value =
						(long)(symp[j].n_value + strp);
			    }
			}
			if(lflg && nsect != -1 && sect_start_symbol == 0 &&
			   sect_size != 0){
			    if (symp==NULL)
				symp = (struct nlist *) malloc(
					sizeof(struct nlist));
			    else
				symp = (struct nlist *) realloc(symp,
					(i+1)*sizeof(struct nlist));
			    if (symp == NULL)
				    error(1, "out of memory");
			    symp[i].n_un.n_name = ".section_start";
			    symp[i].n_type = N_SECT;
			    symp[i].n_sect = nsect;
			    symp[i].n_value = sect_addr;
			    i++;
			}
		    }
		    if(zflg){
			ml = mlist(fileno(fi), off, &(mag_un.mag_mh),
				   load_commands, &mlist_cnt);
			if(ml != NULL){
			    for(j = 0; j < i; j++){
				if((symp[j].n_type & N_TYPE) == N_UNDF ||
				   (symp[j].n_type & N_TYPE) == N_ABS ||
				   (symp[j].n_type & N_STAB) != 0 ||
				   (*symp[j].n_un.n_name != '_') )
				    continue;
				mp = bsearch(&symp[j].n_value, ml, mlist_cnt,
				     sizeof(struct mlist),
				     (int (*)(const void *, const void *))find);
				if(mp != NULL)
				    symp[j].n_un.n_name = mp->m_name;
			    }
			}
		    }
		    if(pflg == 0)
			qsort(symp, i, sizeof(struct nlist), compare);
		    if((archive || narg > 1) && oflg == 0)
			if(archive)
			    printf("\n%-0.16s:\n", archdr.ar_name);
			else
			    printf("\n%s:\n", *xargv);
		    if(mflg && !uflg)
			pmachsyms(symp, i, sections, nsects);
		    else
			psyms(symp, i);
		    if(ml != NULL){
			free((char *)ml);
			ml = NULL;
		    }
		    if(symp != NULL){
			free((char *)symp);
			symp = NULL;
		    }
		    if(strp != NULL){
			free((char *)strp);
			strp = NULL;
		    }
		    if(load_commands != NULL){
			free((char *)load_commands);
			load_commands = NULL;
		    }
		    if(sections != NULL){
			free((char *)sections);
			sections = NULL;
		    }
		}
		else {
			if (N_BADMAG(mag_un.mag_exp))
				continue;
			n = mag_un.mag_exp.a_syms / sizeof(struct nlist);
			if (n == 0) {
				error(0, "no name list");
				continue;
			}
			if (N_STROFF(mag_un.mag_exp) + sizeof (off_t) >
			    (archive ? arsize : stb.st_size)) {
				error(0, "old format .o (no string table) or truncated file");
				continue;
			}

			fseek(fi, off + N_SYMOFF(mag_un.mag_exp), 0);
			symp = getsyms(n, &i);

			fseek(fi, off + N_STROFF(mag_un.mag_exp), 0);
			if (archive && ftell(fi)+sizeof(off_t) >= next_off) {
				error(0, "no string table (old format .o?)");
				continue;
			}
			if (fread((char *)&strsiz,sizeof(strsiz),1,fi) != 1) {
				error(0, "no string table (old format .o?)");
				goto out;
			}
			strp = (char *)malloc(strsiz);
			if (strp == NULL)
				error(1, "ran out of memory");
			if (fread(strp+sizeof(strsiz),strsiz-sizeof(strsiz),
			    1,fi) != 1)
				error(1, "error reading string table");

			if (xflg==0){
			    for (j = 0; j < i; j++){
				if(symp[j].n_un.n_strx == 0)
				    symp[j].n_un.n_name = "";
				else if(symp[j].n_un.n_strx < 0 ||
					symp[j].n_un.n_strx > strsiz)
				    symp[j].n_un.n_name = "bad string index";
				else
				    symp[j].n_un.n_name =
						    symp[j].n_un.n_strx + strp;
				if((symp[j].n_type & N_TYPE) == N_INDR){
				    if(symp[j].n_value == 0)
					symp[j].n_value = (long)"";
				    else if(symp[j].n_value < 0 ||
					    symp[j].n_value > strsiz)
					symp[j].n_value =
						(long)"bad string index";
				    else
					symp[j].n_value =
						(long)(symp[j].n_value + strp);
				}
			    }
			}
			if (pflg==0)
				qsort(symp, i, sizeof(struct nlist), compare);
			if ((archive || narg>1) && oflg==0)
			    if(archive)
				printf("\n%-0.16s:\n", archdr.ar_name);
			    else
				printf("\n%s:\n", *xargv);
			psyms(symp, i);
			if (symp)
				free((char *)symp), symp = 0;
			if (strp)
				free((char *)strp), strp = 0;
		}
	} while(archive && nextel(fi));
out:
	fclose(fi);
}

struct nlist *
getsyms(nsyms, countp)
int nsyms;
int *countp;
{
	int i;
	struct nlist sym, *symp;

	symp = NULL;
	i = 0;
	while (--nsyms >= 0) {
		fread((char *)&sym, 1, sizeof(sym), fi);
		if (gflg && (sym.n_type&N_EXT)==0)
			continue;
		if (sflg) {
			if(((sym.n_type & N_TYPE) == N_SECT) &&
			    (sym.n_sect == nsect)){
				if(lflg && sym.n_value == sect_addr){
					sect_start_symbol = 1;
				}
			}
			else
			    continue;
		}
		if ((sym.n_type&N_STAB) && (!aflg||gflg||uflg))
			continue;
		if (symp==NULL)
			symp = (struct nlist *) malloc(sizeof(struct nlist));
		else
			symp = (struct nlist *) realloc(symp,
				(i+1)*sizeof(struct nlist));
		if (symp == NULL)
			error(1, "out of memory");
		symp[i++] = sym;
	}
	*countp = i;
	return(symp);
}

psyms(symp, nsyms)
	register struct nlist *symp;
	int nsyms;
{
	register int n, c;

	for (n=0; n<nsyms; n++) {
		if (xflg){
			printf("%08x %02x %02x %04x ",
			    symp[n].n_value, symp[n].n_type & 0xff,
			    symp[n].n_sect & 0xff, symp[n].n_desc & 0xffff);
			if(symp[n].n_un.n_strx == 0)
			    printf("%08x (null)", symp[n].n_un.n_strx);
			else if(symp[n].n_un.n_strx < 0 ||
				symp[n].n_un.n_strx > strsiz)
			    printf("%08x (bad string index)",
				   symp[n].n_un.n_strx);
			else
			    printf("%08x %s", symp[n].n_un.n_strx,
				   symp[n].n_un.n_strx + strp);
			if ((symp[n].n_type & N_TYPE) == N_INDR){
			    if(symp[n].n_value == 0)
				printf(" (indirect for %08x (null))\n",
				       symp[n].n_value);
			    else if(symp[n].n_value < 0 ||
				    symp[n].n_value > strsiz)
				printf(" (indirect for %08x (bad string "
				       "index))\n", symp[n].n_value);
			    else
				printf(" (indirect for %08x %s)\n",
				       symp[n].n_value,
				       symp[n].n_value + strp);
			}
			else
			    printf("\n");
			continue;
		}
		c = symp[n].n_type;
		if (c & N_STAB) {
			if (oflg) {
				if (archive)
					printf("%s:%-0.16s:", *xargv,
					       archdr.ar_name);
				else
					printf("%s:", *xargv);
			}
			printf("%08x - %02x %04x %5.5s %s\n",
			    symp[n].n_value,
			    symp[n].n_sect & 0xff, symp[n].n_desc & 0xffff,
			    stab(symp[n].n_type & 0xff), symp[n].n_un.n_name);
			continue;
		}
		if (c == N_FN)
			c = 'f';
		else switch (c&N_TYPE) {

		case N_UNDF:
			c = 'u';
			if (symp[n].n_value)
				c = 'c';
			break;
		case N_ABS:
			c = 'a';
			break;
		case N_TEXT:
			c = 't';
			break;
		case N_DATA:
			c = 'd';
			break;
		case N_BSS:
			c = 'b';
			break;
		case N_SECT:
			if(symp[n].n_sect == text_nsect)
				c = 't';
			else if(symp[n].n_sect == data_nsect)
				c = 'd';
			else if(symp[n].n_sect == bss_nsect)
				c = 'b';
			else
				c = 's';
			break;
		case N_INDR:
			c = 'i';
			break;
		default:
			c = '?';
			break;
		}
		if(uflg && c!='u')
			continue;
		if (oflg) {
			if (archive)
				printf("%s:%-0.16s:", *xargv, archdr.ar_name);
			else
				printf("%s:", *xargv);
		}
		if (symp[n].n_type&N_EXT && c != '?')
			c = toupper(c);
		if (!uflg && !jflg) {
			if (c=='u' || c=='U' || c=='i' || c=='I')
				printf("        ");
			else
				printf("%08x", symp[n].n_value);
			printf(" %c ", c);
		}
		if (!jflg && (symp[n].n_type & N_TYPE) == N_INDR)
			printf("%s (indirect for %s)\n",
			       symp[n].n_un.n_name,
			       (char *)symp[n].n_value);
		else
			printf("%s\n", symp[n].n_un.n_name);
l1:		;
	}
}

pmachsyms(symp, nsyms, sections, nsects)
struct nlist *symp;
int nsyms;
struct section **sections;
int nsects;
{
    int n;

	for(n = 0; n < nsyms; n++){
		if (xflg){
			printf("%08x %02x %02x %04x ",
			    symp[n].n_value, symp[n].n_type & 0xff,
			    symp[n].n_sect & 0xff, symp[n].n_desc & 0xffff);
			if(symp[n].n_un.n_strx == 0)
			    printf("%08x (null)", symp[n].n_un.n_strx);
			else if(symp[n].n_un.n_strx < 0 ||
				symp[n].n_un.n_strx > strsiz)
			    printf("%08x (bad string index)",
				   symp[n].n_un.n_strx);
			else
			    printf("%08x %s", symp[n].n_un.n_strx,
				   symp[n].n_un.n_strx + strp);
			if ((symp[n].n_type & N_TYPE) == N_INDR){
			    if(symp[n].n_value == 0)
				printf(" (indirect for %08x (null))\n",
				       symp[n].n_value);
			    else if(symp[n].n_value < 0 ||
				    symp[n].n_value > strsiz)
				printf(" (indirect for %08x (bad string "
				       "index))\n", symp[n].n_value);
			    else
				printf(" (indirect for %08x %s)\n",
				       symp[n].n_value,
				       symp[n].n_value + strp);
			}
			else
			    printf("\n");
			continue;
		}
		if (symp[n].n_type & N_STAB) {
			if (oflg) {
				if (archive)
					printf("%s:%-0.16s:", *xargv,
					       archdr.ar_name);
				else
					printf("%s:", *xargv);
			}
			printf("%08x - %02x %04x %5.5s %s\n",
			    symp[n].n_value,
			    symp[n].n_sect & 0xff, symp[n].n_desc & 0xffff,
			    stab(symp[n].n_type & 0xff), symp[n].n_un.n_name);
			continue;
		}
		if (oflg) {
			if (archive)
				printf("%s:%-0.16s:", *xargv, archdr.ar_name);
			else
				printf("%s:", *xargv);
		}
		if( ((symp[n].n_type & N_TYPE) == N_UNDF &&
		     symp[n].n_value == 0) ||
		     (symp[n].n_type & N_TYPE) == N_INDR )
			printf("        ");
		else
			printf("%08x", symp[n].n_value);

		switch (symp[n].n_type & N_TYPE) {

		case N_UNDF:
			if (symp[n].n_value)
				printf(" (common) ");
			else
				printf(" (undefined) ");
			break;
		case N_ABS:
			printf(" (absolute) ");
			break;
		case N_INDR:
			printf(" (indirect) ");
			break;
		case N_SECT:
			if(symp[n].n_sect >= 1 && symp[n].n_sect <= nsects)
				printf(" (%0.16s,%0.16s) ",
				       sections[symp[n].n_sect - 1]->segname,
				       sections[symp[n].n_sect - 1]->sectname);
			else
				printf(" (?,?) ");
			break;
		case N_TEXT:
			printf(" N_TEXT ");
			break;
		case N_DATA:
			printf(" N_DATA ");
			break;
		case N_BSS:
			printf(" N_BSS ");
			break;
		default:
			printf(" (?) ");
			break;
		}
		if(symp[n].n_type & N_EXT)
			printf("external ");
		else
			printf("non-external ");
		if ((symp[n].n_type & N_TYPE) == N_INDR)
			printf("%s (for %s)\n",
			       symp[n].n_un.n_name,
			       (char *)symp[n].n_value);
		else
			printf("%s\n", symp[n].n_un.n_name);
	}
}

compare(p1, p2)
struct nlist *p1, *p2;
{
	register i;

	if (nflg) {
		if (p1->n_value > p2->n_value)
			return(rflg);
		if (p1->n_value < p2->n_value)
			return(-rflg);
	}
	if(xflg)
	    return (rflg * strcmp(p1->n_un.n_strx + strp,
				  p2->n_un.n_strx + strp));
	else
	    return (rflg * strcmp(p1->n_un.n_name, p2->n_un.n_name));
}

static
long
find(
const long *value,
const struct mlist *mp)
{
	return(*value - mp->m_value);
}

nextel(af)
FILE *af;
{
	register char *cp;
	register r;

	for(;;){
		fseek(af, next_off, 0);
		r = fread((char *)&archdr, 1, sizeof(struct ar_hdr), af);
		if (r != sizeof(struct ar_hdr))
			return(0);
		for (cp = archdr.ar_name;
		     cp < &archdr.ar_name[sizeof(archdr.ar_name)];
		     cp++)
			if (*cp == ' ')
				*cp = '\0';
		arsize = atol(archdr.ar_size);
		if (arsize & 1)
			++arsize;
		off = ftell(af);	/* beginning of current element */
		next_off = off + arsize;/* beginning of next element */
		if(membername == NULL || strncmp(membername, archdr.ar_name,
					         sizeof(archdr.ar_name)) == 0)
			return(1);
	}
}

error(n, s)
char *s;
{
	fprintf(stderr, "nm: %s:", *xargv);
	if (archive) {
		fprintf(stderr, "(%-0.16s)", archdr.ar_name);
		fprintf(stderr, ": ");
	} else
		fprintf(stderr, " ");
	fprintf(stderr, "%s\n", s);
	if (n)
		exit(2);
	errs = 1;
}

struct	stabnames {
	int	st_value;
	char	*st_name;
} stabnames[] ={
	N_GSYM, "GSYM",
	N_FNAME, "FNAME",
	N_FUN, "FUN",
	N_STSYM, "STSYM",
	N_LCSYM, "LCSYM",
	N_RSYM, "RSYM",
	N_SLINE, "SLINE",
	N_SSYM, "SSYM",
	N_SO, "SO",
	N_LSYM, "LSYM",
	N_SOL, "SOL",
	N_PSYM, "PSYM",
	N_ENTRY, "ENTRY",
	N_LBRAC, "LBRAC",
	N_RBRAC, "RBRAC",
	N_BCOMM, "BCOMM",
	N_ECOMM, "ECOMM",
	N_ECOML, "ECOML",
	N_LENG, "LENG",
	N_PC, "PC",
	0, 0
};

char *
stab(val)
{
	register struct stabnames *sp;
	static char prbuf[32];

	for (sp = stabnames; sp->st_name; sp++)
		if (sp->st_value == val)
			return (sp->st_name);
	sprintf(prbuf, "%02x", val);
	return (prbuf);
}

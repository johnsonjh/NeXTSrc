/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)nlist.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#if	NeXT
#include <stdlib.h>
#endif	NeXT
#include <sys/types.h>

/* Stuff lifted from <a.out.h> and <sys/exec.h> since they are gone */
/*
 * Header prepended to each a.out file.
 */
struct exec {
#if	sun || NeXT
unsigned short  a_machtype;     /* machine type */
unsigned short  a_magic;        /* magic number */
#else	sun || NeXT
	long	a_magic;	/* magic number */
#endif	sun || NeXT
unsigned long	a_text;		/* size of text segment */
unsigned long	a_data;		/* size of initialized data */
unsigned long	a_bss;		/* size of uninitialized data */
unsigned long	a_syms;		/* size of symbol table */
unsigned long	a_entry;	/* entry point */
unsigned long	a_trsize;	/* size of text relocation */
unsigned long	a_drsize;	/* size of data relocation */
};

#define	OMAGIC	0407		/* old impure format */
#define	NMAGIC	0410		/* read-only text */
#define	ZMAGIC	0413		/* demand load format */

#define	N_BADMAG(x) \
    (((x).a_magic)!=OMAGIC && ((x).a_magic)!=NMAGIC && ((x).a_magic)!=ZMAGIC)
#define	N_TXTOFF(x) \
	((x).a_magic==ZMAGIC ? 0 : sizeof (struct exec))
#define N_SYMOFF(x) \
	(N_TXTOFF(x) + (x).a_text+(x).a_data + (x).a_trsize+(x).a_drsize)

#include <nlist.h>
#include <stdio.h>
#include <sys/loader.h>

/*
 * nlist - retreive attributes from name list (string table version)
 */
nlist(name, list)
	char *name;
	struct nlist *list;
{
	register struct nlist *p, *q;
	register char *s1, *s2;
	register n, m;
	int maxlen, nreq;
	FILE *f;
	FILE *sf;
	off_t sa;		/* symbol address */
	off_t ss;		/* start of strings */
	struct exec buf;
	struct nlist space[BUFSIZ/sizeof (struct nlist)];

	maxlen = 0;
	for (q = list, nreq = 0; q->n_un.n_name && q->n_un.n_name[0]; q++, nreq++) {
		q->n_type = 0;
		q->n_value = 0;
		q->n_desc = 0;
		q->n_sect = 0;
		n = strlen(q->n_un.n_name);
		if (n > maxlen)
			maxlen = n;
	}
	f = fopen(name, "r");
	if (f == NULL)
		return (-1);
	fread((char *)&buf, sizeof buf, 1, f);
	if (N_BADMAG(buf) && *((long *)&buf) != MH_MAGIC) {
		fclose(f);
		return (-1);
	}
	if (*((long *)&buf) == MH_MAGIC) {
	    struct mach_header mh;
	    struct load_command *load_commands, *lcp;
	    struct symtab_command *stp;
	    long i;

		fseek(f, 0, 0);
		if (fread((char *)&mh, sizeof (mh), 1, f) != 1) {
			fclose(f);
			return (-1);
		}
		load_commands = (struct load_command *)malloc(mh.sizeofcmds);
		if (load_commands == NULL) {
			fclose(f);
			return (-1);
		}
		if (fread((char *)load_commands, mh.sizeofcmds, 1, f) != 1) {
			free(load_commands);
			fclose(f);
			return (-1);
		}
		stp = NULL;
		lcp = load_commands;
		for (i = 0; i < mh.ncmds; i++) {
			if (lcp->cmdsize % sizeof(long) != 0 ||
			    lcp->cmdsize <= 0 ||
			    (char *)lcp + lcp->cmdsize >
			    (char *)load_commands + mh.sizeofcmds) {
				free(load_commands);
				fclose(f);
				return (-1);
			}
			if (lcp->cmd == LC_SYMTAB) {
				if (lcp->cmdsize !=
				   sizeof(struct symtab_command)) {
					free(load_commands);
					fclose(f);
					return (-1);
				}
				stp = (struct symtab_command *)lcp;
				break;
			}
			lcp = (struct load_command *)
			      ((char *)lcp + lcp->cmdsize);
		}
		if (stp == NULL) {
			free(load_commands);
			fclose(f);
			return (-1);
		}
		sa = stp->symoff;
		ss = stp->stroff;
		n = stp->nsyms * sizeof(struct nlist);
		free(load_commands);
	}
	else {
		sa = N_SYMOFF(buf);
		ss = sa + buf.a_syms;
		n = buf.a_syms;
	}
	sf = fopen(name, "r");
	if (sf == NULL) {
		/* ??? */
		fclose(f);
		return(-1);
	}
	fseek(f, sa, 0);
	while (n) {
		m = sizeof (space);
		if (n < m)
			m = n;
		if (fread((char *)space, m, 1, f) != 1)
			break;
		n -= m;
		for (q = space; (m -= sizeof(struct nlist)) >= 0; q++) {
			char nambuf[BUFSIZ];

			if (q->n_un.n_strx == 0 || q->n_type & N_STAB)
				continue;
			fseek(sf, ss+q->n_un.n_strx, 0);
			fread(nambuf, maxlen+1, 1, sf);
			for (p = list; p->n_un.n_name && p->n_un.n_name[0]; p++) {
				s1 = p->n_un.n_name;
				s2 = nambuf;
				while (*s1) {
					if (*s1++ != *s2++)
						goto cont;
				}
				if (*s2)
					goto cont;
				p->n_value = q->n_value;
				p->n_type = q->n_type;
				p->n_desc = q->n_desc;
				p->n_sect = q->n_sect;
				if (--nreq == 0)
					goto alldone;
				break;
		cont:		;
			}
		}
	}
alldone:
	fclose(f);
	fclose(sf);
	return (nreq);
}

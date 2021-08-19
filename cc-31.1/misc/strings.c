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
static char sccsid[] = "@(#)strings.c	5.1 (Berkeley) 5/31/85";
#endif not lint

#include <stdio.h>
#include <a.out.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/loader.h>

struct mach_header mh;
struct load_command *load_commands, *lcp;
struct segment_command *sgp;
struct section *sp, *text_sect, *data_sect;

struct exec exec;

long	ftell();

/*
 * strings
 */

char	*infile = "Standard input";
int	oflg;
int	asdata;
int	allsections;
long	offset;
int	minlength = 4;

main(argc, argv)
	int argc;
	char *argv[];
{
	register int i, j;

	argc--, argv++;
	while (argc > 0 && argv[0][0] == '-') {
		if (argv[0][1] == 0)
			asdata++;
		else for (i = 1; argv[0][i] != 0; i++) switch (argv[0][i]) {

		case 'o':
			oflg++;
			break;

		case 'a':
			allsections++;
			break;

		default:
			if (!isdigit(argv[0][i])) {
				fprintf(stderr, "Usage: strings [ -a ] [ -o ] [ -# ] [ file ... ]\n");
				exit(1);
			}
			minlength = argv[0][i] - '0';
			for (i++; isdigit(argv[0][i]); i++)
				minlength = minlength * 10 + argv[0][i] - '0';
			i--;
			break;
		}
		argc--, argv++;
	}
	do {
		if (argc > 0) {
			if (freopen(argv[0], "r", stdin) == NULL) {
				perror(argv[0]);
				exit(1);
			}
			infile = argv[0];
			argc--, argv++;
		}
		fseek(stdin, (long) 0, L_SET);
		if (asdata) {
			find((long) 100000000L);
			continue;
		}
		fseek(stdin, (long) 0, L_SET);
		if (fread((char *)&mh, sizeof(struct mach_header), 1, stdin) !=
		    1 || mh.magic != MH_MAGIC){
			fseek(stdin, (long) 0, L_SET);
			find((long) 100000000L);
			continue;
		}
		load_commands = (struct load_command *)malloc(mh.sizeofcmds);
		if(load_commands == NULL){
		    printf("strings: out of memory\n");
		    exit(1);
		}
		if(fread((char *)load_commands, mh.sizeofcmds, 1, stdin) != 1){
		    printf("strings: can't read load commands of %s\n", infile);
		    continue;
		}
		lcp = load_commands;
		for(i = 0; i < mh.ncmds; i++){
		    if(lcp->cmd != LC_SEGMENT)
			continue;
		    sgp = (struct segment_command *)lcp;
		    sp = (struct section *)((char *)sgp +
			    sizeof(struct segment_command));
		    for(j = 0; j < sgp->nsects; j++){
			if(allsections){
			    if((sp->flags & S_ZEROFILL) != S_ZEROFILL){
				fseek(stdin, sp->offset , L_SET);
				find(sp->size);
			    }
			}
			else{
			    if((sp->flags & S_ZEROFILL) != S_ZEROFILL &&
			       (strcmp(sp->sectname, SECT_TEXT) != 0 ||
				strcmp(sp->segname, SEG_TEXT) != 0)){
				fseek(stdin, sp->offset , L_SET);
				find(sp->size);
			    }
			}
			sp++;
		    }
		    lcp = (struct load_command *)
				((char *)lcp + lcp->cmdsize);
		}
		free(load_commands);
		continue;
	} while (argc > 0);
}

find(cnt)
	long cnt;
{
	static char buf[BUFSIZ];
	register char *cp;
	register int c, cc;

	cp = buf, cc = 0;
	for (; cnt != 0; cnt--) {
		c = getc(stdin);
		if (c == '\n' || dirt(c) || cnt == 0) {
			if (cp > buf && cp[-1] == '\n')
				--cp;
			*cp++ = 0;
			if (cp > &buf[minlength]) {
				if (oflg)
					printf("%7D ", ftell(stdin) - cc - 1);
				printf("%s\n", buf);
			}
			cp = buf, cc = 0;
		} else {
			if (cp < &buf[sizeof buf - 2])
				*cp++ = c;
			cc++;
		}
		if (ferror(stdin) || feof(stdin))
			break;
	}
}

dirt(c)
	int c;
{

	switch (c) {

	case '\n':
	case '\f':
		return (0);

	case 0177:
		return (1);

	default:
		return (c > 0200 || c < ' ');
	}
}

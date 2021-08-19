#include <sys/file.h>
#include <stdio.h>

#include "term.desc"

int fake_exec[8];

/* typewriter driving table structure*/
struct nt {
	int bset;
	int breset;
	int Hor;
	int Vert;
	int Newline;
	int Char;
	int Em;
	int Halfline;
	int Adj;
	char *twinit;
	char *twrest;
	char *twnl;
	char *hlr;
	char *hlf;
	char *flr;
	char *bdon;
	char *bdoff;
	char *ploton;
	char *plotoff;
	char *up;
	char *down;
	char *right;
	char *left;
	char *codetab[256-32];
	int zzz;
} nt;

main(argc, argv)
int argc;
char **argv;
{
	int offset, fd, i, len;
	char **tp, **np;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s OUTFILE\n", argv[0]);
		exit(1);
	}
	fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "%s: Couldn't open %s\n", argv[0], argv[1]);
		exit(1);
	}
	offset = sizeof(t);
	nt = *(struct nt *)&t;
	for (tp = &t.twinit, np = &nt.twinit; tp < (char **)&t.zzz; tp++,np++) {
		if (*tp != NULL) {
			*np = (char *)offset;
			offset += max(strlen(*tp)+1, tp>=t.codetab ? 3 : 0);
		}
	}
	fake_exec[2] = offset;
	i = write(fd,(char *)fake_exec,sizeof(fake_exec));
	if (i != sizeof(fake_exec)) {
		fprintf(stderr, "%s: Couldn't write fake_exec\n", argv[0]);
		exit(1);
	}
	i = write(fd, (char *)&nt, sizeof(nt));
	if (i != sizeof(nt)) {
		fprintf(stderr, "%s: Couldn't write nt\n", argv[0]);
		exit(1);
	}
	for (tp = &t.twinit; tp < (char **)&t.zzz; tp++) {
		if (*tp != NULL) {
			len = max(strlen(*tp)+1, tp>=t.codetab ? 3 : 0);
			if (write(fd, *tp, len) != len) {
				fprintf(stderr, "%s: Couldn't write t string\n",
				    argv[0]);
				exit(1);
			}
		}
	}
	close(fd);
	exit(0);
}

max(a,b)
{
	return(a>b?a:b);
}

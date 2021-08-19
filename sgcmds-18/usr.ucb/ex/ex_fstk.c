#include "ex.h"

static int alttos = -1;
static int altbot;
static struct fstk {
	char	file[FNSIZE];
	int		dot;
} 
altstk [FSTKSZ];

extern int altdot;

fs_push (cp, dot)
char *cp;
int dot;
{
	if (alttos != -1) {
		alttos = (alttos + 1) % FSTKSZ;
		if (alttos == altbot)
			altbot = (altbot + 1) % FSTKSZ;
	} 
	else {
		alttos = 0;
	}

	strcpy (altstk[alttos].file, cp);
	altstk[alttos].dot = dot;
#ifdef MAD
	altdot = dot;
	strcpy(altfile, cp);
#endif MAD

}


fs_pop (c)
char c;
{
	if (alttos == -1) {
		altfile[0] = 0;
		altdot = 0;
		return;
	} 
	else {
		strcpy (altfile, altstk[alttos].file);
		altdot = altstk[alttos].dot;
	}

	if (c != 'E') {
		if (alttos == altbot) {
			alttos = -1;
			altbot = 0;
		} else {
			alttos = ((alttos == 0) ? FSTKSZ : alttos) - 1;
		}
	}
}


fs_print ()
{
	int istk;

	if (alttos == -1)
		return;
	if (inopen)
		pofix();
	printf (" #  line filename\n");
	printf ("--- ---- -------------\n");
	for (istk = altbot; ; istk = (istk + 1) % FSTKSZ) {
		printf ("%2d.  %3d %s\n",
		((istk >= altbot) ? istk - altbot : (FSTKSZ - altbot) + istk) + 1,
		altstk[istk].dot, altstk[istk].file); 
		if (istk == alttos)
			break;
	}
	flush();
}

#ifdef notdef
fs_pushs (file)
char *file;
{
	int istk;
	int len = strlen (file);
	struct fstk element;

	if (alttos == -1) {
		error ("No stack|File stack is empty.");
		return;	/* Just in case? */
	}
	istk = alttos - 1;
	while (istk > 0) {
		if (!strncmp (altstk[istk].file, file, len)) {
			element = altstk [istk];
			fs_push (element.file, element.dot);
			return;
		} 
		istk--;
	}
	error ("Not in stack|Cannot find file in file stack.");
}

fs_pushn (n)
int n;
{
	struct fstk element;
	int lim;

	n--;
	if (altbot < alttos) {
		lim = alttos - altbot;
	} else {
		lim = alttos + (FSTKSZ - altbot);
	}
	if (n < 0 || n > lim) {
		error ("Out of range|The number argument to pn is out of range.");
	}
	n = (altbot + n) % FSTKSZ;
	element = altstk [n];
	fs_push (element.file, element.dot);
}
#endif notdef

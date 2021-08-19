/*
 * quick routine for loading up the short nlist info from vmunix.
 *
 * return zero on success, 1 on failure
 **********************************************************************
 * HISTORY
 * 06-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to return as soon as all symbols have been found.
 *
 * 26-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	One more try...  When we initialize the nlist array, place the
 *	number of entries into a counter.  Subtract one from the counter
 *	every time that we find a symbol that we are looking for.  If we
 *	didn't find a symbol the counter will be positive and we should
 *	indicate that nlist should be called.
 *
 * 06-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed incorrect terminating condition check.
 *
 * 05-Nov-85  Rudy Nedved (ern) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */
#include <stdio.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/stat.h>

#define NLISTFILE "/etc/vmnlist"

vmnlist(nlistf,nl)
char *nlistf;
struct nlist nl[];
{
    register FILE *f;
    struct stat stb,stb2;
    char symnam[100];
    register char *p;
    struct nlist n;
    register struct nlist *t;
    register nlsize;

    if (strcmp (nlistf, "/vmunix"))
	return 1;
    if (stat ("/vmunix", &stb) < 0 || stat (NLISTFILE, &stb2) < 0)
	return 1;
    if (stb.st_mtime >= stb2.st_mtime || stb2.st_size == 0)
	return 1;
    if ((f = fopen (NLISTFILE, "r")) == NULL)
	return 1;
    nlsize = 0;
    for (t=nl; t->n_un.n_name != NULL && t->n_un.n_name[0] != '\0'; ++t) {
	t->n_type = 0;
	t->n_value = 0;
	nlsize++;
    }
    for(;;) {
	p = symnam;
	while ((*p++ = fgetc(f)) != '\0' && !feof(f))
	    ;
	if (feof(f)) break;
	if (fread(&n, sizeof(struct nlist), 1, f) != 1) break;
	/* find the symbol in nlist */
	for (t=nl; t->n_un.n_name != NULL && t->n_un.n_name[0] != '\0'; ++t)
	    if (t->n_type == 0 && t->n_value == 0 &&
		strcmp(symnam,t->n_un.n_name) == 0) {
		n.n_un.n_name = t->n_un.n_name;
		*t = n;
		if (--nlsize == 0) {
		    fclose(f);
		    return(0);
		}
		break;
	    }
    }
    fclose(f);
    return(1);
}

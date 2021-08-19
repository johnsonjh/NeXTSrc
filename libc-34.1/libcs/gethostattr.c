/*
 * gethostattr(ptrs, ptrcnt)
 * char *ptrs[];
 * int ptrcnt;
 *
 * This routine takes a pointer to an array of ptrcnt character pointers.
 * It will place a pointer to each attribute of the current host into
 * this array.  The pointers are pointing to a static buffer, so they
 * should be copied if necessary.  The routine returns -1 on error, or
 * the number of pointers placed in the array.  In addition, the pointer
 * after the last pointer used is set to NULL if there is room in the
 * array.
 *
 **********************************************************************
 * HISTORY
 * 08-Nov-85  Jonathan McElravy (jm) at Carnegie-Mellon University
 *	  Added an fclose to close attributes file on success.
 *
 * 12-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#include <stdio.h>
#include <libc.h>

static char buf[BUFSIZ];

gethostattr(ptrs, ptrcnt)
char *ptrs[];
int ptrcnt;
{
    char hname[256], *p, *q;
    int count;
    FILE *f;

    if (gethostname(hname, 256) < 0)
	return(-1);
    if ((f = fopen("/etc/attributes", "r")) == NULL)
	return(-1);
    while (p = fgets(buf, BUFSIZ, f)) {
	if (strcmp(nxtarg(&p, ":"), hname)) continue;
	fclose(f);
	count = 0;
	while (*(q = nxtarg(&p, ":\n"))) {
	    if (count < ptrcnt)
		ptrs[count] = q;
	    count++;
	}
	if (count < ptrcnt)
	    ptrs[count] = NULL;
	return(count);
    }
    fclose(f);
    return(-1);
}

/*
 *	bsearch -- generic binary search, like qsort.
 *
 **********************************************************************
 * HISTORY
 * 03-Feb-86  David Nichols (nichols) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

/* Bsearch does a binary search on the array.  It returns either the index
   of the matching element or the index of the largest element less than the
   key.  The caller must use the compar function to determine if the key was
   found or not. */
int bsearch (base, nel, width, key, compar)
    char *base;			/* start of the array to search */
    int nel;			/* size of the array */
    int width;			/* size of an element of the array */
    char *key;			/* pointer to a key to find */
    int (*compar)();		/* comparison routine like in qsort */
{
    int     l = 0;
    int     u = nel - 1;
    int     m;
    int     r;

    /* Invariant: key > all elements in [0..l) and key < all elements in
       (u..nel-1]. */
    while (l <= u) {
	m = (l + u) / 2;
	r = (*compar) (key, base + (m * width));
	if (r == 0)
	    return m;
	else if (r < 0)
	    u = m - 1;
	else
	    l = m + 1;
    }
    return u;			/* Which should == l - 1 */
}

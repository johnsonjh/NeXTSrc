/*
 * This is the structure that the mlist() routine returns a pointer to.
 * The method name string is made up of the class name and the method name and
 * looks like:
 *	"-[Classname method:name:]"
 * for instance names and:
 *	"+[Classname method:name:]"
 * for class methods.
 */
struct mlist {
	long m_value;	/* address of the method */
	char *m_name;	/* name of the method */
};

/*
 * mlist() returns a pointer to an array of mlist structures and returns the
 * number of sturctures in the array indirectly through mlist_cnt out of
 * the file represented by the open file descriptor, fd, and the pointers to
 * it's mach header and load commands, mh and lc.  offset is the offset from
 * the begining of the file descriptor where the object starts (zero for object
 * files and non-zero for archives).  If any error occurs then this routine
 * returns NULL and zero indirectly through mlist_cnt.  The space for the mlist
 * structures and the strings are allocated contiguously as one block and the
 * pointer to the mlist structrures returned is the start of that block.  This
 * allows a single free() call to be done on the return value of this routine.
 */
extern struct mlist *mlist(long fd, long offset, struct mach_header *mh,
			   struct load_command *lc, long *mlist_cnt);

/*
 * This is what an entry in the shared library initialization section is.
 * These entries are produced in their own section by the program mkshlib(l)
 * and when a program that uses something from the a shared library that needs
 * initialization then it forces loading the object file in the host shared
 * library object file which has the initialization tables for that entry which
 * then becomes part of the file that uses the shared library.
 */
struct shlib_init {
    long value;		/* the value to be stored at the address */
    long *address;	/* the address to store the value */
};
/*
 * These two symbols bound the shared library initialization section.  The first
 * is in the the shared library initialization section.  The second is the
 * following that section.  Currently these symbols are defined in initsyms.o
 * which is made by the program make_initsyms (the source is in this directory).
 * That object along with the object resulting from this file is linked with
 * crt0.o to form the final crt0.o .
 */
extern struct shlib_init _fvmlib_init0, _fvmlib_init1;

/*
 * _init_shlibs() does the shared library initialization.  It basicly runs over
 * the shared library initialization section and processes each entry.
 */
void
_init_shlibs()
{
    struct shlib_init *p;

	p = &(_fvmlib_init0);
	while(p < &(_fvmlib_init1)){
	    *(p->address) = p->value;
	    p++;
	}
}

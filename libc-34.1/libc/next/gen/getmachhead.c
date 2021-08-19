#include <stdlib.h>
#include <ldsyms.h>

/*
 * This routine returns an array of pointers to mach headers (the executable
 * and the shared libs) in the mach executable it is linked into.  It returns
 * zero if the malloc() it uses for the array fails.  The last entry in the
 * array is zero.  It uses the link editor defined symbol _mh_execute_header
 * and just looks through the load commands.
 */
struct mach_header **
getmachheaders()
{
	struct mach_header *mhp, **headers;
	struct fvmlib_command *flp;
	long i, n;

	mhp = (struct mach_header *)(&_mh_execute_header);
	flp = (struct fvmlib_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	n = 0;
	for(i = 0; i < mhp->ncmds; i++){
	    if(flp->cmd == LC_LOADFVMLIB)
		n++;
	    flp = (struct fvmlib_command *)((char *)flp + flp->cmdsize);
	}
	headers = (struct mach_header **)
		  malloc((n + 2) * sizeof(struct mach_header *));
	if(headers == (struct mach_header **)0)
	    return((struct mach_header **)0);

	headers[0] = (struct mach_header *)(&_mh_execute_header);
	mhp = (struct mach_header *)(&_mh_execute_header);
	flp = (struct fvmlib_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	n = 1;
	for(i = 0; i < mhp->ncmds; i++){
	    if(flp->cmd == LC_LOADFVMLIB)
		headers[n++] = (struct mach_header *)(flp->fvmlib.header_addr);
	    flp = (struct fvmlib_command *)((char *)flp + flp->cmdsize);
	}
	headers[n] = (struct mach_header *)0;

	return((struct mach_header **)headers);
}

#include <ldsyms.h>

/*
 * This routine returns the segment_command structure for the named segment if
 * it exist in the mach executible it is linked into.  Otherwise it returns
 * zero.  It uses the link editor defined symbol _mh_execute_header and just
 * looks through the load commands.  Since these are mapped into the text
 * segment they are read only and thus const.
 */
const struct segment_command *
getsegbyname(
    char *segname)
{
	struct mach_header *mhp;
	struct segment_command *sgp;
	long i;

	mhp = (struct mach_header *)(&_mh_execute_header);
	sgp = (struct segment_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	for(i = 0; i < mhp->ncmds; i++){
	    if(sgp->cmd == LC_SEGMENT)
		if(strncmp(sgp->segname, segname, sizeof(sgp->segname)) == 0)
		    return(sgp);
	    sgp = (struct segment_command *)((char *)sgp + sgp->cmdsize);
	}
	return((struct segment_command *)0);
}

#include <ldsyms.h>

extern struct section *getsectbyname(char *segname, char *sectname);

/*
 * This routine returns the highest address of the segments in the program (NOT
 * the shared libraries).  It is intended to be used as a stop gap for programs
 * that make UNIX style assumptions about how memory is allocated.  Typicly the
 * asumptions under which this is used is that memory is contiguously allocated
 * by the program's text and data from address 0 with no gaps.  The value of
 * this differs from the value of &_end in a UNIX program in that this routine
 * returns the address of the end of the segment not the end of the last section
 * in that segment as would be the value of the symbol &_end.
 */
unsigned long
get_end(void)
{
    struct mach_header *mhp;
    struct segment_command *sgp;
    unsigned long i, _end;

	_end = 0;
	mhp = (struct mach_header *)(&_mh_execute_header);
	sgp = (struct segment_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	for(i = 0; i < mhp->ncmds; i++){
	    if(sgp->cmd == LC_SEGMENT)
		if(sgp->vmaddr + sgp->vmsize > _end)
		    _end = sgp->vmaddr + sgp->vmsize;
	    sgp = (struct segment_command *)((char *)sgp + sgp->cmdsize);
	}
	return(_end);
}

/*
 * This returns what was the value of the UNIX link editor defined symbol
 * _etext (the first address after the text section).  Note this my or may not
 * be the only section in the __TEXT segment.
 */
unsigned long
get_etext(void)
{
    struct section *sp;

	sp = getsectbyname(SEG_TEXT, SECT_TEXT);
	if(sp)
	    return(sp->addr + sp->size);
	else
	    return(0);
}

/*
 * This returns what was the value of the UNIX link editor defined symbol
 * _edata (the first address after the data section).  Note this my or may not
 * be the last non-zero fill section in the __DATA segment.
 */
unsigned long
get_edata(void)
{
    struct section *sp;

	sp = getsectbyname(SEG_DATA, SECT_DATA);
	if(sp)
	    return(sp->addr + sp->size);
	else
	    return(0);
}

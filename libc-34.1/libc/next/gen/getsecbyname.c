#include <ldsyms.h>
#include <string.h>

/*
 * This routine returns the section structure for the named section in the
 * named segment for the mach_header pointer passed to it if it exist.
 * Otherwise it returns zero.
 */
const struct section *
getsectbynamefromheader(
    struct mach_header *mhp,
    char *segname,
    char *sectname)
{
	struct segment_command *sgp;
	struct section *sp;
	long i, j;

	sgp = (struct segment_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	for(i = 0; i < mhp->ncmds; i++){
	    if(sgp->cmd == LC_SEGMENT)
		if(strncmp(sgp->segname, segname, sizeof(sgp->segname)) == 0 ||
		   mhp->filetype == MH_OBJECT){
		    sp = (struct section *)((char *)sgp +
			 sizeof(struct segment_command));
		    for(j = 0; j < sgp->nsects; j++){
			if(strncmp(sp->sectname, sectname,
			   sizeof(sp->sectname)) == 0 &&
			   strncmp(sp->segname, segname,
			   sizeof(sp->segname)) == 0)
			    return(sp);
			sp = (struct section *)((char *)sp +
			     sizeof(struct section));
		    }
		}
	    sgp = (struct segment_command *)((char *)sgp + sgp->cmdsize);
	}
	return((struct section *)0);
}

/*
 * This routine returns the a pointer the section structure of the named
 * section in the named segment if it exist in the mach executable it is
 * linked into.  Otherwise it returns zero.
 */
const struct section *
getsectbyname(
    char *segname,
    char *sectname)
{
	return(getsectbynamefromheader(
		(struct mach_header *)&_mh_execute_header, segname, sectname));
}

/*
 * This routine returns the a pointer to the data for the named section in the
 * named segment if it exist in the mach executable it is linked into.  Also
 * it returns the size of the section data indirectly through the pointer size.
 * Otherwise it returns zero for the pointer and the size.
 */
char *
getsectdata(
    char *segname,
    char *sectname,
    int *size)
{
	const struct section *sp;

	sp = getsectbyname(segname, sectname);
	if(sp == (struct section *)0){
	    *size = 0;
	    return((char *)0);
	}
	*size = sp->size;
	return((char *)(sp->addr));
}

/*
 * This routine returns the a pointer to the data for the named section in the
 * named segment if it exist in the mach header passed to it.  Also it returns
 * the size of the section data indirectly through the pointer size.  Otherwise
 *  it returns zero for the pointer and the size.
 */
char *
getsectdatafromheader(
    struct mach_header *mhp,
    char *segname,
    char *sectname,
    int *size)
{
	const struct section *sp;

	sp = getsectbynamefromheader(mhp, segname, sectname);
	if(sp == (struct section *)0){
	    *size = 0;
	    return((char *)0);
	}
	*size = sp->size;
	return((char *)(sp->addr));
}

/*
 * This routine returns the a pointer to the data for the named section in the
 * named segment if it exist in the named shared library the executable it is
 * linked into.  Also it returns the size of the section data indirectly
 * through the pointer size.  Otherwise it returns zero for the pointer and
 * the size.  The last component of the shared libraries name must be of the
 * form libx.X.shlib.  Where the library name passed to this routine would
 * be libx and x is any string.  In libx.X.shlib, X is any single character.
 */
char *
getsectdatafromlib(
    char *libname,
    char *segname,
    char *sectname,
    int *size)
{
	struct mach_header *mhp;
	struct fvmlib_command *flp;
	const struct section *sp;
	long i, libnamelen, namelen;
	char *name, *p;

	libnamelen = strlen(libname);
	mhp = (struct mach_header *)(&_mh_execute_header);
	flp = (struct fvmlib_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	for(i = 0; i < mhp->ncmds; i++){
	    if(flp->cmd == LC_LOADFVMLIB){
		name = (char *)flp + flp->fvmlib.name.offset;
		p = strrchr(name, '/');
		if(p == NULL)
		    p = name;
		else
		    p++;
		namelen = strlen(p);
		if(namelen >= libnamelen + sizeof(".X.shlib") - 1 &&
		   strncmp(p, libname, libnamelen) == 0 &&
		   *(p + libnamelen) == '.' &&
		   strcmp(p + libnamelen + 2, ".shlib") == 0){
		    sp = getsectbynamefromheader(
				(struct mach_header *)flp->fvmlib.header_addr,
				segname, sectname);
		    if(sp == (struct section *)0){
			*size = 0;
			return((char *)0);
		    }
		    *size = sp->size;
		    return((char *)(sp->addr));
		}
	    }
	    flp = (struct fvmlib_command *)((char *)flp + flp->cmdsize);
	}

	*size = 0;
	return((char *)0);
}

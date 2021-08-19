#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/loader.h>
#define SECT_OBJC_RUNTIME "__runtime_setup"
#include <sys/types.h>
#include <sys/stat.h>
#include <mach.h>
#include <mach_error.h>
#include <libc.h>

/*
 * This really is the segment alignment of the input file and is an assumption
 * on what the segment alignment is since it can't exactly be determined from
 * the file itself.
 */
const long SEGALIGN = 0x2000;

/* This is set in the routine main() */
extern char *progname;

/* These variables are set in the routine map_input() */
static char *input_addr = NULL;	/* address of where the input file is mapped */
static long input_size = 0;	/* size of the input file */
static long input_mode = 0;	/* mode of the input file */
static struct mach_header
		*mhp = NULL;	/* pointer to the input file's mach header */
static struct load_command
	*load_commands = NULL;	/* pointer to the input file's load commands */

/*
 * Structures used in objc_runtime_setup in updating sections in the segments
 * of the input file.  There is one such structure for each segment and section.
 */
struct rep_seg {
    long fileoff;		/* original file offset */
    long filesize;		/* original file size */
    long vmsize;		/* original vm size */
    long padsize;		/* new pad size */
    struct segment_command *sgp;/* pointer to the segment_command */
};
static struct rep_seg *segs = NULL;

struct rep_sect {
    long offset;		/* original file offset */
    struct section *sp;		/* pointer to the section structure */
};
static struct rep_sect *sects = NULL;

/* Internal routines */
static long map_input(
    char *input);
static long objc_runtime_setup(
    char *input,
    char *output,
    char *objc_runtime_setup_contents,
    unsigned long objc_runtime_setup_size);
static int qsort_vmaddr(
    const struct rep_seg *seg1,
    const struct rep_seg *seg2);
static int qsort_fileoff(
    const struct rep_seg *seg1,
    const struct rep_seg *seg2);
static int qsort_offset(
    const struct rep_sect *sect1,
    const struct rep_sect *sect2);
static void *allocate(
    long size);
static void error(
    const char *format,
    ...);
static void fatal(
    const char *format,
    ...);
static void system_error(
    const char *format,
    ...);
static void system_fatal(
    const char *format,
    ...);
static void machkern_error(
    kern_return_t r,
    char *format,
    ...);
static long round(
    long v,
    unsigned long r);

/*
 * add_objc_runtime_setup is passed a name of an object file and pointer to the
 * contents of the objc_runtime_setup section and it's size and creates an
 * output file with that section added or releaced.  It returns 1 if successfull
 * and zero otherwize.
 */
long
add_objc_runtime_setup(
char *input,
char *output,
char *contents,
long size)
{
	/* map in the input */
	if(map_input(input) == 0)
	    return(0);

	/* create the output with the contents */
	if(objc_runtime_setup(input, output, contents, size) == 0)
	    return(0);

	return(1);
}

/*
 * map_input maps the input file into memory.  The address it is mapped at is
 * left in input_addr and the size is left in input_size.  The input file is
 * checked to be an object file and that the headers are checked to be correct
 * enough to loop through them.  The pointer to the mach header is left in mhp
 * and the pointer to the load commands is left in load_commands.  It returns
 * 1 if it is successfull and zero otherwise.
 */
static
long
map_input(
char *input)
{
    int fd;
    long i;
    struct stat stat_buf;
    kern_return_t r;
    struct load_command *lcp;

	/* Open the input file and map it in */
	if((fd = open(input, O_RDONLY)) == -1){
	    system_error("can't open input file: %s", input);
	    return(0);
	}
	if(fstat(fd, &stat_buf) == -1){
	    system_error("Can't stat input file: %s", input);
	    close(fd);
	    return(0);
	}
	input_size = stat_buf.st_size;
	input_mode = stat_buf.st_mode;
	if((r = map_fd(fd, 0, &input_addr, TRUE, input_size)) != KERN_SUCCESS){
	    machkern_error(r, "Can't map input file: %s", input);
	    close(fd);
	    return(0);
	}
	close(fd);

	if(sizeof(struct mach_header) > input_size){
	    error("truncated or malformed object (mach header would extend "
		  "past the end of the file) in: %s", input);
	    goto map_input_cleanup;
	}
	mhp = (struct mach_header *)input_addr;
	if(mhp->sizeofcmds + sizeof(struct mach_header) > input_size){
	    error("truncated or malformed object (load commands would extend "
		  "past the end of the file) in: %s", input);
	    goto map_input_cleanup;
	}
	load_commands = (struct load_command *)((char *)input_addr +
			    sizeof(struct mach_header));
	lcp = load_commands;
	for(i = 0; i < mhp->ncmds; i++){
	    if(lcp->cmdsize % sizeof(long) != 0){
		error("load command %d size not a multiple of sizeof(long) "
		      "in: %s", i, input);
		goto map_input_cleanup;
	    }
	    if(lcp->cmdsize <= 0){
		error("load command %d size is less than or equal to zero "
		      "in: %s", i, input);
		goto map_input_cleanup;
	    }
	    if((char *)lcp + lcp->cmdsize >
	       (char *)load_commands + mhp->sizeofcmds){
		error("load command %d extends past end of all load commands "
		      "in: %s", i, input);
		goto map_input_cleanup;
	    }
	    lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	}
	return(1);

map_input_cleanup:
	/*
	 * Cleanup if there were errors in map_input().
	 */
	if(input_addr != NULL){
	    if((r = vm_deallocate(task_self(), (vm_address_t)input_addr,
				  input_size)) != KERN_SUCCESS)
		machkern_error(r,"Can't deallocate input file's mapped memory");
	}
	return(0);
}

/*
 * objc_runtime_setup writes a modified version of the input file to output
 * adding or replacing the (__OBJC,__runtime_setup) section with the contents
 * passed to it.
 */
static
long
objc_runtime_setup(
char *input,
char *output,
char *objc_runtime_setup_contents,
unsigned long objc_runtime_setup_size)
{
    long i, j, k, nsegs, nsects;
    long high_reloc_seg, low_noreloc_seg, high_noreloc_seg, low_linkedit;
    long oldvmaddr, oldoffset, newvmaddr, newoffset, newsectsize;
    struct mach_header *new_mhp;
    struct load_command *lcp, *new_lcp, *new_load_commands;
    struct segment_command *sgp, *linkedit_sgp, *objc_sgp;
    struct section *sp, *first_sp, *objc_runtime_setup_sp, *last_objc_sp;
    struct symtab_command *stp;
    struct symseg_command *ssp;
    int outfd;
    vm_address_t pad_addr;
    long size;
    kern_return_t r;

	segs = NULL;
	new_mhp = NULL;
	sects = NULL;
	pad_addr = 0;
	outfd = -1;

	high_reloc_seg = 0;
	low_noreloc_seg = input_size;
	high_noreloc_seg = 0;
	low_linkedit = input_size;

	nsegs = 0;
	segs = allocate(mhp->ncmds * sizeof(struct rep_seg));
	bzero(segs, mhp->ncmds * sizeof(struct rep_seg));
	nsects = 0;

	stp = NULL;
	ssp = NULL;
	linkedit_sgp = NULL;
	first_sp = NULL;
	objc_sgp = NULL;
	objc_runtime_setup_sp = NULL;

	/*
	 * First pass over the load commands and determine if the file is laided
	 * out in an order that the new section can be replaced or added.
	 */
	lcp = load_commands;
	for(i = 0; i < mhp->ncmds; i++){
	    switch(lcp->cmd){
	    case LC_SEGMENT:
		sgp = (struct segment_command *)lcp;
		sp = (struct section *)((char *)sgp +
					sizeof(struct segment_command));
		segs[nsegs++].sgp = sgp;
		nsects += sgp->nsects;
		if(strcmp(sgp->segname, SEG_LINKEDIT) != 0){
		    if(strncmp(SEG_OBJC, sgp->segname,
			       sizeof(sgp->segname)) == 0){
			if(objc_sgp != NULL){
			    error("more than one " SEG_OBJC " segment found "
				  "in: %s", input);
			    goto objc_runtime_setup_cleanup;
			}
			objc_sgp = sgp;
		    }
		    if(sgp->flags & SG_NORELOC){
			if(sgp->filesize != 0){
			    if(sgp->fileoff + sgp->filesize > high_noreloc_seg)
				high_noreloc_seg = sgp->fileoff + sgp->filesize;
			    if(sgp->fileoff < low_noreloc_seg)
				low_noreloc_seg = sgp->fileoff;
			}
		    }
		    else{
			if(sgp->filesize != 0 &&
			   sgp->fileoff + sgp->filesize > high_reloc_seg)
			    high_reloc_seg = sgp->fileoff + sgp->filesize;
		    }
		}
		else{
		    if(linkedit_sgp != NULL){
			error("more than one " SEG_LINKEDIT " segment found "
			      "in: %s", input);
			goto objc_runtime_setup_cleanup;
		    }
		    linkedit_sgp = sgp;
		}
		for(j = 0; j < sgp->nsects; j++){
		    if(sp->nreloc != 0 && sp->reloff < low_linkedit)
			low_linkedit = sp->reloff;
		    if(first_sp == NULL)
			first_sp = sp;
		    if(strncmp(SEG_OBJC, sp->segname,
			       sizeof(sp->segname)) == 0 &&
		       strncmp(SECT_OBJC_RUNTIME, sp->sectname,
			       sizeof(sp->sectname)) == 0){
			if(objc_runtime_setup_sp != NULL){
			    error("more than one (%0.16s,%0.16s) section in: "
				  "%s", sp->segname, sp->sectname, input);
			    goto objc_runtime_setup_cleanup;
			}
			objc_runtime_setup_sp = sp;
			if(sp->flags & S_ZEROFILL){
			    error("(%0.16s,%0.16s) is a fill section in: %s",
				  sp->segname, sp->sectname, input);
			    goto objc_runtime_setup_cleanup;
			}
			if(j != sgp->nsects - 1){
			    error("(%0.16s,%0.16s) is not the last section in "
				  "the " SEG_OBJC " segment in: %s",
				  sp->segname, sp->sectname, input);
			    goto objc_runtime_setup_cleanup;
			}
			if(sp->offset + sp->size > input_size)
			    error("truncated or malformed object (section "
				  "contents of (%0.16s,%0.16s) extends "
				  "past the end of the file) in: %s",
				  sp->segname, sp->sectname, input);
		    }
		    sp++;
		}
		break;
	    case LC_SYMTAB:
		if(stp != NULL)
		    fatal("more than one symtab_command found in: %s", input);
		stp = (struct symtab_command *)lcp;
		if(stp->nsyms != 0 && stp->symoff < low_linkedit)
		    low_linkedit = stp->symoff;
		if(stp->strsize != 0 && stp->stroff < low_linkedit)
		    low_linkedit = stp->stroff;
		break;
	    case LC_SYMSEG:
		if(ssp != NULL)
		    fatal("more than one symseg_command found in: %s", input);
		ssp = (struct symseg_command *)lcp;
		if(ssp->size != 0 && ssp->offset < low_linkedit)
		    low_linkedit = ssp->offset;
		break;
	    case LC_THREAD:
	    case LC_UNIXTHREAD:
	    case LC_LOADFVMLIB:
	    case LC_IDFVMLIB:
	    case LC_IDENT:
		break;
	    default:
		error("unknown load command %d in: %s (result maybe bad)", i,
		      input);
		break;
	    }
	    lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	}

	if(objc_sgp == NULL){
	    error("file: %s does not contain an " SEG_OBJC " segment", input);
	    goto objc_runtime_setup_cleanup;
	}
	if(objc_runtime_setup_sp == NULL && 
	   first_sp->offset - (sizeof(struct mach_header) + mhp->sizeofcmds) <
	   sizeof(struct section)){
	    error("file: %s does not have enough header padding to add a "
		  "section", input);
	    goto objc_runtime_setup_cleanup;
	}
	if(high_reloc_seg > low_noreloc_seg ||
	   high_reloc_seg > low_linkedit ||
	   high_noreloc_seg > low_linkedit){
	    error("contents of input file: %s not in an order that the "
		  "new section can be added or replaced by this program",
		  input);
	    goto objc_runtime_setup_cleanup;
	}
	if(low_noreloc_seg != input_size &&
	   low_noreloc_seg != objc_sgp->fileoff + objc_sgp->filesize){
	    error("file's: %s " SEG_OBJC " segment is not the last segment "
		  "before the segments requiring no relocation", input);
	    goto objc_runtime_setup_cleanup;
	}

	/*
	 * If the section must be added rather than just updated then rebuild
	 * the headers with the new section in it.  And reset the pointers into
	 * the load commands to point to the newly allocated load commands.
	 */
	if(objc_runtime_setup_sp == NULL){
	    new_mhp = (struct mach_header *)allocate(
			sizeof(struct mach_header) + mhp->sizeofcmds +
			sizeof(struct section));
	    *new_mhp = *mhp;
	    new_mhp->sizeofcmds += sizeof(struct section);
	    new_load_commands = (struct load_command *)((char *)new_mhp +
				sizeof(struct mach_header));
	    lcp = load_commands;
	    new_lcp = new_load_commands;
	    nsegs = 0;

	    for(i = 0; i < mhp->ncmds; i++){
		memcpy((char *)new_lcp, (char *)lcp, lcp->cmdsize);
	    	if(new_lcp->cmd == LC_SEGMENT){
		    segs[nsegs++].sgp = (struct segment_command *)new_lcp;
		    if((struct segment_command *)lcp == linkedit_sgp)
			linkedit_sgp = (struct segment_command *)new_lcp;
		}
		if((struct symtab_command *)lcp == stp)
		    stp = (struct symtab_command *)new_lcp;
		if((struct symseg_command *)lcp == ssp)
		    ssp = (struct symseg_command *)new_lcp;
		if((struct segment_command *)lcp == objc_sgp){
		    objc_sgp = (struct segment_command *)new_lcp;
		    objc_runtime_setup_sp =
				   (struct section *)((char *)objc_sgp +
				   sizeof(struct segment_command) +
				   objc_sgp->nsects * sizeof(struct section));
		    last_objc_sp = (struct section *)((char *)objc_sgp +
				    sizeof(struct segment_command) +
				    (objc_sgp->nsects - 1) *
				    sizeof(struct section));
		    objc_sgp->cmdsize += sizeof(struct section);
		    objc_sgp->nsects++;
		    nsects++;
		    memset(objc_runtime_setup_sp, '\0', sizeof(struct section));
		    strcpy(objc_runtime_setup_sp->sectname, SECT_OBJC_RUNTIME);
		    strcpy(objc_runtime_setup_sp->segname, SEG_OBJC);
		    objc_runtime_setup_sp->addr = round(last_objc_sp->addr +
							last_objc_sp->size,
							sizeof(long));
		    objc_runtime_setup_sp->size = 0;
		    objc_runtime_setup_sp->offset = last_objc_sp->offset +
						    objc_runtime_setup_sp->addr-
						    last_objc_sp->addr;
		    objc_runtime_setup_sp->align = 2;
		    objc_runtime_setup_sp->reloff = 0;
		    objc_runtime_setup_sp->nreloc = 0;
		    objc_runtime_setup_sp->flags = 0;
		}
		new_lcp = (struct load_command *)((char *)new_lcp +
						 new_lcp->cmdsize);
		lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	    }
	    mhp = new_mhp;
	    load_commands = new_load_commands;
	}

	qsort(segs, nsegs, sizeof(struct rep_seg),
	      (int (*)(const void *, const void *))qsort_vmaddr);

	sects = allocate(nsects * sizeof(struct rep_sect));
	bzero(sects, nsects * sizeof(struct rep_sect));

	/*
	 * First go through the segments and adjust the segment offsets, sizes
	 * and addresses without adjusting the offset to the relocation entries.
	 * This program can only handle object files that have contigious
	 * address spaces starting at zero and that the offsets in the file for
	 * the contents of the segments also being contiguious and in the same
	 * order as the vmaddresses.
	 */
	oldvmaddr = 0;
	newvmaddr = 0;
	k = 0;
	for(i = 0; i < nsegs; i++){
	    if(segs[i].sgp->vmaddr != oldvmaddr){
		newvmaddr = segs[i].sgp->vmaddr;
		oldvmaddr = newvmaddr;
	    }
	    segs[i].filesize = segs[i].sgp->filesize;
	    segs[i].vmsize = segs[i].sgp->vmsize;
	    segs[i].sgp->vmaddr = newvmaddr;
	    sp = (struct section *)((char *)(segs[i].sgp) +
				    sizeof(struct segment_command));
	    if(segs[i].sgp->filesize != 0){
		newsectsize = 0;
		if(segs[i].sgp == objc_sgp || segs[i].sgp->flags == SG_NORELOC){
		    for(j = 0; j < segs[i].sgp->nsects; j++){

			/* begin bug fix - snaroff (8/3/90) */
			if ((sp->offset == 0) && (sp->flags != S_ZEROFILL)) 
		          sp->offset = (sp+1)->offset;
			/* end bug fix - snaroff (8/3/90) */

			sects[k + j].sp = sp;
			if(sp == objc_runtime_setup_sp)
			    sp->size = objc_runtime_setup_size;
			sp->addr += newvmaddr - oldvmaddr;
			newsectsize = (sp->addr - newvmaddr) + sp->size;
			sp++;
		    }
		    if(strcmp(segs[i].sgp->segname, SEG_LINKEDIT) != 0){
			segs[i].sgp->filesize = round(newsectsize, SEGALIGN);
			segs[i].sgp->vmsize = round(newsectsize, SEGALIGN);
			segs[i].padsize = segs[i].sgp->filesize  - newsectsize;
		    }
		}
		else{
		    for(j = 0; j < segs[i].sgp->nsects; j++)
			sects[k + j].sp = sp;
		}
	    }
	    else{
		for(j = 0; j < segs[i].sgp->nsects; j++)
		    sects[k + j].sp = sp;
	    }
	    oldvmaddr += segs[i].vmsize;
	    newvmaddr += segs[i].sgp->vmsize;
	    k += segs[i].sgp->nsects;
	}

	qsort(segs, nsegs, sizeof(struct rep_seg),
	      (int (*)(const void *, const void *))qsort_fileoff);
	qsort(sects, nsects, sizeof(struct rep_sect),
	      (int (*)(const void *, const void *))qsort_offset);

	oldoffset = 0;
	newoffset = 0;
	k = 0;
	for(i = 0; i < nsegs; i++){
	    if(segs[i].sgp->filesize != 0){
		if(segs[i].sgp->fileoff != oldoffset){
		    oldoffset = segs[i].sgp->fileoff;
		    newoffset = oldoffset;
		}
		segs[i].fileoff = segs[i].sgp->fileoff;
		if(strcmp(segs[i].sgp->segname, SEG_LINKEDIT) != 0)
		    segs[i].sgp->fileoff = newoffset;
		sp = (struct section *)((char *)(segs[i].sgp) +
					sizeof(struct segment_command));
		if(segs[i].sgp == objc_sgp || segs[i].sgp->flags == SG_NORELOC){
		    for(j = 0; j < segs[i].sgp->nsects; j++){
			sects[k + j].offset = sp->offset;
			sp->offset += newoffset - oldoffset;
			sp++;
		    }
		}
		/*
		 * If it is not the LINKEDIT segment or the last segment then
		 * move up the offsets.
		 */
		if(strcmp(segs[i].sgp->segname, SEG_LINKEDIT) != 0 ||
		   i != nsegs - 1){
		    oldoffset += segs[i].filesize;
		    newoffset += segs[i].sgp->filesize;
		}
	    }
	    k += segs[i].sgp->nsects;
	}

	/*
	 * Now update the offsets to the linkedit information.
	 */
	if(oldoffset != low_linkedit){
	    error("contents of input file: %s not in an order that the "
		  "specified sections can be replaced by this program", input);
	    goto objc_runtime_setup_cleanup;
	}
	for(i = 0; i < nsegs; i++){
	    sp = (struct section *)((char *)(segs[i].sgp) +
				    sizeof(struct segment_command));
	    for(j = 0; j < segs[i].sgp->nsects; j++){
		if(sp->nreloc != 0)
		    sp->reloff += newoffset - oldoffset;
		sp++;
	    }
	}
	if(stp != NULL){
	    if(stp->nsyms != 0);
		stp->symoff += newoffset - oldoffset;
	    if(stp->strsize != 0)
		stp->stroff += newoffset - oldoffset;
	}
	if(ssp != NULL){
	    if(ssp->size != 0)
		ssp->offset += newoffset - oldoffset;
	}
	if(linkedit_sgp != NULL){
	    linkedit_sgp->fileoff += newoffset - oldoffset;
	}

	/*
	 * Now write the new file by writing the header and modified load
	 * commands, then the segments with any new sections and finally
	 * the link edit info.
	 */
	if((outfd = open(output, O_CREAT | O_WRONLY | O_TRUNC ,input_mode)) ==
	   								    -1){
	    system_error("can't create output file: %s", output);
	    goto objc_runtime_setup_cleanup;
	}

	if(r = vm_allocate(task_self(), &pad_addr, SEGALIGN, 1) !=
	   							  KERN_SUCCESS){
	    machkern_error(r, "vm_allocate() failed");
	    goto objc_runtime_setup_cleanup;
	}

	k = 0;
	for(i = 0; i < nsegs; i++){
	    if(segs[i].sgp == objc_sgp){
		for(j = 0; j < segs[i].sgp->nsects; j++){
		    sp = sects[k + j].sp;
		    if(sp == objc_runtime_setup_sp){
			lseek(outfd, sp->offset, L_SET);
			if(write(outfd, objc_runtime_setup_contents,
				 objc_runtime_setup_size) !=
						       objc_runtime_setup_size){
			    system_error("can't write section contents for "
					 "section (%s,%s) to output file: %s", 
					 sp->segname, sp->sectname, output);
			    goto objc_runtime_setup_cleanup;
			}
		    }
		    else{
			/* write the original section */
			if(sects[k + j].offset + sp->size > input_size){
			    error("truncated or malformed object file: %s "
				  "(section (%0.16s,%0.16s) extends past the "
				  "end of the file)",input, sp->segname,
				  sp->sectname);
			    goto objc_runtime_setup_cleanup;
			}
			lseek(outfd, sp->offset, L_SET);
		 
			if(write(outfd,(char *)input_addr + sects[k + j].offset,
			   sp->size) != sp->size){
			    system_fatal("can't write section contents for "
					 "section (%s,%s) to output file: %s", 
					 sp->segname, sp->sectname, output);
			    goto objc_runtime_setup_cleanup;
			}
		    }
		}
		/* write the segment padding */
		if(write(outfd, (char *)pad_addr, segs[i].padsize) !=
			 segs[i].padsize){
		    system_error("can't write segment padding for segment %s to"
				 " output file: %s", segs[i].sgp->segname,
				 output);
		    goto objc_runtime_setup_cleanup;
		}
	    }
	    else{
		/* write the original segment */
		if(strcmp(segs[i].sgp->segname, SEG_LINKEDIT) != 0 ||
		   i != nsegs - 1){
		    if(segs[i].fileoff + segs[i].sgp->filesize > input_size){
			error("truncated or malformed object file: %s "
			      "(segment: %s extends past the end of "
			      "the file)", input, segs[i].sgp->segname);
			goto objc_runtime_setup_cleanup;
		    }
		    if(write(outfd, (char *)input_addr + segs[i].fileoff,
		       segs[i].sgp->filesize) != segs[i].sgp->filesize){
			system_error("can't write segment contents for "
				     "segment: %s to output file: %s", 
				     segs[i].sgp->segname, output);
			goto objc_runtime_setup_cleanup;
		    }
		}
	    }
	    k += segs[i].sgp->nsects;
	}
	/* write the linkedit info */
	size = input_size - low_linkedit;
	if(write(outfd, (char *)input_addr + low_linkedit, size) != size){
	    system_error("can't write link edit information to output file: %s",
			 output);
	    goto objc_runtime_setup_cleanup;
	}
	lseek(outfd, 0, L_SET);
	size = sizeof(struct mach_header) + mhp->sizeofcmds;
	if(write(outfd, mhp, size) != size){
	    system_error("can't write headers to output file: %s", output);
	    goto objc_runtime_setup_cleanup;
	}

	if(close(outfd) == -1){
	    system_error("can't close output file: %s", output);
	    outfd = -1;
	    goto objc_runtime_setup_cleanup;
	}

	if(segs != NULL)
	    free(segs);
	if(sects != NULL)
	    free(sects);
	if(new_mhp != NULL)
	    free(new_mhp);
	if(pad_addr != 0){
	    if((r = vm_deallocate(task_self(), (vm_address_t)pad_addr,
				  SEGALIGN)) != KERN_SUCCESS){
		machkern_error(r, "Can't deallocate segment pad buffer");
		return(0);
	    }
	}
	return(1);

objc_runtime_setup_cleanup:
	/*
	 * Cleanup if there were errors.
	 */
	if(outfd != -1)
	    close(outfd);
	if(segs != NULL)
	    free(segs);
	if(sects != NULL)
	    free(sects);
	if(new_mhp != NULL)
	    free(new_mhp);
	if(pad_addr != 0){
	    if((r = vm_deallocate(task_self(), (vm_address_t)pad_addr,
				  SEGALIGN)) != KERN_SUCCESS){
		machkern_error(r, "Can't deallocate segment pad buffer");
	    }
	}
	return(0);
}

/*
 * Function for qsort for comparing segments vmaddr feilds
 */
static
int
qsort_vmaddr(
const struct rep_seg *seg1,
const struct rep_seg *seg2)
{
	return((long)(seg1->sgp->vmaddr) - (long)(seg2->sgp->vmaddr));
}

/*
 * Function for qsort for comparing segments fileoff fields
 */
static
int
qsort_fileoff(
const struct rep_seg *seg1,
const struct rep_seg *seg2)
{
	return((long)(seg1->sgp->fileoff) - (long)(seg2->sgp->fileoff));
}

/*
 * Function for qsort for comparing sections offset fields
 */
static
int
qsort_offset(
const struct rep_sect *sect1,
const struct rep_sect *sect2)
{
	return((long)(sect1->sp->offset) - (long)(sect2->sp->offset));
}

/*
 * allocate is just a wrapper around malloc that prints and error message and
 * exits if the malloc fails.
 */
static
void *
allocate(
long size)
{
    void *p;

	if((p = (void *)malloc(size)) == (char *)0)
	    system_fatal("virtual memory exhausted (malloc failed)");
	return(p);
}

/*
 * Print the error message and set the non-fatal error indication.
 */
static
void
error(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
	va_end(ap);
}

/*
 * Print the fatal error message, and exit non-zero.
 */
static
void
fatal(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

/*
 * Print the error message along with the system error message, set the
 * non-fatal error indication.
 */
static
void
system_error(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, " (%s)\n", strerror(errno));
	va_end(ap);
}

/*
 * Print the fatal message along with the system error message, and exit
 * non-zero.
 */
static
void
system_fatal(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, " (%s)\n", strerror(errno));
	va_end(ap);
	exit(1);
}

/*
 * Print the error message along with the mach error string.
 */
static
void
machkern_error(
kern_return_t r,
char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, " (%s)\n", mach_error_string(r));
	va_end(ap);
}

/*
 * Round v to a multiple of r.
 */
//static
long
round(
long v,
unsigned long r)
{
	r--;
	v += r;
	v &= ~(long)r;
	return(v);
}

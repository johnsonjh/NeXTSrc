/*
 * The segedit(1) program.  This program extracts and replaces sections from
 * an object file.  Only sections in segments that have been marked that they
 * have no relocation can be replaced (SG_NORELOC).  This program takes the
 * following options:
 *   -extract <segname> <sectname> <filename>
 *   -replace <segname> <sectname> <filename>
 *   -output <filename>
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/loader.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mach.h>

/* These variables are set from the command line arguments */
static char *progname;	/* name of the program for error messages (argv[0]) */

static char *input,	/* object file to extract/replace sections from */
     	    *output;	/* new object file with replaced sections, if any
			   -replace options */

/* structure for holding -extract's arguments */
struct extract {
    char *segname;		/* segment name */
    char *sectname;		/* section name */
    char *filename;		/* file to put the section contents in */
    long found;			/* set when the section is found */
    struct extract *next;	/* next extract structure, NULL if last */
} *extracts;			/* first extract structure, NULL if none */

/* structure for holding -replace's arguments */
struct replace {
    char *segname;		/* segment name */
    char *sectname;		/* section name */
    char *filename;		/* file to get the section contents from */
    long found;			/* set when the section is found */
    long size;			/* size of new section contents */
    struct replace *next;	/* next replace structure, NULL if last */
} *replaces;			/* first replace structure, NULL if none */

/*
 * Structures used in replace_sections in replaceing sections in the segments
 * of the input file.  There is one such structure for each segment and section.
 */
struct rep_seg {
    long modified;		/* this segment has a replaced section */
    long fileoff;		/* original file offset */
    long filesize;		/* original file size */
    long vmsize;		/* original vm size */
    long padsize;		/* new pad size */
    struct segment_command *sgp;/* pointer to the segment_command */
} *segs;

struct rep_sect {
    struct replace *rp;		/* pointer to the replace structure */
    long offset;		/* original file offset */
    struct section *sp;		/* pointer to the section structure */
} *sects;

/* These variables are set in the routine map_input() */
static void *input_addr;	/* address of where the input file is mapped */
static long input_size;		/* size of the input file */
static long input_mode;		/* mode of the input file */
static struct mach_header *mhp;	/* pointer to the input file's mach header */
static struct load_command
		*load_commands;	/* pointer to the input file's load commands */

/* Internal routines */
static void map_input();
static void extract_sections();
static void replace_sections();
static int cmp_qsort(const struct rep_seg *seg1,
		     const struct rep_seg *seg2);
static void *allocate(long size);
static void usage();
static void error(const char *format, ...);
static void fatal(const char *format, ...);
static void system_error(const char *format, ...);
static void system_fatal(const char *format, ...);
static void mach_fatal(kern_return_t r, char *format, ...);
static long round(long v, unsigned long r);

void
main(
int argc,
char *argv[],
char *envp[])
{
    int i;
    struct extract *ep;
    struct replace *rp;

	progname = argv[0];

	for (i = 1; i < argc; i++) {
	    if(argv[i][0] == '-'){
		switch(argv[i][1]){
		case 'e':
		    if(i + 4 > argc){
			error("missing arguments to %s option", argv[i]);
			usage();
		    }
		    ep = allocate(sizeof(struct extract));
		    ep->segname =  argv[i + 1];
		    ep->sectname = argv[i + 2];
		    ep->filename = argv[i + 3];
		    ep->found = 0;
		    ep->next = extracts;
		    extracts = ep;
		    i += 3;
		    break;
		case 'r':
		    if(i + 4 > argc){
			error("missing arguments to %s option", argv[i]);
			usage();
		    }
		    rp = allocate(sizeof(struct replace));
		    rp->segname =  argv[i + 1];
		    rp->sectname = argv[i + 2];
		    rp->filename = argv[i + 3];
		    rp->next = replaces;
		    replaces = rp;
		    i += 3;
		    break;
		case 'o':
		    if(output != NULL)
			fatal("more than one %s option", argv[i]);
		    output = argv[i + 1];
		    i += 1;
		    break;
		default:
		    error("unrecognized option: %s", argv[i]);
		    usage();
		}
	    }
	    else{
		if(input != NULL){
		    fatal("only one input file can be specified");
		    usage();
		}
		input = argv[i];
	    }
	}

	if(input == NULL){
	    error("no input file specified");
	    usage();
	}
	if(replaces != NULL && output == NULL)
	    fatal("output file must be specified via -o <filename> when "
		  "replacing a section");

	if(extracts == NULL && replaces == NULL){
	    error("no -extract or -replace options specified");
	    usage();
	}

	map_input();

	if(extracts != NULL)
	    extract_sections();

	if(replaces != NULL)
	    replace_sections();

	exit(0);
}

/*
 * map_input maps the input file into memory.  The address it is mapped at is
 * left in input_addr and the size is left in input_size.  The input file is
 * checked to be an object file and that the headers are checked to be correct
 * enough to loop through them.  The pointer to the mach header is left in mhp
 * and the pointer to the load commands is left in load_commands.
 */
static
void
map_input()
{
    int fd;
    long i;
    struct stat stat_buf;
    kern_return_t r;
    struct load_command *lcp;

	/* Open the input file and map it in */
	if((fd = open(input, O_RDONLY)) == -1)
	    system_fatal("can't open input file: %s", input);
	if(fstat(fd, &stat_buf) == -1)
	    system_fatal("Can't stat input file: %s", input);
	input_size = stat_buf.st_size;
	input_mode = stat_buf.st_mode;
	if((r = map_fd(fd, 0, &input_addr, TRUE, input_size)) != KERN_SUCCESS)
	    mach_fatal(r, "Can't map input file: %s", input);
	close(fd);

	if(sizeof(struct mach_header) > input_size)
	    fatal("truncated or malformed object (mach header would extend "
		  "past the end of the file) in: %s", input);
	mhp = (struct mach_header *)input_addr;
	if(mhp->sizeofcmds + sizeof(struct mach_header) > input_size)
	    fatal("truncated or malformed object (load commands would extend "
		  "past the end of the file) in: %s", input);
	load_commands = (struct load_command *)((char *)input_addr +
			    sizeof(struct mach_header));
	lcp = load_commands;
	for(i = 0; i < mhp->ncmds; i++){
	    if(lcp->cmdsize % sizeof(long) != 0)
		error("load command %d size not a multiple of sizeof(long) "
		      "in: %s", i, input);
	    if(lcp->cmdsize <= 0)
		fatal("load command %d size is less than or equal to zero "
		      "in: %s", i, input);
	    if((char *)lcp + lcp->cmdsize >
	       (char *)load_commands + mhp->sizeofcmds)
		fatal("load command %d extends past end of all load commands "
		      "in: %s", i, input);
	    lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	}
}

/*
 * This routine extracts the sections in the extracts list from the input file
 * and writes then to the file specified in the list.
 */
static
void
extract_sections()
{
    long i, j, errors;
    struct load_command *lcp;
    struct segment_command *sgp;
    struct section *sp;
    struct extract *ep;
    int fd;

	lcp = load_commands;
	for(i = 0; i < mhp->ncmds; i++){
	    if(lcp->cmd == LC_SEGMENT){
		sgp = (struct segment_command *)lcp;
		sp = (struct section *)((char *)sgp +
					sizeof(struct segment_command));
		for(j = 0; j < sgp->nsects; j++){
		    ep = extracts;
		    while(ep != NULL){
			if(ep->found == 0 &&
			   strncmp(ep->segname, sp->segname,
				   sizeof(sp->segname)) == 0 &&
			   strncmp(ep->sectname, sp->sectname,
				   sizeof(sp->sectname)) == 0){
			    if(sp->flags & S_ZEROFILL)
				fatal("meaningless to extract zero fill "
				      "section (%s,%s) in: %s", sp->segname,
				      sp->sectname, input);
			    if(sp->offset + sp->size > input_size)
				fatal("truncated or malformed object (section "
				      "contents of (%s,%s) extends past the "
				      "end of the file) in: %s", sp->segname,
				      sp->sectname, input);
			     if((fd = open(ep->filename, O_WRONLY | O_CREAT |
					   O_TRUNC, 0666)) == -1)
				system_fatal("can't create: %s", ep->filename);
			     if(write(fd, (char *)input_addr + sp->offset,
				     sp->size) != sp->size)
				system_fatal("can't write: %s", ep->filename);
			     if(close(fd) == -1)
				system_fatal("can't close: %s", ep->filename);
			     ep->found = 1;
			}
			ep = ep->next;
		    }
		    sp++;
		}
	    }
	    lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	}

	errors = 0;
	ep = extracts;
	while(ep != NULL){
	    if(ep->found == 0){
		error("section (%s,%s) not found in: %s", ep->segname,
		      ep->sectname, input);
		errors = 1;
	    }
	    ep = ep->next;
	}
	if(errors != 0)
	    exit(1);
}

static
void
replace_sections()
{
    long i, j, k, l, errors, nsegs, nsects;
    long high_reloc_seg, low_noreloc_seg, high_noreloc_seg, low_linkedit;
    long oldvmaddr, oldoffset, newvmaddr, newoffset, oldsectsize, newsectsize;
    struct load_command *lcp;
    struct segment_command *sgp, *linkedit_sgp;
    struct section *sp;
    struct symtab_command *stp;
    struct symseg_command *ssp;
    struct replace *rp;
    struct stat stat_buf;
    int outfd, sectfd;
    vm_address_t sect_addr, pad_addr;
    long size;
    kern_return_t r;

	errors = 0;

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

	/*
	 * First pass over the load commands and determine if the file is laided
	 * out in an order that the specified sections can be replaced.  Also
	 * determine if the specified sections exist in the input file and if
	 * it is marked with no relocation so it can be replaced. 
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
		    if(linkedit_sgp != NULL)
			fatal("more than one " SEG_LINKEDIT " segment found "
			      "in: %s", input);
		    linkedit_sgp = sgp;
		}
		for(j = 0; j < sgp->nsects; j++){
		    if(sp->nreloc != 0 && sp->reloff < low_linkedit)
			low_linkedit = sp->reloff;
		    rp = replaces;
		    while(rp != NULL){
			if(rp->found == 0 &&
			   strncmp(rp->segname, sp->segname,
				   sizeof(sp->segname)) == 0 &&
			   strncmp(rp->sectname, sp->sectname,
				   sizeof(sp->sectname)) == 0){
			    if(sp->flags & S_ZEROFILL){
				error("can't replace zero fill section (%0.16s,"
				      "%0.16s) in: %s", sp->segname,
				      sp->sectname, input);
				errors = 1;
			    }
			    if((sgp->flags & SG_NORELOC) == 0){
				error("can't replace section (%0.16s,%0.16s) "
				      "in: %s because it requires relocation",
				      sp->segname, sp->sectname, input);
				errors = 1;
			    }
			    if(sp->offset + sp->size > input_size)
				fatal("truncated or malformed object (section "
				      "contents of (%0.16s,%0.16s) extends "
				      "past the end of the file) in: %s",
				      sp->segname, sp->sectname, input);
			    rp->found = 1;
			}
			rp = rp->next;
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
		error("unknown load command %d (result maybe bad)", i);
		break;
	    }
	    lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	}
	rp = replaces;
	while(rp != NULL){
	    if(rp->found == 0){
		error("section (%s,%s) not found in: %s", rp->segname,
		      rp->sectname, input);
		errors = 1;
	    }
	    else{
		if(stat(rp->filename, &stat_buf) == -1){
		    system_error("Can't stat file: %s to replace section "
				 "(%s,%s) with", rp->filename, rp->segname,
				 rp->sectname);
		    errors = 1;
		}
		rp->size = stat_buf.st_size;
	    }
	    rp = rp->next;
	}
	if(errors != 0)
	    exit(1);

	if(high_reloc_seg > low_noreloc_seg ||
	   high_reloc_seg > low_linkedit ||
	   high_noreloc_seg > low_linkedit)
	    fatal("contents of input file: %s not in an order that the "
		  "specified sections can be replaced by this program", input);

	qsort(segs, nsegs, sizeof(struct rep_seg),
	      (int (*)(const void *, const void *))cmp_qsort);

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
	if(nsegs > 1)
	    oldoffset = segs[0].sgp->fileoff;
	else
	    oldoffset = 0;
	newoffset = 0;
	k = 0;
	for(i = 0; i < nsegs; i++){
	    if(segs[i].sgp->vmaddr != oldvmaddr)
		fatal("addresses of input file: %s not in an order that the "
		      "specified sections can be replaced by this program",
		      input);
	    segs[i].filesize = segs[i].sgp->filesize;
	    segs[i].vmsize = segs[i].sgp->vmsize;
	    segs[i].sgp->vmaddr = newvmaddr;
	    if(segs[i].sgp->filesize != 0){
		if(segs[i].sgp->fileoff != oldoffset)
		    fatal("segment offsets of input file: %s not in an order "
			  "that the specified sections can be replaced by this "
			  "program", input);
		segs[i].fileoff = segs[i].sgp->fileoff;
		if(strcmp(segs[i].sgp->segname, SEG_LINKEDIT) != 0 ||
		   i != nsegs - 1)
		    segs[i].sgp->fileoff = newoffset;
		sp = (struct section *)((char *)(segs[i].sgp) +
					sizeof(struct segment_command));
		oldsectsize = 0;
		newsectsize = 0;
		if(segs[i].sgp->flags & SG_NORELOC){
		    for(j = 0; j < segs[i].sgp->nsects; j++){
			sects[k + j].sp = sp;
			sects[k + j].offset = sp->offset;
			oldsectsize += sp->size;
			rp = replaces;
			while(rp != NULL){
			    if(strncmp(rp->segname, sp->segname,
				       sizeof(sp->segname)) == 0 &&
			       strncmp(rp->sectname, sp->sectname,
				       sizeof(sp->sectname)) == 0){
				sects[k + j].rp = rp;
				segs[i].modified = 1;
				sp->size = round(rp->size, 1 << sp->align);
				break;
			    }
			    rp = rp->next;
			}
			sp->offset = newoffset + newsectsize;
			sp->addr   = newvmaddr + newsectsize;
			newsectsize += sp->size;
			sp++;
		    }
		    if(strcmp(segs[i].sgp->segname, SEG_LINKEDIT) != 0 ||
		       i != nsegs - 1){
			if(segs[i].sgp->filesize != round(oldsectsize,
							  PAGESIZE))
			    fatal("contents of input file: %s not in a format "
				  "that the specified sections can be replaced "
				  "by this program", input);
			segs[i].sgp->filesize = round(newsectsize, PAGESIZE);
			segs[i].sgp->vmsize = round(newsectsize, PAGESIZE);
			segs[i].padsize = segs[i].sgp->filesize  - newsectsize;
		    }
		}
		if(strcmp(segs[i].sgp->segname, SEG_LINKEDIT) != 0 ||
		   i != nsegs - 1){
		    oldoffset += segs[i].filesize;
		    newoffset += segs[i].sgp->filesize;
		}
	    }
	    oldvmaddr += segs[i].vmsize;
	    newvmaddr += segs[i].sgp->vmsize;
	    k += segs[i].sgp->nsects;
	}

	/*
	 * Now update the offsets to the linkedit information.
	 */
	if(oldoffset != low_linkedit)
	    fatal("contents of input file: %s not in an order that the "
		  "specified sections can be replaced by this program", input);
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
	if((outfd = open(output, O_CREAT | O_WRONLY | O_TRUNC ,input_mode)) 
	   == -1)
	    system_fatal("can't create output file: %s", output);

	if(r = vm_allocate(task_self(), &pad_addr, PAGESIZE, 1) != KERN_SUCCESS)
	    mach_fatal(r, "vm_allocate() failed");

	k = 0;
	for(i = 0; i < nsegs; i++){
	    if(segs[i].modified){
		for(j = 0; j < segs[i].sgp->nsects; j++){
		    /* if the section is replaced write the replaced section */
		    sp = sects[k + j].sp;
		    rp = sects[k + j].rp;
		    if(rp != NULL){
			if((sectfd = open(rp->filename, O_RDONLY)) == -1)
		    	    system_fatal("can't open file: %s to replace "
					 "section (%s,%s) with", rp->filename,
					 rp->segname, rp->sectname);
			if((r = map_fd(sectfd, 0, &sect_addr, TRUE, rp->size))
			   != KERN_SUCCESS)
	    		    mach_fatal(r, "Can't map file: %s", rp->filename);
			for(l = rp->size + 1; l < sp->size; l++)
			    *((char *)sect_addr + l) = '\0';
			if(write(outfd, sect_addr, sp->size) != sp->size)
			    system_fatal("can't write new section contents for "
					 "section (%s,%s) to output file: %s", 
					 rp->segname, rp->sectname, output);
			if(close(sectfd) == -1)
		    	    system_error("can't close file: %s to replace "
					 "section (%s,%s) with", rp->filename,
					 rp->segname, rp->sectname);
			if((r = vm_deallocate(task_self(), sect_addr, rp->size))
			   != KERN_SUCCESS)
	    		    mach_fatal(r, "Can't deallocate memory for mapped "
				       "file: %s", rp->filename);
		    }
		    else{
			/* write the original section */
			if(sects[k + j].offset + sp->size > input_size)
			    fatal("truncated or malformed object file: %s "
				  "(section (%0.16s,%0.16s) extends past the "
				  "end of the file)",input, sp->segname,
				  sp->sectname);
			if(write(outfd,(char *)input_addr + sects[k + j].offset,
			   sp->size) != sp->size)
			    system_fatal("can't write section contents for "
					 "section (%s,%s) to output file: %s", 
					 rp->segname, rp->sectname, output);
		    }
		    sp++;
		}
		/* write the segment padding */
		if(write(outfd, pad_addr, segs[i].padsize) != segs[i].padsize)
		    system_fatal("can't write segment padding for segment %s to"
				 " output file: %s", segs[i].sgp->segname,
				 output);
	    }
	    else{
		/* write the original segment */
		if(strcmp(segs[i].sgp->segname, SEG_LINKEDIT) != 0 ||
		   i != nsegs - 1){
		    if(segs[i].fileoff + segs[i].sgp->filesize > input_size)
			fatal("truncated or malformed object file: %s "
			      "(segment: %s extends past the end of "
			      "the file)", input, segs[i].sgp->segname);
		    if(write(outfd, (char *)input_addr + segs[i].fileoff,
		       segs[i].sgp->filesize) != segs[i].sgp->filesize)
			system_fatal("can't write segment contents for "
				     "segment: %s to output file: %s", 
				     segs[i].sgp->segname, output);
		}
	    }
	    k += segs[i].sgp->nsects;
	}
	/* write the linkedit info */
	size = input_size - low_linkedit;
	if(write(outfd, (char *)input_addr + low_linkedit, size) != size)
	    system_fatal("can't write link edit information to output file: %s",
			 output);
	lseek(outfd, 0, L_SET);
	size = sizeof(struct mach_header) + mhp->sizeofcmds;
	if(write(outfd, input_addr, size) != size)
	    system_fatal("can't write headers to output file: %s", output);

	if(close(outfd) == -1)
	    system_fatal("can't close output file: %s", output);
}

/*
 * Function for qsort for comparing segment's vmaddrs
 */
static
int
cmp_qsort(
const struct rep_seg *seg1,
const struct rep_seg *seg2)
{
	return((long)(seg1->sgp->vmaddr) - (long)(seg2->sgp->vmaddr));
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
 * Print the usage message and exit non-zero.
 */
static
void
usage()
{
	fprintf(stderr, "Usage: %s <input file> [-extract <segname> <sectname> "
			"<filename>] ...\n\t[[-replace <segname> <sectname> "
			"<filename>] ... -output <filename>]\n", progname);
	exit(1);
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
 * Print the fatal error message along with the mach error string, cleanup and
 * exit non-zero.
 */
static
void
mach_fatal(
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
	exit(1);
}

/*
 * Round v to a multiple of r.
 */
static
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

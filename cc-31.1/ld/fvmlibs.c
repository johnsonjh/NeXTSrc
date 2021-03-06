#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
#ifndef RLD
/*
 * This file contains the routines to manage the fixed VM shared libraries
 * that the output file uses.
 */
#include <stdio.h>
#include <strings.h>
#include <sys/loader.h>
#include <mach.h>
#include <ar.h>

#include "ld.h"
#include "objects.h"
#include "sections.h"
#include "fvmlibs.h"

/* the pointer to the head of the load fixed VM shared library commamds */
struct merged_fvmlib *merged_fvmlibs = NULL;

/* the pointer to the head of the fixed VM shared library segments */
struct merged_segment *fvmlib_segments = NULL;

static struct merged_fvmlib *lookup_merged_fvmlib(struct fvmlib_command *fl);
static void add_fvmlib_segment(struct segment_command *sg, char *fvmlib_name);

/*
 * merge_fvmlibs() handles the things relelated to fixed VM libraries.  It does
 * two things.  First any object file that has a LC_LOADFVMLIB command in it
 * indicated that library must be loaded in the final output.  So this routine
 * collects all the load fixed VM library load commands into a list.  Second
 * for the link editor to do overlap checking of the segments in a fixed VM
 * library with all the segments in the output file the program mkshlib(l)
 * builds an object file with segments marked with SG_FVMLIB and one
 * LC_LOADFVMLIB command for the library those segments are in.  So this routine
 * also collects a list of these segments so the overlap checking can be done.
 */
void
merge_fvmlibs(void)
{
    long i, nload_fvmlibs, nid_fvmlibs, nfvmlib_segments;
    struct mach_header *mh;
    struct load_command *lc;
    struct segment_command *sg;
    struct fvmlib_command *fl;
    struct merged_fvmlib *mfl;

#ifdef DEBUG
	/* The compiler "warnings: `fl' and `mfl' may be used uninitialized */
	/* in this function" can safely be ignored */
	fl = NULL;;
	mfl = NULL;;
#endif DEBUG

	/*
	 * First process all the load commands for the fixed VM libraries
	 * loaded.  This needs to be done first so that the fvmlib segments
	 * can be associated with the library they are for.
	 */
	nload_fvmlibs = 0;
	nid_fvmlibs = 0;
	nfvmlib_segments = 0;
	mh = (struct mach_header *)cur_obj->obj_addr;
	lc = (struct load_command *)((char *)cur_obj->obj_addr +
				     sizeof(struct mach_header));
	for(i = 0; i < mh->ncmds; i++){
	    if(lc->cmd == LC_LOADFVMLIB){
		fl = (struct fvmlib_command *)lc;
		mfl = lookup_merged_fvmlib(fl);
		nload_fvmlibs++;
	    }
	    if(lc->cmd == LC_IDFVMLIB){
		fl = (struct fvmlib_command *)lc;
		mfl = lookup_merged_fvmlib(fl);
		nid_fvmlibs++;
	    }
	    else if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		if(sg->flags == SG_FVMLIB)
		    nfvmlib_segments++;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}

	/*
	 * If the output file type is MH_FVMLIB then there should be one
	 * LC_IDFVMLIB command seen in all the input files (that check is done
	 * layout_segments() in layout.c.  No LC_LOADFVMLIB commands should be
	 * seen in the input files for the MH_FVMLIB output format and this is
	 * checked in check_cur_obj() in pass1.c.
	 */
	if(filetype == MH_FVMLIB){
	    if(nid_fvmlibs > 1){
		error_with_cur_obj("contains more than one LC_IDFVMLIB load "
				   "command");
		return;
	    }
	}
	/*
	 * For output file types that are not MH_FVMLIB then.  There must be
	 * exactly one LC_LOADFVMLIB command seen in the object file which is
	 * the fixed VM library associated with these segments (if it has any
	 * of these segments).  There should not be any LC_IDFVMLIB commands
	 * seen in the input files if the output format is not MH_FVMLIB and
	 * this is checked in check_cur_obj() in pass1.c.
	 */
	else{
	    if(nfvmlib_segments == 0)
		return;
	    if(nload_fvmlibs == 0){
		error_with_cur_obj("contains SG_FVMLIB segments but no "
				   "LC_LOADFVMLIB load command");
		return;
	    }
	    if(nload_fvmlibs > 1){
		error_with_cur_obj("contains SG_FVMLIB segments and more than "
				   "one LC_LOADFVMLIB load command");
		return;
	    }
	    if(mfl->multiple)
		return;

	    lc = (struct load_command *)((char *)cur_obj->obj_addr +
					 sizeof(struct mach_header));
	    for(i = 0; i < mh->ncmds; i++){
		if(lc->cmd == LC_SEGMENT){
		    sg = (struct segment_command *)lc;
		    if(sg->flags == SG_FVMLIB)
			add_fvmlib_segment(sg,
					   (char *)fl + fl->fvmlib.name.offset);
		}
		lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    }
	}
}

/*
 * lookup_merged_fvmlib() adds the LC_LOADFVMLIB structure passed to it to the
 * merged list of fixed VM libraries.  It warns if it see the same library
 * twice.
 */
static
struct merged_fvmlib *
lookup_merged_fvmlib(
struct fvmlib_command *fl)
{
    char *fvmlib_name;
    struct merged_fvmlib **p, *mfl;

	fvmlib_name = (char *)fl + fl->fvmlib.name.offset;
	p = &merged_fvmlibs;
	while(*p){
	    mfl = *p;
	    if(strcmp(mfl->fvmlib_name, fvmlib_name) == 0){
		if(mfl->multiple == FALSE){
		    if(mfl->fl->cmd == LC_IDFVMLIB)
			error("multiple object files identify fixed VM library "
			      "%s", fvmlib_name);
		    else
			warning("multiple object files load fixed VM library "
				"%s", fvmlib_name);
		    print_obj_name(mfl->definition_object);
		    print("%s fixed VM library %s\n",
			  mfl->fl->cmd == LC_LOADFVMLIB ? "loads" :"identifies",
			  fvmlib_name);
		    mfl->multiple = TRUE;
		}
		print_obj_name(cur_obj);
		print("%s fixed VM library %s\n",
		      mfl->fl->cmd == LC_LOADFVMLIB ? "loads" : "identifies",
		      fvmlib_name);
		return(mfl);
	    }
	    p = &(mfl->next);
	}
	*p = allocate(sizeof(struct merged_fvmlib));
	memset(*p, '\0', sizeof(struct merged_fvmlib));
	mfl = *p;
	mfl->fl = fl;
	mfl->fvmlib_name = fvmlib_name;
	mfl->definition_object = cur_obj;
	mfl->multiple = FALSE;
	return(mfl);
}

/*
 * add_fvmlib_segment() adds the specified segment to the list of
 * fvmlib_segments as comming from the specified fvmlib_name.
 */
static
void
add_fvmlib_segment(
struct segment_command *sg,
char *fvmlib_name)
{
    struct merged_segment **p, *msg;

	p = &fvmlib_segments;
	while(*p){
	    msg = *p;
	    p = &(msg->next);
	}
	*p = allocate(sizeof(struct merged_segment));
	msg = *p;
	memset(msg, '\0', sizeof(struct merged_segment));
	msg->sg = *sg;
	msg->filename = fvmlib_name;
}

#ifdef DEBUG
/*
 * print_load_fvmlibs_list() prints the fvmlib segments table.  For debugging.
 */
void
print_load_fvmlibs_list(void)
{
    struct merged_fvmlib **p, *mfl;

	print("Load fvmlibs list\n");
	p = &merged_fvmlibs;
	while(*p){
	    mfl = *p;
	    if(mfl->fl->cmd == LC_LOADFVMLIB)
		print("    LC_LOADFVMLIB\n");
	    else
		print("    LC_IDFVMLIB\n");
	    print("\tcmdsize %d\n", mfl->fl->cmdsize);
	    if(mfl->fl->fvmlib.name.offset < mfl->fl->cmdsize)
		print("\tname %s (offset %d)\n",
		       (char *)(mfl->fl) + mfl->fl->fvmlib.name.offset,
		       mfl->fl->fvmlib.name.offset);
	    else
		print("\tname ?(bad offset %d)\n",mfl->fl->fvmlib.name.offset);
	    print("\tminor version %d\n", mfl->fl->fvmlib.minor_version);
	    print("\theader addr 0x%08x\n", mfl->fl->fvmlib.header_addr);
	    print("    fvmlib_name %s\n", mfl->fvmlib_name);
	    print("    definition_object ");
	    print_obj_name(mfl->definition_object);
	    print("\n");
	    print("    multiple %s\n", mfl->multiple == TRUE ? "TRUE" :
		   "FALSE");
	    p = &(mfl->next);
	}
}

/*
 * print_fvmlib_segments() prints the fvmlib segments table.  For debugging.
 */
void
print_fvmlib_segments(void)
{
    struct merged_segment **p, *msg;
    long flags;

	print("FVMLIB segments\n");
	p = &fvmlib_segments;
	while(*p){
	    msg = *p;
	    print("    filename %s\n", msg->filename);
	    print("\t      cmd LC_SEGMENT\n");
	    print("\t  cmdsize %d\n", msg->sg.cmdsize);
	    print("\t  segname %0.16s\n", msg->sg.segname);
	    print("\t   vmaddr 0x%08x\n", msg->sg.vmaddr);
	    print("\t   vmsize 0x%08x\n", msg->sg.vmsize);
	    print("\t  fileoff %d\n", msg->sg.fileoff);
	    print("\t filesize %d\n", msg->sg.filesize);
	    print("\t  maxprot 0x%08x\n", msg->sg.maxprot);
	    print("\t initprot 0x%08x\n", msg->sg.initprot);
	    print("\t   nsects %d\n", msg->sg.nsects);
	    print("\t    flags");
	    if(msg->sg.flags == 0)
		print(" (none)\n");
	    else{
		flags = msg->sg.flags;
		if(flags & SG_HIGHVM){
		    print(" HIGHVM");
		    flags &= ~SG_HIGHVM;
		}
		if(flags & SG_FVMLIB){
		    print(" FVMLIB");
		    flags &= ~SG_FVMLIB;
		}
		if(flags & SG_NORELOC){
		    print(" NORELOC");
		    flags &= ~SG_NORELOC;
		}
		if(flags)
		    print(" 0x%x (unknown flags)\n", flags);
		else
		    print("\n");
	    }
	    p = &(msg->next);
	}
}
#endif DEBUG
#endif !defined(RLD)

#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines that drives the layout phase of the
 * link-editor.  In this phase the output file's addresses and offset are
 * set up.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/loader.h> 
#include <nlist.h>
#include <reloc.h>
#include <mach.h>
#include <streams/streams.h>

#include "ld.h"
#include "specs.h"
#include "fvmlibs.h"
#include "objects.h"
#include "sections.h"
#include "symbols.h"
#include "layout.h"
#include "pass1.h"
#include "pass2.h"
#include "sets.h"
#include "rld.h"

/* The output file's mach header */
struct mach_header output_mach_header = { 0 };

/*
 * The output file's symbol table load command and the offsets used in the
 * second pass to output the symbol table and string table.
 */
struct symtab_info output_symtab_info = { 0 };

/*
 * The output file's thread load command and the machine specific information
 * for it.
 */
struct thread_info output_thread_info = { 0 };

/*
 * The thread states that are currently known by this link editor.
 * (for the specific cputypes and cpusubtypes)
 */
/* cputype == CPU_TYPE_MC680x0, all cpusubtype's */
static struct NeXT_thread_state_regs mc680x0 = { 0 };

static void layout_segments(void);
#ifndef RLD
static enum bool check_reserved_segment(char *segname,
					char *reserved_error_string);
static void check_overlap(struct merged_segment *msg1,
			  struct merged_segment *msg2);
static void check_for_overlapping_segments(void);
static void print_load_map(void);
static void print_load_map_for_objects(struct merged_section *ms);
#endif !defined(RLD)

/*
 * layout() is called from main() and lays out the output file.
 */
void
layout(void)
{
#ifdef RLD
	memset(&output_mach_header, '\0', sizeof(struct mach_header));
	memset(&output_symtab_info, '\0', sizeof(struct symtab_info));
	memset(&output_thread_info, '\0', sizeof(struct thread_info));
	memset(&mc680x0, '\0', sizeof(struct NeXT_thread_state_regs));
#endif RLD
	/*
	 * First finish creating all sections that will be in the final output
	 * file.  This involves defining common symbols which can create a
	 * (__DATA,__common) section and creating sections from files (via
	 * -sectcreate options).
	 */
	define_common_symbols();
	/*
	 * Process the command line specifications for the sections including
	 * creating sections from files.
	 */
#ifndef RLD
	process_section_specs();
#endif !defined(RLD)

	/*
	 * So literal pointer sections can use indirect symbols these need to
	 * be resolved before the literal pointer section is merged.
	 */
	reduce_indr_symbols();
	if(errors)
	    return;

	/*
	 * Now that the alignment of all the sections has been determined (from
	 * the command line and the object files themselves) the literal
	 * sections can be merged with the correct alignment and their sizes
	 * in the output file can be determined.
	 */
	merge_literal_sections();
#ifdef DEBUG
	if(debug & 0x200000)
	    print_merged_section_stats();
#endif DEBUG

#ifndef RLD
	/*
	 * Layout any sections that have -sectorder options specified for them.
	 */
	layout_ordered_sections();
#endif RLD

	/*
	 * Set the segment addresses, protections and offsets into the file.
	 */
	layout_segments();

#ifndef RLD
	if(load_map)
	    print_load_map();
#endif !defined(RLD)

#ifdef DEBUG
	if(debug & 0x80){
	    print_mach_header();
	    print_merged_sections("after layout");
	    print_symtab_info();
	    print_thread_info();
	}
	if(debug & 0x100)
	    print_symbol_list("after layout", FALSE);
	if(debug & 0x100000)
	    print_object_list();
#endif DEBUG
}

/*
 * layout_segments() basicly lays out the addresses and file offsets of
 * everything in the ouput file (since everything can be in a segment).
 * It checks for the link editor reserved segment "__PAGEZERO" and "__LINKEDIT"
 * and prints an error message if they exist in the output.  It creates these
 * segments if this is the right file type and the right options are specified.
 * It processes all the segment specifications from the command line options.
 * Sets the addresses of all segments and sections in those segments and sets
 * the sizes of all segments.  Also sets the file offsets of all segments,
 * sections, relocation information and symbol table information.  It creates
 * the mach header that will go in the output file.  It numbers the merged
 * sections with their section number they will have in the output file.
 */
static
void
layout_segments(void)
{
    long i, ncmds, sizeofcmds, headers_size, offset;
    unsigned long addr, size, max_first_align, pad;
    struct merged_segment **p, *msg, *first_msg, *prev_msg;
    struct merged_section **content, **zerofill, *ms;
#ifndef RLD
    struct merged_fvmlib **q, *mfl;
    long nfvmlibs;
    struct segment_spec *seg_spec;
    enum bool address_zero_specified;
#endif !defined(RLD)
    struct merged_symbol *merged_symbol;

    static struct merged_segment linkedit_segment = { 0 };
    static struct merged_segment pagezero_segment = { 0 };
    static struct merged_segment object_segment = { 0 };

#if defined(DEBUG) || defined(RLD)
	/* The compiler "warning: `prev_msg' may be used uninitialized in */
	/* this function" can safely be ignored */
	prev_msg = NULL;
#endif

#ifdef RLD
	memset(&object_segment, '\0', sizeof(struct merged_segment));
#endif RLD

	/*
	 * If the file type is MH_OBJECT then place all the sections in one
	 * unnamed segment.
	 */
	if(filetype == MH_OBJECT){
	    object_segment.filename = outputfile;
	    content = &(object_segment.content_sections);
	    zerofill = &(object_segment.zerofill_sections);
#ifdef RLD
	    original_merged_segments = merged_segments;
#endif RLD
	    p = &merged_segments;
	    while(*p){
		msg = *p;
		object_segment.sg.nsects += msg->sg.nsects;
		*content = msg->content_sections;
		while(*content){
		    ms = *content;
		    content = &(ms->next);
		}
		*zerofill = msg->zerofill_sections;
		while(*zerofill){
		    ms = *zerofill;
		    zerofill = &(ms->next);
		}
		p = &(msg->next);
	    }
	    if(object_segment.sg.nsects != 0)
		merged_segments = &object_segment;
	}

#ifndef RLD
	/*
	 * Create the link edit segment if specified and size it.
	 */
	if(filetype == MH_EXECUTE || filetype == MH_FVMLIB){
	    if(check_reserved_segment(SEG_LINKEDIT, "segment " SEG_LINKEDIT
				   " reserved for the -seglinkedit option")){
		/*
		 * Now that the above check has been made and the segment is
		 * known not to exist create the link edit segment if specified.
		 */
		if(seglinkedit == TRUE){
		    /*
		     * Fill in the merged segment.  In this case only the
		     * segment name and filesize of the segment are not zero
		     * or NULL.  Note the link edit segment is unique in that
		     * it's filesize is not rounded to the segment alignment.
		     * This can only be done because this is the last segment
		     * in the file (right before end of file).
		     */
		    linkedit_segment.filename = outputfile;
		    strcpy(linkedit_segment.sg.segname, SEG_LINKEDIT);
		    if(save_reloc)
			linkedit_segment.sg.filesize += nreloc *
						sizeof(struct relocation_info);
		    if(strip_level != STRIP_ALL)
			linkedit_segment.sg.filesize +=
					(nmerged_symbols + nlocal_symbols) *
					sizeof(struct nlist) +
					merged_string_size + local_string_size +
					STRING_SIZE_OFFSET;
		    else
			warning("segment created for -seglinkedit zero size "
			        "(output file stripped)");
		    linkedit_segment.sg.vmsize =
				round(linkedit_segment.sg.filesize, segalign);
		    /* place this last in the merged segment list */
		    p = &merged_segments;
		    while(*p){
			msg = *p;
			p = &(msg->next);
		    }
		    *p = &linkedit_segment;
		}
	    }
	}

	/*
	 * If the the file type is MH_EXECUTE and address zero has not been
	 * assigned to a segment create the "__PAGEZERO" segment.
	 */
	if(filetype == MH_EXECUTE){
	    if(check_reserved_segment(SEG_PAGEZERO, "segment " SEG_PAGEZERO
				      " reserved for address zero through "
				      "segment alignment")){
		/*
		 * There shouldn't be any segment specifications for this
		 * segment (address or protection).
		 */
		seg_spec = lookup_segment_spec(SEG_PAGEZERO);
		if(seg_spec != NULL){
		    if(seg_spec->addr_specified)
			error("specified address for segment " SEG_PAGEZERO
			      " ignored (segment " SEG_PAGEZERO " reserved for "
			      "address zero through segment alignment)");
		    if(seg_spec->prot_specified)
			error("specified protection for segment " SEG_PAGEZERO
			      " ignored (segment " SEG_PAGEZERO " reserved for "
			      "address zero through segment alignment and has "
			      "no assess protections)");
		    seg_spec->processed = TRUE;
		}
		address_zero_specified = FALSE;
		for(i = 0; i < nsegment_specs; i++){
		    if(segment_specs[i].addr_specified &&
		       segment_specs[i].addr == 0 &&
		       &(segment_specs[i]) != seg_spec){
			address_zero_specified = TRUE;
			break;
		    }
		}
		if(address_zero_specified == FALSE &&
		   (seg1addr_specified == FALSE || seg1addr != 0)){
		    pagezero_segment.filename = outputfile;
		    pagezero_segment.addr_set = TRUE;
		    pagezero_segment.prot_set = TRUE;
		    strcpy(pagezero_segment.sg.segname, SEG_PAGEZERO);
		    pagezero_segment.sg.vmsize = segalign;
		    /* place this first in the merged segment list */
		    pagezero_segment.next = merged_segments;
		    merged_segments = &pagezero_segment;
		}
	    }
	}

	/*
	 * Process the command line specifications for the segments setting the
	 * addresses and protections for the specified segments into the merged
	 * segments.
	 */
	process_segment_specs();
#endif !defined(RLD)

	/*
	 * Set the address of the first segment via the -seg1addr option or the
	 * default first address of the output format.
	 */
	first_msg = merged_segments;
	if(first_msg == &pagezero_segment)
	    first_msg = first_msg->next;
	if(first_msg != NULL){
	    if(seg1addr_specified == TRUE){
		if(seg1addr % segalign != 0)
		    fatal("-seg1addr: 0x%x not a multiple of the segment "
			  "alignment (0x%x)", seg1addr, segalign);
		if(first_msg->addr_set == TRUE)
		    fatal("address of first segment: %0.16s set both by name "
			  "and with the -seg1addr option",
			  first_msg->sg.segname);
		first_msg->sg.vmaddr = seg1addr;
		first_msg->addr_set = TRUE;
	    }
	    else{
		if(first_msg->addr_set == FALSE){
		    if(filetype == MH_EXECUTE)
			first_msg->sg.vmaddr = segalign;
		    else
			first_msg->sg.vmaddr = 0;
		    first_msg->addr_set = TRUE;
		}
	    }
	}

#ifndef RLD
	/*
	 * If there is a "__TEXT" segment who's protection has not been set
	 * set it's inital protection to "r-x" and it's maximum protection
	 * to the default "rwx".  Also do the same for the "__LINKEDIT" segment
	 * if it was created.
	 */
	msg = lookup_merged_segment(SEG_TEXT);
	if(msg != NULL && msg->prot_set == FALSE){
	    msg->sg.initprot = VM_PROT_READ | VM_PROT_EXECUTE;
	    msg->sg.maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
	    msg->prot_set = TRUE;
	}
	if(seglinkedit){
	    msg = lookup_merged_segment(SEG_LINKEDIT);
	    if(msg != NULL && msg->prot_set == FALSE){
		msg->sg.initprot = VM_PROT_READ | VM_PROT_EXECUTE;
		msg->sg.maxprot =VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
		msg->prot_set = TRUE;
	    }
	}
#endif !defined(RLD)

	/*
	 * Size and count the load commands in the output file which include:
	 *   The segment load commands
	 *   The load fvmlib commands
	 *   A symtab command
	 *   A thread command (if the file type is NOT MH_FVMLIB)
	 */
	ncmds = 0;
	sizeofcmds = 0;
	/*
	 * Size the segment commands and accumulate the number of commands and
	 * size of them.
	 */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    msg->sg.cmd = LC_SEGMENT;
	    msg->sg.cmdsize = sizeof(struct segment_command) +
			      msg->sg.nsects * sizeof(struct section);
	    ncmds++;
	    sizeofcmds += msg->sg.cmdsize;
	    p = &(msg->next);
	}
#ifndef RLD
	/*
	 * Accumulate the number of commands for the fixed VM shared libraries
	 * and their size.  Since the commands themselves come from the input
	 * object files the 'cmd' and 'cmdsize' fields are already set.
	 */
	nfvmlibs = 0;
	q = &merged_fvmlibs;
	while(*q){
	    mfl = *q;
	    nfvmlibs++;
	    sizeofcmds += mfl->fl->cmdsize;
	    q = &(mfl->next);
	}
	if(filetype == MH_FVMLIB && nfvmlibs != 1){
	    if(nfvmlibs == 0)
		error("no LC_IDFVMLIB command in the linked object files");
	    else
		error("more than one LC_IDFVMLIB command in the linked object "
		      "files");
	}
	ncmds += nfvmlibs;
#endif !defined(RLD)
	/*
	 * Create the symbol table load command.
	 */
	output_symtab_info.symtab_command.cmd = LC_SYMTAB;
	output_symtab_info.symtab_command.cmdsize =
						sizeof(struct symtab_command);
	if(strip_level != STRIP_ALL){
	    output_symtab_info.symtab_command.nsyms = nmerged_symbols +
						      nlocal_symbols;
	    output_symtab_info.symtab_command.strsize = merged_string_size +
							local_string_size +
							STRING_SIZE_OFFSET;
	    output_symtab_info.output_strsize = STRING_SIZE_OFFSET;
	}
	ncmds++;
	sizeofcmds += output_symtab_info.symtab_command.cmdsize;
	/*
	 * Create the unix thread command if this is filetype is not a fixed VM
	 * shared library and we have seen an object file so we know what type
	 * of machine this is for.
	 */
 	if(filetype != MH_FVMLIB && cputype != 0 &&
 	   first_msg != NULL && first_msg->content_sections != NULL){
	    output_thread_info.thread_in_output = TRUE;
	    output_thread_info.thread_command.cmd = LC_UNIXTHREAD;
	    output_thread_info.thread_command.cmdsize =
						sizeof(struct thread_command) +
						2 * sizeof(long);
	    if(cputype == CPU_TYPE_MC680x0){
		output_thread_info.flavor = NeXT_THREAD_STATE_REGS;
		output_thread_info.count = NeXT_THREAD_STATE_REGS_COUNT;
		output_thread_info.entry_point = &(mc680x0.pc);
		output_thread_info.state = &mc680x0;
		output_thread_info.thread_command.cmdsize += sizeof(long) *
					    NeXT_THREAD_STATE_REGS_COUNT;
	    }
	    else{
		fatal("internal error: layout_segments() called with unknown "
		      "cputype (%d) set", cputype);
	    }
	    sizeofcmds += output_thread_info.thread_command.cmdsize;
	    ncmds++;
	}
	else{
	    output_thread_info.thread_in_output = FALSE;
	}

	/*
	 * Fill in the mach_header for the output file.
	 */
	output_mach_header.magic = MH_MAGIC;
	output_mach_header.cputype = cputype;
	output_mach_header.cpusubtype = cpusubtype;
	output_mach_header.filetype = filetype;
	output_mach_header.ncmds = ncmds;
	output_mach_header.sizeofcmds = sizeofcmds;
	output_mach_header.flags = 0;
	if(base_obj != NULL)
	    output_mach_header.flags |= MH_INCRLINK;

	/*
	 * The total headers size needs to be known in the case of MH_EXECUTE
	 * and MH_FVMLIB format file types because their headers get loaded as
	 * part of of the first segment.  For MH_FVMLIB file types the headers
	 * are placed on their own page (the size of the segment alignment).
	 */
	headers_size = sizeof(struct mach_header) + sizeofcmds;
	if(filetype == MH_FVMLIB){
	    if(headers_size > segalign)
		fatal("size of headers (0x%x) exceeds the segment alignment "
		      "(0x%x) (would cause the addresses not to be fixed)",
		      headers_size, segalign);
	    headers_size = segalign;
	}

	/*
	 * For MH_EXECUTE formats the as much of the segment padding that can
	 * be is moved to the begining of the segment just after the headers.
	 * This is done so that the headers could added to by a smart program
	 * like segedit(1) some day.
	 */
	if(filetype == MH_EXECUTE){
	    if(first_msg != NULL){
		size = 0;
		content = &(first_msg->content_sections);
		if(*content){
		    max_first_align = 1 << (*content)->s.align;
		    while(*content){
			ms = *content;
			if((1 << ms->s.align) > segalign)
			    error("alignment (0x%x) of section (%0.16s,%0.16s) "
				  "greater than the segment alignment (0x%x)",
				  1 << ms->s.align, ms->s.segname,
				  ms->s.sectname, segalign);
			size = round(size, 1 << ms->s.align);
			if((1 << ms->s.align) > max_first_align)
			    max_first_align = 1 << ms->s.align;
			size += ms->s.size;
			content = &(ms->next);
		    }
		    if(errors == 0){
			pad = ((round(size + round(headers_size,
				      max_first_align), segalign) -
			       (size + round(headers_size, max_first_align))) /
				 max_first_align) * max_first_align;
			if(pad > headerpad)
			    headerpad = pad;
			headers_size += headerpad;
		    }
		}
	    }
	}

	/*
	 * Assign the section addresses relitive to the start of their segment
	 * to accumulate the file and vm sizes of the segments.
	 */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    if(msg != &pagezero_segment && msg != &linkedit_segment){
		if(msg == first_msg &&
		   (filetype == MH_EXECUTE || filetype == MH_FVMLIB))
		    addr = headers_size;
		else
		    addr = 0;
		content = &(msg->content_sections);
		while(*content){
		    ms = *content;
		    if((1 << ms->s.align) > segalign)
			error("alignment (0x%x) of section (%0.16s,%0.16s) "
			      "greater than the segment alignment (0x%x)",
			      1 << ms->s.align, ms->s.segname, ms->s.sectname,
			      segalign);
		    addr = round(addr, 1 << ms->s.align);
		    ms->s.addr = addr;
		    addr += ms->s.size;
		    content = &(ms->next);
		}
		if(msg == &object_segment)
		    msg->sg.filesize = addr;
		else
		    msg->sg.filesize = round(addr, segalign);
		zerofill = &(msg->zerofill_sections);
		while(*zerofill){
		    ms = *zerofill;
		    if((1 << ms->s.align) > segalign)
			error("alignment (0x%x) of section (%0.16s,%0.16s) "
			      "greater than the segment alignment (0x%x)",
			      1 << ms->s.align, ms->s.segname, ms->s.sectname,
			      segalign);
		    addr = round(addr, 1 << ms->s.align);
		    ms->s.addr = addr;
		    addr += ms->s.size;
		    zerofill = &(ms->next);
		}
		if(msg == &object_segment)
		    msg->sg.vmsize = addr;
		else
		    msg->sg.vmsize = round(addr, segalign);
	    }
	    p = &(msg->next);
	}

#ifdef RLD
	/*
	 * For rld() the output format is MH_OBJECT and only the contents of the
	 * segment (the entire vmsize not just the filesize) without headers is
	 * allocated and the address the segment is linked to is the address of
	 * this memory.
	 */
	output_size = 0;
	if(first_msg != NULL){
	    kern_return_t r;
	    long allocate_size;

	    output_size = headers_size + first_msg->sg.vmsize;
	    allocate_size = output_size;
	    if(strip_level != STRIP_ALL)
		allocate_size += output_symtab_info.symtab_command.nsyms *
				 sizeof(struct nlist) +
				 output_symtab_info.symtab_command.strsize;
	    if((r = vm_allocate(task_self(), (vm_address_t *)&output_addr,
				allocate_size, TRUE)) != KERN_SUCCESS)
		mach_fatal(r, "can't vm_allocate() memory for output of size "
			   "%d", allocate_size);
	    sets[cur_set].output_addr = output_addr;
	    sets[cur_set].output_size = output_size;
	    if(address_func != NULL)
		first_msg->sg.vmaddr =
		      (*address_func)(output_size, headers_size) + headers_size;
	     else
		first_msg->sg.vmaddr = (long)output_addr + headers_size;
	}
#endif RLD
	/*
	 * Set the addresses of segments that have not had their addresses set
	 * and set the addresses of all sections (previously set relitive to the
	 * start of their section and here just moved by the segment address).
	 * The addresses of segments that are not set are set to the ending
	 * address of the previous segment (note that the address of the first
	 * segment has been set above).
	 */
	p = &merged_segments;
	if(*p){
	    prev_msg = *p;
	    p = &(prev_msg->next);
	}
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    /*
	     * The first segment has had it's address set previouly above so
	     * this test will always fail for it and there is no problem of
	     * trying to use prev_msg for the first segment.
	     */
	    if(msg->addr_set == FALSE){
		msg->sg.vmaddr = prev_msg->sg.vmaddr + prev_msg->sg.vmsize;
		msg->addr_set = TRUE;
	    }
	    if(msg != &pagezero_segment && msg != &linkedit_segment){
		content = &(msg->content_sections);
		while(*content){
		    ms = *content;
		    ms->s.addr += msg->sg.vmaddr;
		    content = &(ms->next);
		}
		zerofill = &(msg->zerofill_sections);
		while(*zerofill){
		    ms = *zerofill;
		    ms->s.addr += msg->sg.vmaddr;
		    zerofill = &(ms->next);
		}
	    }
	    prev_msg = msg;
	    p = &(msg->next);
	}
	/*
	 * Set the protections of segments that have not had their protection
	 * set to the default protection (allowing all types of access).
	 */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    if(msg->prot_set == FALSE){
		msg->sg.initprot = VM_PROT_READ | VM_PROT_WRITE |
				  VM_PROT_EXECUTE;
		msg->sg.maxprot = VM_PROT_READ | VM_PROT_WRITE |
				  VM_PROT_EXECUTE;
		msg->prot_set = TRUE;
	    }
	    p = &(msg->next);
	}

#ifndef RLD
	/*
	 * Check for overlapping segments (including fvmlib segments).
	 */
	check_for_overlapping_segments();
#endif RLD

	/*
	 * Assign all file offsets.  Things with offsets appear in the following
	 * order in the file:
	 *    The segments (and their content sections)
	 *    The relocation entries for the content sections
	 *    The symbol table
	 *    The string table.
	 */
	offset = headers_size;
	/* set the offset to the segments and sections */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    if(msg != &pagezero_segment && msg != &linkedit_segment){
		if(msg == first_msg &&
		   (filetype == MH_EXECUTE || filetype == MH_FVMLIB)){
		    msg->sg.fileoff = 0;
		    content = &(msg->content_sections);
		    if(*content){
			ms = *content;
			offset = ms->s.addr - msg->sg.vmaddr;
		    }
		}
		else
		    msg->sg.fileoff = offset;
		content = &(msg->content_sections);
		while(*content){
		    ms = *content;
		    ms->s.offset = offset;
		    if(ms->next != NULL)
			offset += (ms->next->s.addr - ms->s.addr);
		    content = &(ms->next);
		}
		if(msg->sg.filesize == 0)
		    msg->sg.fileoff = 0;
		if(msg == first_msg &&
		   (filetype == MH_EXECUTE || filetype == MH_FVMLIB))
		    offset = msg->sg.filesize;
		else
		    if(msg->sg.filesize != 0)
			offset = msg->sg.fileoff + msg->sg.filesize;
	    }
	    p = &(msg->next);
	}

	/*
	 * The offsets to all the link edit structures in the file must be on
	 * boundaries that they can be mapped into memory and then used as is.
	 * The maximum alignment of all structures in a Mach-O file is
	 * sizeof(long) so the offset must be rounded to this as the sections
	 * and segments may not be rounded to this.
	 */
	offset = round(offset, sizeof(long));
#ifdef RLD
	/*
	 * For RLD if there is any symbol table it is written past the size
	 * of the output_size.  Room has been allocated for it above if the
	 * strip_level != STRIP_ALL.
	 */
	offset = output_size;
#endif RLD

	/* the linkedit segment will start here */
	linkedit_segment.sg.fileoff = offset;

	/* set the offset to the relocation entries (if in the output file) */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(save_reloc && ms->s.nreloc != 0){
		    ms->s.reloff = offset;
		    offset += ms->s.nreloc * sizeof(struct relocation_info);
		}
		else{
		    ms->s.reloff = 0;
		    ms->s.nreloc = 0;
		}
		content = &(ms->next);
	    }
	    p = &(msg->next);
	}
	/* set the offset to the symbol table and string table */
	if(strip_level != STRIP_ALL){
	    output_symtab_info.symtab_command.symoff = offset;
	    offset += output_symtab_info.symtab_command.nsyms *
		      sizeof(struct nlist);
	    output_symtab_info.symtab_command.stroff = offset;
	    offset += output_symtab_info.symtab_command.strsize;
	}
#ifndef RLD
	/* set the size of the output file */
	output_size = offset;
#endif !defined(RLD)

	/*
	 * Set the output section number in to each merged section to be used
	 * to set symbol's section numbers and local relocation section indexes.
	 */
	i = 1;
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		ms->output_sectnum = i++;
		content = &(ms->next);
	    }
	    zerofill = &(msg->zerofill_sections);
	    while(*zerofill){
		ms = *zerofill;
		ms->output_sectnum = i++;
		zerofill = &(ms->next);
	    }
	    p = &(msg->next);
	}

#ifndef RLD
	/*
	 * Define the loader defined symbols.  This is done here because the
	 * address of the headers is needed to defined the symbol for MH_EXECUTE
	 * filetypes.
	 */
	if(filetype == MH_EXECUTE &&
	   first_msg != NULL && first_msg != &linkedit_segment)
	    define_link_editor_execute_symbols(first_msg->sg.vmaddr);
	if(filetype == MH_PRELOAD)
	    define_link_editor_preload_symbols();
#endif !defined(RLD)

	/*
	 * Now with the addresses of the segments and sections set and the
	 * sections numbered for the output file set the values and section
	 * numbers of the merged symbols.
	 */
	layout_merged_symbols();

	/*
	 * Set the entry point to either the specified symbol name's value or
	 * the address of the first section.
	 */
	if(output_thread_info.thread_in_output == TRUE){
	    if(entry_point_name != NULL){
		merged_symbol = *(lookup_symbol(entry_point_name));
		/*
		 * If the symbol is not found, undefined or common the
		 * entry point can't be set.
		 */
		if(merged_symbol == NULL ||
		   merged_symbol->nlist.n_type == (N_EXT | N_UNDF))
		    fatal("entry point symbol name: %s not defined",
			  entry_point_name);
		*output_thread_info.entry_point = merged_symbol->nlist.n_value;
	    }
	    else{
		*output_thread_info.entry_point =
					    first_msg->content_sections->s.addr;
	    }
	}
	else{
	    if(entry_point_name != NULL)
		warning("specified entry point symbol name ignored, output "
 			"file type has no entry point or no non-zerofill "
 			"sections");
	}
}

#ifndef RLD
/*
 * check_reserved_segment() checks to that the reserved segment is NOT in the
 * merged segment list and returns TRUE if so.  If it the segment is found in
 * the merged segment list it prints an error message stating the segment exists
 * and prints the string passed to it as to why this segment is reserved.  Then
 * It prints all sections created from files in that segment and all object
 * files that contain that segment.  Finally it returns FALSE in this case.
 */
static
enum bool
check_reserved_segment(
char *segname,
char *reserved_error_string)
{
    struct merged_segment *msg;
    struct merged_section **content, *ms;

    long i, j;
    struct object_list *object_list, **q;
    struct object_file *object_file;
    struct section *s;

	msg = lookup_merged_segment(segname);
	if(msg != NULL){
	    error("segment %s exist in the output file (%s)", segname,
		  reserved_error_string);
	    /*
	     * Loop through the content sections and report any sections
	     * created from files.
	     */
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->contents_filename != NULL)
		    print("section (%0.16s,%0.16s) created from file "
			   "%s\n", ms->s.segname, ms->s.sectname,
			   ms->contents_filename);
		content = &(ms->next);
	    }
	    /*
	     * Loop through all the objects and report those that have
	     * this segment.
	     */
	    for(q = &objects; *q; q = &(object_list->next)){
		object_list = *q;
		for(i = 0; i < object_list->used; i++){
		    object_file = &(object_list->object_files[i]);
		    if(object_file == base_obj)
			continue;
		    for(j = 0; j < object_file->nsection_maps; j++){
			s = object_file->section_maps[j].s;
			if(strcmp(s->segname, segname) == 0){
			    print_obj_name(object_file);
			    print("contains section (%0.16s,%0.16s)\n",
				   s->segname, s->sectname);
			}
		    }
		}
	    }
	    return(FALSE);
	}
	return(TRUE);
}

/*
 * check_for_overlapping_segments() checks for overlapping segments in the
 * output file and in the segments from the fixed VM shared libraries it uses.
 */
static
void
check_for_overlapping_segments(void)
{
    struct merged_segment **p1, **p2, **last_merged, **last_fvmseg,
			  *msg1, *msg2;

	/*
	 * To make the checking loops below clean the fvmlib segment list is
	 * attached to the end of the merged segment list and then detached
	 * before we return.
	 */
	last_merged = &merged_segments;
	while(*last_merged){
	    msg1 = *last_merged;
	    last_merged = &(msg1->next);
	}
	if(fvmlib_segments != NULL){
	    *last_merged = fvmlib_segments;

	    last_fvmseg = &fvmlib_segments;
	    while(*last_fvmseg){
		msg1 = *last_fvmseg;
		last_fvmseg = &(msg1->next);
	    }
	}
	else
	    last_fvmseg = last_merged;
	*last_fvmseg = base_obj_segments;

	p1 = &merged_segments;
	while(*p1){
	    msg1 = *p1;
	    p2 = &(msg1->next);
	    while(*p2){
		msg2 = *p2;
		check_overlap(msg1, msg2);
		p2 = &(msg2->next);
	    }
	    p1 = &(msg1->next);
	}
	*last_merged = NULL;
	*last_fvmseg = NULL;
}

/*
 * check_overlap() checks if the two segments passed to it overlap and if so
 * prints an error message.
 */
static
void
check_overlap(
struct merged_segment *msg1,
struct merged_segment *msg2)
{
	if(msg1->sg.vmsize == 0 || msg2->sg.vmsize == 0)
	    return;

	if(msg1->sg.vmaddr > msg2->sg.vmaddr){
	    if(msg2->sg.vmaddr + msg2->sg.vmsize <= msg1->sg.vmaddr)
		return;
	}
	else{
	    if(msg1->sg.vmaddr + msg1->sg.vmsize <= msg2->sg.vmaddr)
		return;
	}
	error("%0.16s segment (address = 0x%x size = 0x%x) of %s overlaps with "
	      "%0.16s segment (address = 0x%x size = 0x%x) of %s",
	      msg1->sg.segname, msg1->sg.vmaddr,msg1->sg.vmsize,msg1->filename,
	      msg2->sg.segname, msg2->sg.vmaddr,msg2->sg.vmsize,msg2->filename);
}

/*
 * print_load_map() is called from layout() if a load map is requested.
 */
static
void
print_load_map(void)
{
    long i;
    struct merged_segment *msg;
    struct merged_section *ms;
    struct common_symbol *common_symbol;

	print("Load map for: %s\n", outputfile);
	print("Segment name     Section name     Address    Size\n");
	for(msg = merged_segments; msg ; msg = msg->next){
	    print("%-16s %-16s 0x%08x 0x%08x\n",
		   msg->sg.segname, "", msg->sg.vmaddr, msg->sg.vmsize);
	    for(ms = msg->content_sections; ms ; ms = ms->next){
		print("%-16s %-16s 0x%08x 0x%08x",
		       ms->s.segname, ms->s.sectname, ms->s.addr, ms->s.size);
		if(ms->contents_filename)
		    print(" from the file: %s\n", ms->contents_filename);
		else{
		    if(ms->order_load_maps){
			print("\n");
			for(i = 0; i < ms->norder_load_maps; i++){
			    print("\t\t\t\t  0x%08x 0x%08x ",
			      fine_reloc_output_offset(
				ms->order_load_maps[i].section_map,
				ms->order_load_maps[i].value -
				ms->order_load_maps[i].section_map->s->addr) +
				ms->order_load_maps[i].section_map->
							output_section->s.addr,
				ms->order_load_maps[i].size);
			    if(ms->order_load_maps[i].archive_name != NULL)
				print("%s:",
					   ms->order_load_maps[i].archive_name);
			    print("%s:%s\n",
				  ms->order_load_maps[i].object_name,
				  ms->order_load_maps[i].symbol_name);
			}
		    }
		    else{
			print("\n");
			print_load_map_for_objects(ms);
		    }
		}
	    }
	    for(ms = msg->zerofill_sections; ms ; ms = ms->next){
		print("%-16s %-16s 0x%08x 0x%08x\n",
		       ms->s.segname, ms->s.sectname, ms->s.addr, ms->s.size);
		if(ms->order_load_maps){
		    for(i = 0; i < ms->norder_load_maps; i++){
			print("\t\t\t\t  0x%08x 0x%08x ",
			    fine_reloc_output_offset(
				ms->order_load_maps[i].section_map,
				ms->order_load_maps[i].value -
				ms->order_load_maps[i].section_map->s->addr) +
				ms->order_load_maps[i].section_map->
							output_section->s.addr,
				ms->order_load_maps[i].size);
			if(ms->order_load_maps[i].archive_name != NULL)
			    print("%s:", ms->order_load_maps[i].archive_name);
			print("%s:%s\n",
			      ms->order_load_maps[i].object_name,
			      ms->order_load_maps[i].symbol_name);
		    }
		}
		else{
		    print_load_map_for_objects(ms);
		    if(common_load_map.common_ms == ms){
			common_symbol = common_load_map.common_symbols;
			for(i = 0; i < common_load_map.ncommon_symbols; i++){
			    print("\t\t\t\t  0x%08x 0x%08x symbol: %s\n",
			       common_symbol->merged_symbol->nlist.n_value,
			       common_symbol->common_size,
			       common_symbol->merged_symbol->nlist.n_un.n_name);
			    common_symbol++;
			}
			common_load_map.common_ms = NULL;
			common_load_map.ncommon_symbols = 0;
			free(common_load_map.common_symbols);
		    }
		}
	    }
	    if(msg->next != NULL)
		print("\n");
	}

	if(base_obj){
	    print("\nLoad map for base file: %s\n", base_obj->file_name);
	    print("Segment name     Section name     Address    Size\n");
	    for(msg = base_obj_segments; msg ; msg = msg->next){
		print("%-16s %-16s 0x%08x 0x%08x\n",
		       msg->sg.segname, "", msg->sg.vmaddr, msg->sg.vmsize);
	    }
	}

	if(fvmlib_segments != NULL){
	    print("\nLoad map for fixed VM shared libraries\n");
	    print("Segment name     Section name     Address    Size\n");
	    for(msg = fvmlib_segments; msg ; msg = msg->next){
		print("%-16s %-16s 0x%08x 0x%08x %s\n",
		       msg->sg.segname, "", msg->sg.vmaddr, msg->sg.vmsize,
		       msg->filename);
	    }
	}
}

/*
 * print_load_map_for_objects() prints the load map for each object that has
 * a non-zero size in the specified merge section.
 */
static
void
print_load_map_for_objects(
struct merged_section *ms)
{
    long i, j, k;
    struct object_list *object_list, **p;
    struct object_file *object_file;
    struct fine_reloc *fine_relocs;

	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    for(i = 0; i < object_list->used; i++){
		object_file = &(object_list->object_files[i]);
		if(object_file == base_obj)
		    continue;
		for(j = 0; j < object_file->nsection_maps; j++){
		    if(object_file->section_maps[j].output_section == ms &&
		       object_file->section_maps[j].s->size != 0){

		        if(object_file->section_maps[j].nfine_relocs != 0){
			    fine_relocs =
				object_file->section_maps[j].fine_relocs;
			    for(k = 0;
				k < object_file->section_maps[j].nfine_relocs;
				k++){
				print("       (input address 0x%08x) 0x%08x "
				      "0x%08x ",
				      object_file->section_maps[j].s->addr +
					fine_relocs[k].input_offset,
				      ms->s.addr +
					fine_relocs[k].output_offset,
				      k == object_file->section_maps[j].
							     nfine_relocs - 1 ?
				      object_file->section_maps[j].s->size -
					fine_relocs[k].input_offset :
				      fine_relocs[k + 1].input_offset -
					fine_relocs[k].input_offset);
				print_obj_name(object_file);
				print("\n");
			    }
			}
			else{
			    print("\t\t\t\t  0x%08x 0x%08x ",
				   ms->s.addr +
				   object_file->section_maps[j].offset,
				   object_file->section_maps[j].s->size);
			    print_obj_name(object_file);
			    print("\n");
			}
		    }
		}
	    }
	}
}
#endif !defined(RLD)

#ifdef DEBUG
/*
 * print_mach_header() prints the output file's mach header.  For debugging.
 */
void
print_mach_header(void)
{
	print("Mach header for output file\n");
	print("    magic = 0x%x\n", output_mach_header.magic);
	print("    cputype = %d\n", output_mach_header.cputype);
	print("    cpusubtype = %d\n", output_mach_header.cpusubtype);
	print("    filetype = %d\n", output_mach_header.filetype);
	print("    ncmds = %d\n", output_mach_header.ncmds);
	print("    sizeofcmds = %d\n", output_mach_header.sizeofcmds);
	print("    flags = %d\n", output_mach_header.flags);
}

/*
 * print_symtab_info() prints the output file's symtab command.  For
 * debugging.
 */
void
print_symtab_info(void)
{
	print("Symtab info for output file\n");
	print("    cmd = %d\n", output_symtab_info.symtab_command.cmd);
	print("    cmdsize = %d\n", output_symtab_info.symtab_command.cmdsize);
	print("    nsyms = %d\n", output_symtab_info.symtab_command.nsyms);
	print("    symoff = %d\n", output_symtab_info.symtab_command.symoff);
	print("    strsize = %d\n", output_symtab_info.symtab_command.strsize);
	print("    stroff = %d\n", output_symtab_info.symtab_command.stroff);
}

/*
 * print_thread_info() prints the output file's thread information.  For
 * debugging.
 */
void
print_thread_info(void)
{
	print("Thread info for output file\n");
	print("    flavor = %d\n", output_thread_info.flavor);
	print("    count = %d\n", output_thread_info.count);
	print("    entry_point = 0x%x", output_thread_info.entry_point);
	if(output_thread_info.entry_point != NULL)
	    print(" (0x%x)\n", *output_thread_info.entry_point);
	else
	    print("\n");
}
#endif DEBUG

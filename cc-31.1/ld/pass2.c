#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines that drives pass2 of the link-editor.  In
 * pass2 the output is created and written.  The sections from the input files
 * are copied into the output and relocated.  The headers, relocation entries,
 * symbol table and string table are all copied into the output file.
 */
#include <libc.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/loader.h>
#include <mach.h>
#include <nlist.h>
#include <reloc.h>

#include "ld.h"
#include "objects.h"
#include "fvmlibs.h"
#include "sections.h"
#include "symbols.h"
#include "layout.h"
#include "pass2.h"
#include "sets.h"

/*
 * The total size of the output file and the memory buffer for the output file.
 */
long output_size = 0;
char *output_addr = NULL;

/*
 * This is used to setting the SG_NORELOC flag in the segment flags correctly.
 * This is an array of pointers to the merged sections in the output file that
 * is used by the relocation routines to set the field 'referenced' in the
 * merged section structure by indexing this array (directly without subtracting
 * one from the section number) with the section number of a merged symbol that
 * is refered to in a relocation entry.  The array is created by the routine
 * create_output_sections_array() in here. Then after the 'referenced' field is
 * set by the relocation routines (like generic_reloc() in generic_reloc.c) and
 * the 'relocated' field is set by output_section() in sections.c then the
 * routine set_SG_NORELOC_flags() in here can use these two fields to set the
 * SG_NORELOC flag in the segments that have no relocation to or for them.
 */
struct merged_section **output_sections = NULL;

#ifndef RLD
/* the file descriptor of the output file */
static int fd = 0;

/*
 * This structure is used to describe blocks of the output file that are flushed
 * to the disk file with output_flush.  It is kept in an ordered list starting
 * with output_blocks.
 */
static struct block {
    long offset;	/* starting offset of this block */
    long size;		/* size of this block */
    long written_offset;/* first page offset after starting offset */
    long written_size;	/* size of written area from written_offset */
    struct block *next; /* next block in the list */
} *output_blocks;

static void setup_output_flush(void);
static void final_output_flush(void);
#ifdef DEBUG
static void print_block_list(void);
#endif DEBUG
static struct block *get_block(void);
static void remove_block(struct block *block);
static long trunc(long v, unsigned long r);
#endif !defined(RLD)
static void create_output_sections_array(void);
static void set_SG_NORELOC_flags(void);
static void output_headers(void);

/*
 * pass2() creates the output file and the memory buffer to create the file
 * into.  It drives the process to get everything copied into the buffer for
 * the output file.  It then writes the output file and deallocates the buffer.
 */ 
void
pass2(void)
{
    long i, j;
    struct object_list *object_list, **p;
#ifndef RLD
    int mode;
    struct stat stat_buf;
    kern_return_t r;

	/*
	 * Create the output file.  The unlink() is done to handle the problem
	 * when the outputfile is not writable but the directory allows the
	 * file to be removed (since the file may not be there the return code
	 * of the unlink() is ignored).
	 */
	(void)unlink(outputfile);
	if((fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1)
	    system_fatal("can't create output file: %s", outputfile);
	if(fstat(fd, &stat_buf) == -1)
	    system_fatal("can't stat file: %s", outputfile);
	/*
	 * Turn the execute bits on or off depending if there are any undefined
	 * symbols in the output file.  If the file existed before the above
	 * open() call the creation mode in that call would have been ignored
	 * so it has to be set explicitly in any case.
	 */
	if(output_mach_header.flags & MH_NOUNDEFS)
	    mode = (stat_buf.st_mode & 0777) | 0111;
	else
	    mode = (stat_buf.st_mode & 0777) & ~0111;
	if(fchmod(fd, mode) == -1)
	    system_fatal("can't execution permissions output file: %s",
			 outputfile);

	/*
	 * Create the buffer to copy the parts of the output file into.
	 */
	if((r = vm_allocate(task_self(), (vm_address_t *)&output_addr,
			    output_size, TRUE)) != KERN_SUCCESS)
	    mach_fatal(r, "can't vm_allocate() buffer for output file of size "
		       "%d", output_size);

	/*
	 * Set up for flushing pages to the output file as they fill up.
	 */
	if(flush)
	    setup_output_flush();
#endif !defined(RLD)

	/*
	 * Create the array of pointers to merged sections in the output file
	 * so the relocation routines can use it to set the 'referenced' fields
	 * in the merged section structures.
	 */
	create_output_sections_array();

	/*
	 * Copy the merged literal sections and the sections created from files
	 * into the output object file.
	 */
	output_literal_sections();
#ifndef RLD
	output_sections_from_files();
#endif !defined(RLD)

	/*
	 * For each non-literal content section in each object file loaded 
	 * relocate it into the output file (along with the relocation entries).
	 * Then relocate local symbols into the output file for the loaded
	 * objects.
	 */
	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    for(i = 0; i < object_list->used; i++){
		cur_obj = &(object_list->object_files[i]);
		/* print the object file name if tracing */
		if(trace){
		    print_obj_name(cur_obj);
		    print("\n");
		}
		if(cur_obj != base_obj){
		    for(j = 0; j < cur_obj->nsection_maps; j++){
#ifdef RLD
			if(cur_obj->set_num == cur_set)
#endif RLD
			    if(cur_obj->section_maps[j].s->flags == 0){
				output_section(&(cur_obj->section_maps[j]));
			    }
#ifdef RLD
			if(cur_obj->section_maps[j].nfine_relocs != 0){
			    free(cur_obj->section_maps[j].fine_relocs);
			    cur_obj->section_maps[j].fine_relocs = NULL;
			    cur_obj->section_maps[j].nfine_relocs = 0;
			}
#endif RLD
		    }
		}
		output_local_symbols();
#ifdef RLD
		if(cur_obj->nundefineds != 0){
		    free(cur_obj->undefined_maps);
		    cur_obj->undefined_maps = NULL;
		    cur_obj->nundefineds = 0;
		}
#endif RLD
	    }
	}

	/*
	 * Set the SG_NORELOC flag in the segments that had no relocation to
	 * or for them.
	 */
	set_SG_NORELOC_flags();

	/*
	 * Copy the merged symbol table into the output file.
	 */
	output_merged_symbols();

	/*
	 * Copy the headers into the output file.
	 */
	output_headers();

#ifndef RLD
	if(flush){
	    /*
	     * Flush the sections that have been scatter loaded.
	     */
	    flush_scatter_copied_sections();
	    /*
	     * flush the remaining part of the object file that is not a full
	     * page.
	     */
	    final_output_flush();
	}
	else{
	    /*
	     * Write the entire object file.
	     */
	    if(write(fd, output_addr, output_size) != output_size)
		system_fatal("can't write output file");
	    if(close(fd) == -1)
		system_fatal("can't close output file");

	    if((r = vm_deallocate(task_self(), (vm_address_t)output_addr,
				  output_size)) != KERN_SUCCESS)
		mach_fatal(r, "can't vm_deallocate() buffer for output file");
	}
#endif RLD
}


/*
 * create_output_sections_array() creates the output_sections array and fills
 * it in with the pointers to the merged sections in the output file.  This 
 * is used by the relocation routines to set the field 'referenced' in the
 * merged section structure by indexing this array (directly without subtracting
 * one from the section number) with the section number of a merged symbol that
 * is refered to in a relocation entry.
 */
void
create_output_sections_array(void)
{
    long i, nsects;
    struct merged_segment **p, *msg;
    struct merged_section **content, **zerofill, *ms;

	nsects = 1;
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    nsects += msg->sg.nsects;
	    p = &(msg->next);
	}

	output_sections = (struct merged_section **)
			  allocate(nsects * sizeof(struct merged_section *));

	i = 1;
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		output_sections[i++] = ms;
		content = &(ms->next);
	    }
	    zerofill = &(msg->zerofill_sections);
	    while(*zerofill){
		ms = *zerofill;
		output_sections[i++] = ms;
		zerofill = &(ms->next);
	    }
	    p = &(msg->next);
	}
}

/*
 * set_SG_NORELOC_flags() sets the SG_NORELOC flag in the segment that have no
 * relocation to or from them.  This uses the fields 'referenced' and
 * 'relocated' in the merged section structures.  The array that was created
 * by the routine create_output_sections_array() to help set the above
 * 'referenced' field is deallocated in here.
 */
static
void
set_SG_NORELOC_flags(void)
{
    struct merged_segment **p, *msg;
    struct merged_section **content, **zerofill, *ms;
    enum bool relocated, referenced;

	free(output_sections);
	output_sections = NULL;

	p = &merged_segments;
	while(*p){
	    relocated = FALSE;
	    referenced = FALSE;
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->relocated == TRUE)
		    relocated = TRUE;
		if(ms->referenced == TRUE)
		    referenced = TRUE;
		content = &(ms->next);
	    }
	    zerofill = &(msg->zerofill_sections);
	    while(*zerofill){
		ms = *zerofill;
		/* a zero fill section can't be relocated */
		if(ms->referenced == TRUE)
		    referenced = TRUE;
		zerofill = &(ms->next);
	    }
	    if(relocated == FALSE && referenced == FALSE)
		msg->sg.flags |= SG_NORELOC;
	    p = &(msg->next);
	}
}

#ifndef RLD
/*
 * setup_output_flush() flushes the gaps between things in the file that are
 * holes created by alignment.  This must stay in lock step with the layout
 * routine that lays out the file (layout_segments() in layout.c).
 */
static
void
setup_output_flush(void)
{
    long offset;
    struct merged_segment **p, *msg;
    struct merged_section **content, *ms;

	offset = sizeof(struct mach_header) + output_mach_header.sizeofcmds;

	/* the offsets to the contents of the sections */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->s.size != 0){
		    if(ms->s.offset != offset)
			output_flush(offset, ms->s.offset - offset);
		    offset = ms->s.offset + ms->s.size;
		}
		content = &(ms->next);
	    }
	    p = &(msg->next);
	}

	/* the offsets to the relocation entries */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->s.nreloc != 0){
		    if(ms->s.reloff != offset)
			output_flush(offset, ms->s.reloff - offset);
		    offset = ms->s.reloff +
			     ms->s.nreloc * sizeof(struct relocation_info);
		}
		content = &(ms->next);
	    }
	    p = &(msg->next);
	}

	if(strip_level != STRIP_ALL){
	    /* the offsets to the symbol table and string table */
	    if(output_symtab_info.symtab_command.symoff != offset)
		output_flush(offset, output_symtab_info.symtab_command.symoff -
				     offset);
	    offset = output_symtab_info.symtab_command.symoff +
		     output_symtab_info.symtab_command.nsyms *
							sizeof(struct nlist);

	    /*
	     * This is flushed to output_symtab_info.symtab_command.stroff plus
	     * output_symtab_info.output_strsize and not just to
	     * output_symtab_info.symtab_command.stroff because the first byte
	     * can't be used to store a string because a symbol with a string
	     * offset of zero (nlist.n_un.n_strx == 0) is defined to be a symbol
	     * with a null name "".  So this byte(s) have to be flushed.
	     */
	    if(output_symtab_info.symtab_command.stroff +
	       output_symtab_info.output_strsize != offset)
		output_flush(offset, output_symtab_info.symtab_command.stroff +
				     output_symtab_info.output_strsize -offset);
	    offset = output_symtab_info.symtab_command.stroff +
		     output_symtab_info.symtab_command.strsize;
	}

	/* the offset to the end of the file */
	if(offset != output_size)
	    output_flush(offset, output_size - offset);
}

/*
 * output_flush() takes an offset and a size of part of the output file, known
 * in the comments as the new area, and causes any fully flushed pages to be
 * written to the output file the new area in combination with previous areas
 * creates.  The data structure output_blocks has ordered blocks of areas that
 * have been flushed which are maintained by this routine.  Any area can only
 * be flushed once and an error will result is the new area overlaps with a
 * previously flushed area.
 *
 * The goal of this is to again minimize the number of dirty pages the link
 * editor has and hopfully improve performance in a memory starved system and
 * to prevent these pages to be written to the swap area when they could just be
 * written to the output file (if only external pagers worked well ...).
 */
void
output_flush(
long offset,
long size)
{ 
    long write_offset, write_size;
    struct block **p, *block, *before, *after;
    kern_return_t r;

	if(flush == FALSE)
	    return;

	if(offset < 0 || size < 0 || offset + size > output_size)
	    fatal("internal error: output_flush(offset = %d, size = %d) out of "
		  "range for output_size = %d", offset, size, output_size);

#ifdef DEBUG
	if(debug & 0x1000)
	    print_block_list();
	if(debug & 0x800)
	    print("output_flush(offset = %d, size %d)", offset, size);
#endif DEBUG

	if(size == 0){
#ifdef DEBUG
	if(debug & 0x800)
	    print("\n");
#endif DEBUG
	    return;
	}

	/*
	 * Search through the ordered output blocks to find the block before the
	 * new area and after the new area if any exist.
	 */
	before = NULL;
	after = NULL;
	p = &(output_blocks);
	while(*p){
	    block = *p;
	    if(offset < block->offset){
		after = block;
		break;
	    }
	    else{
		before = block;
	    }
	    p = &(block->next);
	}

	/*
	 * Check for overlap of the new area with the block before and after the
	 * new area if there are such blocks.
	 */
	if(before != NULL){
	    if(before->offset + before->size > offset)
		fatal("internal error: output_flush(offset = %d, size = %d) "
		      "overlaps with flushed block(offset = %d, size = %d)",
		      offset, size, before->offset, before->size);
	}
	if(after != NULL){
	    if(offset + size > after->offset)
		fatal("internal error: output_flush(offset = %d, size = %d) "
		      "overlaps with flushed block(offset = %d, size = %d)",
		      offset, size, after->offset, after->size);
	}

	/*
	 * Now see how the new area fits in with the blocks before and after it
	 * (that is does it touch both, one or the other or neither blocks).
	 * For each case first the offset and size to write (write_offset and
	 * write_size) are set for the area of full pages that can now be
	 * written from the block.  Then the area written in the block
	 * (->written_offset and ->written_size) are set to reflect the total
	 * area in the block now written.  Then offset and size the block
	 * refers to (->offset and ->size) are set to total area of the block.
	 * Finally the links to others blocks in the list are adjusted if a
	 * block is added or removed.
	 *
	 * See if there is a block before the new area and the new area
	 * starts at the end of that block.
	 */
	if(before != NULL && before->offset + before->size == offset){
	    /*
	     * See if there is also a block after the new area and the new area
	     * ends at the start of that block.
	     */
	    if(after != NULL && offset + size == after->offset){
		/*
		 * This is the case where the new area exactly fill the area
		 * between two existing blocks.  The total area is folded into
		 * the block before the new area and the block after the new
		 * area is removed from the list.
		 */
		if(before->offset == 0 && before->written_size == 0){
		    write_offset = 0;
		    before->written_offset = 0;
		}
		else
		    write_offset =before->written_offset + before->written_size;
		if(after->written_size == 0)
		    write_size = trunc(after->offset + after->size -
				       write_offset, host_pagesize);
		else
		    write_size = trunc(after->written_offset - write_offset,
				       host_pagesize);
		if(write_size != 0){
		    before->written_size += write_size;
		}
		if(after->written_size != 0)
		    before->written_size += after->written_size;
		before->size += size + after->size;

		/* remove the block after the new area */
		before->next = after->next;
		remove_block(after);
	    }
	    else{
		/*
		 * This is the case where the new area starts at the end of the
		 * block just before it but does not end where the block after
		 * it (if any) starts.  The new area is folded into the block
		 * before the new area.
		 */
		write_offset = before->written_offset + before->written_size;
		write_size = trunc(offset + size - write_offset, host_pagesize);
		if(write_size != 0)
		    before->written_size += write_size;
		before->size += size;
	    }
	}
	/*
	 * See if the new area and the new area ends at the start of the block
	 * after it (if any).
	 */
	else if(after != NULL && offset + size == after->offset){
	    /*
	     * This is the case where the new area ends at the begining of the
	     * block just after it but does not start where the block before it.
	     * (if any) ends.  The new area is folded into this block after the
	     * new area.
	     */
	    write_offset = round(offset, host_pagesize);
	    if(after->written_size == 0)
		write_size = trunc(after->offset + after->size - write_offset,
				   host_pagesize);
	    else
		write_size = trunc(after->written_offset - write_offset,
				   host_pagesize);
	    if(write_size != 0){
		after->written_offset = write_offset;
		after->written_size += write_size;
	    }
	    after->offset = offset;
	    after->size += size;
	}
	else{
	    /*
	     * This is the case where the new area neither starts at the end of
	     * the block just before it (if any) or ends where the block after
	     * it (if any) starts.  A new block is created and the new area is
	     * is placed in it.
	     */
	    write_offset = round(offset, host_pagesize);
	    write_size = trunc(offset + size - write_offset, host_pagesize);
	    block = get_block();
	    block->offset = offset;
	    block->size = size;
	    block->written_offset = write_offset;
	    block->written_size = write_size;
	    /*
	     * Insert this block in the ordered list in the correct place.
	     */
	    if(before != NULL){
		block->next = before->next;
		before->next = block;
	    }
	    else{
		block->next = output_blocks;
		output_blocks = block;
	    }
	}

	/*
	 * Now if there are full pages to write write them to the output file.
	 */
	if(write_size != 0){
#ifdef DEBUG
	if((debug & 0x800) || (debug & 0x400))
	    print(" writing (write_offset = %d write_size = %d)\n",
		   write_offset, write_size);
#endif DEBUG
	    lseek(fd, write_offset, L_SET);
	    if(write(fd, output_addr + write_offset, write_size) != write_size)
		system_fatal("can't write to output file");
	    if((r = vm_deallocate(task_self(), (vm_address_t)(output_addr +
				  write_offset), write_size)) != KERN_SUCCESS)
		mach_fatal(r, "can't vm_deallocate() buffer for output file");
	}
#ifdef DEBUG
	else{
	    if(debug & 0x800)
		print(" no write\n");
	}
#endif DEBUG
}

/*
 * final_output_flush() flushes the last part of the last page of the object
 * file if it does not round out to exactly a page.
 */
static
void
final_output_flush(void)
{ 
    struct block *block;
    long write_offset, write_size;
    kern_return_t r;

#ifdef DEBUG
	/* The compiler "warning: `write_offset' may be used uninitialized in */
	/* this function" can safely be ignored */
	write_offset = 0;
	if((debug & 0x800) || (debug & 0x400)){
	    print("final_output_flush block_list:\n");
	    print_block_list();
	}
#endif DEBUG

	write_size = 0;
	block = output_blocks;
	if(block != NULL){
	    if(block->offset != 0)
		fatal("internal error: first block not at offset 0");
	    if(block->written_size != 0){
		if(block->written_offset != 0)
		    fatal("internal error: first block written_offset not 0");
		write_offset = block->written_size;
		write_size = block->size - block->written_size;
	    }
	    else{
		write_offset = block->offset;
		write_size = block->size;
	    }
	    if(block->next != NULL)
		fatal("internal error: more then one block in final list");
	}
	if(write_size != 0){
#ifdef DEBUG
	    if((debug & 0x800) || (debug & 0x400))
		printf(" writing (write_offset = %d write_size = %d)\n",
		       write_offset, write_size);
#endif DEBUG
	    lseek(fd, write_offset, L_SET);
	    if(write(fd, output_addr + write_offset, write_size) != write_size)
		system_fatal("can't write to output file");
	    if((r = vm_deallocate(task_self(), (vm_address_t)(output_addr +
				  write_offset), write_size)) != KERN_SUCCESS)
		mach_fatal(r, "can't vm_deallocate() buffer for output file");
	}
}

#ifdef DEBUG
/*
 * print_block_list() prints the list of blocks.  Used for debugging.
 */
static
void
print_block_list(void)
{
    struct block **p, *block;

	p = &(output_blocks);
	if(*p == NULL)
	    print("Empty block list\n");
	while(*p){
	    block = *p;
	    print("block 0x%x\n", block);
	    print("    offset %d\n", block->offset);
	    print("    size %d\n", block->size);
	    print("    written_offset %d\n", block->written_offset);
	    print("    written_size %d\n", block->written_size);
	    print("    next 0x%x\n", block->next);
	    p = &(block->next);
	}
}
#endif DEBUG

/*
 * get_block() returns a pointer to a new block.  This could be done by
 * allocating block of these placing them on a free list and and handing them
 * out.  The maximum number of blocks needed would be one for each content
 * section, one for each section that has relocation entries (if saving them)
 * and one for the symbol and string table.  For the initial release of this
 * code this number is typicly around 8 it is not a big win so each block is
 * just allocated and free'ed.
 */
static
struct block *
get_block(void)
{
    struct block *block;

	block = allocate(sizeof(struct block));
	return(block);
}

/*
 * remove_block() throws away the block specified.  See comments in get_block().
 */
static
void
remove_block(
struct block *block)
{
	free(block);
}

/*
 * trunc() truncates the value 'v' to the power of two value 'r'.  If v is
 * less than zero it returns zero.
 */
static
long
trunc(
long v,
unsigned long r)
{
	if(v < 0)
	    return(0);
	return(v & ~(r - 1));
}
#endif !defined(RLD)

/*
 * output_headers() copys the headers of the object file into the buffer for
 * the output file.
 */
static
void
output_headers(void)
{
    long header_offset;
    struct merged_segment **p, *msg;
    struct merged_section **content, **zerofill, *ms;
#ifndef RLD
    struct merged_fvmlib **q, *mfl;
#endif !defined(RLD)

	header_offset = 0;

	/* first the mach header */
	memcpy(output_addr, &output_mach_header, sizeof(struct mach_header));
	header_offset += sizeof(struct mach_header);

	/* next the segment load commands (and section structures) */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    memcpy(output_addr + header_offset, &(msg->sg),
		   sizeof(struct segment_command));
	    header_offset += sizeof(struct segment_command);
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		memcpy(output_addr + header_offset, &(ms->s),
		       sizeof(struct section));
		header_offset += sizeof(struct section);
		content = &(ms->next);
	    }
	    zerofill = &(msg->zerofill_sections);
	    while(*zerofill){
		ms = *zerofill;
		memcpy(output_addr + header_offset, &(ms->s),
		       sizeof(struct section));
		header_offset += sizeof(struct section);
		zerofill = &(ms->next);
	    }
	    p = &(msg->next);
	}

#ifndef RLD
	/* next the fixed VM shared library load commands */
	q = &merged_fvmlibs;
	while(*q){
	    mfl = *q;
	    memcpy(output_addr + header_offset, mfl->fl, mfl->fl->cmdsize);
	    header_offset += mfl->fl->cmdsize;
	    q = &(mfl->next);
	}
#endif !defined(RLD)

	/* next the symbol table load command */
	memcpy(output_addr + header_offset,
	       &(output_symtab_info.symtab_command),
	       output_symtab_info.symtab_command.cmdsize);
	header_offset += output_symtab_info.symtab_command.cmdsize;

	/* next the thread command if the output file has one */
	if(output_thread_info.thread_in_output == TRUE){
	    /* the thread command itself */
	    memcpy(output_addr + header_offset,
		   &(output_thread_info.thread_command),
		   sizeof(struct thread_command));
	    header_offset += sizeof(struct thread_command);
	    /* the flavor of the thread state */
	    memcpy(output_addr + header_offset, &(output_thread_info.flavor),
		   sizeof(long));
	    header_offset += sizeof(long);
	    /* the count of longs of the thread state */
	    memcpy(output_addr + header_offset, &(output_thread_info.count),
		   sizeof(long));
	    header_offset += sizeof(long);
	    /* the thread state */
	    memcpy(output_addr + header_offset, output_thread_info.state,
		   output_thread_info.count * sizeof(long));
	    header_offset += output_thread_info.count * sizeof(long);
	}
#ifndef RLD
	output_flush(0, sizeof(struct mach_header) +
			output_mach_header.sizeofcmds);
#endif !defined(RLD)
}

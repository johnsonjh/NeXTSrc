#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines that drives pass1 of the link-editor.  In
 * pass1 the objects needed from archives are selected for loading and all of
 * the things that need to be merged from the input objects are merged into
 * tables (for output and relocation on the second pass).
 */
#include <libc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/loader.h>
#include <nlist.h>
#include <reloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mach.h>
#include <ar.h>
#include <ranlib.h>
#include <ldsyms.h>

#include "ld.h"
#include "pass1.h"
#include "objects.h"
#include "fvmlibs.h"
#include "sections.h"
#include "symbols.h"
#include "sets.h"

#ifndef RLD
/* the user specified directories to search for -lx names, and the number
   of them */
char **search_dirs = NULL;
long nsearch_dirs = 0;

/* the standard directories to search for -lx names */
char *standard_dirs[] = {
    "/lib/",
    "/usr/lib/",
    "/usr/local/lib/",
    NULL
};

/* the pointer to the head of the base object file's segments */
struct merged_segment *base_obj_segments = NULL;
#endif !defined(RLD)

/*
 * This is a pointer to the strings of the table of contents of a library.  It
 * has to be static and not local so that pass1() can set it and
 * ranlib_bsearch() can use it.  This is done instead of assigning to ran_name
 * so that the library can be mapped read only and thus not get dirty and maybe
 * written to the swap area by the kernel.
 */
static char *strings = NULL;

static int ranlib_bsearch(const char *symbol_name, const struct ranlib *ran);
static void check_cur_obj(void);
static void check_size_offset(unsigned long size, unsigned long offset,
			      unsigned long align, char *size_str,
			      char *offset_str, long cmd);
static void check_size_offset_sect(unsigned long size, unsigned long offset,
				   unsigned long align, char *size_str,
				   char *offset_str, long cmd, long sect,
				   char *segname, char *sectname);
#ifndef RLD
static void collect_base_obj_segments(void);
static void add_base_obj_segment(struct segment_command *sg, char *filename);
static char *mkstr(const char *args, ...);
#endif !defined(RLD)

/*
 * pass1() is called from main() and is passed the name of a file and a flag
 * indicating if it is a path searched abbrevated file name (a -lx argument).
 *
 * If the name is a path searched abbrevated file name (of the form "-lx")
 * then it is searched for in the search_dirs list (created from the -Ldir
 * arguments) and then in the list of standard_dirs.  The string "x" of the
 * "-lx" argument will be converted to "libx.a" if the string does not end in
 * ".o", in which case it is left alone.
 *
 * If the file is the object file for a the base of an incremental load then
 * base_name is TRUE and the pointer to the object_file structure for it,
 * base_obj, is set when it is allocated.
 * 
 * If the file turns out to be just a plain object then merge() is called to
 * merge its symbolic information and it will be unconditionally loaded.
 *
 * Object files in archives are loaded only if they resolve currently undefined
 * references.
 */
void
pass1(
char *name,
enum bool lname,
enum bool base_name)
{
    int fd;
    long i, j;
    char *file_name;
#ifndef RLD
    char *p;
    enum bool libname;
#endif !defined(RLD)
    struct stat stat_buf;
    kern_return_t r;
    long file_size;
    char *file_addr;

    struct ar_hdr *symdef_ar_hdr;
    long offset, symdef_length, nranlibs, string_size;
    struct ranlib *ranlibs, *ranlib;
    struct undefined_list *undefined;
    struct merged_symbol *merged_symbol;
    enum bool member_loaded;

#ifdef DEBUG
	/* The compiler "warning: `file_name' may be used uninitialized in */
	/* this function" can safely be ignored */
	file_name = NULL;
#endif DEBUG

	fd = -1;
#ifndef RLD
	if(lname){
	    if(name[0] != '-' || name[1] != 'l')
		fatal("Internal error: pass1() called with name of: %s and "
		      "lname == TRUE", name);
	    p = &name[2];
	    p = strrchr(p, '.');
	    if(p != NULL && strcmp(p, ".o") == 0){
		p = &name[2];
		libname = FALSE;
	    }
	    else{
		p = mkstr("lib", &name[2], ".a", NULL);
		libname = TRUE;
	    }
	    for(i = 0; i < nsearch_dirs ; i++){
		file_name = mkstr(search_dirs[i], "/", p, NULL);
		if((fd = open(file_name, O_RDONLY, 0)) != -1)
		    break;
		free(file_name);
	    }
	    if(fd == -1){
		for(i = 0; standard_dirs[i] != NULL ; i++){
		    file_name = mkstr(standard_dirs[i], p, NULL);
		    if((fd = open(file_name, O_RDONLY, 0)) != -1)
			break;
		    free(file_name);
		}
		if(fd == -1)
		    fatal("Can't locate file for: %s", name);
	    }
	    if(libname)
		free(p);
	}
	else
#endif !defined(RLD)
	{
	    if((fd = open(name, O_RDONLY, 0)) == -1){
		system_error("Can't open: %s", name);
		return;
	    }
	    file_name = name;
	}

	/*
	 * Now that the file_name has been determined and opened get it into
	 * memory by mapping it.
	 */
	if(fstat(fd, &stat_buf) == -1){
	    system_fatal("Can't stat file: %s", file_name);
	    close(fd);
	    return;
	}
	file_size = stat_buf.st_size;
	/*
	 * For some reason mapping files with zero size fails so it has to
	 * be handled specially.
	 */
	if(file_size == 0){
	    error("file: %s is empty (not an object or archive)", file_name);
	    close(fd);
	    return;
	}
	if((r = map_fd((int)fd, (vm_offset_t)0, (vm_offset_t *)&file_addr,
	    (boolean_t)TRUE, (vm_size_t)file_size)) != KERN_SUCCESS){
	    close(fd);
	    mach_fatal(r, "can't map file: %s", file_name);
	}
	if((r = vm_protect(task_self(), (vm_address_t)file_addr, file_size,
			   FALSE, VM_PROT_READ)) != KERN_SUCCESS){
	    close(fd);
	    mach_fatal(r, "can't make memory for mapped file: %s read-only",
		       file_name);
	}
	close(fd);

	/*
	 * Determine what type of file it is (archive or object file).
	 */
	if(SARMAG > file_size){
	    error("truncated or malformed object or library: %s (file size too "
		  "small)", file_name);
	    return;
	}
	offset = SARMAG;
	if(strncmp(file_addr, ARMAG, SARMAG) != 0){
	    /* This is an object file so it is unconditionally loaded */
	    cur_obj = new_object_file();
#ifdef RLD
	    cur_obj->file_name = allocate(strlen(file_name) + 1);
	    strcpy(cur_obj->file_name, file_name);
#else
	    cur_obj->file_name = file_name;
#endif RLD
	    cur_obj->obj_addr = file_addr;
	    cur_obj->obj_size = file_size;
#ifndef RLD
	    /*
	     * If this is the base file of an incremental link then set the
	     * pointer to the object file.
	     */
	    if(base_name == TRUE)
		base_obj = cur_obj;
#endif !defined(RLD)

	    merge();

#ifndef RLD
	    /*
	     * If this is the base file of an incremental link then collect it's
	     * segments for overlap checking.
	     */
	    if(base_name == TRUE)
		collect_base_obj_segments();
#endif !defined(RLD)
	    return;
	}
#ifndef RLD
	/*
	 * This is an archive so conditionally load those objects that defined
	 * currently undefined symbols.
	 */
	if(base_name)
	    fatal("base file of incremental link (argument of -A): %s should't "
		  "be an archive", file_name);
#endif !defined(RLD)

	/*
	 * If there are no undefined symbols then the archive doesn't have
	 * to be searched because archive members are only loaded to resolve
	 * undefined references.
	 */
	if(undefined_list.next == &undefined_list)
	    return;

#ifdef RLD
	new_archive(file_name, file_addr, file_size);
#endif RLD
	/*
	 * The file is an archive so get the symdef file
	 */
	if(offset == file_size){
	    warning("empty archive: %s (can't load from it)", file_name);
	    return;
	}
	if(offset + sizeof(struct ar_hdr) > file_size){
	    error("truncated or malformed archive: %s (archive header of first "
		  "member extends past the end of the file, can't load from "
		  " it)", file_name);
	    return;
	}
	symdef_ar_hdr = (struct ar_hdr *)(file_addr + offset);
	offset += sizeof(struct ar_hdr);
	if(strncmp(symdef_ar_hdr->ar_name, SYMDEF, sizeof(SYMDEF) - 1) != 0){
	    error("archive: %s has no table of contents, add one with "
		  "ranlib(1) (can't load from it)", file_name);
	    return;
	}
	if(stat_buf.st_mtime > strtol(symdef_ar_hdr->ar_date, NULL, 10)){
	    error("table of contents for archive: %s is out of date; rerun "
		  "ranlib(1) (can't load from it)", file_name);
	    return;
	}
	symdef_length = strtol(symdef_ar_hdr->ar_size, NULL, 10);
	/*
	 * The contents of a __.SYMDEF file is begins with a word giving the
	 * size in bytes of ranlib structures which immediately follow, and
	 * then continues with a string table consisting of a word giving the
	 * number of bytes of strings which follow and then the strings
	 * themselves.  So the smallest valid size is two words long.
	 */
	if(symdef_length < 2 * sizeof(long)){
	    error("size of table of contents for archive: %s too small to be "
		  "a valid table of contents (can't load from it)", file_name);
	    return;
	}
	if(offset + symdef_length > file_size){
	    error("truncated or malformed archive: %s (table of contents "
		  "extends past the end of the file, can't load from it)",
		  file_name);
	    return;
	}
	nranlibs = *((long *)(file_addr + offset)) / sizeof(struct ranlib);
	offset += sizeof(long);
	ranlibs = (struct ranlib *)(file_addr + offset);
	offset += sizeof(struct ranlib) * nranlibs;
	if(nranlibs == 0){
	    warning("empty table of contents: %s (can't load from it)",
		    file_name);
	    return;
	}
	if(offset - (2 * sizeof(long) + sizeof(struct ar_hdr) + SARMAG) >
	   symdef_length){
	    error("truncated or malformed archive: %s (ranlib structures in "
		  "table of contents extends past the end of the table of "
		  "contents, can't load from it)", file_name);
	    return;
	}
	string_size = *((long *)(file_addr + offset));
	offset += sizeof(long);
	strings = file_addr + offset;
	offset += string_size;
	if(offset - (2 * sizeof(long) + sizeof(struct ar_hdr) + SARMAG) >
	   symdef_length){
	    error("truncated or malformed archive: %s (ranlib strings in "
		  "table of contents extends past the end of the table of "
		  "contents, can't load from it)", file_name);
	    return;
	}
	if(symdef_length == 2 * sizeof(long)){
	    warning("empty table of contents for archive: %s (can't load from "
		    "it)", file_name);
	    return;
	}

	/*
	 * Check the string offset and the member offsets of the ranlib structs.
	 */
	for(i = 0; i < nranlibs; i++){
	    if(ranlibs[i].ran_un.ran_strx < 0 ||
	       ranlibs[i].ran_un.ran_strx >= string_size){
		error("malformed table of contents in: %s (ranlib struct %d "
		      "has bad string index, can't load from it)", file_name,i);
		return;
	    }
	    if(ranlibs[i].ran_off < 0 ||
	       ranlibs[i].ran_off + sizeof(struct ar_hdr) >= file_size){
		error("malformed table of contents in: %s (ranlib struct %d "
		      "has bad library member offset, can't load from it)",
		      file_name, i);
		return;
	    }
	    /*
	     * These should be on 4 byte boundaries because the maximum
	     * alignment of the header structures and relocation are 4 bytes.
	     * But this is has to be 2 bytes because that's the way ar(1) has
	     * worked historicly in the past.  Fortunately this works on the
	     * 68k machines but will have to change when this is on a real
	     * machine.
	     */
	    if(ranlibs[i].ran_off % sizeof(short) != 0){
		error("malformed table of contents in: %s (ranlib struct %d "
		      "library member offset not a multiple of %d bytes, can't "
		      "load from it)", file_name, i, sizeof(short));
		return;
	    }
	}

	/*
	 * Two possible algorithms are used to determine which members from the
	 * archive are to be loaded.  The first is faster and requires the 
	 * ranlib structures to be in sorted order (as produced by the ranlib(1)
	 * -s option).  The only case this can't be done is when more than one
	 * library member in the same archive defines the same symbol.  In this
	 * case ranlib(1) will never sort the ranlib structures but will leave
	 * them in the order of the archive so that the proper member that
	 * defines a symbol that is defined in more that one object is loaded.
	 */
	if(strncmp(symdef_ar_hdr->ar_name, SYMDEF_SORTED,
		   sizeof(SYMDEF_SORTED) - 1) == 0){
	    /*
	     * Now go through the undefined symbol list and look up each symbol
	     * in the sorted ranlib structures looking to see it their is a
	     * library member that satisfies this undefined symbol.  If so that
	     * member is loaded and merge() is called.
	     */
	    for(undefined = undefined_list.next;
		undefined != &undefined_list;
		/* no increment expression */){
		/* If this symbol is no longer undefined delete it and move on*/
		if(undefined->merged_symbol->nlist.n_type != (N_UNDF | N_EXT) ||
		   undefined->merged_symbol->nlist.n_value != 0){
		    undefined = undefined->next;
		    delete_from_undefined_list(undefined->prev);
		    continue;
		}
		ranlib = bsearch(undefined->merged_symbol->nlist.n_un.n_name, 
			   ranlibs, nranlibs, sizeof(struct ranlib),
			   (int (*)(const void *, const void *))ranlib_bsearch);
		if(ranlib != NULL){

		    /* there is a member that defineds this symbol so load it */
		    cur_obj = new_object_file();
#ifdef RLD
		    cur_obj->file_name = allocate(strlen(file_name) + 1);
		    strcpy(cur_obj->file_name, file_name);
#else
		    cur_obj->file_name = file_name;
#endif RLD
		    cur_obj->obj_addr = file_addr +
					ranlib->ran_off + sizeof(struct ar_hdr);
		    cur_obj->ar_hdr = (struct ar_hdr *)(file_addr +
							ranlib->ran_off);
		    cur_obj->obj_size = strtol(cur_obj->ar_hdr->ar_size, NULL,
					       10);
		    if(ranlib->ran_off + sizeof(struct ar_hdr) +
						cur_obj->obj_size > file_size){
			error("malformed library: %s (member %s extends past "
			      "the end of the file, can't load from it)",
			      file_name, obj_member_name(cur_obj));
			return;
		    }
		    if(whyload){
			print_obj_name(cur_obj);
			print("loaded to resolve symbol: %s\n", 
			       undefined->merged_symbol->nlist.n_un.n_name);
		    }

		    merge();

		    /* make sure this symbol got defined */
		    if(errors == 0 && 
		       undefined->merged_symbol->nlist.n_type == (N_UNDF|N_EXT)
		       && undefined->merged_symbol->nlist.n_value == 0){
			error("malformed table of contents in library: %s "
			      "(member %s did not defined symbol %s)",
			      file_name, obj_member_name(cur_obj),
			      undefined->merged_symbol->nlist.n_un.n_name);
		    }
		    undefined = undefined->next;
		    delete_from_undefined_list(undefined->prev);
		    continue;
		}
		undefined = undefined->next;
	    }
	}
	else{
	    /*
	     * The slower algorithm.  Lookup each symbol in the table of
	     * contents to see if is undefined.  If so that member is loaded
	     * and merge() is called.  A complete pass over the table of
	     * contents without loading a member terminates searching
	     * the library.  This could be made faster if this wrote on the
	     * ran_off to indicate the member at that offset was loaded and
	     * then it's symbol would be not be looked up on later passes.
	     * But this is not done because it would dirty the table of contents
	     * and cause the possibility of more swapping and if fast linking is
	     * wanted then the table of contents can be sorted.
	     */
	    warning("table of contents of library: %s not sorted slower link "
		    "editing will result (use the ranlib(1) -s option)",
		    file_name);
	    member_loaded = TRUE;
	    while(member_loaded == TRUE && errors == 0){
		member_loaded = FALSE;
		for(i = 0; i < nranlibs; i++){
		    merged_symbol = *(lookup_symbol(strings +
						   ranlibs[i].ran_un.ran_strx));
		    if(merged_symbol != NULL){
			if(merged_symbol->nlist.n_type == (N_UNDF | N_EXT) &&
			   merged_symbol->nlist.n_value == 0){
			    /*
			     * This symbol is defined in this member so load it.
			     */
			    cur_obj = new_object_file();
			    cur_obj->file_name = file_name;
			    cur_obj->obj_addr = file_addr + ranlibs[i].ran_off +
						sizeof(struct ar_hdr);
			    cur_obj->ar_hdr = (struct ar_hdr *)(file_addr +
							ranlibs[i].ran_off);
			    cur_obj->obj_size = strtol(cur_obj->ar_hdr->ar_size,
						       NULL, 10);
			    if(ranlibs[i].ran_off + sizeof(struct ar_hdr) +
						cur_obj->obj_size > file_size){
				error("malformed library: %s (member %s "
				      "extends past the end of the file, can't "
				      "load from it)", file_name,
				      obj_member_name(cur_obj));
				return;
			    }
			    if(whyload){
				print_obj_name(cur_obj);
				print("loaded to resolve symbol: %s\n", 
				       merged_symbol->nlist.n_un.n_name);
			    }

			    merge();

			    /* make sure this symbol got defined */
			    if(errors == 0 &&
			       merged_symbol->nlist.n_type == (N_UNDF | N_EXT)
			       && merged_symbol->nlist.n_value == 0){
				error("malformed table of contents in library: "
				      "%s (member %s did not defined "
				      "symbol %s)", file_name,
				      obj_member_name(cur_obj),
				      merged_symbol->nlist.n_un.n_name);
			    }
			    /*
			     * Skip any other symbols that are defined in this
			     * member since it has just been loaded.
			     */
			    for(j = i; j + 1 < nranlibs; j++){
				if(ranlibs[i].ran_off != ranlibs[j + 1].ran_off)
				    break;
			    }
			    i = j;
			    member_loaded = TRUE;
			}
		    }
		}
	    }
	}
}

/*
 * Function for bsearch() for finding a symbol name.
 */
static
int
ranlib_bsearch(
const char *symbol_name,
const struct ranlib *ran)
{
	return(strcmp(symbol_name, strings + ran->ran_un.ran_strx));
}

/*
 * merge() merges all the global information from the cur_obj into the merged
 * data structures for the output object file to be built from.
 */
void
merge(void)
{
	/* print the object file name if tracing */
	if(trace){
	    print_obj_name(cur_obj);
	    print("\n");
	}

	/* check the header and load commands of the object file */
	check_cur_obj();
	if(errors)
	    return;

	/* if this object has any fixed VM shared library stuff merge it */
	if(cur_obj->fvmlib_stuff){
#ifndef RLD
	    merge_fvmlibs();
	    if(errors)
		return;
#else defined(RLD)
	    if(cur_obj != base_obj){
		error_with_cur_obj("can't dynamicly load fixed VM shared "
				   "library");
		return;
	    }
#endif defined(RLD)
	}

	/* merged it's sections */
	merge_sections();
	if(errors)
	    return;

	/* merged it's symbols */
	merge_symbols();
	if(errors)
	    return;
}

/*
 * check_cur_obj() checks to see if the cur_obj object file is really an object
 * file and that all the offset and sizes in the headers are within the memory
 * the object file is mapped in.  This allows the rest of the code in the link
 * editor to use the offsets and sizes in the headers without bounds checking.
 * 
 * Since this is making a pass through the headers a number of things are filled
 * in in the object structrure for this object file including: the symtab field,
 * the section_maps and nsection_maps fields (this routine allocates the
 * section_map structures and fills them in too), and the fvmlib_stuff field is
 * set if any SG_FVMLIB segments or LC_LOADFVMLIB commands are seen.
 */
static
void
check_cur_obj(void)
{
    long i, j;
    struct mach_header *mh;
    struct load_command *lc, *load_commands;
    struct segment_command *sg;
    struct section *s;
    struct symtab_command *st;
    struct symseg_command *ss;
    struct fvmlib_command *fl;
    char *fvmlib_name;

    static const struct symtab_command empty_symtab = { 0 };

	/* check to see the mach_header is valid */
	if(sizeof(struct mach_header) > cur_obj->obj_size){
	    error_with_cur_obj("truncated or malformed object (mach header "
			       "extends past the end of the file)");
	    return;
	}
	mh = (struct mach_header *)cur_obj->obj_addr;
	if(mh->magic != MH_MAGIC){
	    error_with_cur_obj("bad magic number (not a Mach-O file)");
	    return;
	}
	/* make sure the cputype of this object matches other objects loaded */
	if(mh->cputype != 0 && mh->cpusubtype != 0){
	    if(cputype){
		if(cputype != mh->cputype){
		    error_with_cur_obj("cputype (%d) does not match cputype "
				       "(%d) of objects files previously "
				       "loaded", mh->cputype, cputype);
		    return;
		}
		if(cpusubtype < mh->cpusubtype)
		    cpusubtype = mh->cpusubtype;
	    }
	    else{
		if(mh->cputype != CPU_TYPE_MC680x0){
		    error_with_cur_obj("cputype (%d) unknown", mh->cputype);
		    return;
		}
		cputype = mh->cputype;
		cpusubtype = mh->cpusubtype;
	    }
	}
	if(mh->sizeofcmds + sizeof(struct mach_header) > cur_obj->obj_size){
	    error_with_cur_obj("truncated or malformed object (load commands "
			       "extend past the end of the file)");
	    return;
	}
	if((mh->flags & MH_INCRLINK) != 0){
	    error_with_cur_obj("was the output of an incremental link, can't "
			       "be link edited again");
	    return;
	}

	/* check to see that the load commands are valid */
	load_commands = (struct load_command *)((char *)cur_obj->obj_addr +
			    sizeof(struct mach_header));
	st = NULL;
	lc = load_commands;
	for(i = 0; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0){
		error_with_cur_obj("load command %d size not a multiple of "
				   "sizeof(long)", i);
		return;
	    }
	    if(lc->cmdsize <= 0){
		error_with_cur_obj("load command %d size is less than or equal "
				   "to zero", i);
		return;
	    }
	    if((char *)lc + lc->cmdsize >
	       (char *)load_commands + mh->sizeofcmds){
		error_with_cur_obj("load command %d extends past end of all "
				   "load commands", i);
		return;
	    }
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		if(sg->cmdsize != sizeof(struct segment_command) +
				     sg->nsects * sizeof(struct section)){
		    error_with_cur_obj("cmdsize field of load command %d is "
				       "inconsistant for a segment command "
				       "with the number of sections it has", i);
		    return;
		}
		if(sg->flags == SG_FVMLIB){
		    if(sg->nsects != 0){
			error_with_cur_obj("SG_FVMLIB segment %0.16s contains "
					   "sections and shouldn't",
					   sg->segname);
			return;
		    }
		    cur_obj->fvmlib_stuff = TRUE;
		    break;
		}
		check_size_offset(sg->filesize, sg->fileoff, sizeof(long),
				  "filesize", "fileoff", i);
		if(errors)
		    return;
		/*
		 * Segments without sections are an error to see on input except
		 * for the segments created by the link-editor (which are
		 * recreated).
		 */
		if(sg->nsects == 0){
		    if(strcmp(sg->segname, SEG_PAGEZERO) != 0 &&
		       strcmp(sg->segname, SEG_LINKEDIT) != 0){
			error_with_cur_obj("segment %0.16s contains no "
					   "sections and can't be link-edited",
					   sg->segname);
			return;
		    }
		}
		else{
		    /*
		     * Doing a reallocate here is not bad beacuse in the
		     * normal case this is an MH_OBJECT file type and has only
		     * one section.  So this only gets done once per object.
		     */
		    cur_obj->section_maps = reallocate(cur_obj->section_maps,
					(cur_obj->nsection_maps + sg->nsects) *
					sizeof(struct section_map));
		    memset(cur_obj->section_maps + cur_obj->nsection_maps, '\0',
			   sg->nsects * sizeof(struct section_map));
		}
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0 ; j < sg->nsects ; j++){
		    cur_obj->section_maps[cur_obj->nsection_maps++].s = s;
		    /* check to see that segment name in the section structure
		       matches the one in the segment command if this is not in
		       an MH_OBJECT filetype */
		    if(mh->filetype != MH_OBJECT &&
		       strcmp(sg->segname, s->segname) != 0){
			error_with_cur_obj("segment name %0.16s of section %d "
				"(%0.16s,%0.16s) in load command %d does not "
				"match segment name %0.16s", s->segname, j,
				s->segname, s->sectname, i, sg->segname);
			return;
		    }
		    /* check to see that flags (type) of this section is some
		       thing the link-editor understands */
		    if(s->flags != 0 /* S_CONTENTS */ &&
		       s->flags != S_ZEROFILL &&
		       s->flags != S_CSTRING_LITERALS &&
		       s->flags != S_4BYTE_LITERALS &&
		       s->flags != S_8BYTE_LITERALS &&
		       s->flags != S_LITERAL_POINTERS){
			error_with_cur_obj("unknown flags (type) of section %d "
					   "(%0.16s,%0.16s) in load command %d",
					   j, s->segname, s->sectname, i);
			return;
		    }
		    /* check to make sure the alignment is reasonable */
		    if(s->align > MAXSECTALIGN){
			error_with_cur_obj("align (%d) of section %d "
			    "(%0.16s,%0.16s) in load command %d greater "
			    "than maximum section alignment (%d)", s->align,
			     j, s->segname, s->sectname, i, MAXSECTALIGN);
			return;
		    }
		    /* check the size and offset of the contents if it has any*/
		    if(s->flags == 0 /* S_CONTENTS */ ||
		       s->flags == S_CSTRING_LITERALS ||
		       s->flags == S_4BYTE_LITERALS ||
		       s->flags == S_8BYTE_LITERALS ||
		       s->flags == S_LITERAL_POINTERS){
			check_size_offset_sect(s->size, s->offset, sizeof(char),
			    "size", "offset", i, j, s->segname, s->sectname);
			if(errors)
			    return;
		    }
		    /* check the relocation entries if it can have them */
		    if(s->flags == S_ZEROFILL ||
		       s->flags == S_CSTRING_LITERALS ||
		       s->flags == S_4BYTE_LITERALS ||
		       s->flags == S_8BYTE_LITERALS){
			if(s->nreloc != 0){
			    error_with_cur_obj("section %d (%0.16s,%0.16s) in "
				"load command %d has relocation entries which "
				"it shouldn't for its type (flags)", j,
				 s->segname, s->sectname, i);
			    return;
			}
		    }
		    else{
			if(s->nreloc != 0){
			    if(mh->cputype == 0 && mh->cpusubtype == 0){
				error_with_cur_obj("section %d (%0.16s,%0.16s)"
				    "in load command %d has relocation entries "
				    "but the cputype and cpusubtype for the "
				    "object are not set", j, s->segname,
				    s->sectname, i);
				return;
			    }
			}
			else{
			    check_size_offset_sect(s->nreloc * sizeof(struct
				 relocation_info), s->reloff, sizeof(long),
				 "nreloc * sizeof(struct relocation_info)",
				 "reloff", i, j, s->segname, s->sectname);
			    if(errors)
				return;
			}
		    }
		    s++;
		}
		break;

	    case LC_SYMTAB:
		if(st != NULL){
		    error_with_cur_obj("contains more than one LC_SYMTAB load "
				       "command");
		    return;
		}
		st = (struct symtab_command *)lc;
		if(st->cmdsize != sizeof(struct symtab_command)){
		    error_with_cur_obj("cmdsize of load command %d incorrect "
				       "for LC_SYMTAB", i);
		    return;
		}
		check_size_offset(st->nsyms * sizeof(struct nlist), st->symoff,
				  sizeof(long), "nsyms * sizeof(struct nlist)",
				  "symoff", i);
		if(errors)
		    return;
		check_size_offset(st->strsize, st->stroff, sizeof(long),
				  "strsize", "stroff", i);
		if(errors)
		    return;
		cur_obj->symtab = st;
		break;

	    case LC_SYMSEG:
		ss = (struct symseg_command *)lc;
		if(ss->size != 0){
		    warning_with_cur_obj("contains obsolete LC_SYMSEG load "
			"command with non-zero size (produced with a pre-1.0 "
			"version of the compiler, please recompile)");
		}
		break;

	    case LC_IDFVMLIB:
		if(filetype != MH_FVMLIB){
		    error_with_cur_obj("LC_IDFVMLIB load command in object "
				       "file (should not be in an input file "
				       "to the link editor for output "
				       "filetypes other than MH_FVMLIB)");
		    return;
		}
		cur_obj->fvmlib_stuff = TRUE;
		break;

	    case LC_LOADFVMLIB:
		if(filetype == MH_FVMLIB){
		    error_with_cur_obj("LC_LOADFVMLIB load command in object "
				       "file (should not be in an input file "
				       "to the link editor for the output "
				       "filetype MH_FVMLIB)");
		    return;
		}
		fl = (struct fvmlib_command *)lc;
		if(fl->cmdsize < sizeof(struct fvmlib_command)){
		    error_with_cur_obj("cmdsize of load command %d incorrect "
				       "for LC_LOADFVMLIB", i);
		    return;
		}
		if(fl->fvmlib.name.offset >= fl->cmdsize){
		    error_with_cur_obj("name.offset of load command %d extends "
				       "past the end of the load command", i); 
		    return;
		}
		fvmlib_name = (char *)fl + fl->fvmlib.name.offset;
		for(j = 0 ; j < fl->cmdsize - fl->fvmlib.name.offset; j++){
		    if(fvmlib_name[j] == '\0')
			break;
		}
		if(j >= fl->cmdsize - fl->fvmlib.name.offset){
		    error_with_cur_obj("library name of load command %d "
				       "not null terminated", i);
		    return;
		}
		cur_obj->fvmlib_stuff = TRUE;
		break;

	    case LC_UNIXTHREAD:
	    case LC_THREAD:
	    case LC_IDENT:
		break;

	    default:
		error_with_cur_obj("load command %d unknown cmd", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	/*
	 * If this object does not have a symbol table command then set it's
	 * symtab pointer to the empty symtab.  This makes symbol number range
	 * checks in relocation cleaner.
	 */
	if(cur_obj->symtab == NULL)
	    cur_obj->symtab = (struct symtab_command *)&empty_symtab;
}

#ifdef RLD
/*
 * merge_base_program() does the pass1 functions for the base program that
 * called rld_load() using it's SEG_LINKEDIT.  It does the same things as the
 * routines pass1(),check_obj() and merge() except that the offset are assumed
 * to be correct in most cases (if they weren't the program would not be
 * executing).
 * 
 * A hand crafted object structure is created so to work with the rest of the
 * code.  Like check_obj() a number of things are filled in in the object
 * structrure including: the symtab field, the section_maps and nsection_maps
 * fields (this routine allocates the section_map structures and fills them in
 * too), all fvmlib stuff is ignored since the output is loaded as well as
 * linked.  The file offsets in symtab are faked since this is not a file mapped
 * into memory but rather a running process.  This involves setting where the
 * object starts to the address of the _mh_execute_header and calcating the
 * file offset of the symbol and string table as the differences of the
 * addresses from the _mh_execute_header.  This makes using the rest of the
 * code easy.
 */
void
merge_base_program(
struct segment_command *seg_linkedit)
{
    long i, j;
    struct mach_header *mh;
    struct load_command *lc, *load_commands;
    struct segment_command *sg;
    struct section *s;
    struct symtab_command *st;

    static const struct symtab_command empty_symtab = { 0 };
    static struct symtab_command base_program_symtab = { 0 };

    /*
     * The NeXT global variable that gets set to argv in crt0.o.  Used here to
     * set the name of the base program's object file (NXArgv[0]).
     */
    extern char **NXArgv;

	/*
	 * Hand craft the object structure as in pass1().  Note the obj_size
	 * feild should never be tested against since this is not a file mapped
	 * into memory but rather a running program.
	 */
	cur_obj = new_object_file();
	cur_obj->file_name = NXArgv[0];
	cur_obj->obj_addr = (char *)&_mh_execute_header;
	cur_obj->obj_size = 0;
	base_obj = cur_obj;
	/* Set the output cpu types from the base program's header */
	mh = (struct mach_header *)&_mh_execute_header;
	cputype = mh->cputype;
	cpusubtype = mh->cpusubtype;

	/*
	 * Go through the load commands and do what would be done in check_obj()
	 * but not checking for offsets.
	 */
	load_commands = (struct load_command *)((char *)cur_obj->obj_addr +
			    sizeof(struct mach_header));
	st = NULL;
	lc = load_commands;
	for(i = 0; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0){
		error_with_cur_obj("load command %d size not a multiple of "
				   "sizeof(long)", i);
		return;
	    }
	    if(lc->cmdsize <= 0){
		error_with_cur_obj("load command %d size is less than or equal "
				   "to zero", i);
		return;
	    }
	    if((char *)lc + lc->cmdsize >
	       (char *)load_commands + mh->sizeofcmds){
		error_with_cur_obj("load command %d extends past end of all "
				   "load commands", i);
		return;
	    }
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		if(sg->cmdsize != sizeof(struct segment_command) +
				     sg->nsects * sizeof(struct section)){
		    error_with_cur_obj("cmdsize field of load command %d is "
				       "inconsistant for a segment command "
				       "with the number of sections it has", i);
		    return;
		}
		if(sg->flags == SG_FVMLIB){
		    if(sg->nsects != 0){
			error_with_cur_obj("SG_FVMLIB segment %0.16s contains "
					   "sections and shouldn't",
					   sg->segname);
			return;
		    }
		    break;
		}
		/*
		 * Segments without sections are an error to see on input except
		 * for the segments created by the link-editor (which are
		 * recreated).
		 */
		if(sg->nsects == 0){
		    if(strcmp(sg->segname, SEG_PAGEZERO) != 0 &&
		       strcmp(sg->segname, SEG_LINKEDIT) != 0){
			error_with_cur_obj("segment %0.16s contains no "
					   "sections and can't be link-edited",
					   sg->segname);
			return;
		    }
		}
		else{
		    /*
		     * Doing a reallocate here is not bad beacuse in the
		     * normal case this is an MH_OBJECT file type and has only
		     * one section.  So this only gets done once per object.
		     */
		    cur_obj->section_maps = reallocate(cur_obj->section_maps,
					(cur_obj->nsection_maps + sg->nsects) *
					sizeof(struct section_map));
		    memset(cur_obj->section_maps + cur_obj->nsection_maps, '\0',
			   sg->nsects * sizeof(struct section_map));
		}
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0 ; j < sg->nsects ; j++){
		    cur_obj->section_maps[cur_obj->nsection_maps++].s = s;
		    /* check to see that segment name in the section structure
		       matches the one in the segment command if this is not in
		       an MH_OBJECT filetype */
		    if(mh->filetype != MH_OBJECT &&
		       strcmp(sg->segname, s->segname) != 0){
			error_with_cur_obj("segment name %0.16s of section %d "
				"(%0.16s,%0.16s) in load command %d does not "
				"match segment name %0.16s", s->segname, j,
				s->segname, s->sectname, i, sg->segname);
			return;
		    }
		    /* check to see that flags (type) of this section is some
		       thing the link-editor understands */
		    if(s->flags != 0 /* S_CONTENTS */ &&
		       s->flags != S_ZEROFILL &&
		       s->flags != S_CSTRING_LITERALS &&
		       s->flags != S_4BYTE_LITERALS &&
		       s->flags != S_8BYTE_LITERALS &&
		       s->flags != S_LITERAL_POINTERS){
			error_with_cur_obj("unknown flags (type) of section %d "
					   "(%0.16s,%0.16s) in load command %d",
					   j, s->segname, s->sectname, i);
			return;
		    }
		    /* check to make sure the alignment is reasonable */
		    if(s->align > MAXSECTALIGN){
			error_with_cur_obj("align (%d) of section %d "
			    "(%0.16s,%0.16s) in load command %d greater "
			    "than maximum section alignment (%d)", s->align,
			     j, s->segname, s->sectname, i, MAXSECTALIGN);
			return;
		    }
		    s++;
		}
		break;

	    case LC_SYMTAB:
		if(st != NULL){
		    error_with_cur_obj("contains more than one LC_SYMTAB load "
				       "command");
		    return;
		}
		st = (struct symtab_command *)lc;
		if(st->cmdsize != sizeof(struct symtab_command)){
		    error_with_cur_obj("cmdsize of load command %d incorrect "
				       "for LC_SYMTAB", i);
		    return;
		}
		break;

	    case LC_SYMSEG:
	    case LC_IDFVMLIB:
	    case LC_LOADFVMLIB:
	    case LC_UNIXTHREAD:
	    case LC_THREAD:
	    case LC_IDENT:
		break;

	    default:
		error_with_cur_obj("load command %d unknown cmd", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	/*
	 * Now the slightly tricky part of faking up a symtab command that
	 * appears to have offsets to the symbol and string table when added
	 * to the cur_obj->obj_addr get the correct addresses.
	 */
	if(st != NULL && st->nsyms != 0){
	    if(st->symoff < seg_linkedit->fileoff ||
	       st->symoff + st->nsyms * sizeof(struct nlist) >
				seg_linkedit->fileoff + seg_linkedit->filesize){
		error_with_cur_obj("symbol table is not contained in "
				   SEG_LINKEDIT " segment");
		return;
	    }
	    if(st->stroff < seg_linkedit->fileoff ||
	       st->stroff + st->strsize >
				seg_linkedit->fileoff + seg_linkedit->filesize){
		error_with_cur_obj("string table is not contained in "
				   SEG_LINKEDIT " segment");
		return;
	    }
	    base_program_symtab = *st;
	    base_program_symtab.symoff = (seg_linkedit->vmaddr + (st->symoff -
					  seg_linkedit->fileoff)) -
					  (long)cur_obj->obj_addr;
	    base_program_symtab.stroff = (seg_linkedit->vmaddr + (st->stroff -
					  seg_linkedit->fileoff)) -
					  (long)cur_obj->obj_addr;
	    cur_obj->symtab = &base_program_symtab;
	}
	/*
	 * If this object does not have a symbol table command then set it's
	 * symtab pointer to the empty symtab.  This makes symbol number range
	 * checks in relocation cleaner.
	 */
	else{
	    cur_obj->symtab = (struct symtab_command *)&empty_symtab;
	}

	/*
	 * Now finish with the base program by doing what would be done in
	 * merge() by merging the base program's sections and symbols.
	 */
	/* merged the base program's sections */
	merge_sections();

	/* merged the base program's symbols */
	merge_symbols();
}
#endif RLD

/*
 * check_size_offset() is used by check_cur_obj() to check a pair of sizes,
 * and offsets from the object file to see it they are aligned correctly and
 * containded with in the file.
 */
static
void
check_size_offset(
unsigned long size,
unsigned long offset,
unsigned long align,
char *size_str,
char *offset_str,
long cmd)
{
	if(size != 0){
#if mc68000
	    /*
	     * For the mc68000 the alignment is only a warning because it can
	     * deal with all accesses on bad alignment.  When this is put on a
	     * real machine (RISC) the alignment check will cause an error.
	     */
	    if(offset % align != 0){
		warning_with_cur_obj("%s in load command %d not aligned on %d "
				     "byte boundary", offset_str, cmd, align);
		return;
	    }
#else !mc68000
#error "check_size_offset() must be updated for non-mc68000 machines"
#endif
	    if(offset > cur_obj->obj_size){
		error_with_cur_obj("%s in load command %d extends past the "
				   "end of the file", offset_str, cmd);
		return;
	    }
	    if(offset + size > cur_obj->obj_size){
		error_with_cur_obj("%s plus %s in load command %d extends past "
				   "the end of the file", offset_str, size_str,
				   cmd);
		return;
	    }
	}
}

/*
 * check_size_offset_sect() is used by check_cur_obj() to check a pair of sizes,
 * and offsets from a section in the object file to see it they are aligned
 * correctly and containded with in the file.
 */
static
void
check_size_offset_sect(
unsigned long size,
unsigned long offset,
unsigned long align,
char *size_str,
char *offset_str,
long cmd,
long sect,
char *segname,
char *sectname)
{
	if(size != 0){
#if mc68000
	    /*
	     * For the mc68000 the alignment is only a warning because it can
	     * deal with all accesses on bad alignment.  When this is put on a
	     * real machine (RISC) the alignment check will cause an error.
	     */
	    if(offset % align != 0){
		warning_with_cur_obj("%s of section %d (%0.16s,%0.16s) in load "
		    "command %d not aligned on %d byte boundary", offset_str,
		    sect, segname, sectname, cmd, align);
		return;
	    }
#else !mc68000
#error "check_size_offset_sect() must be updated for non-mc68000 machines"
#endif
	    if(offset > cur_obj->obj_size){
		error_with_cur_obj("%s of section %d (%0.16s,%0.16s) in load "
		    "command %d extends past the end of the file", offset_str,
		    sect, segname, sectname, cmd);
		return;
	    }
	    if(offset + size > cur_obj->obj_size){
		error_with_cur_obj("%s plus %s of section %d (%0.16s,%0.16s) "
		    "in load command %d extends past the end of the file",
		    offset_str, size_str, sect, segname, sectname, cmd);
		return;
	    }
	}
}

#ifndef RLD
/*
 * collect_base_obj_segments() collects the segments from the base file on a
 * merged segment list used for overlap checking in
 * check_for_overlapping_segments().
 */
static
void
collect_base_obj_segments(void)
{
    long i;
    struct mach_header *mh;
    struct load_command *lc, *load_commands;
    struct segment_command *sg;

	mh = (struct mach_header *)base_obj->obj_addr;
	load_commands = (struct load_command *)((char *)base_obj->obj_addr +
			    sizeof(struct mach_header));
	lc = load_commands;
	for(i = 0; i < mh->ncmds; i++){
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		add_base_obj_segment(sg, base_obj->file_name);
		break;

	    default:
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * add_base_obj_segment() adds the specified segment to the list of
 * base_obj_segments as comming from the specified base filename.
 */
static
void
add_base_obj_segment(
struct segment_command *sg,
char *filename)
{
    struct merged_segment **p, *msg;

	p = &base_obj_segments;
	while(*p){
	    msg = *p;
	    p = &(msg->next);
	}
	*p = allocate(sizeof(struct merged_segment));
	msg = *p;
	memset(msg, '\0', sizeof(struct merged_segment));
	msg->sg = *sg;
	msg->filename = filename;
}

/*
 * Mkstr() creates a string that is the concatenation of a variable number of
 * strings.  It is pass a variable number of pointers to strings and the last
 * pointer is NULL.  It returns the pointer to the string it created.  The
 * storage for the string is malloc()'ed can be free()'ed when nolonger needed.
 */
static
char *
mkstr(
const char *args,
...)
{
    va_list ap;
    char *s, *p;
    long size;

	size = 0;
	if(args != NULL){
	    size += strlen(args);
	    va_start(ap, args);
	    p = (char *)va_arg(ap, char *);
	    while(p != NULL){
		size += strlen(p);
		p = (char *)va_arg(ap, char *);
	    }
	}
	s = allocate(size + 1);
	*s = '\0';

	if(args != NULL){
	    (void)strcat(s, args);
	    va_start(ap, args);
	    p = (char *)va_arg(ap, char *);
	    while(p != NULL){
		(void)strcat(s, p);
		p = (char *)va_arg(ap, char *);
	    }
	    va_end(ap);
	}
	return(s);
}
#endif !defined(RLD)

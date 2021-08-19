#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines to manage the merging of the symbols.
 * It builds a merged symbol table and string table for external symbols.
 * It also contains all other routines that deal with symbols.
 */
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <sys/loader.h>
#include <mach.h>
#include <nlist.h>
#include <ar.h>
#include <ldsyms.h>

#include "ld.h"
#include "specs.h"
#include "objects.h"
#include "sections.h"
#include "symbols.h"
#include "layout.h"
#include "pass2.h"
#include "sets.h"
#include "hash_string.h"

/*
 * The head of the symbol list and the total count of all external symbols
 * in the list.
 */
struct merged_symbol_list *merged_symbol_lists = NULL;
long nmerged_symbols = 0;

/*
 * This is set by lookup_symbol() and used by enter_symbol(). When a symbol
 * is not found in the symbol_list by lookup_symbol() it returns a pointer
 * to a hash_table entry in a merged_symbol_list.  The merged_symbol_list that
 * pointer is in is pointed to by merged_symbol_list_for_enter_symbol.
 */
static struct merged_symbol_list *merged_symbol_list_for_enter_symbol = NULL;

/*
 * The head of the list of the blocks that store the strings for the merged
 * symbols and the total size of all the strings.
 */
struct string_block *merged_string_blocks = NULL;
long merged_string_size = 0;

/*
 * The number of local symbols that will appear in the output file and the
 * size of their strings.
 */
long nlocal_symbols = 0;
long local_string_size = 0;

/*
 * The things to deal with creating local symbols with the object file's name
 * for a given section.  If the section name is (__TEXT,__text) these are the
 * same as a UNIX link editor's file.o symbols for the text section.
 */
extern struct sect_object_symbols sect_object_symbols = { 0 };

/*
 * The head of the undefined list and the list of free undefined structures.
 * These are circular lists so they can be searched from start to end and so
 * new items can be put on the end.  These two structure never has their
 * merged_symbol filled in but they only serve as the heads and tails of there
 * lists.
 */
struct undefined_list undefined_list = {
    NULL, &undefined_list, &undefined_list
};
static struct undefined_list free_list = {
    NULL, &free_list, &free_list
};
/*
 * The structures for the undefined list are allocated in blocks and placed on
 * a free list.  They are allocated in blocks so they can be free()'ed quickly.
 */
#define NUNDEF_BLOCKS	680
static struct undefined_block {
    struct undefined_list undefineds[NUNDEF_BLOCKS];
    struct undefined_block *next;
} *undefined_blocks;

#ifndef RLD
/*
 * The common symbol load map.  Only allocated and filled in if load map is
 * requested.
 */
struct common_load_map common_load_map = { 0 };

/*
 * These two symbols, undefined_symbol and indr_symbol, and the object_file,
 * command_line_object are used by the routines command_line_symbol() and
 * command_line_indr_symbol() to create symbols from the command line options
 * (-u and -i).
 */
static struct nlist undefined_symbol = {
    {0},		/* n_un.n_strx */
    N_UNDF | N_EXT,	/* n_type */
    NO_SECT,		/* n_sect */
    0,			/* n_desc */
    0			/* n_value */
};
static struct nlist indr_symbol = {
    {0},		/* n_un.n_strx */
    N_INDR | N_EXT,	/* n_type */
    NO_SECT,		/* n_sect */
    0,			/* n_desc */
    0			/* n_value */
};
static struct object_file command_line_object = {
    "command line"	/* file name */
			/* remaining feilds zero */
};

/*
 * This symbol is used by the routines that define link editor defined symbols
 */
static struct object_file link_edit_symbols_object = {
    "link editor" 	/* file_name */
			/* remaining feilds zero */
};
#endif !defined(RLD)

/*
 * These symbols are used when defining common symbols.  In the RLD case they
 * are templates and thus const and the real versions of these symbols are in
 * the sets array.
 */
static
#ifdef RLD
const 
#endif RLD
struct section link_edit_common_section = {
    SECT_COMMON,	/* sectname */
    SEG_DATA,		/* segname */
    0,			/* addr */
    0,			/* size */
    0,			/* offset */
    0,			/* align */
    0,			/* reloff */
    0,			/* nreloc */
    S_ZEROFILL,		/* flags */
    0,			/* reserved1 */
    0,			/* reserved2 */
};

static 
#ifdef RLD
const 
#endif RLD
struct section_map link_edit_section_maps = {
#ifdef RLD
    NULL,		/* struct section *s */
#else
    &link_edit_common_section, /* struct section *s */
#endif RLD
    NULL,		/* output_section */
    0,			/* offset */
    0,			/* flush_offset */
    NULL,		/* fine_relocs */
    0,			/* nfine_relocs */
    FALSE,		/* no_load_order */
    NULL,		/* load_orders */
    0			/* nload_orders */
};

#ifndef RLD
struct symtab_command link_edit_common_symtab = {
    LC_SYMTAB,		/* cmd */
    sizeof(struct symtab_command),	/* cmdsize */
    0,			/* symoff */
    0,			/* nsyms */
    0,			/* stroff */
    1			/* strsize */
};
#endif !defined(RLD)

#ifdef RLD
const 
#endif RLD
struct object_file link_edit_common_object = {
    "link editor",	/* file_name */
    0,			/* obj_addr */
    0, 			/* obj_size */
    FALSE,		/* fvmlib_stuff */
    NULL,		/* ar_hdr */
    1,			/* nsection_maps */
#ifdef RLD
    NULL,		/* section_maps */
    NULL,		/* symtab */
#else
    &link_edit_section_maps,	/* section_maps */
    &link_edit_common_symtab,	/* symtab */
#endif RLD
    0,			/* nundefineds */
    NULL,		/* undefined_maps */
    0,			/* local_string_offset */
    NULL		/* cur_section_map */
#ifdef RLD
    ,0,			/* set_num */
    FALSE		/* user_obj_addr */
#endif RLD
};

/*
 * This is the list of multiply defined symbol names.  It is used to make sure
 * an error message for each name is only printed once and it is traced only
 * once.
 */
static char **multiple_defs = NULL;
static long nmultiple_defs = 0;

/*
 * This is the count of indirect symbols in the merged symbol table.  It is used
 * as the size of an array that needed to be allocated to reduce chains of
 * indirect symbols to their final symbol and to detect circular chains.
 */
static nindr_symbols = 0;

/*
 * This is set and used in define_common_symbols() and also used in 
 * layout_merged_symbols() to properly set the MH_NOUNDEFS flag (which in turn
 * properly set the execute bits of the file).
 */
static enum bool commons_exist = FALSE;

static enum bool is_output_local_symbol(unsigned char n_type,
					char *symbol_name);
static struct merged_symbol * enter_symbol(struct merged_symbol **hash_pointer,
					 struct nlist *object_symbol,
					 char *object_strings,
					 struct object_file *definition_object);
static void enter_indr_symbol(struct merged_symbol *merged_symbol,
			      struct nlist *object_symbol,
			      char *object_strings,
			      struct object_file *definition_object);
static char *enter_string(char *symbol_name);
static void add_to_undefined_list(struct merged_symbol *merged_symbol);
static void multiply_defined(struct merged_symbol *merged_symbol,
			     struct nlist *object_symbol, char *object_strings);
static void trace_object_symbol(struct nlist *symbol, char *strings);
static void trace_merged_symbol(struct merged_symbol *merged_symbol);
static void trace_symbol(char *symbol_name, struct nlist *nlist,
			 struct object_file *object_file,
			 char *indr_symbol_name);
#ifndef RLD
static void define_link_editor_symbol(char *symbol_name, unsigned char type,
				      unsigned char sect, short desc,
				      unsigned long value,
				      struct object_file *definition_object);
#endif !defined(RLD)
static struct string_block *get_string_block(char *symbol_name);

/*
 * Check all the fields of the given symbol in the current object to make sure
 * it is valid.  This is required to that the rest of the code can assume that
 * use the values in the symbol without futher checks and without causing an
 * error.
 */
static
inline
void
check_symbol(
struct nlist *symbol,
char *strings,
long index)
{
	/* check the n_strx field of this symbol */
	if(symbol->n_un.n_strx < 0 ||
	   symbol->n_un.n_strx >= cur_obj->symtab->strsize){
	    error_with_cur_obj("bad string table index (%d) for symbol %d",
			       symbol->n_un.n_strx, index);
	    return;
	}

	/* check the n_type field of this symbol */
	switch(symbol->n_type & N_TYPE){
	case N_UNDF:
	    if((symbol->n_type & N_STAB) == 0 &&
	       (symbol->n_type & N_EXT) == 0){
		error_with_cur_obj("undefined symbol %d (%s) is not also "
				   "external symbol (N_EXT)", index,
				   symbol->n_un.n_strx == 0 ? "NULL name" :
				   strings + symbol->n_un.n_strx);
		return;
	    }
	    /* fall through to the check below */
	case N_ABS:
	    if((symbol->n_type & N_STAB) == 0 &&
	       symbol->n_sect != NO_SECT){
		error_with_cur_obj("symbol %d (%s) must have NO_SECT for "
			    "its n_sect field given its type (N_ABS)", index,
			    symbol->n_un.n_strx == 0 ? "NULL name" :
			    strings + symbol->n_un.n_strx);
		return;
	    }
	    break;
	case N_SECT:
	    if((symbol->n_type & N_STAB) == 0 &&
	       symbol->n_sect == NO_SECT){
		error_with_cur_obj("symbol %d (%s) must not have NO_SECT "
			    "for its n_sect field given its type (N_SECT)",
			    index, symbol->n_un.n_strx == 0 ? "NULL name" :
			    strings + symbol->n_un.n_strx);
		return;
	    }
	    break;
	case N_INDR:
	    if(symbol->n_type & N_EXT){
		/* note n_value is an unsigned long and can't be < 0 */
		if(symbol->n_value >= cur_obj->symtab->strsize){
		    error_with_cur_obj("bad string table index (%d) for "
			"indirect name for symbol %d (%s)",
			symbol->n_value, index, symbol->n_un.n_strx == 0 ?
			"NULL name" : strings + symbol->n_un.n_strx);
		    return;
		}
	    }
	    else if((symbol->n_type & N_STAB) == 0){
		error_with_cur_obj("indirect symbol %d (%s) is not also "
				   "external symbol (N_EXT)", index,
				   symbol->n_un.n_strx == 0 ? "NULL name" :
				   strings + symbol->n_un.n_strx, index);
		return;
	    }
	    break;
	default:
	    if((symbol->n_type & N_STAB) == 0){
		error_with_cur_obj("symbol %d (%s) has unknown n_type field "
				   "(0x%x)", index, symbol->n_un.n_strx == 0 ?
				   "NULL name" : strings + symbol->n_un.n_strx,
				   symbol->n_type);
		return;
	    }
	    break;
	}

	/*
	 * Check the n_sect field, note sections are numbered from 1 up to and
	 * including the total number of sections (that is the test is > not
	 * >= ).
	 */
	if(symbol->n_sect > cur_obj->nsection_maps){
	    error_with_cur_obj("symbol %d (%s)'s n_sect field (%d) is "
		"greater than the number of sections in this object (%d)",
		index, symbol->n_un.n_strx == 0 ? "NULL name" : strings +
		symbol->n_un.n_strx, symbol->n_sect, cur_obj->nsection_maps);
	    return;
	}
}

/*
 * relocate_symbol() relocates the specified symbol pointed to by nlist in the
 * object file pointed to by object file.  It modifies the section number of
 * the symbol and the value of the symbol to what it should be in the output
 * file.
 */
static
inline
void
relocate_symbol(
struct nlist *nlist,
struct object_file *object_file)
{
    struct section_map *section_map;

	/*
	 * If this symbol is not in a section then it is not changed.
	 */
	if(nlist->n_sect == NO_SECT)
	    return;
#ifdef RLD
	/*
	 * If this symbol is not in the current set of objects being linked
	 * and loaded it does not get relocated.
	 */
	if(object_file->set_num != cur_set)
	    return;
#endif RLD

	/*
	 * Change the section number of this symbol to the section number it
	 * will have in the output file.  For RLD all section numbers are left
	 * as they are in the input file they came from so that a future call
	 * to trace_symbol() will work.  If they are are written to an output
	 * file then they are updated in the output memory buffer by the
	 * routines that output the symbols so to leave the merged symbol table
	 * data structure the way it is.
	 */
	section_map = &(object_file->section_maps[nlist->n_sect - 1]);
#ifndef RLD
	nlist->n_sect = section_map->output_section->output_sectnum;
#endif RLD

	/*
	 * If this symbol comes from base file of an incremental load
	 * then it's value is not adjusted.
	 */
	if(object_file == base_obj)
	    return;
	/*
	 * Adjust the value of this symbol by it's section.  The base
	 * of the section in the object file it came from is subtracted
	 * the base of the section in the output file is added and the
	 * offset this section appears in the output section is added.
	 *
	 * value += - old_section_base_address
	 *	    + new_section_base_address
	 *	    + offset_in_the_output_section;
	 *
	 * If the symbol is in a section that has fine relocation then
	 * it's value is set to where the value is in the output file
	 * by using the offset in the input file's section and getting
	 * the offset in the output file's section (via the fine
	 * relocation structures) and adding the address of that section
	 * in the output file.
	 */
	if(section_map->nfine_relocs == 0)
	    nlist->n_value += - section_map->s->addr
			      + section_map->output_section->s.addr
			      + section_map->offset;
	else
	    nlist->n_value = fine_reloc_output_offset(section_map,
						      nlist->n_value -
						      section_map->s->addr)
			     + section_map->output_section->s.addr;
}

/*
 * merge_symbols() merges the symbols from the current object (cur_obj) into
 * the merged symbol table.
 */
void
merge_symbols(void)
{
    long i, j, object_undefineds;
    struct nlist *object_symbols;
    char *object_strings;
    struct merged_symbol **hash_pointer, *merged_symbol;

#if defined(DEBUG) || defined(RLD)
	/* The compiler "warning: `merged_symbol' may be used uninitialized */
	/* in this function" can safely be ignored */
	merged_symbol = NULL;
#endif

	/* If this object file has no symbols then just return */
	if(cur_obj->symtab == NULL)
	    return;

	/* setup pointers to the symbol table and string table */
	object_symbols = (struct nlist *)(cur_obj->obj_addr +
					  cur_obj->symtab->symoff);
	object_strings = (char *)(cur_obj->obj_addr + cur_obj->symtab->stroff);


	/*
	 * For all the strings of the symbols to be valid the string table must
	 * end with a '\0'.
	 */
	if(cur_obj->symtab->strsize > 0 &&
	   object_strings[cur_obj->symtab->strsize - 1] != '\0'){
	    error_with_cur_obj("string table does not end with a '\\0'");
	    return;
	}

	/*
	 * If this object is not the base file count the number of undefined
	 * externals and commons in this object so that an undefined external
	 * map for this object can be allocated and then it will be filled in 
	 * as these undefined external symbols are looked up in the merged
	 * symbol table.  This map will be used when doing relocation for
	 * external relocation entries in pass2 (and is not needed for the base
	 * file because that is not relocated or copied in to the output).
	 */
        if(cur_obj != base_obj){
	    object_undefineds = 0;
	    for(i = 0; i < cur_obj->symtab->nsyms; i++){
		check_symbol(&(object_symbols[i]), object_strings, i);
		if(errors)
		    return;
		if(object_symbols[i].n_type == (N_EXT | N_UNDF))
		    object_undefineds++;
	    }
	    cur_obj->nundefineds = object_undefineds;
	    if(cur_obj->nundefineds != 0)
		cur_obj->undefined_maps = allocate(object_undefineds *
						  sizeof(struct undefined_map));
	}

	/*
	 * If local section object symbols were specified and if local symbols
	 * are to appear in the output file see if this object file has this
	 * section and if so account for this symbol.
	 */
	if(sect_object_symbols.specified &&
	   strip_level != STRIP_ALL && strip_level != STRIP_NONGLOBALS &&
	   (cur_obj != base_obj || strip_base_symbols == FALSE)){
	    if(sect_object_symbols.ms == NULL)
	        sect_object_symbols.ms = lookup_merged_section(
						  sect_object_symbols.segname,
						  sect_object_symbols.sectname);
	    if(sect_object_symbols.ms != NULL){
		if(sect_object_symbols.ms->s.flags == S_CSTRING_LITERALS ||
		   sect_object_symbols.ms->s.flags == S_4BYTE_LITERALS ||
		   sect_object_symbols.ms->s.flags == S_8BYTE_LITERALS ||
		   sect_object_symbols.ms->s.flags == S_LITERAL_POINTERS){
		    warning("section (%s,%s) is a literal section "
			    "and can't be used with -sectobjectsymbols",
			    sect_object_symbols.segname,
			    sect_object_symbols.sectname);
		    sect_object_symbols.specified = FALSE;
		}
		else{
		    /*
		     * See if this object file has the section that the section
		     * object symbols are being created for.
		     */
		    for(i = 0; i < cur_obj->nsection_maps; i++){
			if(sect_object_symbols.ms ==
			   cur_obj->section_maps[i].output_section){
			    nlocal_symbols++;
			    if(cur_obj->ar_hdr == NULL)
				local_string_size +=
						 strlen(cur_obj->file_name) + 1;
			    else
				local_string_size +=
					   strlen(obj_member_name(cur_obj)) + 1;
			    break;
			}
		    }
		}
	    }
	}

	/*
	 * Now merge the external symbols are looked up and merged based
	 * what was found if anything.  Locals are counted if they will
	 * appear in the output file based on the strip level.
	 */
	if(strip_level == STRIP_NONE &&
	   (cur_obj != base_obj || strip_base_symbols == FALSE)){
	    cur_obj->local_string_offset = local_string_size;
	    local_string_size += cur_obj->symtab->strsize;
	}
	object_undefineds = 0;
	for(i = 0; i < cur_obj->symtab->nsyms; i++){
	    if(object_symbols[i].n_type & N_EXT){
		/*
		 * Do the trace of this symbol if specified.
		 */
		if(ntrace_syms != 0){
		    for(j = 0; j < ntrace_syms; j++){
			if(strcmp(trace_syms[j], object_strings +
				  object_symbols[i].n_un.n_strx) == 0){
			    trace_object_symbol(&(object_symbols[i]),
						object_strings);
			    break;
			}
		    }
		}
		/* lookup the symbol and see if it has already been seen */
		hash_pointer = lookup_symbol(object_strings +
					     object_symbols[i].n_un.n_strx);
		if(*hash_pointer == NULL){
		    /*
		     * If this is the basefile and the symbol is not a
		     * definition of a symbol (or an indirect) then don't enter
		     * this symbol into the symbol table.
		     */
		    if(cur_obj != base_obj ||
		       (object_symbols[i].n_type != (N_EXT | N_UNDF) &&
		        object_symbols[i].n_type != (N_EXT | N_INDR) ) ){
			/* the symbol has not been seen yet so just enter it */
			merged_symbol = enter_symbol(hash_pointer,
					         &(object_symbols[i]),
						 object_strings, cur_obj);
		    }
		}
		/* the symbol has been seen so merge it */
		else{
		    merged_symbol = *hash_pointer;
		    /*
		     * If the object's symbol was undefined ignore it and just
		     * use the merged symbol.
		     */
		    if(object_symbols[i].n_type == (N_EXT | N_UNDF) &&
		       object_symbols[i].n_value == 0){
			;
		    }
		    /*
		     * See if the object's symbol is a common.
		     */
		    else if(object_symbols[i].n_type == (N_EXT | N_UNDF) &&
			    object_symbols[i].n_value != 0){
			/*
			 * See if the merged symbol is a common or undefined.
			 */
			if(merged_symbol->nlist.n_type == (N_EXT | N_UNDF)){
			    /*
			     * If the merged symbol is a common use the larger
			     * of the two commons.  Else the merged symbol is
			     * a common so use the common symbol.
			     */
			    if(merged_symbol->nlist.n_value != 0){
				if(object_symbols[i].n_value >
				   merged_symbol->nlist.n_value)
				    merged_symbol->nlist.n_value =
						     object_symbols[i].n_value;
			    }
			    else
				merged_symbol->nlist.n_value =
						     object_symbols[i].n_value;
			}
			/*
			 * The merged symbol is not a common or undefined and
			 * the object symbol is a common so just ignore the
			 * object's common symbol and use the merged defined
			 * symbol.
			 */
		    }
		    /*
		     * If the merged symbol is undefined or common (and at this
		     * point the object's symbol is known not to be undefined
		     * or common) used the object's symbol.
		     */
		    else if(merged_symbol->nlist.n_type == (N_EXT | N_UNDF)){
			/* one could also say:
			 *  && merged_symbol->nlist.n_value == 0 &&
			 *     merged_symbol->nlist.n_value != 0
			 * if the above test but that is always true.
			 */
			merged_symbol->nlist.n_type = object_symbols[i].n_type;
			merged_symbol->nlist.n_sect = object_symbols[i].n_sect;
			merged_symbol->nlist.n_desc = object_symbols[i].n_desc;
			if(merged_symbol->nlist.n_type == (N_EXT | N_INDR))
			    enter_indr_symbol(merged_symbol,
					      &(object_symbols[i]),
					      object_strings, cur_obj);
			else
			    merged_symbol->nlist.n_value =
						      object_symbols[i].n_value;
			merged_symbol->definition_object = cur_obj;
		    }
		    /*
		     * Now all other combinations of symbols are multiply
		     * defined.
		     */
#ifdef 0
		    /*
		     * This exception has been removed in the cc-19 release.
		     * The line of thinking is that there are cases where
		     * absolute symbols are used only to get the behavior of
		     * defined and undefined symbols for things like correct
		     * archive semantics.  So the thought is that if more than
		     * one of these symbols is loaded then the behavior of
		     * multiply defined symbols should occur.
		     *
		     * The one exception that is allowed is if both
		     * symbols are absolute symbols and have the same value.
		     */
		    else if(merged_symbol->nlist.n_type != (N_EXT | N_ABS) ||
			    object_symbols[i].n_type != (N_EXT | N_ABS) ||
			    object_symbols[i].n_value !=
						  merged_symbol->nlist.n_value){
			multiply_defined(merged_symbol, &(object_symbols[i]),
					 object_strings);
		    }
#else
		    else
			multiply_defined(merged_symbol, &(object_symbols[i]),
					 object_strings);
#endif 0
		}
		/*
		 * If this symbol was undefined or a common in this object
		 * and the object is not the basefile enter a pointer to the
		 * merged symbol and its index in the object file's undefined
		 * map.
		 */
		if(object_symbols[i].n_type == (N_EXT | N_UNDF) &&
		   cur_obj != base_obj){
		    cur_obj->undefined_maps[object_undefineds].index = i;
		    cur_obj->undefined_maps[object_undefineds].merged_symbol =
								merged_symbol;
		    object_undefineds++;
		}
	    }
	    else if(cur_obj != base_obj || strip_base_symbols == FALSE){
		if(strip_level == STRIP_NONE){
		    nlocal_symbols++;
		}
		else if(is_output_local_symbol(object_symbols[i].n_type,
			    object_symbols[i].n_un.n_strx == 0 ? "" :
			    object_strings + object_symbols[i].n_un.n_strx)){
		    nlocal_symbols++;
		    local_string_size += object_symbols[i].n_un.n_strx == 0 ? 0:
					 strlen(object_strings +
						object_symbols[i].n_un.n_strx)
					 + 1;
		}
	    }
	}
}

#ifndef RLD
/*
 * command_line_symbol() looks up a symbol name that comes from a command line
 * argument (like -u symbol_name) and returns a pointer to the merged symbol
 * table entry for it.  If the symbol doesn't exist it enters an undefined
 * symbol for it.
 */
struct merged_symbol *
command_line_symbol(
char *symbol_name)
{
    long i;
    struct merged_symbol **hash_pointer, *merged_symbol;

	/*
	 * Do the trace of this symbol if specified.
	 */
	if(ntrace_syms != 0){
	    for(i = 0; i < ntrace_syms; i++){
		if(strcmp(trace_syms[i], symbol_name) == 0){
		    trace_symbol(symbol_name, &(undefined_symbol),
			     &(command_line_object), "error in trace_symbol()");
		    break;
		}
	    }
	}
	/* lookup the symbol and see if it has already been seen */
	hash_pointer = lookup_symbol(symbol_name);
	if(*hash_pointer == NULL){
	    /*
	     * The symbol has not been seen yet so just enter it as an
	     * undefined symbol and it will be returned.
	     */
	    merged_symbol = enter_symbol(hash_pointer, &(undefined_symbol),
					 symbol_name, &(command_line_object));
	}
	/* the symbol has been seen so just use it */
	else{
	    merged_symbol = *hash_pointer;
	}
	return(merged_symbol);
}

/*
 * command_line_indr_symbol() creates an indirect symbol for symbol_name to
 * indr_symbol_name.  It is used for -i command line options.  Since this is
 * a defining symbol the problems of multiply defined symbols can happen.  This
 * and the tracing is not too neat as far as the code goes but it does exactly
 * what is intended.  That is exactly one error message for each symbol and
 * exactly one trace for each object or command line option for each symbol.
 */
void
command_line_indr_symbol(
char *symbol_name,
char *indr_symbol_name)
{
    long i, j;
    enum bool was_traced;
    struct merged_symbol **hash_pointer, *merged_symbol, *merged_indr_symbol;

	/*
	 * Do the trace of the symbol_name if specified.
	 */
	was_traced = FALSE;
	if(ntrace_syms != 0){
	    for(i = 0; i < ntrace_syms; i++){
		if(strcmp(trace_syms[i], symbol_name) == 0){
		    trace_symbol(symbol_name, &(indr_symbol),
				 &(command_line_object), indr_symbol_name);
		    was_traced = TRUE;
		    break;
		}
	    }
	}
	/* lookup the symbol_name and see if it has already been seen */
	hash_pointer = lookup_symbol(symbol_name);
	if(*hash_pointer == NULL){
	    /*
	     * The symbol has not been seen yet so just enter it as an
	     * undefined and it will be changed to a proper merged indirect
	     * symbol.
	     */
	    merged_symbol = enter_symbol(hash_pointer, &(undefined_symbol),
					 symbol_name, &(command_line_object));
	}
	else{
	    /*
	     * The symbol exist.  So if the symbol is anything but a common or
	     * undefined then it is multiply defined.
	     */
	    merged_symbol = *hash_pointer;
	    if(merged_symbol->nlist.n_type != (N_UNDF | N_EXT)){
		/*
		 * It is multiply defined so the logic of the routine
		 * multiply_defined() is copied here so that tracing a symbol
		 * from the command line can be done.
		 */
		for(i = 0; i < nmultiple_defs; i++){
		    if(strcmp(multiple_defs[i],
			      merged_symbol->nlist.n_un.n_name) == 0)
			break;
		}
		for(j = 0; j < ntrace_syms; j++){
		    if(strcmp(trace_syms[j],
			      merged_symbol->nlist.n_un.n_name) == 0)
			break;
		}
		if(i == nmultiple_defs){
		    error("multiple definitions of symbol %s",
			  merged_symbol->nlist.n_un.n_name);
		    multiple_defs = reallocate(multiple_defs, (nmultiple_defs +
					       1) * sizeof(char *));
		    multiple_defs[nmultiple_defs++] =
					       merged_symbol->nlist.n_un.n_name;
		    if(j == ntrace_syms)
			trace_merged_symbol(merged_symbol);
		}
		if(was_traced == FALSE)
		    trace_symbol(symbol_name, &(indr_symbol),
				 &(command_line_object), indr_symbol_name);
		return;
	    }
	}
	nindr_symbols++;
	/* Now change this symbol to an indirect symbol type */
	merged_symbol->nlist.n_type = N_INDR | N_EXT;
	merged_symbol->nlist.n_sect = NO_SECT;
	merged_symbol->nlist.n_desc = 0;

	/* lookup the indr_symbol_name and see if it has already been seen */
	hash_pointer = lookup_symbol(indr_symbol_name);
	if(*hash_pointer == NULL){
	    /*
	     * The symbol has not been seen yet so just enter it after tracing
	     * if the symbol is specified.
	     */
	    for(i = 0; i < ntrace_syms; i++){
		if(strcmp(trace_syms[i], indr_symbol_name) == 0){
		    trace_symbol(indr_symbol_name, &(undefined_symbol),
			     &(command_line_object), "error in trace_symbol()");
		    break;
		}
	    }
	    merged_indr_symbol = enter_symbol(hash_pointer, &(undefined_symbol),
				      indr_symbol_name, &(command_line_object));
	}
	else{
	    merged_indr_symbol = *hash_pointer;
	}
	merged_symbol->nlist.n_value = (unsigned long)merged_indr_symbol;
}
#endif !defined(RLD)

/*
 * is_output_local_symbol() returns TRUE or FALSE depending if the local symbol
 * type and name passed to it will be in the output file's symbol table based
 * on the level of symbol stripping.
 */
static
enum bool
is_output_local_symbol(
unsigned char n_type,
char *symbol_name)
{
	switch(strip_level){
	    case STRIP_NONE:
		return(TRUE);
	    case STRIP_ALL:
	    case STRIP_NONGLOBALS:
		return(FALSE);
	    case STRIP_DEBUG:
		if(n_type & N_STAB ||
		   (*symbol_name == 'L' && (n_type & N_STAB) == 0))
		    return(FALSE);
		else
		    return(TRUE);
	    case STRIP_L_SYMBOLS:
		if(*symbol_name == 'L' && (n_type & N_STAB) == 0)
		    return(FALSE);
		else
		    return(TRUE);
	}
	/* never gets here but shuts up a bug in -Wall */
	return(TRUE);
}

/*
 * lookup_symbol() returns a pointer to a hash_table entry for the symbol name
 * passed to it.  Either the symbol is found in which case the hash_table entry
 * pointed to by the return value points to the merged_symbol for that symbol.
 * If the symbol is not found the hash_table entry pointed to the the return
 * value is NULL.  In this case that pointer can be used in the call to
 * enter_symbol() to enter the symbol.  This is the routine that actually
 * allocates the merged_symbol_list's and enter_symbol() just uses the
 * hash_pointer returned and the list pointer that is set into
 * merged_symbol_list_for_enter_symbol to enter the symbol.
 */
struct merged_symbol **
lookup_symbol(
char *symbol_name)
{
    struct merged_symbol_list **p, *merged_symbol_list;
    struct merged_symbol **hash_pointer;
    long hash_index, i;

	hash_index = hash_string(symbol_name) % SYMBOL_LIST_HASH_SIZE;
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    hash_pointer = merged_symbol_list->hash_table + hash_index;
	    i = 1;
	    do{
		if(*hash_pointer == NULL){
		    if(merged_symbol_list->used == NSYMBOLS)
			break;
		    merged_symbol_list_for_enter_symbol = merged_symbol_list;
		    return(hash_pointer);
		}
		if(strcmp((*hash_pointer)->nlist.n_un.n_name, symbol_name) == 0)
		    return(hash_pointer);
		hash_pointer += i;
		i += 2;
		if(hash_pointer >=
		   merged_symbol_list->hash_table + SYMBOL_LIST_HASH_SIZE)
		    hash_pointer -= SYMBOL_LIST_HASH_SIZE;
	    }while(i < SYMBOL_LIST_HASH_SIZE);
	    if(i > SYMBOL_LIST_HASH_SIZE)
		fatal("internal error, lookup_symbol() failed");
	}
	*p = allocate(sizeof(struct merged_symbol_list));
	merged_symbol_list = *p;
	merged_symbol_list->used = 0;
	merged_symbol_list->next = NULL;
	merged_symbol_list->hash_table = allocate(sizeof(struct merged_symbol *)
						 * SYMBOL_LIST_HASH_SIZE);
	memset(merged_symbol_list->hash_table, '\0',
	       sizeof(struct merged_symbol *) * SYMBOL_LIST_HASH_SIZE);
	hash_pointer = merged_symbol_list->hash_table + hash_index;
	merged_symbol_list_for_enter_symbol = merged_symbol_list;
	return(hash_pointer);
}

/*
 * enter_symbol() enters the symbol passed to it in the merged symbol table (in
 * the segment of the list pointed to by merged_symbol_list_for_enter_symbol)
 * and sets the hash table pointer to passed to it to the merged_symbol.
 */
static
struct merged_symbol *
enter_symbol(
struct merged_symbol **hash_pointer,
struct nlist *object_symbol,
char *object_strings,
struct object_file *definition_object)
{
    struct merged_symbol *merged_symbol;

	if(hash_pointer <  merged_symbol_list_for_enter_symbol->hash_table ||
	   hash_pointer >= merged_symbol_list_for_enter_symbol->hash_table +
			   SYMBOL_LIST_HASH_SIZE)
	    fatal("internal error, enter_symbol() passed bad hash_pointer");

	merged_symbol = merged_symbol_list_for_enter_symbol->merged_symbols +
			merged_symbol_list_for_enter_symbol->used++;
	if(cur_obj != base_obj || strip_base_symbols == FALSE)
	    nmerged_symbols++;
	*hash_pointer = merged_symbol;
	merged_symbol->nlist = *object_symbol;
	merged_symbol->nlist.n_un.n_name = enter_string(object_strings +
						 object_symbol->n_un.n_strx);
	merged_symbol->definition_object = definition_object;
	if(object_symbol->n_type == (N_UNDF | N_EXT) &&
	   object_symbol->n_value == 0)
	    add_to_undefined_list(merged_symbol);

	if(object_symbol->n_type == (N_INDR | N_EXT))
	    enter_indr_symbol(merged_symbol, object_symbol, object_strings,
			      definition_object);
	return(merged_symbol);
}

/*
 * enter_indr_symbol() enters the indirect symbol for the object_symbol passed
 * to it into the merged_symbol passed to it.
 */
static
void
enter_indr_symbol(
struct merged_symbol *merged_symbol,
struct nlist *object_symbol,
char *object_strings,
struct object_file *definition_object)
{
    struct merged_symbol **hash_pointer, *indr_symbol;

	nindr_symbols++;
	hash_pointer = lookup_symbol(object_strings + object_symbol->n_value);
	if(*hash_pointer != NULL){
	    indr_symbol = *hash_pointer;
	}
	else{
	    indr_symbol =
		    merged_symbol_list_for_enter_symbol->merged_symbols +
		    merged_symbol_list_for_enter_symbol->used++;
	    if(cur_obj != base_obj || strip_base_symbols == FALSE)
		nmerged_symbols++;
	    *hash_pointer = indr_symbol;
	    indr_symbol->nlist.n_type = N_UNDF | N_EXT;
	    indr_symbol->nlist.n_sect = NO_SECT;
	    indr_symbol->nlist.n_desc = 0;
	    indr_symbol->nlist.n_value = 0;
	    indr_symbol->nlist.n_un.n_name = enter_string(object_strings +
						      object_symbol->n_value);
	    indr_symbol->definition_object = definition_object;
	    add_to_undefined_list(indr_symbol);
	}
	merged_symbol->nlist.n_value = (unsigned long)indr_symbol;
}
/*
 * enter_string() places the symbol_name passed to it in the first string block
 * that will hold the string.  Since the string indexes will be assigned after
 * all the strings are entered putting the strings in the first block that fits
 * can be done rather than only last block.
 */
static
char *
enter_string(
char *symbol_name)
{
    struct string_block **p, *string_block;
    long len;
    char *r;

	len = strlen(symbol_name) + 1;
	for(p = &(merged_string_blocks); *p; p = &(string_block->next)){
	    string_block = *p;
	    if(len > string_block->size - string_block->used)
		continue;
#ifdef RLD
	    if(string_block->set_num != cur_set)
		continue;
#endif RLD
	    if(strip_base_symbols == TRUE &&
	       (cur_obj == base_obj && string_block->base_strings == FALSE) ||
	       (cur_obj != base_obj && string_block->base_strings == TRUE) )
		continue;
	    r = strcpy(string_block->strings + string_block->used, symbol_name);
	    string_block->used += len;
	    if(strip_base_symbols == FALSE ||
	       string_block->base_strings == FALSE)
		merged_string_size += len;
	    return(r);
	}
	*p = allocate(sizeof(struct string_block));
	string_block = *p;
	string_block->size = (len > host_pagesize ? len : host_pagesize);
	string_block->used = len;
	string_block->next = NULL;
	string_block->strings = allocate(string_block->size);
	string_block->base_strings = cur_obj == base_obj ? TRUE : FALSE;
#ifdef RLD
	string_block->set_num = cur_set;
#endif RLD
	r = strcpy(string_block->strings, symbol_name);
	if(strip_base_symbols == FALSE ||
	   string_block->base_strings == FALSE)
	    merged_string_size += len;
	return(r);
}

/*
 * add_to_undefined_list() adds a pointer to a merged symbol to the list of
 * undefined symbols.
 */
static
void
add_to_undefined_list(
struct merged_symbol *merged_symbol)
{
    struct undefined_block **p;
    struct undefined_list *new, *undefineds;
    long i;

	if(free_list.next == &free_list){
	    for(p = &(undefined_blocks); *p; p = &((*p)->next))
		;
	    *p = allocate(sizeof(struct undefined_block));
	    (*p)->next = 0;
	    undefineds = (*p)->undefineds;

	    /* add the newly allocated items to the empty free_list */
	    free_list.next = &undefineds[0];
	    undefineds[0].prev = &free_list;
	    undefineds[0].next = &undefineds[1];
	    for(i = 1 ; i < NUNDEF_BLOCKS - 1 ; i++){
		undefineds[i].prev  = &undefineds[i-1];
		undefineds[i].next  = &undefineds[i+1];
		undefineds[i].merged_symbol = NULL;
	    }
	    free_list.prev = &undefineds[i];
	    undefineds[i].prev = &undefineds[i-1];
	    undefineds[i].next = &free_list;
	}
	/* take the first one off the free list */
	new = free_list.next;
	new->next->prev = &free_list;
	free_list.next = new->next;

	/* fill in the pointer to the undefined symbol */
	new->merged_symbol = merged_symbol;

	/* put this at the end of the undefined list */
	new->prev = undefined_list.prev;
	new->next = &undefined_list;
	undefined_list.prev->next = new;
	undefined_list.prev = new;
}

/*
 * delete_from_undefined_list() is used by pass1() after a member is loaded from
 * an archive that satisifies an undefined symbol.  It is also called from
 * pass1() when it comes across a symbol on the undefined list that is no longer
 * undefined.
 */
void
delete_from_undefined_list(
struct undefined_list *undefined)
{
	/* take this out of the list */
	undefined->prev->next = undefined->next;
	undefined->next->prev = undefined->prev;

	/* put this at the end of the free list */
	undefined->prev = free_list.prev;
	undefined->next = &free_list;
	free_list.prev->next = undefined;
	free_list.prev = undefined;
	undefined->merged_symbol = NULL;
}

/*
 * multiply_defined() prints and traces the multiply defined symbol if it hasn't
 * been printed yet.  It's slow with it linear searches and a relocate but this
 * is an error case.
 */
static
void
multiply_defined(
struct merged_symbol *merged_symbol,
struct nlist *object_symbol,
char *object_strings)
{
    long i, j;

	for(i = 0; i < nmultiple_defs; i++){
	    if(strcmp(multiple_defs[i], merged_symbol->nlist.n_un.n_name) == 0)
		break;
	}
	for(j = 0; j < ntrace_syms; j++){
	    if(strcmp(trace_syms[j], merged_symbol->nlist.n_un.n_name) == 0)
		break;
	}
	if(i == nmultiple_defs){
	    error("multiple definitions of symbol %s",
		  merged_symbol->nlist.n_un.n_name);
	    multiple_defs = reallocate(multiple_defs,
				       (nmultiple_defs + 1) * sizeof(char *));
	    multiple_defs[nmultiple_defs++] = merged_symbol->nlist.n_un.n_name;
	    if(j == ntrace_syms)
		trace_merged_symbol(merged_symbol);
	}
	if(j == ntrace_syms)
	    trace_object_symbol(object_symbol, object_strings);
}

/*
 * trace_object_symbol() traces a symbol that comes from an object file.
 */
static
void
trace_object_symbol(
struct nlist *symbol,
char *strings)
{
    char *indr_symbol_name;

	if(symbol->n_type == (N_INDR | N_EXT))
	    indr_symbol_name = strings + symbol->n_value;
	else
	    indr_symbol_name = "error in trace_symbol()";
	trace_symbol(strings + symbol->n_un.n_strx, symbol, cur_obj,
		     indr_symbol_name);
}

/*
 * trace_merged_symbol() traces a symbol that is in the merged symbol table.
 */
static
void
trace_merged_symbol(
struct merged_symbol *merged_symbol)
{
    char *indr_symbol_name;

	if(merged_symbol->nlist.n_type == (N_INDR | N_EXT))
	    indr_symbol_name = ((struct merged_symbol *)
			(merged_symbol->nlist.n_value))->nlist.n_un.n_name;
	else
	    indr_symbol_name = "error in trace_symbol()";
	trace_symbol(merged_symbol->nlist.n_un.n_name, &(merged_symbol->nlist),
		     merged_symbol->definition_object, indr_symbol_name);
}

/*
 * trace_symbol() is the routine that really does the work of printing the
 * symbol its type and the file it is in.
 */
static
void
trace_symbol(
char *symbol_name,
struct nlist *nlist,
struct object_file *object_file,
char *indr_symbol_name)
{
	print_obj_name(object_file);
	switch(nlist->n_type){
	case N_UNDF | N_EXT:
	    if(nlist->n_value == 0)
		print("reference to undefined %s\n", symbol_name);
	    else
		print("definition of common %s (size %d)\n", symbol_name,
		       nlist->n_value);
	    break;
	case N_ABS | N_EXT:
	    print("definition of absolute %s (value 0x%x)\n", symbol_name,
		   nlist->n_value);
	    break;
	case N_SECT | N_EXT:
	    print("definition of %s in section (%0.16s,%0.16s)\n", symbol_name,
		   object_file->section_maps[nlist->n_sect - 1].s->segname,
		   object_file->section_maps[nlist->n_sect - 1].s->sectname);
	    break;
	case N_INDR | N_EXT:
	    print("definition of %s as indirect for %s\n", symbol_name,
		   indr_symbol_name);
	    break;
	default:
	    print("unknown type (0x%x) of %s\n", nlist->n_type, symbol_name);
	    break;
	}
}

#ifndef RLD
/*
 * free_pass1_symbol_data() free()'s all symbol data only used in pass1().
 */
void
free_pass1_symbol_data(void)
{
    struct merged_symbol_list **p, *merged_symbol_list;

	/*
	 * Free the hash table for the symbol lists.
	 */
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    if(merged_symbol_list->hash_table != NULL){
		free(merged_symbol_list->hash_table);
		merged_symbol_list->hash_table = NULL;
	    }
	}
	free_undefined_list();
}
#endif !defined(RLD)

/*
 * free_undefined_list() free's up the memory for the undefined list.
 */
void
free_undefined_list(void)
{
    struct undefined_block *up, *undefined_block;
	/*
	 * Free the undefined list
	 */
	for(up = undefined_blocks; up; ){
	    undefined_block = up->next;
	    free(up);
	    up = undefined_block;
	}
	undefined_blocks = NULL;
	undefined_list.next = &undefined_list;
	undefined_list.prev = &undefined_list;
	free_list.next = &free_list;
	free_list.prev = &free_list;
}

/*
 * define_common_symbols() defines common symbols if there are any in the merged
 * symbol table.  The symbols are defined in the link editor reserved zero-fill
 * section (__DATA,__common) and the segment and section are created if needed.
 * The section is looked up to see it there is a section specification for it
 * and if so the same processing as in process_section_specs() is done here.
 * If there is a spec it uses the alignment if it is greater than the merged
 * alignment and warns if it is less.  Also it checks to make sure that no
 * section is to be created from a file for this reserved section.
 */
void
define_common_symbols(void)
{
    struct section_spec *sect_spec;
    struct merged_section *ms;
    struct section *s;

    unsigned long i, j, common_size, align;
    struct merged_symbol_list **p, *merged_symbol_list;
    struct merged_symbol *merged_symbol;
    struct common_symbol *common_symbol;

    struct object_list *object_list, **q;
    struct object_file *object_file;

    struct nlist *common_nlist;
    char *common_names;
    long n_strx;

#if defined(DEBUG) || defined(RLD)
	/*
	 * The compiler warning that these symbols may be used uninitialized
	 * in this function can safely be ignored.
	 */
	common_symbol = NULL;
	common_nlist = NULL;
	common_names = NULL;;
	n_strx = 0;
#endif

#ifdef RLD
	sets[cur_set].link_edit_common_object =
		      link_edit_common_object;
	sets[cur_set].link_edit_common_object.set_num =
		      cur_set;
	sets[cur_set].link_edit_common_object.section_maps = 
		      &sets[cur_set].link_edit_section_maps;
	sets[cur_set].link_edit_section_maps =
		      link_edit_section_maps;
	sets[cur_set].link_edit_section_maps.s =
		      &sets[cur_set].link_edit_common_section;
	sets[cur_set].link_edit_common_section =
		      link_edit_common_section;
#endif RLD

#ifndef RLD
	/* see if there is a section spec for (__DATA,__common) */
	sect_spec = lookup_section_spec(SEG_DATA, SECT_COMMON);
	if(sect_spec != NULL){
	    if(sect_spec->contents_filename != NULL){
		error("section (" SEG_DATA "," SECT_COMMON ") reserved for "
		      "allocating common symbols and can't be created from the "
		      "file: %s", sect_spec->contents_filename);
		return;
	    }
	    sect_spec->processed = TRUE;
	}
#else
	sect_spec = NULL;
#endif !defined(RLD)

	/* see if there is a merged section for (__DATA,__common) */
	ms = lookup_merged_section(SEG_DATA, SECT_COMMON);
	if(ms != NULL && ms->s.flags != S_ZEROFILL){
	    error("section (" SEG_DATA "," SECT_COMMON ") reserved for "
		  "allocating common symbols and exists in the loaded "
		  "objects not as a zero fill section");
	    /*
	     * Loop through all the objects and report those that have this
	     * section and then return.
	     */
	    for(q = &objects; *q; q = &(object_list->next)){
		object_list = *q;
		for(i = 0; i < object_list->used; i++){
		    object_file = &(object_list->object_files[i]);
		    for(j = 0; j < object_file->nsection_maps; j++){
			s = object_file->section_maps[j].s;
			if(strcmp(s->segname, SEG_DATA) == 0 &&
			   strcmp(s->sectname, SECT_COMMON) == 0){
			    print_obj_name(object_file);
			    print("contains section (" SEG_DATA ","
				   SECT_COMMON ")\n");
			}
		    }
		}
	    }
	    return;
	}
#ifndef RLD
	else{
	    /*
	     * This needs to be done here on the chance there is a common
	     * section but no commons get defined.  This is also done below
	     * if the common section is created.
	     */
	    if(sect_spec != NULL && sect_spec->order_filename != NULL &&
	       ms != NULL){
		ms->order_filename = sect_spec->order_filename;
		ms->order_addr = sect_spec->order_addr;
		ms->order_size = sect_spec->order_size;
	    }
	}
#endif !defined(RLD)

	/*
	 * Determine if there are any commons to be defined if not just return.
	 * If a load map is requested then the number of commons to be defined
	 * is determined so a common load map can be allocated.
	 */
	commons_exist = FALSE;
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    for(i = 0; i < merged_symbol_list->used; i++){
		merged_symbol = &(merged_symbol_list->merged_symbols[i]);
		if(merged_symbol->nlist.n_type == (N_EXT | N_UNDF) &&
		   merged_symbol->nlist.n_value != 0){
		    /*
		     * If the output format is MH_FVMLIB then commons are not
		     * allowed because it there address may not remain fixed
		     * on sucessive link edits.  Each one is traced below.
		     */
		    if(filetype == MH_FVMLIB)
			error("common symbols not allowed with MH_FVMLIB "
			      "output format");
		    commons_exist = TRUE;
#ifndef RLD
		    if(sect_spec != NULL && sect_spec->order_filename != NULL){
			link_edit_common_symtab.nsyms++;
			link_edit_common_symtab.strsize +=
				   strlen(merged_symbol->nlist.n_un.n_name) + 1;
		    }
		    else if(load_map)
			common_load_map.ncommon_symbols++;
		    else
#endif !defined(RLD)
			break;
		}
	    }
	}
	if(commons_exist == FALSE)
	    return;

	/*
	 * Now that the checks above have been done if commons are not to be
	 * defined just return.
	 */
	if(define_comldsyms == FALSE)
	    return;

	/*
	 * Create the (__DATA,__common) section if needed and set the
	 * alignment for it.
	 */
	if(ms == NULL){
#ifdef RLD
	    ms = create_merged_section(
				     &(sets[cur_set].link_edit_common_section));
#else
	    ms = create_merged_section(&link_edit_common_section);
#endif RLD
	    if(sect_spec != NULL && sect_spec->align_specified)
		ms->s.align = sect_spec->align;
	    else
		ms->s.align = defaultsectalign;
	    if(sect_spec != NULL && sect_spec->order_filename != NULL){
		ms->order_filename = sect_spec->order_filename;
		ms->order_addr = sect_spec->order_addr;
		ms->order_size = sect_spec->order_size;
	    }
	}
	else{
	    if(sect_spec != NULL && sect_spec->align_specified){
		if(ms->s.align > sect_spec->align)
		    warning("specified alignment (0x%x) for section (" SEG_DATA
			    "," SECT_COMMON ") not used (less than the "
			    "required alignment in the input files (0x%x))",
			    1 << sect_spec->align, 1 << ms->s.align);
		else
		    ms->s.align = sect_spec->align;
	    }
	}

#ifndef RLD
	/*
	 * If the common section has an order file then create a symbol table
	 * and string table for it and the load map will be generated off of
	 * these tables in layout_ordered_section() in sections.c.  If not and
	 * a load map is requested then set up the common load map.  This is
	 * used by print_load_map() in layout.c and the common_symbols allocated
	 * here are free()'ed in there also.
	 */
	if(sect_spec != NULL && sect_spec->order_filename != NULL){
	    link_edit_common_symtab.strsize =
			round(link_edit_common_symtab.strsize, sizeof(long));
	    link_edit_common_object.obj_size =
			link_edit_common_symtab.nsyms * sizeof(struct nlist) +
			link_edit_common_symtab.strsize;
	    link_edit_common_object.obj_addr =
			allocate(link_edit_common_object.obj_size);
	    link_edit_common_symtab.symoff = 0;
	    link_edit_common_symtab.stroff = link_edit_common_symtab.nsyms *
					     sizeof(struct nlist);
	    common_nlist = (struct nlist *)link_edit_common_object.obj_addr;
	    common_names = (char *)(link_edit_common_object.obj_addr +
	    		            link_edit_common_symtab.stroff);
	    n_strx = 1;
	}
	else if(load_map){
	    common_load_map.common_ms = ms;
	    common_load_map.common_symbols = allocate(
					common_load_map.ncommon_symbols *
					sizeof(struct common_symbol));
	    common_symbol = common_load_map.common_symbols;
	}
#endif !defined(RLD)

	/*
	 * Now define the commons.  This is requires building a "link editor"
	 * object file and changing these symbols to be defined in the (__DATA,
	 * __common) section in that "file".  By doing this in this way these
	 * symbols are handled normally throught the rest of the link editor.
	 * Also these symbols are trace as they are defined if they are to be
	 * traced.
	 */
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    for(i = 0; i < merged_symbol_list->used; i++){
		merged_symbol = &(merged_symbol_list->merged_symbols[i]);
		if(merged_symbol->nlist.n_type == (N_EXT | N_UNDF) &&
		   merged_symbol->nlist.n_value != 0){
		    /*
		     * Commons are not allowed with MH_FVMLIB formats so trace
		     * each one.  An error message for this has been printed
		     * above.
		     */
		    if(filetype == MH_FVMLIB)
			trace_merged_symbol(merged_symbol);
		    /* determine the alignment of this symbol */
		    common_size = merged_symbol->nlist.n_value;
		    align = 0;
		    while(1 << align < common_size && align < ms->s.align)
			align++;
		    /* round the address of the section to this alignment */
#ifdef RLD
		    sets[cur_set].link_edit_common_section.size = round(
		       sets[cur_set].link_edit_common_section.size, 1 << align);
#else
		    link_edit_common_section.size = round(
				link_edit_common_section.size, 1 << align);
#endif RLD
		    /*
		     * Change this symbol's type, section number, address and
		     * object file it is defined in to be the (__DATA,__common)
		     * of the "link editor" object file at the address for it.
		     */
		    merged_symbol->nlist.n_type = N_SECT | N_EXT;
		    merged_symbol->nlist.n_sect = 1;
#ifdef RLD
		    merged_symbol->nlist.n_value =
				    sets[cur_set].link_edit_common_section.size;
		    merged_symbol->definition_object =
				       &(sets[cur_set].link_edit_common_object);
		    /* Create the space for this symbol */
		    sets[cur_set].link_edit_common_section.size += common_size;
#else
		    merged_symbol->nlist.n_value =link_edit_common_section.size;
		    merged_symbol->definition_object =
						&link_edit_common_object;
		    /* Create the space for this symbol */
		    link_edit_common_section.size += common_size;
#endif RLD
		    /*
		     * Do the trace of this symbol if specified now that it has
		     * been defined.
		     */
		    if(ntrace_syms != 0){
			for(j = 0; j < ntrace_syms; j++){
			    if(strcmp(trace_syms[j],
				      merged_symbol->nlist.n_un.n_name) == 0){
				trace_merged_symbol(merged_symbol);
				break;
			    }
			}
		    }
#ifndef RLD
		    /*
		     * Set the entries in the common symbol table if the section
		     * is to be ordered or in the load map if producing it
		     */
		    if(sect_spec != NULL && sect_spec->order_filename != NULL){
			common_nlist->n_un.n_strx = n_strx;
			common_nlist->n_type = N_SECT | N_EXT;
			common_nlist->n_sect = 1;
			common_nlist->n_desc = 0;
			common_nlist->n_value = merged_symbol->nlist.n_value;
			strcpy(common_names + n_strx,
			       merged_symbol->nlist.n_un.n_name);
			common_nlist++;
			n_strx += strlen(merged_symbol->nlist.n_un.n_name) + 1;
		    }
		    else if(load_map){
			common_symbol->merged_symbol = merged_symbol;
			common_symbol->common_size = common_size;
			common_symbol++;
		    }
#endif !defined(RLD)
		}
	    }
	}

	/*
	 * Now that this section in this "object file" is built merged it into
	 * the merged section list (as would be done in merge_sections()).
	 */
#ifdef RLD
	sets[cur_set].link_edit_common_object.section_maps[0].output_section =
									     ms;
	ms->s.size = round(ms->s.size, 1 << ms->s.align);
	sets[cur_set].link_edit_common_object.section_maps[0].offset =
								     ms->s.size;
	ms->s.size += sets[cur_set].link_edit_common_section.size;
#else
	link_edit_common_object.section_maps[0].output_section = ms;
	ms->s.size = round(ms->s.size, 1 << ms->s.align);
	link_edit_common_object.section_maps[0].offset = ms->s.size;
	ms->s.size += link_edit_common_section.size;
#endif RLD
}

#ifndef RLD
/*
 * define_link_editor_execute_symbols() is called when the output file type is
 * MH_EXECUTE and it defines the loader defined symbols for this file type.
 * For the MH_EXECUTE file type there is one loader defined symbol which is the
 * address of the header.  Since this symbol is not in a section (it is before
 * the first section) it is an absolute symbol.
 */
void
define_link_editor_execute_symbols(
unsigned long header_address)
{
	define_link_editor_symbol(_MH_EXECUTE_SYM, N_EXT | N_ABS, NO_SECT, 0,
				header_address, &link_edit_symbols_object);
}

/*
 * define_link_editor_preload_symbols() is called when the output file type is
 * MH_PRELOAD and it defines the loader defined symbols for this file type.
 * For the MH_PRELOAD file type there are loader defined symbols for the
 * beginning and ending of each segment and section.  Their names are of the
 * form: <segname>{,<sectname>}{__begin,__end} .  They are N_SECT symbols for
 * the closest section they belong to (in some cases the *__end symbols will
 * be outside the section).
 */
void
define_link_editor_preload_symbols(void)
{
    long nsects, i, j, first_section;
    struct section *sections;
    struct section_map *section_maps;
    struct merged_segment **p, *msg;
    struct merged_section **q, *ms;
    char symbol_name[sizeof(ms->s.segname) + sizeof(ms->s.sectname) +
		     sizeof("__begin")];

	/* count the number of merged sections */
	nsects = 0;
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    nsects += msg->sg.nsects;
	    p = &(msg->next);
	}

	/*
	 * Create the sections and section maps for the sections in the
	 * "link editor" object file.  To make it easy all merged sections
	 * will be in this object file.  The addr in all of the sections
	 * and the offset in all the maps will be zero so that
	 * layout_symbols() will set the final value of these symbols 
	 * to their correct location in the output file.
	 */
	sections = allocate(nsects * sizeof(struct section));
	memset(sections, '\0', nsects * sizeof(struct section));
	section_maps = allocate(nsects * sizeof(struct section_map));
	memset(section_maps, '\0', nsects * sizeof(struct section_map));
	link_edit_symbols_object.nsection_maps = nsects;
	link_edit_symbols_object.section_maps = section_maps;

	i = 0;
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    /* create the symbol for the beginning of the segment */
	    strncpy(symbol_name, msg->sg.segname, sizeof(msg->sg.segname));
	    strcat(symbol_name, "__begin");
	    define_link_editor_symbol(symbol_name, N_EXT | N_SECT, i+1, 0, 0,
				      &link_edit_symbols_object);
	    first_section = i + 1;
	    for(j = 0; j < 2 ; j++){
		if(j == 0)
		    /* process the content sections */
		    q = &(msg->content_sections);
		else
		    /* process the zerofill sections */
		    q = &(msg->zerofill_sections);
		while(*q){
		    ms = *q;
		    /* create the section and map for this section */
		    strncpy(sections[i].sectname, ms->s.sectname,
			    sizeof(ms->s.sectname));
		    strncpy(sections[i].segname, ms->s.segname,
			    sizeof(ms->s.segname));
		    section_maps[i].s = &(sections[i]);
		    section_maps[i].output_section = ms;
		    /* create the symbol for the beginning of the section */
		    strncpy(symbol_name, ms->s.segname,
			    sizeof(ms->s.segname));
		    strncat(symbol_name, ms->s.sectname,
			    sizeof(ms->s.sectname));
		    strcat(symbol_name, "__begin");
		    define_link_editor_symbol(symbol_name, N_EXT | N_SECT, i+1,
					      0, 0, &link_edit_symbols_object);
		    /* create the symbol for the end of the section */
		    strncpy(symbol_name, ms->s.segname,
			    sizeof(ms->s.segname));
		    strncat(symbol_name, ms->s.sectname,
			    sizeof(ms->s.sectname));
		    strcat(symbol_name, "__end");
		    define_link_editor_symbol(symbol_name, N_EXT | N_SECT, i+1,
				      0, ms->s.size, &link_edit_symbols_object);
		    i++;
		    q = &(ms->next);
		}
	    }
	       
	    /* create the symbol for the end of the segment */
	    strncpy(symbol_name, msg->sg.segname,
		    sizeof(msg->sg.segname));
	    strcat(symbol_name, "__end");
	    define_link_editor_symbol(symbol_name, N_EXT | N_SECT,
				      first_section, 0, msg->sg.vmsize,
				      &link_edit_symbols_object);
	    p = &(msg->next);
	}
}

/*
 * define_link_editor_symbol() is passed then name of a link editor defined
 * symbol and the information to define it.  If this symbol exist it must be
 * undefined or it is an error.  If it exist and link editor defined symbols
 * are being defined it is defined using the information passed to it.
 */
static
void
define_link_editor_symbol(
char *symbol_name,
unsigned char type,
unsigned char sect,
short desc,
unsigned long value,
struct object_file *definition_object)
{
    long i;
    struct merged_symbol *merged_symbol;

	/* look up the symbol to see if it is present */
	merged_symbol = *(lookup_symbol(symbol_name));
	/* if it is not present just return */
	if(merged_symbol == NULL)
	    return;
	/*
	 * The symbol is present and must be undefined unless it is defined
	 * in the base file of an incremental link.
	 */
	if(merged_symbol->nlist.n_type != (N_EXT | N_UNDF) ||
	   merged_symbol->nlist.n_value != 0){
	    if(merged_symbol->definition_object != base_obj){
		error("loaded objects attempt to redefine link editor "
		      "defined symbol %s", symbol_name);
		trace_merged_symbol(merged_symbol);
	    }
	    return;
	}

	/*
	 * Now that the checks above have been done if link editor defined
	 * symbols are not to be defined just return.
	 */
	if(define_comldsyms == FALSE)
	    return;

	/* define this symbol */
	merged_symbol->nlist.n_type = type;
	merged_symbol->nlist.n_sect = sect;
	merged_symbol->nlist.n_desc = desc;
	merged_symbol->nlist.n_value = value;
	merged_symbol->definition_object = definition_object;

	/*
	 * Do the trace of this symbol if specified now that it has
	 * been defined.
	 */
	if(ntrace_syms != 0){
	    for(i = 0; i < ntrace_syms; i++){
		if(strcmp(trace_syms[i], symbol_name) == 0){
		    trace_merged_symbol(merged_symbol);
		    break;
		}
	    }
	}
}
#endif !defined(RLD)

/*
 * reduce_indr_symbols() reduces indirect symbol chains to have all the indirect
 * symbols point at their leaf symbol.  Also catch loops of indirect symbols.
 */
void
reduce_indr_symbols(void)
{
    long i, j, k, indr_depth;
    struct merged_symbol_list **p, *merged_symbol_list;
    struct merged_symbol *merged_symbol, **indr_symbols, *indr_symbol;

	indr_symbols = allocate(nindr_symbols * sizeof(struct merged_symbol *));
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    for(i = 0; i < merged_symbol_list->used; i++){
		merged_symbol = &(merged_symbol_list->merged_symbols[i]);

		/*
		 * Reduce indirect symbol chains to have all the indirect
		 * symbols point at their leaf symbol.  Also catch loops of
		 * indirect symbols.  If an indirect symbol was previously
		 * in a loop it's n_value is set to zero so not to print the
		 * loop more than once.
		 */
		if(merged_symbol->nlist.n_type == (N_EXT | N_INDR) &&
		   merged_symbol->nlist.n_value != 0){
		    indr_symbols[0] = merged_symbol;
		    indr_depth = 1;
		    indr_symbol = (struct merged_symbol *)
						(merged_symbol->nlist.n_value);
		    while(indr_symbol->nlist.n_type == (N_EXT | N_INDR) &&
		          indr_symbol->nlist.n_value != 0){
			for(j = 0; j < indr_depth; j++){
			    if(indr_symbols[j] == indr_symbol)
				break;
			}
			if(j == indr_depth){
			    indr_symbols[indr_depth++] = indr_symbol;
			    indr_symbol = (struct merged_symbol *)
						(indr_symbol->nlist.n_value);
			}
			else{
			    error("indirect symbol loop:");
			    for(k = j; k < indr_depth; k++){
				trace_merged_symbol(indr_symbols[k]);
				indr_symbols[k]->nlist.n_value = 0;
			    }
			    indr_symbol->nlist.n_value = 0;
			}
		    }
		    if(indr_symbol->nlist.n_type != (N_EXT | N_INDR)){
			for(j = 0; j < indr_depth; j++){
			    indr_symbols[j]->nlist.n_value = 
						    (unsigned long)indr_symbol;
			}
		    }
		}
	    }
	}
	free(indr_symbols);
}

/*
 * layout_merged_symbols() sets the values and section numbers of the merged
 * symbols.  Also checks for undefined symbol and prints their name if the
 * symbol in not allowed to be undefined.
 */
void
layout_merged_symbols(void)
{
    long i, j;
    struct merged_symbol_list **p, *merged_symbol_list;
    struct merged_symbol *merged_symbol;
    enum bool printed_undef, allowed_undef, noundefs;

	printed_undef = FALSE;
	noundefs = TRUE;
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    for(i = 0; i < merged_symbol_list->used; i++){
		merged_symbol = &(merged_symbol_list->merged_symbols[i]);

		/*
		 * If the output file is not relocatable check to see if this
		 * symbol is undefined.  If it is and it is not on the allowed
		 * undefined list print it's name.
		 */
		if(merged_symbol->nlist.n_type == N_EXT | N_UNDF &&
		   merged_symbol->nlist.n_value == 0){
		    allowed_undef = FALSE;
		    if(nundef_syms != 0){
			for(j = 0; j < nundef_syms; j++){
			    if(strcmp(undef_syms[j],
				  merged_symbol->nlist.n_un.n_name) == 0){
				allowed_undef = TRUE;
				break;
			    }
			}
		    }
		    if(allowed_undef == FALSE)
			noundefs = FALSE;
		    if(save_reloc == FALSE){
			if(allowed_undef == FALSE){
			    if(printed_undef == FALSE){
				error("Undefined symbols:");
				printed_undef = TRUE;
			    }
			    print("%s\n", merged_symbol->nlist.n_un.n_name);
			}
		    }
		}
		/* relocate the symbol */
		relocate_symbol(&(merged_symbol->nlist),
				merged_symbol->definition_object);
	    }
	}
	/*
	 * The MH_NOUNDEFS flag is set only if there are no undefined symbols
	 * or commons left undefined.  This is only set if we think the file is
	 * executable as the execute bits are based on this.
	 */
	if(noundefs == TRUE &&
	   (define_comldsyms == TRUE || commons_exist == FALSE))
	    output_mach_header.flags |= MH_NOUNDEFS;
}

/*
 * output_local_symbols() copys the local symbols and their strings from the
 * current object file into the output file's memory buffer.  The symbols also
 * get relocated.
 */
void
output_local_symbols(void)
{
    long i, flush_symbol_offset, start_nsyms,
	    flush_string_offset, start_string_size;
    struct nlist *object_symbols, *nlist;
    char *object_strings, *string;

	/* If local symbols are to appear in the output file just return */
	if(strip_level == STRIP_ALL || strip_level == STRIP_NONGLOBALS)
	    return;

	/* If this object file has no symbols then just return */
	if(cur_obj->symtab == NULL)
	    return;

	/* If this is the base file and base file symbols are stripped return */
	if(cur_obj == base_obj && strip_base_symbols == TRUE)
	    return;

#ifdef RLD
	/* If this object is not from the current set then just return */
	if(cur_obj->set_num != cur_set)
	    return;
#endif RLD

	/* setup pointers to the symbol table and string table */
	object_symbols = (struct nlist *)(cur_obj->obj_addr +
					  cur_obj->symtab->symoff);
	object_strings = (char *)(cur_obj->obj_addr + cur_obj->symtab->stroff);

	flush_symbol_offset =
			output_symtab_info.symtab_command.symoff +
			output_symtab_info.output_nsyms * sizeof(struct nlist);
	flush_string_offset = output_symtab_info.symtab_command.stroff +
			      output_symtab_info.output_strsize;
	start_nsyms = output_symtab_info.output_nsyms;
	start_string_size = output_symtab_info.output_strsize;

	/* If we are creating section object symbols, create one if needed */
	if(sect_object_symbols.ms != NULL){
	    /*
	     * See if this object file has the section that the section object
	     * symbols are being created for.
	     */
	    for(i = 0; i < cur_obj->nsection_maps; i++){
		if(sect_object_symbols.ms == 
		   cur_obj->section_maps[i].output_section){
		    /* make the nlist entry in the output file */
		    nlist = (struct nlist *)(output_addr +
			    output_symtab_info.symtab_command.symoff +
			    output_symtab_info.output_nsyms *
							  sizeof(struct nlist));
		    nlist->n_value = 
			    cur_obj->section_maps[i].output_section->s.addr +
			    cur_obj->section_maps[i].offset;
		    nlist->n_sect =
			cur_obj->section_maps[i].output_section->output_sectnum;
		    nlist->n_type = N_SECT;
		    nlist->n_desc = 0;
		    output_symtab_info.output_nsyms++;

		    nlist->n_un.n_strx = output_symtab_info.output_strsize;
		    string = output_addr +
			     output_symtab_info.symtab_command.stroff +
			     output_symtab_info.output_strsize;
		    if(cur_obj->ar_hdr == NULL){
			strcpy(string, cur_obj->file_name);
			output_symtab_info.output_strsize +=
						 strlen(cur_obj->file_name) + 1;
		    }
		    else{
			strcpy(string, obj_member_name(cur_obj));
			output_symtab_info.output_strsize +=
				       strlen(obj_member_name(cur_obj)) + 1;
		    }
		    break;
		}
	    }
	}

	for(i = 0; i < cur_obj->symtab->nsyms; i++){
	    /*
	     * If this is a local symbol and it is to be in the output file then
	     * copy it and it's string into the output file and relocate the
	     * symbol.
	     */
	    if((object_symbols[i].n_type & N_EXT) == 0 && 
	       (strip_level == STRIP_NONE ||
	        is_output_local_symbol(object_symbols[i].n_type,
		    object_symbols[i].n_un.n_strx == 0 ? "" :
		    object_strings + object_symbols[i].n_un.n_strx))){

		/* copy the nlist to the output file */
	        nlist = (struct nlist *)(output_addr +
			output_symtab_info.symtab_command.symoff +
			output_symtab_info.output_nsyms * sizeof(struct nlist));
		*nlist = object_symbols[i];
		relocate_symbol(nlist, cur_obj);
#ifdef RLD
		/*
		 * Now change the section number of this symbol to the section
		 * number it will have in the output file.  For RLD all this
		 * has to be done on for only the symbol in an output file and
		 * not in the merged symbol table.  relocate_symbol() does not
		 * modify n_sect for RLD.
		 */
		if(nlist->n_sect != NO_SECT)
		    nlist->n_sect = cur_obj->section_maps[nlist->n_sect - 1].
				    output_section->output_sectnum;
#endif RLD
		output_symtab_info.output_nsyms++;

		if(strip_level == STRIP_NONE){
			nlist->n_un.n_strx += STRING_SIZE_OFFSET +
					      cur_obj->local_string_offset;
		}
		else{
		    /* copy the string to the output file (if it has one) */
		    if(object_symbols[i].n_un.n_strx != 0){
			nlist->n_un.n_strx = output_symtab_info.output_strsize;
			string = output_addr +
				 output_symtab_info.symtab_command.stroff +
				 output_symtab_info.output_strsize;
			strcpy(string,
			       object_strings + object_symbols[i].n_un.n_strx);
			output_symtab_info.output_strsize +=
				      strlen(object_strings +
					     object_symbols[i].n_un.n_strx) + 1;
		    }
		}
	    }
	}
	if(strip_level == STRIP_NONE){
	    memcpy(output_addr +
		   output_symtab_info.symtab_command.stroff +
		   output_symtab_info.output_strsize,
		   object_strings,
		   cur_obj->symtab->strsize);
	    output_symtab_info.output_strsize += cur_obj->symtab->strsize;
	}
#ifndef RLD
	output_flush(flush_symbol_offset, (output_symtab_info.output_nsyms -
					  start_nsyms) * sizeof(struct nlist));
	output_flush(flush_string_offset, output_symtab_info.output_strsize -
					  start_string_size);
#endif !defined(RLD)
}

/*
 * output_merged_symbols() readies the merged symbols for the output file (sets
 * string indexes and handles indirect symbols) and copies the merged symbols
 * and their strings to the output file.
 */
void
output_merged_symbols(void)
{
    long i, index, flush_symbol_offset, start_nsyms,
		   flush_string_offset, start_string_size;
    struct merged_symbol_list **p, *merged_symbol_list;
    struct merged_symbol *merged_symbol, *indr_symbol;
    struct string_block **q, *string_block;
    struct nlist *nlist;

	if(strip_level == STRIP_ALL)
	    return;

	/*
	 * Set the relitive indexes for each merged string block.
	 */
	index = 0;
	for(q = &(merged_string_blocks); *q; q = &(string_block->next)){
	    string_block = *q;
	    if(strip_base_symbols == TRUE && string_block->base_strings == TRUE)
		continue;
#ifdef RLD
	    if(string_block->set_num != cur_set)
		continue;
#endif RLD
	    string_block->index = index,
	    index += string_block->used;
	}

	/*
	 * Indirect symbols are readied for output.  For indirect symbols that
	 * the symbol they are refering to is defined (not undefined or common)
	 * the the type, value, etc. of the refered symbol is propagated to
	 * indirect symbol.  If the indirect symbol is not defined then the
	 * value field is set to the index of the string the symbol is refering
	 * to.
	 */
	string_block = merged_string_blocks;
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    for(i = 0; i < merged_symbol_list->used; i++){
		merged_symbol = &(merged_symbol_list->merged_symbols[i]);

		if(merged_symbol->nlist.n_type == (N_EXT | N_INDR)){
		    indr_symbol =
			(struct merged_symbol *)(merged_symbol->nlist.n_value);
		    /*
		     * Check to see if this symbol is defined (not undefined or 
		     * common)
		     */
		    if(indr_symbol->nlist.n_type != (N_EXT | N_UNDF)){
			merged_symbol->nlist.n_type = indr_symbol->nlist.n_type;
			merged_symbol->nlist.n_sect = indr_symbol->nlist.n_sect;
			merged_symbol->nlist.n_desc = indr_symbol->nlist.n_desc;
			merged_symbol->nlist.n_value =
						     indr_symbol->nlist.n_value;
		    }
		    else{
			if(indr_symbol->nlist.n_un.n_name <
							string_block->strings ||
			   indr_symbol->nlist.n_un.n_name >=
				     string_block->strings + string_block->used)
			    string_block = get_string_block(
						indr_symbol->nlist.n_un.n_name);
			merged_symbol->nlist.n_value =
					     output_symtab_info.output_strsize +
					     string_block->index +
					     (indr_symbol->nlist.n_un.n_name -
					      string_block->strings);
		    }
		}
	    }
	}

	flush_symbol_offset =
			output_symtab_info.symtab_command.symoff +
			output_symtab_info.output_nsyms * sizeof(struct nlist);
	flush_string_offset = output_symtab_info.symtab_command.stroff +
			      output_symtab_info.output_strsize;
	start_nsyms = output_symtab_info.output_nsyms;
	start_string_size = output_symtab_info.output_strsize;
	/*
	 * Copy the merged symbols into the memory buffer for the output file
	 * and assign string indexes to all of the symbols
	 */
	nlist = (struct nlist *)(output_addr +
		output_symtab_info.symtab_command.symoff +
		output_symtab_info.output_nsyms * sizeof(struct nlist));
	string_block = merged_string_blocks;
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    for(i = 0; i < merged_symbol_list->used; i++){
		merged_symbol = &(merged_symbol_list->merged_symbols[i]);
		if(strip_base_symbols == TRUE && 
		   merged_symbol->definition_object == base_obj)
		    continue;
#ifdef RLD
		if(merged_symbol->definition_object->set_num != cur_set)
		    continue;
#endif RLD
		if(merged_symbol->nlist.n_un.n_name < string_block->strings ||
		   merged_symbol->nlist.n_un.n_name >=
			     string_block->strings + string_block->used)
		    string_block = get_string_block(
					      merged_symbol->nlist.n_un.n_name);
		*nlist = merged_symbol->nlist;
		nlist->n_un.n_strx = output_symtab_info.output_strsize +
				     string_block->index +
				     (merged_symbol->nlist.n_un.n_name -
				     string_block->strings);
#ifdef RLD
		/*
		 * Now change the section number of this symbol to the section
		 * number it will have in the output file.  For RLD all this
		 * has to be done on for only the symbol in an output file and
		 * not in the merged symbol table.  relocate_symbol() does not
		 * modify n_sect for RLD.
		 */
		if(nlist->n_sect != NO_SECT)
		    nlist->n_sect = merged_symbol->definition_object->
				    section_maps[nlist->n_sect - 1].
				    output_section->output_sectnum;
#endif RLD
		nlist++;
	    }
	}
	output_symtab_info.output_nsyms += nmerged_symbols;

	/*
	 * Copy the merged strings into the memory buffer for the output file.
	 */
	for(q = &(merged_string_blocks); *q; q = &(string_block->next)){
	    string_block = *q;
	    if(strip_base_symbols == TRUE && string_block->base_strings == TRUE)
		continue;
#ifdef RLD
	    if(string_block->set_num != cur_set)
		continue;
#endif RLD
	    memcpy(output_addr + output_symtab_info.symtab_command.stroff +
				 output_symtab_info.output_strsize,
		   string_block->strings,
		   string_block->used);
	    output_symtab_info.output_strsize += string_block->used;
	}

#ifndef RLD
	output_flush(flush_symbol_offset, (output_symtab_info.output_nsyms -
					  start_nsyms) * sizeof(struct nlist));
	output_flush(flush_string_offset, output_symtab_info.output_strsize -
					  start_string_size);
#endif !defined(RLD)
}

/*
 * get_string_block() returns a pointer to the string block the specified
 * symbol name is in.
 */
static
struct string_block *
get_string_block(
char *symbol_name)
{
    struct string_block **p, *string_block;

	for(p = &(merged_string_blocks); *p; p = &(string_block->next)){
	    string_block = *p;
	    if(symbol_name >= string_block->strings &&
	       symbol_name < string_block->strings + string_block->used)
		return(string_block);
	}
	fatal("internal error: get_string_block() called with symbol_name (%s) "
	      "not in the string blocks");
	return(NULL); /* to prevent warning from compiler */
}

#ifndef RLD
/*
 * merged_symbol_output_index() returns the index in the output file's symbol
 * table for the merged_symbol pointer passed to it.  Merged symbols are in the 
 * output file just after the local symbols.  This is only used to update
 * relocation entries (which are not saved in most uses of the link editor) so
 * the speed of this is not important enough to add a symbol index field to the
 * merged symbol structure to make it faster (this would just use more memory
 * and could make the general case slower).
 */
long
merged_symbol_output_index(
struct merged_symbol *merged_symbol)
{
    struct merged_symbol_list **p, *merged_symbol_list;
    long index;

	index = nlocal_symbols;
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    if(merged_symbol >= &(merged_symbol_list->merged_symbols[0]) &&
	       merged_symbol < &(merged_symbol_list->merged_symbols[NSYMBOLS]))
		return(index + (merged_symbol -
				&(merged_symbol_list->merged_symbols[0])));
	    index += merged_symbol_list->used;
	}
	fatal("internal error, merged_symbol_output_index() called with bad "
	      "merged symbol pointer");
	return(0); /* to prevent warning from compiler */
}
#endif !defined(RLD)

#ifdef RLD
/*
 * free_multiple_defs() frees the multiple_defs array and resets the count to
 * zero if it exist.
 */
void
free_multiple_defs(void)
{
	if(nmultiple_defs != 0){
	    free(multiple_defs);
	    multiple_defs = NULL;
	    nmultiple_defs = 0;
	}
}

/*
 * remove_merged_symbols() removes the merged symbols that are defined in the
 * current object file set and their strings.  This take advantage of the fact
 * that symbols from the current set of symbols were all merged after the
 * previous set.
 */
void
remove_merged_symbols(void)
{
    long i;
    struct merged_symbol_list *merged_symbol_list, *prev_merged_symbol_list,
			      *next_merged_symbol_list;
    struct merged_symbol **hash_table, *merged_symbol;

    struct string_block *string_block, *prev_string_block, *next_string_block;

	/*
	 * The compiler "warning: `prev_merged_symbol_list' and
	 * `prev_string_block' may be used uninitialized in this function"
	 * can safely be ignored.
	 */
	prev_merged_symbol_list = NULL;
	prev_string_block = NULL;

	/*
	 * Clear all the merged symbol table entries for symbols that come
	 * from the current set of object files.  This is done by walking the
	 * hashtable an not the symbol list so the hashtable can be cleared
	 * out also.
	 */
	for(merged_symbol_list = merged_symbol_lists;
	    merged_symbol_list != NULL;
	    merged_symbol_list = merged_symbol_list->next){
	    if(merged_symbol_list->hash_table != NULL){
		hash_table = merged_symbol_list->hash_table;
		for(i = 0; i < SYMBOL_LIST_HASH_SIZE; i++){
		    if(hash_table[i] != NULL){
			merged_symbol = hash_table[i];
			if(merged_symbol->definition_object->set_num ==cur_set){
			    memset(merged_symbol, '\0',
				   sizeof(struct merged_symbol));
			    hash_table[i] = NULL;
			    merged_symbol_list->used--;
			}
		    }
		}
	    }
	}
	/*
	 * Find the first symbol list that now has 0 entries used.
	 */
	for(merged_symbol_list = merged_symbol_lists;
	    merged_symbol_list != NULL;
	    merged_symbol_list = merged_symbol_list->next){
	    if(merged_symbol_list->used == 0)
		break;
	    prev_merged_symbol_list = merged_symbol_list;
	}
	/*
	 * If there are any symbol lists with 0 entries used free their hash
	 * tables and the list.
	 */
	if(merged_symbol_list != NULL && merged_symbol_list->used == 0){
	    /*
	     * First set the pointer to this list in the previous list to
	     * NULL.
	     */
	    if(merged_symbol_list == merged_symbol_lists)
		merged_symbol_lists = NULL;
	    else
		prev_merged_symbol_list->next = NULL;
	    /*
	     * Now free the hash table for this list the list itself and do the
	     * same for all remaining lists.
	     */
	    do {
		free(merged_symbol_list->hash_table);
		next_merged_symbol_list = merged_symbol_list->next;
		free(merged_symbol_list);
		merged_symbol_list = next_merged_symbol_list;
	    }while(merged_symbol_list != NULL);
	}

	/*
	 * Find the first string block for the current set of object files.
	 */
	for(string_block = merged_string_blocks;
	    string_block != NULL;
	    string_block = string_block->next){
	    if(string_block->set_num == cur_set)
		break;
	    prev_string_block = string_block;
	}
	/*
	 * If there are any string blocks for the current set of object files
	 * free their strings and the blocks.
	 */
	if(string_block != NULL && string_block->set_num == cur_set){
	    /*
	     * First set the pointer to this block in the previous block to
	     * NULL.
	     */
	    if(string_block == merged_string_blocks)
		merged_string_blocks = NULL;
	    else
		prev_string_block->next = NULL;
	    /*
	     * Now free the stings for this block the block itself and do the
	     * same for all remaining blocks.
	     */
	    do {
		free(string_block->strings);
		next_string_block = string_block->next;
		free(string_block);
		string_block = next_string_block;
	    }while(string_block != NULL);
	}
}
#endif RLD

#ifdef DEBUG
/*
 * print_symbol_list() prints the merged symbol table.  Used for debugging.
 */
void
print_symbol_list(
char *string,
enum bool input_based)
{
    struct merged_symbol_list **p, *merged_symbol_list;
    long i;
    struct nlist *nlist;
    struct section *s;
    struct section_map *maps;
    struct merged_symbol **hash_table;

	print("Merged symbol list (%s)\n", string);
	for(p = &merged_symbol_lists; *p; p = &(merged_symbol_list->next)){
	    merged_symbol_list = *p;
	    print("merged_symbols\n");
	    for(i = 0; i < merged_symbol_list->used; i++){
		print("%-4d[0x%x]\n", i,
		       merged_symbol_list->merged_symbols[i]);
		nlist = &((merged_symbol_list->merged_symbols + i)->nlist);
		print("    n_name %s\n", nlist->n_un.n_name);
		print("    n_type ");
		switch(nlist->n_type){
		case N_UNDF | N_EXT:
		    if(nlist->n_value == 0)
			print("N_UNDF\n");
		    else
			print("common (size %d)\n", nlist->n_value);
		    break;
		case N_ABS | N_EXT:
		    print("N_ABS\n");
		    break;
		case N_SECT | N_EXT:
		    print("N_SECT\n");
		    break;
		case N_INDR | N_EXT:
		    print("N_INDR for %s\n", ((struct merged_symbol *)
				(nlist->n_value))->nlist.n_un.n_name);
		    break;
		default:
		    print("unknown 0x%x\n", nlist->n_type);
		    break;
		}
		print("    n_sect %d ", nlist->n_sect);
		maps = (merged_symbol_list->merged_symbols + i)->
			definition_object->section_maps;
		if(nlist->n_sect == NO_SECT)
		    print("NO_SECT\n");
		else{
		    if(input_based == TRUE)
			print("(%0.16s,%0.16s)\n",
			       maps[nlist->n_sect - 1].s->segname,
			       maps[nlist->n_sect - 1].s->sectname);
		    else{
			s = get_output_section(nlist->n_sect);
			if(s != NULL)
			    print("(%0.16s,%0.16s)\n",s->segname, s->sectname);
			else
			    print("(bad section #%d)\n", nlist->n_sect);
		    }
		}
		print("    n_desc 0x%04x\n", nlist->n_desc);
		print("    n_value 0x%08x\n", nlist->n_value);
#ifdef RLD
		print("    definition_object ");
		print_obj_name(
		       merged_symbol_list->merged_symbols[i].definition_object);
		print("\n");
		print("    set_num %d\n", merged_symbol_list->merged_symbols[i].
		      definition_object->set_num);
#endif RLD
	    }
	    print("hash_table 0x%x\n", merged_symbol_list->hash_table);
	    if(merged_symbol_list->hash_table != NULL){
		hash_table = merged_symbol_list->hash_table;
		for(i = 0; i < SYMBOL_LIST_HASH_SIZE; i++){
		    print("    %-4d [0x%x] (%s)\n", i, hash_table + i,
			   *(hash_table + i) == NULL ? "NULL" :
			   (*(hash_table + i))->nlist.n_un.n_name);
		}
	    }
	}
}

/*
 * get_output_section() returns a pointer to the output section structure for
 * the section number passed to it.  It returns NULL for section numbers that
 * are not in the output file.
 */
struct section *
get_output_section(
long sect)
{
    struct merged_segment **p, *msg;
    struct merged_section **content, **zerofill, *ms;

	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->output_sectnum == sect)
		    return(&(ms->s));
		content = &(ms->next);
	    }
	    zerofill = &(msg->zerofill_sections);
	    while(*zerofill){
		ms = *zerofill;
		if(ms->output_sectnum == sect)
		    return(&(ms->s));
		zerofill = &(ms->next);
	    }
	    p = &(msg->next);
	}
	return(NULL);
}

#ifndef RLD
/*
 * print_undefined_list() prints the undefined symbol list.  Used for debugging.
 */
void
print_undefined_list(void)
{
    struct undefined_list *undefined;

	print("Undefined list\n");
	for(undefined = undefined_list.next;
	    undefined != &undefined_list;
	    undefined = undefined->next){
	    print("    %s", undefined->merged_symbol->nlist.n_un.n_name);
	    if(undefined->merged_symbol->nlist.n_type == (N_UNDF | N_EXT) ||
	       undefined->merged_symbol->nlist.n_value != 0)
		print("\n");
	    else
		print(" (no longer undefined)\n");
	}
}
#endif !defined(RLD)
#endif DEBUG

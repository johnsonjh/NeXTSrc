#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines to manage the merging of the sections that
 * appear in the headers of the input files.  It builds a merged section table
 * (which is a linked list of merged_segments with merged_sections linked to
 * them).  The merged section list becomes the output files's section list.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/loader.h>
#include <mach.h>
#include <nlist.h>
#include <reloc.h>
#include <ar.h>

#include "ld.h"
#include "specs.h"
#include "objects.h"
#include "symbols.h"
#include "sections.h"
#include "cstring_literals.h"
#include "4byte_literals.h"
#include "8byte_literals.h"
#include "literal_pointers.h"
#include "pass2.h"
#include "generic_reloc.h"
#include "sets.h"
#include "hash_string.h"

/* the pointer to the head of the output file's section list */
struct merged_segment *merged_segments = NULL;
#ifdef RLD
/*
 * The pointer to the head of the output file's section list before they are
 * all placed in one segment for the MH_OBJECT format in layout().  This is
 * used in reset_merged_sections() to put the list back with it's original
 * segments.
 */
struct merged_segment *original_merged_segments = NULL;
#endif RLD

/*
 * The total number relocation entries, used only in layout() to help
 * calculate the size of the link edit segment.
 */
long nreloc = 0;

/* table for S_* flags for error messages */
static const char * const section_flags[] = {
	"(none)",
	"S_ZEROFILL",
	"S_CSTRING_LITERALS",
	"S_4BYTE_LITERALS",
	"S_8BYTE_LITERALS",
	"S_LITERAL_POINTERS"
};

#ifndef RLD
/*
 * These are the arrays used for finding archive,object,symbol name
 * triples and object,symbol name pairs when processing -sectorder options.
 * The array of symbol names is the load_order structure pointed to by
 * the cur_load_orders field of the object_file structure.
 */
struct archive_name {
    char *archive_name;	/* name of the archive file */
    struct object_name
	*object_names;	/* names of the archive members */
    long nobject_names;	/* number of archive members */
};
static struct archive_name *archive_names = NULL;
static long narchive_names = 0;

struct object_name {
    char *object_name;	/* name of object file */
    long index_length;	/* if this is not in an archive its an index into the */
			/*  object_name to the base name of the object file */
			/*  name else it is the length of the object name */
			/*  which is an archive member name that may have */
			/*  been truncated. */
    struct object_file	/* pointer to the object file */
	*object_file;
};
static struct object_name *object_names = NULL;
static long nobject_names = 0;

struct load_symbol {
    char *symbol_name;	/* the symbol name this is hashed on */
    char *object_name;	/* the loaded object that contains this symbol */
    char *archive_name;	/* the loaded archive that contains this object */
			/*  or NULL if not in an archive */
    long index_length;	/* if archive_name is NULL the this is index into the */
			/*  object_name to the base name of the object file */
			/*  name else it is the length of the object name */
			/*  which is an archive member name that may have */
			/*  been truncated. */
    struct load_order
	*load_order;	/* the load order for the above triple names */
    struct load_symbol
	*other_names;	/* other load symbols for the same symbol_name */
    struct load_symbol
	*next;		/* next hash table pointer */
};
#define LOAD_SYMBOL_HASHTABLE_SIZE 10000
static struct load_symbol **load_symbol_hashtable = NULL;
static struct load_symbol *load_symbols = NULL;
static long load_symbols_size = 0;
static long load_symbols_used = 0;
static long ambigious_specifications = 0;

static void layout_ordered_section(
    struct merged_section *ms);
static void create_name_arrays(
    void);
static struct archive_name *create_archive_name(
    char *archive_name);
static void create_object_name(
    struct object_name **object_names,
    long *nobject_names,
    char *object_name,
    long index_length,
    char *archive_name);
static void free_name_arrays(
    void);
static void create_load_symbol_hash_table(
    long nsection_symbols);
static void free_load_symbol_hash_table(
    void);
static void create_load_symbol_hash_table_for_object(
    char *archive_name,
    char *object_name,
    long index_length,
    struct load_order *load_orders,
    long nload_orders);
static struct load_order *lookup_load_order(
    char *archive_name,
    char *object_name,
    char *symbol_name,
    struct merged_section *ms,
    long line_number);
static char * trim(
    char *name);
static struct section_map *lookup_section_map(
    char *archive_name,
    char *object_name);
static int qsort_load_order_values(
    const struct load_order *load_order1,
    const struct load_order *load_order2);
static int qsort_load_order_names(
    const struct load_order *load_order1,
    const struct load_order *load_order2);
static int bsearch_load_order_names(
    char *symbol_name,
    const struct load_order *load_order);
static int qsort_archive_names(
    const struct archive_name *archive_name1,
    const struct archive_name *archive_name2);
static int bsearch_archive_names(
    const char *name,
    const struct archive_name *archive_name);
static int qsort_object_names(
    const struct object_name *object_name1,
    const struct object_name *object_name2);
static int bsearch_object_names(
    const char *name,
    const struct object_name *object_name);
static int qsort_fine_reloc_input_offset(
    const struct fine_reloc *fine_reloc1,
    const struct fine_reloc *fine_reloc2);
static int qsort_order_load_map_orders(
    const struct order_load_map *order_load_map1,
    const struct order_load_map *order_load_map2);
static void create_order_load_maps(
    struct merged_section *ms,
    long norders);
static void scatter_copy(
    struct section_map *map,
    char *contents);
#endif !defined(RLD)
#ifdef DEBUG
static void print_load_order(
    struct load_order *load_order,
    long nload_order,
    struct merged_section *ms,
    struct object_file *object_file,
    char *string);
static void print_load_symbol_hash_table(
    void);
#endif DEBUG

/*
 * merge_sections() merges the sections of the current object file (cur_obj)
 * into the merged section list that will be in the output file.  For each
 * section in the current object file it records the offset that section will
 * start in the output file.  It also accumulates the size of each merged
 * section, the number of relocation entries in it and the maximum alignment.
 */
void
merge_sections(void)
{
    long i;
    struct section *s;
    struct merged_section *ms;

	for(i = 0; i < cur_obj->nsection_maps; i++){
	    s = cur_obj->section_maps[i].s;
	    ms = create_merged_section(s);
	    if(errors)
		return;
	    cur_obj->section_maps[i].output_section = ms;
	    switch(ms->s.flags){
	    case 0: /* regular content sections */
	    case S_ZEROFILL:
		/*
		 * For the base file of an incremental link all that is needed
		 * is the section (and it's alignment) so the symbols can refer
		 * to them.  Their contents do not appear in the output file.
		 */
		if(cur_obj != base_obj){
		    cur_obj->section_maps[i].flush_offset = ms->s.size;
		    ms->s.size = round(ms->s.size, 1 << s->align);
		    cur_obj->section_maps[i].offset = ms->s.size;
		    ms->s.size   += s->size;
		    ms->s.nreloc += s->nreloc;
		    nreloc += s->nreloc;
		}
		if(s->align > ms->s.align)
		    ms->s.align = s->align;
		break;
	    case S_CSTRING_LITERALS:
	    case S_4BYTE_LITERALS:
	    case S_8BYTE_LITERALS:
	    case S_LITERAL_POINTERS:
		if(s->align > ms->s.align)
		    ms->s.align = s->align;
		break;
	    }
	}
}

/*
 * create_merged_section() looks for the section passed to it in the merged
 * section list.  If the section is found then it is check to see the flags
 * of the section matches and if so returns a pointer to the merged section
 * structure for it.  If the flags don't match it is an error.  If no merged
 * section structure is found then one is created and added to the end of the
 * list and a pointer to it is returned.
 */
struct merged_section *
create_merged_section(
struct section *s)
{
    struct merged_segment **p, *msg;
    struct merged_section **q, **r, *ms;

	p = &merged_segments;
	while(*p){
	    msg = *p;
	    /* see if this is section is in this segment */
	    if(strncmp(msg->sg.segname, s->segname, sizeof(s->segname)) == 0){
		/*
		 * Depending on the flags of the section depends on which list
		 * it might be found in.  In either case it must not be found in
		 * the other list.
		 */
		if(s->flags == S_ZEROFILL){
		    q = &(msg->zerofill_sections);
		    r = &(msg->content_sections);
		}
		else{
		    q = &(msg->content_sections);
		    r = &(msg->zerofill_sections);
		}
		/* check to see if it is in the list it might be found in */
		while(*q){
		    ms = *q;
		    if(strncmp(ms->s.sectname, s->sectname,
			       sizeof(s->sectname)) == 0){
			if(ms->s.flags != s->flags){
			    error_with_cur_obj("section's (%0.16s,%0.16s) "
					       "flags %s does not match "
					       "previous objects flags %s",
					       s->segname, s->sectname,
					       section_flags[s->flags],
					       section_flags[ms->s.flags]);
			    return(NULL);
			}
			return(ms);
		    }
		    q = &(ms->next);
		}
		/*
		 * It was not found in the list it might be in so check to make
		 * sure it is not in the other list where it shouldn't be. 
		 */
		while(*r){
		    ms = *r;
		    if(strncmp(ms->s.sectname, s->sectname,
			       sizeof(s->sectname)) == 0){
			error_with_cur_obj("section's (%0.16s,%0.16s) "
					   "flags %s does not match "
					   "previous objects flags %s",
					   s->segname, s->sectname,
					   section_flags[s->flags],
					   section_flags[ms->s.flags]);
			return(NULL);
		    }
		    r = &(ms->next);
		}
		/* add it to the list it should be in */
		msg->sg.nsects++;
		*q = allocate(sizeof(struct merged_section));
		ms = *q;
		memset(ms, '\0', sizeof(struct merged_section));
		strncpy(ms->s.sectname, s->sectname, sizeof(s->sectname));
		strncpy(ms->s.segname, s->segname, sizeof(s->segname));
		ms->s.flags = s->flags;
		if(ms->s.flags == S_CSTRING_LITERALS){
		    ms->literal_data = allocate(sizeof(struct cstring_data));
		    memset(ms->literal_data, '\0', sizeof(struct cstring_data));
		    ms->literal_merge = cstring_merge;
		    ms->literal_order = cstring_order;
		    ms->literal_output = cstring_output;
		    ms->literal_free = cstring_free;
		}
		if(ms->s.flags == S_4BYTE_LITERALS){
		    ms->literal_data = allocate(sizeof(struct literal4_data));
		    memset(ms->literal_data, '\0',sizeof(struct literal4_data));
		    ms->literal_merge = literal4_merge;
		    ms->literal_order = literal4_order;
		    ms->literal_output = literal4_output;
		    ms->literal_free = literal4_free;
		}
		if(ms->s.flags == S_8BYTE_LITERALS){
		    ms->literal_data = allocate(sizeof(struct literal8_data));
		    memset(ms->literal_data, '\0',sizeof(struct literal8_data));
		    ms->literal_merge = literal8_merge;
		    ms->literal_order = literal8_order;
		    ms->literal_output = literal8_output;
		    ms->literal_free = literal8_free;
		}
		if(ms->s.flags == S_LITERAL_POINTERS){
		    ms->literal_data =
				  allocate(sizeof(struct literal_pointer_data));
		    memset(ms->literal_data, '\0',
					   sizeof(struct literal_pointer_data));
		    ms->literal_merge = literal_pointer_merge;
		    ms->literal_order = literal_pointer_order;
		    ms->literal_output = literal_pointer_output;
 		    ms->literal_free = literal_pointer_free;
		}
#ifdef RLD
		ms->set_num = cur_set;
#endif RLD
		return(ms);
	    }
	    p = &(msg->next);
	}
	/*
	 * The segment this section is in wasn't found so add a merged segment
	 * for it and add a merged section to that segment for this section.
	 */
	*p = allocate(sizeof(struct merged_segment));
	msg = *p;
	memset(msg, '\0', sizeof(struct merged_segment));
	strncpy(msg->sg.segname, s->segname, sizeof(s->segname));
	msg->sg.nsects = 1;
	msg->filename = outputfile;
#ifdef RLD
	msg->set_num = cur_set;
#endif RLD
	if(s->flags == S_ZEROFILL)
	    q = &(msg->zerofill_sections);
	else
	    q = &(msg->content_sections);
	*q = allocate(sizeof(struct merged_section));
	ms = *q;
	memset(ms, '\0', sizeof(struct merged_section));
	strncpy(ms->s.sectname, s->sectname, sizeof(s->sectname));
	strncpy(ms->s.segname, s->segname, sizeof(s->segname));
	ms->s.flags = s->flags;
	if(ms->s.flags == S_CSTRING_LITERALS){
	    ms->literal_data = allocate(sizeof(struct cstring_data));
	    memset(ms->literal_data, '\0', sizeof(struct cstring_data));
	    ms->literal_merge = cstring_merge;
	    ms->literal_order = cstring_order;
	    ms->literal_output = cstring_output;
	    ms->literal_free = cstring_free;
	}
	if(ms->s.flags == S_4BYTE_LITERALS){
	    ms->literal_data = allocate(sizeof(struct literal4_data));
	    memset(ms->literal_data, '\0', sizeof(struct literal4_data));
	    ms->literal_merge = literal4_merge;
	    ms->literal_order = literal4_order;
	    ms->literal_output = literal4_output;
	    ms->literal_free = literal4_free;
	}
	if(ms->s.flags == S_8BYTE_LITERALS){
	    ms->literal_data = allocate(sizeof(struct literal8_data));
	    memset(ms->literal_data, '\0', sizeof(struct literal8_data));
	    ms->literal_merge = literal8_merge;
	    ms->literal_order = literal8_order;
	    ms->literal_output = literal8_output;
	    ms->literal_free = literal8_free;
	}
	if(ms->s.flags == S_LITERAL_POINTERS) {
	    ms->literal_data = allocate(sizeof(struct literal_pointer_data));
	    memset(ms->literal_data, '\0', sizeof(struct literal_pointer_data));
	    ms->literal_merge = literal_pointer_merge;
	    ms->literal_order = literal_pointer_order;
	    ms->literal_output = literal_pointer_output;
	    ms->literal_free = literal_pointer_free;
	}
#ifdef RLD
	ms->set_num = cur_set;
#endif RLD
	return(ms);
}

/*
 * lookup_merged_segment() looks up the specified segment name 
 * in the merged segment list and returns a pointer to the
 * merged segment if it exist.  It returns NULL if it doesn't exist.
 */
struct merged_segment *
lookup_merged_segment(
char *segname)
{
    struct merged_segment **p, *msg;

	p = &merged_segments;
	while(*p){
	    msg = *p;
	    if(strncmp(msg->sg.segname, segname, sizeof(msg->sg.segname)) == 0)
		return(msg);
	    p = &(msg->next);
	}
	return(NULL);
}

/*
 * lookup_merged_section() looks up the specified section name 
 * (segname,sectname) in the merged section list and returns a pointer to the
 * merged section if it exist.  It returns NULL if it doesn't exist.
 */
struct merged_section *
lookup_merged_section(
char *segname,
char *sectname)
{
    struct merged_segment **p, *msg;
    struct merged_section **q, *ms;

	p = &merged_segments;
	while(*p){
	    msg = *p;
	    if(strncmp(msg->sg.segname, segname, sizeof(msg->sg.segname)) == 0){
		q = &(msg->content_sections);
		while(*q){
		    ms = *q;
		    if(strncmp(ms->s.sectname, sectname,
			       sizeof(ms->s.sectname)) == 0){
			return(ms);
		    }
		    q = &(ms->next);
		}
		q = &(msg->zerofill_sections);
		while(*q){
		    ms = *q;
		    if(strncmp(ms->s.sectname, sectname,
			       sizeof(ms->s.sectname)) == 0){
			return(ms);
		    }
		    q = &(ms->next);
		}
		return(NULL);
	    }
	    p = &(msg->next);
	}
	return(NULL);
}

/*
 * merge_literal_sections() goes through all the object files to be loaded and
 * merges the literal sections from them.  This is called from layout() and has
 * to be done after all the alignment from all the sections headers have been
 * merged and the command line section alignment has been folded in.  This way
 * the individual literal items from all the objects can be aligned to the
 * output alignment.
 */
void
merge_literal_sections(void)
{
    long i, j;
    struct object_list *object_list, **p;
    struct merged_section *ms;
#ifndef RLD
    struct merged_segment **q, *msg;
    struct merged_section **content;

	/*
	 * If any literal (except literal pointer) section has an order file
	 * then process it for that section.
	 */
	q = &merged_segments;
	while(*q){
	    msg = *q;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->order_filename != NULL &&
		   (ms->s.flags == S_CSTRING_LITERALS ||
		    ms->s.flags == S_4BYTE_LITERALS ||
		    ms->s.flags == S_8BYTE_LITERALS)){
		    (*ms->literal_order)(ms->literal_data, ms);
		}
		content = &(ms->next);
	    }
	    q = &(msg->next);
	}
#endif !defined(RLD)

	/*
	 * Merged the literals for each object for each section that is a 
	 * literal (but not a literal pointer section).
	 */
	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    for(i = 0; i < object_list->used; i++){
		cur_obj = &(object_list->object_files[i]);
		if(cur_obj == base_obj)
		    continue;
#ifdef RLD
		if(cur_obj->set_num != cur_set)
		    continue;
#endif RLD
		for(j = 0; j < cur_obj->nsection_maps; j++){
		    ms = cur_obj->section_maps[j].output_section;
		    if(ms->s.flags == S_CSTRING_LITERALS ||
		       ms->s.flags == S_4BYTE_LITERALS ||
		       ms->s.flags == S_8BYTE_LITERALS)
			(*ms->literal_merge)(ms->literal_data, ms,
					     cur_obj->section_maps[j].s,
					     &(cur_obj->section_maps[j]));
		}
	    }
	}

#ifndef RLD
	/*
	 * Now that the the literals are all merged if any literal pointer
	 * section has an order file then process it for that section.
	 */
	q = &merged_segments;
	while(*q){
	    msg = *q;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->order_filename != NULL &&
		   ms->s.flags == S_LITERAL_POINTERS){
		    (*ms->literal_order)(ms->literal_data, ms);
		}
		content = &(ms->next);
	    }
	    q = &(msg->next);
	}
#endif !defined(RLD)
	/*
	 * Now that the the literals are all merged merge the literal pointers
	 * for each object for each section that is a a literal pointer section.
	 */
	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    for(i = 0; i < object_list->used; i++){
		cur_obj = &(object_list->object_files[i]);
		if(cur_obj == base_obj)
		    continue;
#ifdef RLD
		if(cur_obj->set_num != cur_set)
		    continue;
#endif RLD
		for(j = 0; j < cur_obj->nsection_maps; j++){
		    ms = cur_obj->section_maps[j].output_section;
		    if(ms->s.flags == S_LITERAL_POINTERS)
			(*ms->literal_merge)(ms->literal_data, ms,
					     cur_obj->section_maps[j].s,
					     &(cur_obj->section_maps[j]));
		}
	    }
	}
}

#ifndef RLD
/*
 * layout_ordered_sections() calles layout_ordered_section() for each section
 * that has an order file specified with -sectorder.
 */
void
layout_ordered_sections(void)
{
    enum bool ordered_sections;
    struct merged_segment **p, *msg;
    struct merged_section **content, **zerofill, *ms;
    struct object_file *last_object;

	/*
	 * Determine if their are any sections that have an order file and if
	 * not just return.  This saves creating the name arrays when there is
	 * no need to.
	 */
	ordered_sections = FALSE;
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->order_filename != NULL){
		    ordered_sections = TRUE;
		    break;
		}
		content = &(ms->next);
	    }
	    zerofill = &(msg->zerofill_sections);
	    while(*zerofill){
		ms = *zerofill;
		if(ms->order_filename != NULL){
		    ordered_sections = TRUE;
		    break;
		}
		zerofill = &(ms->next);
	    }
	    if(ordered_sections == TRUE)
		break;
	    p = &(msg->next);
	}
	if(ordered_sections == FALSE)
	    return;

	/*
	 * Add the object file the common symbols that the link editor allocated
	 * into the object file list.
	 */
	last_object = add_last_object_file(&link_edit_common_object);

	/*
	 * Build the arrays of archive names and object names which along
	 * with the load order maps will be use to search for archive,object,
	 * symbol name triples from the load order files specified by the user.
	 */
	create_name_arrays();
#ifdef DEBUG
	if(debug & 0x2000)
	    print_name_arrays();
#endif DEBUG

	/*
	 * For each merged section that has a load order file layout all objects
	 * that have this section in it.
	 */
	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		/* no load order for this section continue */
		if(ms->order_filename == NULL){
		    content = &(ms->next);
		    continue;
		}
		/*
		 * If a regular section (not a literal section) then layout
		 * the sections using symbol names.  Literal sections are
		 * handled by their specific literal merge functions.
		 */
		if(ms->s.flags == 0)
		    layout_ordered_section(ms);
		content = &(ms->next);
	    }
	    zerofill = &(msg->zerofill_sections);
	    while(*zerofill){
		ms = *zerofill;
		/* no load order for this section continue */
		if(ms->order_filename == NULL){
		    zerofill = &(ms->next);
		    continue;
		}
		layout_ordered_section(ms);
		zerofill = &(ms->next);
	    }
	    p = &(msg->next);
	}

	/*
	 * Free the space for the symbol table.
	 */
	free_load_symbol_hash_table();

	/*
	 * Free the space for the name arrays if there has been no load order
	 * map specified (this is because the map has pointers to the object
	 * names that were allocated in the name arrarys).
	 */
	if(load_map == FALSE)
	    free_name_arrays();

	/*
	 * Remove the object file the common symbols that the link editor
	 * allocated from the object file list.
	 */
	remove_last_object_file(last_object);
}

/*
 * layout_ordered_section() creates the fine reloc maps for the section in
 * each object from the load order file specified with -sectorder.
 */
static
void
layout_ordered_section(
struct merged_section *ms)
{
    long i, j, k, l;
    struct object_list *object_list, **q;

    long nsect, nload_orders, nsection_symbols;
    struct load_order *load_orders;
    enum bool start_section;

    struct nlist *object_symbols;
    char *object_strings;

    long n, order, output_offset, line_number, line_length;
    long unused_specifications, no_specifications;
    char *line, *archive_name, *object_name, *symbol_name;
    struct load_order *load_order;
    struct section_map *section_map;
    kern_return_t r;

    struct fine_reloc *fine_relocs;

	/*
	 * Reset the count of the number of symbol for this
	 * section (used as the number of load_symbol structs
	 * to allocate).
	 */
	nsection_symbols = 0;

	/*
	 * For each object file that has this section process it.
	 */
	for(q = &objects; *q; q = &(object_list->next)){
	    object_list = *q;
	    for(i = 0; i < object_list->used; i++){
		cur_obj = &(object_list->object_files[i]);
		if(cur_obj == base_obj)
		    continue;
		/*
		 * Reset the current section map which points to the
		 * load order map and count in this object that
		 * is being processed for this merged section.  This
		 * will be used in later loops to avoid going through
		 * the section maps again.
		 */
		cur_obj->cur_section_map = NULL;

		for(j = 0; j < cur_obj->nsection_maps; j++){
		    if(cur_obj->section_maps[j].output_section != ms)
			continue;
		    if(cur_obj->section_maps[j].s->size == 0)
			continue;
		    /*
		     * Count the number of symbols in this section in
		     * this object file.  For this object nsect is the
		     * section number for the merged section.  Also
		     * acount for one extra symbol if there is no symbol
		     * at the beginning of the section.
		     */
		    object_symbols = (struct nlist *)(cur_obj->obj_addr 
					     + cur_obj->symtab->symoff);
		    object_strings = (char *)(cur_obj->obj_addr +
					       cur_obj->symtab->stroff);
		    nsect = j + 1;
		    nload_orders = 0;
		    start_section = FALSE;
		    for(k = 0; k < cur_obj->symtab->nsyms; k++){
			if(object_symbols[k].n_sect == nsect &&
			   (object_symbols[k].n_type & N_STAB) == 0){
			    nload_orders++;
			    if(object_symbols[k].n_value == 
			       cur_obj->section_maps[j].s->addr)
				start_section = TRUE;
			}
		    }
		    if(start_section == FALSE)
			nload_orders++;

		    /*
		     * Allocate the load order map for this section in
		     * this object file and set the current section map
		     * in this object that will point to the load order
		     * map and count.
		     */
		    load_orders = allocate(sizeof(struct load_order) *
					   nload_orders);
		    memset(load_orders, '\0',
			   sizeof(struct load_order) * nload_orders);
		    cur_obj->section_maps[j].nload_orders= nload_orders;
		    cur_obj->section_maps[j].load_orders = load_orders;
		    cur_obj->cur_section_map =
					    &(cur_obj->section_maps[j]);

		    /*
		     * Fill in symbol names and values the load order
		     * map for this section in this object file.
		     */
		    l = 0;
		    if(start_section == FALSE){
			load_orders[l].name = ".section_start";
			load_orders[l].value =
				       cur_obj->section_maps[j].s->addr;
			l++;
		    }
		    for(k = 0; k < cur_obj->symtab->nsyms; k++){
			if(object_symbols[k].n_sect == nsect &&
			   (object_symbols[k].n_type & N_STAB) == 0){
			    load_orders[l].name = object_strings +
					  object_symbols[k].n_un.n_strx;
			    load_orders[l].value =
					  object_symbols[k].n_value;
			    l++;
			}
		    }
#ifdef DEBUG
		    if(debug & 0x4000)
			print_load_order(load_orders, nload_orders, ms,
					 cur_obj, "names and values");
#endif DEBUG

		    /*
		     * Sort the load order map by symbol value so the
		     * size and input offset fields can be set.
		     */
		    qsort(load_orders,
			  nload_orders,
			  sizeof(struct load_order),
			  (int (*)(const void *, const void *))
					       qsort_load_order_values);
		    for(l = 0; l < nload_orders - 1; l++){
			load_orders[l].input_offset =
				       load_orders[l].value -
				       cur_obj->section_maps[j].s->addr;
			load_orders[l].input_size =
					      load_orders[l + 1].value -
					      load_orders[l].value;
		    }
		    load_orders[l].input_offset = load_orders[l].value -
				       cur_obj->section_maps[j].s->addr;
		    load_orders[l].input_size =
				      cur_obj->section_maps[j].s->addr +
				      cur_obj->section_maps[j].s->size -
				      load_orders[l].value;
#ifdef DEBUG
		    if(debug & 0x8000)
			print_load_order(load_orders, nload_orders, ms,
					 cur_obj, "sizes and offsets");
#endif DEBUG

		    /*
		     * Now sort the load order map by symbol name so
		     * that it can be used for lookup.
		     */
		    qsort(load_orders,
			  nload_orders,
			  sizeof(struct load_order),
			  (int (*)(const void *, const void *))
					       qsort_load_order_names);
#ifdef DEBUG
		    if(debug & 0x10000)
			print_load_order(load_orders, nload_orders, ms,
					 cur_obj, "sorted by name");
#endif DEBUG
		    /*
		     * Increment the number of load_symbol needed for
		     * this section by the number of symbols in this
		     * object.
		     */
		    nsection_symbols += nload_orders;

		    /*
		     * Since there can only be one of these sections in
		     * the section map and it was found just break out
		     * of the loop looking for it.
		     */
		    break;
		}
	    }
	}
	/*
	 * Create the load_symbol hash table.  Used for looking up
	 * symbol names and trying to match load order file lines to
	 * them if the line is not a perfect match.
	 */
	create_load_symbol_hash_table(nsection_symbols);
	
	/*
	 * Clear the counter of ambigious secifications before the next
	 * section is processed.
	 */
	ambigious_specifications = 0;
#ifdef DEBUG
	if(debug & 0x2000)
	    print_load_symbol_hash_table();
#endif DEBUG

	/*
	 * Parse the load order file by changing '\n' to '\0'.
	 */
	for(i = 0; i < ms->order_size; i++){
	    if(ms->order_addr[i] == '\n')
		ms->order_addr[i] = '\0';
	}

	/*
	 * For lines in the order file set the orders and output_offset
	 * in the load maps for this section in all the object files
	 * that have this section.
	 */
	order = 1;
	output_offset = 0;
	line_number = 1;
	unused_specifications = 0;
	for(i = 0; i < ms->order_size; ){
	    line = ms->order_addr + i;
	    line_length = strlen(line);

	    parse_order_line(line, &archive_name, &object_name, &symbol_name,
			     ms, line_number);

	    load_order = lookup_load_order(archive_name, object_name,
					   symbol_name, ms, line_number);
	    if(load_order != NULL){
		if(load_order->order != 0){
		    if(archive_name == NULL)
			warning("multiple specification of %s:%s in "
				"-sectorder file: %s line %d for "
				"section (%0.16s,%0.16s)", object_name,
				symbol_name, ms->order_filename,
				line_number, ms->s.segname,
				ms->s.sectname);
		    else
			warning("multiple specification of %s:%s:%s in "
				"-sectorder file: %s line %d for "
				"section (%0.16s,%0.16s)", archive_name,
				object_name, symbol_name,
				ms->order_filename, line_number,
				ms->s.segname, ms->s.sectname);
		}
		else{
		    load_order->order = order++;
		    output_offset = round(output_offset,
					  (1 << ms->s.align));
		    load_order->output_offset = output_offset;
		    output_offset += load_order->input_size;
		}
	    }
	    else{
		if(strcmp(symbol_name, ".section_all") == 0){
		    section_map = lookup_section_map(archive_name,
						     object_name);
		    if(section_map != NULL){
			section_map->no_load_order = TRUE;
			output_offset = round(output_offset,
					      (1 << ms->s.align));
			section_map->offset = output_offset;
			output_offset += section_map->s->size;
		    }
		    else if(sectorder_detail == TRUE){
			if(archive_name == NULL){
			    warning("specification of %s:%s in "
				    "-sectorder file: %s line %d for "
				    "section (%0.16s,0.16s) not used "
				    "(object with that section not in "
				    "loaded objects)", object_name,
				    symbol_name, ms->order_filename,
				    line_number, ms->s.segname,
				    ms->s.sectname);
			}
			else{
			    warning("specification of %s:%s:%s in "
				    "-sectorder file: %s line %d for "
				    "section (%0.16s,0.16s) not used "
				    "(object with that section not in "
				    "loaded objects)", archive_name,
				    object_name, symbol_name,
				    ms->order_filename, line_number,
				    ms->s.segname, ms->s.sectname);
			}
		    }
		    else{
			unused_specifications++;
		    }
		}
		else if(sectorder_detail == TRUE){
		    if(archive_name == NULL){
			warning("specification of %s:%s in -sectorder "
				"file: %s line %d for section (%0.16s,"
				"%0.16s) not found in loaded objects",
				object_name, symbol_name,
				ms->order_filename, line_number,
				ms->s.segname, ms->s.sectname);
		    }
		    else{
			warning("specification of %s:%s:%s in "
				"-sectorder file: %s line %d for "
				"section (%0.16s,%0.16s) not found in "
				"loaded objects", archive_name,
				object_name, symbol_name,
				ms->order_filename, line_number,
				ms->s.segname, ms->s.sectname);
		    }
		}
		else{
		    unused_specifications++;
		}
	    }
	    i += line_length + 1;
	    line_number++;
	}

	/*
	 * Deallocate the memory for the load order file now that it is
	 * nolonger needed (since the memory has been written on it is
	 * allways deallocated so it won't get written to the swap file
	 * unnecessarily).
	 */
	if((r = vm_deallocate(task_self(), (vm_address_t)
	    ms->order_addr, ms->order_size)) != KERN_SUCCESS)
	    mach_fatal(r, "can't vm_deallocate() memory for -sectorder "
		       "file: %s for section (%0.16s,%0.16s)",
		       ms->order_filename, ms->s.segname,
		       ms->s.sectname);
	ms->order_addr = NULL;

	/*
	 * For all entries in the load maps that do not have an order
	 * because they were not specified in the load order file
	 * assign them an order.
	 */
	no_specifications = 0;
	for(q = &objects; *q; q = &(object_list->next)){
	    object_list = *q;
	    for(i = 0; i < object_list->used; i++){
		cur_obj = &(object_list->object_files[i]);
		if(cur_obj == base_obj)
		    continue;
		if(cur_obj->cur_section_map == NULL)
		    continue;
#ifdef DEBUG
		if(debug & 0x20000)
		    print_load_order(
				cur_obj->cur_section_map->load_orders,
				cur_obj->cur_section_map->nload_orders,
				ms, cur_obj, "file orders assigned");
#endif DEBUG
		load_order = cur_obj->cur_section_map->load_orders;
		n = cur_obj->cur_section_map->nload_orders;
		for(j = 0; j < n; j++){
		    if(load_order[j].order == 0){
			if(cur_obj->cur_section_map->no_load_order ==
			   TRUE)
			    continue;
			load_order[j].order = order++;
			output_offset = round(output_offset,
					      (1 << ms->s.align));
			load_order[j].output_offset = output_offset;
			output_offset += load_order[j].input_size;
			if(sectorder_detail == TRUE)
			    if(cur_obj->ar_hdr == NULL)
				warning("no specification for %s:%s in "
					"-sectorder file: %s for "
					"section (%0.16s,%0.16s)",
					cur_obj->file_name,
					load_order[j].name,
					ms->order_filename,
					ms->s.segname, ms->s.sectname);
			    else{
				k = sizeof(cur_obj->ar_hdr->ar_name) -1;
				if(cur_obj->ar_hdr->ar_name[k] == ' '){
				    do{
					if(cur_obj->ar_hdr->ar_name[k]
					   != ' ')
					    break;
					k--;
				    }while(k > 0);
				}
				warning("no specification for %s:%0.*s:"
					"%s in -sectorder file: %s for "
					"section (%0.16s,%0.16s)",
					cur_obj->file_name,
					k+1, cur_obj->ar_hdr->ar_name,
					load_order[j].name,
					ms->order_filename,
					ms->s.segname, ms->s.sectname);
			    }
			else
			    no_specifications++;
		    }
		    else{
			if(cur_obj->cur_section_map->no_load_order ==
			   TRUE){
			    if(cur_obj->ar_hdr == NULL){
				error("specification for both %s:%s "
				      "and %s:%s in -sectorder file: "
				      "%s for section (%0.16s,%0.16s) "
				      "(not allowed)",
				      cur_obj->file_name,
				      ".section_all",
				      cur_obj->file_name,
				      load_order[j].name,
				      ms->order_filename,
				      ms->s.segname, ms->s.sectname);
			    }
			    else{
				k = sizeof(cur_obj->ar_hdr->ar_name) -1;
				if(cur_obj->ar_hdr->ar_name[k] == ' '){
				    do{
					if(cur_obj->ar_hdr->ar_name[k]
					   != ' ')
					    break;
					k--;
				    }while(k > 0);
				}
				error("specification for both "
				      "%s:%0.*s:%s and %s:%0.*:%s "
				      "in -sectorder file: %s for "
				      "section (%0.16s,%0.16s) "
				      "(not allowed)",
				      cur_obj->file_name,
				      k+1, cur_obj->ar_hdr->ar_name,
				      ".section_all",
				      cur_obj->file_name,
				      k, cur_obj->ar_hdr->ar_name,
				      load_order[j].name,
				      ms->order_filename,
				      ms->s.segname, ms->s.sectname);
			    }
			}
		    }
		}
#ifdef DEBUG
		if(debug & 0x40000)
		    print_load_order(
				cur_obj->cur_section_map->load_orders,
				cur_obj->cur_section_map->nload_orders,
				ms, cur_obj, "all orders assigned");
#endif DEBUG
	    }
	}
	if(sectorder_detail == FALSE){
	    if(unused_specifications != 0)
		warning("%d symbols specified in -sectorder file: %s "
			"for section (%0.16s,%0.16s) not found in "
			"loaded objects", unused_specifications,
			ms->order_filename, ms->s.segname,
			ms->s.sectname);
	    if(no_specifications != 0)
		warning("%d symbols have no specifications in "
			"-sectorder file: %s for section (%0.16s,"
			"%0.16s)",no_specifications, ms->order_filename,
			ms->s.segname, ms->s.sectname);
	    if(ambigious_specifications != 0)
		warning("%d symbols have ambigious specifications in "
			"-sectorder file: %s for section (%0.16s,"
			"%0.16s)", ambigious_specifications,
			ms->order_filename, ms->s.segname,
			ms->s.sectname);
	}

	/*
	 * There can be seen a ".section_all" and symbol names for the
	 * same object file and these are reported as an error not a
	 * warning.
	 */
	if(errors)
	    return;

	/*
	 * Now the final size of the merged section can be set with all
	 * the contents of the section laid out.
	 */
	ms->s.size = output_offset;

	/*
	 * Finally the fine relocation maps can be allocated and filled
	 * in from the load order maps.
	 */
	for(q = &objects; *q; q = &(object_list->next)){
	    object_list = *q;
	    for(i = 0; i < object_list->used; i++){
		cur_obj = &(object_list->object_files[i]);
		if(cur_obj == base_obj)
		    continue;
		if(cur_obj->cur_section_map == NULL)
		    continue;
		/*
		 * If this object file has no load orders (a
		 * .section_all for it was specified) then just
		 * create a single fine relocation entry for it
		 * that take care of the whole section.
		 */
		if(cur_obj->cur_section_map->no_load_order == TRUE){
		    fine_relocs = allocate(sizeof(struct fine_reloc));
		    cur_obj->cur_section_map->fine_relocs = fine_relocs;
		    cur_obj->cur_section_map->nfine_relocs = 1;
		    fine_relocs[0].input_offset = 0;
		    fine_relocs[0].output_offset =
				       cur_obj->cur_section_map->offset;
		    continue;
		}
		n = cur_obj->cur_section_map->nload_orders;
		load_orders = cur_obj->cur_section_map->load_orders;
		fine_relocs = allocate(sizeof(struct fine_reloc) * n);
		cur_obj->cur_section_map->fine_relocs = fine_relocs;
		cur_obj->cur_section_map->nfine_relocs = n;
		for(j = 0; j < n ; j++){
		    fine_relocs[j].input_offset =
					   load_orders[j].input_offset;
		    fine_relocs[j].output_offset =
					   load_orders[j].output_offset;
		}
		/*
		 * Leave the fine relocation map in sorted order by
		 * their input offset so that the pass2 routines can
		 * use them.
		 */
		qsort(fine_relocs,
		      n,
		      sizeof(struct fine_reloc),
		      (int (*)(const void *, const void *))
					 qsort_fine_reloc_input_offset);

		/*
		 * The load order maps are now no longer needed unless
		 * the load map (-M) has been specified.
		 */
		if(load_map == FALSE){
		    free(cur_obj->cur_section_map->load_orders);
		    cur_obj->cur_section_map->load_orders = NULL;
		    cur_obj->cur_section_map->nload_orders = 0;
		}
	    }
	}

	/*
	 * If the load map option (-M) is specified build the
	 * structures to print the map.
	 */
	if(load_map == TRUE)
	    create_order_load_maps(ms, order - 1);
}

/*
 * parse_order_line() parses a load order line into it's archive name, object
 * name and symbol name.  The format for the lines is the following:
 *
 * [<archive name>:]<object name>:<symbol name>
 *
 * If the archive name is not present NULL is returned, if the object name is
 * not present it is set to point at "" and if the symbol name is not present it
 * is set to "".
 */
void
parse_order_line(
char *line,
char **archive_name,
char **object_name,
char **symbol_name,
struct merged_section *ms,
long line_number)
{
    long line_length;
    char *left_bracket;

	/*
	 * The trim has to be done before the checking for objective-C names
	 * syntax because it could have spaces at the end of the line.
	 */ 
	line = trim(line);

	line_length = strlen(line);
	if(line_length == 0){
	    *archive_name = NULL;
	    (*object_name) = "";
	    (*symbol_name) = "";
	    return;
	}

	/*
	 * To allow the objective-C symbol syntax of:
	 * +-[ClassName(CategoryName) Method:Name]
	 * since the method name can have ':'s the brackets
	 * have to be recognized.  This is the only place where
	 * the link editor knows about this.
	 */
	if(line[line_length - 1] == ']'){
	    left_bracket = strrchr(line, '[');
	    if(left_bracket == NULL)
		fatal("format error in -sectorder file: %s line %d "
		      "for section (%0.16s,%0.16s) (no matching "
		      "'[' for ending ']' found in symbol name)",
		      ms->order_filename, line_number,
		      ms->s.segname, ms->s.sectname);
	    *left_bracket = '\0';
	    *symbol_name = strrchr(line, ':');
	    *left_bracket = '[';
	}
	else
	    *symbol_name = strrchr(line, ':');

	if(*symbol_name == NULL){
	    *symbol_name = line;
	    line = "";
	}
	else{
	    **symbol_name = '\0';
	    (*symbol_name)++;
	}

	*object_name = strrchr(line, ':');
	if(*object_name == NULL){
	    *object_name = line;
	    *archive_name = NULL;
	}
	else{
	    **object_name = '\0';
	    (*object_name)++;
	    *archive_name = line;
	}
}

/*
 * create_name_arrays() build the sorted arrays of archive names and object
 * names which along with the load order maps will be use to search for archive,
 * object,symbol name triples from the load order files specified by the user.
 */
static
void
create_name_arrays(void)
{
    long i, j;
    struct object_list *object_list, **p;
    struct archive_name *ar;
    struct ar_hdr dummy;
    char *ar_name, *last_slash;

	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    for(i = 0; i < object_list->used; i++){
		cur_obj = &(object_list->object_files[i]);
		if(cur_obj == base_obj)
		    continue;
		if(cur_obj->ar_hdr != NULL){
		    ar = create_archive_name(cur_obj->file_name);
		    ar_name = allocate(sizeof(dummy.ar_name) + 1);
    		    strncpy(ar_name, cur_obj->ar_hdr->ar_name,
			    sizeof(dummy.ar_name));
		    ar_name[sizeof(dummy.ar_name)] = '\0';
    		    for(j = sizeof(dummy.ar_name) - 1; j >= 0 ; j--){
			if(ar_name[j] != ' ')
			    break;
			else
			    ar_name[j] = '\0';
		    }
		    create_object_name(&(ar->object_names),&(ar->nobject_names),
				       ar_name, j + 1, cur_obj->file_name);
		}
		else{
		    last_slash = strrchr(cur_obj->file_name, '/');
		    if(last_slash == NULL)
			j = 0;
		    else
			j = last_slash - cur_obj->file_name + 1;
		    create_object_name(&object_names, &nobject_names,
				       cur_obj->file_name, j, NULL);
		}
	    }
	}

	/*
	 * Sort the arrays of names.
	 */
	if(narchive_names != 0){
	    archive_names = reallocate(archive_names, 
				       sizeof(struct archive_name) *
				       narchive_names);
	    qsort(archive_names,
		  narchive_names,
		  sizeof(struct archive_name),
		  (int (*)(const void *, const void *))qsort_archive_names);
	    for(i = 0; i < narchive_names; i++){
		archive_names[i].object_names = reallocate(
						archive_names[i].object_names,
						sizeof(struct object_name) *
						archive_names[i].nobject_names);
		qsort(archive_names[i].object_names,
		      archive_names[i].nobject_names,
		      sizeof(struct object_name),
		      (int (*)(const void *, const void *))qsort_object_names);
	    }
	}
	if(nobject_names != nobjects)
	    object_names = reallocate(object_names,
				  sizeof(struct object_name) * nobject_names);
	qsort(object_names,
	      nobject_names,
	      sizeof(struct object_name),
	      (int (*)(const void *, const void *))qsort_object_names);
}

/*
 * create_object_name() creates a slot in the archive names array for the name
 * passed to it.  The name may be seen more than once.  The archive name must
 * not have a ':' in it since that is used to delimit names in the -sectorder
 * files.
 */
static
struct archive_name *
create_archive_name(
char *archive_name)
{
    long i;
    struct archive_name *ar;

	if(strchr(archive_name, ':') != NULL)
	    fatal("archive name: %s has a ':' (it can't when -sectorder "
		  "options are used)", archive_name);
	ar = archive_names;
	for(i = 0; i < narchive_names; i++){
	    if(strcmp(ar->archive_name, archive_name) == 0)
		return(ar);
	    ar++;
	}
	if(archive_names == NULL)
	    archive_names = allocate(sizeof(struct archive_name) * nobjects);
	ar = archive_names + narchive_names;
	narchive_names++;
	ar->archive_name = archive_name;
	ar->object_names = NULL;
	ar->nobject_names = 0;
	return(ar);
}

/*
 * create_object_name() creates a slot in the object names array passed to it
 * for the name passed to it and the current object (cur_obj).  The size of the
 * array is in nobject_names.  Both the object names array and it's size are
 * passed indirectly since it may be allocated to add the name.  The name should
 * not be duplicated in the array.  If this objects array is for an archive
 * the archive_name is passed for error messages and is NULL in not in an
 * archive.  The object name must not have a ':' in it since that is used to
 * delimit names in the -sectorder files.
 */
static
void
create_object_name(
struct object_name **object_names,
long *nobject_names,
char *object_name,
long index_length,
char *archive_name)
{
    long n, i;
    struct object_name *o;

	if(strchr(object_name, ':') != NULL)
	    if(archive_name != NULL)
		fatal("archive member name: %s(%s) has a ':' in it (it can't "
		      "when -sectorder options are used)", archive_name,
		      object_name);
	    else
		fatal("object file name: %s has a ':' in it (it can't when "
		      "-sectorder options are used)", object_name);

	o = *object_names;
	n = *nobject_names;
	for(i = 0; i < n; i++){
	    if(strcmp(o->object_name, object_name) == 0)
		if(archive_name != NULL)
		    warning("duplicate archive member name: %s(%s) loaded ("
			    "ambigious when -sectorder options are used)",
			    archive_name, object_name);
		else
		    warning("duplicate object file name: %s loaded ("
			    "ambigious when -sectorder options are used)",
			    object_name);
	    o++;
	}
	if(*object_names == NULL)
	    *object_names = allocate(sizeof(struct object_name) * nobjects);
	o = *object_names + *nobject_names;
	(*nobject_names)++;
	o->object_name = object_name;
	o->object_file = cur_obj;
	o->index_length = index_length;
}

/*
 * free_name_arrays() frees up the space created for the sorted name arrays.
 */
static
void
free_name_arrays(void)
{
    long i, j;

	if(archive_names != NULL){
	    for(i = 0; i < narchive_names; i++){
		for(j = 0; j < archive_names[i].nobject_names; j++){
		    free(archive_names[i].object_names[j].object_name);
		}
	    }
	    free(archive_names);
	    archive_names = NULL;
	    narchive_names = 0;
	}
	if(object_names != NULL){
	    free(object_names);
	    object_names = NULL;
	    nobject_names = 0;
	}
}

/*
 * create_load_symbol_hash_table() creates a hash table of all the symbol names
 * in the section for the current section map.  This table is use by
 * lookup_load_order when an exact match for the specification can't be found.
 */
static
void
create_load_symbol_hash_table(
long nsection_symbols)
{
    long i, j;

	/* set up the hash table */
	if(load_symbol_hashtable == NULL)
	    load_symbol_hashtable = allocate(sizeof(struct load_symbol *) *
					     LOAD_SYMBOL_HASHTABLE_SIZE);
	memset(load_symbol_hashtable, '\0', sizeof(struct load_symbol *) *
					    LOAD_SYMBOL_HASHTABLE_SIZE);

	/* set up the load_symbols */
	if(nsection_symbols > load_symbols_size){
	    load_symbols_size = nsection_symbols;
	    load_symbols = reallocate(load_symbols, sizeof(struct load_symbol) *
						    load_symbols_size);
	}
	memset(load_symbols, '\0', sizeof(struct load_symbol) *
				   load_symbols_size);
	load_symbols_used = 0;

	for(i = 0; i < narchive_names; i++){
	    for(j = 0; j < archive_names[i].nobject_names; j++){
		if(archive_names[i].object_names[j].object_file->
							cur_section_map != NULL)
		    create_load_symbol_hash_table_for_object(
			    archive_names[i].archive_name,
			    archive_names[i].object_names[j].object_name,
			    archive_names[i].object_names[j].index_length,
			    archive_names[i].object_names[j].object_file->
						 cur_section_map->load_orders,
			    archive_names[i].object_names[j].object_file->
						 cur_section_map->nload_orders);
	    }
	}

	for(j = 0; j < nobject_names; j++){
	    if(object_names[j].object_file->cur_section_map != NULL)
		create_load_symbol_hash_table_for_object(
		    NULL,
		    object_names[j].object_name,
		    object_names[j].index_length,
		    object_names[j].object_file->cur_section_map->load_orders,
		    object_names[j].object_file->cur_section_map->nload_orders);
	}
}

/*
 * free_load_symbol_hash_table() frees up the space used by the symbol hash
 * table.
 */
static
void
free_load_symbol_hash_table(
void)
{
	/* free the hash table */
	if(load_symbol_hashtable != NULL)
	    free(load_symbol_hashtable);
	load_symbol_hashtable = NULL;

	/* free the load_symbols */
	if(load_symbols != NULL)
	    free(load_symbols);
	load_symbols_size = 0;
	load_symbols_used = 0;
}

/*
 * create_load_symbol_hash_table_for_object() is used by
 * create_load_symbol_hash_table() to create the hash table of all the symbol
 * names in the section that is being scatter loaded.  This routine enters all
 * the symbol names in the load_orders in to the hash table for the specified
 * archive_name object_name pair.
 */
static
void
create_load_symbol_hash_table_for_object(
char *archive_name,
char *object_name,
long index_length,
struct load_order *load_orders,
long nload_orders)
{
    long i, hash_index;
    struct load_symbol *load_symbol, *hash_load_symbol, *other_name;

	for(i = 0; i < nload_orders; i++){
	    /*
	     * Get a new load symbol and set the fields for it this load order
	     * entry.
	     */
	    load_symbol = load_symbols + load_symbols_used;
	    load_symbols_used++;
	    load_symbol->symbol_name = load_orders[i].name;
	    load_symbol->object_name = object_name;
	    load_symbol->archive_name = archive_name;
	    load_symbol->index_length = index_length;
	    load_symbol->load_order = &(load_orders[i]);

	    /* find this symbol's place in the hash table */
	    hash_index = hash_string(load_orders[i].name) %
			 LOAD_SYMBOL_HASHTABLE_SIZE;
	    for(hash_load_symbol = load_symbol_hashtable[hash_index]; 
		hash_load_symbol != NULL;
		hash_load_symbol = hash_load_symbol->next){
		if(strcmp(load_orders[i].name,
			  hash_load_symbol->symbol_name) == 0)
		    break;
	    }
	    /* if the symbol was not found in the hash table enter it */
	    if(hash_load_symbol == NULL){
		load_symbol->other_names = NULL;
		load_symbol->next = load_symbol_hashtable[hash_index]; 
		load_symbol_hashtable[hash_index] = load_symbol;
	    }
	    else{
		/*
		 * If the symbol was found in the hash table go through the
		 * other load symbols for the same name checking if their is
		 * another with exactly the same archive and object name and
		 * generate a warning if so.  Then add this load symbol to the
		 * list of other names.
		 */
		for(other_name = hash_load_symbol;
		    other_name != NULL;
		    other_name = other_name->other_names){

		    if(archive_name != NULL){
			if(strcmp(other_name->object_name, object_name) == 0 &&
			   other_name->archive_name != NULL &&
			   strcmp(other_name->archive_name, archive_name) == 0){
			    warning("symbol appears more than once in the same "
				    "file (%s:%s:%s) which is abiguous when "
				    "using a -sectorder option",
				    other_name->archive_name,
				    other_name->object_name,
				    other_name->symbol_name);
			    break;
			}
		    }
		    else{
			if(strcmp(other_name->object_name, object_name) == 0 &&
			   other_name->archive_name == NULL){
			    warning("symbol appears more than once in the same "
				    "file (%s:%s) which is abiguous when using "
				    "a -sectorder option",
				    other_name->object_name,
				    other_name->symbol_name);
			    break;
			}
		    }
		}
		load_symbol->other_names = hash_load_symbol->other_names;
		hash_load_symbol->other_names = load_symbol;
		load_symbol->next = NULL;
	    }
	}
}

/*
 * lookup_load_order() is passed an archive, object, symbol name triple and that
 * is looked up in the name arrays and the load order map and returns a pointer
 * to the load order map that matches it.  Only archive_name may be NULL on
 * input.  It returns NULL if not found.
 */
static
struct load_order *
lookup_load_order(
char *archive_name,
char *object_name,
char *symbol_name,
struct merged_section *ms,
long line_number)
{
    struct archive_name *a;
    struct object_name *o;
    struct load_order *l;
    long n;

    long hash_index, number_of_matches;
    struct load_symbol *hash_load_symbol, *other_name, *first_match;
    char *last_slash, *base_name, *archive_base_name;

	if(archive_name != NULL){
	    a = bsearch(archive_name, archive_names, narchive_names,
			sizeof(struct archive_name),
			(int (*)(const void *, const void *))
						  	 bsearch_archive_names);
	    if(a == NULL)
		goto no_exact_match;
	    o = a->object_names;
	    n = a->nobject_names;
	}
	else{
	    o = object_names;
	    n = nobject_names;
	}

	o = bsearch(object_name, o, n, sizeof(struct object_name),
		    (int (*)(const void *, const void *))bsearch_object_names);
	if(o == NULL)
	    goto no_exact_match;
	if(o->object_file->cur_section_map == NULL)
	    goto no_exact_match;

	l = o->object_file->cur_section_map->load_orders;
	n = o->object_file->cur_section_map->nload_orders;
	l = bsearch(symbol_name, l, n, sizeof(struct load_order),
		    (int (*)(const void *, const void *))
						      bsearch_load_order_names);
	if(l == NULL)
	    goto no_exact_match;
	return(l);

no_exact_match:
	/*
	 * To get here an exact match of the archive_name, object_name, and
	 * symbol_name was not found so try to find some load_order for the
	 * symbol_name using the hash table of symbol names.  First thing here
	 * is to strip leading and trailing blanks from the names.
	 */
	archive_name = trim(archive_name);
	object_name = trim(object_name);
	symbol_name = trim(symbol_name);

	/* find this symbol's place in the hash table */
	hash_index = hash_string(symbol_name) % LOAD_SYMBOL_HASHTABLE_SIZE;
	for(hash_load_symbol = load_symbol_hashtable[hash_index]; 
	    hash_load_symbol != NULL;
	    hash_load_symbol = hash_load_symbol->next){
	    if(strcmp(symbol_name, hash_load_symbol->symbol_name) == 0)
		break;
	}
	/* if the symbol was not found then give up */
	if(hash_load_symbol == NULL)
	    return(NULL);

	/* if this symbol is in only one object file then use that */
	if(hash_load_symbol->other_names == NULL)
	    return(hash_load_symbol->load_order);

	/*
	 * Now try to see if their is just one name that has not had an order
	 * specified for it and use that if that is the case.  This ignores both
	 * the archive_name and the object_name.
	 */
	number_of_matches = 0;
	first_match = NULL;
	for(other_name = hash_load_symbol;
	    other_name != NULL;
	    other_name = other_name->other_names){
	    if(other_name->load_order->order == 0){
		if(first_match == NULL)
		    first_match = other_name;
		number_of_matches++;
	    }
	}
	if(number_of_matches == 1)
	    return(first_match->load_order);

	/*
	 * Now try to see if their is just one name that the object file name
	 * specified for it matches and use that if that is the case.  Only the
	 * object basename is used and matched against the base name of the
	 * objects or the archive member name that may have been truncated.
	 * This ignores the archive name.
	 */
	last_slash = strrchr(object_name, '/');
	if(last_slash == NULL)
	    base_name = object_name;
	else
	    base_name = last_slash + 1;
	number_of_matches = 0;
	first_match = NULL;
	for(other_name = hash_load_symbol;
	    other_name != NULL;
	    other_name = other_name->other_names){
	    if(other_name->load_order->order == 0){
		if(other_name->archive_name != NULL){
		    if(strncmp(base_name, other_name->object_name,
			       other_name->index_length) == 0){
			if(first_match == NULL)
			    first_match = other_name;
			number_of_matches++;
		    }
		}
		else{
		    if(strcmp(base_name, other_name->object_name +
			      other_name->index_length) == 0){
			if(first_match == NULL)
			    first_match = other_name;
			number_of_matches++;
		    }
		}
	    }
	}
	if(number_of_matches == 1)
	    return(first_match->load_order);

	/*
	 * Now try to see if their is just one name that the base name of the
	 * archive file name specified for it matches and use that if that is
	 * the case.  This ignores the object name.
	 */
	if(archive_name != NULL){
	    last_slash = strrchr(archive_name, '/');
	    if(last_slash == NULL)
		base_name = archive_name;
	    else
		base_name = last_slash + 1;
	    number_of_matches = 0;
	    first_match = NULL;
	    for(other_name = hash_load_symbol;
		other_name != NULL;
		other_name = other_name->other_names){
		if(other_name->load_order->order == 0){
		    if(other_name->archive_name != NULL){
			last_slash = strrchr(other_name->archive_name, '/');
			if(last_slash == NULL)
			    archive_base_name = other_name->archive_name;
			else
			    archive_base_name = last_slash + 1;

			if(strcmp(base_name, archive_base_name) == 0){
			    if(first_match == NULL)
				first_match = other_name;
			    number_of_matches++;
			}
		    }
		}
	    }
	    if(number_of_matches == 1)
		return(first_match->load_order);
	}

	/*
	 * Now we know their is more than one possible match for this symbol
	 * name.  So the first one that does not have an order is picked and
	 * either the ambigious_specifications count is incremented or warnings
	 * are generated.
	 */
	first_match = NULL;
	for(other_name = hash_load_symbol;
	    other_name != NULL;
	    other_name = other_name->other_names){
	    if(other_name->load_order->order == 0){
		first_match = other_name;
		if(sectorder_detail){
		    if(archive_name != NULL){
			if(other_name->archive_name != NULL)
			    warning("ambigious specification of %s:%s:%s in "
				    "-sectorder file: %s line %d for "
				    "section (%0.16s,%0.16s) using %s:%s:%s",
				    archive_name, object_name, symbol_name,
				    ms->order_filename, line_number,
				    ms->s.segname, ms->s.sectname,
				    other_name->object_name,
				    other_name->symbol_name);
			else
			    warning("ambigious specification of %s:%s:%s in "
				    "-sectorder file: %s line %d for "
				    "section (%0.16s,%0.16s) using %s:%s",
				    archive_name, object_name, symbol_name,
				    ms->order_filename, line_number,
				    ms->s.segname, ms->s.sectname,
				    other_name->object_name,
				    other_name->symbol_name);
		    }
		    else{
			if(other_name->archive_name != NULL)
			    warning("ambigious specification of %s:%s in "
				    "-sectorder file: %s line %d for "
				    "section (%0.16s,%0.16s) using %s:%s:%s",
				    object_name, symbol_name,
				    ms->order_filename, line_number,
				    ms->s.segname, ms->s.sectname,
				    other_name->archive_name,
				    other_name->object_name,
				    other_name->symbol_name);
			else
			    warning("ambigious specification of %s:%s in "
				    "-sectorder file: %s line %d for "
				    "section (%0.16s,%0.16s) using %s:%s",
				    object_name, symbol_name,
				    ms->order_filename, line_number,
				    ms->s.segname, ms->s.sectname,
				    other_name->object_name,
				    other_name->symbol_name);
		    }
		}
		break;
	    }
	}
	if(sectorder_detail == TRUE){
	    for(other_name = hash_load_symbol;
		other_name != NULL;
		other_name = other_name->other_names){
		if(other_name->load_order->order == 0 &&
		   first_match != other_name){
		    if(archive_name != NULL){
			if(other_name->archive_name != NULL)
			    warning("specification %s:%s:%s ambigious with "
				    "%s:%s:%s", archive_name, object_name,
				    symbol_name, other_name->archive_name,
				    other_name->object_name,
				    other_name->symbol_name);
			else
			    warning("specification %s:%s:%s ambigious with "
				    "%s:%s", archive_name, object_name,
				    symbol_name, other_name->object_name,
				    other_name->symbol_name);
		    }
		    else{
			if(other_name->archive_name != NULL)
			    warning("specification %s:%s ambigious with "
				    "%s:%s:%s", object_name, symbol_name,
				    other_name->archive_name,
				    other_name->object_name,
				    other_name->symbol_name);
			else
			    warning("specification %s:%s ambigious with "
				    "%s:%s", object_name, symbol_name,
				    other_name->object_name,
				    other_name->symbol_name);
		    }
		}
	    }
	}
	else{
	    ambigious_specifications++;
	}
	return(first_match->load_order);
}

/*
 * trim() is passed a name and trims the spaces off the begining and endding of
 * the name.  It writes '\0' in the spaces at the end of the name.  It returns
 * a pointer into the trimed name.
 */
static
char *
trim(
char *name)
{
    char *p;

	if(name == NULL)
	    return(name);
	
	while(*name != '\0' && *name == ' ')
	    name++;
	if(*name == '\0');
	    return(name);

	p = name;
	while(*p != '\0')
	    p++;
	p--;
	while(p != name && *p == ' ')
	    *p-- = '\0';
	return(name);
}

/*
 * lookup_section_map() is passed an archive, object pair and that is looked up
 * in the name arrays and returns a pointer to the section map that matches it.
 * It returns NULL if not found.
 */
static
struct section_map *
lookup_section_map(
char *archive_name,
char *object_name)
{
    struct archive_name *a;
    struct object_name *o;
    long n;

	if(archive_name != NULL){
	    a = bsearch(archive_name, archive_names, narchive_names,
			sizeof(struct archive_name),
			(int (*)(const void *, const void *))
						  	 bsearch_archive_names);
	    if(a == NULL)
		return(NULL);
	    o = a->object_names;
	    n = a->nobject_names;
	}
	else{
	    o = object_names;
	    n = nobject_names;
	}

	o = bsearch(object_name, o, n, sizeof(struct object_name),
		    (int (*)(const void *, const void *))bsearch_object_names);
	if(o == NULL)
	    return(NULL);
	return(o->object_file->cur_section_map);
}

/*
 * Function for qsort to sort load_order structs by their value
 */
static
int
qsort_load_order_values(
const struct load_order *load_order1,
const struct load_order *load_order2)
{
	return(load_order1->value - load_order2->value);
}

/*
 * Function for qsort to sort load_order structs by their name
 */
static
int
qsort_load_order_names(
const struct load_order *load_order1,
const struct load_order *load_order2)
{
	return(strcmp(load_order1->name, load_order2->name));
}

/*
 * Function for bsearch to search load_order structs for their name
 */
static
int
bsearch_load_order_names(
char *symbol_name,
const struct load_order *load_order)
{
	return(strcmp(symbol_name, load_order->name));
}

/*
 * Function for qsort for comparing archive names.
 */
static
int
qsort_archive_names(
const struct archive_name *archive_name1,
const struct archive_name *archive_name2)
{
	return(strcmp(archive_name1->archive_name,
		      archive_name2->archive_name));
}

/*
 * Function for bsearch for finding archive names.
 */
static
int
bsearch_archive_names(
const char *name,
const struct archive_name *archive_name)
{
	return(strcmp(name, archive_name->archive_name));
}

/*
 * Function for qsort for comparing object names.
 */
static
int
qsort_object_names(
const struct object_name *object_name1,
const struct object_name *object_name2)
{
	return(strcmp(object_name1->object_name,
		      object_name2->object_name));
}

/*
 * Function for bsearch for finding object names.
 */
static
int
bsearch_object_names(
const char *name,
const struct object_name *object_name)
{
	return(strcmp(name, object_name->object_name));
}

/*
 * Function for qsort to sort fine_reloc structs by their input_offset
 */
static
int
qsort_fine_reloc_input_offset(
const struct fine_reloc *fine_reloc1,
const struct fine_reloc *fine_reloc2)
{
	return(fine_reloc1->input_offset - fine_reloc2->input_offset);
}

/*
 * Function for qsort to sort order_load_map structs by their order.
 */
static
int
qsort_order_load_map_orders(
const struct order_load_map *order_load_map1,
const struct order_load_map *order_load_map2)
{
	return(order_load_map1->order - order_load_map2->order);
}

/*
 * create_order_load_map() creates the structures to be use for printing the
 * load map.
 */
static
void
create_order_load_maps(
struct merged_section *ms,
long norders)
{
    long i, j, k, l, m, n;
    struct order_load_map *order_load_maps;
    struct load_order *load_orders;

	order_load_maps = allocate(sizeof(struct order_load_map) * norders);
	ms->order_load_maps = order_load_maps;
	ms->norder_load_maps = norders;
	l = 0;
	for(i = 0; i < narchive_names; i++){
	    for(j = 0; j < archive_names[i].nobject_names; j++){
	        cur_obj = archive_names[i].object_names[j].object_file;
		for(m = 0; m < cur_obj->nsection_maps; m++){
		    if(cur_obj->section_maps[m].output_section != ms)
			continue;
		    n = cur_obj->section_maps[m].nload_orders;
		    load_orders = cur_obj->section_maps[m].load_orders;
		    for(k = 0; k < n ; k++){
		       order_load_maps[l].archive_name = 
						  archive_names[i].archive_name;
		       order_load_maps[l].object_name = 
				   archive_names[i].object_names[j].object_name;
		       order_load_maps[l].symbol_name = load_orders[k].name;
		       order_load_maps[l].symbol_name = load_orders[k].name;
		       order_load_maps[l].value = load_orders[k].value;
		       order_load_maps[l].section_map =
						    &(cur_obj->section_maps[m]);
		       order_load_maps[l].size = load_orders[k].input_size;
		       order_load_maps[l].order = load_orders[k].order;
		       l++;
		    }
		    break;
		}
	    }
	}
	for(j = 0; j < nobject_names; j++){
	    cur_obj = object_names[j].object_file;
	    for(m = 0; m < cur_obj->nsection_maps; m++){
		if(cur_obj->section_maps[m].output_section != ms)
		    continue;
		n = cur_obj->section_maps[m].nload_orders;
		load_orders = cur_obj->section_maps[m].load_orders;
		for(k = 0; k < n ; k++){
		   order_load_maps[l].archive_name = NULL;
		   order_load_maps[l].object_name = object_names[j].object_name;
		   order_load_maps[l].symbol_name = load_orders[k].name;
		   order_load_maps[l].value = load_orders[k].value;
		   order_load_maps[l].section_map = &(cur_obj->section_maps[m]);
		   order_load_maps[l].size = load_orders[k].input_size;
		   order_load_maps[l].order = load_orders[k].order;
		   l++;
		}
	    }
	}

#ifdef DEBUG
	if(debug & 0x80000){
	    for(i = 0; i < norders; i++){
		if(order_load_maps[i].archive_name != NULL)
		    print("%s:", order_load_maps[i].archive_name);
		print("%s:%s\n", order_load_maps[i].object_name,
		order_load_maps[i].symbol_name);
	    }
	}
#endif DEBUG

	qsort(order_load_maps,
	      norders,
	      sizeof(struct order_load_map),
	      (int (*)(const void *, const void *))qsort_order_load_map_orders);
}
#endif !defined(RLD)

/*
 * output_literal_sections() causes each merged literal section to be copied
 * to the output file.  It is called from pass2().
 */
void
output_literal_sections(void)
{
    struct merged_segment **p, *msg;
    struct merged_section **content, *ms;

	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->s.flags == S_CSTRING_LITERALS ||
		   ms->s.flags == S_4BYTE_LITERALS ||
		   ms->s.flags == S_8BYTE_LITERALS ||
		   ms->s.flags == S_LITERAL_POINTERS)
		    (*ms->literal_output)(ms->literal_data, ms);
		content = &(ms->next);
	    }
	    p = &(msg->next);
	}
}

#ifndef RLD
/*
 * output_sections_from_files() causes each section created from a file to be
 * copied to the output file.  It is called from pass2().
 */
void
output_sections_from_files(void)
{
    struct merged_segment **p, *msg;
    struct merged_section **content, *ms;
#ifdef DEBUG
    kern_return_t r;
#endif DEBUG

	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->contents_filename != NULL){
		    memcpy(output_addr + ms->s.offset,
			   ms->file_addr, ms->file_size);
		    /*
		     * The entire section size is flushed (ms->s.size) not just
		     * the size of the file used to create it (ms->filesize) so
		     * to flush the padding due to alignment.
		     */
		    output_flush(ms->s.offset, ms->s.size);
#ifdef DEBUG
		    if((r = vm_deallocate(task_self(), (vm_address_t)
			ms->file_addr, ms->file_size)) != KERN_SUCCESS)
			mach_fatal(r, "can't vm_deallocate() memory for file: "
				   "%s used to create section (%0.16s,%0.16s)",
				   ms->contents_filename, ms->s.segname,
				   ms->s.sectname);
		    ms->file_addr = NULL;
#endif DEBUG
		}
		content = &(ms->next);
	    }
	    p = &(msg->next);
	}
}
#endif !defined(RLD)

/*
 * output_section() copies the contents of a section and it's relocation entries
 * (if saving relocation entries) into the output file's memory buffer.  Then it
 * calls the appropriate routine specific to the target machine to relocate the
 * section and update the relocation entries (if saving relocation entries).
 */
void
output_section(
struct section_map *map)
{
    char *contents;
    struct relocation_info *relocs;
    long start_nrelocs;

#ifdef DEBUG
	/* The compiler "warning: `start_nrelocs' may be used uninitialized */
	/* in this function" can safely be ignored */
    	start_nrelocs = 0;
#endif DEBUG

	/*
	 * If this section has no contents and no relocation entries just
	 * return.  This can happen a lot with object files that have empty
	 * sections.
	 */ 
	if(map->s->size == 0 && map->s->nreloc == 0)
	    return;

	/*
	 * Copy the contents of the section from the input file into the memory
	 * buffer for the output file.
	 */
	if(map->nfine_relocs != 0)
	    contents = allocate(map->s->size);
	else
	    contents = output_addr + map->output_section->s.offset +map->offset;
	memcpy(contents, cur_obj->obj_addr + map->s->offset, map->s->size);

	/*
	 * If the section has no relocation entries then no relocation is to be
	 * done so just flush the contents and return.
	 */
	if(map->s->nreloc == 0){
#ifndef RLD
	    if(map->nfine_relocs != 0){
		scatter_copy(map, contents);
		free(contents);
	    }
	    else
		output_flush(map->output_section->s.offset + map->flush_offset,
			     map->s->size + (map->offset - map->flush_offset));
#endif !defined(RLD)
	    return;
	}
	else
	    map->output_section->relocated = TRUE;

	/*
	 * If the relocation entries appear in the file then copy them from the 
	 * input file into the memory buffer for the output file.
	 */
	if(save_reloc){
	    relocs = (struct relocation_info *)(output_addr +
		     map->output_section->s.reloff +
		     map->output_section->output_nrelocs *
						sizeof(struct relocation_info));
	    memcpy(relocs,
		   cur_obj->obj_addr + map->s->reloff,
		   map->s->nreloc * sizeof(struct relocation_info));
	    start_nrelocs = map->output_section->output_nrelocs;
	    map->output_section->output_nrelocs += map->s->nreloc;
	}
	else{
	    relocs = (struct relocation_info *)(cur_obj->obj_addr +
					        map->s->reloff);
	}

	/*
	 * Relocate the contents of the section (based on the target machine)
	 */
	if(cputype == CPU_TYPE_MC680x0)
	    generic_reloc(contents, relocs, map);
	else
	    fatal("internal error: output_section() called with unknown "
		  "cputype (%d) set", cputype);
#ifndef RLD
	if(map->nfine_relocs != 0){
	    scatter_copy(map, contents);
	    free(contents);
	}
	else
	    output_flush(map->output_section->s.offset + map->flush_offset,
			 map->s->size + (map->offset - map->flush_offset));
	if(save_reloc)
	    output_flush(map->output_section->s.reloff + start_nrelocs *
			 sizeof(struct relocation_info),
			 map->s->nreloc * sizeof(struct relocation_info));
#endif !defined(RLD)
}

#ifndef RLD
/*
 * scatter_copy() copies the relocated contents of a section into the output
 * file's memory buffer based on the section's fine relocation maps.
 */
static
void
scatter_copy(
struct section_map *map,
char *contents)
{
    long i;

	for(i = 0; i < map->nfine_relocs - 1; i++){
	    memcpy(output_addr + map->output_section->s.offset +
					      map->fine_relocs[i].output_offset,
		   contents + map->fine_relocs[i].input_offset,
		   map->fine_relocs[i+1].input_offset -
					      map->fine_relocs[i].input_offset);
	}
	memcpy(output_addr + map->output_section->s.offset +
					  map->fine_relocs[i].output_offset,
	       contents + map->fine_relocs[i].input_offset,
	       map->s->size - map->fine_relocs[i].input_offset);
}

/*
 * flush_scatter_copied_sections() flushes the entire merged section's output
 * for each merged regular (non-literal) content section that has a load order.
 */
void
flush_scatter_copied_sections(void)
{
    struct merged_segment **p, *msg;
    struct merged_section **content, *ms;

	p = &merged_segments;
	while(*p){
	    msg = *p;
	    content = &(msg->content_sections);
	    while(*content){
		ms = *content;
		if(ms->order_filename != NULL && ms->s.flags == 0){
		    output_flush(ms->s.offset, ms->s.size);
		}
		content = &(ms->next);
	    }
	    p = &(msg->next);
	}
}
#endif !defined(RLD)

#ifdef RLD
/*
 * reset_merged_sections() is called from rld_load() to place the merged
 * sections back on their merged segment (layout() placed all of them on the
 * object_segment for the MH_OBJECT filetype) and it zeros the size of each the
 * merged section so it can be accumulated for the next rld_load().
 */
void
reset_merged_sections(void)
{
    struct merged_segment *msg;
    struct merged_section *ms, *prev_ms;

	msg = original_merged_segments;
	if(msg != NULL && merged_segments->content_sections != NULL){
	    ms = merged_segments->content_sections;
	    while(ms != NULL){
		if(strncmp(ms->s.segname, msg->sg.segname,
			   sizeof(msg->sg.segname)) == 0){
		    msg->content_sections = ms;
		    ms->s.size = 0;
		    prev_ms = ms;
		    ms = ms->next;
		    while(ms != NULL && strncmp(ms->s.segname, msg->sg.segname,
			   			sizeof(msg->sg.segname)) == 0){
			ms->s.size = 0;
			prev_ms = ms;
			ms = ms->next;
		    }
		    prev_ms->next = NULL;
		}
		else{
		    msg = msg->next;
		}
	    }
	}

	msg = original_merged_segments;
	if(msg != NULL && merged_segments->zerofill_sections != NULL){
	    ms = merged_segments->zerofill_sections;
	    while(ms != NULL){
		if(strncmp(ms->s.segname, msg->sg.segname,
			   sizeof(msg->sg.segname)) == 0){
		    msg->zerofill_sections = ms;
		    ms->s.size = 0;
		    prev_ms = ms;
		    ms = ms->next;
		    while(ms != NULL && strncmp(ms->s.segname, msg->sg.segname,
			   			sizeof(msg->sg.segname)) == 0){
			ms->s.size = 0;
			prev_ms = ms;
			ms = ms->next;
		    }
		    prev_ms->next = NULL;
		}
		else{
		    msg = msg->next;
		}
	    }
	}
	merged_segments = original_merged_segments;
	original_merged_segments = NULL;
}

/*
 * zero_merged_sections_sizes() is called from rld_load() to zero the size field
 * in the merged sections so the sizes can be accumulated and free the literal
 * data for any literal sections.
 */
void
zero_merged_sections_sizes(void)
{
    struct merged_segment **p, *msg;
    struct merged_section **q, *ms;

	p = &merged_segments;
	while(*p){
	    msg = *p;
	    q = &(msg->content_sections);
	    while(*q){
		ms = *q;
		ms->s.size = 0;
		if(ms->literal_data != NULL){
		    if(ms->literal_free != NULL){
			(*ms->literal_free)(ms->literal_data, ms);
		    }
		}
		q = &(ms->next);
	    }
	    q = &(msg->zerofill_sections);
	    while(*q){
		ms = *q;
		ms->s.size = 0;
		q = &(ms->next);
	    }
	    p = &(msg->next);
	}
}

/*
 * remove_sections() removes the sections and segments that first came from the
 * current set from the merged section list.  The order that sections are
 * merged on to the lists is taken advantaged of here.
 */
void
remove_merged_sections(void)
{
    struct merged_segment *msg, *prev_msg, *next_msg;
    struct merged_section *ms, *prev_ms, *next_ms;

	/* The compiler "warning: `prev_msg' and `prev_ms' may be used */
	/* uninitialized in this function" can safely be ignored */
	prev_msg = NULL;
	prev_ms = NULL;

	if(original_merged_segments != NULL)
	    reset_merged_sections();

	for(msg = merged_segments; msg != NULL; msg = msg->next){
	    /*
	     * If this segment first comes from the current set then all
	     * remaining segments also come from this set and all of their
	     * sections.  So they are all removed from the list.
	     */
	    if(msg->set_num == cur_set){
		if(msg == merged_segments)
		    merged_segments = NULL;
		else
		    prev_msg->next = NULL;
		while(msg != NULL){
		    ms = msg->content_sections;
		    while(ms != NULL){
			if(ms->literal_data != NULL){
			    if(ms->literal_free != NULL){
				(*ms->literal_free)(ms->literal_data, ms);
				free(ms->literal_data);
				ms->literal_data = NULL;
			    }
			}
			next_ms = ms->next;
			free(ms);
			ms = next_ms;
		    }
		    ms = msg->zerofill_sections;
		    while(ms != NULL){
			next_ms = ms->next;
			free(ms);
			ms = next_ms;
		    }
		    next_msg = msg->next;
		    free(msg);
		    msg = next_msg;
		}
		break;
	    }
	    else{
		/*
		 * This segment first comes from other than the current set
		 * so check to see in any of it's sections from from the 
		 * current set and if so remove them.  Again advantage of the
		 * order is taken so that if a section if found to come from
		 * the current set all remaining sections in that list also come
		 * from that set.
		 */
		for(ms = msg->content_sections; ms != NULL; ms = ms->next){
		    if(ms->set_num == cur_set){
			if(ms == msg->content_sections)
			    msg->content_sections = NULL;
			else
			    prev_ms->next = NULL;
			while(ms != NULL){
			    msg->sg.nsects--;
			    if(ms->literal_data != NULL)
				free(ms->literal_data);
			    next_ms = ms->next;
			    free(ms);
			    ms = next_ms;
			}
			break;
		    }
		    prev_ms = ms;
		}
		for(ms = msg->zerofill_sections; ms != NULL; ms = ms->next){
		    if(ms->set_num == cur_set){
			if(ms == msg->zerofill_sections)
			    msg->zerofill_sections = NULL;
			else
			    prev_ms->next = NULL;
			while(ms != NULL){
			    msg->sg.nsects--;
			    next_ms = ms->next;
			    free(ms);
			    ms = next_ms;
			}
			break;
		    }
		    prev_ms = ms;
		}
	    }
	    prev_msg = msg;
	}
}
#endif RLD

#ifdef DEBUG
/*
 * print_merged_sections() prints the merged section table.  For debugging.
 */
void
print_merged_sections(
char *string)
{
    struct merged_segment *msg;
    struct merged_section *ms;

	print("Merged section list (%s)\n", string);
	for(msg = merged_segments; msg ; msg = msg->next){
	    print("    Segment %0.16s\n", msg->sg.segname);
	    print("\tcmd %d\n", msg->sg.cmd);
	    print("\tcmdsize %d\n", msg->sg.cmdsize);
	    print("\tvmaddr 0x%x ", msg->sg.vmaddr);
	    print("(addr_set %s)\n", msg->addr_set ? "TRUE" : "FALSE");
	    print("\tvmsize 0x%x\n", msg->sg.vmsize);
	    print("\tfileoff %d\n", msg->sg.fileoff);
	    print("\tfilesize %d\n", msg->sg.filesize);
	    print("\tmaxprot ");
	    print_prot(msg->sg.maxprot);
	    print(" (prot_set %s)\n", msg->prot_set ? "TRUE" : "FALSE");
	    print("\tinitprot ");
	    print_prot(msg->sg.initprot);
	    print("\n");
	    print("\tnsects %d\n", msg->sg.nsects);
	    print("\tflags %d\n", msg->sg.flags);
#ifdef RLD
	    print("\tset_num %d\n", msg->set_num);
#endif RLD
	    print("\tfilename %s\n", msg->filename);
	    print("\tcontent_sections\n");
	    for(ms = msg->content_sections; ms ; ms = ms->next){
		print("\t    Section (%0.16s,%0.16s)\n",
		       ms->s.segname, ms->s.sectname);
		print("\t\taddr 0x%x\n", ms->s.addr);
		print("\t\tsize %d\n", ms->s.size);
		print("\t\toffset %d\n", ms->s.offset);
		print("\t\talign %d\n", ms->s.align);
		print("\t\tnreloc %d\n", ms->s.nreloc);
		print("\t\treloff %d\n", ms->s.reloff);
		print("\t\tflags %s\n", section_flags[ms->s.flags]);
#ifdef RLD
		print("\t\tset_num %d\n", ms->set_num);
#endif RLD
		if(ms->relocated == TRUE)
    		    print("\t    relocated TRUE\n");
		else
    		    print("\t    relocated FALSE\n");
		if(ms->referenced == TRUE)
    		    print("\t    referenced TRUE\n");
		else
    		    print("\t    referenced FALSE\n");
		if(ms->contents_filename){
		    print("\t    contents_filename %s\n",
			   ms->contents_filename);
		    print("\t    file_addr 0x%x\n", ms->file_addr);
		    print("\t    file_size %d\n", ms->file_size);
		}
		if(ms->order_filename){
		    print("\t    order_filename %s\n",
			   ms->order_filename);
		    print("\t    order_addr 0x%x\n", ms->order_addr);
		    print("\t    order_size %d\n", ms->order_size);
		}
		if(ms->s.flags == S_CSTRING_LITERALS)
		    print_cstring_data(ms->literal_data, "\t    ");
		if(ms->s.flags == S_4BYTE_LITERALS)
		    print_literal4_data(ms->literal_data, "\t    ");
		if(ms->s.flags == S_8BYTE_LITERALS)
		    print_literal8_data(ms->literal_data, "\t    ");
		if(ms->s.flags == S_LITERAL_POINTERS)
		    print_literal_pointer_data(ms->literal_data, "\t    ");
	    }
	    print("\tzerofill_sections\n");
	    for(ms = msg->zerofill_sections; ms ; ms = ms->next){
		print("\t    Section (%0.16s,%0.16s)\n",
		       ms->s.segname, ms->s.sectname);
		print("\t\taddr 0x%x\n", ms->s.addr);
		print("\t\tsize %d\n", ms->s.size);
		print("\t\toffset %d\n", ms->s.offset);
		print("\t\talign %d\n", ms->s.align);
		print("\t\tnreloc %d\n", ms->s.nreloc);
		print("\t\treloff %d\n", ms->s.reloff);
		print("\t\tflags %s\n", section_flags[ms->s.flags]);
#ifdef RLD
		print("\t\tset_num %d\n", ms->set_num);
#endif RLD
	    }
	}
}

/*
 * print_merged_section_stats() prints the stats for the merged sections.
 * For tuning..
 */
void
print_merged_section_stats(void)
{
    struct merged_segment *msg;
    struct merged_section *ms;

	for(msg = merged_segments; msg ; msg = msg->next){
	    for(ms = msg->content_sections; ms ; ms = ms->next){
		if(ms->s.flags == S_LITERAL_POINTERS)
		    literal_pointer_data_stats(ms->literal_data, ms);
		else if(ms->s.flags == S_CSTRING_LITERALS)
		    cstring_data_stats(ms->literal_data, ms);
		else if(ms->s.flags == S_4BYTE_LITERALS)
		    literal4_data_stats(ms->literal_data, ms);
		else if(ms->s.flags == S_8BYTE_LITERALS)
		    literal8_data_stats(ms->literal_data, ms);
	    }
	}
}

/*
 * print_load_order() prints the load_order array passed to it.
 * For debugging.
 */
static
void
print_load_order(
struct load_order *load_order,
long nload_order,
struct merged_section *ms,
struct object_file *object_file,
char *string)
{
    long i;

	print("Load order 0x%x %d entries for (%0.16s,%0.16s) of ",
	      load_order, nload_order, ms->s.segname, ms->s.sectname);
	print_obj_name(object_file);
	print("(%s)\n", string);
	for(i = 0; i < nload_order; i++){
	    print("entry[%d]\n", i);
	    print("           name %s\n", load_order[i].name == NULL ? "null" :
		  load_order[i].name);
	    print("          value 0x%08x\n", load_order[i].value);
	    print("          order %d\n", load_order[i].order);
	    print("   input_offset %d\n", load_order[i].input_offset);
	    print("     input_size %d\n", load_order[i].input_size);
	    print("  output_offset %d\n", load_order[i].output_offset);
	}
}

/*
 * print_name_arrays() prints the sorted arrays of archive and object names.
 * For debugging.
 */
void
print_name_arrays(void)
{
    long i, j;

	print("Sorted archive names:\n");
	for(i = 0; i < narchive_names; i++){
	    print("    archive name %s\n", archive_names[i].archive_name);
	    print("    number of objects %d\n", archive_names[i].nobject_names);
	    print("    Sorted object names:\n");
	    for(j = 0; j < archive_names[i].nobject_names; j++){
		print("\tobject name %s\n",
		      archive_names[i].object_names[j].object_name);
		print("\tlength %d\n",
		      archive_names[i].object_names[j].index_length);
		print("\tobject file 0x%x ",
		      archive_names[i].object_names[j].object_file);
		print_obj_name(archive_names[i].object_names[j].object_file);
		print("\n");
	    }
	}
	print("Sorted object names:\n");
	for(j = 0; j < nobject_names; j++){
	    print("\tobject name %s\n", object_names[j].object_name);
	    print("\tindex %d\n", object_names[j].index_length);
	    print("\tobject file 0x%x ", object_names[j].object_file);
	    print_obj_name(object_names[j].object_file);
	    print("\n");
	}
}

static
void
print_load_symbol_hash_table(void)
{
    long i;
    struct load_symbol *load_symbol, *other_name;

	print("load_symbol_hash_table:\n");
	if(load_symbol_hashtable == NULL)
	    return;
	for(i = 0; i < LOAD_SYMBOL_HASHTABLE_SIZE; i++){
	    if(load_symbol_hashtable[i] != NULL)
		print("[%d]\n", i);
	    for(load_symbol = load_symbol_hashtable[i]; 
	        load_symbol != NULL;
	        load_symbol = load_symbol->next){
		print("load symbol:\n", i);
		if(load_symbol->archive_name != NULL){
		    print("    (%s:%s:%s) length %d\n",
			  load_symbol->archive_name,
			  load_symbol->object_name,
			  load_symbol->symbol_name,
			  load_symbol->index_length);
		}
		else{
		    print("    (%s:%s) index %d\n",
			  load_symbol->object_name,
			  load_symbol->symbol_name,
			  load_symbol->index_length);
		}
		print("    load_order 0x%x\n", load_symbol->load_order);
		print("    other_names 0x%x\n", load_symbol->other_names);
		print("    next 0x%x\n", load_symbol->next);
		for(other_name = load_symbol->other_names;
		    other_name != NULL;
		    other_name = other_name->other_names){
		    print("other name\n");
		    if(other_name->archive_name != NULL){
			print("    (%s:%s:%s) length %d\n",
			      other_name->archive_name,
			      other_name->object_name,
			      other_name->symbol_name,
			      other_name->index_length);
		    }
		    else{
			print("    (%s:%s) index %d\n",
			      other_name->object_name,
			      other_name->symbol_name,
			      other_name->index_length);
		    }
		    print("    load_order 0x%x\n", other_name->load_order);
		    print("    other_names 0x%x\n", other_name->other_names);
		    print("    next 0x%x\n", load_symbol->next);
		}
	    }
	}
}
#endif DEBUG

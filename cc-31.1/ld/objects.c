#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines to manage the table of object files to be
 * loaded.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mach.h>
#include <sys/loader.h>
#include <nlist.h>
#include <ar.h>

#include "ld.h"
#include "objects.h"
#include "sections.h"
#include "symbols.h"
#include "sets.h"

/*
 * The head of the object file list and the total count of all object files
 * in the list.  The number objects is only used in main() to tell if there
 * had been any objects loaded into the output file.
 */
struct object_list *objects = NULL;
long nobjects = 0;

/*
 * A pointer to the current object being processed in pass1 or pass2.
 */
struct object_file *cur_obj = NULL;

/*
 * A pointer to the base object for an incremental link if not NULL.
 */
struct object_file *base_obj = NULL;

/*
 * new_object_file() returns a pointer to the next available object_file
 * structrure.  The object_file structure is allways zeroed.
 */
struct object_file *
new_object_file(void)
{
    struct object_list *object_list, **p;
    struct object_file *object_file;

	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    if(object_list->used == NOBJECTS)
		continue;
	    object_file = &(object_list->object_files[object_list->used]);
	    object_list->used++;
	    nobjects++;
#ifdef RLD
	    object_file->set_num = cur_set;
#endif RLD
	    return(object_file);
	}
	*p = allocate(sizeof(struct object_list));
	object_list = *p;
	memset(object_list, '\0', sizeof(struct object_list));
	object_file = &(object_list->object_files[object_list->used]);
	object_list->used++;
	nobjects++;
#ifdef RLD
	object_file->set_num = cur_set;
#endif RLD
	return(object_file);
}

/*
 * add_last_object_file() adds the specified object file to the end of the
 * object file list.
 */
struct object_file *
add_last_object_file(
struct object_file *new_object)
{
    struct object_file *last_object;

	last_object = new_object_file();
	*last_object = *new_object;
	return(last_object);
}

/*
 * remove_last_object_file() removes the specified object file from the end of
 * the object file list.
 */
void
remove_last_object_file(
struct object_file *last_object)
{
    struct object_list *object_list, **p;
    struct object_file *object_file;

	object_file = NULL;
	object_list = NULL;
	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    object_file = &(object_list->object_files[object_list->used - 1]);
	    if(object_list->used == NOBJECTS)
		continue;
	}
	if(object_file == NULL || object_file != last_object)
	    fatal("internal error: remove_last_object_file() called with "
		  "object file that was not the last object file");
	memset(object_file, '\0', sizeof(struct object_file));
	object_list->used--;
}

/*
 * Print the name of the specified object structure in the form: "filename " or
 * "archive(member) ".  In the second case care is taken to trim the blanks from
 * the member name.
 */
void
print_obj_name(
struct object_file *obj)
{
    long i;

	if(obj->ar_hdr){
	    i = sizeof(obj->ar_hdr->ar_name) - 1;
	    if(obj->ar_hdr->ar_name[i] == ' '){
		do{
		    if(obj->ar_hdr->ar_name[i] != ' ')
			break;
		    i--;
		}while(i > 0);
	    }
	    print("%s(%0.*s) ", obj->file_name, i+1, obj->ar_hdr->ar_name);
	}
	else
	    print("%s ", obj->file_name);
}

/*
 * obj_member_name() returns a pointer to a static buffer that contains the name
 * of the archive member as a null terminated string.  Since this is a pointer
 * to a static buffer the contents must be copied to be relied apon.
 */
char *
obj_member_name(
struct object_file *obj)
{
    long i;
    static char member_name[sizeof(obj->ar_hdr->ar_name) + 1] = { 0 };

	memcpy(member_name, obj->ar_hdr->ar_name, sizeof(obj->ar_hdr->ar_name));
	if(obj->ar_hdr){
	    i = sizeof(obj->ar_hdr->ar_name) - 1;
	    if(obj->ar_hdr->ar_name[i] == ' '){
		do{
		    if(obj->ar_hdr->ar_name[i] != ' ')
			break;
		    i--;
		}while(i > 0);
	    }
	    member_name[i + 1] = '\0';
	    return(member_name);
	}
	else{
	    fatal("internal error: obj_member_name() call with an argument "	
		  "that is not in an archive");
	    return(NULL); /* to prevent a compiler warning */
	}
}

/*
 * print_whatsloaded() prints which object files are loaded.  This has to be
 * called after pass1 to get the correct result.
 */
void
print_whatsloaded(void)
{
    long i, j;
    struct object_list *object_list, **p;
    struct object_file *obj;

	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    for(i = 0; i < object_list->used; i++){
		obj = &(object_list->object_files[i]);
		if(obj->ar_hdr){
		    for(j = 0; j < sizeof(obj->ar_hdr->ar_name); j++){
			if(obj->ar_hdr->ar_name[j] == ' ')
			    break;
		    }
		    print("%s(%0.*s)\n",obj->file_name,
			  j, obj->ar_hdr->ar_name);
		}
		else
		    print("%s\n", obj->file_name);
	    }
	}
}

/*
 * fine_reloc_output_offset() returns the output offset for the specified 
 * input offset and the section map using the fine relocation entries.
 */
unsigned long
fine_reloc_output_offset(
struct section_map *map,
unsigned long input_offset)
{
    int l = 0;
    int u = map->nfine_relocs - 1;
    int m;
    int r;

	if(map->nfine_relocs == 0)
	    fatal("internal error, fine_reloc_output_offset() called with a "
		  "section_map->nfine_relocs == 0");
	l = 0;
	m = 0;
	u = map->nfine_relocs - 1;
	while(l <= u){
	    m = (l + u) / 2;
	    if((r = (input_offset - map->fine_relocs[m].input_offset)) == 0)
		return(map->fine_relocs[m].output_offset);
	    else if (r < 0)
		u = m - 1;
	    else
		l = m + 1;
	}
	if(m == 0 || input_offset > map->fine_relocs[m].input_offset)
	    return(map->fine_relocs[m].output_offset +
		    input_offset - map->fine_relocs[m].input_offset);
	else
	    return(map->fine_relocs[m-1].output_offset +
		    input_offset - map->fine_relocs[m-1].input_offset);
}

#ifdef RLD
/*
 * clean_objects() does two things.  For each object file in the current set
 * it first it deallocates the memory used for the object file.  Then it sets
 * the pointer to the section in each section map to point at the merged section
 * so it still can be used by trace_symbol() on future rld_load()'s (again only
 * for object files in the current set).
 */
void
clean_objects(void)
{
    long i, j;
    struct object_list *object_list, **p;
    struct object_file *object_file;
    kern_return_t r;

	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    for(i = 0; i < object_list->used; i++){
		object_file = &(object_list->object_files[i]);
		if(object_file->set_num != cur_set)
		    continue;
		if(object_file->ar_hdr == NULL &&
		   object_file->obj_size != 0 &&
		   object_file->user_obj_addr == FALSE){
		    if((r = vm_deallocate(task_self(),
				  (vm_address_t)object_file->obj_addr,
				  object_file->obj_size)) != KERN_SUCCESS)
			mach_fatal(r, "can't vm_deallocate() memory for "
				   "mapped file %s",object_file->file_name);
		}
		object_file->obj_addr = NULL;
		object_file->obj_size = 0;
	        object_file->user_obj_addr = FALSE;

		/*
		 * Since the tracing of symbols and the creation of the common
		 * section both use the section's segname and sectname feilds
		 * these need to still be valid after the memory for the file
		 * has been deallocated.  So just set the pointer to point at
		 * the merged section.
		 */
		if(object_file->section_maps != NULL){
		    for(j = 0; j < object_file->nsection_maps; j++){
			object_file->section_maps[j].s = 
			    &(object_file->section_maps[j].output_section->s);
		    }
		}
	    }
	}
}

/*
 * remove_objects() removes the object structures that are from the
 * current object file set.  This takes advantage of the fact
 * that objects from the current set come after the previous set.
 */
void
remove_objects(void)
{
    long i, removed;
    /* The compiler "warning: `prev_object_list' may be used uninitialized in */
    /* this function" can safely be ignored */
    struct object_list *object_list, *prev_object_list, *next_object_list;
    struct object_file *object_file;

	/* The compiler "warning: `prev_object_list' may be used */
	/* uninitialized in this function" can safely be ignored */
	prev_object_list = NULL;

	for(object_list = objects;
	    object_list != NULL;
	    object_list = object_list->next){
	    removed = 0;
	    for(i = 0; i < object_list->used; i++){
		object_file = &(object_list->object_files[i]);
		if(object_file->set_num == cur_set){
		    if(cur_set != -1)
			free(object_file->file_name);
		    if(object_file->section_maps != NULL)
			free(object_file->section_maps);
		    if(cur_obj->undefined_maps != NULL)
			free(cur_obj->undefined_maps);
		    memset(object_file, '\0', sizeof(struct object_file));
		    removed++;
		}
	    }
	    object_list->used -= removed;
	    nobjects -= removed;
	}
	/*
	 * Find the first object list that now has 0 entries used.
	 */
	for(object_list = objects;
	    object_list != NULL;
	    object_list = object_list->next){
	    if(object_list->used == 0)
		break;
	    prev_object_list = object_list;
	}
	/*
	 * If there are any object lists with 0 entries used free them.
	 */
	if(object_list != NULL && object_list->used == 0){
	    /*
	     * First set the pointer to this list in the previous list to
	     * NULL.
	     */
	    if(object_list == objects)
		objects = NULL;
	    else
		prev_object_list->next = NULL;
	    /*
	     * Now free this list and do the same for all remaining lists.
	     */
	    do {
		next_object_list = object_list->next;
		free(object_list);
		object_list = next_object_list;
	    }while(object_list != NULL);
	}
}
#endif RLD

#ifdef DEBUG
/*
 * print_object_list() prints the object table.  Used for debugging.
 */
void
print_object_list(void)
{
    long i, j, k;
    struct object_list *object_list, **p;
    struct object_file *object_file;
    struct fine_reloc *fine_relocs;

	print("Object file list\n");
	for(p = &objects; *p; p = &(object_list->next)){
	    object_list = *p;
	    print("    object_list 0x%x\n", object_list);
	    print("    used %d\n", object_list->used);
	    print("    next 0x%x\n", object_list->next);
	    for(i = 0; i < object_list->used; i++){
		object_file = &(object_list->object_files[i]);
		print("\tfile_name %s\n", object_file->file_name);
		print("\tobj_addr 0x%x\n", object_file->obj_addr);
		print("\tobj_size %d\n", object_file->obj_size);
    		print("\tar_hdr 0x%x", object_file->ar_hdr);
    		if(object_file->ar_hdr != NULL)
		    print(" (%0.12s)\n", object_file->ar_hdr);
		else
		    print("\n");
    		print("\tnsection_maps %d\n", object_file->nsection_maps);
		for(j = 0; j < object_file->nsection_maps; j++){
		    print("\t    (%s,%s)\n",
			   object_file->section_maps[j].s->segname,
			   object_file->section_maps[j].s->sectname);
		    print("\t    offset 0x%x\n",
			   object_file->section_maps[j].offset);
		    print("\t    fine_relocs 0x%x\n",
			   object_file->section_maps[j].fine_relocs);
		    print("\t    nfine_relocs %d\n",
			   object_file->section_maps[j].nfine_relocs);
		    fine_relocs = object_file->section_maps[j].fine_relocs;
		    for(k = 0;
			k < object_file->section_maps[j].nfine_relocs;
			k++){
			print("\t\t%-6d %-6d\n",
			       fine_relocs[k].input_offset,
			       fine_relocs[k].output_offset);
		    }
		}
    		print("\tnundefineds %d\n", object_file->nundefineds);
		for(j = 0; j < object_file->nundefineds; j++){
		    print("\t    (%d,%s)\n",
			   object_file->undefined_maps[j].index,
			   object_file->undefined_maps[j].merged_symbol->nlist.
								n_un.n_name);
		}
#ifdef RLD
		print("\tset_num = %d\n", object_file->set_num);
#endif RLD
	    }
	}
}
#endif DEBUG

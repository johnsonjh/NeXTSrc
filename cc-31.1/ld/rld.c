#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
#ifdef RLD
/*
 * This file contains the three functions of the RLD package.
 */
#include <libc.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/loader.h>
#include <nlist.h>
#include <mach.h>
#include <streams/streams.h>

#include "ld.h"
#include "objects.h"
#include "sections.h"
#include "symbols.h"
#include "pass1.h"
#include "layout.h"
#include "pass2.h"
#include "rld.h"
#include "sets.h"

extern struct segment_command *getsegbyname(const char *segname);

/*
 * The user's address function to be called in layout to get the address of
 * where to link edit the result.
 */
unsigned long (*address_func)(unsigned long size, unsigned long headers_size) =
									   NULL;

/*
 * The stream passed in to the rld routines to print errors on.
 */
static NXStream *error_stream = NULL;

/*
 * The jump buffer to get back to rld_load() or rld_unload() used by the error
 * routines.
 */
static jmp_buf rld_env = { 0 };

/*
 * Indicator that a fatal error occured and that no more processing will be
 * done on all future calls to protect calls from causing a core dump.
 */
static volatile int fatals = 0;

/*
 * The base file name passed to rld_load_basefile() if it has been called.
 * This points at an allocated copy of the name.
 */
static char *base_name = NULL;

/* The internal routine that implements rld_load()'s */
static long internal_rld_load(
    NXStream *stream,
    struct mach_header **header_addr,
    const char * const *object_filenames,
    const char *output_filename,
    const char *file_name,
    const char *obj_addr,
    long obj_size);

/*
 * rld_load() link edits and loads the specified object filenames in the NULL
 * terminated array of object file names, object_files, into the program that
 * called it.  If the program wishes to allow the loaded object files to use
 * symbols from itself it must be built with the -seglinkedit link editor
 * option to have its symbol table mapped into memory.  The symbol table may
 * be trimed to exactly which symbol are allowed to be referenced by use of the
 * '-s list_filenam' option to strip(1).  For this routine only global symbols
 * are used so the -x option to the link editor or strip(1) can be used to save
 * space in the final program.  The set of object files being loaded will only
 * be successfull if there are no link edit errors (undefined symbols, etc.).
 * If an error ocurrs the set of object files is unloaded automaticly.  If
 * errors occur and the value specified for stream is not NULL error messages
 * are printed in that stream.  If the link editing and loading is successfull
 * the address of the header of what was loaded is returned through the pointer
 * header_addr it if is not NULL.  rld_load() returns 1 for success and 0 for
 * failure.  If a fatal system error (out of memory, etc.) occurs then all
 * future calls will fail.
 */
long
rld_load(
NXStream *stream,
struct mach_header **header_addr,
const char * const *object_filenames,
const char *output_filename)
{
	return(internal_rld_load(stream, header_addr, object_filenames,
				 output_filename, NULL, NULL, 0));
}

/*
 * rld_load_from_memory() is the same as rld_load() but loads one object file
 * that has been mapped into memory.  The object is described by its name,
 * object_name, at address object_addr and is of size object_size.
 */
long
rld_load_from_memory(
NXStream *stream,
struct mach_header **header_addr,
const char *object_name,
char *object_addr,
long object_size,
const char *output_filename)
{
	return(internal_rld_load(stream, header_addr, NULL, output_filename,
				 object_name, object_addr, object_size));
}

/*
 * internal_rld_load() is the internal routine that implements rld_load()'s.
 */
static
long
internal_rld_load(
NXStream *stream,
struct mach_header **header_addr,
const char * const *object_filenames,
const char *output_filename,
const char *file_name,
char *obj_addr,
long obj_size)
{
    int i, fd;
    struct segment_command *sg;
    kern_return_t r;
    long symbol_size, deallocate_size;

	error_stream = stream;

	if(header_addr != NULL)
	    *header_addr = NULL;

	/* If a fatal error has ever occured no other calls will be processed */
	if(fatals == 1){
	    print("previous fatal errors occured, can no longer succeed");
	    return(0);
	}

	/*
	 * Set up and handle link edit errors and fatal errors
	 */
	if(setjmp(rld_env) != 0){
	    /*
	     * It takes a longjmp() to get to this point.  If it was not a fatal
	     * error unload the set of object files being loaded.  Otherwise
	     * just return failure.
	     */
	    if(fatals == 0)
		rld_unload(stream);
	    return(0);
	}

	/* Set up the globals for rld */
	progname = "rld()";
	host_pagesize = getpagesize();
	filetype = MH_OBJECT;
	flush = FALSE;
	nmerged_symbols = 0;
	merged_string_size = 0;
	nlocal_symbols = 0;
	local_string_size = 0;
	/*
	 * If there is to be an output file then save the symbols.  Only the
	 * symbols from the current set will be placed in the output file.  The
	 * symbols from the base file are never placed in any output file.
	 */
	strip_base_symbols = TRUE;
	if(output_filename != NULL)
	    strip_level = STRIP_NONE;
	else
	    strip_level = STRIP_ALL;

	/* This must be cleared for each call to rld() */
	errors = 0;

	/*
	 * If the symbols from base program has not been loaded load them.
	 * This will happen the first time rld() is called or will not happen.
	 */
	if(base_obj == NULL){
	    sg = getsegbyname(SEG_LINKEDIT);
	    if(sg != NULL)
		merge_base_program(sg);
	    /*
	     * If there were any errors in processing the base program it is
	     * treated as a fatal error and no futher processing is done.
	     */
	    if(errors){
		fatals = 1;
		return(0);
	    }
	}

	/*
	 * Create an entry in the sets array for this new set.  This has to be
	 * done after the above base program has been merged so it does not
	 * appear apart of any set.
	 */
	new_set();

	/*
	 * The merged section sizes need to be zeroed before we start loading.
	 * The only case they would not be zero would be if a previous rld_load
	 * failed with a pass1 error they would not get reset.
	 */
	zero_merged_sections_sizes();

	/*
	 * Do pass1() for each object file or merge() for the one object in
	 * memory.
	 */
	if(file_name == NULL){
	    for(i = 0; object_filenames[i] != NULL; i++)
		pass1((char *)object_filenames[i], FALSE, FALSE);
	}
	else{
	    cur_obj = new_object_file();
	    cur_obj->file_name = allocate(strlen(file_name) + 1);
	    strcpy(cur_obj->file_name, file_name);
	    cur_obj->user_obj_addr = TRUE;
	    cur_obj->obj_addr = obj_addr;
	    cur_obj->obj_size = obj_size;
	    merge();
	}

	if(errors){
	    rld_unload(stream);
	    return(0);
	}

	layout();
	if(errors){
	    rld_unload(stream);
	    return(0);
	}

	pass2();
	if(errors){
	    rld_unload(stream);
	    return(0);
	}

	/*
	 * Place the merged sections back on their list of their merged segment
	 * (since now the are all in one segment after layout() placed them
	 * there for the MH_OBJECT format) and also reset the sizes of the 
	 * sections to zero for any future loads.
	 */
	reset_merged_sections();

	/*
	 * Clean the object structures of things from this set that are not
	 * needed once the object has been successfully loaded.
	 */
	clean_objects();
	clean_archives();

	if(output_filename != NULL){
	    /*
	     * Create the output file.  The unlink() is done to handle the
	     * problem when the outputfile is not writable but the directory
	     * allows the file to be removed (since the file may not be there
	     * the return code of the unlink() is ignored).
	     */
	    symbol_size = output_symtab_info.symtab_command.nsyms *
			  sizeof(struct nlist) +
			  output_symtab_info.symtab_command.strsize;
	    (void)unlink(output_filename);
	    if((fd = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC,
			  0666)) == -1){
		system_error("can't create output file: %s", output_filename);
	    }
	    else {
		/*
		 * Write the entire output file.
		 */
		if(write(fd, output_addr, output_size + symbol_size) !=
		   output_size + symbol_size){
		    system_error("can't write output file: %s",output_filename);
		}
		if(close(fd) == -1){
		    system_error("can't close output file: %s",output_filename);
		}
	    }
	    /*
	     * Deallocate the pages of memory for the symbol table if there are
	     * any whole pages.
	     */
	    deallocate_size = round(output_size + symbol_size, host_pagesize) -
			      round(output_size, host_pagesize);
	    if(deallocate_size > 0)
		if((r = vm_deallocate(task_self(), (vm_address_t)(output_addr +
				      round(output_size, host_pagesize)),
				      deallocate_size)) != KERN_SUCCESS)
		    mach_fatal(r, "can't vm_deallocate() buffer for output "
			       "file's symbol table");
	}

	/*
	 * Now that this was successfull all that is left to do is return the
	 * address of the header if requested.
	 */
	if(header_addr != NULL)
	    *header_addr = (struct mach_header *)output_addr;

	return(1);
}

/*
 * rld_load_basefile() loads a base file from an object file rather than just
 * picking up the link edit segment from this program.
 */
long
rld_load_basefile(
NXStream *stream,
const char *base_filename)
{
    long size;
    char *addr;
    int fd;
    struct stat stat_buf;
    kern_return_t r;

	error_stream = stream;

	/* If a fatal error has ever occured no other calls will be processed */
	if(fatals == 1){
	    print("previous fatal errors occured, can no longer succeed");
	    return(0);
	}

	/*
	 * Set up and handle link edit errors and fatal errors
	 */
	if(setjmp(rld_env) != 0){
	    /*
	     * It takes a longjmp() to get to this point.  If it was not a fatal
	     * error unload the base file being loaded.  Otherwise just return
	     * failure.
	     */
	    if(fatals == 0)
		rld_unload_all(stream, 1);
	    return(0);
	}

	/* This must be cleared for each call to rld() */
	errors = 0;

	/*
	 * If a base file has been loaded at this point return failure.
	 */
	if(base_obj != NULL){
	    error("a base program is currently loaded");
	    return(0);
	}
	if(cur_set != -1){
	    error("object sets are currently loaded (base file must be loaded"
		  "before object sets)");
	    return(0);
	}

	/* Set up the globals for rld */
	progname = "rld()";
	host_pagesize = getpagesize();
	strip_base_symbols = TRUE;
	base_name = allocate(strlen(base_filename) + 1);
	strcpy(base_name, base_filename);

	/*
	 * If there is to be an output file then save the symbols.  Only the
	 * symbols from the current set will be placed in the output file.  The
	 * symbols from the base file are never placed in any output file.
	 */

	/*
	 * Open this file and map it in.
	 */
	if((fd = open(base_name, O_RDONLY, 0)) == -1){
	    system_error("Can't open: %s", base_name);
	    free(base_name);
	    base_name = NULL;
	    return(0);
	}
	if(fstat(fd, &stat_buf) == -1)
	    system_fatal("Can't stat file: %s", base_name);
	/*
	 * For some reason mapping files with zero size fails so it has to
	 * be handled specially.
	 */
	if(stat_buf.st_size == 0){
	    error("file: %s is empty (not an object)", base_name);
	    close(fd);
	    free(base_name);
	    base_name = NULL;
	    return(0);
	}
	size = stat_buf.st_size;
	if((r = map_fd((int)fd, (vm_offset_t)0, (vm_offset_t *)&addr,
	    (boolean_t)TRUE, (vm_size_t)size)) != KERN_SUCCESS)
	    mach_fatal(r, "can't map file: %s", base_name);
	if((r = vm_protect(task_self(), (vm_address_t)addr, size, FALSE,
			   VM_PROT_READ)) != KERN_SUCCESS)
	    mach_fatal(r, "can't make memory for mapped file: %s "
		       "read-only", base_name);
	close(fd);

	/*
	 * Now that the file is mapped in merge it as the base file.
	 */
	cur_obj = new_object_file();
	cur_obj->file_name = base_name;
	cur_obj->obj_addr = addr;
	cur_obj->obj_size = size;
	base_obj = cur_obj;

	merge();

	if(errors){
	    rld_unload_all(stream, 1);
	    return(0);
	}

	/*
	 * This is called to deallocate the memory for the base file and to
	 * clean up it's section map.
	 */
	clean_objects();
	clean_archives();

	return(1);
}

/*
 * rld_unload() unlinks and unloads that last object set that was loaded.
 * It returns 1 if it is successfull and 0 otherwize.  If any errors ocurr
 * and the specified stream, stream, is not zero the error messages are printed
 * on that stream.
 */
long
rld_unload(
NXStream *stream)
{
    kern_return_t r;

	error_stream = stream;

	/* If a fatal error has ever occured no other calls will be processed */
	if(fatals == 1){
	    print("previous fatal errors occured, can no longer succeed");
	    return(0);
	}

	/*
	 * Set up and handle link edit errors and fatal errors
	 */
	if(setjmp(rld_env) != 0){
	    /*
	     * It takes a longjmp() to get to this point.  If it was a fatal
	     * error or not just return failure.
	     */
	    return(0);
	}

	/* Set up the globals for rld */
	progname = "rld()";

	/* This must be cleared for each call to rld() */
	errors = 0;

	free_multiple_defs();
	free_undefined_list();

	/*
	 * If no set has been loaded at this point return failure.
	 */
	if(cur_set == -1){
	    error("no object sets currently loaded");
	    return(0);
	}

	/*
	 * Remove the merged symbols for the current set of objects.
	 */
	remove_merged_symbols();

	/*
	 * Remove the merged sections for the current set of objects.
	 */
	remove_merged_sections();

	/*
	 * Clean and remove the object strcutures for the current set of
	 * objects.
	 */
	clean_objects();
	clean_archives();
	remove_objects();

	/*
	 * deallocate the output memory for the current set if it had been
	 * allocated.
	 */
	if(sets[cur_set].output_addr != NULL){
	    if((r = vm_deallocate(task_self(),
				  (vm_address_t)sets[cur_set].output_addr,
				  sets[cur_set].output_size)) != KERN_SUCCESS)
		mach_fatal(r, "can't vm_deallocate() memory for output");
	    sets[cur_set].output_addr = NULL;
	}

	/*
	 * The very last thing to do to unload a set is to remove the set
	 * allocated in the sets array and reduce the cur_set.
	 */
	remove_set();

	return(1);
}

/*
 * rld_unload_all() frees up all dynamic memory for the rld package that store
 * the information about all object sets and the base program.  Also if the
 * parameter deallocate_sets is non-zero it deallocates the object sets
 * otherwise it leaves them around and can be still be used by the program.
 * It returns 1 if it is successfull and 0 otherwize.  If any errors ocurr
 * and the specified stream, stream, is not zero the error messages are printed
 * on that stream.
 */
long
rld_unload_all(
NXStream *stream,
long deallocate_sets)
{
    kern_return_t r;

	error_stream = stream;

	/* If a fatal error has ever occured no other calls will be processed */
	if(fatals == 1){
	    print("previous fatal errors occured, can no longer succeed");
	    return(0);
	}

	/*
	 * Set up and handle link edit errors and fatal errors
	 */
	if(setjmp(rld_env) != 0){
	    /*
	     * It takes a longjmp() to get to this point.  If it was a fatal
	     * error or not just return failure.
	     */
	    return(0);
	}

	/* Set up the globals for rld */
	progname = "rld()";

	/* This must be cleared for each call to rld() */
	errors = 0;

	free_multiple_defs();
	free_undefined_list();

	/*
	 * If nothing has been loaded at this point return failure.
	 */
	if(cur_set == -1 && base_obj == NULL){
	    error("no object sets or base program currently loaded");
	    return(0);
	}

	/*
	 * Remove all sets currently loaded.
	 */
	while(cur_set != -1){
	    /*
	     * Remove the merged symbols for the current set of objects.
	     */
	    remove_merged_symbols();

	    /*
	     * Remove the merged sections for the current set of objects.
	     */
	    remove_merged_sections();

	    /*
	     * Clean and remove the object structures for the current set of
	     * objects.
	     */
	    clean_objects();
	    clean_archives();
	    remove_objects();

	    /*
	     * deallocate the output memory for the current set if specified and
	     * it had been allocated.
	     */
	    if(deallocate_sets && sets[cur_set].output_addr != NULL){
		if((r = vm_deallocate(task_self(),
				  (vm_address_t)sets[cur_set].output_addr,
				  sets[cur_set].output_size)) != KERN_SUCCESS)
		    mach_fatal(r, "can't vm_deallocate() memory for output");
	    }
	    sets[cur_set].output_addr = NULL;

	    /*
	     * The very last thing to do to unload a set is to remove the set
	     * allocated in the sets array and reduce the cur_set.
	     */
	    remove_set();
	}
	/*
	 * Remove the merged symbols for the base program.
	 */
	remove_merged_symbols();

	/*
	 * Remove the merged sections for the base program.
	 */
	remove_merged_sections();

	/*
	 * Remove the object structure for the base program.
	 */
	if(base_name != NULL){
	    clean_objects();
	    clean_archives();
	    free(base_name);
	    base_name = NULL;
	}
	remove_objects();

	/*
	 * Now free the memory for the sets.
	 */
	free_sets();

	/*
	 * Set the pointer to the base object to NULL so that if another load
	 * is done it will get reloaded.
	 */
	base_obj = NULL;

	return(1);
}

/*
 * rld_lookup() looks up the specified symbol name, symbol_name, and returns
 * its value indirectly through the pointer specified, value.  It returns
 * 1 if it finds the symbol and 0 otherwise.  If any errors ocurr and the
 * specified stream, stream, is not zero the error messages are printed on
 * that stream (for this routine only internal errors could result).
 */
long
rld_lookup(
NXStream *stream,
const char *symbol_name,
unsigned long *value)
{
    struct merged_symbol **hash_pointer, *merged_symbol;

	error_stream = stream;

	/* If a fatal error has ever occured no other calls will be processed */
	if(fatals == 1){
	    print("previous fatal errors occured, can no longer succeed");
	    return(0);
	}

	/* This must be cleared for each call to rld() */
	errors = 0;

	hash_pointer = lookup_symbol((char *)symbol_name);
	if(*hash_pointer != NULL){
	    merged_symbol = *hash_pointer;
	    if(value != NULL)
		*value = merged_symbol->nlist.n_value;
	    return(1);
	}
	else{
	    if(value != NULL)
		*value = 0;
	    return(0);
	}
}

/*
 * rld_get_current_header() is only used by the objective-C runtime to do
 * unloading to get the current header so it does not have to save this
 * information.  It returns NULL if there is nothing is loaded currently.
 */
/* #private_extern */
char *
rld_get_current_header(
void)
{
	/*
	 * If no set has been loaded at this point return NULL.
	 */
	if(cur_set == -1)
	    return(NULL);
	else
	    return(sets[cur_set].output_addr);
}

/*
 * rld_address_func() is passed a pointer to a function that is then called on
 * subsequent rld_load() calls to get the address that the user wants the object
 * set loaded at.  That function is passed the memory size of the resulting
 * object set.
 */
void
rld_address_func(
unsigned long (*func)(unsigned long size, unsigned long headers_size))
{
	address_func = func;
}

/*
 * All printing of all messages goes through this function.
 */
void
vprint(
const char *format,
va_list ap)
{
	if(error_stream != NULL)
	    NXVPrintf(error_stream, format, ap);
}

/*
 * cleanup() is called by all routines handling fatal errors.
 */
void
cleanup(void)
{
	fatals = 1;
	longjmp(rld_env, 1);
}
#endif RLD

#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines to manage the structures that hold the
 * information for the rld package of the object file sets.
 */
#ifdef RLD
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/loader.h>
#include <mach.h>

#include "ld.h"
#include "objects.h"
#include "sets.h"
/*
 * The ammount to increase the sets array when needed.
 */
#define NSETS_INCREMENT 10

/*
 * A pointer to the array of sets
 */
struct set *sets = NULL;
/*
 * The number of set structures allocated in the above array.
 */
static long nsets = 0;
/*
 * The index into the sets array for the current set.
 */
long cur_set = -1;

/*
 * new_set() allocates another structure for a new set in the sets array and
 * increments the cur_set to index into the sets array for the new set.
 */
void
new_set(void)
{
    long i;

	if(cur_set + 2 > nsets){
	    sets = reallocate(sets,
			      (nsets + NSETS_INCREMENT) * sizeof(struct set));
	    for(i = 0; i < NSETS_INCREMENT; i++){
		memset(sets + nsets + i, '\0', sizeof(struct set));
	    }
	    nsets += NSETS_INCREMENT;
	}
	cur_set++;
}

/*
 * new_archive() is called from pass1() for rld_load and keeps track of the
 * archives a that are mapped so clean_archives can deallocate their memory.
 */
void
new_archive(
char *file_name,
char *file_addr,
long file_size)
{
	sets[cur_set].archives = reallocate(sets[cur_set].archives,
					    (sets[cur_set].narchives + 1) *
					    sizeof(struct archive));

	sets[cur_set].archives[sets[cur_set].narchives].file_name =
						allocate(strlen(file_name) + 1);
	strcpy(sets[cur_set].archives[sets[cur_set].narchives].file_name,
	       file_name);
	sets[cur_set].archives[sets[cur_set].narchives].file_addr = file_addr;
	sets[cur_set].archives[sets[cur_set].narchives].file_size = file_size;
	sets[cur_set].narchives++;
}

/*
 * clean_archives() deallocates any archives that were loaded in the current
 * set.
 */
void
clean_archives(void)
{
    long i;
    kern_return_t r;
    char *file_addr, *file_name;
    long file_size;

	if(sets != NULL && cur_set != -1){
	    for(i = 0; i < sets[cur_set].narchives; i++){
		file_addr = sets[cur_set].archives[i].file_addr;
		file_size = sets[cur_set].archives[i].file_size;
		file_name = sets[cur_set].archives[i].file_name;
		if((r = vm_deallocate(task_self(), (vm_address_t)file_addr,
				      file_size)) != KERN_SUCCESS)
		    mach_fatal(r, "can't vm_deallocate() memory for "
			       "mapped file %s", file_name);
		free(file_name);
	    }
	    if(sets[cur_set].archives != NULL)
		free(sets[cur_set].archives);
	    sets[cur_set].archives = NULL;
	    sets[cur_set].narchives = 0;
	}
}

/*
 * remove_set deallocates the current set structure from the sets array.
 */
void
remove_set(void)
{
	if(cur_set >= 0){
	    sets[cur_set].output_addr = NULL;
	    sets[cur_set].output_size = 0;
	    cur_set--;
	}
}

/*
 * free_sets frees all storage for the sets and resets everything back to the
 * initial state.
 */
void
free_sets(void)
{
	if(sets != NULL)
	    free(sets);

	sets = NULL;
	nsets = 0;
	cur_set = -1;
}
#endif RLD

#ifndef RLD
/*
 * Global types, variables and routines declared in the file fvmlibs.c.
 *
 * The following include file need to be included before this file:
 * #include <sys/loader.h> 
 * #include "ld.h"
 */

struct merged_fvmlib {
    char *fvmlib_name;		/* The name of this fixed VM shared library. */
    struct fvmlib_command *fl;	/* The LC_LOADFVMLIB load command for this */
				/*  fixed VM shared library. */
    struct object_file		/* Pointer to the object file the load */
	*definition_object;	/*  command was found in */
    enum bool multiple;		/* Flag to indicate if this was already */
				/*  loaded from more than one object */
    struct merged_fvmlib *next;	/* The next in the list, NULL otherwise */
};

/* the pointer to the head of the load fixed VM shared library commamds */
extern struct merged_fvmlib *merged_fvmlibs;

/* the pointer to the head of the fixed VM shared library segments */
extern struct merged_segment *fvmlib_segments;

extern void merge_fvmlibs(void);

#ifdef DEBUG
void print_load_fvmlibs_list(void);
void print_fvmlib_segments(void);
#endif DEBUG
#endif !defined(RLD)

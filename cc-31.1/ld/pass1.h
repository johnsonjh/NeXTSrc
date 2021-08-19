/*
 * Global types, variables and routines declared in the file pass1.c.
 *
 * The following include file need to be included before this file:
 * #include "ld.h"
 */

#ifndef RLD
/* the user specified directories to search for -lx filenames, and the number
   of them */
extern char **search_dirs;
extern long nsearch_dirs;

/* the standard directories to search for -lx filenames */
extern char *standard_dirs[];

/* the pointer to the head of the base object file's segments */
extern struct merged_segment *base_obj_segments;
#endif !defined(RLD)

extern void pass1(char *filename, enum bool lname, enum bool base_name);
extern void merge(void);

#ifdef RLD
extern void merge_base_program(struct segment_command *seg_linkedit);
#endif RLD

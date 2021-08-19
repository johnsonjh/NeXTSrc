/*
 * Global types, variables and routines declared in the file cstring_literals.c.
 *
 * The following include files need to be included before this file:
 * #include "ld.h"
 * #include "objects.h"
 */

/*
 * The literal_data which is set into a merged_section's literal_data field for
 * S_CSTRING_LITERALS sections.  The external functions declared at the end of
 * this file operate on this data and are used for the other fields of a
 * merged_section for literals (literal_merge and literal_write).
 */
struct cstring_data {
    struct cstring_bucket **hashtable;		/* the hash table */
    struct cstring_block *cstring_blocks;	/* the cstrings */
#ifdef DEBUG
    long nfiles;	/* number of files with this section */
    long nbytes;	/* total number of bytes in the input files*/
			/*  merged into this section  */
    long ninput_strings;/* number of strings in the input file */
    long noutput_strings;/* number of strings in the output file */
    long nprobes;	/* number of hash probes */
#endif DEBUG
};

/* the number of entries in the hash table */
#define CSTRING_HASHSIZE 1022

/* the hash bucket entries in the hash table points to; allocated as needed */
struct cstring_bucket {
    char *cstring;		/* pointer to the string */
    long offset;		/* offset of this string in the output file */
    struct cstring_bucket *next;/* next in the hash chain */
};

/* the blocks that store the strings; allocated as needed */
struct cstring_block {
    long size;			/* the number of bytes in this block */
    long used;			/* the number of bytes used in this block */
    enum bool full;		/* no more strings are to come from this block*/
    char *cstrings;		/* the strings */
    struct cstring_block *next;	/* the next block */
};

extern void cstring_merge(
    struct cstring_data *data,
    struct merged_section *ms,
    struct section *s,
    struct section_map *section_map);

extern void cstring_order(
    struct cstring_data *data,
    struct merged_section *ms);

extern void get_cstring_from_sectorder(
    struct merged_section *ms,
    long *index,
    char *buffer,
    long line_number,
    long char_pos);

extern long lookup_cstring(
    char *cstring,
    struct cstring_data *data,
    struct merged_section *ms);

extern void cstring_output(
    struct cstring_data *data,
    struct merged_section *ms);

extern void cstring_free(
    struct cstring_data *data);

#ifdef DEBUG
extern void print_cstring_data(
    struct cstring_data *data,
    char *indent);

extern void cstring_data_stats(
    struct cstring_data *data,
    struct merged_section *ms);
#endif DEBUG

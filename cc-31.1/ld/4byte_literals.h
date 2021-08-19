/*
 * Global types, variables and routines declared in the file 4byte_literals.c.
 *
 * The following include files need to be included before this file:
 * #include "ld.h"
 * #include "objects.h"
 */

/*
 * The literal_data which is set into a merged_section's literal_data field for
 * S_4BYTE_LITERALS sections.  The external functions declared at the end of
 * this file operate on this data and are used for the other fields of a
 * merged_section for literals (literal_merge and literal_write).
 */
struct literal4_data {
    struct literal4_block *literal4_blocks;	/* the literal4's */
#ifdef DEBUG
    long nfiles;	/* number of files with this section */
    long nliterals;	/* total number of literals in the input files*/
			/*  merged into this section  */
#endif DEBUG
};

/* the number of entries in the hash table */
#define LITERAL4_BLOCK_SIZE 60

/* The structure to hold an 4 byte literal */
struct literal4 {
    unsigned long long0;
};

/* the blocks that store the liiterals; allocated as needed */
struct literal4_block {
    long used;				/* the number of literals used in */
    struct literal4			/*  this block */
	literal4s[LITERAL4_BLOCK_SIZE];	/* the literals */
    struct literal4_block *next;	/* the next block */
};

extern void literal4_merge(
    struct literal4_data *data,
    struct merged_section *ms,
    struct section *s,
    struct section_map *section_map);

extern void literal4_order(
    struct literal4_data *data,
    struct merged_section *ms);

extern long lookup_literal4(
    struct literal4 literal4,
    struct literal4_data *data,
    struct merged_section *ms);

extern void literal4_output(
    struct literal4_data *data,
    struct merged_section *ms);

extern void literal4_free(
    struct literal4_data *data);

#ifdef DEBUG
extern void print_literal4_data(
    struct literal4_data *data,
    char *indent);

extern void literal4_data_stats(
    struct literal4_data *data,
    struct merged_section *ms);
#endif DEBUG

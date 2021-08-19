/*
 * Global types, variables and routines declared in the file 8byte_literals.c.
 *
 * The following include files need to be included before this file:
 * #include "ld.h"
 * #include "objects.h"
 */

/*
 * The literal_data which is set into a merged_section's literal_data field for
 * S_8BYTE_LITERALS sections.  The external functions declared at the end of
 * this file operate on this data and are used for the other fields of a
 * merged_section for literals (literal_merge and literal_write).
 */
struct literal8_data {
    struct literal8_block *literal8_blocks;	/* the literal8's */
#ifdef DEBUG
    long nfiles;	/* number of files with this section */
    long nliterals;	/* total number of literals in the input files*/
			/*  merged into this section  */
#endif DEBUG
};

/* the number of entries in the hash table */
#define LITERAL8_BLOCK_SIZE 60

/* The structure to hold an 8 byte literal */
struct literal8 {
    unsigned long long0;
    unsigned long long1;
};

/* the blocks that store the liiterals; allocated as needed */
struct literal8_block {
    long used;				/* the number of literals used in */
    struct literal8			/*  this block */
	literal8s[LITERAL8_BLOCK_SIZE];	/* the literals */
    struct literal8_block *next;	/* the next block */
};

extern void literal8_merge(
    struct literal8_data *data,
    struct merged_section *ms,
    struct section *s,
    struct section_map *section_map);

extern void literal8_order(
    struct literal8_data *data,
    struct merged_section *ms);

extern enum bool get_hex_from_sectorder(
    struct merged_section *ms,
    long *index,
    unsigned long *value,
    long line_number);

extern long lookup_literal8(
    struct literal8 literal8,
    struct literal8_data *data,
    struct merged_section *ms);

extern void literal8_output(
    struct literal8_data *data,
    struct merged_section *ms);

extern void literal8_free(
    struct literal8_data *data);

#ifdef DEBUG
extern void print_literal8_data(
    struct literal8_data *data,
    char *indent);

extern void literal8_data_stats(
    struct literal8_data *data,
    struct merged_section *ms);
#endif DEBUG

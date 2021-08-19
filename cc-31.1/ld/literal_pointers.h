/*
 * The literal_data which is set into a merged_section's literal_data field for
 * S_LITERAL_POINTERS sections.  The external functions declared at the end of
 * this file operate on this data and are used for the other fields of a
 * merged_section for literals (literal_merge, literal_write, and literal_free).
 */
struct literal_pointer_data {
    struct literal_pointer_bucket **hashtable;	/* the hash table */
    struct literal_pointer_block		/* the literal pointers */
	*literal_pointer_blocks;
#ifdef DEBUG
    long nfiles;	/* number of files with this section */
    long nliterals;	/* total number of literal pointers in the input files*/
			/*  merged into this section  */
    long nprobes;	/* number of hash probes */
#endif DEBUG
};

/* the number of entries in the hash table */
#define LITERAL_POINTER_HASHSIZE 1000

/* the hash bucket entries in the hash table points to; allocated as needed */
struct literal_pointer_bucket {
    struct literal_pointer
	*literal_pointer;	/* pointer to the literal pointer */
    long output_offset;		/* offset to this pointer in the output file */
    struct literal_pointer_bucket
	*next;			/* next in the hash chain */
};

/* The structure to hold a literal pointer.   This can be one of two things,
 * if the symbol is undefined then merged_symbol is not NULL and points to
 * a merged symbol and with offset defines the merged literal, second the
 * literal being pointed to is in the merged section literal_ms and is at
 * merged_section_offset in that section and offset is added.
 */
struct literal_pointer {
    struct merged_section *literal_ms;	
    long merged_section_offset;
    long offset;
    struct merged_symbol *merged_symbol;
};

/* the number of entries each literal pointer block*/
#define LITERAL_POINTER_BLOCK_SIZE 1000
/* the blocks that store the literals; allocated as needed */
struct literal_pointer_block {
    long used;				/* the number of literals used in */
    struct literal_pointer		/*  this block */
	literal_pointers
	[LITERAL_POINTER_BLOCK_SIZE];	/* the literal pointers */
    struct literal_pointer_block *next;	/* the next block */
};

extern void literal_pointer_merge(
    struct literal_pointer_data *data, 
    struct merged_section *ms,
    struct section *s, 
    struct section_map *section_map);

extern void literal_pointer_order(
    struct literal_pointer_data *data, 
    struct merged_section *ms);

extern void literal_pointer_output(
    struct literal_pointer_data *data, 
    struct merged_section *ms);

extern void literal_pointer_free(
    struct literal_pointer_data *data);

#ifdef DEBUG
extern void print_literal_pointer_data(
    struct literal_pointer_data *data, 
    char *ident);
extern void literal_pointer_data_stats(
    struct literal_pointer_data *data,
    struct merged_section *ms);
#endif DEBUG

/*
 * Global types, variables and routines declared in the file symbols.c.
 *
 * The following include file need to be included before this file:
 * #include <nlist.h>
 * #include "ld.h"
 */

/*
 * This structure holds an external symbol that has been merged and will be in
 * the output file.  The nlist feilds are used as follows:
 *      union {
 *	    char *n_name;  The name of the symbol (pointing into the merged
 * 			   string table).
 *	    long  n_strx;  Only set write before the symbol is written to the
 *			   output file and then the symbol is no longer used.
 *      } n_un;
 *      unsigned char n_type;	Same as in an object file.
 *      unsigned char n_sect;	"
 *      short	      n_desc;	"
 *      unsigned long n_value;	The value of the symbol as it came from the
 * 				object it was defined in for N_SECT and N_ABS
 *				type symbols.
 *				For common symbols the size of the largest
 *				common.
 *				For N_INDR symbols a pointer to the
 *				merged_symbol that it is an indirect for.
 *	
 */
struct merged_symbol {
    struct nlist nlist;		/* the nlist structure of this merged symbol */
    struct object_file		/* pointer to the object file this symbol is */
	*definition_object;	/*  defined in */
};

/*
 * The number of merged_symbol structrures in a merged_symbol_list.
 * THE VALUE OF THIS MACRO MUST BE AN ODD NUMBER (NOT A POWER OF TWO).
 */
#ifndef RLD
#define NSYMBOLS 2001
#else
#define NSYMBOLS 201
#endif RLD
/* The number of size of the hash table in a merged_symbol_list */
#define SYMBOL_LIST_HASH_SIZE	(NSYMBOLS * 2)

/*
 * The structure to hold a chunk the list of merged symbol.
 */
struct merged_symbol_list {
    struct merged_symbol	/* the merged_symbol structures in this chunk */
	merged_symbols[NSYMBOLS];
    long used;			/* the number used in this chunk */
    struct merged_symbol	/* pointer to the hash table for this chunk */
	**hash_table;		/*  (it is a pointer so it can be free'ed) */
    struct merged_symbol_list	/* the next chunk (NULL in no more chunks) */
	*next;
};

/* the blocks that store the strings; allocated as needed */
struct string_block {
    long size;			/* the number of bytes in this block */
    long used;			/* the number of bytes used in this block */
    char *strings;		/* the strings */
    long index;			/* the relitive index into the final symbol */
				/*  table for the block (set in pass2). */
    enum bool base_strings;	/* TRUE if this block is for strings from the */
				/*  base file (used if strip_base_symbols is */
				/*  TRUE) */
#ifdef RLD
    long set_num;		/* the object file set number these strings */
				/*  come from. */
#endif RLD
    struct string_block *next;	/* the next block */
};

/*
 * The structure for the undefined list and the structure that the items
 * are allocated out of.
 */
struct undefined_list {
    struct merged_symbol
	*merged_symbol;		/* the undefined symbol */
    struct undefined_list *prev;/* previous in the chain */
    struct undefined_list *next;/* next in the chain */
};

/*
 * The structure of the load map for common symbols.  This is only used to help
 * print the load map.  It is created by define_commmon_symbols() in symbols.c
 * and used in print_load_map() in layout.c.
 */
struct common_load_map {
    struct merged_section	/* the section common symbol were allocated in*/
	*common_ms;
    long ncommon_symbols;	/* number of common symbols */
    struct common_symbol	/* a pointer to an array of structures (one */
	*common_symbols;	/*  for each common symbol) */
};
struct common_symbol {
    struct merged_symbol	/* a pointer the merged common symbol */
	*merged_symbol;
    unsigned long common_size;	/* the size of the merged common symbol */
};

/*
 * The head of the symbol list and the total count of all external symbols
 * in the list.
 */
extern struct merged_symbol_list *merged_symbol_lists;
extern long nmerged_symbols;

/*
 * The head of the list of the blocks that store the strings for the merged
 * symbols and the total size of all the strings.
 */
extern struct string_block *merged_string_blocks;
extern long merged_string_size;

/*
 * The head of the undefined list itself.  This is a circular list so it can be
 * searched from start to end and so new items can be put on the end.  This 
 * structure never has it's merged_symbol filled in but only serves as the
 * head and tail of the list.
 */
extern struct undefined_list undefined_list;

/*
 * The common symbol load map.  Only allocated and filled in if load map is
 * requested.
 */
extern struct common_load_map common_load_map;

/*
 * The object file that is created for the common symbols to be allocated in.
 */
extern
#ifdef RLD
const 
#endif RLD
struct object_file link_edit_common_object;

/*
 * The number of local symbols that will appear in the output file and the
 * size of their strings.
 */
extern long nlocal_symbols;
extern long local_string_size;

/*
 * The things to deal with creating local symbols with the object file's name
 * for a given section.  If the section name is (__TEXT,__text) these are the
 * same as a UNIX link editor's file.o symbols for the text section.
 */
struct sect_object_symbols {
    enum bool specified; /* if this has been specified on the command line */
    char *segname;	 /* the segment name */
    char *sectname;	 /* the section name */
    struct merged_section *ms;	/* the merged section structure */
};
extern struct sect_object_symbols sect_object_symbols;

/*
 * The strings in the string table can't start at offset 0 because a symbol with
 * a string offset of zero is defined to have a null "" symbol name.  So the
 * first STRING_SIZE_OFFSET bytes are not used and the first string starts after
 * this amount.  Also these first bytes are zero so that if the special case of
 * a zero index is not handled by a program it will happen to work.
 */
#define STRING_SIZE_OFFSET (sizeof(long))

extern void merge_symbols(
    void);
extern struct merged_symbol *command_line_symbol(
    char *symbol_name);
extern struct merged_symbol **lookup_symbol(
    char *symbol_name);
extern void command_line_indr_symbol(
    char *symbol_name,
    char *indr_symbol_name);
extern void delete_from_undefined_list(
    struct undefined_list *undefined);
extern void free_pass1_symbol_data(
    void);
extern void free_undefined_list(
    void);
extern void define_common_symbols(
    void);
extern void define_link_editor_execute_symbols(
    unsigned long header_address);
extern void define_link_editor_preload_symbols(
    void);
extern void reduce_indr_symbols(
    void);
extern void layout_merged_symbols(
    void);
extern void output_local_symbols(
    void);
extern void output_merged_symbols(
    void);
extern long merged_symbol_output_index(
    struct merged_symbol *merged_symbol);

#ifdef RLD
extern void free_multiple_defs(
    void);
extern void remove_merged_symbols(
    void);
#endif RLD

#ifdef DEBUG
extern void print_symbol_list(
    char *string,
    enum bool input_based);
extern struct section *get_output_section(
    long sect);
extern void print_undefined_list(
    void);
#endif DEBUG

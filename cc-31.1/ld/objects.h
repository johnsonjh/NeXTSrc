/*
 * Global types, variables and routines declared in the file objects.c.
 *
 * The following include file need to be included before this file:
 * #include "ld.h"
 */

/*
 * The structure to hold the information for the object files actually loaded.
 */
struct object_file {
    char *file_name;		/* Name of the file the object is in. */
    char *obj_addr;		/* Address of the object file in memory. */
    long obj_size;		/* Size of the object file. */
    enum bool fvmlib_stuff;	/* TRUE if any SG_FVMLIB segments or any */
				/*  LC_LOADFVMLIB or LC_IDFVMLIB load */
				/*  commands in the file. */
    struct ar_hdr *ar_hdr;	/* Archive header (NULL in not in archive). */
    long nsection_maps;		/* Total number of sections in this object. */
    struct section_map		/* A section map for each section in this. */
	*section_maps;		/*  object file. */
    struct symtab_command	/* The LC_SYMTAB load command which has */
	*symtab;		/*  offsets to the symbol and string tables. */
    long nundefineds;		/* Number of undefined symbol in the object */
    struct undefined_map	/* Map of undefined symbol indexes and */
	*undefined_maps;	/*  pointers to merged symbols (for external */
				/*  relocation) */
    long local_string_offset;	/* Start of local string offset if strip_level*/
				/*  == STRIP_NONE and the entire string table */
				/*  in then forwarded to the output file. */
    struct section_map		/* Current map of symbols used for -sectorder */
	*cur_section_map;	/*  that is being processed from this object */
#ifdef RLD
    long set_num;		/* The set number this object belongs to. */
    enum bool user_obj_addr;	/* TRUE if the user allocated the object */
				/*  file's memory (then rld doesn't */
				/*  deallocate it) */
#endif RLD
};

/* The number of object_file structrures in object_list */
#ifndef RLD
#define NOBJECTS 120
#else
#define NOBJECTS 10
#endif

/*
 * The structure to hold a chunk the list of object files actually loaded.
 */
struct object_list {
    struct object_file
	object_files[NOBJECTS]; /* the object_file structures in this chunk */
    long used;			/* the number used in this chunk */
    struct object_list *next;	/* the next chunk (NULL in no more chunks) */
};

/*
 * The structure for information about each section of an object module.  This
 * gets allocated as an array and the sections in it are in the order that
 * they appear in the header so that section numbers (minus one) can be used as
 * indexes into this array.
 */
struct section_map {
    struct section *s;		/* Pointer to the section structure in the    */
				/*  object file. */
    struct merged_section	/* Pointer the output section to get the      */
	*output_section;	/*  section number this will be in the output */
				/*  file used in pass2 to rewrite relocation  */
				/*  entries. */
    long offset;		/* The offset from the start of the section   */
			        /* that this object file's section will start */
				/* at in the output file. */
				/*  the address the section will be at and is */
				/*  used in pass2 to do the relocation.	      */
    long flush_offset;		/* The offset from the start of the section   */
				/*  that is to be used by output_flush() that */
				/*  includes the area before the above offset */
				/*  for alignment. */
    struct fine_reloc		/* Map for relocation of literals or other    */
	*fine_relocs;		/*  items which are smaller than the section. */
    long nfine_relocs;		/* Number of structures in the map */
    enum bool no_load_order;	/* Not to be scattered, loaded normalily */
    struct load_order		/* Map of symbols used for -sectorder */
	*load_orders;
    long nload_orders;		/* Number of structures in the map */
};

/*
 * This structure is used for relocating items in a section from an input file
 * when they are scatered in the output file (normally the section is copied 
 * as a whole).  The address in the input file and the resulting address of
 * the section in the output file are also needed to do the relocation.
 */
struct fine_reloc {
    long input_offset;		/* offset in the input file for the item */
    long output_offset;		/* offset in the output file for the item */
};

/*
 * This structure is used when a section has a -sectorder order file and is used
 * to set the orders and then build fine_reloc structures for it so it can be
 * scatter loaded.
 */
struct load_order {
    char *name;			/* symbol's name */
    long value;			/* symbol's value */
    long order;			/* order in output, 0 if not assigned yet */
    long input_offset;		/* offset in the input file for the item */
    long input_size;		/* size of symbol in the input file */
    long output_offset;		/* offset in the output file for the item */
};

/*
 * This structure holds pairs of indexes into an object files symbol table and
 * pointers to merged symbol table structures for each symbol that is an
 * undefined symbol in an object file.
 */
struct undefined_map {
    long index;		/* index of symbol in the object file's symbol table */
    struct merged_symbol/* pointer to the merged symbol */
	*merged_symbol;
};

/*
 * The head of the object file list and the total count of all object files
 * in the list.
 */
extern struct object_list *objects;
extern long nobjects;

/*
 * A pointer to the current object being processed in pass1 or pass2.
 */
extern struct object_file *cur_obj;

/*
 * A pointer to the base object for an incremental link if not NULL.
 */
extern struct object_file *base_obj;

extern struct object_file *new_object_file(
    void);
extern struct object_file *add_last_object_file(
    struct object_file *new_object);
extern void remove_last_object_file(
    struct object_file *last_object);
extern void print_obj_name(
    struct object_file *obj);
extern char *obj_member_name(
    struct object_file *obj);
extern void print_whatsloaded(
    void);
extern unsigned long fine_reloc_output_offset(
    struct section_map *map,
    unsigned long input_offset);
#ifdef RLD
extern void clean_objects(
    void);
extern void remove_objects(
    void);
#endif RLD

#ifdef DEBUG
void print_object_list(
    void);
#endif DEBUG

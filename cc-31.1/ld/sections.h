/*
 * Global types, variables and routines declared in the file sections.c.
 *
 * The following include file need to be included before this file:
 * #include <sys/loader.h> 
 * #include "ld.h"
 * #include "objects.h"
 */

/*
 * The fields of the segment command in a merged segment are set and
 * maintained as follows:
 *	cmd		set in layout_segments() (layout)
 *	cmdsize		set in layout_segments() (layout)
 *	segname		set when the merged segment is created (pass1)
 *	vmaddr		set in process_segments() or layout_segments() (layout)
 *	vmsize		set in layout_segments() (layout)
 *	fileoff		set in layout_segments() (layout)
 *	filesize	set in layout_segments() (layout)
 *	maxprot		set in process_segments() or layout_segments() (layout)
 *	initprot	set in process_segments() or layout_segments() (layout)
 *	nsects		incremented as each section is merged (pass1)
 *	flags		set to 0 in process_segments and SG_NORELOC
 *			 conditionally or'ed in in pass2() (pass2)
 */
struct merged_segment {
    struct segment_command sg;	/* The output file's segment structure. */
    struct merged_section	/* The list of section that contain contents */
	*content_sections;	/*  that is non-zerofill sections. */
    struct merged_section	/* The list of zerofill sections. */
	*zerofill_sections;
    char *filename;		/* File this segment is in, the output file */
				/*  or a fixed VM shared library */
    enum bool addr_set;		/* TRUE when address of this segment is set */
    enum bool prot_set;		/* TRUE when protextion of this segment is set*/
#ifdef RLD
    long set_num;		/* Object set this segment first appears in. */
#endif RLD
    struct merged_segment *next;/* The next segment in the list. */
};

/*
 * The fields of the section structure in a merged section are set and
 * maintained as follows:
 *	sectname	set when the merged segment is created (pass1)
 *	segname		set when the merged segment is created (pass1)
 *	addr		set in layout_segments (layout)
 *	size		accumulated to the total size (pass1)
 *	offset		set in layout_segments (layout)
 *	align		merged to the maximum alignment (pass1)
 *	reloff		set in layout_segments (layout)
 *	nreloc		accumulated to the total count (pass1)
 *	flags		set when the merged segment is created (pass1)
 *	reserved1	zeroed when created in merged_section (pass1)
 *	reserved2	zeroed when created in merged_section (pass1)
 */
struct merged_section {
    struct section s;		/* The output file's section structure. */
    long output_sectnum;	/* Section number in the output file. */
    long output_nrelocs;	/* The current number of relocation entries */
				/*  written to the output file in pass2 for */
				/*  this section */
    /* These two fields are used to help set the SG_NORELOC flag */
    enum bool relocated;	/* This section was relocated */
    enum bool referenced;	/* This section was referenced by a relocation*/
				/*  entry (local or through a symbol). */
    /* The literal_* fields are used only if this section is a literal section*/
    void (*literal_merge)();	/* The routine to merge the literals. */
    void (*literal_output)();	/* The routine to write the literals. */
    void (*literal_free)();	/* The routine to free the literals. */
    void (*literal_order)();	/* The routine to order the literals. */
    void *literal_data;		/* A pointer to a block of data to help merge */
				/*  and hold the literals. */
    /* These three fields are used only if this section is created from a file*/
    char *contents_filename;	/* File name for the contents of the section */
				/*  if it is created from a file, else NULL */
    char *file_addr;		/* address the above file is mapped at */
    long file_size;		/* size of above file as returned by stat(2) */

    /* These three fields are used only if this section has a -sectorder file */
    char *order_filename;	/* File name that contains the order that */
				/*  symbols are to loaded in this section */
    char *order_addr;		/* address the above file is mapped at */
    long order_size;		/* size of above file as returned by stat(2) */
    struct order_load_map	/* the load map for printing with -M */
	*order_load_maps;
    long norder_load_maps;	/* size of the above map */
#ifdef RLD
    long set_num;		/* Object set this section first appears in. */
#endif RLD
    struct merged_section *next;/* The next section in the list. */
};

/*
 * This is the load map (-M) for sections that have had their sections orders
 * with -sectorder option.
 */
struct order_load_map {
    char *archive_name;		/* archive name */
    char *object_name;		/* object name */
    char *symbol_name;		/* symbol name */
    long value;			/* symbol's value */
    struct section_map
	*section_map;		/* section map to relocate symbol's value */
    long size;			/* size of symbol in the input file */
    long order;			/* order the symbol appears in the section */
};

/* the pointer to the head of the output file's section list */
extern struct merged_segment *merged_segments;
#ifdef RLD
extern struct merged_segment *original_merged_segments;
#endif RLD

/* the total number relocation entries */
extern long nreloc;

extern void merge_sections(
    void);
extern void merge_literal_sections(
    void);
extern void layout_ordered_sections(
    void);
extern void parse_order_line(
    char *line,
    char **archive_name,
    char **object_name,
    char **symbol_name,
    struct merged_section *ms,
    long line_number);
extern void output_literal_sections(
    void);
extern void output_sections_from_files(
    void);
extern void output_section(
    struct section_map *map);
extern void flush_scatter_copied_sections(
    void);
extern struct merged_section *create_merged_section(
    struct section *s);
extern struct merged_segment *lookup_merged_segment(
    char *segname);
extern struct merged_section *lookup_merged_section(
    char *segname,
    char *sectname);
#ifdef RLD
extern void reset_merged_sections(
    void);
extern void zero_merged_sections_sizes(
    void);
extern void remove_merged_sections(
    void);
#endif RLD

#ifdef DEBUG
extern void print_merged_sections(
    char *string);
extern void print_merged_section_stats(
    void);
extern void print_name_arrays(
    void);
#endif DEBUG

/*
 * Global types, variables and routines declared in the file specs.c.
 *
 * The following include file need to be included before this file:
 * #include <mach.h>
 * #include "ld.h"
 */

/* Type to hold the information specified on the command line about segments */
struct segment_spec {
    char *segname;		/* full segment name from command line */
    enum bool addr_specified;	/* TRUE if address has been specified */
    enum bool prot_specified;	/* TRUE if protection has been specified */
    long addr;			/* specified address */
    vm_prot_t maxprot;		/* specified maximum protection */
    vm_prot_t initprot;		/* specified initial protection */
    long nsection_specs;	/* count of section_spec structures below */
    struct section_spec		/* list of section_spec structures for */
	  *section_specs;	/*  -segcreate options */
    enum bool processed;	/* TRUE after this has been processed */
};

/* Type to hold the information about sections specified on the command line */
struct section_spec {
    char *sectname;		/* full section name from command line */
    enum bool align_specified;	/* TRUE if alignment has been specified */
    long align;			/* the alignment (as a power of two) */
    char *contents_filename;	/* file name for the contents of the section */
    char *file_addr;		/* address the above file is mapped at */
    long file_size;		/* size of above file as returned by stat(2) */
    char *order_filename;	/* file name that contains the order that */
				/*  symbols are to loaded in this section */
    char *order_addr;		/* address the above file is mapped at */
    long order_size;		/* size of above file as returned by stat(2) */
    enum bool processed;	/* TRUE after this has been processed */
};

/* The structures to hold the information specified about segments */
extern struct segment_spec *segment_specs;
extern long nsegment_specs;

extern struct segment_spec *create_segment_spec(char *segname);
extern struct section_spec *create_section_spec(struct segment_spec *seg_spec,
						char *sectname);
extern struct segment_spec * lookup_segment_spec(char *segname);
extern struct section_spec *lookup_section_spec(char *segname, char *sectname);
extern void process_section_specs(void);
extern void process_segment_specs(void);
#ifdef DEBUG
extern void print_segment_specs(void);
extern void print_prot(vm_prot_t prot);
#endif DEBUG

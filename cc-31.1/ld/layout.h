/*
 * Global types, variables and routines declared in the file layout.c.
 * (and the global varaiable declared in rld.c).
 *
 * The following include file need to be included before this file:
 * #include <sys/loader.h>
 */

/* The output file's mach header */
extern struct mach_header output_mach_header;

/*
 * The output file's symbol table load command and the offsets used in the
 * second pass to output the symbol table and string table.
 */
struct symtab_info {
    struct symtab_command symtab_command;
    long output_nsyms;		/* the current number of symbols in pass2 */
    long output_strsize;	/* the current string table size in pass2 */
};
extern struct symtab_info output_symtab_info;

/*
 * The output file's thread load command and the machine specific information
 * for it.
 */
struct thread_info {
    struct thread_command thread_command;
    enum bool thread_in_output;	/* TRUE if the output file has a thread cmd */
    unsigned long flavor;	/* the flavor for the registers with the pc */
    unsigned long count;	/* the count (sizeof(long)) of the state */
    int *entry_point;		/* pointer to the entry point in the state */
    void *state;		/* a pointer to a thread state */
};
extern struct thread_info output_thread_info;

extern void layout(void);

#ifdef RLD
/*
 * The user's address function to be called in layout to get the address of
 * where to link edit the result.
 */
extern unsigned long (
    *address_func)(
	unsigned long size,
	unsigned long header_address);
#endif RLD

#ifdef DEBUG
extern void print_mach_header(void);
extern void print_symtab_info(void);
extern void print_thread_info(void);
#endif DEBUG

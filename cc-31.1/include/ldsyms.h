#import <sys/loader.h>
/*
 * This file describes the link editor defined symbols.  The semantics of a link
 * editor is that it is defined by the link editor only if it is referenced and
 * it is an error for the user to define them (see the man page ld(1)).  The
 * standard UNIX link editor symbols: __end, __etext and __edata are not
 * not supported by the NeXT Mach-O link editor.  These symbols are really not
 * meaning full in a Mach-O object file and the link editor symbols that are
 * supported (described here) replaces them.  In the case of the standard UNIX
 * link editor symbols the program can use the symbol __mh_execute_header and
 * walk the load commands of it's program to determine the ending (or begining)
 * of any section or segment in the program.  Note that the compiler prepends an
 * underbar to all external symbol names coded in a high level language.  Thus
 * in 'C' names are coded without an underbar and symbol names in the symbol
 * table have an underbar.  There are two cpp macros for each link editor
 * defined name in this file.  The macro with a leading underbar is the symbol
 * name and the one without is the name as coded in 'C'.
 */

/*
 * The value of the link editor defined symbol _MH_EXECUTE_SYM is the address
 * of the mach header in a Mach-O executable file type.  It does not appear in
 * any file type other than a MH_EXECUTE file type.  The type of the symbol is
 * absolute as the header is not part of any section.
 */
#define _MH_EXECUTE_SYM	"__mh_execute_header"
#define MH_EXECUTE_SYM	"_mh_execute_header"
extern const struct mach_header _mh_execute_header;

/*
 * For the MH_PRELOAD file type the headers are not loaded as part of any
 * segment so the link editor defines symbols defined for the beginning
 * and ending of each segment and each section in each segment.  The names for
 * the symbols for a segment's beginning and end will have the form:
 * __SEGNAME__begin and  __SEGNAME__end where __SEGNAME is the name of the
 * segment.  The names for the symbols for a section's beginning and end will
 * have the form: __SEGNAME__sectname__begin and __SEGNAME__sectname__end where
 * __sectname is the name of the section and __SEGNAME is the segment it is in.
 * 
 * The above symbols' types are those of the section they are refering to.
 * This is true even for symbols who's values are end's of a section and
 * that value is next address after that section and not really in that
 * section.  This results in these symbols having types refering to sections
 * who's values are not in that section.
 */

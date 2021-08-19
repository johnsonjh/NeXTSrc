/*
 * Global types, variables and routines declared in the file ld.c.
 *
 * The following include file need to be included before this file:
 * #include <sys/loader.h>
 * #include <mach.h>
 * #include <stdarg.h>  (included in <stdio.h>)
 */
/* These #undef's are because of the #define's in <mach.h> */
#undef TRUE
#undef FALSE

/* General boolean type */
enum bool { FALSE, TRUE };

/* Type for the possible levels of stripping, in increasing order */
enum strip_levels {
    STRIP_NONE,
    STRIP_L_SYMBOLS,
    STRIP_DEBUG,
    STRIP_NONGLOBALS,
    STRIP_ALL
};

/* name of this program as executed (argv[0]) */
extern char *progname;
/* indication of an error set in error(), for processing a number of errors
   and then exiting */
extern long errors;
/* the pagesize of the machine this program is running on, getpagesize() value*/
extern long host_pagesize;

/* name of output file */
extern char *outputfile;
/* type of output file */
extern long filetype;
/*
 * The cputype and cpusubtype of the object files being loaded which will be
 * the output cpu type and subtype.
 */
extern cpu_type_t cputype;
extern cpu_subtype_t cpusubtype;

extern enum bool trace;			/* print stages of link-editing */
extern enum bool save_reloc;		/* save relocation information */
extern enum bool load_map;		/* print a load map */
extern enum bool define_comldsyms;	/* define common and link-editor defined
					   symbol reguardless of file type */
extern enum bool seglinkedit;		/* create the link edit segment */
extern enum bool whyload;		/* print why archive members are 
					   loaded */
extern enum bool flush;			/* Use the output_flush routine to flush
					   output file by pages */
extern enum bool sectorder_detail;	/* print sectorder warnings in detail */

/* The level of symbol table stripping */
extern enum strip_levels strip_level;
/* Strip the base file symbols (the -A argument's symbols) */
extern enum bool strip_base_symbols;

/* The list of symbols to be traced */
extern char **trace_syms;
extern long ntrace_syms;

/* The list of allowed undefined symbols */
extern char **undef_syms;
extern long nundef_syms;

/* The segment alignment */
extern unsigned long segalign;
/* The maximum segment alignment allowed to be specified, in hex */
#define MAXSEGALIGN		0x8000
/* The default section alignment */
extern unsigned long defaultsectalign;
/* The maximum section alignment allowed to be specified, as a power of two */
#define MAXSECTALIGN		15 /* 2**15 or 0x8000 */
/* The default section alignment if not specified, as a power of two */
#define DEFAULTSECTALIGN	4  /* 2**4 or 16 */

/* The first segment address */
extern unsigned long seg1addr;
extern enum bool seg1addr_specified;

/* The header pad */
extern unsigned long headerpad;

/* The name of the specified entry point */
extern char *entry_point_name;

extern void *allocate(long size);
extern void *reallocate(void *, long size);
extern long round(long v, unsigned long r);
extern void print(const char *format, ...);
extern void vprint(const char *format, va_list ap);
extern void warning(const char *format, ...);
extern void error(const char *format, ...);
extern void fatal(const char *format, ...);
extern void warning_with_cur_obj(const char *format, ...);
extern void error_with_cur_obj(const char *format, ...);
extern void system_error(const char *format, ...);
extern void system_fatal(const char *format, ...);
extern void mach_fatal(kern_return_t r, char *format, ...);

#ifdef DEBUG
extern long debug;			/* link-editor debugging */
#endif DEBUG

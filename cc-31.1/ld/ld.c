#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * The NeXT, Inc. Mach-O (Mach object file format) link-editor.  This file
 * contains the main() routine and the global error handling routines and other
 * miscellaneous small global routines.  It also defines the global varaibles
 * that are set or changed by command line arguments.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <libc.h>
#include <mach.h>
#include <mach_error.h>
#include <sys/loader.h>
#include <ar.h>
#include <nlist.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ld.h"
#include "specs.h"
#include "pass1.h"
#include "objects.h"
#include "fvmlibs.h"
#include "sections.h"
#include "symbols.h"
#include "layout.h"
#include "pass2.h"

/* name of this program as executed (argv[0]) */
char *progname = NULL;
/* indication of an error set in error(), for processing a number of errors
   and then exiting */
long errors = 0;
/* the pagesize of the machine this program is running on, getpagesize() value*/
long host_pagesize = 0;

/* name of output file */
char *outputfile = NULL;
/* type of output file */
long filetype = MH_EXECUTE;
#ifndef RLD
static enum bool filetype_specified = FALSE;
/* if the -A flag is specified use to set the object file type */
static enum bool Aflag_specified = FALSE;
#endif !defined(RLD)
/*
 * The cputype and cpusubtype of the object files being loaded which will be
 * the output cpu type and subtype.
 */
cpu_type_t cputype = 0;
cpu_subtype_t cpusubtype = 0;

enum bool trace = FALSE;		/* print stages of link-editing */
enum bool save_reloc = FALSE;		/* save relocation information */
enum bool load_map = FALSE;		/* print a load map */
enum bool define_comldsyms = TRUE;	/* define common and link-editor defined
					   symbol reguardless of file type */
#ifndef RLD
static enum bool
    dflag_specified = FALSE;		/* the -d flag has been specified */
#endif !defined(RLD)
enum bool seglinkedit = FALSE;		/* create the link edit segment */
#ifndef RLD
static enum bool
    seglinkedit_specified = FALSE;	/* if either -seglinkedit or */
					/*  -noseglinkedit was specified */
#endif !defined(RLD)
enum bool whyload = FALSE;		/* print why archive members are 
					   loaded */
#ifndef RLD
static enum bool whatsloaded = FALSE;	/* print which object files are loaded*/
#endif !defined(RLD)
enum bool flush = TRUE;			/* Use the output_flush routine to flush
					   output file by pages */
enum bool sectorder_detail = FALSE;	/* print sectorder warnings in detail */
static enum bool nowarnings = FALSE;	/* suppress warnings */

/* The level of symbol table stripping */
enum strip_levels strip_level = STRIP_NONE;
/* Strip the base file symbols (the -A argument's symbols) */
enum bool strip_base_symbols = FALSE;

/* The list of symbols to be traced */
char **trace_syms = NULL;
long ntrace_syms = 0;

/* The list of allowed undefined symbols */
char **undef_syms = NULL;
long nundef_syms = 0;

/* The segment alignment */
unsigned long segalign = TARGET_PAGESIZE;
#ifndef RLD
static enum bool segalign_specified = FALSE;
#endif !defined(RLD)

/* The default section alignment */
unsigned long defaultsectalign = DEFAULTSECTALIGN;

/* The first segment address */
unsigned long seg1addr = 0;
enum bool seg1addr_specified = FALSE;

/* The header pad */
unsigned long headerpad = 0;
#ifndef RLD
static enum bool headerpad_specified = FALSE;
#endif !defined(RLD)

/* The name of the specified entry point */
char *entry_point_name = NULL;

#ifdef DEBUG
long debug = 0;				/* link-editor debugging */
#endif DEBUG

#ifdef RLD
/* the cleanup routine for fatal errors to remove the output file */
extern void cleanup(void);
#else !defined(RLD)
static void cleanup(void);

/* The signal hander routine for SIGINT and SIGTERM */
static void handler(int sig);

/* Static routines to help parse arguments */
static enum bool ispoweroftwo(unsigned long x);
static vm_prot_t getprot(char *prot, char **endp);
static enum bool check_max_init_prot(vm_prot_t maxprot, vm_prot_t initprot);

/*
 * main() parses the command line arguments and drives the link-edit process.
 */
void
main(
int argc,
char *argv[],
char *envp[])
{
    long i, symbols_created, objects_specified, sections_created, fd;
    char *p, *symbol_name, *indr_symbol_name, *endp;
    struct segment_spec *seg_spec;
    struct section_spec *sect_spec;
    unsigned long align, tmp;
    struct stat stat_buf;
    kern_return_t r;

	progname = argv[0];
	host_pagesize = getpagesize();

	if(argc == 1)
	    fatal("Usage: %s [options] file [...]", progname);

	/*
	 * If interrupt and termination signal are not being ignored catch
	 * them so things can be cleaned up.
	 */
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
	    signal(SIGINT, handler);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
	    signal(SIGTERM, handler);

	/*
	 * Parse the command line options in this pass and skip the object files
	 * and symbol creation flags in this pass.  This will make sure optionsd
	 * like -Ldir are not position dependent relative to -lx options (the
	 * same for -ysymbol relative to object files, etc).
	 */
	for(i = 1 ; i < argc ; i++){
	    if(*argv[i] != '-'){
		/* object file argv[i] processed in the next pass of
		   parsing arguments */
		continue;
	    }
	    else{
		p = &(argv[i][1]);
		switch(*p){
		case 'l':
		    /* path searched abbrevated file name, processed in the
		       next pass of parsing arguments */
		    break;

		/* Flags effecting search path of -lx arguments */
		case 'L':
		    if(p[1] == '\0')
			fatal("-L: directory name missing");
		    /* add a pathname to the list of search paths */
		    search_dirs = reallocate(search_dirs,
					   (nsearch_dirs + 1) * sizeof(char *));
		    search_dirs[nsearch_dirs++] = &(p[1]);
		    break;
		case 'Z':
		    if(p[1] != '\0')
			goto unknown_flag;
		    /* do not use the standard search path */
		    standard_dirs[0] = NULL;
		    break;

		/* File format flags */
		case 'M':
		    if(strcmp(p, "Mach") == 0){
			if(filetype_specified == TRUE && filetype != MH_EXECUTE)
			    fatal("more than one output filetype specified");
			filetype_specified = TRUE;
			filetype = MH_EXECUTE;
		    }
		    else if(strcmp(p, "M") == 0){
			/* produce load map */
			load_map = TRUE;
		    }
		    else
			goto unknown_flag;
		    break;
		case 'p':
		    if(p[1] != '\0' && strcmp(p, "preload") != 0)
			goto unknown_flag;
		    if(filetype_specified == TRUE && filetype != MH_PRELOAD)
			fatal("more than one output filetype specified");
		    filetype_specified = TRUE;
		    filetype = MH_PRELOAD;
		    break;
		case 'f':
		    if(p[1] == '\0')
			fatal("use of old flag -f (old version of mkshlib(1) "
			      "will not work with this version of ld(1))");
		    else if(strcmp(p, "fvmlib") == 0){
			if(filetype_specified == TRUE && filetype != MH_FVMLIB)
			    fatal("more than one output filetype specified");
			filetype_specified = TRUE;
			filetype = MH_FVMLIB;
		    }
		    else
			goto unknown_flag;
		    break;
		case 'r':
		    if(p[1] != '\0')
			goto unknown_flag;
		    /* save relocation information, and produce a relocatable
		       object */
		    if(filetype_specified == FALSE)
			filetype = MH_OBJECT;
		    save_reloc = TRUE;
		    if(dflag_specified == FALSE)
			define_comldsyms = FALSE;
		    break;
		case 'A':
		    if(p[1] != '\0')
			goto unknown_flag;
		    if(++i >= argc)
			fatal("-A: argument missing");
		    /* object file argv[i] processed in the next pass of
		       parsing arguments */
		    Aflag_specified = TRUE;
		    break;
		case 'd':
		    if(strcmp(p, "d") == 0){
			/* define common symbols and loader defined symbols
			   reguardless of file format */
			dflag_specified = TRUE;
			define_comldsyms = TRUE;
		    }
#ifdef DEBUG
		    else if(strcmp(p, "debug") == 0){
			if(++i >= argc)
			    fatal("-debug: argument missing");
			debug |= strtoul(argv[i], &endp, 16);
			if(*endp != '\0')
			    fatal("argument for -debug %s not a proper "
				  "hexadecimal number", argv[i]);
		    }
#endif DEBUG
		    else
			goto unknown_flag;
		    break;

		case 'n':
		    if(strcmp(p, "noflush") == 0){
			flush = FALSE;
		    }
		    else if(strcmp(p, "noseglinkedit") == 0){
			if(seglinkedit_specified && seglinkedit == TRUE)
			    fatal("both -seglinkedit and -noseglinkedit can't "
				  "be specified");
			seglinkedit = FALSE;
			seglinkedit_specified = TRUE;
		    }
		    else
			goto unknown_flag;
		    break;

		/* Strip the base file symbols (the -A argument's symbols) */
		case 'b':
		    if(p[1] != '\0')
			goto unknown_flag;
		    strip_base_symbols = TRUE;
		    break;

		/*
		 * Stripping level flags, in increasing level of stripping.  The
		 * level of stripping is set to the maximum level specified.
		 */
		case 'X':
		    if(p[1] != '\0')
			goto unknown_flag;
		    if(strip_level < STRIP_L_SYMBOLS)
			strip_level = STRIP_L_SYMBOLS;
		    break;
		case 'S':
		    if(p[1] != '\0')
			goto unknown_flag;
		    if(strip_level < STRIP_DEBUG)
			strip_level = STRIP_DEBUG;
		    break;
		case 'x':
		    if(p[1] != '\0')
			goto unknown_flag;
		    if(strip_level < STRIP_NONGLOBALS)
			strip_level = STRIP_NONGLOBALS;
		    break;
		case 's':
		    if(strcmp(p, "s") == 0){
			strip_level = STRIP_ALL;
		    }
		    /*
		     * Flags for specifing information about sections.
		     */
		    /* create a section from the contents of a file
		       -sectcreate <segname> <sectname> <filename> */
		    else if(strcmp(p, "sectcreate") == 0 ||
		    	    strcmp(p, "segcreate") == 0){ /* the old name */
			if(i + 3 >= argc)
			    fatal("%s: arguments missing", argv[i]);
			seg_spec = create_segment_spec(argv[i+1]);
			sect_spec = create_section_spec(seg_spec, argv[i+2]);
			if(sect_spec->contents_filename != NULL)
			     fatal("section (%s,%s) multiply specified with a "
				   "%s option", argv[i+1], argv[i+2], argv[i]);
			if((fd = open(argv[i+3], O_RDONLY, 0)) == -1)
			    system_fatal("Can't open: %s for %s %s %s",
				    argv[i+3], argv[i], argv[i+1], argv[i+2]);
			if(fstat(fd, &stat_buf) == -1)
			    system_fatal("Can't stat file: %s for %s %s %s",
				    argv[i+3], argv[i], argv[i+1], argv[i+2]);
			/*
			 * For some reason mapping files with zero size fails
			 * so it has to be handled specially.
			 */
			if(stat_buf.st_size != 0){
			    if((r = map_fd((int)fd, (vm_offset_t)0,
				(vm_offset_t *)&(sect_spec->file_addr),
				(boolean_t)TRUE, (vm_size_t)stat_buf.st_size)
				) != KERN_SUCCESS)
				mach_fatal(r, "can't map file: %s for %s %s %s",
				    argv[i+3], argv[i], argv[i+1], argv[i+2]);
			}
			else{
			    sect_spec->file_addr = NULL;
			}
			close(fd);
			sect_spec->file_size = stat_buf.st_size;
			sect_spec->contents_filename = argv[i+3];
			i += 3;
		    }
		    /* specify the alignment of a section as a hexadecimal
		       power of 2
		       -sectalign <segname> <sectname> <number> */
		    else if(strcmp(p, "sectalign") == 0){
			if(i + 3 >= argc)
			    fatal("-sectalign arguments missing");
			seg_spec = create_segment_spec(argv[i+1]);
			sect_spec = create_section_spec(seg_spec, argv[i+2]);
			if(sect_spec->align_specified)
			     fatal("alignment for section (%s,%s) multiply "
				   "specified", argv[i+1], argv[i+2]);
			sect_spec->align_specified = TRUE;
			align = strtoul(argv[i+3], &endp, 16);
			if(*endp != '\0')
			    fatal("argument for -sectalign %s %s: %s not a "
				  "proper hexadecimal number", argv[i+1],
				  argv[i+2], argv[i+3]);
			if(!ispoweroftwo(align))
			    fatal("argument to -sectalign %s %s: %x (hex) must "
				  "be a power of two", argv[i+1], argv[i+2],
				  align);
			if(align != 0)
			    for(tmp = align; (tmp & 1) == 0; tmp >>= 1)
				sect_spec->align++;
			if(sect_spec->align > MAXSECTALIGN)
			    fatal("argument to -sectalign %s %s: %x (hex) must "
				  "equal to or less than %x (hex)", argv[i+1],
				  argv[i+2], align, 1 << MAXSECTALIGN);
			i += 3;
		    }
		    /* specify that section object symbols are to be created
		       for the specified section
		       -sectobjectsymbols <segname> <sectname> */
		    else if(strcmp(p, "sectobjectsymbols") == 0){
			if(i + 2 >= argc)
			    fatal("-sectobjectsymbols arguments missing");
			if(sect_object_symbols.specified &&
			   (strcmp(sect_object_symbols.segname,
				   argv[i+1]) != 0 ||
			    strcmp(sect_object_symbols.sectname,
				   argv[i+2]) != 0) )
			     fatal("-sectobjectsymbols multiply specified (it "
				   "can only be specified for one section)");
			sect_object_symbols.specified = TRUE;
			sect_object_symbols.segname = argv[i+1];
			sect_object_symbols.sectname = argv[i+2];
			i += 2;
		    }
		    /* layout a section in the order the symbols appear in file
		       -sectorder <segname> <sectname> <filename> */
		    else if(strcmp(p, "sectorder") == 0){
			if(i + 3 >= argc)
			    fatal("%s: arguments missing", argv[i]);
			seg_spec = create_segment_spec(argv[i+1]);
			sect_spec = create_section_spec(seg_spec, argv[i+2]);
			if(sect_spec->order_filename != NULL)
			     fatal("section (%s,%s) multiply specified with a "
				   "%s option", argv[i+1], argv[i+2], argv[i]);
			if((fd = open(argv[i+3], O_RDONLY, 0)) == -1)
			    system_fatal("Can't open: %s for %s %s %s",
				    argv[i+3], argv[i], argv[i+1], argv[i+2]);
			if(fstat(fd, &stat_buf) == -1)
			    system_fatal("Can't stat file: %s for %s %s %s",
				    argv[i+3], argv[i], argv[i+1], argv[i+2]);
			/*
			 * For some reason mapping files with zero size fails
			 * so it has to be handled specially.
			 */
			if(stat_buf.st_size != 0){
			    if((r = map_fd((int)fd, (vm_offset_t)0,
				(vm_offset_t *)&(sect_spec->order_addr),
				(boolean_t)TRUE, (vm_size_t)stat_buf.st_size)
				) != KERN_SUCCESS)
				mach_fatal(r, "can't map file: %s for %s %s %s",
				    argv[i+3], argv[i], argv[i+1], argv[i+2]);
			}
			else{
			    sect_spec->order_addr = NULL;
			}
			close(fd);
			sect_spec->order_size = stat_buf.st_size;
			sect_spec->order_filename = argv[i+3];
			i += 3;
		    }
		    else if(strcmp(p, "sectorder_detail") == 0){
			sectorder_detail = TRUE;
		    }
		    /*
		     * Flags for specifing information about segments.
		     */
		    /* specify the address (in hex) of a segment
		       -segaddr <segname> <address> */
		    else if(strcmp(p, "segaddr") == 0){
			if(i + 2 >= argc)
			    fatal("-segaddr: arguments missing");
			seg_spec = create_segment_spec(argv[i+1]);
			if(seg_spec->addr_specified == TRUE)
			    fatal("address of segment %s multiply specified",
				  argv[i+1]);
			seg_spec->addr_specified = TRUE;
			seg_spec->addr = strtoul(argv[i+2], &endp, 16);
			if(*endp != '\0')
			    fatal("address for -segaddr %s %s not a proper "
				  "hexadecimal number", argv[i+1], argv[i+2]);
			i += 2;
		    }
		    /* specify the protection for a segment
		       -segprot <segname> <maxprot> <initprot>
		       where the protections are specified with "rwx" with a 
		       "-" for no protection. */
		    else if(strcmp(p, "segprot") == 0){
			if(i + 3 >= argc)
			    fatal("-segprot: arguments missing");
			seg_spec = create_segment_spec(argv[i+1]);
			if(seg_spec->prot_specified == TRUE)
			    fatal("protection of segment %s multiply "
				  "specified", argv[i]);
			seg_spec->maxprot = getprot(argv[i+2], &endp);
			if(*endp != '\0')
			    fatal("bad character: '%c' in maximum protection: "
				  "%s for segment %s", *endp, argv[i+2],
				  argv[i+1]);
			seg_spec->initprot = getprot(argv[i+3], &endp);
			if(*endp != '\0')
			    fatal("bad character: '%c' in initial protection: "
				  "%s for segment %s", *endp, argv[i+3],
				  argv[i+1]);
			if(check_max_init_prot(seg_spec->maxprot,
					       seg_spec->initprot) == FALSE)
			    fatal("maximum protection: %s for segment: %s "	
				  "doesn't include all initial protections: %s",
				  argv[i+2], argv[i+1], argv[i+3]);
			seg_spec->prot_specified = TRUE;
			i += 3;
		    }
		    /* specify the address (in hex) of the first segment
		       -seg1addr <address> */
		    else if(strcmp(p, "seg1addr") == 0){
			if(i + 1 >= argc)
			    fatal("-seg1addr: arguments missing");
			if(seg1addr_specified == TRUE)
			    fatal("-seg1addr: multiply specified");
			seg1addr = strtoul(argv[i+1], &endp, 16);
			if(*endp != '\0')
			    fatal("address for -seg1addr %s not a proper "
				  "hexadecimal number", argv[i+1]);
			seg1addr_specified = TRUE;
			i += 1;
		    }
		    /* specify the segment alignment as a hexadecimal power of 2
		       -segalign <number> */
		    else if(strcmp(p, "segalign") == 0){
			if(segalign_specified)
			    fatal("-segalign: multiply specified");
			if(++i >= argc)
			    fatal("-segalign: argument missing");
			segalign = strtoul(argv[i], &endp, 16);
			if(*endp != '\0')
			    fatal("argument for -segalign %s not a proper "
				  "hexadecimal number", argv[i]);
			if(!ispoweroftwo(segalign) || segalign == 0)
			    fatal("argument to -segalign: %x (hex) must be a "
				  "non-zero power of two", segalign);
			if(segalign > MAXSEGALIGN)
			    fatal("argument to -segalign: %x (hex) must equal "
				  "to or less than %x (hex)", segalign,
				  MAXSEGALIGN);
			segalign_specified = TRUE;
			if(segalign < (1 << DEFAULTSECTALIGN)){
			    defaultsectalign = 0;
			    align = segalign;
			    while((align & 0x1) != 1){
				defaultsectalign++;
				align >>= 1;
			    }
			}
		    }
		    else if(strcmp(p, "seglinkedit") == 0){
			if(seglinkedit_specified && seglinkedit == FALSE)
			    fatal("both -seglinkedit and -noseglinkedit can't "
				  "be specified");
			seglinkedit = TRUE;
			seglinkedit_specified = TRUE;
		    }
		    else
			goto unknown_flag;
		    break;

		case 'T':
		    if(strcmp(p, "Ttext") == 0 || strcmp(p, "T") == 0){
			if(i + 1 >= argc)
			    fatal("%s: argument missing (also obsolete flag, "
				  "use -segaddr " SEG_TEXT " address)",argv[i]);
			seg_spec = create_segment_spec(SEG_TEXT);
			if(seg_spec->addr_specified == TRUE)
			    fatal("address of segment %s multiply specified",
				  SEG_TEXT);
			seg_spec->addr_specified = TRUE;
			seg_spec->addr = strtoul(argv[i+1], &endp, 16);
			if(*endp != '\0')
			    fatal("address for %s %s not a proper "
				  "hexadecimal number (also obsolete flag, "
				  "use -segaddr " SEG_TEXT " address)",
				  argv[i], argv[i+1]);
			warning("obsolete flag: %s %s (use -segaddr " SEG_TEXT
				" %s) will not be supported in future releases",
				argv[i], argv[i+1], argv[i+1]);
			i++;
		    }
		    else if(strcmp(p, "Tdata") == 0){
			if(i + 1 >= argc)
			    fatal("%s: argument missing (also obsolete flag, "
				  "use -segaddr " SEG_DATA " address)",argv[i]);
			seg_spec = create_segment_spec(SEG_DATA);
			if(seg_spec->addr_specified == TRUE)
			    fatal("address of segment %s multiply specified",
				  SEG_DATA);
			seg_spec->addr_specified = TRUE;
			seg_spec->addr = strtoul(argv[i+1], &endp, 16);
			if(*endp != '\0')
			    fatal("address for %s %s not a proper "
				  "hexadecimal number (also obsolete flag, "
				  "use -segaddr " SEG_DATA " address)",
				  argv[i], argv[i+1]);
			warning("obsolete flag: %s %s (use -segaddr " SEG_DATA
				" %s) will not be supported in future releases",
				argv[i], argv[i+1], argv[i+1]);
			i++;
		    }
		    else
			goto unknown_flag;
		    break;

		case 't':
		    /* trace flag */
		    if(p[1] != '\0')
			goto unknown_flag;
		    trace = TRUE;
		    break;

		case 'o':
		    if(strcmp(p, "object") == 0){
			if(filetype_specified == TRUE && filetype != MH_OBJECT)
			    fatal("more than one output filetype specified");
			filetype_specified = TRUE;
			filetype = MH_OBJECT;
			break;
		    }
		    /* specify the output file name */
		    if(p[1] != '\0')
			goto unknown_flag;
		    if(outputfile != NULL)
			fatal("-o: multiply specified");
		    if(++i >= argc)
			fatal("-o: argument missing");
		    outputfile = argv[i];
		    break;

		/* Flags dealing with symbols */
		case 'a':
		    *p = 'i';
		    warning("obsolete flag: -a%s no longer supported (using "
			  "-%s)", p + 1, p);
		case 'i':
		    if(strcmp(p, "ident") == 0){
			warning("obsolete flag: %s ignored", argv[i]);
			break;
		    }
		    /* create an indirect symbol, symbol_name, to be an indirect
		       symbol for indr_symbol_name */
		    symbol_name = p + 1;
	  	    indr_symbol_name = strchr(p + 1, ':');
		    if(indr_symbol_name == NULL ||
		       indr_symbol_name[1] == '\0' || *symbol_name == ':')
			fatal("-i argument: %s must have a ':' between it's "
			      "symbol names", p + 1);
		    /* the creating of the symbol is done in the next pass of
		       parsing arguments */
		    break;

		case 'u':
		    /* cause the specified symbol name to be undefined */
		    if(p[1] != '\0')
			goto unknown_flag;
		    if(++i >= argc)
			fatal("-u: argument missing");
		    /* the creating of the symbol is done in the next pass of
		       parsing arguments */
		    break;

		case 'e':
		    if(strcmp(p, "execute") == 0){
			if(filetype_specified == TRUE && filetype != MH_EXECUTE)
			    fatal("more than one output filetype specified");
			filetype_specified = TRUE;
			filetype = MH_EXECUTE;
			break;
		    }
		    /* specify the entry point, the symbol who's value to be
		       used as the program counter in the unix thread */
		    if(p[1] != '\0')
			goto unknown_flag;
		    /* check to see if the pointer is not already set */
		    if(entry_point_name != NULL)
			fatal("-e: multiply specified");
		    if(++i >= argc)
			fatal("-e: argument missing");
		    entry_point_name = argv[i];
		    break;

		case 'U':
		    if(p[1] != '\0')
			goto unknown_flag;
		    /* allow the specified symbol name to be undefined */
		    if(++i >= argc)
			fatal("-U: argument missing");
		    undef_syms = reallocate(undef_syms,
					    (nundef_syms + 1) * sizeof(char *));
		    undef_syms[nundef_syms++] = argv[i];
		    break;

		case 'w':
		    if(strcmp(p, "w") == 0)
			nowarnings = TRUE;
		    else if(strcmp(p, "whyload") == 0)
			whyload = TRUE;
		    else if(strcmp(p, "whatsloaded") == 0)
			whatsloaded = TRUE;
		    else
			goto unknown_flag;
		    break;

		case 'y':
		    /* symbol tracing */
		    if(p[1] == '\0')
			fatal("-y: symbol name missing");
		    trace_syms = reallocate(trace_syms,
					    (ntrace_syms + 1) * sizeof(char *));
		    trace_syms[ntrace_syms++] = &(p[1]);
		    break;

		case 'h':
		    /* specify the header pad (in hex)
		       -headerpad <value> */
		    if(strcmp(p, "headerpad") == 0){
			if(i + 1 >= argc)
			    fatal("-headerpad: argument missing");
			if(headerpad_specified == TRUE)
			    fatal("-headerpad: multiply specified");
			headerpad = strtoul(argv[i+1], &endp, 16);
			if(*endp != '\0')
			    fatal("address for -headerpad %s not a proper "
				  "hexadecimal number", argv[i+1]);
			headerpad_specified = TRUE;
			i += 1;
		    }
		    else
			goto unknown_flag;
		    break;

		/* old flags */
		case 'v':
		case 'g':
		case 'G':
		    if(p[1] != '\0')
			goto unknown_flag;
		    warning("obsolete flag: %s ignored", argv[i]);
		    break;

		case 'D':
		    if(p[1] != '\0')
			goto unknown_flag;
		    if(++i >= argc)
			fatal("-D: argument missing (obsolete flag, no longer "
			      "supported)");
		    fatal("-D %s: obsolete flag, no longer supported", argv[i]);

		default:
unknown_flag:
		    fatal("unknown flag: %s", argv[i]);
		}
	    }
	}
	/*
	 * Check for flag combinations that would result in a bad output file.
	 */
	if(save_reloc && strip_level == STRIP_ALL)
	    fatal("can't use -s with -r (resulting file would not be "
		  "relocatable)");
	if(save_reloc && strip_base_symbols == TRUE)
	    fatal("can't use -b with -r (resulting file would not be "
		  "relocatable)");

	/*
	 * If the output file name as not specified set it to the default name
	 * "a.out".  This needs to be done before any segments are merged
	 * because this is used when merging them (the 'filename' field in a
	 * merged_segment is set to it).
	 */
	if(outputfile == NULL)
	    outputfile = "a.out";
	/*
	 * If the -A flag is specified and the file type has not been specified
	 * then make the output file type MH_OBJECT.
	 */
	if(Aflag_specified == TRUE && filetype_specified == FALSE)
	    filetype = MH_OBJECT;

	/*
	 * If neither the -seglinkedit or -noseglinkedit has been specified then
	 * set creation of this segment if the output file type can have one.
	 * If -seglinkedit has been specified make sure the output file type
	 * can have one.
	 */
	if(seglinkedit_specified == FALSE){
	    if((filetype == MH_EXECUTE || filetype == MH_FVMLIB) &&
		strip_level != STRIP_ALL)
		seglinkedit = TRUE;
	    else
		seglinkedit = FALSE;
	}
	else{
	    if(seglinkedit && (filetype != MH_EXECUTE && filetype != MH_FVMLIB))
		fatal("link edit segment can't be created (wrong output file "
		      "type, file type must be MH_EXECUTE or MH_FVMLIB)");
	}

	if(trace)
	    print("%s: Pass 1\n", progname);
	/*
	 * This pass of parsing arguments only processes object files
	 * and creation of symbols now that all the options are set.
	 * This are order dependent and must be processed as they appear
	 * on the command line.
	 */
	symbols_created = 0;
	objects_specified = 0;
	sections_created = 0;
	for(i = 1 ; i < argc ; i++){
	    if(*argv[i] != '-'){
		/* just a normal object file name */
		pass1(argv[i], FALSE, FALSE);
		objects_specified++;
		continue;
	    }
	    else{
		p = &(argv[i][1]);
		switch(*p){
		case 'l':
		    /* path searched abbrevated file name */
		    pass1(argv[i], TRUE, FALSE);
		    objects_specified++;
		    break;
		case 'A':
		    if(base_obj != NULL)
			fatal("only -A argument can be specified");
		    pass1(argv[++i], FALSE, TRUE);
		    objects_specified++;
		    break;
		case 'u':
		    /* cause the specified symbol name to be undefined */
		    (void)command_line_symbol(argv[++i]);
		    symbols_created++;
		    break;
		case 'i':
		    if(strcmp(p, "ident") == 0){
			break;
		    }
		    /* create an indirect symbol, symbol_name, to be an indirect
		       symbol for indr_symbol_name */
		    symbol_name = p + 1;
	  	    indr_symbol_name = strchr(p + 1, ':');
		    *indr_symbol_name = '\0';
		    indr_symbol_name++;
		    command_line_indr_symbol(symbol_name, indr_symbol_name);
		    symbols_created++;
		    break;

		/* multi argument flags */
		case 's':
		    if(strcmp(p, "sectcreate") == 0 ||
		       strcmp(p, "segcreate") == 0){
			sections_created++;
			i += 3;
		    }
		    else if(strcmp(p, "sectalign") == 0 ||
		            strcmp(p, "segprot") == 0 ||
		            strcmp(p, "sectorder") == 0)
			i += 3;
		    else if(strcmp(p, "segaddr") == 0 ||
		            strcmp(p, "sectobjectsymbols") == 0)
			i += 2;
		    else if(strcmp(p, "seg1addr") == 0 ||
		            strcmp(p, "segalign") == 0)
			i++;
		    break;
		case 'o':
		    if(strcmp(p, "object") == 0)
			break;
		    i++;
		    break;
		case 'e':
		    if(strcmp(p, "execute") == 0)
			break;
		    i++;
		    break;
		case 'd':
		    if(strcmp(p, "d") == 0)
			break;
		    i++;
		    break;
		case 'h':
		case 'U':
		case 'T':
		    i++;
		    break;
		}
	    }
	}
	/*
	 * Check to see that the output file will have something in it.
	 */
	if(objects_specified == 0){
	    if(symbols_created != 0 || sections_created != 0)
		warning("no object files specified, only command line created "
			"symbols and/or sections created from files will "
			"appear in the output file");
	    else
		fatal("no object files specified");
	}
	else if(base_obj != NULL && nobjects == 1){
	    if(symbols_created != 0 || sections_created != 0)
		warning("no object files loaded other than base file, only "
			"additional command line created symbols and/or "
			"sections created from files will appear in the output "
			"file");
	    else
		fatal("no object files loaded other than base file");
	}
	else if(nobjects == 0){
	    if(symbols_created != 0 || sections_created != 0)
		warning("no object files loaded, only command line created "
			"symbols and/or sections created from files will "
			"appear in the output file");
	    else
		fatal("no object files loaded");
	}

#ifdef DEBUG
	if(debug & 0x1)
	    print_object_list();
	if(debug & 0x2)
	    print_merged_sections("after pass1");
	if(debug & 0x4)
	    print_symbol_list("after pass1", TRUE);
	if(debug & 0x8)
	    print_undefined_list();
	if(debug & 0x10)
	    print_segment_specs();
	if(debug & 0x20)
	    print_load_fvmlibs_list();
	if(debug & 0x40)
	    print_fvmlib_segments();
	if(debug & 0x200){
	    print("Number of objects loaded = %d\n", nobjects);
	    print("Number of merged symbols = %d\n", nmerged_symbols);
	}
#endif DEBUG

	/*
	 * If there were any errors from pass1() then don't continue.
	 */
	if(errors != 0)
	    exit(1);

	/*
	 * Print which files are loaded if requested.
	 */
	if(whatsloaded == TRUE)
	    print_whatsloaded();

	/*
	 * Clean up any data structures not need for layout() or pass2().
	 */
	if(nsearch_dirs != 0){
	    free(search_dirs);
	    nsearch_dirs = 0;
	}

	/*
	 * Layout the output object file.
	 */
	layout();

	/*
	 * If there were any errors from layout() then don't continue.
	 */
	if(errors != 0)
	    exit(1);

	/*
	 * Clean up any data structures not need for pass2().
	 */
	free_pass1_symbol_data();
	if(ntrace_syms != 0){
	    free(trace_syms);
	    ntrace_syms = 0;
	}
	if(nundef_syms != 0){
	    free(undef_syms);
	    nundef_syms = 0;
	}

	/*
	 * Write the output object file doing relocation on the sections.
	 */
	if(trace)
	    print("%s: Pass 2\n", progname);
	pass2();

	exit(0);
}

/*
 * ispoweroftwo() returns TRUE or FALSE depending if x is a power of two.
 */
static
enum
bool
ispoweroftwo(
unsigned long x)
{
	if(x == 0)
	    return(TRUE);
	while((x & 0x1) != 0x1){
	    x >>= 1;
	}
	if((x & ~0x1) != 0)
	    return(FALSE);
	else
	    return(TRUE);
}

/*
 * getprot() returns the vm_prot for the specified string passed to it.  The
 * string may contain any of the following characters: 'r', 'w', 'x' and '-'
 * representing read, write, execute and no protections.  The pointer pointed
 * to by endp is set to the first character that is not one of the above
 * characters.
 */
static
vm_prot_t
getprot(
char *prot,
char **endp)
{
    vm_prot_t vm_prot;

	vm_prot = VM_PROT_NONE;
	while(*prot){
	    switch(*prot){
	    case 'r':
	    case 'R':
		vm_prot |= VM_PROT_READ;
		break;
	    case 'w':
	    case 'W':
		vm_prot |= VM_PROT_WRITE;
		break;
	    case 'x':
	    case 'X':
		vm_prot |= VM_PROT_EXECUTE;
		break;
	    case '-':
		break;
	    default:
		*endp = prot;
		return(vm_prot);
	    }
	    prot++;
	}
	*endp = prot;
	return(vm_prot);
}

/*
 * check_max_init_prot() checks to make sure that all protections in the initial
 * protection are also in the maximum protection.
 */
static
enum bool
check_max_init_prot(
vm_prot_t maxprot,
vm_prot_t initprot)
{
	if(((initprot & VM_PROT_READ)    && !(maxprot & VM_PROT_READ)) ||
	   ((initprot & VM_PROT_WRITE)   && !(maxprot & VM_PROT_WRITE)) ||
	   ((initprot & VM_PROT_EXECUTE) && !(maxprot & VM_PROT_EXECUTE)) )
	return(FALSE);
	return(TRUE);
}

/*
 * cleanup() is called by all routines handling fatal errors to remove the
 * output file if it had been created by the link editor and exit non-zero.
 */
static
void
cleanup(void)
{
	if(output_addr != NULL)
	    unlink(outputfile);
	exit(1);
}

/*
 * handler() is the routine for catching SIGINT and SIGTERM signals.
 * It cleans up and exit()'s non-zero.
 */
static
void
handler(
int sig)
{
	cleanup();
}
#endif !defined(RLD)

/*
 * allocate() is just a wrapper around malloc that prints and error message and
 * exits if the malloc fails.
 */
void *
allocate(
long size)
{
    void *p;

	if(size == 0)
	    return(NULL);
	if((p = malloc(size)) == NULL)
	    system_fatal("virtual memory exhausted (malloc failed)");
	return(p);
}

/*
 * reallocate() is just a wrapper around realloc that prints and error message
 * and exits if the realloc fails.
 */
void *
reallocate(
void *p,
long size)
{
	if(p == NULL)
	    return(allocate(size));
	if((p = realloc(p, size)) == NULL)
	    system_fatal("virtual memory exhausted (realloc failed)");
	return(p);
}

/*
 * round() rounds v to a multiple of r.
 */
long
round(
long v,
unsigned long r)
{
	r--;
	v += r;
	v &= ~(long)r;
	return(v);
}

#ifndef RLD
/*
 * All printing of all messages goes through this function.
 */
void
vprint(
const char *format,
va_list ap)
{
	vprintf(format, ap);
}
#endif !defined(RLD)

/*
 * The print function that just calls the above vprint() function.
 */
void
print(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
	vprint(format, ap);
	va_end(ap);
}


/*
 * Print the warning message.  This is non-fatal and does not set 'errors'.
 */
void
warning(
const char *format,
...)
{
    va_list ap;

	if(nowarnings == TRUE)
	    return;
	va_start(ap, format);
        print("%s: warning ", progname);
	vprint(format, ap);
        print("\n");
	va_end(ap);
}

/*
 * Print the error message and set the 'error' indication.
 */
void
error(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: ", progname);
	vprint(format, ap);
        print("\n");
	va_end(ap);
	errors = 1;
}

/*
 * Print the fatal error message, and exit non-zero.
 */
void
fatal(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: ", progname);
	vprint(format, ap);
        print("\n");
	va_end(ap);
	cleanup();
}

/*
 * Print the current object file name and warning message.
 */
void
warning_with_cur_obj(
const char *format,
...)
{
    va_list ap;

	if(nowarnings == TRUE)
	    return;
	va_start(ap, format);
        print("%s: warning ", progname);
	print_obj_name(cur_obj);
	vprint(format, ap);
        print("\n");
	va_end(ap);
}

/*
 * Print the current object file name and error message, set the non-fatal
 * error indication.
 */
void
error_with_cur_obj(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: ", progname);
	print_obj_name(cur_obj);
	vprint(format, ap);
        print("\n");
	va_end(ap);
	errors = 1;
}

/*
 * Print the error message along with the system error message, set the
 * non-fatal error indication.
 */
void
system_error(
const char *format,
...)
{
    va_list ap;
    int errnum;

	errnum = errno;
	va_start(ap, format);
        print("%s: ", progname);
	vprint(format, ap);
	print(" (%s, errno = %d)\n", strerror(errnum), errnum);
	va_end(ap);
	errors = 1;
}

/*
 * Print the fatal message along with the system error message, and exit
 * non-zero.
 */
void
system_fatal(
const char *format,
...)
{
    va_list ap;
    int errnum;

	errnum = errno;
	va_start(ap, format);
        print("%s: ", progname);
	vprint(format, ap);
	print(" (%s, errno = %d)\n", strerror(errnum), errnum);
	va_end(ap);
	cleanup();
}

/*
 * Print the fatal error message along with the mach error string, and exit
 * non-zero.
 */
void
mach_fatal(
kern_return_t r,
char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: ", progname);
	vprint(format, ap);
	print(" (%s)\n", mach_error_string(r));
	va_end(ap);
	cleanup();
}

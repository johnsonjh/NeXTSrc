#include <stdio.h>
#include <sys/file.h>
#include <varargs.h>
#include <a.out.h>
#include <sys/loader.h>
#include <signal.h>
#include <sys/wait.h>
#include <ar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mach.h>
#include <errno.h>
#define NIL	0

extern int errno;		/* UNIX system call error number */
extern char *sys_errlist[];	/* UNIX system call error list */
extern int sys_nerr;		/* number of strings in error list */

extern char *malloc(), *realloc(), *mktemp();

/* These are set from the command line arguments */
static char *progname;	/* name of the program for error messages (argv[0]) */
static long nflag;	/* do not create the indirect objects */
static long mflag = 1;	/* produce mach-O object files flag */
static long dflag;	/* leave the created object files around */
static long vflag;	/* verbose flag for commands run by run() */
static char
     *list_filename,	/* the file name that contains the list of symbols */
     *input_file,	/* the input file */
     *output_file;	/* the output file */

/*
 * The pointer to the archive header of the current member being processed and
 * the member_name (null terminated and trailing blanks stripped).  If the
 * input file is NOT an archive the archive header pointer will be NULL.
 */
static struct ar_hdr *ar_hdrp;
static char member_name[sizeof(ar_hdrp->ar_name) + 1];

/*
 * The temporary directory to create the output archive's objects in and the
 * index to the basename when this directory name has a filename added to it.
 */
static char *tempdir,
	    mktemp_string[] = "indrXXXXXX";
static long basename_index;

/*
 * Data structures to hold the symbol names (both the indirect and undefined
 * symbol names) from the list file.
 */
struct symbol {
    char *name;			/* name of the symbol */
    long type;			/* type of this symbol (N_INDR or N_UNDF) */
    char *indr;			/* name for indirection if N_INDR type */
    struct symbol *next;	/* next symbol in the hash chain */
};
/* The symbol hash table is hashed on the name field */
#define SYMBOL_HASH_SIZE	100
static struct symbol *symbol_hash[SYMBOL_HASH_SIZE];

/*
 * Data structures to hold the object names (both the names from the archive
 * and the names created for indirect symbols).  If this is for indirect symbols
 * then the indr and undef fields are non-zero.
 */
struct object {
    char *basename;		/* the base name of the object file */
    char *filename;		/* the name of the temporary object file */
    char *indr;			/* the name of the indirect symbol or zero */
    char *undef;		/* the name of the undefined symbol or zero */
    struct object *next;	/* the next object in the hash chain */
};
/* The object hash table is hashed on the basename field */
#define OBJECT_HASH_SIZE	100
static struct object *object_hash[OBJECT_HASH_SIZE];

/*
 * Ordered lists of the objects as they will be put in the archive to be
 * created.  The objects created for indirect symbols are placed in first in
 * the archive in the order the symbol names appear in the list file (so to get
 * known order.  Then all of the original objects from the old archive are
 * placed in the archive preserving their order.
 */
#define INITIAL_LIST_SIZE	100
struct list {
    void **list;	/* the order list */
    long used;		/* the number used in the list */
    long size;		/* the current size of the list */
};
static struct list archive_list, indr_list, run_list;

/*
 * All the structures for a mach-O file (less the string table).  The only
 * things that needed to be set at runtime are st.strsize and the string table
 * indexes for the symbols.
 */
static struct mach {
    struct mach_header mh;
    struct symtab_command st;
    struct nlist indr, undef;
} mach = {
       {MH_MAGIC,
	CPU_TYPE_MC680x0,
	CPU_SUBTYPE_MC68030,
	MH_OBJECT,
	1,
	sizeof(struct symtab_command),
	0},

       {LC_SYMTAB,
	sizeof(struct symtab_command),
	sizeof(struct mach_header) + sizeof(struct symtab_command),
	2,
	sizeof(struct mach_header) + sizeof(struct symtab_command) +
	    2 * sizeof(struct nlist),
	0},

       { {0},
	N_INDR | N_EXT,
	NO_SECT,
	0,
	0},

       { {0},
	N_UNDF | N_EXT,
	NO_SECT,
	0,
	0}
};

/*
 * All the structures for a a.out file (less the string table).  The only
 * things that needed to be set at runtime are the string table indexes for
 * the symbols.
 */
static struct aout {
    struct exec header;
    struct nlist indr, undef;
} aout = {
       {2, /* really M_68030, but the constant doesn't exist */
	OMAGIC,
	0,
	0,
	0,
	2 * sizeof(struct nlist),
	0,
	0,
	0},

       { {0},
	N_INDR | N_EXT,
	NO_SECT,
	0,
	0},

       { {0},
	N_UNDF | N_EXT,
	NO_SECT,
	0,
	0}
};
/*
 * Pointers to the indirect and undefined symbols.  These are set to either
 * the symbols in mach or aout depending on the -m flag.
 */
static struct nlist *indrp, *undefp;

/*
 * The string table maintained by start_string_table(), add_to_string_table()
 * and end_string_table();
 */
#define INITIAL_STRING_TABLE_SIZE	100
static struct {
    char *strings;
    long size;
    long index;
} string_table;

static void usage();
static void handler(int sig);
static void cleanup(long exit_status);
static void process_list();
static void translate_input();
static void copy_nonobject(void *addr, long size, long mode);
static void translate_object(void *addr, long size, long mode);
static void make_indr_objects();
static void make_new_archive();
static void enter_symbol(char *name, long type, char *indr);
static struct symbol *lookup_symbol(char *name);
static void enter_object(char *basename, char *filename, char *indr,
    			 char *undef, struct list *list);
static void start_string_table();
static long add_to_string_table(char *p);
static void end_string_table();
static void add_list(struct list *list, void *item);
static long run();
static long hash_string(char *key);
static long round(long v, unsigned long r);
static char *makestr(/* char *, ... */);
static void *allocate(long size);
static void *reallocate(void *p, long size);
static void error(/* char *fmt, args, ... */);
static void fatal_input(/* char *fmt, args, ... */);
static void fatal(/* char *fmt, args, ... */);
#if 0
static void fatal(char *fmt, ...);
#endif
static void perror_fatal(/* char *fmt, args, ... */);
static void mach_fatal(/* kern_return_t r, char *fmt, args, ... */);

/*
 * This program takes the following options:
 *
 *	% progname [-mdv] list_filename input_file output_file
 *
 * It builds the output file by translating each symbol name listed in
 * list file to the same name with and underbar prepended to it in all the
 * objects in the input file.  Then it if the input file is an archive and the
 * -n flag is not specified then it creates an object for each of these
 * symbols with an indirect symbol for the symbol name with an underbar and
 * adds that to the output archive.
 *
 * The -m flag tells to produce mach-O relocatable files for the object files
 *	  with the indirect symbols.
 * The -d flag is for debugging which leaves the created objects around.
 * The -v flag is for verbose printing of commands that are exec'ed.
 * The -n flag is to suppress creating the indirect objects.
 */
void
main(
    int argc,
    char *argv[],
    char *envp[])
{
	long i, j, no_flags_left;

	progname = argv[0];
	signal(SIGINT, handler);

	/*
	 * Parse the flags.
	 */
	no_flags_left = FALSE;
	for(i = 1; i < argc ; i++){
	    if(argv[i][0] != '-' || no_flags_left){
		if(list_filename == NIL)
		    list_filename = argv[i];
		else if(input_file == NIL)
		    input_file = argv[i];
		else if(output_file == NIL)
		    output_file = argv[i];
		else
		    usage();
		continue;
	    }
	    if(argv[i][1] == '\0'){
		no_flags_left = TRUE;
		continue;
	    }
	    for(j = 1; argv[i][j] != '\0'; j++){
		switch(argv[i][j]){
		case 'n':
		    nflag = TRUE;
		    break;
		case 'm':
		    mflag = TRUE;
		    break;
		case 'd':
		    dflag = TRUE;
		    break;
		case 'v':
		    vflag = TRUE;
		    break;
		default:
		    error("unknown flag -%c\n", progname, argv[i][j]);
		    usage();
		}
	    }
	}
	if(list_filename == NIL ||
	   input_file == NIL || output_file == NIL)
	    usage();

	/*
	 * Set the pointers to the indirect and undefined symbols for the type
	 * of object files that will be created.
	 */
	if(mflag){
	    indrp = &(mach.indr);
	    undefp = &(mach.undef);
	}
	else{
	    indrp = &(aout.indr);
	    undefp = &(aout.undef);
	}
	/*
	 * Set up a temporary directory name to use for creating objects.  Use a
	 * known directory if the debugging flag is on.
	 */
	if(dflag)
	    tempdir = "bozodir";
	else{
	    tempdir = mktemp(mktemp_string);
	}
	basename_index = strlen(tempdir) + strlen("/");

	/*
	 * Now do the work.
	 */

	/* process the list of symbols and create the data structures */
	process_list();

	/* translate the symbols in the input file */
	translate_input();

	/*
	 * Make the objects for the indirect symbols in the -n flag is not
	 * specified and the input file was an archive.
	 */
	if(nflag == FALSE && ar_hdrp != NIL)
	    make_indr_objects();

	/*
	 * Make the new archive with translated objects and created objects
	 * (if no -n flag) if the input file was an archive
	 */
	if(ar_hdrp != NIL)
	    make_new_archive();

	/* clean up and exit with a zero status */
	cleanup(0);
}

/*
 * Print the current usage message and exit non-zero.
 */
static
void
usage()
{
	fprintf(stderr, "Usage: %s [-nmdv] <filename of symbol list> "
		"<input file> <output file>\n", progname);
	exit(1);
}

/*
 * The signal handler routine to clean up on interrupt.
 */
static
void
handler(
    int sig)
{
	cleanup(1);
}

/*
 * This routine cleans up the files that were created temporary files and
 * exits with the exit_status.
 */
static
void
cleanup(
    long exit_status)
{
	if(!dflag){
	    if(tempdir != (char *)0 && ar_hdrp != NIL){
		run_list.used = 0;
		add_list(&run_list, "rm");
		add_list(&run_list, "-r");
		add_list(&run_list, tempdir);
		if(run()){
		    error("internal remove of temporary directory: %s and it's "
			  "contents failed", tempdir);
		    exit_status = 1;
		}
	    }
	}
	exit(exit_status);
}

/*
 * process the symbols listed in list_filename.  This enters each symbol as an
 * indirect symbol into the symbol hash table as well as the undefined symbol
 * for the indirection.  Then it creates the name of the object file that will
 * be used to put these symbols in.
 */ 
static
void
process_list()
{
	FILE *list;
	char buf[BUFSIZ], *symbol_name, *_symbol_name, *object_filename;
	long len, symbol_number;

	if((list = fopen(list_filename, "r")) == NULL)
	    perror_fatal("can't open: %s", list_filename);

	symbol_number = 0;
	buf[BUFSIZ-1] = '\0';
	while(fgets(buf, BUFSIZ-1, list) != NULL){
	    len = strlen(buf);
	    if(buf[len-1] != '\n')
		fatal("symbol name: %s too long from file: %s", buf,
		       list_filename);
	    buf[len-1] = '\0';

	    symbol_name = makestr(buf, NIL);
	    _symbol_name = makestr("_", symbol_name, NIL);
	    enter_symbol(symbol_name, N_INDR, _symbol_name);
	    enter_symbol(_symbol_name, N_UNDF, NIL);

	    if(nflag == FALSE){
		sprintf(buf, "%05d", symbol_number++);
		object_filename = makestr(tempdir, "/", "INDR", buf, ".o", NIL);
		enter_object(object_filename + basename_index,
			     object_filename,
			     symbol_name,
			     _symbol_name,
			     &indr_list);
	    }
	}
	if(ferror(list))
	    perror_fatal("can't read: %s", list_filename);
	fclose(list);
}

/*
 * Translate the objects in the input file by changing the external symbols
 * that match indirect symbols in the symbol hash table to the symbol that the
 * indirect symbol is for.
 *
 * NOTE: The input is memory mapped and this will ONLY work on machines that
 * will allow long word (32 bit) accesses on short word (16 bit) boundaries.
 * Given the long words in the object file headers and that archive members'
 * sizes are only required to be multiples of shorts.  It could still work if
 * all the members' sizes are multiples of longs.
 */
static
void
translate_input()
{
	int fd, oumask;
	struct stat stat_buf;
	kern_return_t r;
	long i, size, offset, member_size, member_mode;
	void *addr, *member_addr;
	char *armagp;

	/* Open the input file and map it in */
	if((fd = open(input_file, O_RDONLY)) == -1)
	    perror_fatal("can't open input file: %s", input_file);
	if(fstat(fd, &stat_buf) == -1)
	    perror_fatal("Can't stat input file: %s", input_file);
	size = stat_buf.st_size;
	if((r = map_fd(fd, 0, &addr, TRUE, size)) != KERN_SUCCESS)
	    mach_fatal(r, "Can't map input file: %s", input_file);
	close(fd);

	/*
	 * This test makes the assumption that SARMAG < sizeof(struct exec)
	 * and SARMAG < sizeof(struct mach_header).  Also the assumption is
	 * made if this test passes that the next test can be made to determine
	 * if this is an object file or not.
	 */
	if(SARMAG > size)
	    fatal("truncated or malformed input file: %s", input_file);
	if(*((long *)addr) == MH_MAGIC || !N_BADMAG(*((struct exec *)addr))){
	    oumask = umask(0);
	    translate_object(addr, size, stat_buf.st_mode & 0777);
	    (void)umask(oumask);
	    return;
	}
	armagp = (char *)addr;
	if(strncmp(armagp, ARMAG, SARMAG) != 0)
	    fatal("input file: %s is not an object file or an archive",
		  input_file);

	/*
	 * Create the temporary directory to use for creating objects from an
	 * archive.
	 */
	run_list.used = 0;
	add_list(&run_list, "mkdir");
	add_list(&run_list, tempdir);
	if(run())
	    if(dflag)
		error("internal make of directory: %s failed", tempdir);
	    else
		fatal("internal make of directory: %s failed", tempdir);

	offset = SARMAG;
	if(size == offset)
	    perror_fatal("input file: %s is empty", input_file);

	oumask = umask(0);
	member_name[sizeof(ar_hdrp->ar_name)] = '\0';
	while(size != offset){
	    if(offset + sizeof(struct ar_hdr) > size)
		fatal("truncated or malformed archive: %s (archive header at "
		      "offset %d would extend past the end of the file)",
		      input_file, offset);

	    ar_hdrp = (struct ar_hdr *)(addr + offset);
	    for(i = sizeof(ar_hdrp->ar_name) - 1 ; i >= 0 ; i--){
		if(ar_hdrp->ar_name[i] != ' '){
		    i++;
		    break;
		}
	    }
	    if(i > 0)
		ar_hdrp->ar_name[i] = '\0';
	    strncpy(member_name, ar_hdrp->ar_name, sizeof(ar_hdrp->ar_name));

	    member_size = atol(ar_hdrp->ar_size);
	    if(offset + sizeof(struct ar_hdr) + member_size > size)
		fatal("truncated or malformed archive: %s (size of member: %s "
		      "would extend past the end of the file)", input_file,
		      member_name);
	    member_addr = (void *)(addr + offset + sizeof(struct ar_hdr));
	    sscanf(ar_hdrp->ar_mode, "%o", &member_mode);
	    member_mode &= 0777;

	    if(strncmp(member_name, "__.SYMDEF", sizeof("__.SYMDEF") - 1) != 0){
		if(member_size < sizeof(long) ||
		   (*((long *)member_addr) != MH_MAGIC &&
		   (N_BADMAG(*((struct exec *)member_addr)))) )
		    copy_nonobject(member_addr, member_size, member_mode);
		else
		    translate_object(member_addr, member_size, member_mode);
	    }

	    /* move on to the next member */
	    offset += sizeof(struct ar_hdr) + round(member_size, sizeof(short));

	    /*
	     * If this check does not fail then the assumptions of needing to
	     * access longs on short boundaries is not required.
	     */
	    if(dflag && round(member_size, sizeof(short)) % sizeof(long) != 0)
		error("warning size of: %s(%s) not a multiple of sizeof(long)",
		      input_file, member_name);
	}
	(void)umask(oumask);
}

/*
 * Copy the contents of this non-object file into a temporary file so that it
 * gets placed in the output archive unchanged.
 */
static
void
copy_nonobject(
    void *addr,
    long size,
    long mode)
{
	char *object_filename;
	int fd;

	object_filename = makestr(tempdir, "/", member_name, NIL);
	enter_object(object_filename + basename_index,
		     object_filename, NIL, NIL, &archive_list);

	if((fd = open(object_filename, O_WRONLY | O_CREAT, mode)) == -1)
	    perror_fatal("can't create: %s", object_filename);
	if(write(fd, addr, size) != size)
	    perror_fatal("can't write: %s", object_filename);
	if(close(fd) == -1)
	    perror_fatal("can't close: %s", object_filename);
}

/*
 * translate the one object's symbols which match the symbols for which indirect
 * symbols are to be created.  The object file name is either the member_name if
 * the input file is an archive or the output_file if the input is an object
 * file.  The contents of the object is at addr and is size in length.  Mode is  * the mode of the file to be created.
 */
static
void
translate_object(
    void *addr,
    long size,
    long mode)
{

	long mach, symoff, stroff, segoff, nsyms, strsize, segsize, i, j;
	struct mach_header *mhp;
	struct load_command *load_commands, *lcp;
	struct symtab_command *stp;
	struct symseg_command *ssp;
	struct exec *execp;

	char *object_filename;
	int fd;
	struct nlist *symbols, *nlistp;
	char *strings;
	struct symbol *sp;

	long new_nsyms;
	struct nlist *new_symbols;

	mach = *((long *)addr) == MH_MAGIC;
	if(mach){
	    if(sizeof(struct mach_header) > size)
		fatal_input("truncated or malformed object (mach header would "
			    "extend past the end of the file) in: ");
	    mhp = (struct mach_header *)addr;
	    if(mhp->filetype != MH_OBJECT)
		fatal_input("file not a relocatable object (filetype != "
			    "MH_OBJECT) in: ");
	    if(mhp->sizeofcmds + sizeof(struct mach_header) > size)
		fatal_input("truncated or malformed object (load commands "
			    "would extend past the end of the file) in :");
	    load_commands = (struct load_command *)((char *)addr +
				sizeof(struct mach_header));
	    stp = NIL;
	    ssp = NIL;
	    lcp = load_commands;
	    for(i = 0; i < mhp->ncmds; i++){
		if(lcp->cmdsize % sizeof(long) != 0)
		    fatal_input("load command %d size not a multiple of "
				"sizeof(long) in: ", i);
		if(lcp->cmdsize <= 0)
		    fatal_input("load command %d size is less than or equal to "
				"zero in: ", i);
		if((char *)lcp + lcp->cmdsize >
		   (char *)load_commands + mhp->sizeofcmds)
		    fatal_input("load command %d extends past end of all load "
				"commands in: ", i);
		if(lcp->cmd == LC_SYMTAB){
		    if(lcp->cmdsize != sizeof(struct symtab_command))
			fatal_input("load command %d incorrect size for symtab "
				    "command in: ", i);
		    if(stp == NIL)
			stp = (struct symtab_command *)lcp;
		    else
			fatal_input("more that one symtab load command in: ");
		}
		if(lcp->cmd == LC_SYMSEG){
		    if(lcp->cmdsize != sizeof(struct symseg_command))
			fatal_input("load command %d incorrect size for symseg "
				    "command in: ", i);
		    if(ssp == NIL)
			ssp = (struct symseg_command *)lcp;
		    else
			fatal_input("more that one symseg load command in: ");
		}
		lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	    }
	    if(stp == NIL)
		fatal_input("no symtab load command in: ");
	    symoff = stp->symoff;
	    nsyms = stp->nsyms;
	    stroff = stp->stroff;
	    strsize = stp->strsize;
	    if(ssp != NIL){
		segoff = ssp->offset;
		segsize = ssp->size;
	    }
	    else{
		segoff = stroff + strsize;
		segsize = 0;
	    }
	}
	else{
	    if(sizeof(struct exec) > size)
		fatal_input("truncated or malformed object (exec header "
			    "would extend past the end of the file) in: ");
	    execp = (struct exec *)addr;
	    symoff = N_SYMOFF(*execp);
	    nsyms = execp->a_syms / sizeof(struct nlist);
	    stroff = N_STROFF(*execp);
	    if(stroff + sizeof(long) > size)
		fatal_input("truncated or malformed object (string table size "
			    "would extend past the end of the file) in: ");
	    strsize = *(long *)((char *)addr + stroff);
	    segoff = stroff + strsize;
	    segsize = size - segoff;
	}

	if(symoff + nsyms * sizeof(struct nlist) > size)
	    fatal_input("truncated or malformed object (symbol table would "
		  "extend past the end of the file) in: ");
	if(stroff + strsize > size)
	    fatal_input("truncated or malformed object (string table would "
		  "extend past the end of the file) in: ");
	if(segoff + segsize > size)
	    fatal_input("truncated or malformed object (gdb symbol segments "
		  "would extend past the end of the file) in: ");

	if(symoff + nsyms * sizeof(struct nlist) > stroff)
	    fatal_input("malformed object (symbol table is does not precede "
		  "string table in the file) in: ");
	if(stroff + strsize > segoff)
	    fatal_input("malformed object (string table is does not precede "
		  "gdb symbol segments in the file) in: ");

	if(ar_hdrp != NIL){
	    object_filename = makestr(tempdir, "/", member_name, NIL);
	    enter_object(object_filename + basename_index,
			 object_filename, NIL, NIL, &archive_list);
	}
	else{
	    object_filename = output_file;
	}

	if((fd = open(object_filename, O_WRONLY | O_CREAT, mode)) == -1)
	    perror_fatal("can't create: %s", object_filename);
	if(write(fd, addr, symoff) != symoff)
	    perror_fatal("can't write: %s", object_filename);

	start_string_table();
	symbols = (struct nlist *)((char *)addr + symoff);
	nlistp = symbols;
	strings = (char *)addr + stroff;
	for(i = 0; i < nsyms; i++){
	    if(nlistp->n_type & N_EXT){
		if(nlistp->n_un.n_strx){
		    if(nlistp->n_un.n_strx > 0 &&
		       nlistp->n_un.n_strx < strsize){
			sp = lookup_symbol(strings + nlistp->n_un.n_strx);
			if(sp != NIL){
			    if(sp->type == N_UNDF)
				fatal_input("symbol name: %s conflicts with "
					    "symbol name created for "
					    "indirection in: ", sp->name);
			    nlistp->n_un.n_strx = add_to_string_table(sp->indr);
			}
			else{
			    nlistp->n_un.n_strx = add_to_string_table(
				    strings + nlistp->n_un.n_strx);
			}
		    }
		    else
			fatal_input("bad string table index in symbol %d in: ",
				    i);
		}
	    }
	    else{
		if(nlistp->n_un.n_strx){
		    if(nlistp->n_un.n_strx > 0 && nlistp->n_un.n_strx < strsize)
			nlistp->n_un.n_strx = add_to_string_table(
				strings + nlistp->n_un.n_strx);
		    else
			fatal_input("bad string table index in symbol %d in: ",
				    i);
		}
	    }
	    nlistp++;
	}
	/*
	 * This is a hack to keep the full reference object of a host shared
	 * library correct when it is processed by this program.  To do this
	 * The name of the object, "__.FVMLIB_REF", is check for and if this 
	 * is it an undefined symbol for each indirect symbol is added so to
	 * cause all the indrect objects to be loaded.
	 */
	new_symbols = NULL;
	if(strcmp(member_name, "__.FVMLIB_REF") == 0){
	    new_nsyms = 0;
	    for(i = 0; i < SYMBOL_HASH_SIZE; i++){
		sp = symbol_hash[i];
		while(sp != NULL){
		    if(sp->type == N_INDR)
			new_nsyms++;
		    sp = sp->next;
		}
	    }
	    new_symbols = allocate((nsyms + new_nsyms) * sizeof(struct nlist));
	    memcpy(new_symbols, symbols, nsyms * sizeof(struct nlist));
	    j = nsyms;
	    for(i = 0; i < SYMBOL_HASH_SIZE; i++){
		sp = symbol_hash[i];
		while(sp != NULL){
		    if(sp->type == N_INDR){
			new_symbols[j].n_un.n_strx =
						add_to_string_table(sp->name);
			new_symbols[j].n_type = N_UNDF | N_EXT;
			new_symbols[j].n_sect = NO_SECT;
			new_symbols[j].n_desc = 0;
			new_symbols[j].n_value = 0;
			j++;
		    }
		    sp = sp->next;
		}
	    }
	    symbols = new_symbols;
	    nsyms += new_nsyms;
	}
	end_string_table();

	if(write(fd, (char *)symbols, nsyms * sizeof(struct nlist)) !=
	   nsyms * sizeof(struct nlist))
	    perror_fatal("can't write symbol table to: %s", object_filename);
	if(write(fd, string_table.strings, string_table.index) !=
	   string_table.index)
	    perror_fatal("can't write string table to: %s", object_filename);
	if(write(fd, (char *)addr + segoff, segsize) != segsize)
	    perror_fatal("can't write gdb symbol segment to: %s",
			 object_filename);
	if(mach){
	    if(stp != NIL){
		stp->nsyms = nsyms;
		stroff = stp->symoff + nsyms * sizeof(struct nlist);
		stp->stroff = stroff;
		stp->strsize = string_table.index;
	    }
	    if(ssp != NIL)
		ssp->offset = stroff + string_table.index;
	    lseek(fd, sizeof(struct mach_header), L_SET);
	    if(write(fd, (char *)load_commands, mhp->sizeofcmds) !=
	       mhp->sizeofcmds)
		perror_fatal("can't write load commands to: %s",
			     object_filename);
	}

	if(new_symbols != NULL)
	    free(new_symbols);

	if(close(fd) == -1)
	    perror_fatal("can't close: %s", object_filename);
}

/*
 * make_indr_objects writes an object file for each symbol name that was in the
 * list file.  The object contains an indirect symbol for the symbol_name.  The
 * symbol name used as the indirect symbol has been constructed by prepending
 * an underbar to symbol_name previously in process_list().
 */
static
void
make_indr_objects()
{
	long i;
	struct object *op;
	FILE *object;

	for(i = 0; i < indr_list.used; i++){
	    op = (struct object *)indr_list.list[i];
	    if((object = fopen(op->filename, "w")) == NULL)
		perror_fatal("can't create: %s", op->filename);

	    start_string_table();
	    undefp->n_un.n_strx = add_to_string_table(op->undef);
	    indrp->n_un.n_strx = add_to_string_table(op->indr);
	    indrp->n_value = undefp->n_un.n_strx;
	    end_string_table();

	    if(mflag){
		mach.st.strsize = string_table.index;
		if(fwrite((char *)&mach, sizeof(struct mach), 1, object) != 1)
		    perror_fatal("can't write mach headers and symbol table to "
				 "object file: %s", op->filename);
	    }
	    else{
		*((long *)string_table.strings) = string_table.index;
		if(fwrite((char *)&aout, sizeof(struct aout), 1, object) != 1)
		    perror_fatal("can't write header and symbol table to "
				 "object file: %s", op->filename);
	    }

	    if(fwrite(string_table.strings, 1, string_table.index, object)
	       != string_table.index)
		perror_fatal("can't write string table to object file: %s",
			     op->filename);

	    if(fclose(object) == EOF)
		perror_fatal("can't close object file: %s", op->filename);
	}
}

/*
 * archive all the object created and ranlib it.
 */
static
void
make_new_archive()
{
	long i;
	struct object *op;
	FILE *object;
	
	errno = 0;
	if ((unlink(output_file) == -1) && (errno != ENOENT))
	    fatal("can't remove: %s", output_file);

	run_list.used = 0;
	add_list(&run_list, "ar");
	if(vflag)
	    add_list(&run_list, "rcv");
	else
	    add_list(&run_list, "rc");
	add_list(&run_list, output_file);

	if(nflag == FALSE){
	    for(i = 0; i < indr_list.used; i++){
		op = (struct object *)indr_list.list[i];
		add_list(&run_list, op->filename);
	    }
	}
	for(i = 0; i < archive_list.used; i++){
	    op = (struct object *)archive_list.list[i];
	    add_list(&run_list, op->filename);
	}
	if(run())
	    fatal("internal archive of: %s failed", output_file);

	run_list.used = 0;
	add_list(&run_list, "ranlib");
	add_list(&run_list, "-s");
	add_list(&run_list, output_file);
	if(run())
	    fatal("internal ranlib of: %s failed", output_file);
}

/*
 * enter a symbol and it's type into the symbol hash table checking for
 * duplicates.  Duplicates cause a fatal error to be printed.
 */
static
void
enter_symbol(
    char *name,
    long type,
    char *indr)
{
	long hash_key;
	struct symbol *sp;

	hash_key = hash_string(name) % SYMBOL_HASH_SIZE;
	sp = symbol_hash[hash_key];
	while(sp != NIL){
	    if(strcmp(name, sp->name) == 0){
		fatal("to create %s symbol: %s would conflict with also to be "
		      "created %s symbol: %s",
		      type == N_INDR ? "indirect" : "undefined", name,
		      sp->type == N_INDR ? "indirect" : "undefined", sp->name);
	    }
	    sp = sp->next;
	}
	sp = (struct symbol *)allocate(sizeof(struct symbol));
	sp->name = name;
	sp->type = type;
	sp->indr = indr;
	sp->next = symbol_hash[hash_key];
	symbol_hash[hash_key] = sp;
}

/*
 * lookup a symbol name in the symbol hash table returning a pointer to the
 * symbol structure for it.  A nil pointer is returned if not found.
 */
static
struct symbol *
lookup_symbol(
    char *name)
{
	long hash_key;
	struct symbol *sp;

	hash_key = hash_string(name) % SYMBOL_HASH_SIZE;
	sp = symbol_hash[hash_key];
	while(sp != NIL){
	    if(strcmp(name, sp->name) == 0){
		return(sp);
	    }
	    sp = sp->next;
	}
	return(NIL);
}

/*
 * enter an object and it's informaton into the object hash table checking for
 * the basename not being too long and for duplicate basenames.  Then add the
 * created object structure to the specified list.
 */
static
void
enter_object(
    char *basename,
    char *filename,
    char *indr,
    char *undef,
    struct list *list)
{
	long hash_key;
	struct object *op;
	struct ar_hdr ar_hdr;

	if(strlen(basename) > sizeof(ar_hdr.ar_name)){
	    if(indr != NIL)
		fatal("object file base name: %s too long to fit in archive "
		      "header (object name created from indirect symbol %s)",
		      basename, indr);
	    else
		fatal("object file base name: %s too long to fit in archive "
		      "header", basename);
	}
	hash_key = hash_string(basename) % OBJECT_HASH_SIZE;
	op = object_hash[hash_key];
	while(op != NIL){
	    if(strcmp(basename, op->basename) == 0){
		if(indr != NIL || op->indr != NIL)
		    fatal("more than one object with the same base name: %s "
			  "caused by indirect symbol %s", basename,
			  indr != NIL ? indr : op->indr);
		else
		    fatal("more than one object with the same base name: %s",
			  basename);
	    }
	    op = op->next;
	}
	op = (struct object *)allocate(sizeof(struct object));
	op->basename = basename;
	op->filename = filename;
	op->indr = indr;
	op->undef = undef;
	op->next = object_hash[hash_key];
	object_hash[hash_key] = op;

	add_list(list, (void *)op);
}

/*
 * This routine is called before calls to add_to_string_table() are made to
 * setup or reset the string table structure.  The first four bytes string
 * table are zeroed and the first string is placed after that.  This allows
 * the length to be stored there if needed.
 */
static
void
start_string_table()
{
	if(string_table.size == 0){
	    string_table.size = INITIAL_STRING_TABLE_SIZE;
	    string_table.strings = (char *)allocate(string_table.size);
	}
	*((long *)string_table.strings) = 0;
	string_table.index = sizeof(long);
}

/*
 * This routine adds the specified string to the string table structure and
 * returns the index of the string in the table.
 */
static
long
add_to_string_table(
    char *p)
{
	long len, index;

	len = strlen(p) + 1;
	if(string_table.size < string_table.index + len){
	    string_table.strings = (char *)reallocate(string_table.strings,
						      string_table.size * 2);
	    string_table.size *= 2;
	}
	index = string_table.index;
	strcpy(string_table.strings + string_table.index, p);
	string_table.index += len;
	return(index);
}

/*
 * This routine is called after all calls to add_to_string_table() are made to
 * round off the size of the string table.  It zero the rounded bytes and sets
 * the first four bytes string table to its length;
 * the length to be stored there if needed.
 */
static
void
end_string_table()
{
	long length;

	length = round(string_table.index, sizeof(long));
	bzero(string_table.strings + string_table.index,
	      length - string_table.index);
	*((long *)string_table.strings) = length;
	string_table.index = length;
}

/*
 * Add the item to the specified list.  Lists can be reused just by setting
 * list->used to zero.  The item after the last item is allways set to NIL
 * so that list->list can be used for things like execv() calls directly.
 */
static
void
add_list(
    struct list *list,
    void *item)
{
	if(list->used + 1 >= list->size){
	    if(list->size == 0){
		list->list = allocate(INITIAL_LIST_SIZE * sizeof(void *));
		list->size = INITIAL_LIST_SIZE;
		list->used = 0;
	    }
	    else{
		list->list = reallocate(list->list,
					list->size * 2 * sizeof(void *));
		list->size *= 2;
	    }
	}
	list->list[list->used++] = item;
	list->list[list->used] = NIL;
}

/*
 * run() does an exec using the run_list that has been created.
 * A zero return value indicates success.
 */
static
long
run()
{
    char *cmd, **ex, **p;
    int forkpid, waitpid, termsig;
    union wait waitstatus;
    extern char **environ;

	/*
	 * This is a very bad hack to give as much argument space to the command
	 * line arguments (the libsys-22 with librld overflowed this).
	 */
	environ[0] = 0;

	ex = (char **)run_list.list;
	cmd = (char *)(run_list.list[0]);

	if(vflag){
	    fprintf(stderr, "+ %s ", cmd);
	    p = &(ex[1]);
	    while(*p != (char *)0)
		    fprintf(stderr, "%s ", *p++);
	    fprintf(stderr, "\n");
	}

#ifndef NORUN
	forkpid = fork();
	if(forkpid == -1)
	    perror_fatal("can't fork a new process to run: %s", cmd);

	if(forkpid == 0){
	    if(execvp(cmd, ex) == -1)
		perror_fatal("can't find or exec: %s", cmd);
	}
	else{
	    waitpid = wait(&waitstatus);
	    if(waitpid == -1)
		perror_fatal("wait on forked process %d failed", forkpid);
	    termsig = waitstatus.w_termsig;
	    if(termsig != 0 && termsig != SIGINT)
		fatal("fatal error in %s", cmd);
	    return(waitstatus.w_retcode != 0 || termsig != 0);
	}
#else NORUN
	return(0);
#endif NORUN
}

/*
 * A hash function used for converting a string into a single number.  It is
 * then usually mod'ed with the hash table size to get an index into the hash
 * table.
 */
static
long
hash_string(
    char *key)
{
	char *cp;
	long k;

	cp = key;
	k = 0;
	while(*cp)
	    k = (((k << 1) + (k >> 14)) ^ (*cp++)) & 0x3fff;
	return(k);
}

/*
 * Round v to a multiple of r.
 */
static
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

/*
 * makestr is passed a variable number of strings and returns a pointer to
 * an allocated area that contains the concatination of these strings.
 */
static
char *
makestr(va_alist)
va_dcl
{
    va_list ap;
    long len;
    char *p, *q, *r;

	len = 0;
	va_start(ap);
	p = va_arg(ap, char *);
	while(p != NIL){
	    len += strlen(p);
	    p = va_arg(ap, char *);
	}
	va_end(ap);

	p = (char *)allocate(len + 1);
	q = p;
	va_start(ap);
	r = va_arg(ap, char *);
	while(r != NIL){
	    strcpy(q, r);
	    q += strlen(r);
	    r = va_arg(ap, char *);
	}
	va_end(ap);
	return(p);
}

/*
 * allocate is just a wrapper around malloc that prints and error message and
 * exits if the malloc fails.
 */
static
void *
allocate(
    long size)
{
	void *p;

	if((p = (void *)malloc(size)) == (char *)0)
	    perror_fatal("virtual memory exhausted (malloc failed)");
	return(p);
}

/*
 * reallocate is just a wrapper around realloc that prints and error message and
 * exits if realloc fails.
 */
static
void *
reallocate(
    void *p,
    long size)
{
	if((p = realloc(p, size)) == (char *)0)
	    perror_fatal("virtual memory exhausted (realloc failed)");
	return(p);
}

/*
 * Print the error message and return to the caller.
 */
static
void
error(va_alist) /* char *fmt, args, ... */
va_dcl
{
    char *fmt;
    va_list ap;

	va_start(ap);
        fprintf(stderr, "%s: ", progname);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
	va_end(ap);
}

/*
 * Print the fatal error message the input filename, cleanup and exit non-zero.
 */
static
void
fatal_input(va_alist) /* char *fmt, args, ... */
va_dcl
{
    char *fmt;
    va_list ap;

	va_start(ap);
        fprintf(stderr, "%s: ", progname);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
	if(ar_hdrp != NIL)
	    fprintf(stderr, "%s(%s)\n", input_file, member_name);
	else
	    fprintf(stderr, "\n", input_file);
	va_end(ap);
	cleanup(1);
}
#if 0
static
void
fatal(char *fmt)
{
    va_list ap;

	va_start(ap, fmt);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, ap);
	if(ar_hdrp != NIL)
	    fprintf(stderr, "%s(%s)\n", input_file, member_name);
	else
	    fprintf(stderr, "\n", input_file);
	va_end(ap);
	cleanup(1);
}
#endif

/*
 * Print the fatal error message, cleanup and exit non-zero.
 */
static
void
fatal(va_alist) /* char *fmt, args, ... */
va_dcl
{
    char *fmt;
    va_list ap;

	va_start(ap);
        fprintf(stderr, "%s: ", progname);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
	va_end(ap);
	cleanup(1);
}

/*
 * Print the fatal error message along with the system error message, cleanup
 * and exit non-zero.
 */
static
void
perror_fatal(va_alist) /* char *fmt, args, ... */
va_dcl
{
    char *fmt;
    va_list ap;

	va_start(ap);
        fprintf(stderr, "%s: ", progname);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
	if(errno < sys_nerr)
	    fprintf(stderr, " (%s)\n", sys_errlist[errno]);
	else
	    fprintf(stderr, " (errno = %d)\n", errno);
	va_end(ap);
	cleanup(1);
}

/*
 * Print the fatal error message along with the mach error string, cleanup and
 * exit non-zero.
 */
static
void
mach_fatal(va_alist) /* kern_return_t r, char *fmt, args, ... */
va_dcl
{
    va_list ap;
    kern_return_t r;
    char *fmt;

	va_start(ap);
        fprintf(stderr, "%s: ", progname);
	r = va_arg(ap, kern_return_t);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, " (%s)\n", mach_error_string(r));
	va_end(ap);
	cleanup(1);
}

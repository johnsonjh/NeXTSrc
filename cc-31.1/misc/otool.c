#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <a.out.h>
#include <sys/loader.h>
#include <objc/objc-runtime.h>
/*
 * These can't use the pagesize system call because these values are constants
 * in the link editor.  These constants are hold overs from the SUN when both
 * binaries could run on the same system.  This stuff should have been moved
 * into a.out.h.
 */
#ifdef NeXT
#define N_PAGSIZ(x) 0x2000
#define N_SEGSIZ(x) 0x20000
#endif NeXT
/*
 * These versions of the macros correct the fact that the text address of
 * OMAGIC files is always 0.  Only the first is wrong but the others use
 * the first.  It should be corrected in a.out.h.
 */
#define NN_TXTADDR(x) \
	(((x).a_magic==OMAGIC)? 0 : N_PAGSIZ(x))
#define NN_DATADDR(x) \
	(((x).a_magic==OMAGIC)? (NN_TXTADDR(x)+(x).a_text) \
	: (N_SEGSIZ(x)+((NN_TXTADDR(x)+(x).a_text-1) & ~(N_SEGSIZ(x)-1))))
#define NN_BSSADDR(x)  (NN_DATADDR(x)+(x).a_data)
#include <ar.h>
#include <sys/types.h>
#include <ranlib.h>
#include <symseg.h>
/*
 * In the 1.0 version of the link editor defined symbol _SHLIB_INIT was the
 * shared library initialization table.  It was a pointer to an array of
 * pointers that point at arrays of shared library initialization structures.
 * In the 1.0 version of the link editor this is no longer a link editor
 * defined symbol and the shared library initialization structures are in the
 * FVMLIB_INIT0 section.
 */
#define _SHLIB_INIT	"__shared_library_initialization"
/*
 * Also in the 1.0 versions of shared libraries this was prefix to the symbol
 * names of the arrays of shared library initialization structures.  The
 * base name of the object file is the rest of the name.  Each of these arrays'
 * last structure has zeroes in all its fields.  The 1.0 version of the link
 * editor looked for these symbol names if the link editor symbol _SHLIB_INIT
 * was referenced and built a table of the values of init symbol names for the
 * link editor defined symbol (zero terminated).
 */
#define INIT_SYMBOL_NAME	".shared_library_initialization_"
/*
 * The shared library initialization structure used in the 1.0 arrays and now
 * in the FVMLIB_INIT0 section.
 */
struct shlib_init {
    long value;		/* the value to be stored at the address */
    long address;	/* the address to store the value */
};

#define TRUE	1
#define	FALSE	0
#define	NIL	0

/*
 * For system call errors the error messages allways contains
 * sys_errlist[errno] as part of the message.
 */
extern char *sys_errlist[];
extern int errno;

char *rindex(char *, char);
char *ctime(long *);

/* Name of this program for error messages (argv[0]) */
char *progname;

/*
 * The flags to indicate the actions to perform.
 */
long aflag = FALSE; /* print the archive header */
long hflag = FALSE; /* print the exec or mach header */
long lflag = FALSE; /* print the load commands */
long Lflag = FALSE; /* print the shared library names */
long tflag = FALSE; /* print the text */
long dflag = FALSE; /* print the data */
long oflag = FALSE; /* print the objctive-C info */
long Oflag = FALSE; /* print the objctive-C selector strings only */
long rflag = FALSE; /* print the relocation entries */
long Sflag = FALSE; /* print the contents of the __.SYMDEF file */
long gflag = FALSE; /* print the GDB symbol table */
long Gflag = FALSE; /* print the global data size */
long vflag = FALSE; /* print verbosely (symbolicly) when possible */
long Vflag = FALSE; /* print dissassembled operands verbosely */
long cflag = FALSE; /* print the argument and environ strings of a core file */
long iflag = FALSE; /* print the shared library initialization table */
long Tflag = 0;     /* correct address of text for -T with a.out files */
long Wflag = FALSE; /* print the mod time of an archive as an decimal number */
long Xflag = FALSE; /* don't print leading address in disassembly, for testing*/
long Bflag = FALSE; /* create shared library branch table */
long slot = 0;	    /* first branch table slot */
long Yflag = FALSE; /* external text relocation symbol names */
long Zflag = FALSE; /* data relocation types sum */
char *pflag; 	    /* procedure name to start disassembling from */
char *segname,
     *sectname;	    /* name of the section to print the contents of */

/*
 * The following are used for symbolic disassembly and refer to the currently
 * being dissembled object.
 */
/* numericly sorted global and static symbols and their number */
struct nlist *sort_syms;
long nsort_syms;
/* numericly sorted text relocation entries and their number */
struct relocation_info *sort_treloc;
long nsort_trelocs;
/* numericly sorted init relocation entries and their number */
struct relocation_info *sort_ireloc;
long nsort_irelocs;
/* original symbols and their number */
struct nlist *syms;
long nsyms;

long ndatalocal, ndataextern;

/*
 * Declarations for the static routines in this file.
 */
static void usage();
static void process(long, char *, char *, struct ar_hdr *, long, long);
static long sym_compare(struct nlist *, struct nlist *);
static long symname_compare(struct nlist *, struct nlist *);
static long rel_compare(struct relocation_info *, struct relocation_info *);
static long rel_bsearch(long *address, struct relocation_info *rel);
static int cmp_bsearch(const char *name, const struct nlist *sym);
static void print_ar_hdr(struct ar_hdr *, char *, char *);
static void print_symdef(long, long, char *, char *);
static void print_exec(struct exec *);
static void print_mach(struct mach_header *);
static void print_loadcmds(struct mach_header *, struct load_command *, long);
static void print_libraries(struct mach_header *, struct load_command *);
static void print_argstrings(long, long, struct mach_header *,
			     struct load_command *, char *, char *);
static long get_sect_info(struct mach_header *, struct load_command *,
			  long *, long *, long *, char *, char *, long *);
static void get_rel_info(struct mach_header *, struct load_command *,
			 long *, long *, long *, char *, char *);
static void get_sym_info(struct mach_header *, struct load_command *,
			  long *, long *, long *);
static void get_str_info(struct mach_header *, struct load_command *,
			  long *, long *);
static void get_gdb_info(struct mach_header *, struct load_command *,
			  long *, long *);
static void print_text(long, long, long, long, long , char *, char *);
static void print_label(long, long);
static long print_symbol(long, long);
static long print_symbol_in_init(long, long);
static void print_data(long, long, long, long, long, char *, char *);
static void print_sect(struct mach_header *, struct load_command *,
		       long, long, long, long, long, char *, char *, long);
static void print_cstring_section(char *, long, long);
static void print_cstring_char(char);
static void print_literal4_section(char *, long, long);
static void print_literal4(long, float);
static void print_literal8_section(char *, long, long);
static void print_literal8(long, long, double);
static void print_literal_pointer_section( struct mach_header *,
	struct load_command *, int, long, char *, char *, char *, long, long);
static void print_default_section(char *, long, long);
static void print_objc(long, long, struct mach_header *, struct load_command *,
		       char *, char *);
static void print_shlib_init(long, long, struct mach_header *,
			     struct load_command *, char *, char *);
static void print_method_list(struct objc_method_list *, struct section *,
			      struct section *, struct objc_symtab *, char *);
static void print_reloc(struct relocation_info *, long, struct nlist *,
			long, char *, long, long, struct mach_header *,
			struct load_command *);
static void print_mach_reloc(int, long, char *, char *, struct mach_header *,
			     struct load_command *, struct nlist *, long,
			     char *, long);
static void print_gdb(long, long, long, char *, char *);
static void print_symbol_root(struct symbol_root_header *, char *);
static void print_loadmap(long, char *, long);
static void print_typevector(long, char *, long);
static long type_index(long, struct typevector *, long, long);
static void print_blockvector(long, long, char *, long);
static long block_index(long, struct blockvector *, long, long);
static long symbol_index(long, struct block *, long, long);
static void print_sourcevector(long, char *, long);
static void print_line(struct symbol_root *, char *, long, long);
static void data_reloc_sum(struct relocation_info *, long);
static void extern_text_reloc(struct relocation_info *, long, struct nlist *,
			      long, char *, long);
static void branch_table(char *);
static long disassemble(char *, long);
static long print_ef(long, long, char *, long);
static long round(long, unsigned long);

void
main(argc, argv)
int argc;
char *argv[];
{
    long i, j, fd, len, num_files, rest_files, size, offset;
    char *p, *filename, *membername, armag[SARMAG];
    struct ar_hdr ar_hdr;
    char ar_name[sizeof(ar_hdr.ar_name) + 1];
    struct stat stat_buf;

	progname = argv[0];
	if(argc <= 1){
	    usage();
	    exit(1);
	}
	/*
	 * Parse the flags.
	 */
	num_files = 0;
	rest_files = FALSE;
	for(i = 1; i < argc ; i++){
	    if(argv[i][0] != '-' || rest_files){
		num_files++;
		continue;
	    }
	    if(argv[i][1] == '\0'){
		rest_files = TRUE;
		continue;
	    }
	    if(argv[i][1] == 'p'){
		if(argc <=  i + 1){
		    fprintf(stderr, "%s: -p requires an argument (a text symbol"
			    "name)\n", progname);
		    usage();
		    exit(1);
		}
		if(pflag)
		    fprintf(stderr, "%s: only one -p flag can be specified\n",
			    progname);
		pflag = argv[i + 1];
		i++;
		continue;
	    }
	    if(argv[i][1] == 's'){
		if(argc <=  i + 2){
		    fprintf(stderr, "%s: -s requires two arguments (a segment "
			    "name and a section name)\n", progname);
		    usage();
		    exit(1);
		}
		if(sectname != NULL)
		    fprintf(stderr, "%s: only one -s flag can be specified\n",
			    progname);
		segname  = argv[i + 1];
		sectname = argv[i + 2];
		i += 2;
		continue;
	    }
	    if(argv[i][1] == 'T'){
		if(argc <=  i + 1){
		    fprintf(stderr, "%s: -T requires an argument (a hex "
			    "address)\n", progname);
		    usage();
		    exit(1);
		}
		if(Tflag){
		    fprintf(stderr, "%s: only one -T flag can be specified\n",
			    progname);
		    usage();
		    exit(1);
		}
		Tflag = strtoul(argv[i + 1], NULL, 16);
		i++;
		continue;
	    }
	    if(argv[i][1] == 'B'){
		if(argc <=  i + 1){
		    Bflag = TRUE;
		}
		else{
		    if(Bflag){
			fprintf(stderr, "%s: only one -B flag can be "
			        "specified\n", progname);
			usage();
			exit(1);
		    }
		    slot = strtoul(argv[i + 1], NULL, 10);
		    Bflag = TRUE;
		    i++;
		}
		continue;
	    }
	    for(j = 1; argv[i][j] != '\0'; j++){
		switch(argv[i][j]){
		case 'V':
		    Vflag = TRUE;
		case 'v':
		    vflag = TRUE;
		    break;
		case 'a':
		    aflag = TRUE;
		    break;
		case 'h':
		    hflag = TRUE;
		    break;
		case 'l':
		    lflag = TRUE;
		    break;
		case 'L':
		    Lflag = TRUE;
		    break;
		case 't':
		    tflag = TRUE;
		    break;
		case 'd':
		    dflag = TRUE;
		    break;
		case 'o':
		    oflag = TRUE;
		    break;
		case 'O':
		    Oflag = TRUE;
		    break;
		case 'r':
		    rflag = TRUE;
		    break;
		case 'S':
		    Sflag = TRUE;
		    break;
		case 'G':
		    Gflag = TRUE;
		    break;
		case 'g':
		    gflag = TRUE;
		    break;
		case 'c':
		    cflag = TRUE;
		    break;
		case 'i':
		    iflag = TRUE;
		    break;
		case 'W':
		    Wflag = TRUE;
		    break;
		case 'X':
		    Xflag = TRUE;
		    break;
		case 'Z':
		    Zflag = TRUE;
		    break;
		case 'Y':
		    Yflag = TRUE;
		    break;
		default:
		    fprintf(stderr, "%s: Unknown flag %s\n", progname, argv[i]);
		    usage();
		    exit(1);
		}
	    }
	}
	if(!aflag && !hflag && !lflag && !Lflag && !tflag && !dflag && !oflag &&
	   !Oflag && !rflag && !Sflag && !gflag && !Zflag && !Yflag && !Bflag &&
	   !Gflag && !cflag && !iflag && !segname){
	    fprintf(stderr, "%s: One of -ahlLtdoOrSgcis must be specified\n",
		    progname);
	    usage();
	    exit(1);
	}
	if(num_files == 0){
	    fprintf(stderr, "%s: At least one file must be specified\n",
		    progname);
	    usage();
	    exit(1);
	}
	if(segname != NULL && sectname != NULL){
	    if(strcmp(segname, SEG_TEXT) == 0 &&
	       strcmp(sectname, SECT_TEXT) == 0){
		tflag = TRUE;
		segname = NULL;
		sectname = NULL;
	    }
	    if(strcmp(segname, SEG_TEXT) == 0 &&
	       strcmp(sectname, SECT_FVMLIB_INIT0) == 0){
		iflag = TRUE;
		segname = NULL;
		sectname = NULL;
	    }
	}

	/*
	 * For each filename determine if the file name is a regular file,
	 * an archive or an archive member then call process() to process it.
	 */
	rest_files = FALSE;
	for(i = 1; i < argc ; i++){
	    if(argv[i][0] == '-' && !rest_files){
		if(argv[i][1] == '\0')
		    rest_files = TRUE;
		if(argv[i][1] == 'p' || argv[i][1] == 'T' || argv[i][1] == 'B')
		    i++;
		if(argv[i][1] == 's')
		    i += 2;
		continue;
	    }
	    filename = argv[i];
	    membername = NIL;
	    /*
	     * Look for a file name of the form "archive(member)" which is
	     * to mean a member in that archive (the member name must be at
	     * least one character long to be recognized as this form).
	     */
	    len = strlen(filename);
	    if(filename[len-1] == ')'){
		p = rindex(filename, '(');
		if(p != NIL && (p - filename) > 1){
		    membername = p+1;
		    *p = '\0';
		    filename[len-1] = '\0';
		}
	    }
	    fd = open(filename, O_RDONLY);
	    if(fd < 0){
		fprintf(stderr, "%s : Can't open %s (%s)\n", progname,
			filename, sys_errlist[errno]);
		continue;
	    }
	    if(fstat(fd, &stat_buf) < 0){
		fprintf(stderr, "%s : Can't stat %s (%s)\n", progname,
			filename, sys_errlist[errno]);
		continue;
	    }
	    if(read(fd, (char *)armag, SARMAG) != SARMAG){
		fprintf(stderr, "%s : Can't read header of %s (%s)\n",
			progname, filename, sys_errlist[errno]);
		close(fd);
		continue;
	    }
	    if(strncmp(armag, ARMAG, SARMAG) != 0){
		process(fd, filename, NIL, NULL, 0, stat_buf.st_size);
		close(fd);
		continue;
	    }
	    /*
	     * Now it is known that this file is an archive.  So read each
	     * archive header and process the member if so indicated.
	     */
	    if(membername == NIL && !Yflag)
		printf("Archive : %s\n", filename);
	    offset = SARMAG;
	    size = read(fd, (char *)&ar_hdr, sizeof(ar_hdr));
	    if(size == -1){
		fprintf(stderr, "%s : Can't read archive header of %s (%s)\n",
			progname, filename, sys_errlist[errno]);
		close(fd);
		continue;
	    }
	    if(size == 0){
		fprintf(stderr, "%s : archive %s is empty\n",
			progname, filename);
		close(fd);
		continue;
	    }
	    if(Wflag)
		printf("Modification time = %d\n", stat_buf.st_mtime);
	    while(size != 0){
		if(size != sizeof(ar_hdr)){
		    fprintf(stderr, "%s : Short read on archive header of %s ",
			    progname, filename);
		    fprintf(stderr, " at offset %d (bad archive format)\n",
			    offset);
		    break;
		}
		/* place a null an the end of the name */
		strncpy(ar_name, ar_hdr.ar_name, sizeof(ar_hdr.ar_name));
		ar_name[sizeof(ar_hdr.ar_name)] = '\0';
		/* place a null an the end of the name if possible */
		for(j = sizeof(ar_hdr.ar_name) - 1 ; j >=0 ; j--){
		    if(ar_name[j] != ' '){
			j++;
			break;
		    }
		}
		if(j > 0)
		    ar_name[j] = '\0';

		/* process this member if so indicated */
		if(membername != NIL){
		    if(strcmp(membername, ar_name) == 0)
			process(fd, filename, ar_name, &ar_hdr,
				offset + sizeof(ar_hdr), atol(ar_hdr.ar_size));
		}
		else{
		    process(fd, filename, ar_name, &ar_hdr,
			    offset + sizeof(ar_hdr), atol(ar_hdr.ar_size));
		}

		/* move on to the next member */
		offset += sizeof(ar_hdr) +
			  round(atol(ar_hdr.ar_size), sizeof(short));
		lseek(fd, offset, 0);
		size = read(fd, (char *)&ar_hdr, sizeof(ar_hdr));
	    }
	    close(fd);
	}

	if(Zflag){
	    printf("total number of local data relocation entries = %d\n",
		   ndatalocal);
	    printf("total number of extern data relocation entries = %d\n",
		   ndataextern);
	}
	exit(0);
}

/*
 * Print the current usage message.
 */
static
void
usage()
{
	fprintf(stderr,
		"Usage: %s [-ahlLtdorSgvVc] <object file> ...\n",
		progname);

	fprintf(stderr, "\t-a print the archive header\n");
	fprintf(stderr, "\t-h print the mach header\n");
	fprintf(stderr, "\t-l print the load commands\n");
	fprintf(stderr, "\t-L print shared libraries used\n");
	fprintf(stderr, "\t-t print the text(disassemble with -v)\n");
	fprintf(stderr, "\t-p <routine name>  start dissassemble from routine "
		"name\n");
	fprintf(stderr, "\t-s <segname> <sectname> print contents of "
		"section\n");
	fprintf(stderr, "\t-d print the data\n");
	fprintf(stderr, "\t-o print the Objective-C segment\n");
	fprintf(stderr, "\t-O print the Objective-C string section\n");
	fprintf(stderr, "\t-r print the relocation entries\n");
	fprintf(stderr, "\t-S print the contents of the __.SYMDEF file\n");
	fprintf(stderr, "\t-v print verbosely (symbolicly) when possible\n");
	fprintf(stderr, "\t-V print disassembled operands symbolicly\n");
	fprintf(stderr, "\t-c print argument strings of a core file\n");
	fprintf(stderr, "\t-X print no leading addresses or headers\n");
}

/*
 * Now do the print the stuff indicated by the flags for this object.
 * Reading only what is really needed indicated by the flags and doing
 * the best that can be done even if things can't be read.
 */
static
void
process(fd, filename, membername, par_hdr, offset, size)
long fd;
char *filename;
char *membername;
struct ar_hdr *par_hdr;
long offset;
long size;
{
    struct exec exec;
    struct mach_header mh;
    struct load_command *lcp;
    long mach;
    long text_offset, text_size, text_addr;
    long data_offset, data_size, data_addr;
    long sect_offset, sect_size, sect_addr, sect_flags;
    long trel_offset, trel_size, ntrels;
    long drel_offset, drel_size, ndrels;
    long irel_offset, irel_size;
    struct relocation_info *treloc, *dreloc;
    long sym_offset, sym_size, i, len, type;
    long str_offset, str_size;
    char *str, *p;
    long gdb_offset, gdb_size;

	mach = FALSE;
	treloc = NIL;
	dreloc = NIL;
	str = NIL;
	lcp = NIL;

	if(aflag)
	    print_ar_hdr(par_hdr, filename, membername);

	if(membername != NIL &&
	   strncmp(membername, "__.SYMDEF", sizeof("__.SYMDEF") - 1) == 0){
	    if(Sflag)
		print_symdef(fd, offset, filename, membername);
	    return;
	}

	if(vflag && hflag || lflag || Lflag || tflag || dflag || oflag ||
	   Oflag || rflag || gflag || Zflag || Gflag || cflag || iflag){
	    if(membername != NIL)
		printf("%s(%s):\n", filename, membername);
	    else
		printf("%s:\n", filename);
	}

	if(hflag || lflag || Lflag || tflag || dflag || oflag || Oflag ||
	   rflag || gflag || Zflag || Yflag || Bflag || Gflag || cflag ||
	   iflag || segname){
	    lseek(fd, offset, 0);
	    if(read(fd, (char *)&exec, sizeof(exec)) != sizeof(exec)){
		if(membername != NIL)
		    fprintf(stderr,
			    "%s : Can't read header of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr, "%s : Can't read header of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		return;
	    }
	    if(*((long *)&exec) == MH_MAGIC){
		mach = TRUE;
		lseek(fd, offset, 0);
		if(read(fd, (char *)&mh, sizeof(mh)) != sizeof(mh)){
		    if(membername != NIL)
			fprintf(stderr,
				"%s : Can't read mach header of %s(%s) (%s)\n",
				progname, filename, membername,
				sys_errlist[errno]);
		    else
			fprintf(stderr,
				"%s : Can't read mach header of %s (%s)\n",
				progname, filename, sys_errlist[errno]);
		    return;
		}
		if((lcp = (struct load_command *)malloc(mh.sizeofcmds))
		    == (struct load_command *)0){
		    fprintf(stderr, "%s : Ran out of memory (%s)\n",
			    progname, sys_errlist[errno]);
		    exit(1);
		}
		if(read(fd, (char *)lcp, mh.sizeofcmds) !=
		   mh.sizeofcmds){
		    if(membername != NIL)
			fprintf(stderr,
			   "%s : Can't read load commands of %s(%s) (%s)\n",
			    progname, filename, membername,
			    sys_errlist[errno]);
		    else
			fprintf(stderr,
			       "%s : Can't read load commands of %s (%s)\n",
			       progname, filename, sys_errlist[errno]);
		    lcp = (struct load_command *)0;
		}
	    }
	}

	if(rflag || Yflag || (tflag && Vflag)){
	    if(mach){
		get_rel_info(&mh, lcp, &trel_offset, &trel_size, &ntrels,
			     SEG_TEXT, SECT_TEXT);
	    }
	    else{
		trel_offset = N_TXTOFF(exec) + exec.a_text + exec.a_data;
		trel_size = exec.a_trsize;
		ntrels = exec.a_trsize / sizeof(struct relocation_info);
	    }
	    if((treloc = (struct relocation_info *)malloc(trel_size)) ==
	       (struct relocation_info *)0){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + trel_offset, 0);
	    if((read(fd, treloc, trel_size)) != trel_size){
		if(membername != NIL)
		    fprintf(stderr,
			    "%s : Can't read text relocation of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr,
			    "%s : Can't read text relocation of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		treloc = (struct relocation_info *)0;
	    }
	}

	if(rflag || Zflag){
	    if(mach){
		get_rel_info(&mh, lcp, &drel_offset, &drel_size, &ndrels,
			     SEG_DATA, SECT_DATA);
	    }
	    else{
		drel_offset = N_TXTOFF(exec) + exec.a_text + exec.a_data +
			      exec.a_trsize;
		drel_size = exec.a_drsize;
		ndrels = exec.a_drsize / sizeof(struct relocation_info);
	    }
	    if((dreloc = (struct relocation_info *)malloc(drel_size)) ==
	       (struct relocation_info *)0){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + drel_offset, 0);
	    if((read(fd, dreloc, drel_size)) != drel_size){
		if(membername != NIL)
		    fprintf(stderr,
			    "%s : Can't read data relocation of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr,
			    "%s : Can't read data relocation of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		dreloc = (struct relocation_info *)0;
	    }
	}

	if(iflag && vflag){
	    get_rel_info(&mh, lcp, &irel_offset, &irel_size, &nsort_irelocs,
			 SEG_TEXT, SECT_FVMLIB_INIT0);
	    if((sort_ireloc = (struct relocation_info *)malloc(irel_size)) ==
	       (struct relocation_info *)0){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + irel_offset, 0);
	    if((read(fd, sort_ireloc, irel_size)) != irel_size){
		if(membername != NIL)
		    fprintf(stderr,
			    "%s : Can't read init relocation of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr,
			    "%s : Can't read init relocation of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		sort_ireloc = (struct relocation_info *)0;
	    }
	    else
		qsort(sort_ireloc, nsort_irelocs,sizeof(struct relocation_info),
		      (int (*)(const void *, const void *))rel_compare);
	}

	if((rflag && vflag) || (tflag && vflag) || (oflag && vflag) ||
	   Yflag || Bflag || Gflag || iflag){
	    if(mach){
		get_sym_info(&mh, lcp, &sym_offset, &sym_size, &nsyms);
	    }
	    else{
		sym_offset = N_SYMOFF(exec);
		sym_size = exec.a_syms;
		nsyms = exec.a_syms / sizeof(struct nlist);
	    }
	    if((syms = (struct nlist *)malloc(sym_size)) == (struct nlist *)0){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + sym_offset, 0);
	    if((read(fd, syms, sym_size)) != sym_size){
		if(membername != NIL)
		    fprintf(stderr,
			    "%s : Can't read symbol table of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr,
			    "%s : Can't read symbol table %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		syms = (struct nlist *)0;
	    }
	}

	if((rflag && vflag) || gflag || (tflag && vflag) || (oflag && vflag) ||
	   Yflag || Bflag || Gflag || iflag){
	    if(mach){
		get_str_info(&mh, lcp, &str_offset, &str_size);
		get_gdb_info(&mh, lcp, &gdb_offset, &gdb_size);
	    }
	    else{
		str_offset = N_STROFF(exec);
		lseek(fd, offset + str_offset, 0);
		if(read(fd, &str_size, sizeof(long)) != sizeof(long)){
		    if(membername != NIL)
			fprintf(stderr,
				"%s : Can't read string size of %s(%s) (%s)\n",
				progname, filename, membername,
				sys_errlist[errno]);
		    else
			fprintf(stderr,
				"%s : Can't read string size of %s (%s)\n",
				progname, filename, sys_errlist[errno]);
		    exit(1);
		}
	    	gdb_offset = N_STROFF(exec) + str_size,
		gdb_size = size - gdb_offset;
	    }
	    if((rflag && vflag) || (tflag && vflag) || (oflag && vflag) ||
	       Yflag || Bflag || Gflag || iflag){
		if((str = (char *)malloc(str_size)) == NIL){
		    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			    sys_errlist[errno]);
		    exit(1);
		}
		lseek(fd, offset + str_offset, 0);
		if((read(fd, str, str_size)) != str_size){
		    if(membername != NIL)
			fprintf(stderr,
				"%s : Can't read string table of %s(%s) (%s)\n",
				progname, filename, membername,
				sys_errlist[errno]);
		    else
			fprintf(stderr,
				"%s : Can't read string table of %s (%s)\n",
				progname, filename, sys_errlist[errno]);
		    str = NIL;
		}
	    }
	}
	if((tflag && vflag) || (oflag && vflag) || iflag){
	    if((sort_syms = (struct nlist *)malloc(sym_size)) ==
	       (struct nlist *)0){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    nsort_syms = 0;
	    for(i = 0; i < nsyms; i++){
		if(syms[i].n_un.n_strx > 0 &&
		    syms[i].n_un.n_strx < str_size){
		    p = str + syms[i].n_un.n_strx;
		}
		else
		    p = "symbol with bad string index";
		syms[i].n_un.n_name = p;
		if(syms[i].n_type & ~(N_TYPE|N_EXT))
		    continue;
		type = syms[i].n_type & N_TYPE;
		if(type == N_ABS || type == N_TEXT || type == N_DATA || 
		   type == N_BSS || type == N_SECT){
		    len = strlen(p);
		    if((type == N_TEXT ||
			(type == N_SECT && syms[i].n_sect == 1) ) &&
		       len > sizeof(".o") - 1 &&
		       strcmp(p + (len - (sizeof(".o") - 1)), ".o") == 0){
			if(!iflag || len <= sizeof(INIT_SYMBOL_NAME) - 1 ||
			   strncmp(p, INIT_SYMBOL_NAME,
				   sizeof(INIT_SYMBOL_NAME) - 1) != 0)
			continue;
		    }
		    if(strcmp(p, "gcc_compiled.") == 0)
			continue;
		    sort_syms[nsort_syms++] = syms[i];
		}
	    }
	    qsort(sort_syms, nsort_syms, sizeof(struct nlist),
	          (int (*)(const void *, const void *))sym_compare);
	}
	if(Gflag){
	    if((sort_syms = (struct nlist *)malloc(sym_size)) ==
	       (struct nlist *)0){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    nsort_syms = 0;
	    for(i = 0; i < nsyms; i++){
		if((syms[i].n_type & N_EXT) == 0)
		    continue;
		type = syms[i].n_type & N_TYPE;
		if(type == N_DATA || (type == N_SECT && syms[i].n_sect == 2))
		    sort_syms[nsort_syms++] = syms[i];
	    }
	    qsort(sort_syms, nsort_syms, sizeof(struct nlist),
	          (int (*)(const void *, const void *))sym_compare);
	}

	if(tflag && Vflag){
	    if((sort_treloc = (struct relocation_info *)malloc(trel_size)) ==
	       (struct relocation_info *)0){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    bcopy(treloc, sort_treloc, trel_size);
	    nsort_trelocs = ntrels;
	    qsort(sort_treloc, nsort_trelocs, sizeof(struct relocation_info),
	          (int (*)(const void *, const void *))rel_compare);
	}
	if(Bflag){
	    for(i = 0; i < nsyms; i++){
		if(syms[i].n_un.n_strx > 0 &&
		    syms[i].n_un.n_strx < str_size){
		    p = str + syms[i].n_un.n_strx;
		}
		else
		    p = "";
		syms[i].n_un.n_name = p;
	    }
	    qsort(syms, nsyms, sizeof(struct nlist),
	          (int (*)(const void *, const void *))symname_compare);
	}

	if(hflag)
	    if(mach)
		print_mach(&mh);
	    else{
		if(N_BADMAG(exec))
		    print_mach((struct mach_header *)&exec);
		else
		    print_exec(&exec);
	    }

	if(mach && lflag && lcp != (struct load_command *)0)
	    print_loadcmds(&mh, lcp, size);

	if(mach && Lflag && lcp != (struct load_command *)0)
	    print_libraries(&mh, lcp);

	if(cflag)
	    print_argstrings(fd, offset, &mh, lcp, filename, membername);

	if(tflag){
	    if(mach){
		get_sect_info(&mh, lcp, &text_offset, &text_size, &text_addr,
			      SEG_TEXT, SECT_TEXT, NULL);
	    }
	    else{
		if(exec.a_magic == ZMAGIC){
		    text_offset = N_TXTOFF(exec) + sizeof(struct exec);
		    text_size = exec.a_text - sizeof(struct exec);
		    text_addr = NN_TXTADDR(exec) + sizeof(struct exec);
		}
		else{
		    text_offset = N_TXTOFF(exec);
		    text_size = exec.a_text;
		    if(Tflag)
			text_addr = Tflag;
		    else 
			text_addr = NN_TXTADDR(exec);
		}
	    }
	    print_text(fd, offset, text_offset, text_size, text_addr,
		       filename, membername);
	}

	if(dflag || Gflag){
	    if(mach){
		get_sect_info(&mh, lcp, &data_offset, &data_size, &data_addr,
			      SEG_DATA, SECT_DATA, NULL);
	    }
	    else{
		data_offset = N_TXTOFF(exec) + exec.a_text;
		data_size = exec.a_data;
		data_addr = NN_DATADDR(exec);
	    }
	    if(dflag)
		print_data(fd, offset, data_offset, data_size, data_addr,
			   filename, membername);
	    if(Gflag){
		if(nsort_syms == 0)
		    printf("Global data size = 0\n");
		else{
		    printf("Global data size = %d\n",
			   data_size - (sort_syms[0].n_value - data_addr));
		    printf("Static data size = %d\n",
			   sort_syms[0].n_value - data_addr);
		}
	    }
	}

	if(segname != NULL && sectname != NULL && mach){
	    if(get_sect_info(&mh, lcp, &sect_offset, &sect_size, &sect_addr,
			     segname, sectname, &sect_flags) != 0){
		print_sect(&mh, lcp, fd, offset, sect_offset, sect_size,
			   sect_addr, filename, membername, sect_flags);
	    }
	}

	if((oflag || Oflag) && mach)
	    print_objc(fd, offset, &mh, lcp, filename, membername);

	if(iflag)
	    print_shlib_init(fd, offset, &mh, lcp, filename, membername);

	if(rflag){
	    if(mach){
		print_mach_reloc(fd, offset, filename, membername, &mh,
				 lcp, syms, nsyms, str, str_size);
	    }
	    else{
		if(treloc != (struct relocation_info *)0){
		    printf("Text relocation information\n");
		    print_reloc(treloc, ntrels, syms, nsyms, str, str_size,
				mach, &mh, lcp);
		}
		if(dreloc != (struct relocation_info *)0){
		    printf("Data relocation information\n");
		    print_reloc(dreloc, ndrels, syms, nsyms, str, str_size,
				mach, &mh, lcp);
		}
	    }
	}

	if(gflag)
	    print_gdb(fd, offset + gdb_offset, gdb_size, filename, membername);

	if(Zflag){
	    if(dreloc != (struct relocation_info *)0){
		data_reloc_sum(dreloc, ndrels);
	    }
	}

	if(Yflag){
	    if(treloc != (struct relocation_info *)0){
		extern_text_reloc(treloc, ntrels, syms, nsyms, str, str_size);
	    }
	}

	if(Bflag)
	    branch_table(filename);

	if(treloc != NIL)
	    free(treloc);
	if(sort_treloc != NIL){
	    free(sort_treloc);
	    sort_treloc = NIL;
	    nsort_trelocs = 0;
	}
	if(dreloc != NIL)
	    free(dreloc);
	if(syms != NIL){
	    free(syms);
	    syms = NIL;
	    nsyms = 0;
	}
	if(sort_syms != NIL){
	    free(sort_syms);
	    sort_syms = NIL;
	    nsort_syms = 0;
	}
	if(str != NIL)
	    free(str);
	if(lcp != NIL)
	    free(lcp);
}

/*
 * Function for qsort for comparing symbols.
 */
static
long
sym_compare(sym1, sym2)
struct nlist *sym1, *sym2;
{
    return(sym1->n_value - sym2->n_value);
}

/*
 * Function for qsort for comparing symbols.
 */
static
long
symname_compare(sym1, sym2)
struct nlist *sym1, *sym2;
{
    return(strcmp(sym1->n_un.n_name, sym2->n_un.n_name));
}

/*
 * Function for qsort for comparing relocation entries.
 */
static
long
rel_compare(rel1, rel2)
struct relocation_info *rel1, *rel2;
{
    struct scattered_relocation_info *srel;
    long r_address1, r_address2;

	if((rel1->r_address & R_SCATTERED) != 0){
	    srel = (struct scattered_relocation_info *)rel1;
	    r_address1 = srel->r_address;
	}
	else
	    r_address1 = rel1->r_address;
	if((rel2->r_address & R_SCATTERED) != 0){
	    srel = (struct scattered_relocation_info *)rel2;
	    r_address2 = srel->r_address;
	}
	else
	    r_address2 = rel2->r_address;
	return(r_address1 - r_address2);
}

/*
 * Function for bsearch for searching relocation entries.
 */
static
long
rel_bsearch(address, rel)
long *address;
struct relocation_info *rel;
{
    struct scattered_relocation_info *srel;
    long r_address;

	if((rel->r_address & R_SCATTERED) != 0){
	    srel = (struct scattered_relocation_info *)rel;
	    r_address = srel->r_address;
	}
	else
	    r_address = rel->r_address;
	return(*address - r_address);
}

/*
 * Print the archive header.
 */
static
void
print_ar_hdr(par_hdr, filename, membername)
struct ar_hdr *par_hdr;
char *filename;
char *membername;
{
    long i, j, date, uid, gid, mode, size;
    char *p;

	if(membername == NIL)
	    return;

	date = atol(par_hdr->ar_date);
	uid = atol(par_hdr->ar_uid);
	gid = atol(par_hdr->ar_gid);
	size = atol(par_hdr->ar_size);
	sscanf(par_hdr->ar_mode, "%o", &mode);
	if(vflag){
	    for(i = 6 ; i >= 0 ; i -= 3){
		j = ((mode >> i) & 07);
		if(j & 04)
		    printf("r");
		else
		    printf("-");
		if(j & 02)
		    printf("w");
		else
		    printf("-");
		if(j & 01)
		    printf("x");
		else
		    printf("-");
	    }
	}
	else
	    printf("0%03o ", mode & 0777);
	p = ctime(&date);
	p[24] = '\0';
	if(vflag)
	    printf("%3d/%-3d%6d %s %s\n", uid, gid, size, p,
		   membername);
	else
	    printf("%3d/%-3d%6d %12d %s\n", uid, gid, size, date,
		   membername);
}

/*
 * Print_symdef prints the contents of the __.SYMDEF file.  The fd is the
 * file decriptor of the archive and offset is the offset to the start of
 * the __.SYMDEF file.
 */
static
void
print_symdef(fd, offset, filename, membername)
long fd, offset;
char *filename, *membername;
{
    long ran_size, nranlib, str_size, i, j;
    struct ranlib *ranlib;
    char *strings;
    struct ar_hdr ar_hdr;

	lseek(fd, offset, 0);
	if(read(fd, &ran_size, sizeof(long)) != sizeof(long)){
	    fprintf(stderr,
		    "%s : Can't read size of ranlib structs of %s(%s) (%s)\n",
		    progname, filename, membername, sys_errlist[errno]);
	    return;
	}
	nranlib = ran_size / sizeof(struct ranlib);
	if((ranlib = (struct ranlib *)malloc(nranlib * sizeof(struct ranlib)))
	   == (struct ranlib *)0){
	    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
		    sys_errlist[errno]);
	    exit(1);
	}
	if(read(fd, ranlib, ran_size) != ran_size){
	    fprintf(stderr,
		    "%s : Can't read ranlib structures from %s(%s) (%s)\n",
		    progname, filename, membername, sys_errlist[errno]);
	    free(ranlib);
	    return;
	}
	if(read(fd, &str_size, sizeof(long)) != sizeof(long)){
	    fprintf(stderr,
		    "%s : Can't read ranlib string size of %s(%s) (%s)\n",
		    progname, filename, membername, sys_errlist[errno]);
	    free(ranlib);
	    return;
	}
	if((strings = (char *)malloc(str_size)) == NIL){
	    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
		    sys_errlist[errno]);
	    exit(1);
	}
	if((read(fd, strings, str_size)) != str_size){
	    fprintf(stderr,
		    "%s : Can't read ranlib string table of %s(%s) (%s)\n",
		    progname, filename, membername, sys_errlist[errno]);
	    free(ranlib);
	    free(strings);
	    return;
	}

	printf("Table of contents from %s\n", membername);
	printf("size of ranlib structures %d (number %d)\n", ran_size, nranlib);
	printf("size of strings %d", str_size);
	if(str_size % sizeof(long) != 0)
	    printf(" (not multiple of sizeof(long))\n");
	else
	    printf("\n");
	if(vflag)
	    printf("object         symbol name\n");
	else
	    printf("object offset  string index\n");
	for(i = 0; i < nranlib; i++){
	    if(vflag){
		lseek(fd, ranlib[i].ran_off, 0);
		if(read(fd, (char *)&ar_hdr, sizeof(ar_hdr)) != sizeof(ar_hdr)){
		    printf("?(%-11d) ", ranlib[i].ran_off);
		}
		else{
		    printf("%-16.16s ", ar_hdr.ar_name);
		}
		if(ranlib[i].ran_un.ran_strx < str_size)
		    printf("%s\n", strings + ranlib[i].ran_un.ran_strx);
		else
		    printf("?(%d)\n", ranlib[i].ran_un.ran_strx);
	    }
	    else{
		printf("%-14d %d\n", ranlib[i].ran_off,
			ranlib[i].ran_un.ran_strx);
	    }
	}

	free(ranlib);
	free(strings);
}

/*
 * This routine drives the printing of all the symbol segments of a GDB symbol
 * table.  It is passed a pointer to where the GDB symbol table is to start
 * as well as the file descriptor and filename and member name.
 */
static
void
print_gdb(fd, gdb_offset, gdb_size, filename, membername)
long fd, gdb_offset, gdb_size;
char *filename, *membername;
{
    struct symbol_root_header symbol_root;
    long read_size, length, symseg_size;
    char *symseg;

	length = 0;
	symseg_size = 0;

	printf("GDB symbol table (at offset %d, size %d)\n", gdb_offset,
		gdb_size);
	while(1){
	    /*
	     * First read the symbol_root.  The last symbol segment is followed
	     * by a long with a zero value.  That's what the header file
	     * symseg.h says but it really just gets a zero read.
	     */
	    if(length >= gdb_size)
		break;
	    lseek(fd, gdb_offset + length, 0);
	    read_size = read(fd, &symbol_root,
			     sizeof(struct symbol_root_header));
	    if(read_size == 0){
		if(symseg_size != 0)
		    free(symseg);
		return;
	    }
	    if(read_size == -1 ||
	       read_size != sizeof(struct symbol_root_header)){
		if(membername != NIL)
		    fprintf(stderr, "%s : Can't read GDB symbol root of %s(%s) "
			    "(%s)\n", progname, filename, membername,
			    sys_errlist[errno]);
		else
		    fprintf(stderr, "%s : Can't read GDB symbol root of %s "
			    "(%s)\n", progname, filename, sys_errlist[errno]);
		return;
	    }
	    if(symbol_root.length <= 0){
		if(membername != NIL)
		    fprintf(stderr, "%s : Zero or negative length of GDB "
			    "symbol root of %s(%s)\n", progname, filename,
			    membername);
		else
		    fprintf(stderr, "%s : Zero or negative length of GDB "
			    "symbol root of %s\n", progname, filename);
		return;
	    }
	    if(symbol_root.length % sizeof(long) != 0)
		printf("symbol root size not a multiple of sizeof(long)\n");

	    /*
	     * Now read the symbol segment.
	     */
	    if(symseg_size < symbol_root.length){
		if(symseg_size != 0)
		    free(symseg);
		if((symseg = (char *)malloc(symbol_root.length)) == NIL){
		    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			    sys_errlist[errno]);
		    exit(1);
		}
		symseg_size = symbol_root.length;
	    }
	    lseek(fd, gdb_offset + length, 0);
	    if(read(fd, symseg, symbol_root.length) != symbol_root.length){
		if(membername != NIL)
		    fprintf(stderr,
			"%s : Can't read GDB symbol segment of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr,
			    "%s : Can't read GDB symbol segment of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		return;
	    }
	    print_symbol_root(&symbol_root, symseg);

	    length += symbol_root.length;
	    printf("\n");
	}

	if(gdb_size != length)
	    printf("GDB symbol size inconsistant\n");
}

/*
 * This routine prints the symbol_root of a GDB symbol segment.
 */
static
void
print_symbol_root(root_header, symseg)
struct symbol_root_header *root_header;
char *symseg;
{
    struct symbol_root *root;
    struct mach_root *mach_root;
    struct indirect_root *indirect;
    struct mach_indirect_root *mach_indirect;
    struct common_root *common;
    struct shlib_root *shlib;
    struct mach_shlib_root *mach_shlib;
    struct alias_root *alias;
    long i;
    char *p;

	if(root_header->format == SYMBOL_ROOT_FORMAT){
	    root = (struct symbol_root *)symseg;
	    printf("Symbol root:\n");
	    if(vflag)
		printf("       format SYMBOL_ROOT_FORMAT\n");
	    else
		printf("       format %d\n", root->format);
	    printf("       length %d\n", root->length);
	    printf("     ldsymoff %d (bytes) %d (index)\n", root->ldsymoff,
		   root->ldsymoff / sizeof(struct nlist));
	    printf("      textrel 0x%08x\n", root->textrel);
	    printf("      datarel 0x%08x\n", root->datarel);
	    printf("       bssrel 0x%08x\n", root->bssrel);
	    if(vflag &&
	       (long)root->filename > 0 && (long)root->filename <= root->length)
		printf("     filename %s\n", symseg + (long)root->filename);
	    else
		printf("     filename %d\n", (long)root->filename);
	    if(vflag &&
	       (long)root->filedir > 0 && (long)root->filedir <= root->length)
		printf("      filedir %s\n", symseg + (long)root->filedir);
	    else
		printf("      filedir %d\n", (long)root->filedir);
	    printf("  blockvector %d\n", (long)root->blockvector);
	    printf("   typevector %d\n", (long)root->typevector);
	    if(vflag){
		if(root->language == language_c)
		    printf("     language language_c\n");
		else
		    printf("     language %d\n", (long)root->language);
	    }
	    else
		printf("     language %d\n", (long)root->language);

	    if(vflag &
	       (long)root->version > 0 && (long)root->version <= root->length)
		printf("      version %s", symseg + (long)root->version);
	    else
		printf("      version %d\n", (long)root->version);
	    if(vflag &&
	       (long)root->compilation > 0 &&
	       (long)root->compilation <= root->length)
		printf("  compilation %s", symseg + (long)root->compilation);
	    else
		printf("  compilation %d\n", (long)root->compilation);
	    printf("      databeg 0x%08x\n", (long)root->databeg);
	    printf("       bssbeg 0x%08x\n", (long)root->bssbeg);
	    printf(" sourcevector %d\n", (long)root->sourcevector);
	    print_typevector((long)root->typevector, symseg, root->length);
	    print_blockvector((long)root->blockvector, (long)root->typevector,
			      symseg, root->length);
	    print_sourcevector((long)root->sourcevector, symseg, root->length);
	}
	else if(root_header->format == MACH_ROOT_FORMAT){
	    mach_root = (struct mach_root *)symseg;
	    printf("Mach root:\n");
	    if(vflag)
		printf("       format MACH_ROOT_FORMAT\n");
	    else
		printf("       format %d\n", mach_root->format);
	    printf("       length %d\n", mach_root->length);
	    printf("     ldsymoff %d (bytes) %d (index)\n", mach_root->ldsymoff,
		   mach_root->ldsymoff / sizeof(struct nlist));
	    printf("      loadmap %d\n", mach_root->loadmap);
	    if(vflag &&
	       (long)mach_root->filename > 0 &&
	       (long)mach_root->filename <= mach_root->length)
		printf("     filename %s\n",symseg + (long)mach_root->filename);
	    else
		printf("     filename %d\n", (long)mach_root->filename);
	    if(vflag &&
	       (long)mach_root->filedir > 0 &&
	       (long)mach_root->filedir <= mach_root->length)
		printf("      filedir %s\n", symseg + (long)mach_root->filedir);
	    else
		printf("      filedir %d\n", (long)mach_root->filedir);
	    printf("  blockvector %d\n", (long)mach_root->blockvector);
	    printf("   typevector %d\n", (long)mach_root->typevector);
	    if(vflag){
		if(mach_root->language == language_c)
		    printf("     language language_c\n");
		else
		    printf("     language %d\n", (long)mach_root->language);
	    }
	    else
		printf("     language %d\n", (long)mach_root->language);

	    if(vflag &
	       (long)mach_root->version > 0 &&
	       (long)mach_root->version <= mach_root->length)
		printf("      version %s", symseg + (long)mach_root->version);
	    else
		printf("      version %d\n", (long)mach_root->version);
	    if(vflag &&
	       (long)mach_root->compilation > 0 &&
	       (long)mach_root->compilation <= mach_root->length)
		printf("  compilation %s",
		       symseg + (long)mach_root->compilation);
	    else
		printf("  compilation %d\n", (long)mach_root->compilation);
	    printf(" sourcevector %d\n", (long)mach_root->sourcevector);
	    print_loadmap((long)mach_root->loadmap, symseg, mach_root->length);
	    print_typevector((long)mach_root->typevector, symseg,
			     mach_root->length);
	    print_blockvector((long)mach_root->blockvector,
			      (long)mach_root->typevector,
			      symseg, mach_root->length);
	    print_sourcevector((long)mach_root->sourcevector,
			       symseg, mach_root->length);
	}
	else if(root->format == INDIRECT_ROOT_FORMAT){
	    indirect = (struct indirect_root *)symseg;
	    printf("Indirect symbol root:\n");
	    if(vflag)
		printf("      format INDIRECT_ROOT_FORMAT\n");
	    else
		printf("      format %d\n", indirect->format);
	    printf("      length %d\n", indirect->length);
	    printf("    ldsymoff %d (bytes) %d (index)\n", indirect->ldsymoff,
		   indirect->ldsymoff / sizeof(struct nlist));
	    printf("     textrel 0x%08x\n", indirect->textrel);
	    printf("     datarel 0x%08x\n", indirect->datarel);
	    printf("      bssrel 0x%08x\n", indirect->bssrel);
	    printf("    textsize 0x%08x\n", indirect->textsize);
	    printf("    datasize 0x%08x\n", indirect->datasize);
	    printf("     bsssize 0x%08x\n", indirect->bsssize);
	    printf("       mtime 0x%08x\n", indirect->mtime);
	    printf("  fileoffset 0x%08x\n", indirect->fileoffset);
	    printf("    filename %s\n", indirect->filename);
	}
	else if(root->format == MACH_INDIRECT_ROOT_FORMAT){
	    mach_indirect = (struct mach_indirect_root *)symseg;
	    printf("Mach indirect symbol root:\n");
	    if(vflag)
		printf("      format MACH_INDIRECT_ROOT_FORMAT\n");
	    else
		printf("      format %d\n", mach_indirect->format);
	    printf("      length %d\n", mach_indirect->length);
	    printf("    ldsymoff %d (bytes) %d (index)\n",
		   mach_indirect->ldsymoff,
		   mach_indirect->ldsymoff / sizeof(struct nlist));
	    printf("     loadmap %d\n", mach_indirect->loadmap);
	    printf("       mtime 0x%08x\n", mach_indirect->mtime);
	    printf("  fileoffset 0x%08x\n", mach_indirect->fileoffset);
	    printf("    filename %s\n", mach_indirect->filename);
	    print_loadmap((long)mach_indirect->loadmap, symseg,
			  mach_indirect->length);
	}
	else if(root->format == COMMON_ROOT_FORMAT){
	    common = (struct common_root *)symseg;
	    printf("Common symbol root:\n");
	    if(vflag)
		printf("      format COMMON_ROOT_FORMAT\n");
	    else
		printf("      format %d\n", common->format);
	    printf("      length %d\n", common->length);
	    printf("       nsyms %d\n", common->nsyms);
	    p = common->data;
	    printf("    filename %s\n", p);
	    while(*p)
		p++;
	    p++;
	    for(i = 0 ;
		i < common->nsyms && p < (char *)common + common->length;
		i++){
		printf("    sym name %s\n", p);
		while(*p)
		    p++;
		p++;
	    }
	}
	else if(root->format == SHLIB_ROOT_FORMAT){
	    shlib = (struct shlib_root *)symseg;
	    printf("Shared library symbol root:\n");
	    if(vflag)
		printf("        format SHLIB_ROOT_FORMAT\n");
	    else
		printf("        format %d\n", shlib->format);
	    printf("        length %d\n", shlib->length);
	    printf("      ldsymoff %d (bytes) %d (index)\n", shlib->ldsymoff,
		   shlib->ldsymoff / sizeof(struct nlist));
	    printf("       textrel 0x%08x\n", shlib->textrel);
	    printf(" globaldatarel 0x%08x\n", shlib->globaldatarel);
	    printf(" staticdatarel 0x%08x\n", shlib->staticdatarel);
	    printf(" globaldatabeg 0x%08x\n", (long)shlib->globaldatabeg);
	    printf(" staticdatabeg 0x%08x\n", (long)shlib->staticdatabeg);
	    printf("globaldatasize 0x%08x\n", (long)shlib->globaldatasize);
	    printf("staticdatasize 0x%08x\n", (long)shlib->staticdatasize);
	    printf("  symreloffset 0x%08x\n", shlib->symreloffset);
	    printf("      filename %s\n", shlib->filename);
	}
	else if(root->format == MACH_SHLIB_ROOT_FORMAT){
	    mach_shlib = (struct mach_shlib_root *)symseg;
	    printf("Mach shared library symbol root:\n");
	    if(vflag)
		printf("        format MACH_SHLIB_ROOT_FORMAT\n");
	    else
		printf("        format %d\n", mach_shlib->format);
	    printf("        length %d\n", mach_shlib->length);
	    printf("      ldsymoff %d (bytes) %d (index)\n",
		   mach_shlib->ldsymoff,
		   mach_shlib->ldsymoff / sizeof(struct nlist));
	    printf("       loadmap %d\n", mach_shlib->loadmap);
	    printf("      filename %s\n", mach_shlib->filename);
	    printf("  symreloffset 0x%08x\n", mach_shlib->symreloffset);
	    print_loadmap((long)mach_shlib->loadmap, symseg,
			  mach_shlib->length);
	}
	else if(root->format == ALIAS_ROOT_FORMAT){
	    alias = (struct alias_root *)symseg;
	    printf("Symbol alias root:\n");
	    if(vflag)
		printf("        format ALIAS_ROOT_FORMAT\n");
	    else
		printf("        format %d\n", alias->format);
	    printf("        length %d\n", alias->length);
	    printf("      naliases %d\n", alias->naliases);
	    p = alias->data;
	    for(i = 0 ;
		i < alias->naliases && p < (char *)alias + alias->length;
		i++){
		printf(" original name %s\n", p);
		while(*p)
		    p++;
		p++;
		printf("    alias name %s\n", p);
		while(*p)
		    p++;
		p++;
	    }
	}
	else{
	    printf("Symbol_root:\n");
	    printf("      format %d (Unknown format)\n", root_header->format);
	    printf("      length %d\n", root_header->length);
	}
}

/*
 * This routine prints the loadmap of a GDB symbol segment.
 */
static
void
print_loadmap(loadmap_offset, symseg, length)
long loadmap_offset;
char *symseg;
long length;
{
    long i;
    struct loadmap *loadmap;
    struct map *map;

	if(loadmap_offset > 0 &&
	   loadmap_offset <= length){
	    loadmap = (struct loadmap *)(symseg + loadmap_offset);
	    printf("Load map: (nmaps = %d)\n", loadmap->nmaps);
	}
	else{
	    printf("Bad loadmap (%d)\n", loadmap_offset);
	    return;
	}

	for(i = 0; i < loadmap->nmaps; i++){
	    /*
	     * Check to make sure that this array entry indexed by i
	     * is not past the end of the symbol segment.
	     */
	    if(loadmap_offset + sizeof(struct loadmap) +
	       (i-1) * sizeof(struct map *) > length){
		printf("Bad loadmap->nmaps %d\n", loadmap->nmaps);
		return;
	    }
	    printf("    index [%d]\n", i);

	    if((long)(loadmap->map[i]) != 0 &&
	       (long)(loadmap->map[i]) > 0 && 
	       (long)(loadmap->map[i]) <= length){
		map = (struct map *)(symseg + (long)(loadmap->map[i]));
	    }
	    else{
		printf("    map %d (Bad offset)\n",
		       (long)(loadmap->map[i]));
		break;
	    }
	    printf("    map %d\n", (long)(loadmap->map[i]));
	    printf(" reladdr 0x%08x\n", map->reladdr);
	    printf("    size 0x%08x\n", map->size);
	    printf("  ldaddr 0x%08x\n", map->ldaddr);
	}
}

/*
 * This routine prints the typevector of a GDB symbol segment.
 */
static
void
print_typevector(typevector_offset, symseg, length)
long typevector_offset;
char *symseg;
long length;
{
    long i, j, index;
    struct typevector *typevector;
    struct type *type;
    struct field *field;

    static char *type_code_names[] = {
	" UNDEF", "   PTR", " ARRAY", "STRUCT", " UNION", "  ENUM", "  FUNC",
	"   INT", "   FLT", "  VOID", "   SET", " RANGE", "PASARY" };

	if(typevector_offset > 0 &&
	   typevector_offset <= length){
	    typevector = (struct typevector *)(symseg + typevector_offset);
	    printf("Type vector: (length = %d)\n", typevector->length);
	}
	else{
	    printf("Bad typevector (%d)\n", typevector_offset);
	    return;
	}

	printf("index type   code length target point  func flags nfld flds name\n");
	for(i = 0; i < typevector->length; i++){
	    /*
	     * Check to make sure that this array entry indexed by i
	     * is not past the end of the symbol segment.
	     */
	    if(typevector_offset + sizeof(struct typevector) +
	       (i-1) * sizeof(struct type *) > length){
		printf("Bad typevector->length %d\n", typevector->length);
		return;
	    }

	    printf("[%3d] ", i);
	    if((long)(typevector->type[i]) != 0 &&
	       (long)(typevector->type[i]) > 0 && 
	       (long)(typevector->type[i]) <= length){
		type = (struct type *)(symseg + (long)(typevector->type[i]));
	    }
	    else{
		/*
		 * The type vector may have empty slots which contain zero.
		 */
		if((long)(typevector->type[i]) == 0)
		    printf("%4d\n", (long)(typevector->type[i]));
		else
		    printf("%4d (Bad offset)\n", (long)(typevector->type[i]));
		break;
	    }

	    if(vflag){
		printf("%4d ", (long)(typevector->type[i]));
		if((long)type->code > (long)TYPE_CODE_PASCAL_ARRAY)
		    printf("%6d ", (long)type->code);
		else
		    printf("%s ", type_code_names[(long)type->code]);
		printf("%6d ", type->length);
		index = type_index((long)(type->target_type), typevector,
				   typevector_offset, length);
		if(index == -1)
		    printf("%6d ", (long)(type->target_type));
		else
		    printf(" [%3d] ", index);
		index = type_index((long)(type->pointer_type), typevector,
				   typevector_offset, length);
		if(index == -1)
		    printf("%5d ", (long)(type->pointer_type));
		else
		    printf("[%3d] ", index);
		index = type_index((long)(type->function_type), typevector,
				   typevector_offset, length);
		if(index == -1)
		    printf("%5d ", (long)(type->function_type));
		else
		    printf("[%3d] ", index);
		if(type->flags == TYPE_FLAG_UNSIGNED)
		    printf("UNSIG ");
		else
		    printf("%5d ", type->flags);
		printf("%4d %4d ", type->nfields, (long)(type->fields));
		if((long)(type->name) == 0 || (long)(type->name) < 0 ||
		   (long)(type->name) > length)
		    printf("%4d\n", (long)(type->name));
		else
		    printf("%s\n", symseg + (long)(type->name));
	    }
	    else{
		printf("%4d %6d %6d %6d %5d %5d %5d %4d %4d %4d\n",
		    (long)(typevector->type[i]),
		    (long)(type->code),
		    type->length,
		    (long)(type->target_type),
		    (long)(type->pointer_type),
		    (long)(type->function_type),
		    type->flags,
		    type->nfields,
		    (long)(type->fields),
		    (long)(type->name) );
	    }
	    if(type->nfields != 0){
		printf("field bitpos bitsize  type name\n");
		if((long)(type->fields) != 0 &&
		   (long)(type->fields) > 0 && 
		   (long)(type->fields) <= length){
		    field = (struct field *)(symseg + (long)type->fields);
		}
		else{
		    printf("%5d\n", type->fields);
		    continue;
		}
		for(j = 0; j < type->nfields; j++){
		    if((long)type->fields + j * sizeof(struct field) >
		       length){
			printf("Bad type->nfields %d\n", type->nfields);
			break;
		    }
		    if(vflag){
			printf("%5d %6d %7d ",
			       (long)type->fields + j * sizeof(struct field),
			       field[j].bitpos, field[j].bitsize);
			index = type_index((long)(field[j].type), typevector,
					   typevector_offset, length);
			if(index == -1)
			    printf("%5d ", (long)(field[j].type));
			else
			    printf("[%3d] ", index);
			if((long)(field[j].name) == 0 ||
			   (long)(field[j].name) < 0 ||
			   (long)(field[j].name) > length)
			    printf("%4d\n", (long)(field[j].name));
			else
			    printf("%s\n", symseg + (long)(field[j].name));
		    }
		    else{
			printf("%5d %6d %7d %5d %4d\n",
			    (long)type->fields + j * sizeof(struct field),
			    field[j].bitpos,
			    field[j].bitsize,
			    (long)(field[j].type),
			    (long)(field[j].name));
		    }
		}
	    }
	}
}

/*
 * This function returns an index into the typevector->type[] array that matches
 * the type_off passed to it.  If no such offset can be found in the type array
 * or some other error in the structure a -1 is returned. The typevector maybe
 * 0 and in that case a -1 is also returned.
 */
static
long
type_index(type_offset, typevector, typevector_offset, length)
long type_offset;
struct typevector *typevector;
long typevector_offset;
long length;
{
    long i;

	if(type_offset == 0 || type_offset < 0 || type_offset > length)
	    return(-1);
	if(typevector == (struct typevector *)0)
	    return(-1);

	for(i = 0; i < typevector->length; i++){
	    /*
	     * Check to make sure that this array entry indexed by i
	     * is not past the end of the symbol segment.
	     */
	    if(typevector_offset + sizeof(struct typevector) +
	       (i-1) * sizeof(struct type *) > length)
		return(-1);
	    /*
	     * Ignore zero types and ones that are out of range.
	     */
	    if((long)(typevector->type[i]) == 0)
		continue;
	    if((long)(typevector->type[i]) < 0 ||
	       (long)(typevector->type[i]) > length)
		continue;
	    if((long)(typevector->type[i]) == type_offset)
		return(i);
	}
	return(-1);
}

/*
 * This routine prints the blockvector of a GDB symbol segment.
 */
static
void
print_blockvector(blockvector_offset, typevector_offset, symseg, length)
long blockvector_offset;
long typevector_offset;
char *symseg;
long length;
{
    long i, j, index;
    struct blockvector *blockvector;
    struct block *block;
    struct symbol *symbol;
    struct typevector *typevector;

    static char *space_names[] = { " UNDEF", "   VAR", "STRUCT", " LABEL" };
    static char *class_names[] = { "   UNDEF", "   CONST", "  STATIC",
	   "REGISTER", "     ARG", "   LOCAL", " TYPEDEF", "   LABEL",
	   "   BLOCK", "EXTERNAL", "CONBYTES" };


	if(blockvector_offset > 0 &&
	   blockvector_offset <= length){
	    blockvector =
		(struct blockvector *)(symseg + blockvector_offset);
	    printf("Block vector: (nblocks = %d)\n", blockvector->nblocks);
	}
	else{
	    printf("Bad blockvector (%d)\n", blockvector_offset);
	    return;
	}

	if(typevector_offset > 0 &&
	   typevector_offset <= length){
	    typevector = (struct typevector *)(symseg + typevector_offset);
	}
	else{
	    typevector = (struct typevector *)0;
	}

	for(i = 0; i < blockvector->nblocks; i++){
	    /*
	     * Check to make sure that this array entry indexed by i
	     * is not past the end of the symbol segment.
	     */
	    if(blockvector_offset + sizeof(struct blockvector) +
	       (i-1) * sizeof(struct block *) > length){
		printf("Bad blockvector->nblocks %d\n", blockvector->nblocks);
		return;
	    }
	    printf("    index [%d]\n", i);

	    if((long)(blockvector->block[i]) != 0 &&
	       (long)(blockvector->block[i]) > 0 && 
	       (long)(blockvector->block[i]) <= length){
		block =
		    (struct block *)(symseg + (long)(blockvector->block[i]));
	    }
	    else{
		printf("    block %d (Bad offset)\n",
		       (long)(blockvector->block[i]));
		break;
	    }
	    printf("    block %d\n", (long)(blockvector->block[i]));
	    printf("startaddr 0x%08x\n", block->startaddr);
	    printf("  endaddr 0x%08x\n", block->endaddr);
	    index = symbol_index((long)(block->function), block,
				 (long)(blockvector->block[i]), length);
	    if(!vflag || index == -1)
		printf(" function %d\n", (long)(block->function));
	    else
		printf(" function [%d]\n", index);
	    printf("    nsyms %d\n", block->nsyms);
	    if(block->nsyms > 0){
		printf("index symbol  space    class  type      value name\n");
		for(j = 0; j < block->nsyms; j++){
		    printf("[%3d] ", j);
		    /*
		     * Check to make sure that this array entry indexed by j
		     * is not past the end of the symbol segment.
		     */
		    if((long)(blockvector->block[i]) + sizeof(struct block) +
		       (j-1) * sizeof(struct symbol *) > length){
			printf("Bad block->nsyms\n");
			break;
		    }
		    printf("%6d ", (long)block->sym[j]);
		    /*
		     * Ignore zero symbols and ones that are out of range.
		     */
		    if((long)(block->sym[j]) == 0 ||
		       (long)(block->sym[j]) < 0 ||
		       (long)(block->sym[j]) > length){
			printf("\n");
			continue;
		    }
		    symbol = (struct symbol *)(symseg + (long)(block->sym[j]));
		    if(vflag){
			if((long)symbol->namespace > (long)LABEL_NAMESPACE)
			    printf("%6d ", (long)symbol->namespace);
			else
			    printf("%s ", space_names[(long)symbol->namespace]);
			if((long)symbol->class > (long)LOC_CONST_BYTES)
			    printf("%8d ", (long)symbol->class);
			else
			    printf("%s ", class_names[(long)symbol->class]);
			index = type_index((long)(symbol->type), typevector,
					   typevector_offset, length);
			if(index == -1)
			    printf("%5d ", (long)symbol->type);
			else
			    printf("[%3d] ", index);
			if(symbol->class == LOC_BLOCK){
			    index = block_index((long)(symbol->value.block),
					        blockvector, blockvector_offset,
						length);
			    if(index == -1)
				printf("%10d ", (long)symbol->value.block);
			    else
				printf("     [%3d] ", index);
			}
			else if(symbol->class == LOC_CONST_BYTES)
			    printf("%10d ", (long)symbol->value.bytes);
			else
			    printf("0x%08x ", (long)symbol->value.value);
			if((long)(symbol->name) == 0 ||
			   (long)(symbol->name) < 0 ||
			   (long)(symbol->name) > length)
			    printf("%4d\n", (long)(symbol->name));
			else
			    printf("%s\n", symseg + (long)(symbol->name));
		    }
		    else{
			printf("%6d %8d %5d ",
				(long)symbol->namespace,
				(long)symbol->class,
				(long)symbol->type);
			if(symbol->class == LOC_BLOCK)
			    printf("%10d ", (long)symbol->value.block);
			else if(symbol->class == LOC_CONST_BYTES)
			    printf("%10d ", (long)symbol->value.bytes);
			else
			    printf("0x%08x ", (long)symbol->value.value);
			printf("%4d\n", (long)symbol->name);
		    }
		}
	    }
	}
}

/*
 * This function returns an index into the typevector->block[] array that
 * matches the block_offset passed to it.  If no such offset can be found in
 * the block array or some other error in the structure a -1 is returned. The
 * blockvector pointer is assumed to be good.
 */
static
long
block_index(block_offset, blockvector, blockvector_offset, length)
long block_offset;
struct blockvector *blockvector;
long blockvector_offset;
long length;
{
    long i;

	if(block_offset == 0 || block_offset < 0 || block_offset > length)
	    return(-1);

	for(i = 0; i < blockvector->nblocks; i++){
	    /*
	     * Check to make sure that this array entry indexed by i
	     * is not past the end of the symbol segment.
	     */
	    if(blockvector_offset + sizeof(struct blockvector) +
	       (i-1) * sizeof(struct block *) > length)
		return(-1);
	    /*
	     * Ignore zero blocks and ones that are out of range.
	     */
	    if((long)(blockvector->block[i]) == 0)
		continue;
	    if((long)(blockvector->block[i]) < 0 ||
	       (long)(blockvector->block[i]) > length)
		continue;
	    if((long)(blockvector->block[i]) == block_offset)
		return(i);
	}
	return(-1);
}

/*
 * This function returns an index into the block->syms[] array that
 * matches the sym_offset passed to it.  If no such offset can be found in
 * the syms array or some other error in the structure a -1 is returned. The
 * block pointer is assumed to be good.
 */
static
long
symbol_index(sym_offset, block, block_offset, length)
long sym_offset;
struct block *block;
long block_offset;
long length;
{
    long i;

	if(sym_offset == 0 || sym_offset < 0 || sym_offset > length)
	    return(-1);

	for(i = 0; i < block->nsyms; i++){
	    /*
	     * Check to make sure that this array entry indexed by i
	     * is not past the end of the symbol segment.
	     */
	    if(block_offset + sizeof(struct block) +
	       (i-1) * sizeof(struct symbol *) > length)
		return(-1);
	    /*
	     * Ignore zero symbols and ones that are out of range.
	     */
	    if((long)(block->sym[i]) == 0)
		continue;
	    if((long)(block->sym[i]) < 0 ||
	       (long)(block->sym[i]) > length)
		continue;
	    if((long)(block->sym[i]) == sym_offset)
		return(i);
	}
	return(-1);
}

/*
 * This routine prints the sourcevector of a GDB symbol segment.
 * source->contents.item[] is an array of longs with source->contents.nitems
 * entries.  If the item is negitive it is a new line number (with the sign
 * changed).  Otherwise the item is address for the current line.  If there
 * is no line number before the address then the current line is one more than
 * the old line.
 */
static
void
print_sourcevector(sourcevector_offset, symseg, length)
long sourcevector_offset;
char *symseg;
long length;
{
    long i, j, line, *p;
    struct sourcevector *sourcevector;
    struct source *source;

	if(sourcevector_offset > 0 &&
	   sourcevector_offset <= length){
	    sourcevector =
		(struct sourcevector *)(symseg + sourcevector_offset);
	    printf("Source vector: (length = %d)\n", sourcevector->length);
	}
	else{
	    printf("Bad sourcevector (%d)\n", sourcevector_offset);
	    return;
	}

	for(i = 0; i < sourcevector->length; i++){
	    /*
	     * Check to make sure that this array entry indexed by i
	     * is not past the end of the symbol segment.
	     */
	    if(sourcevector_offset + sizeof(struct sourcevector) +
	       (i-1) * sizeof(struct source *) > length){
		printf("Bad sourcevector->length %d\n", sourcevector->length);
		return;
	    }
	    printf(" index [%d]\n", i);

	    if((long)(sourcevector->source[i]) != 0 &&
	       (long)(sourcevector->source[i]) > 0 && 
	       (long)(sourcevector->source[i]) <= length){
		source =
		    (struct source *)(symseg + (long)(sourcevector->source[i]));
	    }
	    else{
		printf("source %d (Bad offset)\n",
		       (long)(sourcevector->source[i]));
		break;
	    }
	    printf("source %d\n", (long)(sourcevector->source[i]));
	    if(vflag && 
	       (long)(source->name) > 0 &&
	       (long)(source->name) <= length)
		    printf("  name %s\n", symseg + (long)(source->name));
	    else
		printf("  name %d\n", source->name);
	    printf("nitems %d\n", source->contents.nitems);

	    if(source->contents.nitems > 0){
		printf("line address\n");
		line = 0;
		p = (long *)source->contents.item;
		for(j = 0; j < source->contents.nitems; j++){
		    /*
		     * Check to make sure that this array entry indexed by j
		     * is not past the end of the symbol segment.  Ugly but
		     * the best that can be done.
		     */
		    if((long)p + j * sizeof(long) >
		       (long)symseg + length){
			printf("Bad source->contents.nitems\n");
			break;
		    }
		    if(p[j] < 0)
			line = -p[j];
		    else{
			line++;
			printf("%4d 0x%08x\n", line, p[j]);
		    }
		}
	    }
	}
}

/*
 * This routine was used for gathering stats on what the number of runtime
 * relocation entries would be like.
 */
static
void
data_reloc_sum(reloc, nreloc)
struct relocation_info *reloc;
long nreloc;
{
    struct relocation_info *p;
    long i, l, e;

	l = 0;
	e = 0;
	for(i = 0 ; i < nreloc ; i++){
	    p = reloc + i;
	    if(p->r_extern)
		e++;
	    else
		l++;
	}
	if(vflag)
	    printf("exteral = %d local = %d\n", e, l);
	ndatalocal += l;
	ndataextern += e;
}

/*
 * Print the exec header.
 */
static
void
print_exec(exec)
struct exec *exec;
{
	printf("Exec header\n");
#ifdef sun
	printf(" machtype    magic    text    data     bss ");
	printf("   syms      entry  trsize  drsize\n");
	if(vflag){
	    switch(exec->a_machtype){
	    case M_OLDSUN2:
		printf("M_OLDSUN2");
		break;
	    case M_68010:
		printf("  M_68010");
		break;
	    case M_68020:
		printf("  M_68020");
		break;
	    default:
		printf(" %8d", exec->a_machtype);
		break;
	    }
	}
	else
	    printf(" %8d", exec->a_machtype);
#else
	printf("    magic    text    data     bss ");
	printf("   syms      entry  trsize  drsize\n");
#endif sun
	if(vflag){
	    switch(exec->a_magic){
	    case OMAGIC:
		printf("   OMAGIC ");
		break;
	    case NMAGIC:
		printf("   NMAGIC ");
		break;
	    case ZMAGIC:
		printf("   ZMAGIC ");
		break;
	    default:
		printf("     %04o ", exec->a_magic);
	    }
	}
	else
	    printf("     %04o ", exec->a_magic);

	printf("%7d %7d %7d ", exec->a_text, exec->a_data, exec->a_bss);
	printf("%7d 0x%08x %7d %7d\n",
	       exec->a_syms, exec->a_entry, exec->a_trsize, exec->a_drsize);
	printf(" N_TXTADDR  N_DATADDR  N_BSSADDR  N_TXTOFF");
	printf("   N_TXTOFF+a_text\n");
	printf(" 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
		NN_TXTADDR(*exec), NN_DATADDR(*exec), NN_BSSADDR(*exec),
		N_TXTOFF(*exec), N_TXTOFF(*exec)+exec->a_text);
}

/*
 * Print the mach header.
 */
static
void
print_mach(mh)
struct mach_header *mh;
{
    long flags;

	printf("Mach header\n");
	printf("      magic cputype cpusubtype   filetype ncmds sizeofcmds "
	       "     flags\n");
	if(vflag){
	    if(mh->magic == MH_MAGIC)
		printf("   MH_MAGIC");
	    else
		printf(" 0x%08x", mh->magic);
	    switch(mh->cputype){
#if 0
	    case CPU_TYPE_VAX:
		printf("     VAX");
		switch(mh->cpusubtype){
		case CPU_SUBTYPE_VAX780:
		    printf("     VAX780");
		    break;
		case CPU_SUBTYPE_VAX785:
		    printf("     VAX785");
		    break;
		case CPU_SUBTYPE_VAX750:
		    printf("     VAX750");
		    break;
		case CPU_SUBTYPE_VAX730:
		    printf("     VAX730");
		    break;
		case CPU_SUBTYPE_UVAXI:
		    printf("     UVAXI");
		    break;
		case CPU_SUBTYPE_UVAXII:
		    printf("     UVAXII");
		    break;
		case CPU_SUBTYPE_VAX8200:
		    printf("    VAX8200");
		    break;
		case CPU_SUBTYPE_VAX8500:
		    printf("    VAX8500");
		    break;
		case CPU_SUBTYPE_VAX8600:
		    printf("    VAX8600");
		    break;
		case CPU_SUBTYPE_VAX8650:
		    printf("    VAX8650");
		    break;
		case CPU_SUBTYPE_VAX8800:
		    printf("    VAX8800");
		    break;
		case CPU_SUBTYPE_UVAXIII:
		    printf("    UVAXIII");
		    break;
		default:
		    printf(" %10d", mh->cpusubtype);
		    break;
		}
		break;
	    case CPU_TYPE_ROMP:
		printf("    ROMP");
		switch(mh->cpusubtype){
		case CPU_SUBTYPE_RT_PC:
		    printf("      RT_PC");
		    break;
		case CPU_SUBTYPE_RT_APC:
		    printf("     RT_APC");
		    break;

		default:
		    printf(" %10d", mh->cpusubtype);
		    break;
		}
		break;
	    case CPU_TYPE_NS32032:
		printf(" NS32032");
		switch(mh->cpusubtype){
		case CPU_SUBTYPE_MMAX:
		    printf("       MMAX");
		    break;
		case CPU_SUBTYPE_SQT:
		    printf("        SQT");
		    break;
		default:
		    printf(" %10d", mh->cpusubtype);
		    break;
		}
		break;
	    case CPU_TYPE_NS32332:
		printf(" NS32332");
		switch(mh->cpusubtype){
		case CPU_SUBTYPE_MMAX:
		    printf("       MMAX");
		    break;
		case CPU_SUBTYPE_SQT:
		    printf("        SQT");
		    break;
		default:
		    printf(" %10d", mh->cpusubtype);
		    break;
		}
		break;
#endif
	    case CPU_TYPE_MC680x0:
		printf(" MC680x0");
		switch(mh->cpusubtype){
		case CPU_SUBTYPE_MC68030:
		    printf("    MC68030");
		    break;
		case CPU_SUBTYPE_MC68040:
		    printf("    MC68040");
		    break;
		default:
		    printf(" %10d", mh->cpusubtype);
		    break;
		}
		break;
	    default:
		printf(" %7d %10d", mh->cputype, mh->cpusubtype);
		break;
	    }
	    switch(mh->filetype){
	    case MH_OBJECT:
		printf("     OBJECT");
		break;
	    case MH_EXECUTE:
		printf("    EXECUTE");
		break;
	    case MH_FVMLIB:
		printf("     FVMLIB");
		break;
	    case MH_CORE:
		printf("       CORE");
		break;
	    case MH_PRELOAD:
		printf("    PRELOAD");
		break;
	    default:
		printf(" %10d", mh->filetype);
		break;
	    }
	    printf(" %5d %10d", mh->ncmds, mh->sizeofcmds);
	    flags = mh->flags;
	    if(flags & MH_NOUNDEFS){
		printf("   NOUNDEFS");
		flags &= ~MH_NOUNDEFS;
	    }
	    if(flags & MH_INCRLINK){
		printf(" INCRLINK");
		flags &= ~MH_INCRLINK;
	    }
	    if(flags != 0 || mh->flags == 0)
		printf(" 0x%08x", mh->flags);
	    printf("\n");
	}
	else{
	    printf(" 0x%08x %7d %10d 0x%08x %5d %10d 0x%08x\n", mh->magic,
		   mh->cputype, mh->cpusubtype, mh->filetype,
		   mh->ncmds, mh->sizeofcmds, mh->flags);
	}
}

/*
 * Print the load commands.
 */
static
void
print_loadcmds(mh, initlc, size)
struct mach_header *mh;
struct load_command *initlc;
long size;
{
    long i, j, k, flavor, count;
    char *p, *state;
    struct load_command *lc;
    struct segment_command *sg;
    struct thread_command *ut;
    struct symtab_command *st;
    struct symseg_command *ss;
    struct fvmlib_command *fl;
    struct section *s;
    struct NeXT_thread_state_regs *cpu;
    struct NeXT_thread_state_68882 *fpu;
    struct NeXT_thread_state_user_reg *user_reg;

	lc = initlc;
	for(i = 0 ; i < mh->ncmds; i++){
	    printf("Load command %d\n", i);
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		printf("      cmd LC_SEGMENT\n");
		printf("  cmdsize %d", sg->cmdsize);
		if(sg->cmdsize != sizeof(struct segment_command) +
				     sg->nsects * sizeof(struct section))
		    printf(" Inconsistant size\n");
		else
		    printf("\n");
         	printf("  segname %0.16s\n", sg->segname);
		printf("   vmaddr 0x%08x\n", sg->vmaddr);
		printf("   vmsize 0x%08x\n", sg->vmsize);
		printf("  fileoff %d", sg->fileoff);
		if(sg->fileoff > size)
		    printf(" (past end of file)\n");
		else
		    printf("\n");
		printf(" filesize %d", sg->filesize);
		if(sg->fileoff + sg->filesize > size)
		    printf(" (past end of file)\n");
		else
		    printf("\n");
		if(vflag){
		    if((sg->maxprot &
		      ~(VM_PROT_READ  | VM_PROT_WRITE  | VM_PROT_EXECUTE)) != 0)
			printf("  maxprot ?(0x%08x)\n", sg->maxprot);
		    else{
			if(sg->maxprot & VM_PROT_READ)
			    printf("  maxprot r");
			else
			    printf("  maxprot -");
			if(sg->maxprot & VM_PROT_WRITE)
			    printf("w");
			else
			    printf("-");
			if(sg->maxprot & VM_PROT_EXECUTE)
			    printf("x\n");
			else
			    printf("-\n");
		    }
		    if((sg->initprot &
		      ~(VM_PROT_READ  | VM_PROT_WRITE  | VM_PROT_EXECUTE)) != 0)
			printf(" initprot ?(0x%08x)\n", sg->initprot);
		    else{
			if(sg->initprot & VM_PROT_READ)
			    printf(" initprot r");
			else
			    printf(" initprot -");
			if(sg->initprot & VM_PROT_WRITE)
			    printf("w");
			else
			    printf("-");
			if(sg->initprot & VM_PROT_EXECUTE)
			    printf("x\n");
			else
			    printf("-\n");
		    }
		}
		else{
		    printf("  maxprot 0x%08x\n", sg->maxprot);
		    printf(" initprot 0x%08x\n", sg->initprot);
		}
		printf("   nsects %d\n", sg->nsects);
		if(vflag){
		    printf("    flags");
		    if(sg->flags == 0)
			printf(" (none)\n");
		    else{
			if(sg->flags & SG_HIGHVM){
			    printf(" HIGHVM");
			    sg->flags &= ~SG_HIGHVM;
			}
			if(sg->flags & SG_FVMLIB){
			    printf(" FVMLIB");
			    sg->flags &= ~SG_FVMLIB;
			}
			if(sg->flags & SG_NORELOC){
			    printf(" NORELOC");
			    sg->flags &= ~SG_NORELOC;
			}
			if(sg->flags)
			    printf(" 0x%x (unknown flags)\n", sg->flags);
			else
			    printf("\n");
		    }
		}
		else{
		    printf("    flags %d\n", sg->flags);
		}

		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0 ; j < sg->nsects ; j++){
		    if((char *)s + sizeof(struct section) >
		       (char *)initlc + mh->sizeofcmds){
			printf("section structure command extends past end of "
			       "load commands\n");
		    }
		    printf("Section\n");
		    printf("  sectname %0.16s\n", s->sectname);
		    printf("   segname %0.16s", s->segname);
		    if(strcmp(sg->segname, s->segname) != 0 &&
		       mh->filetype != MH_OBJECT)
			printf(" (does not match segment)\n");
		    else
			printf("\n");
		    printf("      addr 0x%08x\n", s->addr);
		    printf("      size 0x%08x", s->size);
		    if((s->flags & S_ZEROFILL) != 0 && 
		       s->offset + s->size > size)
			printf(" (past end of file)\n");
		    else
			printf("\n");
		    printf("    offset %d", s->offset);
		    if(s->offset > size)
			printf(" (past end of file)\n");
		    else
			printf("\n");
		    printf("     align 2^%d (%d)\n", s->align, 1<<s->align);
		    printf("    reloff %d", s->reloff);
		    if(s->reloff > size)
			printf(" (past end of file)\n");
		    else
			printf("\n");
		    printf("    nreloc %d", s->nreloc);
		    if(s->reloff + s->nreloc * sizeof(struct relocation_info) >
		       size)
			printf(" (past end of file)\n");
		    else
			printf("\n");
		    if(vflag){
			printf("     flags");
			if(s->flags == S_ZEROFILL)
			    printf(" S_ZEROFILL\n");
			else if(s->flags == S_CSTRING_LITERALS)
			    printf(" S_CSTRING_LITERALS\n");
			else if(s->flags == S_4BYTE_LITERALS)
			    printf(" S_4BYTE_LITERALS\n");
			else if(s->flags == S_8BYTE_LITERALS)
			    printf(" S_8BYTE_LITERALS\n");
			else
			    printf(" 0x%08x\n", s->flags);
		    }
		    else
			printf("     flags 0x%08x\n", s->flags);
		    printf(" reserved1 %d\n", s->reserved1);
		    printf(" reserved2 %d\n", s->reserved2);
		    if((char *)s + sizeof(struct section) >
		       (char *)initlc + mh->sizeofcmds)
			return;
		    s++;
		}
		break;

	    case LC_SYMTAB:
		st = (struct symtab_command *)lc;
		printf("     cmd LC_SYMTAB\n");
		printf(" cmdsize %d", st->cmdsize);
		if(st->cmdsize != sizeof(struct symtab_command))
		    printf(" Incorrect size\n");
		else
		    printf("\n");
		printf("  symoff %d", st->symoff);
		if(st->symoff > size)
		    printf(" (past end of file)\n");
		else
		    printf("\n");
		printf("   nsyms %d", st->nsyms);
		if(st->symoff + st->nsyms * sizeof(struct nlist) > size)
		    printf(" (past end of file)\n");
		else
		    printf("\n");
		printf("  stroff %d", st->stroff);
		if(st->stroff > size)
		    printf(" (past end of file)\n");
		else
		    printf("\n");
		printf(" strsize %d", st->strsize);
		if(st->stroff + st->strsize > size)
		    printf(" (past end of file)\n");
		else
		    printf("\n");
		break;

	    case LC_SYMSEG:
		ss = (struct symseg_command *)lc;
		printf("     cmd LC_SYMSEG\n");
		printf(" cmdsize %d", ss->cmdsize);
		if(ss->cmdsize != sizeof(struct symseg_command))
		    printf(" Incorrect size\n");
		else
		    printf("\n");
		printf("  offset %d", ss->offset);
		if(ss->offset > size)
		    printf(" (past end of file)\n");
		else
		    printf("\n");
		printf("    size %d", ss->size);
		if(ss->offset + ss->size > size)
		    printf(" (past end of file)\n");
		else
		    printf("\n");
		break;

	    case LC_IDFVMLIB:
	    case LC_LOADFVMLIB:
		fl = (struct fvmlib_command *)lc;
	        if(fl->cmd == LC_IDFVMLIB)
		    printf("           cmd LC_IDFVMLIB\n");
		else
		    printf("           cmd LC_LOADFVMLIB\n");
		printf("       cmdsize %d", fl->cmdsize);
		if(fl->cmdsize < sizeof(struct fvmlib_command))
		    printf(" Incorrect size\n");
		else
		    printf("\n");
		if(fl->fvmlib.name.offset < fl->cmdsize){
		    p = (char *)fl + fl->fvmlib.name.offset;
		    printf("          name %s (offset %d)\n", p,
			   fl->fvmlib.name.offset);
		}
		else{
		    printf("          name ?(bad offset %d)\n",
			   fl->fvmlib.name.offset);
		}
		printf(" minor version %d\n", fl->fvmlib.minor_version);
		printf("   header addr 0x%08x\n", fl->fvmlib.header_addr);
		break;

	    case LC_UNIXTHREAD:
	    case LC_THREAD:
		ut = (struct thread_command *)lc;
	        if(ut->cmd == LC_UNIXTHREAD)
		    printf("        cmd LC_UNIXTHREAD\n");
		else
		    printf("        cmd LC_THREAD\n");
		state = (char *)ut + sizeof(struct thread_command);
		printf("    cmdsize %d\n", ut->cmdsize);
#ifdef NeXT
	    	if(mh->cputype == CPU_TYPE_MC680x0 &&
		   (mh->cpusubtype == CPU_SUBTYPE_MC68030 ||
		    mh->cpusubtype == CPU_SUBTYPE_MC68040)){
		    p = (char *)ut + ut->cmdsize;
		    while(state < p){
			flavor = *((unsigned long *)state);
			state += sizeof(unsigned long);
			count = *((unsigned long *)state);
			state += sizeof(unsigned long);
			switch(flavor){
			case NeXT_THREAD_STATE_REGS:
			    printf("     flavor NeXT_THREAD_STATE_REGS\n");
			    if(count == NeXT_THREAD_STATE_REGS_COUNT)
				printf("      count NeXT_THREAD_STATE_"
				       "REGS_COUNT\n");
			    else
			        printf("      count %d (not NeXT_THREAD_STATE_"
				       "REGS_COUNT)\n");
			    cpu = (struct NeXT_thread_state_regs *)state;
			    state += sizeof(struct NeXT_thread_state_regs);
			    printf(" dregs ");
			    for(j = 0 ; j < 8 ; j++)
				printf(" %08x", cpu->dreg[j]);
			    printf("\n");
			    printf(" aregs ");
			    for(j = 0 ; j < 8 ; j++)
				printf(" %08x", cpu->areg[j]);
			    printf("\n");
			    printf(" pad 0x%04x sr 0x%04x pc 0x%08x\n", 
				    cpu->pad0 & 0x0000ffff,
				    cpu->sr & 0x0000ffff,
				    cpu->pc);
			    break;
			case NeXT_THREAD_STATE_68882:
			    printf("     flavor NeXT_THREAD_STATE_68882\n");
			    if(count == NeXT_THREAD_STATE_68882_COUNT)
				printf("      count NeXT_THREAD_STATE_"
				       "68882_COUNT\n");
			    else
				printf("      count %d (not NeXT_THREAD_STATE_"
				       "68882_COUNT\n");
			    fpu = (struct NeXT_thread_state_68882 *)state;
			    state += sizeof(struct NeXT_thread_state_68882);
			    for(j = 0 ; j < 8 ; j++)
				printf(" fp reg %d %08x %08x %08x\n", j,
				       fpu->regs[j].fp[0],
				       fpu->regs[j].fp[1],
				       fpu->regs[j].fp[2]);
			    printf(" cr 0x%08x sr 0x%08x state 0x%08x\n", 
				    fpu->cr, fpu->sr, fpu->state);
			    break;
			case NeXT_THREAD_STATE_USER_REG:
			    printf("     flavor NeXT_THREAD_STATE_USER_REG\n");
			    if(count == NeXT_THREAD_STATE_USER_REG_COUNT)
				printf("      count NeXT_THREAD_STATE_"
				       "USER_REG_COUNT\n");
			    else
			        printf("      count %d (not NeXT_THREAD_STATE_"
				       "USER_REG_COUNT");
			    user_reg =
				(struct NeXT_thread_state_user_reg *)state;
			    state += sizeof(struct NeXT_thread_state_user_reg);
			    printf(" user_reg 0x%08x\n", user_reg->user_reg);
			    break;
			default:
			    printf("Unknown flavor of thread state 0x%x\n",
				   flavor);
			    goto thread_done;
			}
		    }
		    break;
		}
#endif NeXT
		printf("      state (Unknown cputype/cpusubtype):\n");
		for(j = 0 ; j < ut->cmdsize - sizeof(struct thread_command); ){
		    for(k = 0 ;
			k < 8 * sizeof(long) &&
			j + k < ut->cmdsize - sizeof(struct thread_command) ;
			k += sizeof(long) )
			printf("%08x ",*((long *)(state + j + k)));
		    printf("\n");
		    j += k;
		}
thread_done:
		break;
	    case LC_IDENT:
		printf("          cmd LC_IDENT\n");
		printf("      cmdsize %d", lc->cmdsize);
		if(lc->cmdsize < sizeof(struct ident_command))
		    printf(" Incorrect size\n");
		else
		    printf("\n");
		p = ((char *)lc) + sizeof(struct ident_command);
		while(p < (char *)(lc + lc->cmdsize) ){
		    if(*p == '\0'){
			p++;
			continue;
		    }
		    printf(" ident string %s\n", p);
		    while(*p != '\0')
			p++;
		    p++;
		}
		break;

	    default:
		printf("      cmd ?(0x%08x) Unknown load command\n",lc->cmd);
		printf("  cmdsize %d\n", lc->cmdsize);
		if(lc->cmdsize <= 0){
		    printf("load command %d size negative or zero (can't "
			   "advance to other load commands)\n", i);
		    return;
		}
		p = (char *)lc;
		for(j = 0 ; j < lc->cmdsize ; ){
		    for(k = 0 ;
			k < 8 * sizeof(long) && j + k < lc->cmdsize ;
			k += sizeof(long) )
			printf("%08x ",*((long *)(p + j + k)));
		    printf("\n");
		    j += k;
		}
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		return;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

/*
 * Print the libraries names from the LC_LOADFVMLIB commands.
 */
static
void
print_libraries(mh, lc)
struct mach_header *mh;
struct load_command *lc;
{
    long i, j;
    struct fvmlib_command *fl;
    char *p;
    struct load_command *initlc;

	initlc = lc;
	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_LOADFVMLIB || lc->cmd == LC_IDFVMLIB){
		fl = (struct fvmlib_command *)lc;
		if(fl->cmdsize < sizeof(struct fvmlib_command)){
		    printf("\tIncorrect size of LC_LOADFVMLIB command %d\n", i);
		    continue;
		}
		if(fl->fvmlib.name.offset < fl->cmdsize){
		    p = (char *)fl + fl->fvmlib.name.offset;
		    printf("\t%s (minor version %d)\n", p,
			   fl->fvmlib.minor_version);
		}
		else{
		    printf("\tBad offset (%d) for name of LC_LOADFVMLIB "
			   "command %d\n", fl->fvmlib.name.offset, i);
		}
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		return;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

#define STACK_ADDR 0x4000000
/*
 * Print the argument strings (from a core file).
 */
static
void
print_argstrings(fd, offset, mh, lc, filename, membername)
long fd;
long offset;
struct mach_header *mh;
struct load_command *lc;
char *filename;
char *membername;
{
    long i, j;
    struct segment_command *sg;
    char *stack, *stack_top, *p, *q;
    struct load_command *initlc;

	initlc = lc;
	printf("Argument strings on the stack at: 0x%x\n", STACK_ADDR);
	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		if(sg->vmaddr + sg->vmsize == STACK_ADDR){
		    if((stack = malloc(sg->filesize)) == NIL){
			fprintf(stderr, "%s : Ran out of memory (%s)\n",
				progname, sys_errlist[errno]);
			exit(1);
		    }
		    lseek(fd, offset + sg->fileoff, 0);
		    if((read(fd, stack, sg->filesize)) != sg->filesize){
			if(membername != NIL)
			    fprintf(stderr, "%s : Can't read stack segment of "
				    "%s(%s) (%s)\n", progname, filename,
				    membername, sys_errlist[errno]);
			else
			    fprintf(stderr, "%s : Can't read stack segment of "
				    "%s (%s)\n", progname, filename,
				    sys_errlist[errno]);
			free(stack);
			return;
		    }

		    stack_top = stack + sg->filesize;

		    /* the first thing on the stack is a long 0 */
		    stack_top -= 4;
		    p = (char *)stack_top;

		    p--; /* first character before the long 0 */
		    q = p;
		    /* Stop when we find another long 0 */
		    while(p > stack && (*p != '\0' || *(p-1) != '\0' ||
			  *(p-2) != '\0' || *(p-3) != '\0')){
			p--;
			/* step back over the string to its start */
			while(p > stack && *p != '\0')
			    p--;
		    }

		    p++; /* step forward to the start of the first string */
		    while(p < q){
			printf("\t%s\n", p);
			p += strlen(p) + 1;
		    }
		    free(stack);
		    return;
		}
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		return;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

/*
 * Get the infomation about the specified section of a mach object file
 */
static
long
get_sect_info(mh, lc, offset, size, addr, segname, sectname, flags)
struct mach_header *mh;
struct load_command *lc;
long *offset, *size, *addr, *flags;
char *segname, *sectname;
{
    long i, j;
    struct segment_command *sg;
    struct section *s;
    struct load_command *initlc;

	initlc = lc;

	*offset = 0;
	*size = 0;
	*addr = 0;
	if(flags != NULL)
	    *flags = 0;

	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		if(mh->filetype == MH_OBJECT ||
		   strcmp(sg->segname, segname) == 0){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0 ; j < sg->nsects ; j++){
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds){
			    printf("section structure command extends past end "
				   "of load commands\n");
			}
			if(strcmp(s->sectname, sectname) == 0){
			    *offset = s->offset;
			    *size = s->size;
			    *addr = s->addr;
			    if(flags != NULL)
				*flags = s->flags;
			    return(1);
			}
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds)
			    return(0);
			s++;
		    }
		}
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		return(0);
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		return(0);
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
	return(0);
}

/*
 * Get the relocation infomation about the specified section.
 */
static
void
get_rel_info(mh, lc, offset, size, num, segname, sectname)
struct mach_header *mh;
struct load_command *lc;
long *offset, *size, *num;
char *segname, *sectname;
{
    long i, j;
    struct segment_command *sg;
    struct section *s;
    struct load_command *initlc;

	initlc = lc;

	*offset = 0;
	*size = 0;
	*num = 0;

	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		if(mh->filetype == MH_OBJECT ||
		   strcmp(sg->segname, segname) == 0){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0 ; j < sg->nsects ; j++){
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds){
			    printf("section structure command extends past end "
				   "of load commands\n");
			}
			if(strcmp(s->sectname, sectname) == 0 &&
			   strcmp(s->segname, segname) == 0){
			    *offset = s->reloff;
			    *size = s->nreloc *
				    sizeof(struct relocation_info);
			    *num = s->nreloc;
			    return;
			}
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds)
			    return;
			s++;
		    }
		}
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		return;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

/*
 * Get the infomation about the symbol table of a mach object file
 */
static
void
get_sym_info(mh, lc, offset, size, num)
struct mach_header *mh;
struct load_command *lc;
long *offset, *size, *num;
{
    long i;
    struct symtab_command *st;
    struct load_command *initlc;

	initlc = lc;

	*offset = 0;
	*size = 0;
	*num = 0;

	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_SYMTAB){
		st = (struct symtab_command *)lc;
		*offset = st->symoff;
		*size = st->nsyms * sizeof(struct nlist);
		*num = st->nsyms;
		return;
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		return;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

/*
 * Get the infomation about the string table of a mach object file
 */
static
void
get_str_info(mh, lc, offset, size)
struct mach_header *mh;
struct load_command *lc;
long *offset, *size;
{
    long i;
    struct symtab_command *st;
    struct load_command *initlc;

	initlc = lc;

	*offset = 0;
	*size = 0;

	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_SYMTAB){
		st = (struct symtab_command *)lc;
		*offset = st->stroff;
		*size = st->strsize;
		return;
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		return;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

/*
 * Get the infomation about the gdb symbol segments of a mach object file
 */
static
void
get_gdb_info(mh, lc, offset, size)
struct mach_header *mh;
struct load_command *lc;
long *offset, *size;
{
    long i;
    struct symseg_command *ss;
    struct load_command *initlc;

	initlc = lc;

	*offset = 0;
	*size = 0;

	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_SYMSEG){
		ss = (struct symseg_command *)lc;
		*offset = ss->offset;
		*size = ss->size;
		return;
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		return;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		return;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

/*
 * Print the text segment.
 */
static
void
print_text(fd, offset, text_offset, text_size, text_addr,
	   filename, membername)
long fd, offset, text_offset, text_size, text_addr;
char *filename;
char *membername;
{
    char *text, *p;
    long i, j, start_offset;

	printf("Text segment\n");
	if((text = malloc(text_size)) == NIL){
	    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
		    sys_errlist[errno]);
	    exit(1);
	}
	lseek(fd, offset + text_offset, 0);
    	if((read(fd, text, text_size)) != text_size){
	    if(membername != NIL)
		fprintf(stderr, "%s : Can't read text of %s(%s) (%s)\n",
			progname, filename, membername, sys_errlist[errno]);
	    else
		fprintf(stderr, "%s : Can't read text of %s (%s)\n",
			progname, filename, sys_errlist[errno]);
	    free(text);
	    return;
	}
	if(vflag){
	    if(pflag){
		for(i = 0; i < nsort_syms; i++){
		    if(strcmp(sort_syms[i].n_un.n_name, pflag) == 0)
			break;
		}
		if(i == nsort_syms ||
		   ((sort_syms[i].n_type & N_TYPE) != N_TEXT &&
		    ((sort_syms[i].n_type & N_TYPE) != N_SECT ||
		      sort_syms[i].n_sect != 1)) ) {
		    if(membername != NIL)
			fprintf(stderr, "%s : Can't find text symbol %s in "
				"%s(%s)\n", progname, pflag, filename,
				membername);
		    else
			fprintf(stderr, "%s : Can't find text symbol %s in "
				"%s\n", progname, pflag, filename);
		    free(text);
		    return;
		}
		if(sort_syms[i].n_value < text_addr || 
		   sort_syms[i].n_value > text_addr + text_size){
		    if(membername != NIL)
			fprintf(stderr, "%s : text symbol %s not in text of "
				"%s(%s)\n", progname, pflag, filename,
				membername);
		    else
			fprintf(stderr, "%s : text symbol %s not in text of "
				"%s\n", progname, pflag, filename);
		    free(text);
		    return;
		}
		start_offset = sort_syms[i].n_value - text_addr;
		p = text + start_offset;
		text_addr = sort_syms[i].n_value;
	    }
	    else{
		start_offset = 0;
		p = text;
	    }
	    for(i = start_offset ; i < text_size ; ){
		print_label(text_addr, 1);
		j = disassemble(p, text_addr);
		p += j;
		text_addr += j;
		i += j;
	    }
	}
	else{
	    for(i = 0 ; i < text_size ; i += j , text_addr += j){
		printf("%08x ", text_addr);
		for(j = 0;
		    j < 8 * sizeof(short) && i + j < text_size;
		    j += sizeof(short))
		    printf("%04x ",
			   (long)*((short *)(text + i + j)) & 0x0000ffff);
		printf("\n");
	    }
	}
	free(text);
}

/*
 * Print_label prints a symbol name for the addr if a symbol exist with the
 * same address in label form, namely:.
 *
 * <symbol name>:\n
 *
 * The colon and the newline are printed if colon_and_newline is non-zero.
 */
static
void
print_label(addr, colon_and_newline)
long addr;
long colon_and_newline;
{
    long high, low, mid;

	low = 0;
	high = nsort_syms - 1;
	mid = (high - low) / 2;
	while(high >= low){
	    if(sort_syms[mid].n_value == addr){
		printf("%s",sort_syms[mid].n_un.n_name);
		if(colon_and_newline)
		    printf(":\n");
		return;
	    }
	    if(sort_syms[mid].n_value > addr){
		high = mid - 1;
		mid = (high + low) / 2;
	    }
	    else{
		low = mid + 1;
		mid = (high + low) / 2;
	    }
	}
}

/*
 * Print_symbol prints a symbol name for the addr if a symbol exist with the
 * same address.  Nothing else is printed, no whitespace, no newline.  If it
 * prints something then it returns TRUE, else it returns FALSE.
 */
static
long
print_symbol(value, pc)
long value, pc;
{
    long high, low, mid;
    struct scattered_relocation_info *sreloc;
    long r_address;

	if(!Vflag)
	    return(FALSE);

	low = 0;
	high = nsort_trelocs - 1;
	mid = (high - low) / 2;
	while(high >= low){
	    if((sort_treloc[mid].r_address) & R_SCATTERED != 0){
		sreloc = (struct scattered_relocation_info *)(sort_treloc+mid);
		r_address = sreloc->r_address;
	    }
	    else{
		if(sort_treloc[mid].r_address == pc){
		    if(sort_treloc[mid].r_extern &&
		       sort_treloc[mid].r_symbolnum >= 0 &&
		       sort_treloc[mid].r_symbolnum < nsyms){
			if(value != 0)
			    printf("%s+0x%x",
			       syms[sort_treloc[mid].r_symbolnum].n_un.n_name,
			       value);
			else
			    printf("%s",
			       syms[sort_treloc[mid].r_symbolnum].n_un.n_name);
			return(TRUE);
		    }
		    break;
		}
		r_address = sort_treloc[mid].r_address;
	    }
	    if(r_address > pc){
		high = mid - 1;
		mid = (high + low) / 2;
	    }
	    else{
		low = mid + 1;
		mid = (high + low) / 2;
	    }
	}

	low = 0;
	high = nsort_syms - 1;
	mid = (high - low) / 2;
	while(high >= low){
	    if(sort_syms[mid].n_value == value){
		printf("%s",sort_syms[mid].n_un.n_name);
		return(TRUE);
	    }
	    if(sort_syms[mid].n_value > value){
		high = mid - 1;
		mid = (high + low) / 2;
	    }
	    else{
		low = mid + 1;
		mid = (high + low) / 2;
	    }
	}
	return(FALSE);
}

/*
 * Print_symbol prints a symbol name for the addr if a symbol exist with the
 * same address.  Nothing else is printed, no whitespace, no newline.  If it
 * prints something then it returns TRUE, else it returns FALSE.
 */
static
long
print_symbol_in_init(value, pc)
long value, pc;
{
    long high, low, mid;
    struct scattered_relocation_info *sreloc;
    long r_address;

	low = 0;
	high = nsort_irelocs - 1;
	mid = (high - low) / 2;
	while(high >= low){
	    if((sort_ireloc[mid].r_address) & R_SCATTERED != 0){
		sreloc = (struct scattered_relocation_info *)(sort_ireloc+mid);
		r_address = sreloc->r_address;
	    }
	    else{
		if(sort_ireloc[mid].r_address == pc){
		    if(sort_ireloc[mid].r_extern &&
		       sort_ireloc[mid].r_symbolnum >= 0 &&
		       sort_ireloc[mid].r_symbolnum < nsyms){
			if(value != 0)
			    printf("%s+0x%x",
			       syms[sort_ireloc[mid].r_symbolnum].n_un.n_name,
			       value);
			else
			    printf("%s",
			       syms[sort_ireloc[mid].r_symbolnum].n_un.n_name);
			return(TRUE);
		    }
		    break;
		}
		r_address = sort_ireloc[mid].r_address;
	    }
	    if(r_address > pc){
		high = mid - 1;
		mid = (high + low) / 2;
	    }
	    else{
		low = mid + 1;
		mid = (high + low) / 2;
	    }
	}

	low = 0;
	high = nsort_syms - 1;
	mid = (high - low) / 2;
	while(high >= low){
	    if(sort_syms[mid].n_value == value){
		printf("%s",sort_syms[mid].n_un.n_name);
		return(TRUE);
	    }
	    if(sort_syms[mid].n_value > value){
		high = mid - 1;
		mid = (high + low) / 2;
	    }
	    else{
		low = mid + 1;
		mid = (high + low) / 2;
	    }
	}
	return(FALSE);
}

/*
 * Print the data segment.
 */
static
void
print_data(fd, offset, data_offset, data_size, data_addr, filename, membername)
long fd;
long offset, data_offset, data_size, data_addr;
char *filename;
char *membername;
{
    char *data;
    long i, j, addr;

	printf("Data segment\n");
	if((data = malloc(data_size)) == NIL){
	    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
		    sys_errlist[errno]);
	    exit(1);
	}
	lseek(fd, offset + data_offset, 0);
    	if((read(fd, data, data_size)) != data_size){
	    if(membername != NIL)
		fprintf(stderr, "%s : Can't read data of %s(%s) (%s)\n",
			progname, filename, membername, sys_errlist[errno]);
	    else
		fprintf(stderr, "%s : Can't read data of %s (%s)\n",
			progname, filename, sys_errlist[errno]);
	    free(data);
	    return;
	}
	for(i = 0 , addr = data_addr ; i < data_size ; i += j , addr += j){
	    printf("%08x ", addr);
	    for(j = 0;
		j < 8 * sizeof(short) && i + j < data_size;
		j += sizeof(short))
		printf("%04x ",
		       (long)*((short *)(data + i + j)) & 0x0000ffff);
	    printf("\n");
	}
	free(data);
}

/*
 * Print the contents of a section
 */
static
void
print_sect(mhp, lcp, fd, offset, sect_offset, sect_size, sect_addr, filename,
	   membername, flags)
struct mach_header *mhp;
struct load_command *lcp;
long fd;
long offset, sect_offset, sect_size, sect_addr;
char *filename;
char *membername;
long flags;
{
    char *sect;

	if(flags == S_ZEROFILL){
	    printf("Section (%0.16s,%0.16s) is a zerofill section and has no "
		   "contents in the file\n", segname, sectname);
	    return;
	}
	if(Xflag == FALSE)
	    printf("Contents of (%0.16s,%0.16s) section\n", segname, sectname);
	if((sect = malloc(sect_size)) == NIL){
	    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
		    sys_errlist[errno]);
	    exit(1);
	}
	lseek(fd, offset + sect_offset, 0);
    	if((read(fd, sect, sect_size)) != sect_size){
	    if(membername != NIL)
		fprintf(stderr, "%s : Can't read section (%0.16s,%0.16s) of "
			"%s(%s) (%s)\n", progname, segname, sectname, filename,
			membername, sys_errlist[errno]);
	    else
		fprintf(stderr, "%s : Can't read section (%0.16s,%0.16s) of %s"
			" (%s)\n", progname, segname, sectname, filename,
			sys_errlist[errno]);
	    free(sect);
	    return;
	}

	if(vflag){
	    switch(flags){
	    case 0:
		print_default_section(sect, sect_addr, sect_size);
		break;
	    case S_CSTRING_LITERALS:
		print_cstring_section(sect, sect_addr, sect_size);
		break;
	    case S_4BYTE_LITERALS:
		print_literal4_section(sect, sect_addr, sect_size);
		break;
	    case S_8BYTE_LITERALS:
		print_literal8_section(sect, sect_addr, sect_size);
		break;
	    case S_LITERAL_POINTERS:
		print_literal_pointer_section(mhp, lcp, fd, offset, filename,
					membername, sect, sect_addr, sect_size);
		break;
	    default:
		printf("Unknown section type (flags = 0x%x)\n", flags);
		print_default_section(sect, sect_addr, sect_size);
		break;
	    }
	}
	else
	    print_default_section(sect, sect_addr, sect_size);

	free(sect);
}

static
void
print_cstring_section(sect, sect_addr, sect_size)
char *sect;
long sect_addr;
long sect_size;
{
    long i;

	for(i = 0; i < sect_size ; i++){
	    if(Xflag == FALSE)
		printf("%08x  ", sect_addr + i);

	    for( ; i < sect_size && sect[i] != '\0'; i++)
		print_cstring_char(sect[i]);
	    if(i < sect_size && sect[i] == '\0')
		printf("\n");
	}
}

static
void
print_cstring_char(char c)
{
	if(isprint(c)){
	    if(c == '\\')	/* backslash */
		printf("\\\\");
	    else		/* all other printable characters */
		printf("%c", c);
	}
	else{
	    switch(c){
	    case '\n':		/* newline */
		printf("\\n");
		break;
	    case '\t':		/* tab */
		printf("\\t");
		break;
	    case '\v':		/* vertical tab */
		printf("\\v");
		break;
	    case '\b':		/* backspace */
		printf("\\b");
		break;
	    case '\r':		/* carriage return */
		printf("\\r");
		break;
	    case '\f':		/* formfeed */
		printf("\\f");
		break;
	    case '\a':		/* audiable alert */
		printf("\\a");
		break;
	    default:
		printf("\\%03o", c);
	    }
	}
}

static
void
print_literal4_section(sect, sect_addr, sect_size)
char *sect;
long sect_addr;
long sect_size;
{
    long i, l;
    float f;

	for(i = 0; i < sect_size ; i += sizeof(float)){
	    if(Xflag == FALSE)
		printf("%08x  ", sect_addr + i);
	    f = (float)*((float *)(sect + i));
	    l = (long)*((long *)(sect + i));
	    print_literal4(l, f);
	}
}

static
void
print_literal4(long l, float f)
{
	printf("0x%08x", l);
	if((l & 0x7f800000) != 0x7f800000){
	    printf(" (%.20e)\n", f);
	}
	else{
	    if(l == 0x7f800000)
		printf(" (+Infinity)\n");
	    else if(l == 0xff800000)
		printf(" (-Infinity)\n");
	    else if((l & 0x00400000) == 0x00400000)
		printf(" (non-signaling Not-a-Number)\n");
	    else
		printf(" (signaling Not-a-Number)\n");
	}
}

static
void
print_literal8_section(sect, sect_addr, sect_size)
char *sect;
long sect_addr;
long sect_size;
{
    long i, l0, l1;
    double d;

	for(i = 0; i < sect_size ; i += sizeof(double)){
	    if(Xflag == FALSE)
		printf("%08x  ", sect_addr + i);
	    d = (double)*((double *)(sect + i));
	    l0 = (long)*((long *)(sect + i)),
	    l1 = (long)*((long *)(sect + i + 4));
	    print_literal8(l0, l1, d);
	}
}

static
void
print_literal8(long l0, long l1, double d)
{
	printf("0x%08x 0x%08x", l0, l1);
	if(finite(d))
	    printf(" (%.20e)\n", d);
	else{
	    if(l0 == 0x7ff00000 && l1 == 0)
		printf(" (+Infinity)\n");
	    else if(l0 == 0xfff00000 && l1 == 0)
		printf(" (-Infinity)\n");
	    else if((l0 & 0x00080000) == 0x00080000)
		printf(" (non-signaling Not-a-Number)\n");
	    else
		printf(" (signaling Not-a-Number)\n");
	}
}

static
void
print_literal_pointer_section(mh, lc, fd, offset, filename, membername,
			      sect, sect_addr, sect_size)
struct mach_header *mh;
struct load_command *lc;
int fd;
long offset;
char *filename;
char *membername;
char *sect;
long sect_addr;
long sect_size;
{
    long i, j, l, k, l0, l1, found;
    struct load_command *init_lc;
    struct segment_command *sg;
    struct section *s;
    struct literal_section {
	struct section *s;
	char *contents;
    } *literal_sections;
    long nliteral_sections;
    char *contents;
    float f;
    double d;
    struct relocation_info *sort_reloc, *reloc;
    long nsort_relocs, rel_offset, rel_size;
    struct nlist *syms;
    long sym_offset, sym_size, nsyms;
    char *str;
    long str_offset, str_size;

	literal_sections = NULL;
	nliteral_sections = 0;
	init_lc = lc;
	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0 ; j < sg->nsects ; j++, s++){
		    if(s->flags == S_CSTRING_LITERALS ||
		       s->flags == S_4BYTE_LITERALS ||
		       s->flags == S_8BYTE_LITERALS){
			if((contents = malloc(s->size)) == NIL){
			    fprintf(stderr, "%s : Ran out of memory (%s)\n",
				    progname, sys_errlist[errno]);
			    exit(1);
			}
			lseek(fd, offset + s->offset, 0);
			if((read(fd, contents, s->size)) != s->size){
			    if(membername != NIL)
				fprintf(stderr, "%s : Can't read section "
					"(%0.16s,%0.16s) of %s(%s) (%s)\n",
					progname, segname, sectname, filename,
					membername, sys_errlist[errno]);
			    else
				fprintf(stderr, "%s : Can't read section "
					"(%0.16s,%0.16s) of %s (%s)\n",
					progname, segname, sectname, filename,
					sys_errlist[errno]);
			    free(contents);
			    continue;
			}
			if(literal_sections == NULL){
			    if((literal_sections = (struct literal_section *)
				malloc(sizeof(struct literal_section) *
				        (nliteral_sections + 1))) == NIL){
				fprintf(stderr, "%s : Ran out of memory (%s)\n",
					progname, sys_errlist[errno]);
				exit(1);
			    }
			}
			else{
			    if((literal_sections =(struct literal_section *)
				realloc(literal_sections,
				        sizeof(struct literal_section) *
				        (nliteral_sections + 1))) == NIL){
				fprintf(stderr, "%s : Ran out of memory (%s)\n",
					progname, sys_errlist[errno]);
				exit(1);
			    }
			}
			literal_sections[nliteral_sections].s = s;
			literal_sections[nliteral_sections].contents = contents;
			nliteral_sections++;
		    }
		}
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}

	/* get a sorted list of relocation entries for this section */
	get_rel_info(mh, init_lc, &rel_offset, &rel_size, &nsort_relocs,
		     segname, sectname);
	if((sort_reloc = (struct relocation_info *)malloc(rel_size)) == NULL){
	    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
		    sys_errlist[errno]);
	    exit(1);
	}
	lseek(fd, offset + rel_offset, 0);
	if((read(fd, sort_reloc, rel_size)) != rel_size){
	    if(membername != NIL)
		fprintf(stderr, "%s : Can't read relocation entries for section"
			" (%0.16s,%0.16s) of %s(%s) (%s)\n", progname, segname,
			sectname, filename, membername, sys_errlist[errno]);
	    else
		fprintf(stderr, "%s : Can't read relocation entries for section"
			" (%0.16s,%0.16s) of %s (%s)\n", progname, segname,
			sectname, filename, sys_errlist[errno]);
	    sort_reloc = (struct relocation_info *)0;
	    nsort_relocs = 0;
	}
	else
	    qsort(sort_reloc, nsort_relocs, sizeof(struct relocation_info),
		  (int (*)(const void *, const void *))rel_compare);

	/* get the symbol table */
	get_sym_info(mh, init_lc, &sym_offset, &sym_size, &nsyms);
	if((syms = (struct nlist *)malloc(sym_size)) == (struct nlist *)0){
	    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
		    sys_errlist[errno]);
	    exit(1);
	}
	lseek(fd, offset + sym_offset, 0);
	if((read(fd, syms, sym_size)) != sym_size){
	    if(membername != NIL)
		fprintf(stderr, "%s : Can't read symbol table of %s(%s) (%s)\n",
			progname, filename, membername, sys_errlist[errno]);
	    else
		fprintf(stderr, "%s : Can't read symbol table %s (%s)\n",
			progname, filename, sys_errlist[errno]);
	    syms = (struct nlist *)0;
	    nsyms = 0;
	}

	/* get the string table */
	get_str_info(mh, init_lc, &str_offset, &str_size);
	if((str = (char *)malloc(str_size)) == NIL){
	    fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
		    sys_errlist[errno]);
	    exit(1);
	}
	lseek(fd, offset + str_offset, 0);
	if((read(fd, str, str_size)) != str_size){
	    if(membername != NIL)
		fprintf(stderr, "%s : Can't read string table of %s(%s) (%s)\n",
			progname, filename, membername,
			sys_errlist[errno]);
	    else
		fprintf(stderr, "%s : Can't read string table of %s (%s)\n",
			progname, filename, sys_errlist[errno]);
	    str = NIL;
	    str_size = 0;
	}

	/* loop through the literal pointer section and print the pointers */
	for(i = 0; i < sect_size ; i += sizeof(long)){
	    if(Xflag == FALSE)
		printf("%08x  ", sect_addr + i);
	    l = (long)*((long *)(sect + i));
	    /*
	     * If there is an external relocation entry for this pointer then
	     * print the symbol and any offset.
	     */
	    reloc = bsearch(&i, sort_reloc, nsort_relocs,
		   	    sizeof(struct relocation_info),
			    (int (*)(const void *, const void *))rel_bsearch);
	    if(reloc != NULL && (reloc->r_address & R_SCATTERED) == 0 &&
	       reloc->r_extern == 1){
		printf("external relocation entry for symbol:");
		if(reloc->r_symbolnum >= 0 && reloc->r_symbolnum < nsyms &&
		   syms[reloc->r_symbolnum].n_un.n_strx > 0 &&
		   syms[reloc->r_symbolnum].n_un.n_strx < str_size){
		    if(l != 0)
			printf("%s+0x%x\n",
			       str + syms[reloc->r_symbolnum].n_un.n_strx, l);
		    else
			printf("%s\n",
			       str + syms[reloc->r_symbolnum].n_un.n_strx);
		}
		else{
		    printf("bad relocation entry\n");
		}
		continue;
	    }
	    found = FALSE;
	    for(j = 0; j < nliteral_sections; j++){
		if(l >= literal_sections[j].s->addr &&
		   l < literal_sections[j].s->addr +
		       literal_sections[j].s->size){
		    printf("%0.16s:%0.16s:", literal_sections[j].s->segname,
			   literal_sections[j].s->sectname);
		    switch(literal_sections[j].s->flags){
		    case S_CSTRING_LITERALS:
			for(k = l - literal_sections[j].s->addr;
			    k < literal_sections[j].s->size &&
					literal_sections[j].contents[k] != '\0';
			    k++)
			    print_cstring_char(literal_sections[j].contents[k]);
			printf("\n");
			break;
		    case S_4BYTE_LITERALS:
			l0 = (long)*((long *)(literal_sections[j].contents +
					      l - literal_sections[j].s->addr));
			f = (float)*((float *)(literal_sections[j].contents +
					      l - literal_sections[j].s->addr));
			print_literal4(l0, f);
			break;
		    case S_8BYTE_LITERALS:
			l0 = (long)*((long *)(literal_sections[j].contents +
					      l - literal_sections[j].s->addr));
			l1 = (long)*((long *)(literal_sections[j].contents + 4 +
					      l - literal_sections[j].s->addr));
			d = (double)*((double *)(literal_sections[j].contents +
					      l - literal_sections[j].s->addr));
			print_literal8(l0, l1, d);
			break;
		    }
		    found = TRUE;
		    break;
		}
	    }
	    if(found == FALSE){
		printf("0x%x (not in a literal section)\n", l);
	    }
	}

	if(literal_sections != NULL){
	    for(i = 0; i < nliteral_sections; i++)
	        free(literal_sections[i].contents);
	    free(literal_sections);
	}
	if(sort_reloc != NULL)
	    free(sort_reloc);
	if(syms != NULL)
	    free(syms);
	if(str != NULL)
	    free(str);
}

static
void
print_default_section(sect, sect_addr, sect_size)
char *sect;
long sect_addr;
long sect_size;
{
    long i, j, addr;

	for(i = 0 , addr = sect_addr ; i < sect_size ; i += j , addr += j){
	    if(Xflag == FALSE)
		printf("%08x  ", addr);
	    for(j = 0;
		j < 4 * sizeof(long) && i + j < sect_size;
		j += sizeof(long))
		printf("%08x ", (long)*((long *)(sect + i + j)));
	    printf("\n");
	}
}

/*
 * Print the objc segment.
 */
static
void
print_objc(fd, offset, mh, lc, filename, membername)
long fd;
long offset;
struct mach_header *mh;
struct load_command *lc;
char *filename;
char *membername;
{

    long i, j;
    struct segment_command *sg;
    struct section *s, *sym, *mod, *sel;
    struct objc_module *modules, *m;
    struct objc_symtab *symbols, *t;
    char *selectors, *p, **sel_ref;
    struct objc_class *objc_class;
    struct objc_category *objc_category;
    struct objc_ivar_list *ilist;
    struct objc_ivar *ivar;
    struct objc_method_list *mlist;
    struct load_command *initlc;

	printf("Objc segment printing currently not updated with respect to "
	       "removal of the (__OBJC,__symbol_table) section");

	initlc = lc;

	sym = NULL;
	mod = NULL;
	sel = NULL;
	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		if(mh->filetype == MH_OBJECT ||
		   strcmp(sg->segname, SEG_OBJC) == 0){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0 ; j < sg->nsects ; j++){
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds){
			    printf("section structure command extends past end "
				   "of load commands\n");
			}
			if(strcmp(s->sectname, SECT_OBJC_SYMBOLS) == 0){
			    if(sym != NULL)
				printf("more than one " SECT_OBJC_SYMBOLS
				       " section\n");
			    else
				sym = s;
			}
			else if(strcmp(s->sectname, SECT_OBJC_MODULES) == 0){
			    if(mod != NULL)
				printf("more than one " SECT_OBJC_MODULES
				       " section\n");
			    else
				mod = s;
			}
			else if(strcmp(s->sectname, SECT_OBJC_STRINGS) == 0){
			    if(sel != NULL)
				printf("more than one " SECT_OBJC_STRINGS
				       " section\n");
			    else
				sel = s;
			}
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds)
			    break;
			s++;
		    }
		}
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		break;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");

	printf("Objective-C segment\n");
	if(mod == NULL)
	    printf("No " SECT_OBJC_MODULES " section header\n");
	else if(mod->size <= 0){
	    printf("Bad size of " SECT_OBJC_MODULES " section (%d)\n",
		   mod->size);
	    mod->size = 0;
	}
	if(sym == NULL)
	    printf("No " SECT_OBJC_SYMBOLS " section header\n");
	else if(sym->size <= 0){
	    printf("Bad size of " SECT_OBJC_SYMBOLS " section (%d)\n",
		   sym->size);
	    sym->size = 0;
	}
	if(sel == NULL)
	    printf("No " SECT_OBJC_STRINGS " section header\n");
	else if(sel->size <= 0){
	    printf("Bad size of " SECT_OBJC_STRINGS " section (%d)\n",
		   sel->size);
	    sel->size = 0;
	}
	if(mod != NULL && mod->size > 0){
	    if((modules = (struct objc_module *)malloc(mod->size)) == NIL){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + mod->offset, 0);
	    if((read(fd, modules, mod->size)) != mod->size){
		if(membername != NIL)
		    fprintf(stderr,"%s : Can't read modules of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr, "%s : Can't read modules of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		free(modules);
		return;
	    }
	}
	if(sym != NULL && sym->size > 0){
	    if((symbols = (struct objc_symtab *)malloc(sym->size)) == NIL){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + sym->offset, 0);
	    if((read(fd, symbols, sym->size)) != sym->size){
		if(membername != NIL)
		    fprintf(stderr,"%s : Can't read symbols of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr, "%s : Can't read symbols of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		free(symbols);
		return;
	    }
	}
	if(sel != NULL && sel->size > 0){
	    if((selectors = malloc(sel->size)) == NIL){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + sel->offset, 0);
	    if((read(fd, selectors, sel->size)) != sel->size){
		if(membername != NIL)
		    fprintf(stderr,"%s : Can't read selectors of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr, "%s : Can't read selectors of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		free(selectors);
		return;
	    }
	}

	if(oflag && mod != NULL){
	    for(m = modules;
	        (char *)m < (char *)modules + mod->size;
		m = (struct objc_module *)((char *)m + m->size) ){

		if((char *)m + m->size > (char *)m + mod->size)
		    printf("module extends past end of " SECT_OBJC_MODULES
			   " section\n");
		printf("Module 0x%x\n",
			mod->addr + (char *)m - (char *)modules);

		printf("    version %d\n", m->version);
		printf("       size %d\n", m->size);
		if(vflag){
		    if(sel != NULL && (long)m->name >= sel->addr &&
		       (long)m->name < sel->addr + sel->size)
			printf("       name %s\n",
			       selectors + (long)m->name - sel->addr);
		    else
			printf("       name 0x%08x (not in " SECT_OBJC_STRINGS
			       " section)\n", m->name);
		}
		else
		    printf("       name 0x%08x\n", m->name);

		if(sym == NULL || (long)m->symtab < sym->addr ||
		   (long)m->symtab >= sym->addr + sym->size){
		    printf("     symtab 0x%08x (not in " SECT_OBJC_SYMBOLS
			   " section)\n", m->symtab);
		    continue;
		}
		printf("     symtab 0x%08x\n", m->symtab);
		t = (struct objc_symtab *)((char *)symbols +
					   ((long)m->symtab - sym->addr));
		if((char *)t + sizeof(struct objc_symtab) >
		   (char *)symbols + sym->size)
		    printf("\tsymtab extends past end of " SECT_OBJC_SYMBOLS
			   " section\n");
		printf("\tsel_ref_cnt %d\n", t->sel_ref_cnt);
		if((long)t->refs >= sym->addr &&
		   (long)t->refs < sym->addr + sym->size){
		    printf("\trefs 0x%08x", t->refs);
		    if((long)t->refs + t->sel_ref_cnt * sizeof(char *) >
							sym->addr + sym->size)
			printf(" (extends past end of " SECT_OBJC_SYMBOLS
			       " section)\n");
		    else
			printf("\n");
		    sel_ref = (char **)((char *)symbols +
					((long)t->refs - sym->addr));
		    for(i = 0; i < t->sel_ref_cnt; i++){
			if((char *)sel_ref + i * sizeof(char *) >
			   (char *)symbols + sym->addr){
			    printf("\t    (remaining sel_ref entries past the "
				   "end of the " SECT_OBJC_SYMBOLS
				   " section)\n");
			    break;
			}
			printf("\t    0x%08x", sel_ref[i]);
			if(vflag){
			    if((long)sel_ref[i] >= sel->addr &&
			       (long)sel_ref[i] < sel->addr + sel->size)
				printf(" %s\n", selectors +
				       (long)sel_ref[i] - sel->addr);
			     else
				printf(" (not in " SECT_OBJC_STRINGS
			       	       " section)\n");
			}
			else
			    printf("\n");
		    }
		}
		else
		    printf("\trefs 0x%08x (not in " SECT_OBJC_SYMBOLS
			   " section)\n", t->refs);

		printf("\tcls_def_cnt %d\n", t->cls_def_cnt);
		printf("\tcat_def_cnt %d\n", t->cat_def_cnt);
		if(t->cls_def_cnt > 0)
		    printf("\tClass Definitions\n");
		for(i = 0; i < t->cls_def_cnt; i++){
		    if((long)&(t->defs[i]) - (long)symbols > sym->size){
			printf("\t(remaining class defs entries entends past "
			       "the end of the " SECT_OBJC_SYMBOLS
			       " section)\n");
			break;
		    }
		    if((long)t->defs[i] >= sym->addr &&
		       (long)t->defs[i] < sym->addr + sym->size){
			printf("\tdefs[%d] 0x%08x", i, t->defs[i]);

			objc_class = (struct objc_class *)((char *)symbols +
				      ((long)t->defs[i] - sym->addr));
print_objc_class:
			if((long)objc_class + sizeof(struct objc_class) -
			   (long)symbols > sym->size)
			    printf(" (entends past the end of the "
				   SECT_OBJC_SYMBOLS " section)\n");
			else
			    printf("\n");
			printf("\t\t      isa 0x%08x", objc_class->isa);
			if(vflag && CLS_GETINFO(objc_class, CLS_META)){
			    if((long)objc_class->isa >= sel->addr &&
			       (long)objc_class->isa < sel->addr + sel->size)
				printf(" %s\n", selectors +
				       (long)objc_class->isa - sel->addr);
			     else
				printf(" (not in " SECT_OBJC_STRINGS
			       	       " section)\n");
			}
			else
			    printf("\n");

			printf("\t      super_class 0x%08x",
				objc_class->super_class);
			if(vflag){
			    if((long)objc_class->super_class >= sel->addr &&
			       (long)objc_class->super_class < sel->addr + sel->size)
				printf(" %s\n", selectors +
				       (long)objc_class->super_class - sel->addr);
			     else
				printf(" (not in " SECT_OBJC_STRINGS
			       	       " section)\n");
			}
			else
			    printf("\n");

			printf("\t\t     name 0x%08x", objc_class->name);
			if(vflag){
			    if((long)objc_class->name >= sel->addr &&
			       (long)objc_class->name < sel->addr + sel->size)
				printf(" %s\n", selectors +
				       (long)objc_class->name - sel->addr);
			     else
				printf(" (not in " SECT_OBJC_STRINGS
			       	       " section)\n");
			}
			else
			    printf("\n");
			printf("\t\t  version 0x%08x\n", objc_class->version);
			printf("\t\t     info 0x%08x", objc_class->info);
			if(vflag){
			    if(CLS_GETINFO(objc_class, CLS_CLASS))
				printf(" CLS_CLASS\n");
			    else if(CLS_GETINFO(objc_class, CLS_META))
				printf(" CLS_META\n");
			    else
				printf("\n");
                        }
			else
			    printf("\n");
			printf("\t    instance_size 0x%08x\n",
				objc_class->instance_size);

			printf("\t\t    ivars 0x%08x", objc_class->ivars);
			if((long)objc_class->ivars >= sym->addr &&
			   (long)objc_class->ivars < sym->addr + sym->size){
			    printf("\n");
			    ilist = (struct objc_ivar_list *)((char *)symbols +
				    ((long)objc_class->ivars - sym->addr));
			    if((char *)ilist + sizeof(struct objc_ivar_list) >
			       (char *)symbols + sym->size)
				printf("\t\t objc_ivar_list extends past end "
				       "of " SECT_OBJC_SYMBOLS " section\n");

			    printf("\t\t       ivar_count %d\n", 
					ilist->ivar_count);
			    ivar = ilist->ivar_list;
			    for(j = 0; j < ilist->ivar_count; j++, ivar++){
				if((char *)ivar > (char *)symbols + sym->size){
				    printf("\t\t remaining ivar's extend past "
					   " the of " SECT_OBJC_SYMBOLS
					   " section\n");
				    continue;
				}
				printf("\t\t\tivar_name 0x%08x",
				       ivar->ivar_name);
				if(vflag){
				    if((long)ivar->ivar_name >= sel->addr &&
				       (long)ivar->ivar_name <
							sel->addr + sel->size)
					printf(" %s\n", selectors +
					   (long)ivar->ivar_name - sel->addr);
				     else
					printf(" (not in " SECT_OBJC_STRINGS
					       " section)\n");
				}
				else
				    printf("\n");
				printf("\t\t\tivar_type 0x%08x",
				       ivar->ivar_type);
				if(vflag){
				    if((long)ivar->ivar_type >= sel->addr &&
				       (long)ivar->ivar_type <
							sel->addr + sel->size)
					printf(" %s\n", selectors +
					   (long)ivar->ivar_type - sel->addr);
				     else
					printf(" (not in " SECT_OBJC_STRINGS
					       " section)\n");
				}
				else
				    printf("\n");
				printf("\t\t      ivar_offset 0x%08x\n",
				       ivar->ivar_offset);
			    }
			}
			else{
			    printf(" (not in " SECT_OBJC_SYMBOLS " section)\n");
			}

			printf("\t\t  methods 0x%08x", objc_class->methods);
			if((long)objc_class->methods >= sym->addr &&
			   (long)objc_class->methods < sym->addr + sym->size){
			    printf("\n");
			    mlist = (struct objc_method_list *)((char *)symbols
				    + ((long)objc_class->methods - sym->addr));
			    print_method_list(mlist, sym, sel, symbols,
					      selectors);
			}
			else{
			    printf(" (not in " SECT_OBJC_SYMBOLS " section)\n");
			}
			printf("\t\t    cache 0x%08x\n", objc_class->cache);

			if(CLS_GETINFO(objc_class, CLS_CLASS)){
			    printf("\tMeta Class");
			    if((long)objc_class->isa >= sym->addr &&
			       (long)objc_class->isa < sym->addr + sym->size){
				objc_class = (struct objc_class *)
					     ((char *)symbols +
					((long)(objc_class->isa) - sym->addr));
				goto print_objc_class;
			    }
			    else
				printf(" (not in " SECT_OBJC_SYMBOLS
				       " section)\n");
			}
		    }
		    else
			printf("\tdefs[%d] 0x08x (not in " SECT_OBJC_SYMBOLS
			       " section)\n", i, t->defs[i]);
		}
		if(t->cat_def_cnt > 0)
		    printf("\tCategory Definitions\n");
		for(i = 0; i < t->cat_def_cnt; i++){
		    if((long)&(t->defs[i + t->cls_def_cnt]) - (long)symbols >
			sym->size){
			printf("\t(remaining category defs entries entends "
			       "past the end of the " SECT_OBJC_SYMBOLS
			       " section)\n");
			break;
		    }
		    if((long)t->defs[i + t->cls_def_cnt] >= sym->addr &&
		       (long)t->defs[i + t->cls_def_cnt] <
							sym->addr + sym->size){
			printf("\tdefs[%d] 0x%08x", i + t->cls_def_cnt,
			       t->defs[i + t->cls_def_cnt]);

			objc_category = (struct objc_category *)((char *)symbols
			     + ((long)t->defs[i + t->cls_def_cnt] - sym->addr));
			if((long)objc_category + sizeof(struct objc_category) -
			   (long)symbols > sym->size)
			    printf(" (entends past the end of the "
				   SECT_OBJC_SYMBOLS " section)\n");
			else
			    printf("\n");
			printf("\t       category name 0x%08x",
			       objc_category->category_name);
			if(vflag){
			    if((long)objc_category->category_name >=
								   sel->addr &&
			       (long)objc_category->category_name <
							sel->addr + sel->size)
				printf(" %s\n", selectors +
				       (long)objc_category->category_name -
				       sel->addr);
			     else
				printf(" (not in " SECT_OBJC_STRINGS
			       	       " section)\n");
			}
			else
			    printf("\n");
			printf("\t\t  class name 0x%08x",
			       objc_category->class_name);
			if(vflag){
			    if((long)objc_category->class_name >= sel->addr &&
			       (long)objc_category->class_name <
							  sel->addr + sel->size)
				printf(" %s\n", selectors +
				       (long)objc_category->class_name -
				       sel->addr);
			     else
				printf(" (not in " SECT_OBJC_STRINGS
			       	       " section)\n");
			}
			else
			    printf("\n");
			printf("\t    instance methods 0x%08x",
			       objc_category->instance_methods);
			if((long)objc_category->instance_methods >= sym->addr &&
			   (long)objc_category->instance_methods <
							 sym->addr + sym->size){
			    printf("\n");
			    mlist = (struct objc_method_list *)((char *)symbols
				    + ((long)objc_category->instance_methods -
				    sym->addr));
			    print_method_list(mlist, sym, sel, symbols,
					      selectors);
			}
			else{
			    printf(" (not in " SECT_OBJC_SYMBOLS " section)\n");
			}
			printf("\t       class methods 0x%08x",
			       objc_category->class_methods);
			if((long)objc_category->class_methods >= sym->addr &&
			   (long)objc_category->class_methods <
							 sym->addr + sym->size){
			    printf("\n");
			    mlist = (struct objc_method_list *)((char *)symbols
				    + ((long)objc_category->class_methods -
				    sym->addr));
			    print_method_list(mlist, sym, sel, symbols,
					      selectors);
			}
			else{
			    printf(" (not in " SECT_OBJC_SYMBOLS " section)\n");
			}
		    }
		    else
			printf("\tdefs[%d] 0x08x (not in " SECT_OBJC_SYMBOLS
			       " section)\n", i + t->cls_def_cnt,
			       t->defs[i + t->cls_def_cnt]);
		}
	    }
	}
	if(Oflag){
	    if(sel != NULL){
		printf("Selectors\n\toffset\tselector\n");
		for(p = selectors; p < selectors + sel->size; p += strlen(p)+1)
		    printf("\t%-7d\t%s\n", p - selectors, p);
	    }
	}

	if(sym != NULL)
	    free(symbols);
	if(mod != NULL)
	    free(modules);
	if(sel != NULL)
	    free(selectors);
}

static
void
print_method_list(mlist, sym, sel, symbols, selectors)
struct objc_method_list *mlist;
struct section *sym, *sel;
struct objc_symtab *symbols;
char *selectors;
{
    long i;
    struct objc_method *method;

	if((char *)mlist + sizeof(struct objc_method_list) >
	   (char *)symbols + sym->size)
	    printf("\t\t objc_method_list extends past end "
		   "of " SECT_OBJC_SYMBOLS " section\n");

	printf("\t\t      method_next 0x%08x\n",
	       mlist->method_next);
	printf("\t\t     method_count %d\n",
	       mlist->method_count);
	
	method = mlist->method_list;
	for(i = 0; i < mlist->method_count; i++, method++){
	    if((char *)method > (char *)symbols + sym->size){
		printf("\t\t remaining method's extend past the of "
		       SECT_OBJC_SYMBOLS " section\n");
		return;
	    }
	    printf("\t\t      method_name 0x%08x",
		   method->method_name);
	    if(vflag){
		if((long)method->method_name >= sel->addr &&
		   (long)method->method_name < sel->addr + sel->size)
		    printf(" %s\n", selectors +
		           (long)method->method_name - sel->addr);
		 else
		    printf(" (not in " SECT_OBJC_STRINGS " section)\n");
	    }
	    else
		printf("\n");

	    printf("\t\t     method_types 0x%08x",
		   method->method_types);
	    if(vflag){
		if((long)method->method_types >= sel->addr &&
		   (long)method->method_types < sel->addr + sel->size)
		    printf(" %s\n", selectors +
			   (long)method->method_types - sel->addr);
		 else
		    printf(" (not in " SECT_OBJC_STRINGS " section)\n");
	    }
	    else
		printf("\n");
	    printf("\t\t       method_imp 0x%08x ",
		   method->method_imp);
	    if(vflag)
		print_label((long)method->method_imp, 0);
	    printf("\n");
	}
}

/*
 * Print the shared library initialization table.
 */
static
void
print_shlib_init(fd, offset, mh, lc, filename, membername)
long fd;
long offset;
struct mach_header *mh;
struct load_command *lc;
char *filename;
char *membername;
{
    char *text, *data, *init;
    long text_offset, text_size, text_addr,
	 data_offset, data_size, data_addr,
	 init_offset, init_size, init_addr;
    long shlib_addr, *shlib_vector;
    struct shlib_init *shlib_init;
    struct nlist *sp;

	init = NULL;
	if(get_sect_info(mh, lc, &init_offset, &init_size, &init_addr,
		         SEG_TEXT, SECT_FVMLIB_INIT0, NULL)){
	    if((init = malloc(init_size)) == NIL){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + init_offset, 0);
	    if((read(fd, init, init_size)) != init_size){
		if(membername != NIL)
		    fprintf(stderr, "%s : Can't read init of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr, "%s : Can't read init of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		free(init);
		return;
	    }
	}
	else{
	    get_sect_info(mh, lc, &text_offset, &text_size, &text_addr,
			  SEG_TEXT, SECT_TEXT, NULL);
	    if((text = malloc(text_size)) == NIL){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + text_offset, 0);
	    if((read(fd, text, text_size)) != text_size){
		if(membername != NIL)
		    fprintf(stderr, "%s : Can't read text of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr, "%s : Can't read text of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		free(text);
		return;
	    }

	    get_sect_info(mh, lc, &data_offset, &data_size, &data_addr,
			  SEG_DATA, SECT_DATA, NULL);
	    if((data = malloc(data_size)) == NIL){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    lseek(fd, offset + data_offset, 0);
	    if((read(fd, data, data_size)) != data_size){
		if(membername != NIL)
		    fprintf(stderr, "%s : Can't read data of %s(%s) (%s)\n",
			    progname, filename, membername, sys_errlist[errno]);
		else
		    fprintf(stderr, "%s : Can't read data of %s (%s)\n",
			    progname, filename, sys_errlist[errno]);
		free(data);
		return;
	    }
	}

	if(init){
	    printf("Shared library initialization\n");
	    shlib_init = (struct shlib_init *)init;
	    while(shlib_init < (struct shlib_init *)(init + init_size)){
		printf("\tvalue   0x%08x ", shlib_init->value);
		if(vflag)
		    print_symbol_in_init(shlib_init->value, (long)(init_addr +
				 ((char *)(&shlib_init->value) - init)));
		printf("\n");
		printf("\taddress 0x%08x ", shlib_init->address);
		if(vflag)
		    print_symbol_in_init(shlib_init->address, (long)(init_addr +
				 ((char *)(&shlib_init->address) - init)));
		printf("\n");
		shlib_init++;
	    }
	    free(init);
	}
	else{
	    qsort(sort_syms, nsort_syms, sizeof(struct nlist),
		  (int (*)(const void *, const void *))symname_compare);
	    sp = bsearch(_SHLIB_INIT, sort_syms, nsort_syms,
			 sizeof(struct nlist),
			 (int (*)(const void *, const void *))cmp_bsearch);
	    if(sp == NULL){
		if(mh->filetype == MH_OBJECT){
		    free(text);
		    free(data);
		    return;
		}
		printf("Can't find symbol: " _SHLIB_INIT " (trying default "
		       "offset)\n");
		if(text_size < 0x7a + sizeof(long)){
		    printf("default offset not in text section\n");
		    free(text);
		    free(data);
		    return;
		}
		shlib_addr = *((long *)(text + 0x7a));
	    }
	    else
		shlib_addr = sp->n_value;
	    if(shlib_addr < data_addr || shlib_addr >= data_addr + data_size){
		printf("Value of " _SHLIB_INIT " 0x%x not in the data "
		       "section\n", shlib_addr);
		return;
	    }
	    qsort(sort_syms, nsort_syms, sizeof(struct nlist),
		  (int (*)(const void *, const void *))sym_compare);

	    shlib_vector = (long *)(data + shlib_addr - data_addr);
	    printf("Shared library initialization table at 0x%x ", shlib_addr);
	    if(vflag)
		print_label(shlib_addr, 0);
	    printf("\n");
	    while(shlib_vector <= (long *)(data + data_size)){
		printf("    0x%08x ", *shlib_vector);
		if(*shlib_vector == 0){
		    printf("\n");
		    break;
		}
		if(vflag)
		    print_label(*shlib_vector, 0);
		if(*shlib_vector < text_addr ||
		   *shlib_vector >= text_addr + text_size){
		    printf(" (not in the text section)\n");
		}
		else{
		    printf("\n");
		    shlib_init = (struct shlib_init *)
				 (text + *shlib_vector - text_addr);
		    while(shlib_init <=(struct shlib_init *)(text + text_size)){
			printf("\tvalue   0x%08x ", shlib_init->value);
			if(vflag && shlib_init->value != 0)
			    print_label(shlib_init->value, 0);
			printf("\n");
			printf("\taddress 0x%08x ", shlib_init->address);
			if(vflag && shlib_init->address != 0)
			    print_label((long)shlib_init->address, 0);
			printf("\n");
			if(shlib_init->value == 0 || shlib_init->address == 0)
			    break;
			shlib_init++;
		    }
		}
		shlib_vector++;
	    }
	    free(text);
	    free(data);
	}
}

/*
 * Function for bsearch for finding a symbol.
 */
static
int
cmp_bsearch(
const char *name,
const struct nlist *sym)
{
	return(strcmp(name, sym->n_un.n_name));
}

/*
 * This routine was used for gathering stats on what the number of runtime
 * relocation entries would be like.
 */
static
void
extern_text_reloc(reloc, nreloc, syms, nsyms, str, str_size)
struct relocation_info *reloc;
long nreloc;
struct nlist *syms;
long nsyms;
char *str;
long str_size;
{
    struct relocation_info *p;
    long i;

	for(i = 0 ; i < nreloc ; i++){
	    p = reloc + i;
	    if(p->r_extern){
		if(syms == (struct nlist *)0 || str == NIL ||
		   p->r_symbolnum > nsyms ||
		   syms[p->r_symbolnum].n_un.n_strx > str_size)
		    printf("?(%d)\n", p->r_symbolnum);
		else{
		    printf("%s\n", str + syms[p->r_symbolnum].n_un.n_strx);
		}
	    }
	}
}

static
void
branch_table(filename)
char *filename;
{
    long i, len, first, last;
	
	first = slot;
#ifdef PAD_SLOTS
	printf("\n");
	printf("## %s\n", filename);
#endif
	for(i = 0; i < nsyms; i++){
	    if(syms[i].n_type & N_EXT &&
	       (((syms[i].n_type & N_TYPE) == N_TEXT) ||
	       (((syms[i].n_type & N_TYPE) == N_SECT) && syms[i].n_sect == 1))){
#ifdef PAD_SLOTS
		if(slot != first)
		    printf("\n");
#endif
		printf("\t%s", syms[i].n_un.n_name);
		len = strlen(syms[i].n_un.n_name);
		switch(len / 8){
		case 0:
		    printf("\t\t\t\t");
		    break;
		case 1:
		    printf("\t\t\t");
		    break;
		case 2:
		    printf("\t\t");
		    break;
		case 3:
		    printf("\t");
		    break;
		default:
		    printf(" ");
		    break;
		}
#ifdef PAD_SLOT
		printf("%d", slot);
#else
		printf("\n");
#endif
		slot++;
	    }
	}
#ifdef PAD_SLOTS
	if(slot == first){
	    printf("## no text symbols\n");
	    return;
	}
	else{
	    last = slot;
	    if(slot - first > 2)
		slot = ((slot / 10) * 10) + 19;
	    else
		slot = ((slot / 10) * 10) + 9;
	}
	printf("\n\t.empty_slot\t\t\t%d-%d\n", last, slot);
	slot++;
#endif
}

/*
 * Print the relocation entries of all mach object file sections
 */
static
void
print_mach_reloc(fd, offset, filename, membername, mhp, load_commands,
		 syms, nsyms, str, str_size)
int fd;
long offset;
char *filename;
char *membername;
struct mach_header *mhp;
struct load_command *load_commands;
struct nlist *syms;
long nsyms;
char *str;
long str_size;
{
    long i, j, size;
    struct load_command *lc;
    struct segment_command *sg;
    struct section *s;
    struct relocation_info *reloc;

	lc = load_commands;
	for(i = 0 ; i < mhp->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command size not a multiple of sizeof(long)\n");
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		s = (struct section *)
		    ((char *)sg + sizeof(struct segment_command));
		for(j = 0 ; j < sg->nsects ; j++){
		    size = s->nreloc * sizeof(struct relocation_info);
		    if((reloc = (struct relocation_info *)malloc(size)) ==NULL){
			fprintf(stderr, "%s : Ran out of memory (%s)\n",
				progname, sys_errlist[errno]);
			exit(1);
		    }
		    printf("Relocation information (%0.16s,%0.16s) %d entries"
			   "\n", s->segname, s->sectname, s->nreloc);
		    lseek(fd, offset + s->reloff, 0);
		    if((read(fd, reloc, size)) != size){
			if(membername != NIL)
			    fprintf(stderr, "%s : Can't read relocation "
				    "entries for %0.16s section of %0.16s "
				    "segment from %s(%s) (%s)\n", progname,
				    s->sectname, s->segname, filename,
				    membername, sys_errlist[errno]);
			else
			    fprintf(stderr, "%s : Can't read relocation "
				    "entries for %0.16s section of %0.16s "
				    "segment from %s (%s)\n", progname,
				    s->sectname, s->segname, filename,
				    sys_errlist[errno]);
		    }
		    else{
			if(s->nreloc != 0)
			    print_reloc(reloc, s->nreloc, syms, nsyms, str,
					str_size, TRUE, mhp, load_commands);
		    }
		    free(reloc);
		    s++;
		}
	    }
	    if(lc->cmdsize <= 0)
		break;
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * Print the relocation information.
 */
static
void
print_reloc(reloc, nreloc, syms, nsyms, str, str_size, mach, mhp, load_commands)
struct relocation_info *reloc;
long nreloc;
struct nlist *syms;
long nsyms;
char *str;
long str_size;
long mach;
struct mach_header *mhp;
struct load_command *load_commands;
{
    long i, j, k, nsects;
    struct relocation_info *p;
    struct scattered_relocation_info *sr;
    struct load_command *lcp;
    struct segment_command *sgp;
    struct section *sp, **sections;

	if(mach & vflag){
	    nsects = 0;
	    lcp = load_commands;
	    for(i = 0; i < mhp->ncmds; i++){
		if(lcp->cmd == LC_SEGMENT){
		    sgp = (struct segment_command *)lcp;
		    nsects += sgp->nsects;
		}
		lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	    }
	    sections = (struct section **)
			malloc(nsects * sizeof(struct section *));
	    if(sections == NULL){
		fprintf(stderr, "%s : Ran out of memory (%s)\n", progname,
			sys_errlist[errno]);
		exit(1);
	    }
	    k = 0;
	    lcp = load_commands;
	    for(i = 0; i < mhp->ncmds; i++){
		if(lcp->cmd == LC_SEGMENT){
		    sgp = (struct segment_command *)lcp;
		    sp = (struct section *)
			 ((char *)sgp + sizeof(struct segment_command));
		    for(j = 0; j < sgp->nsects; j++){
			sections[k++] = sp++;
		    }
		}
		lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	    }
	}
	printf("address  pcrel length extern scattered symbolnum/value\n");
	for(i = 0 ; i < nreloc ; i++){
	    p = reloc + i;
	    if((p->r_address & R_SCATTERED) != 0){
		sr = (struct scattered_relocation_info *)p; 
		if(vflag){
		    printf("%08x ", sr->r_address);
		    if(sr->r_pcrel)
			printf("True  ");
		    else
			printf("False ");
		    switch(sr->r_length){
		    case 0:
			printf("byte   ");
			break;
		    case 1:
			printf("word   ");
			break;
		    case 2:
			printf("long   ");
			break;
		    default:
			printf("?(%2d)  ", sr->r_length);
			break;
		    }
		    printf("n/a    True       0x%08x\n", sr->r_value);
		}
		else{
		    printf("%08x %1d     %-2d     n/a    1         0x%08x\n",
			   sr->r_address, sr->r_pcrel, sr->r_length,
			   sr->r_value);
		}
	    }
	    else{
		if(vflag){
		    printf("%08x ", p->r_address);
		    if(p->r_pcrel)
			printf("True  ");
		    else
			printf("False ");
		    switch(p->r_length){
		    case 0:
			printf("byte   ");
			break;
		    case 1:
			printf("word   ");
			break;
		    case 2:
			printf("long   ");
			break;
		    default:
			printf("?(%2d)  ", p->r_length);
			break;
		    }
		    if(p->r_extern){
			printf("True   False      ");
			if(syms == (struct nlist *)0 || str == NIL ||
			   p->r_symbolnum > nsyms ||
			   syms[p->r_symbolnum].n_un.n_strx > str_size)
			    printf("?(%d)\n", p->r_symbolnum);
			else{
			    printf("%s\n", str +
					   syms[p->r_symbolnum].n_un.n_strx);
			}
		    }
		    else{
			printf("False  False      ");
			if(mach){
			    printf("%d ", p->r_symbolnum);
			    if(p->r_symbolnum > nsects + 1)
				printf("(?,?)\n");
			    else{
				if(p->r_symbolnum == R_ABS)
				    printf("R_ABS\n");
				else
				    printf("(%0.16s,%0.16s)\n",
				       sections[p->r_symbolnum - 1]->segname,
				       sections[p->r_symbolnum - 1]->sectname);
			    }
			}
			else{
			    switch(p->r_symbolnum){
			    case N_TEXT:
				printf("N_TEXT\n");
				break;
			    case N_DATA:
				printf("N_DATA\n");
				break;
			    case N_BSS:
				printf("N_BSS\n");
				break;
			    case N_ABS:
				printf("N_ABS\n");
				break;
			    default:
				printf("?(%d)\n", p->r_symbolnum);
				break;
			    }
			}
		    }
		}
		else{
		    printf("%08x %1d     %-2d     %1d      0         %d\n",
			   p->r_address, p->r_pcrel, p->r_length, p->r_extern,
			   p->r_symbolnum);
		}
	    }
	}
}

#define	MODE(x)		(((x) >> 3) & 7)
#define	REG(x)		((x) & 7)
#define	DEST_MODE(x)	(((x) >> 6) & 7)
#define	DEST_REG(x)	(((x) >> 9) & 7)
#define B_SIZE	0
#define W_SIZE	1
#define L_SIZE	2
#define	S_SIZE	3
#define	D_SIZE	4
#define	X_SIZE	5
#define	P_SIZE	6

static char wl[] = "wl";
static char size[] = "bwl?";
static char *scales[] = { "", ":2", ":4", ":8" };
static char *aregs[] = { "a0", "a1", "a2", "a3", "a4", "a5", "a6", "sp" };
static char *dregs[] = { "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7" };
static char *fpregs[] = { "fp0", "fp1", "fp2", "fp3", "fp4", "fp5", "fp6",
			  "fp7" };
static char *fpcregs[] = { "fpc?", "fpiar", "fpsr", "fpsr/fpiar", "fpcr",
			   "fpcr/fpiar", "fpcr/fpsr", "fpcr/fpsr/fpiar" }; 
static char *branches[] = { "bra", "bsr", "bhi", "bls", "bcc", "bcs", "bne",
			    "beq", "bvc", "bvs", "bpl", "bmi", "bge", "blt",
			    "bgt", "ble" };
static char *conditions[] = { "t", "f", "hi", "ls", "cc", "cs", "ne",
			    "eq", "vc", "vs", "pl", "mi", "ge", "lt",
			    "gt", "le" };
static char *fpops[] = { "fmove", "fint", "fsinh", "fintrz", "fsqrt",
	"reserved", "flognp1", "reserved", "fetoxm1", "ftanh", "fatan",
	"reserved", "fasin", "fatanh", "fsin", "ftan", "fetox", "ftwotox",
	"ftentox", "reserved", "flogn", "flog10", "flog2", "reserved", "fabs",
	"fcosh", "fneg", "reserved", "facos", "fcos", "fgetexp", "fgetman",
	"fdiv", "fmod", "fadd", "fmul", "fsgldiv", "frem", "fscale", "fsglmul",
	"fsub", "reserved", "reserved", "reserved", "reserved", "reserved",
	"reserved", "reserved", "fsincos", "fsincos", "fsincos", "fsincos",
	"fsincos", "fsincos", "fsincos", "fsincos", "fcmp", "reserved", "ftst"};

static char fpformat[] = "lsxpwdbp";
static long fpsize[] = { L_SIZE, S_SIZE, X_SIZE, P_SIZE, W_SIZE, D_SIZE, B_SIZE,
			 P_SIZE };
static char *fpcond[] = { "f", "eq", "ogt", "oge", "olt", "ole", "ogl", "or",
	"un", "ueq", "ugt", "uge", "ult", "ule", "ne", "t", "sf", "seq", "gt",
	"ge", "lt", "le", "gl", "gle", "ngle", "ngl", "nle", "nlt", "nge",
	"ngt", "sne", "st" };

static char *instr_text;
static long instr_addr;

static
long
disassemble(text, addr)
char *text;
long addr;
{
    unsigned short opword, specop1, specop2;
    long length, i;
    char *reg1, *reg2;

	instr_text = text;
	instr_addr = addr;
	if(Xflag)
	    printf("\t");
	else
	    printf("%08x\t", addr);

	opword = *((short *)text);
	length = sizeof(short);
	switch((opword & 0xf000) >> 12){
	case 0x0:
	    if(opword == 0x003c){
		printf("orb\t#0x%x,cc\n",
		       (long)*((short *)(text + length)) & 0xff);
		length += sizeof(short);
		return(length);
	    }
	    if(opword == 0x007c){
		printf("orw\t#0x%x,sr\n",
		       (long)*((short *)(text + length)) & 0xffff);
		length += sizeof(short);
		return(length);
	    }
	    if((opword & 0xf900) == 0){
		if((opword & 0x00c0) == 0x00c0){
		    specop1 = *((short *)(text + length));
		    length += sizeof(short);
		    if((specop1 & 0x0800) == 0x0800)
			printf("chk2%c\t", size[((opword & 0x0600) >> 9)]);
		    else
			printf("cmp2%c\t", size[((opword & 0x0600) >> 9)]);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, (opword & 0x0600) >> 9);
		    if(((specop1 & 0x8000) >> 15) == 0)
			reg1 = dregs[(specop1 & 0x7000) >> 12];
		    else
			reg1 = aregs[(specop1 & 0x7000) >> 12];
		    printf(",%s\n", reg1);
		    return(length);
		}
	    }
	    if((opword & 0xff00) == 0){
		switch((opword & 0x00c0) >> 6){
		case 0:
		    printf("orb\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, B_SIZE);
		    printf("\n");
		    return(length);
		case 1:
		    printf("orw\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf("\n");
		    return(length);
		case 2:
		    printf("orl\t#0x%x,", *(long *)(text + length));
		    length += sizeof(long);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    printf("\n");
		    return(length);
		}
	    }
	    if((opword & 0xf100) == 0x0100){
		if((opword & 0xf038) == 0x0008){
		    switch((opword & 0x00c0) >> 6){
		    case 0:
			printf("movepw\ta%d@(0x%x:w),d%d\n", REG(opword), 
			       (long)*((short *)(text + length)) & 0xffff,
			       (opword & 0x0e00) >> 9);
			break;
		    case 1:
			printf("movepl\ta%d@(0x%x:w),d%d\n", REG(opword), 
			       (long)*((short *)(text + length)) & 0xffff,
			       (opword & 0x0e00) >> 9);
			break;
		    case 2:
			printf("movepw\td%d,a%d@(0x%x:w)\n",
			       (opword & 0x0e00) >> 9, REG(opword), 
			       (long)*((short *)(text + length)) & 0xffff);
			break;
		    case 3:
			printf("movepl\td%d,a%d@(0x%x:w)\n",
			       (opword & 0x0e00) >> 9, REG(opword), 
			       (long)*((short *)(text + length)) & 0xffff);
			break;
		    }
		    length += sizeof(short);
		    return(length);
		}
		else{
		    switch((opword & 0x00c0) >> 6){
		    case 0:
			printf("btst\td%d,", (opword & 0x0e00) >> 9);
			break;
		    case 1:
			printf("bchg\td%d,", (opword & 0x0e00) >> 9);
			break;
		    case 2:
			printf("bclr\td%d,", (opword & 0x0e00) >> 9);
			break;
		    case 3:
			printf("bset\td%d,", (opword & 0x0e00) >> 9);
			break;
		    }
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    printf("\n");
		    return(length);
		}
	    }
	    if((opword & 0xff00) == 0x0200){
		if(opword == 0x023c){
		    printf("andb\t#0x%x,cc\n",
			   (long)*((short *)(text + length)) & 0xff);
		    length += sizeof(short);
		    return(length);
		}
		if(opword == 0x027c){
		    printf("andw\t#0x%x,sr\n",
			   (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		    return(length);
		}
		switch((opword & 0x00c0) >> 6){
		case 0:
		    printf("andb\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, B_SIZE);
		    break;
		case 1:
		    printf("andw\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    break;
		case 2:
		    printf("andl\t#0x%x,", *(long *)(text + length));
		    length += sizeof(long);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    break;
		default:
		    goto bad;
		}
		printf("\n");
		return(length);
	    }
	    if((opword & 0xff00) == 0x0400){
		switch((opword & 0x00c0) >> 6){
		case 0:
		    printf("subb\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, B_SIZE);
		    break;
		case 1:
		    printf("subw\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    break;
		case 2:
		    printf("subl\t#0x%x,", *(long *)(text + length));
		    length += sizeof(long);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    break;
		default:
		    goto bad;
		}
		printf("\n");
		return(length);
	    }
	    if((opword & 0xff00) == 0x0600){
		switch((opword & 0x00c0) >> 6){
		case 0:
		    printf("addb\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, B_SIZE);
		    break;
		case 1:
		    printf("addw\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    break;
		case 2:
		    printf("addl\t#0x%x,", *(long *)(text + length));
		    length += sizeof(long);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    break;
		default:
		    goto bad;
		}
		printf("\n");
		return(length);
	    }
	    if(opword == 0x0a3c){
		printf("eorb\t#0x%x,cc\n",
		       (long)*((short *)(text + length)) & 0xff);
		length += sizeof(short);
		return(length);
	    }
	    if(opword == 0x0a7c){
		printf("eorw\t#0x%x,sr\n",
		       (long)*((short *)(text + length)) & 0xffff);
		length += sizeof(short);
		return(length);
	    }
	    if((opword & 0xff00) == 0x0800){
		switch((opword & 0x00c0) >> 6){
		case 0:
		    printf("btst\t");
		    break;
		case 1:
		    printf("bchg\t");
		    break;
		case 2:
		    printf("bclr\t");
		    break;
		case 3:
		    printf("bset\t");
		    break;
		}
		printf("#%d,", (long)*((short *)(text + length)) & 0xff);
		length += sizeof(short);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xf9ff) == 0x08fc){
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		if((specop1 & 0x8000) == 0)
		    reg1 = dregs[(specop1 & 0x7000) >> 12];
		else
		    reg1 = aregs[(specop1 & 0x7000) >> 12];

		specop2 = *((short *)(text + length));
		length += sizeof(short);
		if((specop2 & 0x8000) == 0)
		    reg2 = dregs[(specop2 & 0x7000) >> 12];
		else
		    reg2 = aregs[(specop2 & 0x7000) >> 12];
		printf("cas2%c\td%d:d%d,d%d:d%d,(%s):(%s)\n",
			size[((opword & 0x0600) >> 9) - 1],
			specop1 & 7, specop2 & 7,
			(specop1 & 0x01c0) >> 6, (specop2 & 0x01c0) >> 6,
			reg1, reg2);
		return(length);
	    }
	    if((opword & 0xf9c0) == 0x08c0){
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		printf("cas%c\td%d,d%d,", size[((opword & 0x0600) >> 9) - 1],
			specop1 & 7, (specop1 & 0x01c0) >> 6);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, (opword & 0x0600) >> 9);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xff00) == 0x0a00){
		switch((opword & 0x00c0) >> 6){
		case 0:
		    printf("eorb\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, B_SIZE);
		    break;
		case 1:
		    printf("eorw\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    break;
		case 2:
		    printf("eorl\t#0x%x,", *(long *)(text + length));
		    length += sizeof(long);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    break;
		default:
		    goto bad;
		}
		printf("\n");
		return(length);
	    }
	    if((opword & 0xff00) == 0x0c00){
		switch((opword & 0x00c0) >> 6){
		case 0:
		    printf("cmpb\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, B_SIZE);
		    break;
		case 1:
		    printf("cmpw\t#0x%x,",
			   (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    break;
		case 2:
		    printf("cmpl\t#");
		    if(print_symbol(*(long *)(text + length), addr + length) ==
		       TRUE)
			printf(",");
		    else
			printf("x%x,", *(long *)(text + length));
		    length += sizeof(long);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    break;
		default:
		    goto bad;
		}
		printf("\n");
		return(length);
	    }
	    if((opword & 0xff00) == 0x0e00){
		printf("moves%c\t", size[(opword & 0x00c0) >> 6] );
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		if((specop1 & 0x8000) == 0)
		    reg1 = dregs[(specop1 & 0x7000) >> 12];
		else
		    reg1 = aregs[(specop1 & 0x7000) >> 12];
		if((specop1 & 0x0800) == 0){
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, (opword & 0x00c0) >> 6);
		    printf(",%s\n", reg1);
		}
		else{
		    printf("%s,", reg1);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, (opword & 0x00c0) >> 6);
		    printf("\n");
		}
		return(length);
	    }
	    break;
	case 0x1:
	    printf("moveb\t");
	    length += print_ef(MODE(opword), REG(opword),
			       text + length, B_SIZE);
	    printf(",");
	    length += print_ef(DEST_MODE(opword), DEST_REG(opword),
			       text + length, B_SIZE);
	    printf("\n");
	    return(length);
	case 0x2:
	    if((opword & 0x01c0) == 0x0040){
		printf("movel\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf(",%s\n", aregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	    printf("movel\t");
	    length += print_ef(MODE(opword), REG(opword),
			       text + length, L_SIZE);
	    printf(",");
	    length += print_ef(DEST_MODE(opword), DEST_REG(opword),
			       text + length, L_SIZE);
	    printf("\n");
	    return(length);
	case 0x3:
	    if((opword & 0x01c0) == 0x0040){
		printf("movew\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf(",%s\n", aregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	    printf("movew\t");
	    length += print_ef(MODE(opword), REG(opword),
			       text + length, W_SIZE);
	    printf(",");
	    length += print_ef(DEST_MODE(opword), DEST_REG(opword),
			       text + length, W_SIZE);
	    printf("\n");
	    return(length);
	case 0x4:
	    if((opword & 0xff00) == 0x4000){
		if((opword & 0xffc0) == 0x40c0){
		    printf("movew\tsr,");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf("\n");
		    return(length);
		}
		printf("negx%c\t", size[(opword >> 6) & 0x3]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xf140) == 0x4100){
		if(opword & 0x0080)
		    printf("chkw\t");
		else
		    printf("chkl\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf(",%s\n", dregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	    if((opword & 0xfe38) == 0x4800){
		switch((opword >> 6) & 0x7){
		case 0x2:
		    printf("extw\t%s\n", dregs[REG(opword)]);
		    return(length);
		case 0x3:
		    printf("extl\t%s\n", dregs[REG(opword)]);
		    return(length);
		case 0x7:
		    printf("extbl\t%s\n", dregs[REG(opword)]);
		    return(length);
		}
	    }
	    if((opword & 0xf1c0) == 0x41c0){
		printf("lea\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf(",a%d\n", (opword & 0x0e00)>>9);
		return(length);
	    }
	    if((opword & 0xff00) == 0x4200){
		if((opword & 0xffc0) == 0x42c0){
		    printf("movew\tcc,");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf("\n");
		    return(length);
		}
		printf("clr%c\t", size[(opword >> 6) & 0x3]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xff00) == 0x4400){
		if((opword & 0xffc0) == 0x44c0){
		    printf("movew\t");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf(",cc\n");
		    return(length);
		}
		printf("neg%c\t", size[(opword >> 6) & 0x3]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xff00) == 0x4600){
		if((opword & 0xffc0) == 0x46c0){
		    printf("movew\t");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf(",sr\n");
		    return(length);
		}
		printf("not%c\t", size[(opword >> 6) & 0x3]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xfff8) == 0x4808){
		printf("linkl\t%s,#0x%x\n", aregs[REG(opword)],
		       *(long *)(text + length));
		length += sizeof(long);
		return(length);
	    }
	    if((opword & 0xffc0) == 0x4800){
		printf("nbcd\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xfff8) == 0x4840){
		printf("swap\t%s\n", dregs[REG(opword)]);
		return(length);
	    }
	    if((opword & 0xfff8) == 0x4848){
		printf("bkpt\t#%d\n", opword & 0x7);
		return(length);
	    }
	    if((opword & 0xffc0) == 0x4840){
		printf("pea\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xfb80) == 0x4880){
		printf("movem%c\t", size[((opword & 0x0040) >> 6) + 1]);
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		if((opword & 0x0400) == 0x0000){
		    if(specop1 == 0){
			printf("#0x0");
		    }
		    else{
			if(MODE(opword) == 4){
			    for(i = 0; i < 8; i++){
				if((specop1 & 0x8000) != 0){
				    printf("%s", dregs[i]);
				    if(((specop1 << 1) & 0xffff) != 0)
					printf("/");
				}
				specop1 <<= 1;
			    }
			    for(i = 0; i < 8; i++){
				if((specop1 & 0x8000) != 0){
				    printf("%s", aregs[i]);
				    if(((specop1 << 1) & 0xffff) != 0)
					printf("/");
				}
				specop1 <<= 1;
			    }
		        }
		        else{
			    for(i = 0; i < 8; i++){
				if((specop1 & 1) != 0){
				    printf("%s", dregs[i]);
				    if((specop1 >> 1) != 0)
					printf("/");
				}
				specop1 >>= 1;
			    }
			    for(i = 0; i < 8; i++){
				if((specop1 & 1) != 0){
				    printf("%s", aregs[i]);
				    if((specop1 >> 1) != 0)
					printf("/");
				}
				specop1 >>= 1;
			    }
		        }
		    }
		    printf(",");
		    length += print_ef(MODE(opword), REG(opword), text + length,
				       ((opword & 0x0040) >> 6) + 1);
		}
		else{
		    length += print_ef(MODE(opword), REG(opword), text + length,
				       ((opword & 0x0040) >> 6) + 1);
		    printf(",");
		    if(specop1 == 0){
			printf("#0x0");
		    }
		    else{
			if(MODE(opword) == 4){
			    for(i = 0; i < 8; i++){
				if((specop1 & 0x8000) != 0){
				    printf("%s", dregs[i]);
				    if(((specop1 << 1) & 0xffff) != 0)
					printf("/");
				}
				specop1 <<= 1;
			    }
			    for(i = 0; i < 8; i++){
				if((specop1 & 0x8000) != 0){
				    printf("%s", aregs[i]);
				    if(((specop1 << 1) & 0xffff) != 0)
					printf("/");
				}
				specop1 <<= 1;
			    }
		        }
		        else{
			    for(i = 0; i < 8; i++){
				if((specop1 & 1) != 0){
				    printf("%s", dregs[i]);
				    if((specop1 >> 1) != 0)
					printf("/");
				}
				specop1 >>= 1;
			    }
			    for(i = 0; i < 8; i++){
				if((specop1 & 1) != 0){
				    printf("%s", aregs[i]);
				    if((specop1 >> 1) != 0)
					printf("/");
				}
				specop1 >>= 1;
			    }
		        }
		    }
		}
		printf("\n");
		return(length);
	    }
	    if(opword == 0x4afc){
		printf("illegal\n");
		return(length);
	    }
	    if((opword & 0xff00) == 0x4a00){
		if((opword & 0xffc0) == 0x4ac0){
		    printf("tas\t");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf("\n");
		    return(length);
		}
		printf("tst%c\t", size[((opword >> 6) & 0x3)]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xffc0) == 0x4c00){
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		if(specop1 & 0x0800)
		    printf("mulsl\t");
		else
		    printf("mulul\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		if(specop1 & 0x0400)
		    printf(",%s:%s\n", dregs[specop1 & 0x7],
			   dregs[(specop1 >> 12) & 0x7]);
		else
		    printf(",%s\n", dregs[specop1 & 0x7]);
		return(length);
	    }
	    if((opword & 0xffc0) == 0x4c40){
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		if(specop1 & 0x0800)
		    printf("divs");
		else
		    printf("divu");
		if((specop1 & 0x0400) == 0 &&
		   (specop1 & 0x7) == ((specop1 >> 12) & 0x7) ){
		    printf("l\t");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf(",%s\n", dregs[specop1 & 0x7]);
		}
		else if(specop1 & 0x0400){
		    printf("l\t");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf(",%s:%s\n", dregs[specop1 & 0x7],
			   dregs[(specop1 >> 12) & 0x7]);
		}
		else{
		    printf("ll\t");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, W_SIZE);
		    printf(",%s:%s\n", dregs[specop1 & 0x7],
			   dregs[(specop1 >> 12) & 0x7]);
		}
		return(length);
	    }
	    if((opword & 0xfff0) == 0x4e40){
		printf("trap\t#%d\n", opword &0xf);
		return(length);
	    }
	    if((opword & 0xfff8) == 0x4e50){
		printf("linkw\t%s,#0x%x\n", aregs[REG(opword)],
		       (long)*((short *)(text + length)) & 0xffff);
		length += sizeof(short);
		return(length);
	    }
	    if((opword & 0xfff8) == 0x4e58){
		printf("unlk\t%s\n", aregs[REG(opword)]);
		return(length);
	    }
	    if((opword & 0xfff8) == 0x4e60){
		printf("movel\t%s,usp\n", aregs[REG(opword)]);
		return(length);
	    }
	    if((opword & 0xfff8) == 0x4e68){
		printf("movel\tusp,%s\n", aregs[REG(opword)]);
		return(length);
	    }
	    if(opword == 0x4e70){
		printf("reset\n");
		return(length);
	    }
	    if(opword == 0x4e71){
		printf("nop\n");
		return(length);
	    }
	    if(opword == 0x4e72){
		printf("stop\t#0x%x\n",
		       (long)*((short *)(text + length)) & 0xffff);
		length += sizeof(short);
		return(length);
	    }
	    if(opword == 0x4e73){
		printf("rte\n");
		return(length);
	    }
	    if(opword == 0x4e74){
		printf("rtd\t#0x%x\n",
		       (long)*((short *)(text + length)) & 0xffff);
		length += sizeof(short);
		return(length);
	    }
	    if(opword == 0x4e75){
		printf("rts\n");
		return(length);
	    }
	    if(opword == 0x4e76){
		printf("trapv\n");
		return(length);
	    }
	    if(opword == 0x4e77){
		printf("rtr\n");
		return(length);
	    }
	    if((opword & 0xfffe) == 0x4e7a){
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		printf("movec\t");
		if(opword & 0x1){
		    if(specop1 & 0x8000)
			printf("%s,", aregs[(specop1 >> 12) & 0x7]);
		    else
			printf("%s,", dregs[(specop1 >> 12) & 0x7]);
		    switch(specop1 & 0x0fff){
		    case 0x000:
			printf("sfc\n");
			break;
		    case 0x001:
			printf("dfc\n");
			break;
		    case 0x002:
			printf("cacr\n");
			break;
		    case 0x800:
			printf("usp\n");
			break;
		    case 0x801:
			printf("vbr\n");
			break;
		    case 0x802:
			printf("caar\n");
			break;
		    case 0x803:
			printf("msp\n");
			break;
		    case 0x804:
			printf("isp\n");
			break;
		    default:
			printf("???\n");
			break;
		    }
		}
		else{
		    switch(specop1 & 0x0fff){
		    case 0x000:
			printf("sfc,");
			break;
		    case 0x001:
			printf("dfc,");
			break;
		    case 0x002:
			printf("cacr,");
			break;
		    case 0x800:
			printf("usp,");
			break;
		    case 0x801:
			printf("vbr,");
			break;
		    case 0x802:
			printf("caar,");
			break;
		    case 0x803:
			printf("msp,");
			break;
		    case 0x804:
			printf("isp,");
			break;
		    default:
			printf("???,");
			break;
		    }
		    if(specop1 & 0x8000)
			printf("%s\n", aregs[(specop1 >> 12) & 0x7]);
		    else
			printf("%s\n", dregs[(specop1 >> 12) & 0x7]);
		}
		return(length);
	    }
	    if((opword & 0xffc0) == 0x4e80){
		printf("jsr\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf("\n");
		return(length);
	    }
	    if((opword & 0xffc0) == 0x4ec0){
		printf("jmp\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    break;
	case 0x5:
	    if((opword & 0xf0c0) == 0x50c0){
		if((opword & 0x00f8) == 0x00f8){
		    switch(opword & 0x0007){
		    case 2:
			printf("trap%sw\t#0x%x\n",
			       conditions[(opword & 0x0f00) >> 8],
			       (long)*((short *)(text + length)) & 0xffff);
			length += sizeof(short);
			return(length);
		    case 3:
			printf("trap%sl\t#0x%x\n",
			       conditions[(opword & 0x0f00) >> 8],
			       *((long *)(text + length)));
			length += sizeof(long);
			return(length);
		    case 4:
			printf("trap%s\n", conditions[(opword & 0x0f00) >> 8]);
			return(length);
		    }
		}
		if((opword & 0x00f8) == 0x00c8){
		    printf("db%s\t%s,", conditions[(opword & 0x0f00) >> 8],
			   dregs[REG(opword)]);
		    if(print_symbol(addr + length + *((short *)(text + length)),
				    addr + length))
			printf("\n");
		    else
			printf("0x%x\n",
			       addr + length + *((short *)(text + length)));
		    length += sizeof(short);
		    return(length);
		}
		printf("s%s\t", conditions[(opword & 0x0f00) >> 8]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf("\n");
		return(length);
	    }
	    if(opword & 0x0100)
		printf("subq%c\t#0x%x,", size[(opword >> 6) & 0x3],
		       ((opword >> 9) & 0x7) == 0 ? 8 : (opword >> 9) & 0x7);
	    else
		printf("addq%c\t#0x%x,", size[(opword >> 6) & 0x3],
		       ((opword >> 9) & 0x7) == 0 ? 8 : (opword >> 9) & 0x7);
	    length += print_ef(MODE(opword), REG(opword),
			       text + length, W_SIZE);
	    printf("\n");
	    return(length);
	case 0x6:
	    printf("%s", branches[(opword & 0x0f00) >> 8]);
	    if((opword & 0x00ff) == 0x00ff){
		printf("l\t");
		if(print_symbol(addr + length + *((long *)(text + length)),
				addr + length))
		    printf("\n");
		else
		    printf("0x%x\n",addr + length + *((long *)(text + length)));
		length += sizeof(long);
	    }
	    else if((opword & 0x00ff) == 0){
		printf("w\t");
		if(print_symbol(addr + length + *((short *)(text + length)),
				addr + length))
		    printf("\n");
		else
		    printf("0x%x\n",addr+length + *((short *)(text + length)));
		length += sizeof(short);
	    }
	    else{
		printf("s\t");
		if(print_symbol(addr + length + (long)((char)(opword)),
				addr + length))
		    printf("\n");
		else
		    printf("0x%x\n", addr + length + (long)((char)(opword)));
	    }
	    return(length);
	case 0x7:
	    printf("moveq\t#%d,%s\n", (long)((char)(opword)),
		   dregs[(opword >> 9) & 0x7]);
	    return(length);
	case 0x8:
	    if((opword & 0xf1f0) == 0x8100){
		if(opword & 0x0008)
		    printf("sbcd\t%s@-,%s@-\n", aregs[opword & 0x7],
			   aregs[(opword >> 9) & 0x7]);
		else
		    printf("sbcd\t%s,%s\n", dregs[opword & 0x7],
			   dregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	    if((opword & 0xf1f0) == 0x8140){
		if(opword & 0x0008){
		    printf("pack\t%s@-,%s@-,#0x%x\n", aregs[opword & 0x7],
			   aregs[(opword >> 9) & 0x7],
		           (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		}
		else{
		    printf("pack\t%s,%s,#0x%x\n", dregs[opword & 0x7],
			   dregs[(opword >> 9) & 0x7],
		           (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		}
		return(length);
	    }
	    if((opword & 0xf1f0) == 0x8180){
		if(opword & 0x0008){
		    printf("unpk\t%s@-,%s@-,#0x%x\n", aregs[opword & 0x7],
			   aregs[(opword >> 9) & 0x7],
		           (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		}
		else{
		    printf("unpk\t%s,%s,#0x%x\n", dregs[opword & 0x7],
			   dregs[(opword >> 9) & 0x7],
		           (long)*((short *)(text + length)) & 0xffff);
		    length += sizeof(short);
		}
		return(length);
	    }
	    if((opword & 0xf0c0) == 0x80c0){
		if(opword & 0x0100)
		    printf("divs\t");
		else
		    printf("divu\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf(",%s\n", dregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	    if(opword & 0x0100){
		printf("or%c\t%s,", size[(opword >> 6) & 0x3],
		       dregs[(opword >> 9) & 0x7]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf("\n");
		return(length);
	    }
	    else{
		printf("or%c\t", size[(opword >> 6) & 0x3]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf(",%s\n", dregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	case 0xa:
	    break;
	case 0xb:
	    if((opword & 0xf138) == 0xb108 && ((opword >> 6) & 0x7) != 0x7){
		printf("cmpm%c\t%s@+,%s@+\n", size[(opword >> 6) & 0x3],
		       aregs[opword & 0x7], aregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	    switch((opword >> 6) & 0x7){
	    case 0:
	    case 1:
	    case 2:
		printf("cmp%c\t", size[(opword >> 6) & 0x3]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf(",%s\n", dregs[(opword >> 9) & 0x7]);
		return(length);

	    case 3:
		printf("cmpw\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf(",%s\n", aregs[(opword >> 9) & 0x7]);
		return(length);
	    case 7:
		printf("cmpl\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf(",%s\n", aregs[(opword >> 9) & 0x7]);
		return(length);

	    case 4:
	    case 5:
	    case 6:
		printf("eor%c\t%s,", size[(opword >> 6) & 0x3],
		       dregs[(opword >> 9) & 0x7]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, L_SIZE);
		printf("\n");
		return(length);
	    }
	    break;
	case 0xc:
	    if((opword & 0xf1f8) == 0xc140){
		printf("exg\t%s,%s\n", dregs[(opword >> 9) & 0x7],
		       dregs[opword & 0x7]);
		return(length);
	    }
	    if((opword & 0xf1f8) == 0xc148){
		printf("exg\t%s,%s\n", aregs[(opword >> 9) & 0x7],
		       aregs[opword & 0x7]);
		return(length);
	    }
	    if((opword & 0xf1f8) == 0xc188){
		printf("exg\t%s,%s\n", dregs[(opword >> 9) & 0x7],
		       aregs[opword & 0x7]);
		return(length);
	    }
	    if((opword & 0xf1f0) == 0xc100){
		if(opword & 0x0008){
		    printf("abcd\t%s@-,%s@-\n", aregs[opword & 0x7],
			   aregs[(opword >> 9) & 0x7]);
		}
		else{
		    printf("abcd\t%s,%s\n", dregs[opword & 0x7],
			   dregs[(opword >> 9) & 0x7]);
		}
		return(length);
	    }
	    if((opword & 0xf0c0) == 0xc0c0){
		if(opword & 0x0100)
		    printf("muls\t");
		else
		    printf("mulu\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, W_SIZE);
		printf(",%s\n", dregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	    switch((opword >> 6) & 0x7){
	    case 0:
	    case 1:
	    case 2:
		printf("and%c\t", size[(opword >> 6) & 0x3]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, (opword >> 6) & 0x3);
		printf(",%s\n", dregs[(opword >> 9) & 0x7]);
		return(length);
	    case 4:
	    case 5:
	    case 6:
		printf("and%c\t%s,", size[(opword >> 6) & 0x3],
		       dregs[(opword >> 9) & 0x7]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, (opword >> 6) & 0x3);
		printf("\n");
		return(length);
	    }
	    break;
	case 0x9:
	case 0xd:
	    if(opword & 0x4000)
		printf("add");
	    else
		printf("sub");
	    switch((opword >> 6) & 0x7){
	    case 0:
	    case 1:
	    case 2:
		printf("%c\t", size[(opword >> 6) & 0x3]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, (opword >> 6) & 0x3);
		printf(",%s\n", dregs[(opword >> 9) & 0x7]);
		return(length);
	    case 4:
	    case 5:
	    case 6:
		if((opword & 0x0030) == 0x0000){
		    if((opword & 0x0008) == 0){
			printf("x%c\t%s,%s\n", size[(opword >> 6) & 0x3],
			       dregs[opword & 0x7], dregs[(opword >> 9) & 0x7]);
		    }
		    else{
			printf("x%c\t%s@-,%s@-\n", size[(opword >> 6) & 0x3],
			       aregs[opword & 0x7], aregs[(opword >> 9) & 0x7]);
		    }
		    return(length);
		}
		printf("%c\t%s,", size[(opword >> 6) & 0x3],
		       dregs[(opword >> 9) & 0x7]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, (opword >> 6) & 0x3);
		printf("\n");
		return(length);
	    case 3:
	    case 7:
		printf("%c\t", size[((opword >> 8) & 0x1) + 1]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, ((opword >> 8) & 0x1) + 1);
		printf(",%s\n", aregs[(opword >> 9) & 0x7]);
		return(length);
	    }
	case 0xe:
	    if((opword & 0xf8c0) == 0xe8c0){
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		switch((opword >> 8) & 0x7){
		case 0:
		    printf("bftst\t");
		    break;
		case 1:
		    printf("bfextu\t");
		    break;
		case 2:
		    printf("bfchg\t");
		    break;
		case 3:
		    printf("bfexts\t");
		    break;
		case 4:
		    printf("bfclr\t");
		    break;
		case 5:
		    printf("bfffo\t");
		    break;
		case 6:
		    printf("bfset\t");
		    break;
		case 7:
		    printf("bfins\t%s,", dregs[(specop1 >> 12) & 0x7]);
		    break;
		}
		length += print_ef(MODE(opword), REG(opword),
				   text + length, ((opword & 0x0100) >> 8) + 1);
		if(specop1 & 0x0800)
		    printf("{%s,", dregs[(specop1 >> 6) & 0x7]);
		else
		    printf("{#%d,", (specop1 >> 6) & 0x1f);
		if(specop1 & 0x0020)
		    printf("%s}", dregs[specop1 & 0x7]);
		else
		    printf("#%d}", specop1 & 0x1f);
		if((opword & 0x0100) && (opword & 0x0700) != 0x0700)
		    printf(",%s\n", dregs[(specop1 >> 12) & 0x7]);
		else
		    printf("\n");
		return(length);
	    }
	    if((opword & 0xf8c0) == 0xe0c0){
		switch((opword >> 9) & 0x3){
		case 0:
		    printf("as");
		    break;
		case 1:
		    printf("ls");
		    break;
		case 2:
		    printf("rox");
		    break;
		case 3:
		    printf("ro");
		    break;
		}
		if(opword & 0x0100)
		    printf("lw\t");
		else
		    printf("rw\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, ((opword & 0x0100) >> 8) + 1);
		printf("\n");
		return(length);
	    }
	    switch((opword >> 3) & 0x3){
	    case 0:
		printf("as");
		break;
	    case 1:
		printf("ls");
		break;
	    case 2:
		printf("rox");
		break;
	    case 3:
		printf("ro");
		break;
	    }
	    if(opword & 0x0100)
		printf("l%c\t", size[(opword >> 6) & 0x3]);
	    else
		printf("r%c\t", size[(opword >> 6) & 0x3]);
	    if(opword & 0x0020)
		printf("%s,%s\n", dregs[(opword >> 9) & 0x7],
		       dregs[opword & 0x7]);
	    else
		printf("#%d,%s\n",
		       ((opword >> 9) & 0x7) == 0 ? 8 : (opword >> 9) & 0x7,
		       dregs[opword & 0x7]);
	    return(length);
	case 0xf:
	    if((opword & 0x0e00) == 0x0000){
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		switch((specop1 >> 13) & 0x7){
		case 0:
		    if(specop1 & 0x0200){
			if(((specop1 >> 10) & 0x7) == 0x2){
			    if(specop1 & 0x0100){
				printf("pmovefd\ttt0,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			    else{
				printf("pmove\ttt0,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			}
			else if(((specop1 >> 10) & 0x7) == 0x3){
			    if(specop1 & 0x0100){
				printf("pmovefd\ttt1,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			    else{
				printf("pmove\ttt1,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			} else
			    goto bad;
		     }
		     else{
			if(((specop1 >> 10) & 0x7) == 0x2){
			    if(specop1 & 0x0100){
				printf("pmovefd\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",tt0\n");
				return(length);
			    }
			    else{
				printf("pmove\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",tt0\n");
				return(length);
			    }
			}
			else if(((specop1 >> 10) & 0x7) == 0x3){
			    if(specop1 & 0x0100){
				printf("pmovefd\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",tt1\n");
				return(length);
			    }
			    else{
				printf("pmove\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",tt1\n");
				return(length);
			    }
			} else
			    goto bad;
		     }
		case 1:
		    if((specop1 & 0xfde0) == 0x2000){
			if(specop1 & 0x0200)
			    printf("ploadr\t");
			else
			    printf("ploadw\t");
			if((specop1 & 0x18) == 0x10)
			    printf("#0x%x,", specop1 & 0x7);
			else if((specop1 & 0x18) == 0x08)
			    printf("%s,", dregs[specop1 & 0x7]);
			else if((specop1 & 0x1f) == 0x00)
			    printf("sfc,");
			else if((specop1 & 0x1f) == 0x01)
			    printf("dfc,");
			else
			    goto bad;
			length += print_ef(MODE(opword), REG(opword),
					   text + length, L_SIZE);
			printf("\n");
			return(length);
		    }
		    else if((specop1 & 0xe300) == 0x2000){
			if(((specop1 >> 10) & 0x7) == 0x1){
			    printf("pflusha\n");
			    return(length);
			}
			else if(((specop1 >> 10) & 0x7) == 0x4){
			    if((specop1 & 0x18) == 0x10)
				printf("pflush\t#0x%x,#0x%x\n", specop1 & 0x7, 
				       (specop1 >> 5) & 0x7);
			    else if((specop1 & 0x18) == 0x08)
				printf("pflush\t%s,#0x%x\n",
				       dregs[specop1 & 0x7], 
				       (specop1 >> 5) & 0x7);
			    else if((specop1 & 0x1f) == 0x00)
				printf("pflush\tsfc,#0x%x\n",
				       (specop1 >> 5) & 0x7);
			    else if((specop1 & 0x1f) == 0x01)
				printf("pflush\tdfc,#0x%x\n",
				       (specop1 >> 5) & 0x7);
			    else
				goto bad;
			    return(length);
			}
			else if(((specop1 >> 10) & 0x7) == 0x6){
			    if((specop1 & 0x18) == 0x10)
				printf("pflush\t#0x%x,#0x%x,", specop1 & 0x7, 
				       (specop1 >> 5) & 0x7);
			    else if((specop1 & 0x18) == 0x08)
				printf("pflush\t%s,#0x%x,",
				       dregs[specop1 & 0x7],
				       (specop1 >> 5) & 0x7);
			    else if((specop1 & 0x1f) == 0x00)
				printf("pflush\tsfc,#0x%x,",
				       (specop1 >> 5) & 0x7);
			    else if((specop1 & 0x1f) == 0x01)
				printf("pflush\tdfc,#0x%x,",
				       (specop1 >> 5) & 0x7);
			    else
				goto bad;
			    length += print_ef(MODE(opword), REG(opword),
					       text + length, L_SIZE);
			    printf("\n");
			    return(length);
			}
			else
			    goto bad;
		    }
		    else
			goto bad;
		case 2:
		    if(specop1 & 0x0200){
			if(((specop1 >> 10) & 0x7) == 0x0){
			    if(specop1 & 0x0100){
				printf("pmovefd\ttc,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			    else{
				printf("pmove\ttc,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			}
			else if(((specop1 >> 10) & 0x7) == 0x2){
			    if(specop1 & 0x0100){
				printf("pmovefd\tsrp,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			    else{
				printf("pmove\tsrp,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			}
			else if(((specop1 >> 10) & 0x7) == 0x3){
			    if(specop1 & 0x0100){
				printf("pmovefd\tcrp,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			    else{
				printf("pmove\tcrp,");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf("\n");
				return(length);
			    }
			}
			else
			    goto bad;
		     }
		     else{
			if(((specop1 >> 10) & 0x7) == 0x0){
			    if(specop1 & 0x0100){
				printf("pmovefd\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",tc\n");
				return(length);
			    }
			    else{
				printf("pmove\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",tc\n");
				return(length);
			    }
			}
			else if(((specop1 >> 10) & 0x7) == 0x2){
			    if(specop1 & 0x0100){
				printf("pmovefd\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",srp\n");
				return(length);
			    }
			    else{
				printf("pmove\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",srp\n");
				return(length);
			    }
			}
			else if(((specop1 >> 10) & 0x7) == 0x3){
			    if(specop1 & 0x0100){
				printf("pmovefd\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",crp\n");
				return(length);
			    }
			    else{
				printf("pmove\t");
				length += print_ef(MODE(opword), REG(opword),
						   text + length, L_SIZE);
				printf(",crp\n");
				return(length);
			    }
			}
			else
			    goto bad;
		     }
		case 3:
		    if(specop1 & 0x0200){
			printf("pmove\tpsr,");
			length += print_ef(MODE(opword), REG(opword),
					   text + length, L_SIZE);
			printf("\n");
			return(length);
		    }
		    else{
			printf("pmove\t");
			length += print_ef(MODE(opword), REG(opword),
					   text + length, L_SIZE);
			printf(",psr\n");
			return(length);
		    }
		case 4:
		    if(specop1 & 0x0200)
			printf("ptestr\t");
		    else
			printf("ptestw\t");
		    if((specop1 & 0x18) == 0x10)
			printf("#0x%x,", specop1 & 0x7);
		    else if((specop1 & 0x18) == 0x08)
			printf("%s,", dregs[specop1 & 0x7]);
		    else if((specop1 & 0x1f) == 0x00)
			printf("sfc,");
		    else if((specop1 & 0x1f) == 0x01)
			printf("dfc,");
		    else
			goto bad;
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    if(specop1 & 0x0100)
			printf(",#0x%x,%s\n", (specop1 >> 10) & 0x7,
			       aregs[(specop1 >> 5) & 0x7]);
		    else
			printf(",#0x%x\n", (specop1 >> 10) & 0x7,
			       aregs[(specop1 >> 5) & 0x7]);
		    return(length);
		default:
		    goto bad;
		}
	    }
	    if((opword & 0x0e00) != 0x0200)
		goto bad;

	    switch((opword >> 6) & 0x7){
	    case 0:
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		switch((specop1 >> 13) & 0x7){
		case 0:
		    if((specop1 & 0x07f) > 0x3a)
			goto bad;
		    if((specop1 & 0x0078) == 0x0030)
			printf("fsincosx\t%s,%s:%s\n",
			       fpregs[(specop1 >> 10) & 0x7],
			       fpregs[specop1 & 0x7],
			       fpregs[(specop1 >> 7) & 0x7]);
		    else{
			if((((specop1 >> 10) & 0x7) == ((specop1 >> 7) & 0x7))||
			   (specop1 & 0x007f) == 0x003a)
			    printf("%sx\t%s\n", fpops[specop1 & 0x7f],
				   fpregs[(specop1 >> 10) & 0x7]);
			else
			    printf("%sx\t%s,%s\n", fpops[specop1 & 0x7f],
				   fpregs[(specop1 >> 10) & 0x7],
				   fpregs[(specop1 >> 7) & 0x7]);
		    }
		    return(length);
		case 1:
		    goto bad;
		case 2:
		    if((specop1 & 0x1c00) == 0x1c00){
			printf("fmovecrx\t#0x%x,%s\n", specop1 & 0x7f,
			       fpregs[(specop1 >> 7) & 0x7]);
			return(length);
		    }
		    printf("%s%c\t", fpops[specop1 & 0x7f],
			   fpformat[(specop1 >> 10) & 0x7]);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length,
				       fpsize[(specop1 >> 10) & 0x7]);
		    printf(",%s\n", fpregs[(specop1 >> 7) & 0x7]);
		    return(length);
		case 3:
		    printf("fmove%c\t%s,", fpformat[(specop1 >> 10) & 0x7],
		    	   fpregs[(specop1 >> 7) & 0x7]);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length,
				       fpsize[(specop1 >> 10) & 0x7]);
		    if(((specop1 >> 10) & 0x7) == 0x3){
			printf("{#%d}\n", specop1 & 0x0040 ?
			       (specop1 & 0x7f) | 0xffffff80 :
				specop1 & 0x7f);
		    } else if(((specop1 >> 10) & 0x7) == 0x7){
			printf("{%s}\n", dregs[(specop1 >> 4) & 0x7]);
		    } else
			printf("\n");
		    return(length);
		case 4:
		    printf("fmoveml\t");
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    printf(",%s\n", fpcregs[(specop1 >> 10) & 0x7]);
		    return(length);
		case 5:
		    printf("fmoveml\t%s,", fpcregs[(specop1 >> 10) & 0x7]);
		    length += print_ef(MODE(opword), REG(opword),
				       text + length, L_SIZE);
		    printf("\n");
		    return(length);
		case 6:
		case 7:
		    printf("fmovemx\t");
		    if((specop1 & 0x2000) == 0){
			length += print_ef(MODE(opword), REG(opword),
					   text + length, X_SIZE);
			printf(",");
			if((specop1 & 0x0800) == 0x0800){
			    printf("%s\n", dregs[((specop1 & 0x0070) >> 4)] );
			}
			else{
			    if((specop1 & 0x00ff) == 0){
				printf("#0x0\n");
			    }
			    else{
				if((specop1 & 0x1000) == 0x1000){
				    for(i = 0; i < 8; i++){
					if((specop1 & 0x0080) != 0){
					    printf("fp%d", i);
					    if(((specop1 << 1) & 0x00ff) != 0)
						printf("/");
					}
					specop1 <<= 1;
				    }
				}
				else{
				    specop1 &= 0x00ff;
				    for(i = 0; i < 8; i++){
					if((specop1 & 1) != 0){
					    printf("fp%d", i);
					    if((specop1 >> 1) != 0)
						printf("/");
					}
					specop1 >>= 1;
				    }
				}
				printf("\n");
			    }
			}
		    }
		    else{
			if((specop1 & 0x0800) == 0x0800){
			    printf("%s,", dregs[((specop1 & 0x0070) >> 4)] );
			}
			else{
			    if((specop1 & 0x00ff) == 0){
				printf("#0x0,");
			    }
			    else{
				if((specop1 & 0x1000) == 0x1000){
				    for(i = 0; i < 8; i++){
					if((specop1 & 0x0080) != 0){
					    printf("fp%d", i);
					    if(((specop1 << 1) & 0x00ff) != 0)
						printf("/");
					}
					specop1 <<= 1;
				    }
				}
				else{
				    specop1 &= 0x00ff;
				    for(i = 0; i < 8; i++){
					if((specop1 & 1) != 0){
					    printf("fp%d", i);
					    if((specop1 >> 1) != 0)
						printf("/");
					}
					specop1 >>= 1;
				    }
				}
				printf(",");
			    }
			}
			length += print_ef(MODE(opword), REG(opword),
					   text + length, X_SIZE);
			printf("\n");
		    }
		    return(length);
		}
	    case 1:
		specop1 = *((short *)(text + length));
		length += sizeof(short);
		if((opword & 0x003f) == 0x003a){
		    printf("ftrap%sw\t#%0x%x\n", fpcond[specop1 & 0x3f],
			   (long)*((short *)(text + length)));
		    length += sizeof(short);
		    return(length);
		}
		if((opword & 0x003f) == 0x003b){
		    printf("ftrap%sl\t#%0x%x\n", fpcond[specop1 & 0x3f],
			   *((long *)(text + length)));
		    length += sizeof(long);
		    return(length);
		}
		if((opword & 0x003f) == 0x003c){
		    printf("ftrap%s\n", fpcond[specop1 & 0x3f]);
		    return(length);
		}
		if((opword & 0x0038) == 0x0038)
		    goto bad;
		if((opword & 0x0038) == 0x0008){
		    printf("fdb%s\t%s,", fpcond[specop1 & 0x3f],
			   dregs[REG(opword)]);
		    if(print_symbol(addr + length + *((short *)(text + length)),
				    addr + length))
			printf("\n");
		    else
			printf("0x%x\n",
				addr + length + *((short *)(text + length)));
		    length += sizeof(short);
		    return(length);
		}
		printf("fs%s\t", fpcond[specop1 & 0x3f]);
		length += print_ef(MODE(opword), REG(opword),
				   text + length, B_SIZE);
		printf("\n");
		return(length);
		
	    case 2:
		if(opword == 0xf280 && *((short *)(text + length)) == 0){
		    printf("fnop\n");
		    length += sizeof(short);
		    return(length);
		}
		if(opword & 0x20)
		    goto bad;
		printf("fb%sw\t", fpcond[opword & 0x3f]);
		if(print_symbol(addr + length + *((short *)(text + length)),
				addr + length))
		    printf("\n");
		else
		    printf("0x%x\n",addr+length + *((short *)(text + length)));
		length += sizeof(short);
		return(length);

	    case 3:
		if(opword & 0x20)
		    goto bad;
		printf("fb%sl\t", fpcond[opword & 0x3f]);
		if(print_symbol(addr + length + *((long *)(text + length)),
				addr + length))
		    printf("\n");
		else
		    printf("0x%x\n",addr + length + *((long *)(text + length)));
		length += sizeof(long);
		return(length);
	    case 4:
		printf("fsave\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, B_SIZE);
		printf("\n");
		return(length);
	    case 5:
		printf("frestore\t");
		length += print_ef(MODE(opword), REG(opword),
				   text + length, B_SIZE);
		printf("\n");
		return(length);
	    case 6:
	    case 7:
		goto bad;
	    }
	}

bad:
	printf(".word\t0x%04x  | invalid instruction\n",
	       ((long)opword) & 0xffff);
	return(length);
}

union extension {
    short word;
    struct {
	unsigned da : 1;
	unsigned reg : 3;
	unsigned wl : 1;
	unsigned scale : 2;
	unsigned fb : 1;
	int      disp : 8;
    } brief;
    struct {
	unsigned da : 1;
	unsigned reg : 3;
	unsigned wl : 1;
	unsigned scale : 2;
	unsigned fb : 1;
	unsigned bs : 1;
	unsigned is : 1;
	unsigned bdsize : 2;
	unsigned : 1;
	unsigned iis : 3;
    } full;
};

/*
 * Print the effective address mode for the mode and register and the
 * extension word(s) if needed.  The length in bytes of the extension
 * words this effective address used is returned.  Text points to the
 * extension word for this effective address.  Size is the size of the
 * immediate data for the #<data> addressing mode (B_SIZE == byte, etc).
 */
static
long
print_ef(mode, reg, text, size)
long mode, reg;
char *text;
long size;
{
    long length, bd, od, addr, hidigits, lodigits;
    short extword;
    union extension ext;
    char *base, *index, *scale, bd_size, od_size;

	length = 0;
	addr = instr_addr + (text - instr_text);
	switch(mode){
	case 0:
	    printf("%s", dregs[reg]);
	    return(length);
	case 1:
	    printf("%s", aregs[reg]);
	    return(length);
	case 2:
	    printf("%s@", aregs[reg]);
	    return(length);
	case 3:
	    printf("%s@+", aregs[reg]);
	    return(length);
	case 4:
	    printf("%s@-", aregs[reg]);
	    return(length);
	case 5:
	    printf("%s@(0x%x:w)", aregs[reg],
		   ((long)*(short *)text) & 0xffff);
	    length += sizeof(short);
	    return(length);
	case 7:
	    switch(reg){
	    case 0:
		printf("0x%x:w", ((long)*(short *)text) & 0xffff);
		length += sizeof(short);
		return(length);
	    case 1:
		if(print_symbol(*(long *)text, addr) == FALSE)
		    printf("0x%x:l", *(long *)text);
		length += sizeof(long);
		return(length);
	    case 2:
		printf("pc@(0x%x:w)", ((long)*(short *)text) & 0xffff);
		length += sizeof(short);
		return(length);
	    case 3:
		break;
	    case 4:
		if(size == B_SIZE){
		    printf("#0x%x", ((long)*(short *)text) & 0xff);
		    length += sizeof(short);
		    return(length);
		}
		else if(size == W_SIZE){
		    printf("#0x%x", ((long)*(short *)text) & 0xffff);
		    length += sizeof(short);
		    return(length);
		}
		else if(size == L_SIZE){
		    printf("#");
		    if(print_symbol(*(long *)text, addr) == FALSE)
			printf("0x%x", *(long *)text);
		    length += sizeof(long);
		    return(length);
		}
		else if(size == S_SIZE){
		    printf("#%g", *(float *)text);
		    length += sizeof(float);
		    return(length);
		}
		else if(size == D_SIZE){
		    printf("#%g", *(double *)text);
		    length += sizeof(double);
		    return(length);
		}
		else if(size == X_SIZE){
		    printf("#0x%08x%08x%08x", *(long *)text,
		           *(long *)(text + 4), *(long *)(text + 8));
		    length += 12;
		    return(length);
		}
		else if(size == P_SIZE){
		    extword = *(short *)text;
		    if(extword & 0x8000)
			printf("#-%c.", ((*(short *)(text + 2)) & 0xf) + '0');
		    else
			printf("#%c.", ((*(short *)(text + 2)) & 0xf) + '0');
		    printf("%08x%08x",*(long *)(text + 4), *(long *)(text + 8));
		    if(extword & 0x4000)
			printf("e-%03x", extword & 0xfff); 
		    else
			printf("e%03x", extword & 0xfff); 
		    length += 12;
		    return(length);
		}
	    default:
		printf("<bad ef>");
		return(length);
	    }
	}

	/*
	 * To get here we know that the mode is 6 (110) or the mode is 7 (111)
	 * and the register is (011).  So that this uses either a brief or
	 * full extension word.
	 */
	ext.word = *((short *)text);
	length = sizeof(short);;
	if(mode == 6)
	    base = aregs[reg];
	else
	    base = "pc";
	if(ext.brief.da == 0)
	    index = dregs[ext.brief.reg];
	else
	    index = aregs[ext.brief.reg];
	scale = scales[ext.brief.scale];
	/* check for a brief format extension word */
	if(ext.brief.fb == 0){
	    if(ext.brief.disp != 0){
		printf("%s@(0x%x:b,%s:%c%s)", base, ext.brief.disp,
		       index, wl[ext.brief.wl], scale);
	    }
	    /* the displacement is zero so don't print it */
	    else{
		printf("%s@(%s:%c%s)", base, index, wl[ext.brief.wl], scale);
	    }
	}
	/* extension word is a full format extension word */
	else{
	    switch(ext.full.bdsize){
	    case 0:
		printf("<bad ef>");
		return(length);
	    case 1:
		break;
	    case 2:
		bd = *((unsigned short *)(text + length));
		length += sizeof(unsigned short);
		bd_size = 'w';
		break;
	    case 3:
		bd = *((long *)(text + length));
		length += sizeof(long);
		bd_size = 'l';
		break;
	    }
	    switch(ext.full.iis & 0x3){
	    case 0:
	    case 1:
		break;
	    case 2:
		od = *((unsigned short *)(text + length));
		length += sizeof(unsigned short);
		od_size = 'w';
		break;
	    case 3:
		od = *((long *)(text + length));
		length += sizeof(long);
		od_size = 'l';
		break;
	    }
	    /* check if base (address) register is not used */
	    if(ext.full.bs == 1)
		base = "";
	    /* check if base displacement is used */
	    if(ext.full.bdsize != 1){
		/* check if index register is used */
		if(ext.full.is == 0){
		    switch(ext.full.iis){
		    case 0:
			printf("%s@(0x%x:%c,%s:%c%s)", base, bd, bd_size, index, wl[ext.full.wl], scale);
			break;
		    case 1:
			printf("%s@(0x%x:%c,%s:%c%s)@", base, bd, bd_size, index, wl[ext.full.wl], scale);
			break;
		    case 2:
		    case 3:
			printf("%s@(0x%x:%c,%s:%c%s)@(0x%x:%c)", base, bd, bd_size, index, wl[ext.full.wl], scale, od, od_size);
			break;
		    case 5:
			printf("%s@(0x%x:%c)@(%s:%c%s)", base, bd, bd_size, index, wl[ext.full.wl], scale);
			break;
		    case 6:
		    case 7:
			printf("%s@(0x%x:%c)@(0x%x:%c,%s:%c%s)", base, bd, bd_size, od, od_size, index, wl[ext.full.wl], scale);
			break;
		    case 4:
		    default:
			printf("<bad ef>");
			break;
		    }
		}
		/* index register is suppressed */
		else{
		    switch(ext.full.iis){
		    case 0:
			printf("%s@(0x%x:%c)", base, bd, bd_size);
			break;
		    case 1:
			printf("%s@(0x%x:%c)@", base, bd, bd_size);
			break;
		    case 2:
		    case 3:
			printf("%s@(0x%x:%c)@(0x%x:%c)", base, bd, bd_size, od, od_size);
			break;
		    default:
			printf("<bad ef>");
			break;
		    }
		}
	    }
	    /* base displacement is not used */
	    else{
		/* check if index register is used */
		if(ext.full.is == 0){
		    switch(ext.full.iis){
		    case 0:
			printf("%s@(%s:%c%s)", base, index, wl[ext.full.wl], scale);
			break;
		    case 1:
			printf("%s@(%s:%c%s)@", base, index, wl[ext.full.wl], scale);
			break;
		    case 2:
		    case 3:
			printf("%s@(%s:%c%s)@(0x%x:%c)", base, index, wl[ext.full.wl], scale, od, od_size);
			break;
		    case 5:
			printf("%s@@(%s:%c%s)", base, index, wl[ext.full.wl], scale);
			break;
		    case 6:
		    case 7:
			printf("%s@@(0x%x:%c,%s:%c%s)", base, od, od_size, index, wl[ext.full.wl], scale);
			break;
		    case 4:
		    default:
			printf("<bad ef>");
			break;
		    }
		}
		/* index register is suppressed */
		else{
		    switch(ext.full.iis){
		    case 0:
			printf("%s@", base);
			break;
		    case 1:
			printf("%s@@", base);
			break;
		    case 2:
		    case 3:
			printf("%s@@(0x%x:%c)", base, od, od_size);
			break;
		    default:
			printf("<bad ef>");
			break;
		    }
		}
	    }
	}
	return(length);
}

/*
 * This routine rounds v to a multiple of r.  r must be a power of two.
 */
static
long
round(v, r)
long v;
unsigned long r;
{
	r--;
	v += r;
	v &= ~(long)r;
	return(v);
}


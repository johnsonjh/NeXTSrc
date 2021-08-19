/*
 * The strip(1) program.  This understands both Mach-O format (with some
 * restrictions) and a.out formats.  It takes the following options:
 *   -gg	Strip only the symsegs (produced by the -gg compiler option)
 *   -s <file>	Save only the symbols listed in <file> (the symbols are listed
 *		one per line with no white space.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <a.out.h>
#include <stab.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/loader.h>
#include <sys/types.h>
#include <sys/stat.h>

/* These are set from the command line arguments */
static char *progname;	/* name of the program for error messages (argv[0]) */
static long ggflag;	/* strip only the symtab's (created with -gg) */
static char *sfile;	/* filename of global symbol names to keep */
static char *dfile;	/* filename of filenames of debugger symbols to keep */
static long Aflag;	/* save only absolute symbols with non-zero value and
			   .objc_class_name_* symbols */
enum strip_level {
    LEVEL_NONE,		/* not doing level stripping */
    LEVEL_S,		/* -S level, strip only debugger symbols N_STAB */
    LEVEL_x,		/* -x level, strip non-globals */
    LEVEL_X,		/* -X level, strip local symbols with 'L' names */
    LEVEL_A,		/* -A level, save absolute symbols */
    LEVEL_d		/* -d level, save debugger symbols of filenames */
};
static enum strip_level level;	/* strip level, set to constants above */

static long errors;	/* non-fatal error indication, set by error routines */

/*
 * Data structures to perform selective stripping of symbol table entries.
 * save_symbols is the names of the symbols from the -s <file> argument.
 */
static struct save_symbol {
    char *name;		/* name of the global symbol */
    long order;		/* order listed in the file of symbol names */
    struct nlist *sym;	/* pointer to the nlist structure for this symbol */
} *save_symbols;
static long nsave_symbols;
/*
 * These hold the new symbol and string table created by make_new_symtab().
 */
struct nlist *new_symbols;
static long new_nsyms;
static char *new_strings;
static long new_strsize;

/*
 * The names of files to save their debugging symbols.
 */
static char **debug_filenames;
static long ndebug_filenames;

/* Internal routines */
static void strip(
    char *name);
static void setup_save_symbols(
    char *sfile);
static int cmp_qsort_name(
    const struct save_symbol *sym1,
    const struct save_symbol *sym2);
static int cmp_qsort_order(
    const struct save_symbol *sym1,
    const struct save_symbol *sym2);
static long make_new_symtab(
    char *name,
    int fd,
    long symoff,
    long nsyms,
    long stroff,
    long strsize);
static long strip_symtab(
    char *name,
    int fd,
    long symoff,
    long nsyms,
    long stroff,
    long strsize);
static void setup_debug_filenames(
    char *dfile);
static int cmp_bsearch(
    const char *name,
    const struct save_symbol *sym);
static int cmp_qsort_filename(
    const char **name1,
    const char **name2);
static int cmp_bsearch_filename(
    const char *name1,
    const char **name2);
static void *allocate(
    long size);
static void error(
    const char *format,
    ...);
static void fatal(
   const char *format,
   ...);
static void system_error(
   const char *format,
   ...);
static void system_fatal(
   const char *format,
   ...);

void
main(
int argc,
char *argv[],
char *envp[])
{
    int i, args_left, files_specified;

	progname = argv[0];

	for (i = 1; i < argc; i++) {
	    if(argv[i][0] == '-'){
		if(argv[i][1] == '\0')
		    break;
		if(strcmp(argv[i], "-gg") == 0){
		    ggflag = 1;
		}
		else if(strcmp(argv[i], "-s") == 0){
		    if(i + 1 >= argc)
			fatal("-s requires an argument");
		    if(sfile != NULL)
			fatal("only one -s option allowed");
		    sfile = argv[i + 1];
		    i++;
		}
		else if(strcmp(argv[i], "-d") == 0){
		    if(i + 1 >= argc)
			fatal("-d requires an argument");
		    if(dfile != NULL)
			fatal("only one -d option allowed");
		    dfile = argv[i + 1];
		    i++;
		    if(level == 0)
			level = LEVEL_d;
		    break;
		}
		else{
		    switch(argv[i][1]){
		    case 'S':
			level = LEVEL_S;
			break;
		    case 'X':
			level = LEVEL_X;
			break;
		    case 'x':
			level = LEVEL_x;
			break;
		    case 'A':
			if(level == 0)
			    level = LEVEL_A;
			Aflag = 1;
			break;
		    default:
			fatal("unrecognized option: %s", argv[i]);
		    }
		}
	    }
	}

	if(sfile){
	    if(ggflag)
		fatal("both -s and -gg can't be specified");
	    if(dfile)
		fatal("both -s and -d can't be specified");
	    if(level)
		fatal("both -s and any of -S, -X, -x or -A can't be "
		      "specified");
	    setup_save_symbols(sfile);
	}
	if(level && ggflag)
	    fatal("both -gg and any of -S, -X or -x can't be specified");
	if(dfile){
	    if(level == LEVEL_d || level == LEVEL_A)
		fatal("one of -S, -X, or -x must be specified with -d");
	    setup_debug_filenames(dfile);
	}

	files_specified = 0;
	args_left = 1;
	for (i = 1; i < argc; i++) {
	    if(args_left && argv[i][0] == '-'){
		if(argv[i][1] == '\0')
		    args_left = 0;
		else if(strcmp(argv[i], "-s") == 0 ||
			strcmp(argv[i], "-d") == 0)
		    i++;
	    }
	    else{
		strip(argv[i]);
		files_specified++;
	    }
	}
	if(files_specified == 0)
	    fatal("no files specified");

	if(errors)
	    exit(1);
	else
	    exit(0);
}

/*
 * Strip the specified file name according to the specified flags.
 */
static
void
strip(
char *name)
{
    int fd, i, j;
    long size, nsyms, strings_size;
    struct nlist *symbols;
    char *strings;

    struct exec exec;
    long strsize;

    struct mach_header mh;
    struct load_command *load_commands, *lcp;
    struct segment_command *sgp, *ldp;
    struct section *sp;
    struct symtab_command *stp;
    struct symseg_command *ssp;
    struct stat stat_buf;

	if((fd = open(name, O_RDWR)) < 0){
	    system_error("can't open: %s", name);
	    return;
	}
	if(read(fd, (char *)&exec, sizeof(struct exec)) != sizeof(struct exec)){
	    error("can't read header of: %s (not an object file?)", name);
	    close(fd);
	    return;
	}
	if(N_BADMAG(exec) && *((long *)&exec) != MH_MAGIC){
	    error("not in object file format: %s", name);
	    close(fd);
	    return;
	}
	if(fstat(fd, &stat_buf) == -1){
	    system_error("can't stat: %s", name);
	    close(fd);
	    return;
	}
	size = stat_buf.st_size;
	if(*((long *)&exec) == MH_MAGIC){
	    /*
	     * For this program to strip a Mach-O file requires that the order
	     * of things in the file be in a some what restricted order.  This
	     * order, excluding the header and load commands is: 
	     *		contents of all segments and sections
	     *		the relocation entries of all sections 	
	     * 		the symbol and string table
	     *		the symbol segments
	     * This is checked and objects that are in this order are stripped
	     * correctly.  Objects out of this order are not stripped but
	     * objects with are NOT known to be out of order are stripped but
	     * may result in a bad object file.  The __LINKEDIT segment has to
	     * be handled specially since it maps the information that is being
	     * stripped.
	     *
	     * This is done by calculating a size to truncate the file to and
	     * making sure the things not to be stripped are fully contained
	     * before this size.  The size is set to the size of the file
	     * (above) and then set to the smallest offset of the things that
	     * are to be stripped.
	     */
	    (void)lseek(fd, (long)0, L_SET);
	    if(read(fd, (char *)&mh, sizeof(struct mach_header)) != 
	       sizeof(struct mach_header)) {
		error("can't read mach object header of: %s", name);
		close(fd);
		return;
	    }
	    load_commands = (struct load_command *)allocate(mh.sizeofcmds);
	    if(read(fd, (char *)load_commands, mh.sizeofcmds) != mh.sizeofcmds){
		system_error("can't read load commands of: %s", name);
		goto mach_return;
	    }
	    /*
	     * Now select the smallest size to truncate the file to strip what
	     * is specified.
	     */
	    stp = NULL;
	    ssp = NULL;
	    ldp = NULL;
	    lcp = load_commands;
	    for(i = 0; i < mh.ncmds; i++){
		switch(lcp->cmd){
		case LC_SEGMENT:
		    sgp = (struct segment_command *)lcp;
		    sp = (struct section *)((char *)sgp +
			    sizeof(struct segment_command));
		    for(j = 0; j < sgp->nsects; j++){
			if(!ggflag && sp->nreloc != 0 && sp->reloff < size)
			    size = sp->reloff;
			sp++;
		    }
		    if(strcmp(sgp->segname, SEG_LINKEDIT) == 0){
			if(ldp != NULL){
			    error("object file format error (more that one "
				  SEG_LINKEDIT " segment) in: %s", name);
			    goto mach_return;
			}
			ldp = sgp;
		    }
		    break;
		case LC_SYMTAB:
		    if(stp != NULL){
			error("object file format error (more that one "
			      "LC_SYMTAB command) in: %s",name);
			goto mach_return;
		    }
		    stp = (struct symtab_command *)lcp;
		    break;
		case LC_SYMSEG:
		    if(ssp != NULL){
			error("object file format error (more that one "
			      "LC_SYMSEG command) in: %s",name);
			goto mach_return;
		    }
		    ssp = (struct symseg_command *)lcp;
		    if(ggflag && ssp->size != 0)
			size = ssp->offset;
		    break;
		case LC_THREAD:
		case LC_UNIXTHREAD:
		case LC_LOADFVMLIB:
		case LC_IDFVMLIB:
		case LC_IDENT:
		    break;
		default:
		    error("unknown load command %d (stripped result maybe bad)",
			  i);
		    break;
		}
		if(lcp->cmdsize <= 0){
		    error("object file format error (bad size of load command "
			  "%d, negative or zero) in: %s", i, name);
		    goto mach_return;
		}
		lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	    }
	    if(!ggflag && stp != NULL){
		if(stp->nsyms != 0 && stp->symoff < size)
		    size = stp->symoff;
		if(stp->strsize != 0 && stp->stroff < size)
		    size = stp->stroff;
	    }
	    if(ssp != NULL){
		if(ssp->size != 0 && ssp->offset < size)
		    size = ssp->offset;
	    }

	    /*
	     * Now make sure the size to truncate the object file to does not
	     * truncate anything known not to be stripped and strips things that
	     * it should.  Checks checks have been remove from this loop since
	     * they have been done above.
	     */
	    lcp = load_commands;
	    for(i = 0; i < mh.ncmds; i++){
		if(lcp->cmd == LC_SEGMENT &&
		   ldp != (struct segment_command *)lcp){
		    sgp = (struct segment_command *)lcp;
		    sp = (struct section *)((char *)sgp +
			    sizeof(struct segment_command));
		    if(sgp->filesize != 0 &&
		       sgp->fileoff + sgp->filesize > size)
			goto no_strip_mach_return;
		    for(j = 0; j < sgp->nsects; j++){
			if((sp->flags & S_ZEROFILL) != S_ZEROFILL &&
			   sp->size != 0 && sp->offset + sp->size > size)
				goto no_strip_mach_return;
			if(ggflag){
			    if(sp->nreloc != 0 && sp->reloff + sp->nreloc *
			       sizeof(struct relocation_info) > size)
				goto no_strip_mach_return;
			}
			else{
			    if(sp->nreloc != 0 && sp->reloff + sp->nreloc *
			       sizeof(struct relocation_info) < size)
				goto no_strip_mach_return;
			    sp->reloff = 0;
			    sp->nreloc = 0;
			}
			sp++;
		    }
		}
		lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	    }

	    if(ggflag){
		if(stp != NULL){
		    if(stp->nsyms != 0 && stp->symoff + stp->nsyms *
		       sizeof(struct nlist) > size)
			goto no_strip_mach_return;
		    if(stp->strsize != 0 && stp->stroff + stp->strsize > size)
			goto no_strip_mach_return;
		}
		if(size == stat_buf.st_size){
		    error("%s already stripped of -gg symbols", name);
		    goto mach_return;
		}
	    }
	    else{
		if(stp != NULL){
		    if(stp->nsyms != 0 && stp->symoff + stp->nsyms *
		       sizeof(struct nlist) < size)
			goto no_strip_mach_return;
		    if(stp->strsize != 0 && stp->stroff + stp->strsize < size)
			goto no_strip_mach_return;
		    if(sfile != NULL || level){
			if(sfile != NULL){
			    if(make_new_symtab(name, fd, stp->symoff,stp->nsyms,
		    			    stp->stroff, stp->strsize)){
				close(fd);
				return;
			    }
			}
			else{
			    if(strip_symtab(name, fd, stp->symoff, stp->nsyms,
					 stp->stroff, stp->strsize)){
				close(fd);
				return;
			    }
			}
			stp->symoff = size;
			stp->nsyms = new_nsyms; 
			stp->stroff = size + new_nsyms * sizeof(struct nlist);
			stp->strsize = new_strsize;
		    }
		    else {
			stp->symoff = 0;
			stp->nsyms = 0;
			stp->stroff = 0;
			stp->strsize = 0;
		    }
		}
		if(size == stat_buf.st_size){
		    error("%s already stripped", name);
		    goto mach_return;
		}
	    }
	    if(ssp != NULL){
		ssp->offset = 0;
		ssp->size = 0;
	    }
	    if(ldp != NULL){
		if(ldp->fileoff > size)
		    goto no_strip_mach_return;
		ldp->filesize = (size - ldp->fileoff) +
				new_nsyms * sizeof(struct nlist) +
				new_strsize;
		ldp->vmsize = ldp->filesize;
	    }
	    if(ftruncate(fd, size) < 0){
		system_error("can't truncate: %s", name);
		goto mach_return;
	    }
	    (void)lseek(fd, (long)sizeof(struct mach_header), L_SET);
	    if(write(fd, (char *)load_commands, mh.sizeofcmds) !=mh.sizeofcmds){
		system_error("can't rewrite load commands of: %s", name);
		goto mach_return;
	    }
	    if(sfile != NULL || level){
		(void)lseek(fd, size, L_SET);
		if(write(fd, (char *)new_symbols, new_nsyms *
		   sizeof(struct nlist)) != new_nsyms * sizeof(struct nlist)){
		    system_error("can't write new symbol table of: %s", name);
		    goto mach_return;
		}
		if(write(fd, (char *)new_strings, new_strsize) != new_strsize)
		    system_error("can't write new string table of: %s", name);
		    goto mach_return;
	    }
	    goto mach_return;

no_strip_mach_return:
	    error("%s not in an order that can be stripped", name);
mach_return:
	    free(load_commands);
	}
	else{
	    if(ggflag){
		/*
		 * The assumption is that if it has symsegs to strip it also has
		 * a symbol table and thus a string table.
		 */
		if(exec.a_syms == 0){
		    error("%s already stripped of -gg symbols", name);
		    close(fd);
		    return;
		}
		(void)lseek(fd, N_STROFF(exec), L_SET);
		if(read(fd, (char *)&strsize, sizeof(long)) != sizeof(long)){
		    error("can't read string table size of: %s", name);
		    close(fd);
		    return;
		}
		if(N_STROFF(exec) + strsize >= size){
		    error("%s already stripped of -gg symbols", name);
		    close(fd);
		    return;
		}
		size = N_STROFF(exec) + strsize;
	    }
	    else{
		if(exec.a_syms == 0 && exec.a_trsize == 0 && exec.a_drsize ==0){
		    error("%s already stripped", name);
		    close(fd);
		    return;
		}
		size = exec.a_text + exec.a_data;
		if(exec.a_magic != ZMAGIC)
		    size += sizeof(struct exec);
		if(sfile != NULL || level){
		    (void)lseek(fd, N_STROFF(exec), L_SET);
		    if(read(fd, (char *)&strsize, sizeof(long)) !=sizeof(long)){
			error("can't read string table size of: %s", name);
			close(fd);
			return;
		    }
		    if(sfile != NULL)
			if(make_new_symtab(name, fd, N_SYMOFF(exec),
					   exec.a_syms / sizeof(struct nlist),
					   N_STROFF(exec), strsize)){
			    close(fd);
			    return;
			}
		    else
			if(strip_symtab(name, fd, N_SYMOFF(exec),
				        exec.a_syms / sizeof(struct nlist),
					N_STROFF(exec), strsize)){
			    close(fd);
			    return;
			}
		    exec.a_syms = new_nsyms * sizeof(struct nlist);
		}
		else{
		    exec.a_syms = 0;
		}
		exec.a_trsize = 0;
		exec.a_drsize = 0;
	    }
	    if(ftruncate(fd, size) < 0) {
		system_error("can't truncate: %s", name);
		close(fd);
		return;
	    }
	    (void)lseek(fd, (long)0, L_SET);
	    if(write(fd, (char *)&exec, sizeof(struct exec)) !=
	       sizeof(struct exec)){
		system_error("can't rewrite header of: %s", name);
		close(fd);
		return;
	    }
	    if(sfile != NULL){
		(void)lseek(fd, size, L_SET);
		if(write(fd, (char *)new_symbols, new_nsyms *
		   sizeof(struct nlist)) != new_nsyms * sizeof(struct nlist)){
		    system_error("can't write new symbol table of: %s", name);
		    close(fd);
		    return;
		}
		if(write(fd, (char *)new_strings, new_strsize) != new_strsize){
		    system_error("can't write new string table of: %s", name);
		    close(fd);
		    return;
		}
	    }
	}
	close(fd);
}

/*
 * This is called if there is a -s option specified.  It reads the file with
 * the strings in it and places them in an array of save_symbol structures.
 * The file that contains the symbol names must have symbol names one per line
 * with no white space (except the newlines).
 */
static
void
setup_save_symbols(
char *sfile)
{
    int fd, i, strings_size;
    struct stat stat_buf;
    char *strings, *p;

	if((fd = open(sfile, O_RDONLY)) < 0){
	    system_error("can't open: %s", sfile);
	    return;
	}
	if(fstat(fd, &stat_buf) == -1){
	    system_error("can't stat: %s", sfile);
	    close(fd);
	    return;
	}
	strings_size = stat_buf.st_size;
	strings = (char *)allocate(strings_size + 1);
	strings[strings_size] = '\0';
	if(read(fd, strings, strings_size) != strings_size){
	    system_error("can't read: %s", sfile);
	    close(fd);
	    return;
	}
	p = strings;
	for(i = 0; i < strings_size; i++){
	    if(*p == '\n'){
		*p = '\0';
		nsave_symbols++;
	    }
	    p++;
	}
	save_symbols = (struct save_symbol *)allocate(nsave_symbols *
						 sizeof(struct save_symbol));
	p = strings;
	for(i = 0; i < nsave_symbols; i++){
	    save_symbols[i].name = p;
	    save_symbols[i].order = i;
	    p += strlen(p) + 1;
	}

#ifdef DEBUG
	printf("Save symbols:\n");
	for(i = 0; i < nsave_symbols; i++){
	    printf("0x%x name = %s order %d\n", &(save_symbols[i]),
		   save_symbols[i].name, save_symbols[i].order);
	}
#endif DEBUG
}

/*
 * Make a new symbol table and string table from the symbol names in the
 * save_symbol list that are also the symbols in the object file.  The
 * name of the object file, an open file descriptor, the offset to the symbol
 * table, number of symbols, the offset to the string table and the string
 * table size are passed in.  The new symbol table is built and new_symbols
 * is left pointing to it.  The number of new symbols is left in new_nsyms,
 * the new string table is built and new_stings is left pointing to it and
 * new_strsize is left containing it.  This routine returns zero if
 * successfull and non-zero otherwise.
 */
static
long
make_new_symtab(
char *name,
int fd,
long symoff,
long nsyms,
long stroff,
long strsize)
{
    long i, j, missing_syms;
    struct nlist *symbols;
    char *strings, *p;
    struct save_symbol *sp;

	symbols = (struct nlist *)allocate(nsyms * sizeof(struct nlist));
	strings = (char *)allocate(strsize);
	(void)lseek(fd, symoff, L_SET);
	if(read(fd, (char *)symbols, nsyms * sizeof(struct nlist)) !=
	   nsyms * sizeof(struct nlist)){
	    error("can't read symbol table of: %s", name);
	    free(symbols);
	    free(strings);
	    return(1);
	}
	(void)lseek(fd, stroff, L_SET);
	if(read(fd, strings, strsize) != strsize){
	    error("can't read string table of: %s", name);
	    free(symbols);
	    free(strings);
	    return(1);
	}

	qsort(save_symbols, nsave_symbols, sizeof(struct save_symbol),
	      (int (*)(const void *, const void *))cmp_qsort_name);
	for(i = 0; i < nsave_symbols; i++)
	    save_symbols[i].sym = NULL;

	for(i = 0; i < nsyms; i++){
	    if((symbols[i].n_type & N_EXT) == 0)
		continue;
	    if(symbols[i].n_un.n_strx == 0)
		continue;
	    if(symbols[i].n_un.n_strx < 0 || symbols[i].n_un.n_strx > strsize){
		error("bad string index for symbol table entry %d in: %s",
		      name);
		free(symbols);
		free(strings);
		return(1);
	    }
	    sp = bsearch(strings + symbols[i].n_un.n_strx, save_symbols,
			 nsave_symbols, sizeof(struct save_symbol),
			 (int (*)(const void *, const void *))cmp_bsearch);
	    if(sp != NULL){
		if(sp->sym != NULL)
		    error("more than one symbol for: %s found in: %s", sp->name,
			  name);
		else{
		    sp->sym = &(symbols[i]);
		}
	    }
	}

	qsort(save_symbols, nsave_symbols, sizeof(struct save_symbol),
	      (int (*)(const void *, const void *))cmp_qsort_order);
#ifdef DEBUG
	printf("Ordered symbols:\n");
	for(i = 0; i < nsave_symbols; i++){
	    printf("0x%x name = %s order %d\n", &(save_symbols[i]),
		   save_symbols[i].name, save_symbols[i].order);
	}
#endif
	missing_syms = 0;
	new_nsyms = 0;
	new_strsize = sizeof(long);
	for(i = 0; i < nsave_symbols; i++){
	    if(save_symbols[i].sym == NULL){
		if(missing_syms == 0){
		    error("symbols names listed in: %s not in: %s", sfile,name);
		    missing_syms = 1;
		}
		fprintf(stderr, "%s\n", save_symbols[i].name);
	    }
	    else{
		new_nsyms++;
		save_symbols[i].sym->n_un.n_strx = new_strsize;
		new_strsize += strlen(save_symbols[i].name) + 1;
	    }
	}
	if(new_symbols != NULL)
	    free(new_symbols);
	if(new_strings != NULL)
	    free(new_strings);
	new_symbols =(struct nlist *)allocate(new_nsyms * sizeof(struct nlist));
	new_strings = (char *)allocate(new_strsize);

	bcopy((char *)&new_strsize, new_strings, sizeof(long));
	p = new_strings + sizeof(long);
	for(i = 0, j = 0; i < nsave_symbols; i++, j++){
	    if(save_symbols[i].sym != NULL){
	        new_symbols[j] = *(save_symbols[i].sym);
		strcpy(p, save_symbols[i].name);
		p += strlen(save_symbols[i].name) + 1;
	    }
	}

	free(symbols);
	free(strings);
	return(0);
}

/*
 * This is called if there is a -d option specified.  It reads the file with
 * the strings in it and places them in the array debug_filenames and sorts
 * them by name.  The file that contains the file names must have names one
 * per line with no white space (except the newlines).
 */
static
void
setup_debug_filenames(
char *dfile)
{
    int fd, i, strings_size;
    struct stat stat_buf;
    char *strings, *p;

	if((fd = open(dfile, O_RDONLY)) < 0){
	    system_error("can't open: %s", dfile);
	    return;
	}
	if(fstat(fd, &stat_buf) == -1){
	    system_error("can't stat: %s", dfile);
	    close(fd);
	    return;
	}
	strings_size = stat_buf.st_size;
	strings = (char *)allocate(strings_size + 1);
	strings[strings_size] = '\0';
	if(read(fd, strings, strings_size) != strings_size){
	    system_error("can't read: %s", dfile);
	    close(fd);
	    return;
	}
	p = strings;
	for(i = 0; i < strings_size; i++){
	    if(*p == '\n'){
		*p = '\0';
		ndebug_filenames++;
	    }
	    p++;
	}
	debug_filenames = (char **)allocate(ndebug_filenames * sizeof(char *));
	p = strings;
	for(i = 0; i < ndebug_filenames; i++){
	    debug_filenames[i] = p;
	    p += strlen(p) + 1;
	}
	qsort(debug_filenames, ndebug_filenames, sizeof(char *),
	      (int (*)(const void *, const void *))cmp_qsort_filename);

#ifdef DEBUG
	printf("Debug filenames:\n");
	for(i = 0; i < ndebug_filenames; i++){
	    printf("filename = %s\n", debug_filenames[i]);
	}
#endif DEBUG
}

/*
 * Strip the symbol table to the level specified by the command line arguments.
 * The name of the object file, an open file descriptor, the offset to the
 * symbol table, number of symbols, the offset to the string table and the
 * string table size are passed in.  The new symbol table is built and
 * new_symbols is left pointing to it.  The number of new symbols is left in
 * new_nsyms, the new string table is built and new_stings is left pointing to
 * it and new_strsize is left containing it.  This routine returns zero if
 * successfull and non-zero otherwise.
 */
static
long
strip_symtab(
char *name,
int fd,
long symoff,
long nsyms,
long stroff,
long strsize)
{
    long i, j, save_debug;
    struct nlist *symbols;
    char *strings, *p, *saves, *basename;

	save_debug = 0;
	symbols = (struct nlist *)allocate(nsyms * sizeof(struct nlist));
	strings = (char *)allocate(strsize);
	saves = (char *)allocate(nsyms);
	bzero(saves, nsyms);
	(void)lseek(fd, symoff, L_SET);
	if(read(fd, (char *)symbols, nsyms * sizeof(struct nlist)) !=
	   nsyms * sizeof(struct nlist)){
	    error("can't read symbol table of: %s", name);
	    free(symbols);
	    free(strings);
	    return(1);
	}
	(void)lseek(fd, stroff, L_SET);
	if(read(fd, strings, strsize) != strsize){
	    error("can't read string table of: %s", name);
	    free(symbols);
	    free(strings);
	    return(1);
	}

	new_nsyms = 0;
	new_strsize = sizeof(long);
	for(i = 0; i < nsyms; i++){
	    if(symbols[i].n_un.n_strx != 0){
		if(symbols[i].n_un.n_strx < 0 ||
		   symbols[i].n_un.n_strx > strsize){
		    error("bad string index for symbol table entry %d in: "
			  "%s", name);
		    free(symbols);
		    free(strings);
		    return(1);
		}
	    }
	    if((symbols[i].n_type & N_EXT) == 0){
		if(level != LEVEL_x || dfile){
		    if(symbols[i].n_type & N_STAB){
			if(dfile && symbols[i].n_type == N_SO){
			    if(symbols[i].n_un.n_strx != 0){
				basename = strrchr(strings +
						   symbols[i].n_un.n_strx, '/');
				if(basename != NULL)
				    basename++;
				else
				    basename = strings + symbols[i].n_un.n_strx;
				p = bsearch(basename, debug_filenames,
					    ndebug_filenames, sizeof(char *),
			 		    (int (*)(const void *, const void *)
					    )cmp_bsearch_filename);
				/*
				 * Save the bracketing N_SO. For each N_SO that
				 * has a filename there is an N_SO that has a
				 * name of "" which ends the stabs for that file
				 */
				if(*basename != '\0')
				    if(p != NULL)
					save_debug = 1;
				    else
					save_debug = 0;
			    }
			    else{
				save_debug = 0;
			    }
			}
			if(level != LEVEL_S || save_debug){
			    if(symbols[i].n_un.n_strx != 0)
				new_strsize += strlen(strings +
						  symbols[i].n_un.n_strx) + 1;
			    new_nsyms++;
			    saves[i] = 1;
			}
		    }
		    else if(symbols[i].n_un.n_strx != 0 &&
		            strings[symbols[i].n_un.n_strx] == 'L' &&
		            (symbols[i].n_type & N_STAB) == 0){
			if(level != LEVEL_X){
			    new_strsize += strlen(strings +
					      symbols[i].n_un.n_strx) + 1;
			    new_nsyms++;
			    saves[i] = 1;
			}
		    }
		    else if(level != LEVEL_x){
			if(symbols[i].n_un.n_strx != 0)
			    new_strsize += strlen(strings +
						  symbols[i].n_un.n_strx) + 1;
			new_nsyms++;
			saves[i] = 1;
		    }
		}
	    }
	    else{
		if(Aflag){
		    if(symbols[i].n_type == (N_EXT | N_ABS) &&
		       (symbols[i].n_value != 0 ||
		       (symbols[i].n_un.n_strx != 0 &&
			strncmp(strings + symbols[i].n_un.n_strx,
				".objc_class_name_",
				sizeof(".objc_class_name_") - 1) == 0))){
			new_strsize +=
				strlen(strings + symbols[i].n_un.n_strx) + 1;
			new_nsyms++;
			saves[i] = 1;
		    }
		}
		else{
		    new_strsize += strlen(strings + symbols[i].n_un.n_strx) + 1;
		    new_nsyms++;
		    saves[i] = 1;
		}
	    }
	}

	if(new_symbols != NULL)
	    free(new_symbols);
	if(new_strings != NULL)
	    free(new_strings);
	new_symbols =(struct nlist *)allocate(new_nsyms * sizeof(struct nlist));
	new_strings = (char *)allocate(new_strsize);

	bcopy((char *)&new_strsize, new_strings, sizeof(long));
	p = new_strings + sizeof(long);

	j = 0;
	for(i = 0; i < nsyms; i++){
	    if(saves[i]){
		new_symbols[j] = symbols[i];
		if(symbols[i].n_un.n_strx != 0){
		    strcpy(p, strings + symbols[i].n_un.n_strx);
		    new_symbols[j].n_un.n_strx = p - new_strings;
		    p += strlen(p) + 1;
		}
		j++;
	    }
	}

	free(symbols);
	free(strings);
	free(saves);
	return(0);
}

/*
 * Function for qsort for comparing saved symbol names.
 */
static
int
cmp_qsort_name(
const struct save_symbol *sym1,
const struct save_symbol *sym2)
{
	return(strcmp(sym1->name, sym2->name));
}

/*
 * Function for qsort for comparing saved symbol order values.
 */
static
int
cmp_qsort_order(
const struct save_symbol *sym1,
const struct save_symbol *sym2)
{
	return(sym1->order - sym2->order);
}

/*
 * Function for bsearch for finding a symbol.
 */
static
int
cmp_bsearch(
const char *name,
const struct save_symbol *sym)
{
	return(strcmp(name, sym->name));
}

/*
 * Function for qsort for comparing object names.
 */
static
int
cmp_qsort_filename(
const char **name1,
const char **name2)
{
	return(strcmp(*name1, *name2));
}

/*
 * Function for bsearch for finding a object name.
 */
static
int
cmp_bsearch_filename(
const char *name1,
const char **name2)
{
	return(strcmp(name1, *name2));
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
	    system_fatal("virtual memory exhausted (malloc failed)");
	return(p);
}

/*
 * Print the error message and set the non-fatal error indication.
 */
static
void
error(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
	va_end(ap);
	errors = 1;
}

/*
 * Print the fatal error message, and exit non-zero.
 */
static
void
fatal(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

/*
 * Print the error message along with the system error message, set the
 * non-fatal error indication.
 */
static
void
system_error(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, " (%s)\n", strerror(errno));
	va_end(ap);
	errors = 1;
}

/*
 * Print the fatal message along with the system error message, and exit
 * non-zero.
 */
static
void
system_fatal(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, " (%s)\n", strerror(errno));
	va_end(ap);
	exit(1);
}


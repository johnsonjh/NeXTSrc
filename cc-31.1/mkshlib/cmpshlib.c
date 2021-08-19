#undef DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/loader.h>
#include <nlist.h>
#include <mach.h>
#include <libc.h>
#include <reloc.h>
#include "branch.h"
#include "errors.h"
#include "misc.h"
#include "mkshlib.h"

static void usage(void);
static void make_oldtarget_hashtable(char *old_target);
static void check_newtarget(char *new_target);
static void open_target(char *target, struct nlist **symbols, long *nsyms,
			char **strings, long *strsize, char **text,
			long *textaddr, long *textsize);
static void make_sorted_text_symbols(struct nlist *symbols, long nsyms,
				     struct nlist **text_symbols,
				     long *ntext_symbols);
static void make_global_symbols(struct nlist *symbols, long nsyms,
				struct nlist **global_symbols,
				long *nglobal_symbols);
static long get_target_addr(long value, char *name, char *text, long addr,
			    long size, char *target);
static int cmp_qsort(struct nlist *sym1, struct nlist *sym2);
static int cmp_bsearch(long *value, struct nlist *sym);
static struct ext *lookup(char *name);

/* Structure of the members of the hash table for the old target */
struct ext {
    char *name;		  /* symbol name */
    long type;		  /* symbol type (n_type & N_TYPE) */
    long sect;		  /* symbol section (n_sect) */
    long value;		  /* symbol value */
    char *branch_target;  /* if non-zero the branch target symbol name */
    struct ext *next;	  /* next ext on the hash chain */
};
#define EXT_HASH_SIZE	251
struct ext *ext_hash[EXT_HASH_SIZE];

char *progname;		/* name of the program for error messages (argv[0]) */

/* file name of the specification input file */
char *spec_filename = NULL;

char *old_target = NULL;
char *new_target = NULL;

void
main(
int argc,
char *argv[],
char *envp[])
{
    long i;

	progname = argv[0];
	for(i = 1; i < argc; i++){
	    if(argv[i][0] == '-'){
		if(strcmp("-s", argv[i]) == 0){
		    if(i + 1 > argc)
			usage();
		    spec_filename = argv[++i];
		}
		else
		   usage();
	    }
	    else{
		if(old_target == NULL)
		    old_target = argv[i];
		else if(new_target == NULL)
		    new_target = argv[i];
		else
		    usage();
	    }
	}

	if(old_target == NULL || new_target == NULL)
	    usage();

	if(spec_filename != NULL){
	    minor_version = 1;
	    parse_spec();
	}
	else
	    fprintf(stderr, "%s: no -s spec_file specified (nobranch_text "
		    "symbols not checked)", progname);

	make_oldtarget_hashtable(old_target);
	check_newtarget(new_target);
	if(errors){
	    exit(EXIT_FAILURE);
	}
	else{
	    exit(EXIT_SUCCESS);
	}
}

/*
 * Print the usage line and exit indicating failure.
 */
static
void
usage(void)
{
	fprintf(stderr, "Usage: %s [-s spec_file] <old target> <new target>\n",
		progname);
	exit(EXIT_FAILURE);
}

/*
 * Build the hash table of the external symbols of old target shared library.
 */
static
void
make_oldtarget_hashtable(
char *old_target)
{
    struct nlist *symbols, *text_symbols, *np;
    long nsyms, strsize, textaddr, textsize, i, ntext_symbols,
	target_addr, hash_key;
    char *text, *strings;
    struct ext *extp;

	fprintf(stderr, "%s: building hash table of external symbols for old "
		"target: %s\n", progname, old_target);

	open_target(old_target, &symbols, &nsyms, &strings, &strsize,
		    &text, &textaddr, &textsize);

	make_sorted_text_symbols(symbols, nsyms, &text_symbols, &ntext_symbols);

	/*
	 * Build the hash table of external symbols.
	 */
	for(i = 0; i < nsyms; i++){
	    if((symbols[i].n_type & N_EXT) != N_EXT)
		continue;
	    hash_key = hash_string(symbols[i].n_un.n_name) % EXT_HASH_SIZE;
	    extp = ext_hash[hash_key];
	    while(extp != (struct ext *)0){
		if(strcmp(symbols[i].n_un.n_name, extp->name) == 0)
		    fatal("Symbol %s appears more than once in target shared "
			  "library: %s", extp->name, old_target);
		extp = extp->next;
	    }
	    extp = (struct ext *)xmalloc(sizeof(struct ext));
	    extp->name = symbols[i].n_un.n_name;
	    extp->type = symbols[i].n_type & N_TYPE;
	    extp->sect = symbols[i].n_sect;
	    extp->value = symbols[i].n_value;
	    extp->branch_target = (char *)0;
	    extp->next = ext_hash[hash_key];
	    ext_hash[hash_key] = extp;
	    if(extp->type == N_SECT && extp->sect == 1)
		ntext_symbols++;
	}

	/*
	 * For each branch table slot find the name of the symbol in that slot.
	 */
	for(i = 0; i < EXT_HASH_SIZE; i++){
	    extp = ext_hash[i];
	    while(extp != (struct ext *)0){
		if((extp->type == N_SECT && extp->sect == 1) &&
		   strncmp(extp->name, BRANCH_SLOT_NAME,
		           sizeof(BRANCH_SLOT_NAME) - 1) == 0){
		    target_addr = get_target_addr(extp->value, extp->name,
				    text, textaddr, textsize, old_target);
		    np = bsearch(&target_addr, text_symbols, ntext_symbols,
				 sizeof(struct nlist),
			     (int (*)(const void *, const void *)) cmp_bsearch);
		    if(np == (struct nlist *)0)
			fatal("Can't find a text symbol for the target (0x%x) "
			      "of branch table symbol: %s in: %s", target_addr,
			      extp->name, old_target);
		    extp->branch_target = np->n_un.n_name;
		}
		extp = extp->next;
	    }
	}

#ifdef DEBUG
	for(i = 0; i < EXT_HASH_SIZE; i++){
	    extp = ext_hash[i];
	    while(extp != (struct ext *)0){
		printf("name = %s value = 0x%x type = 0x%x sect = %d ",
		       extp->name, extp->value, extp->type, extp->sect);
		if(extp->branch_target != (char *)0)
		    printf("branch_target = %s\n", extp->branch_target);
		else
		    printf("\n");
		extp = extp->next;
	    }
	}
#endif DEBUG
}

/*
 * Check the new target shared library against the old target shared library
 * hash table of symbols.
 */
static
void
check_newtarget(
char *new_target)
{
    struct nlist *symbols, *text_symbols, *global_symbols, *np;
    long nsyms, strsize, textaddr, textsize, ntext_symbols, i, target_addr,
	first_new_data, printed_first_new_data, nglobal_symbols;
    char *strings, *text;
    struct ext *extp;
    long hash_key;
    struct oddball *obp;

	fprintf(stderr, "%s: checking external symbols of new target: "
	        "%s\n", progname, new_target);

	open_target(new_target, &symbols, &nsyms, &strings, &strsize,
		    &text, &textaddr, &textsize);

	make_sorted_text_symbols(symbols, nsyms, &text_symbols, &ntext_symbols);

	make_global_symbols(symbols, nsyms, &global_symbols, &nglobal_symbols);

	/* sort the global symbols by value so they get checked in order */
	qsort(global_symbols, nglobal_symbols, sizeof(struct nlist),
	      (int (*)(const void *, const void *))cmp_qsort);

	/* Check jump table targets */
	fprintf(stderr, "Checking the jump table targets\n");
	for(i = 0; i < nglobal_symbols; i++){
	    if((global_symbols[i].n_type & N_EXT) != N_EXT)
		continue;
	    if((global_symbols[i].n_type & N_TYPE) == N_SECT && 
	        global_symbols[i].n_sect == 1){
		if(strncmp(global_symbols[i].n_un.n_name, BRANCH_SLOT_NAME,
			   sizeof(BRANCH_SLOT_NAME) - 1) == 0){
		    extp = lookup(global_symbols[i].n_un.n_name);
		    if(extp == (struct ext *)0)
			continue;
		    target_addr = get_target_addr(extp->value, extp->name,
				    text, textaddr, textsize, new_target);
		    np = bsearch(&target_addr, text_symbols, ntext_symbols,
				 sizeof(struct nlist),
			      (int (*)(const void *, const void *))cmp_bsearch);
		    if(np == (struct nlist *)0)
			fatal("Can't find a text symbol for the target (0x%x) "
			      "of branch table symbol: %s in: %s", target_addr,
			      extp->name, new_target);
		    if(strcmp(extp->branch_target, np->n_un.n_name) != 0)
			error("Branch table symbol: %s has different targets "
			      "(%s and %s)", extp->name, extp->branch_target,
			      np->n_un.n_name);
		}
	    }

	}

	/* Check global data addresses */
	fprintf(stderr, "Checking the global data addresses\n");
	first_new_data = -1;
	printed_first_new_data = 0;
	for(i = 0; i < nglobal_symbols; i++){
	    if((global_symbols[i].n_type & N_EXT) != N_EXT)
		continue;
	    if((global_symbols[i].n_type & N_TYPE) == N_SECT && 
	        global_symbols[i].n_sect == 2){

		/* if it is a private extern don't check it */
		hash_key = hash_string(global_symbols[i].n_un.n_name) %
							      ODDBALL_HASH_SIZE;
		obp = oddball_hash[hash_key];
		while(obp != (struct oddball *)0){
		    if(strcmp(obp->name, global_symbols[i].n_un.n_name) == 0)
			break;
		    obp = obp->next;
		}
		if(obp != (struct oddball *)0 && obp->private == 1)
		   continue;

		extp = lookup(global_symbols[i].n_un.n_name);
		if(extp == (struct ext *)0){
		    if(first_new_data == -1)
			first_new_data = i;
		    continue;
		}
		if(global_symbols[i].n_value != extp->value){
		    if(!printed_first_new_data && first_new_data != -1){
			error("First new data symbol: %s at address: 0x%x "
			      "before previous old data",
			      global_symbols[first_new_data].n_un.n_name,
			      global_symbols[first_new_data].n_value);
			printed_first_new_data = 1;
		    }
		    error("External data symbol: %s does NOT have the same "
			  "address (0x%x and 0x%x)", extp->name,  extp->value,
			   global_symbols[i].n_value);
		}
	    }
	    else if((global_symbols[i].n_type & N_TYPE) == N_SECT && 
	            global_symbols[i].n_sect == 1){
		hash_key = hash_string(global_symbols[i].n_un.n_name) %
							      ODDBALL_HASH_SIZE;
		obp = oddball_hash[hash_key];
		while(obp != (struct oddball *)0){
		    if(strcmp(obp->name, global_symbols[i].n_un.n_name) == 0)
			break;
		    obp = obp->next;
		}
		if(obp != (struct oddball *)0 &&
		   obp->nobranch == 1 && obp->private == 0){
		    extp = lookup(global_symbols[i].n_un.n_name);
		    if(extp != NULL){
			if(global_symbols[i].n_value != extp->value){
			    error("External nobranch_text symbol: %s does "
				  "NOT have the same address (0x%x and "
				  "0x%x)", extp->name,  extp->value,
				  global_symbols[i].n_value);
			}
		    }
		}
	    }
	}
}

/*
 * Open the target shared library and return information for the symbol and
 * string table and the text segment.
 */
static
void
open_target(
char *target, 	       /* name of the target shared library to open */
struct nlist **symbols,/* pointer to the symbol table to return */
long *nsyms,	       /* pointer to the number of symbols to return */
char **strings,	       /* pointer to the string table to return */
long *strsize,	       /* pointer to the string table size to return */
char **text,	       /* pointer to the text segment to return */
long *textaddr,	       /* pointer to the text segment's address to return */
long *textsize)	       /* pointer to the text segment's size to return */
{
    int fd;
    struct stat stat_buf;
    long size, sizeofcmds, i;
    kern_return_t r;
    char *addr;
    struct mach_header *mhp;
    struct load_command *lcp;
    struct symtab_command *stp;
    struct segment_command *text_seg, *sgp;

	/* Open the file and map it in */
	if((fd = open(target, O_RDONLY)) == -1)
	    perror_fatal("Can't open target shared library: %s", target);
	if(fstat(fd, &stat_buf) == -1)
	    perror_fatal("Can't stat target shared library: %s", target);
	size = stat_buf.st_size;
	if((r = map_fd(fd, 0, &addr, TRUE, size)) != KERN_SUCCESS)
	    mach_fatal(r, "Can't map target shared library: %s", target);
	close(fd);

	/* Check to see if this is a good target shared library */
	if(size < sizeof(struct mach_header))
	    fatal("Bad target shared library file: %s (mach header would "
		  "extend past the end of the file)", target);
	mhp = (struct mach_header *)addr;
	if(mhp->magic != MH_MAGIC)
	    fatal("File: %s not a target shared library (wrong magic number)", 
		  target);
	if(mhp->filetype != MH_FVMLIB)
	    fatal("File: %s not a target shared library (wrong filetype)", 
		  target);
	if(mhp->sizeofcmds + sizeof(struct mach_header) > size)
	    fatal("Bad target shared library file: %s (load commands would "
		  "extend past the end of the file)", target);

	/* Get the load comand for the symbol table and text segment */
	lcp = (struct load_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	stp = (struct symtab_command *)0;
	text_seg = (struct segment_command *)0;
	sizeofcmds = 0;
	for(i = 0; i < mhp->ncmds; i++){
	    switch(lcp->cmd){ 
	    case LC_SYMTAB:
		if(stp != (struct symtab_command *)0)
		    fatal("More than one symtab_command in: %s", target);
		else
		    stp = (struct symtab_command *)lcp;
		break;
	    case LC_SEGMENT:
		sgp = (struct segment_command *)lcp;
		if(strcmp(sgp->segname, SEG_TEXT) == 0){
		    if(text_seg != (struct segment_command *)0)
			fatal("More than one segment_command for the %s "
			      "segment in: %s", SEG_TEXT, target);
		    else
			text_seg = (struct segment_command *)lcp;
		}
		break;
	    }
	    sizeofcmds += lcp->cmdsize;
	    if(sizeofcmds > mhp->sizeofcmds)
		fatal("Bad target shared library file: %s (sum of cmdsize's in "
		      "load commands exceeds sizeofcmds in header)", target);
	    lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	}
	/* Check to see if the symbol table load command is good */
	if(stp == (struct symtab_command *)0)
	    fatal("No symtab_command in: %s", target);
	if(stp->nsyms == 0)
	    fatal("No symbol table in: %s", target);
	if(stp->strsize == 0)
	    fatal("No string table in: %s", target);
	if(stp->symoff + stp->nsyms * sizeof(struct nlist) > size)
	    fatal("Bad target shared library file: %s (symbol table would "
		  "extend past the end of the file)", target);
	if(stp->stroff + stp->strsize > size)
	    fatal("Bad target shared library file: %s (string table would "
		  "extend past the end of the file)", target);
	*symbols = (struct nlist *)(addr + stp->symoff);
	*nsyms = stp->nsyms;
	*strings = (char *)(addr + stp->stroff);
	*strsize = stp->strsize;

	for(i = 0; i < *nsyms; i++){
	    if(((*symbols)[i].n_type & N_EXT) != N_EXT)
		continue;
	    if((*symbols)[i].n_un.n_strx < 0 ||
	       (*symbols)[i].n_un.n_strx > *strsize)
		fatal("Bad string table index (%d) for symbol %d in: %s", 
	    	      (*symbols)[i].n_un.n_strx, i, target);
	    (*symbols)[i].n_un.n_name = *strings + (*symbols)[i].n_un.n_strx;
	}

	/* Check to see if the load command for the text segment is good */
	if(text_seg == (struct segment_command *)0)
	    fatal("No segment_command in: %s for the %s segment", target,
		  SEG_TEXT);
	if(text_seg->fileoff + text_seg->filesize > size)
	    fatal("Bad target shared library file: %s (text segment would "
		  "extend past the end of the file)", target);
	*text = (char *)(addr + text_seg->fileoff);
	*textaddr = text_seg->vmaddr;
	*textsize = text_seg->filesize;
}

/*
 * Build a sorted list of external text symbols.
 */
static
void
make_sorted_text_symbols(
struct nlist *symbols,
long nsyms,
struct nlist **text_symbols,
long *ntext_symbols)
{
    long i, j;

	*ntext_symbols = 0;
	for(i = 0; i < nsyms; i++){
	    if((symbols[i].n_type == (N_SECT | N_EXT) && 
	        symbols[i].n_sect == 1))
		(*ntext_symbols)++;
	}

	/* Build a table of the external text symbols sorted by their address */
	*text_symbols = xmalloc(*ntext_symbols * sizeof(struct nlist));
	j = 0;
	for(i = 0; i < nsyms; i++){
	    if((symbols[i].n_type & N_EXT) != N_EXT)
		continue;
	    if((symbols[i].n_type & N_TYPE) == N_SECT && 
	       symbols[i].n_sect == 1)
		(*text_symbols)[j++] = symbols[i];
	}
	qsort(*text_symbols, *ntext_symbols, sizeof(struct nlist),
	      (int (*)(const void *, const void *))cmp_qsort);
}

/*
 * Build a list of external symbols.
 */
static
void
make_global_symbols(
struct nlist *symbols,
long nsyms,
struct nlist **global_symbols,
long *nglobal_symbols)
{
    long i, j;

	*nglobal_symbols = 0;
	for(i = 0; i < nsyms; i++){
	    if((symbols[i].n_type & N_EXT) == N_EXT)
		(*nglobal_symbols)++;
	}

	/* Build a table of the external symbols */
	*global_symbols = xmalloc(*nglobal_symbols * sizeof(struct nlist));
	j = 0;
	for(i = 0; i < nsyms; i++){
	    if((symbols[i].n_type & N_EXT) == N_EXT)
		(*global_symbols)[j++] = symbols[i];
	}
}

/*
 * Return the target address of the jmp instruction in the text for the given
 * value.
 */
static
long
get_target_addr(
long value,
char *name,
char *text,
long addr,
long size,
char *target)
{
    long offset;
    unsigned short jmp;

	offset = value - addr;
	if(offset < 0 || offset > size)
	    fatal("Value (0x%x) of branch table symbol: %s not in "
		  "the %s segment of: %s", value, name, SEG_TEXT, target);
	if(offset + sizeof(short) + sizeof(long) > size)
	    fatal("Branch instruction for branch table symbol: %s "
		  "would extend past the end of the %s segment in: "
		  " %s", name, SEG_TEXT, target);
	jmp = *(unsigned short *)(text + offset);
	if(jmp != 0x4ef9)
	    fatal("Branch instruction not found at branch table "
		  "symbol: %s in: %s", name, target);
	return(*(long *)(text + offset + sizeof(unsigned short)));
}

/*
 * Function for qsort for comparing symbols.
 */
static
int
cmp_qsort(
struct nlist *sym1,
struct nlist *sym2)
{
	return(sym1->n_value - sym2->n_value);
}

/*
 * Function for bsearch for finding a symbol.
 */
static
int
cmp_bsearch(
long *value,
struct nlist *sym)
{
	return(*value - sym->n_value);
}

/*
 * Lookup a name in the external hash table.
 */
static
struct ext *
lookup(
char *name)
{
    long hash_key;
    struct ext *extp;

	hash_key = hash_string(name) % EXT_HASH_SIZE;
	extp = ext_hash[hash_key];
	while(extp != (struct ext *)0){
	    if(strcmp(name, extp->name) == 0)
		return(extp);
	    extp = extp->next;
	}
	return((struct ext *)0);
}


/*
 * a dummy routine called by the fatal() like routines
 */
void
cleanup(
int sig)
{
	exit(EXIT_FAILURE);
}

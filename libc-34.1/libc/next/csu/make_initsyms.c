/*
 * This program makes an object file that has the two symbols to get to the
 * start and end of the shared library initialization section.  When there is
 * a .section directive this program and the object file it creates will
 * go away.  When .section exist then the following will be in crt0.s:
 * 
 *		.section __TEXT,__fvmlib_init0
 * 		.globl __fvmlib_init0
 *	__fvmlib_init0:
 *		.section __TEXT,__fvmlib_init1
 * 		.globl __fvmlib_init1
 *	__fvmlib_init1:
 * 
 * These symbols are used by the routine _init_shlibs() in init_shlib.c that is
 * linked (along with object file created by this program) with crt0.o to form
 * the final crt0.o .  The program mkshlib(l) produces the shared library
 * initialization data in the first of the two sections and the code in
 * _init_shlibs() uses the addresses of the two symbols to bound the data.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/loader.h>
#include <nlist.h>

/*
 * This is the name of the object file this program creates and the names of
 * the two symbols in the object file.
 */
static char *filename = "initsyms.o";
static char *fvmlib_init0 = "__fvmlib_init0";
static char *fvmlib_init1 = "__fvmlib_init1";

static char *progname;

static long round(long v, unsigned long r);
static void *allocate(long size);
static void system_fatal(const char *format, ...);

void
main(
int argc,
char **argv,
char **envp)
{
    int fd;
    char *buffer;
    long object_size, string_table_size, offset;
    struct mach_header *mh;
    struct segment_command *sg;
    struct section *s;
    struct symtab_command *st;
    struct nlist *symbol;

	/*
	 * This object file is made up of the following:
	 *	mach header
	 * 	one LC_SEGMENT
	 * 	    a section header for the "__FVMLIB_INIT0" section
	 * 	    a section header for the "__FVMLIB_INIT1" section
	 *	an LC_SYMTAB command
	 *	the symbol table
	 *	the string table
	 */
	progname = argv[0];

	if((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
	    system_fatal("can't create file: %s", filename);

	string_table_size = 1 +
			    strlen(fvmlib_init0) + 1 +
			    strlen(fvmlib_init1) + 1;
	string_table_size = round(string_table_size, sizeof(long));

	object_size = sizeof (struct mach_header) +
		      sizeof(struct segment_command) +
		      2 * sizeof(struct section) +
		      sizeof(struct symtab_command) +
		      2 * sizeof(struct nlist) +
		      string_table_size;

	buffer = allocate(object_size);
	memset(buffer, '\0', object_size);

	offset = 0;
	mh = (struct mach_header *)(buffer + offset);
	mh->magic = MH_MAGIC;
	mh->cputype = CPU_TYPE_MC680x0;
	mh->cpusubtype = CPU_SUBTYPE_MC68030;
	mh->filetype = MH_OBJECT;
	mh->ncmds = 2;
	mh->sizeofcmds = sizeof(struct segment_command) +
			 2 * sizeof(struct section) +
			 sizeof(struct symtab_command);
	mh->flags = 0;
	offset += sizeof(struct mach_header);

	sg = (struct segment_command *)(buffer + offset);
	sg->cmd = LC_SEGMENT;
	sg->cmdsize = sizeof(struct segment_command) +
		      2 * sizeof(struct section);
	memset(sg->segname, '\0', sizeof(sg->segname));
	sg->vmaddr = 0;
	sg->vmsize = 0;
	sg->fileoff = sizeof(struct mach_header) + mh->sizeofcmds;
	sg->filesize = 0;
	sg->maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
	sg->initprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
	sg->nsects = 2;
	sg->flags = 0;
	offset += sizeof(struct segment_command);

	s = (struct section *)(buffer + offset);
	strcpy(s->sectname, SECT_FVMLIB_INIT0);
	strcpy(s->segname, SEG_TEXT);
	s->addr = 0;
	s->size = 0;
	s->offset = 0;
	s->align = 2;
	s->reloff = 0;
	s->nreloc = 0;
	s->flags = 0;
	s->reserved1 = 0;
	s->reserved2 = 0;
	offset += sizeof(struct section);

	s = (struct section *)(buffer + offset);
	strcpy(s->sectname, SECT_FVMLIB_INIT1);
	strcpy(s->segname, SEG_TEXT);
	s->addr = 0;
	s->size = 0;
	s->offset = 0;
	s->align = 2;
	s->reloff = 0;
	s->nreloc = 0;
	s->flags = 0;
	s->reserved1 = 0;
	s->reserved2 = 0;
	offset += sizeof(struct section);

	st = (struct symtab_command *)(buffer + offset);
	st->cmd = LC_SYMTAB;
	st->cmdsize = sizeof(struct symtab_command);
	st->symoff = sizeof(struct mach_header) +
		     mh->sizeofcmds;
	st->nsyms = 2;
	st->stroff = sizeof(struct mach_header) +
		     mh->sizeofcmds +
		     2 * sizeof(struct nlist);
	st->strsize = string_table_size;
	offset += sizeof(struct symtab_command);

	symbol = (struct nlist *)(buffer + offset);
	symbol->n_un.n_strx = 1;
	symbol->n_type = N_SECT | N_EXT;
	symbol->n_sect = 1;
	symbol->n_desc = 0;
	symbol->n_value = 0;
	offset += sizeof(struct nlist);

	symbol = (struct nlist *)(buffer + offset);
	symbol->n_un.n_strx = 1 + strlen(fvmlib_init0) + 1;
	symbol->n_type = N_SECT | N_EXT;
	symbol->n_sect = 2;
	symbol->n_desc = 0;
	symbol->n_value = 0;
	offset += sizeof(struct nlist);

	offset += 1;
	strcpy(buffer + offset, fvmlib_init0);
	offset += strlen(fvmlib_init0) + 1;
	strcpy(buffer + offset, fvmlib_init1);
	offset += strlen(fvmlib_init1) + 1;

	if(write(fd, buffer, object_size) != object_size)
	    system_fatal("can't write file: %s", filename);
	if(close(fd) == -1)
	    system_fatal("can't close file: %s", filename);
	free(buffer);
	exit(0);
}

/*
 * round() rounds v to a multiple of r.
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
 * allocate() is just a wrapper around malloc that prints and error message and
 * exits if the malloc fails.
 */
static
void *
allocate(
long size)
{
    void *p;

	if((p = malloc(size)) == NULL)
	    system_fatal("virtual memory exhausted (malloc failed)");
	return(p);
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

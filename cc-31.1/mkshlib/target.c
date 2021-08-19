#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/loader.h>
#include <reloc.h>
#include <mach.h>
#include <libc.h>
#include "mkshlib.h"
#include "errors.h"

extern char *mktemp();

/*
 * These are the names of the temporary files used for the branch table
 * assembly language source and resulting object.
 */
char *branch_source_filename;
char *branch_object_filename;
/*
 * This is the name of the temporary file for the library idenification
 * object file.
 */
char *lib_id_object_filename;

static void build_branch_object(void);
static void build_lib_id_object(void);

/*
 * Build the target shared library.
 */
void
target(void)
{
    char text_addr_string[10], data_addr_string[10];
    struct alias *ap;
    struct oddball *obp;
    long i;

	/*
	 * Build the branch table object from the branch table data structrure
	 * created from the #branch directive in the specification file.
	 */
	build_branch_object();

	/*
	 * Build the library identification object.
	 */
	build_lib_id_object();

	/*
	 * Check to see if all the objects listed exist and are readable
	 */
	for(i = 0; i < nobject_list; i++){
	    if(access(object_list[i]->name, R_OK) == -1)
		perror_error("can't read object file: %s",
			     object_list[i]->name);
	}
	if(errors)
	    cleanup(0);

	/*
	 * Link the branch table object will all the objects in the object data
	 * structure created from the #objects directive in the specification
	 * file, at the addresses specified in there creating the target shared
	 * library.
	 */
	reset_runlist();
	add_runlist(ldname);
	add_runlist("-fvmlib");
	for(i = 0; i < ODDBALL_HASH_SIZE; i++){
	    obp = oddball_hash[i];
	    while(obp != (struct oddball *)0){
		if(obp->undefined){
		    add_runlist("-U");
		    add_runlist(obp->name);
		}
		obp = obp->next;
	    }
	}
	ap = aliases;
	while(ap != (struct alias *)0){
	    add_runlist(makestr("-i", ap->alias_name, ":", ap->real_name,
			(char *)0) );
	    ap = ap->next;
	}
	add_runlist("-segaddr");
	add_runlist(SEG_TEXT);
	sprintf(text_addr_string, "%x", text_addr);
	add_runlist(text_addr_string);
	add_runlist("-segaddr");
	add_runlist(SEG_DATA);
	sprintf(data_addr_string, "%x", data_addr);
	add_runlist(data_addr_string);

	for(i = 0; i < nldflags; i++)
	    add_runlist(ldflags[i]);

	add_runlist("-o");
	add_runlist(target_filename);
	add_runlist(branch_object_filename);
	add_runlist(lib_id_object_filename);
	for(i = 0; i < nobject_list; i++)
	    add_runlist(object_list[i]->name);

	if(run())
	    fatal("internal link edit using: %s failed", ldname);

	if(!dflag)
	    unlink(lib_id_object_filename);
}

/*
 * Build the branch table object by writing an assembly source file and
 * assembling it.  The assembly source has the form:
 *		.text
 *		.globl <slot name>
 *	<slot name>:	jmp	<symbol name>
 * Where <slot name> is some name (out of the name space for high level
 * languages) unique to that slot number and <symbol name> is the name of
 * symbol in that branch table slot.
 */
static
void
build_branch_object(void)
{
    long i;
    FILE *stream;
    char *p;
    struct shlib_info shlib;

	branch_source_filename = "branch.s";
	branch_object_filename = "branch.o";

	(void)unlink(branch_source_filename);
	if((stream = fopen(branch_source_filename, "w")) == NULL)
	    perror_fatal("can't create temporary file: %s for branch table "
			 "assembly source", branch_source_filename);
	if(fprintf(stream, "\t.text\n") == EOF)
	    perror_fatal("can't write to temporary file: %s (branch table "
			 "assembly source)", branch_source_filename);
	if(fprintf(stream, "\t.globl %s\n%s:\n", shlib_symbol_name,
		   shlib_symbol_name) == EOF)
	    perror_fatal("can't write to temporary file: %s (branch table "
			 "assembly source)", branch_source_filename);
	if(fprintf(stream, SHLIB_STRUCT, text_addr, 0, data_addr, 0,
		   minor_version, target_name, sizeof(shlib.name) -
		   (strlen(target_name) + 1)) == EOF)
	    perror_fatal("can't write to temporary file: %s (branch table "
			 "assembly source)", branch_source_filename);
	for(i = 0; i < nbranch_list; i++){
	    if(branch_list[i]->empty_slot){
		if(fprintf(stream, "\t.word 0x50fb,0xfead,0xface\n") == EOF)
		    perror_fatal("can't write to temporary file: %s (branch
				 table assembly source)",
				 branch_source_filename);
	    }
	    else{
		p = branch_slot_symbol(i);
		if(fprintf(stream, "\t.globl %s\n%s:\tjmp\t%s\n", p, p,
			   branch_list[i]->name) == EOF)
		    perror_fatal("can't write to temporary file: %s (branch
				 table assembly source)",
				 branch_source_filename);
	    }
	}
	if(fclose(stream) == EOF)
	    perror_fatal("can't close temporary file: %s for branch table "
			 "assembly source", branch_source_filename);

	/*
	 * Assemble the source for the branch table and create the object file.
	 */
	reset_runlist();
	add_runlist(asname);
	add_runlist("-o");
	add_runlist(branch_object_filename);
	add_runlist(branch_source_filename);

	if(run())
	    fatal("internal assembly using: %s failed", asname);
}

/*
 * Build the library identification object.  This is done by directly writing
 * the object file.  This object file just contains the library identification
 * load command.
 */
static
void
build_lib_id_object(void)
{
    int fd;
    long fvmlib_name_size, object_size, offset;
    char *buffer;
    struct mach_header *mh;
    struct fvmlib_command *fvmlib;

	if(dflag){
	    lib_id_object_filename = "lib_id.o";
	}
	else{
	    lib_id_object_filename = mktemp(savestr("mkshlib_lid_XXXXXX"));
	}

	(void)unlink(lib_id_object_filename);
	if((fd = open(lib_id_object_filename, O_WRONLY | O_CREAT | O_TRUNC,
		      0666)) == -1)
	    perror_fatal("can't create file: %s", lib_id_object_filename);


	/*
	 * This file is made up of the following:
	 *	mach header
	 *	an LC_IDFVMLIB command for the library
	 */
	fvmlib_name_size = round(strlen(target_name) + 1, sizeof(long));
	object_size = sizeof(struct mach_header) +
		      sizeof(struct fvmlib_command) +
		      fvmlib_name_size;
	buffer = (char *)xmalloc(object_size);

	offset = 0;
	mh = (struct mach_header *)(buffer + offset);
	mh->magic = MH_MAGIC;
	mh->cputype = CPU_TYPE_MC680x0;
	mh->cpusubtype = CPU_SUBTYPE_MC68030;
	mh->filetype = MH_OBJECT;
	mh->ncmds = 1;
	mh->sizeofcmds = sizeof(struct fvmlib_command) +
			 fvmlib_name_size;
	mh->flags = MH_NOUNDEFS;
	offset += sizeof(struct mach_header);

	/*
	 * The LC_IDFVMLIB command for the library.  This appearing in an
	 * object is what causes the target library to be mapped in when it
	 * is executed.  The link editor collects these into the output file
	 * it builds from the input files.  Since every object in the host
	 * library refers to the library definition symbol defined in here
	 * this object is always linked with anything that uses this library.
	 */
	fvmlib = (struct fvmlib_command *)(buffer + offset);
	fvmlib->cmd = LC_IDFVMLIB;
	fvmlib->cmdsize = sizeof(struct fvmlib_command) +
			  fvmlib_name_size;
	fvmlib->fvmlib.name.offset = sizeof(struct fvmlib_command);
	fvmlib->fvmlib.minor_version = minor_version;
	fvmlib->fvmlib.header_addr = text_addr;
	offset += sizeof(struct fvmlib_command);
	strcpy(buffer + offset, target_name);
	offset += fvmlib_name_size;

	if(write(fd, buffer, object_size) != object_size)
	    perror_fatal("can't write file: %s", lib_id_object_filename);
	if(close(fd) == -1)
	    perror_fatal("can't close file: %s", lib_id_object_filename);
	free(buffer);
}

/*
 * The host_info() call will be changing (incompatably) for the 2.0 release.
 * For now this works using the old host_info() call.  In the next release
 * after Impulse 0.02 the new interface will take effect BUT WON'T WORK.  The
 * old interface will become xxx_host_info() and this code will change in here.
 */
#undef host_info_BROKEN
#undef Impulse0_02

/*
 * The program ebadexec(1) which trys to determine why an executable file will
 * not execute and gets the wounderful "Bad executable (or shared library)"
 * error from the kernel.
 *
 * WARNING:  This program was not written by someone who understands the kernel
 * but only by a some what informed user.  The documention of the error cases
 * for mach system calls the kernel uses to execute a file is limited to
 * non-existant so this program is a best guess some times.  Also the mach
 * kernel has been severly buggy in this area from the time the kernel
 * code was written (or at least does not conform to the comments in
 * <sys/loader.h> or do anything reasonable in obvious error cases like
 * truncated files).  So this program may report that the executable appears to
 * be executable when it will not run or the kernel loads it wrong and it core
 * dumps.  There appears to be no hope in getting the kernel code right and that
 * is why this program was written.  This program's author is the author of the
 * original full Mach-O link editor and the Mach-O format used at NeXT, Inc.  So
 * you can assume that the author should be the authority on Mach-O files but
 * that doesn't help when the kernel won't run your program.
 */
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <libc.h>

#include <sys/loader.h>
#include <mach.h>
#ifndef host_info_BROKEN
#include <mach_host.h>
extern int host_self();
#endif host_info_BROKEN
/*
 * These are missing from <mach.h>.
 */
extern char *mach_error_string(kern_return_t);

/*
 * The user and group id information of who is trying to run this executable.
 */
static uid_t uid = 0;
static gid_t gid = 0;
static char *uid_name = NULL;
static char *gid_name = NULL;

/*
 * The machine's cputype and cpusubtype.
 */
static cpu_type_t cputype = 0;
static cpu_subtype_t cpusubtype = 0;

/* General function return types */
enum return_t { FAILURE, SUCCESS };

/* Name of this program as executed (argv[0]) */
static const char *progname = NULL;

/*
 * Indication if any errors were detected, this is set by any call to the
 * error routines and is the incication is the file is in fact executable.
 */
static long errors = 0;

/* Name of executable filename that is being checked */
static const char *executable = NULL;

/* The unix thread of the executable */
struct thread_command *unix_thread = NULL;

/*
 * Types of files and their names for this program.
 */
enum filetype_t { EXECUTABLE, SHARED_LIBRARY, COMMAND_INTERPRETER };
static char *filetype_names[] = {
    "executable",
    "shared library",
    "command interpreter"
};

/*
 * Structure to hold information about the files which are needed for execution.
 */
struct file_t {
    const char	   *filename;	/* name of the file */
    enum filetype_t filetype;	/* type the file should be */
    unsigned long   filesize;	/* the size of the file as returned by stat */
    unsigned long   fileaddr;	/* where in memory this file has been mapped */
};
/*
 * Array of all the files needed for execution an the number of them.
 */
static struct file_t *files = NULL;
static long nfiles = 0;

/*
 * Structure to hold information about the segments which are mapped in for
 * execution.
 */
struct segment_t {
    unsigned long file_index;	/* index into the files array for which file */
				/*  this segment is from */
    const struct		/* a pointer to the segment command for this */
	segment_command *sg;	/*  segment */
    unsigned long kern_vmaddr;	/* the address the kernel will put this */
    unsigned long kern_vmsize;	/* the size the kernel will use for this */
};
/*
 * Array of all the segments needed for execution an the number of them.
 */
static struct segment_t *segments = NULL;
static long nsegments = 0;

/*
 * The routines declared in this file.
 */
static enum return_t check_file(
    const char *filename,
    const enum filetype_t filetype,
    const struct fvmlib_command *fl_load);

static const char *file_system_type_of_file(
    const struct stat *st);

static long new_file(
    const char *filename,
    const enum filetype_t filetype,
    const long filesize,
    const long fileaddr);

static enum return_t check_command_interpreter(
    long file_index);

static void check_Mach_O(
    const long file_index,
    const struct fvmlib_command *fl_load);

static enum return_t check_fvmlib_command(
    const struct file_t *file,
    const struct fvmlib_command *fl,
    const long cmd_num);

static struct segment_t *new_segment(
    const long file_index,
    const struct segment_command *sg);

static void add_stack_segment(
    void);

static void process_segments(
    void);

static void check_overlap(
    const struct segment_t *segment1,
    const struct segment_t *segment2);

static enum return_t check_max_init_prot(
    const vm_prot_t maxprot,
    const vm_prot_t initprot);

static void check_entry_point(
    void);

static long round(
    long v,
    unsigned long r);

static long trunc(
    long v,
    unsigned long r);

static void *allocate(
    const long size);

static void *reallocate(
    void *p,
    const long size);

static void vprint(
    const char *format,
    va_list ap);

static void print(
    const char *format,
    ...);

static void warning(
    const char *format,
    ...);

static void error(
    const char *format,
    ...);

static void warning_with_file(
    const struct file_t *file,
    const char *format,
    ...);

static void error_with_file(
    const struct file_t *file,
    const char *format,
    ...);

static void system_error(
    const char *format,
    ...);

static void system_fatal(
    const char *format,
    ...);

static void mach_fatal(
    const kern_return_t r,
    const char *format,
    ...);

/*
 * The program ebadexec takes one argument which is the path name of a file that
 * is to be determined as to why it is not executable.  It trys to determine
 * why it is not executable.
 */
void
main(
const int argc,
const char *argv[],
const char *envp[])
{
    kern_return_t r;
    struct passwd *pwd;
    struct group *grp;

#ifndef host_info_BROKEN
#ifdef Impulse0_02
    struct machine_info mi;
    struct machine_slot ms;
#else
    struct host_basic_info host_basic_info;
    unsigned int size;
#endif Impulse0_02
#endif !defined(host_info_BROKEN)

	progname = argv[0];

	if(argc != 2){
	    print("Usage: %s <executable filename>\n", progname);
	    exit(EXIT_FAILURE);
	}
	executable = argv[1];

	/* Determine the type of host we are running on */
#ifdef host_info_BROKEN
	cputype = CPU_TYPE_MC680x0;
	cpusubtype = CPU_SUBTYPE_MC68030;
#else
#ifdef Impulse0_02
	if((r = host_info(task_self(), &mi)) != KERN_SUCCESS)
		mach_fatal(r, "host_info() failed (can't get cputype)");
	if(mi.max_cpus <= 0)
		fatal("kernel not configured for any processors (can't get "
		      "cputype)");
	/* this is assuming all slots have the same cputype and cpusubtype */
	if((r = slot_info(task_self(), 0, &ms)) != KERN_SUCCESS)
		mach_fatal(r, "slot_info() failed (can't get cputype)");
	cputype = ms.cpu_type;
	cpusubtype = ms.cpu_subtype;
#else
	size = HOST_BASIC_INFO_COUNT;
	if((r = host_info(host_self(), HOST_BASIC_INFO,
			  (host_info_t)(&host_basic_info),
			  &size)) != KERN_SUCCESS)
		mach_fatal(r, "host_info() failed (can't get cputype)");
	cputype = host_basic_info.cpu_type;
	cpusubtype = host_basic_info.cpu_subtype;
#endif Impulse0_02
#endif host_info_BROKEN

	/* Determine who is trying to run this executable */
	uid = getuid();
	gid = getgid();
	pwd = getpwuid(uid);
	if(pwd == NULL)
	    uid_name = "no name for uid";
	else
	    uid_name = pwd->pw_name;
	grp = getgrgid(gid);
	if(grp == NULL)
	    gid_name = "no name for gid";
	else
	    gid_name = grp->gr_name;

	/* Now check the executable file */
	if(check_file(executable, EXECUTABLE, NULL) == SUCCESS){

	    /* add the segment for the unix stack */
	    add_stack_segment();

	    process_segments();

	    /* make sure there was a unix thread found in the executable */
	    if(unix_thread == NULL)
		error("executable file: %s does not contain a LC_UNIXTHREAD "
		      "command", executable);
	    else
		/* check to see the pc in the thread in is some segment */
		check_entry_point();
	}

	if(errors == 0){
	    print("%s: file: %s appears to be executable\n", progname,
		  executable);
	    exit(EXIT_SUCCESS);
	}
	else
	    exit(EXIT_FAILURE);
}

/*
 * Check the filename passed to it to see if the file system aspects
 * (protection, etc.) are correct for execution.  The return status only
 * indicates if futher checks can proceed (not that the file executable
 * which is indicated by the value 'errors').
 */
static
enum return_t
check_file(
const char *filename,
const enum filetype_t filetype,
const struct fvmlib_command *fl_load)
{
    struct stat statbuf;
    long filesize, fileaddr;
    kern_return_t r;
    int fd;
    long file_index;

	/* see if the file exists at all */
	if(access(filename, F_OK) == -1){
	    system_error("%s file: %s does not exist", filetype_names[filetype],
			 filename);
	    return(FAILURE);
	}

	/* see if this user can execute the file */
	if(access(filename, X_OK) == -1){
	    system_error("%s file: %s does not have execute permission for "
			 "user id (%s %d) and group id (%s %d)",
			 filetype_names[filetype], filename, uid_name, uid,
			 gid_name, gid);
	    return(FAILURE);
	}

	/* see if this user can read the file so further checks can be done */
	if(access(filename, R_OK) == -1){
	    system_error("further checks on %s file: %s are not possible "
			 "because user id (%s %d) and group id (%s %d) does "
			 "not have read permission", filetype_names[filetype],
			 filename, uid_name, uid, gid_name, gid);
	    return(FAILURE);
	}

	/* see if this is a regular file, not a directory or something */
	if(stat(filename, &statbuf) == -1)
	    system_fatal("can't stat %s file: %s", filetype_names[filetype],
			 filename);
	if((statbuf.st_mode & S_IFMT) != S_IFREG){
	    error("%s file: %s is a %s file and not a regular file",
		  filetype_names[filetype], filename,
		  file_system_type_of_file(&statbuf));
	    return(FAILURE);
	}

	/* now that the file should be able to be read open it and map it in */
	if((fd = open(filename, O_RDONLY)) == -1)
	    system_fatal("can't open %s file: %s", filetype_names[filetype],
			 filename);
	filesize = statbuf.st_size;
	if((r = map_fd(fd, 0, &fileaddr, TRUE, filesize)) != KERN_SUCCESS)
	    mach_fatal(r, "can't map %s file: %s",filetype_names[filetype],
		       filename);
	close(fd);
	file_index = new_file(filename, filetype, filesize, fileaddr);

	/*
	 * If this is the executable passed to main() and the size is as large
	 * as the minimum size for a command interpreter file (!#<space or tab>
	 * <command interpreter file name>) and starts with "#!" process it as
	 * a command interpreter file.  Otherwise it should be a Mach-O file.
	 */
	if(filename == executable &&
	   filesize >= 2 && strncmp((char *)fileaddr, "#!", 2) == 0){
	    return(check_command_interpreter(file_index));
	}
	else{
	    check_Mach_O(file_index, fl_load);
	    return(SUCCESS);
	}
}

/*
 * file_system_type_of_file() returns a string used for printing the type of
 * a file as viewed by the file system for the stat structure passed to it.
 */
static
const
char *
file_system_type_of_file(
const struct stat *st)
{
	switch(st->st_mode & S_IFMT){
	case S_IFDIR:
		return("directory");
	case S_IFCHR:
		return("character special");
	case S_IFBLK:
		return("block special");
	case S_IFREG:
		return("regular");
	default:
		return("unknown");
	}
}

/*
 * Create a new file structure for the specified file
 */
static
long
new_file(
const char *filename,
const enum filetype_t filetype,
const long filesize,
const long fileaddr)
{
    struct file_t *file;
    long file_index;

	files = reallocate(files, (nfiles + 1) * sizeof(struct file_t));
	file = &(files[nfiles]);
	file_index = nfiles;
	nfiles++;

	memset(file, '\0', sizeof(struct file_t));
	file->filename = filename;
	file->filetype = filetype;
	file->filesize = filesize;
	file->fileaddr = fileaddr;
	return(file_index);
}

/*
 * check_command_interpreter() checks the syntax of the command interpreter
 * line of a file that has been determined to start with "#!".  The maximum
 * length of the line is 32 bytes (coming from the size of the exec structure
 * for a.out files as the number of bytes the kernel reads out of the file
 * to process a #! file).  A space or a tab must follow the #! then the name
 * of the command interpreter file and an optional single prameter (which is
 * not checked for here).
 */
static
enum return_t
check_command_interpreter(
long file_index)
{
    struct file_t *file;
    long i;
    char c, *buf, *command_interpreter;
    const long MAX_COMMAND_INTERPRETER = 32;

	
	file = &(files[file_index]);

	buf = allocate(MAX_COMMAND_INTERPRETER + 1);
	memset(buf, '\0', MAX_COMMAND_INTERPRETER + 1);
	memcpy(buf, (char *)file->fileaddr,
	       file->filesize < MAX_COMMAND_INTERPRETER ?
	       file->filesize : MAX_COMMAND_INTERPRETER);

	if(buf[2] != ' ' && buf[2] != '\t'){
	    warning_with_file(file, "missing space or tab after #! before name "
			      "of command interpreter");
	}
	if(buf[2] == '\n'){
	    error_with_file(file, "missing name of command interpreter after "
			    "#! in file");
	    return(FAILURE);
	}
	i = 2;
	while(i < file->filesize && i < MAX_COMMAND_INTERPRETER){
	    c = buf[i];
	    if(c != ' ' && c != '\t')
		break;
	    i++;
	}
	if(i == file->filesize || i == MAX_COMMAND_INTERPRETER){
	    error_with_file(file, "name of command interpreter not found in "
			    "the first %d characters of the file",
			    MAX_COMMAND_INTERPRETER);
	    return(FAILURE);
	}
	command_interpreter = &(buf[i]);
	while(i < file->filesize && i < MAX_COMMAND_INTERPRETER){
	    c = buf[i];
	    if(c == ' ' || c == '\t' || c == '\n')
		break;
	    i++;
	}
	buf[i] = '\0';

	if(i == MAX_COMMAND_INTERPRETER && 
	   file->filesize > MAX_COMMAND_INTERPRETER){
	    c = ((char *)file->fileaddr)[i];
	    if(c != ' ' && c != '\t' && c != '\n')
		warning_with_file(file, "name of command interpreter (%s) not "
				  "terminated by a white space character in "
				  "the first %d characters of the file",
				  command_interpreter, MAX_COMMAND_INTERPRETER);
	}
	return(check_file(command_interpreter, COMMAND_INTERPRETER, NULL));
}

/*
 * check_Mach_O() checks to see if the files[file_index] is really an object
 * file and that all the offset and sizes in the headers are within the memory
 * the object file is mapped in.  If this file is a shared library the fl_load
 * is the LC_LOADFVMLIB command that caused it to be loaded.
 */
static
void
check_Mach_O(
const long file_index,
const struct fvmlib_command *fl_load)
{
    long i;
    struct file_t *file;
    struct mach_header *mh;
    struct load_command *lc, *load_commands;
    struct segment_command *sg;
    struct symtab_command *st;
    struct fvmlib_command *fl;
    char *load_fvmlib_name, *id_fvmlib_name;

	file = &(files[file_index]);

	/* check to see the mach_header is valid */
	if(sizeof(struct mach_header) > file->filesize){
	    error_with_file(file, "truncated or malformed object (mach header "
			    "extends past the end of the file)");
	    return;
	}
	mh = (struct mach_header *)file->fileaddr;
	if(mh->magic != MH_MAGIC){
	    error_with_file(file, "bad magic number (not a Mach-O file)");
	    return;
	}

	/*
	 * See if the cputype and cpusubtype of this object can run on this
	 * machine.
	 */
	if(mh->cputype != cputype)
		error_with_file(file, "cputype's (%d) does not match machine's "
				"cputype (%d)", mh->cputype, cputype);
	if(mh->cpusubtype > cpusubtype)
		error_with_file(file, "cpusubtype's (%d) is not less than or "
				"equal to the machine's cpusubtype (%d)",
				mh->cpusubtype, cpusubtype);

	/*
	 * Check the object file's type, issue a warning only if not right
	 * because I don't think the kernel will check this when release 2.0
	 * goes out the door.
	 */
	if(file->filetype == SHARED_LIBRARY){
	    if(mh->filetype != MH_FVMLIB)
		warning_with_file(file, "object file is not MH_FVMLIB (which "
				  "it should be for shared library");
	}
	else{
	    if(mh->filetype == MH_FVMLIB)
		warning_with_file(file, "object file is MH_FVMLIB (which "
				  "it should not be for an executable");
	    /*
	     * The kernel may not execute files that have filetypes other than
	     * MH_EXECUTE or MH_CORE.  This should be completely ignored by the
	     * the kernel by release 2.0 but may cause it not to be executed.
	     */
	    if(mh->filetype != MH_EXECUTE && mh->filetype != MH_CORE)
		warning_with_file(file, "object file is not MH_EXECUTE or "
				  "MH_CORE (it may not execute)");
	}

	/* make sure the load commands are in the file */
	if(mh->sizeofcmds + sizeof(struct mach_header) > file->filesize){
	    error_with_file(file, "truncated or malformed object (load "
			    "commands extend past the end of the file)");
	    return;
	}

	/* check to see that the load commands are valid */
	load_commands = (struct load_command *)((char *)file->fileaddr +
			    sizeof(struct mach_header));
	st = NULL;
	lc = load_commands;
	for(i = 0; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		warning_with_file(file, "load command %d size not a multiple "
				  "of sizeof(long)", i);
	    if(lc->cmdsize <= 0){
		error_with_file(file, "load command %d size is less than or "
				"equal to zero", i);
		return;
	    }
	    if((char *)lc + lc->cmdsize >
	       (char *)load_commands + mh->sizeofcmds){
		error_with_file(file, "load command %d extends past end of all "
				"load commands", i);
		return;
	    }
	    switch(lc->cmd){
	    case LC_SEGMENT:
		sg = (struct segment_command *)lc;
		if(sg->cmdsize != sizeof(struct segment_command) +
				     sg->nsects * sizeof(struct section)){
		    error_with_file(file, "cmdsize field of load command %d is "
				    "inconsistant for a segment command with "
				    "the number of sections it has", i);
		}
		if(sg->filesize != 0){
		    if(sg->fileoff > file->filesize){
			error_with_file(file, "fileoff in load command %d "
					"extends past the end of the file", i);
			break;
		    }
		    if(sg->fileoff + sg->filesize > file->filesize){
			error_with_file(file, "fileoff plus filesize in load "
					"command %d extends past the end of "
					"the file", i);
			break;
		    }
		}
		/* add the segment */
		new_segment(file_index, sg);
		break;

	    case LC_IDFVMLIB:
		if(file->filetype != SHARED_LIBRARY){
		    error_with_file(file, "LC_IDFVMLIB load command found in "
				    "it (should not appear in an executable "
				    "file)");
		    break;
		}
		fl = (struct fvmlib_command *)lc;
		if(check_fvmlib_command(file, fl, i) == FAILURE)
		    break;
		id_fvmlib_name = (char *)fl + fl->fvmlib.name.offset;
		load_fvmlib_name =(char *)fl_load + fl_load->fvmlib.name.offset;
		if(strcmp(load_fvmlib_name, id_fvmlib_name) != 0)
		    warning("library name's in executable's LC_LOADFVMLIB "
			    "command (%s) does not match shared library's "
			    "LC_IDFVMLIB command (%s)", load_fvmlib_name,
			    id_fvmlib_name);
		/*
		 * Check the id's minor_version against the load's
		 * minor_version.  For a program to execute the it's minor
		 * number can't be greater that the minor number of the library
		 * that will be used.
		 */
		if(fl_load->fvmlib.minor_version > fl->fvmlib.minor_version)
		    error("executable's minor version (%d) for shared library: "
			  "%s is greater than shared library's minor version "
			  "(%d)", fl_load->fvmlib.minor_version,
			  load_fvmlib_name, fl->fvmlib.minor_version);
		break;

	    case LC_LOADFVMLIB:
		if(file->filetype == SHARED_LIBRARY){
		    error_with_file(file, "LC_LOADFVMLIB load command found in "
				    "it (should not appear in a shared "
				    "library)");
		    break;
		}
		fl = (struct fvmlib_command *)lc;
		if(check_fvmlib_command(file, fl, i) == FAILURE)
		    break;
		load_fvmlib_name = (char *)fl + fl->fvmlib.name.offset;
		/* process the shared library file */
		check_file(load_fvmlib_name, SHARED_LIBRARY, fl);
		break;

	    case LC_UNIXTHREAD:
		if(file->filetype == SHARED_LIBRARY){
		    error_with_file(file, "LC_UNIXTHREAD load command found in "
				    "it (should not appear in a shared "
				    "library)");
		    break;
		}
		/* should see exactly one of these for the executable */
		if(unix_thread != NULL){
		    error_with_file(file, "more than one LC_UNIXTHREAD command "
				    "found in it (should contain exactly one)");
		    break;
		}
		unix_thread = (struct thread_command *)lc;
		break;

	    default:
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * check_fvmlib_command() checks the LC_LOADFVMLIB or LC_IDFVMLIB command to
 * see if it is valid.
 */
static
enum return_t
check_fvmlib_command(
const struct file_t *file,
const struct fvmlib_command *fl,
const long cmd_num)
{
    char *fvmlib_name;
    long i;

	if(fl->cmdsize < sizeof(struct fvmlib_command)){
	    error_with_file(file, "cmdsize of load command %d incorrect for "
			    "%s", cmd_num, fl->cmd == LC_LOADFVMLIB ?
			    "LC_LOADFVMLIB" : "LC_IDFVMLIB");
	    return(FAILURE);
	}
	if(fl->fvmlib.name.offset >= fl->cmdsize){
	    error_with_file(file, "name.offset of load command %d extends past "
			    "the end of the load command", cmd_num); 
	    return(FAILURE);
	}
	fvmlib_name = (char *)fl + fl->fvmlib.name.offset;
	for(i = 0 ; i < fl->cmdsize - fl->fvmlib.name.offset; i++){
	    if(fvmlib_name[i] == '\0')
		break;
	}
	if(i >= fl->cmdsize - fl->fvmlib.name.offset){
	    error_with_file(file, "library name of load command %d not null "
			    "terminated", cmd_num);
	    return(FAILURE);
	}
	return(SUCCESS);
}

/*
 * Create a new segment structure for the specified segment.
 */
static
struct segment_t *
new_segment(
const long file_index,
const struct segment_command *sg)
{
    struct segment_t *segment;

	segments = reallocate(segments,
			      (nsegments + 1) * sizeof(struct segment_t));
	segment = &(segments[nsegments]);
	nsegments++;

	memset(segment, '\0', sizeof(struct segment_t));
	segment->file_index = file_index;
	segment->sg = sg;
	return(segment);
}

/*
 * add_stack_segment() adds a segment representing the stack for the executable.
 * This is done by determining the stack for this program and creating a stack
 * segment from it rather than having a magic constant for the stack address and
 * doing a getrlimit(2) for the size.
 */
static
void
add_stack_segment(
void)
{
    long stack;

    kern_return_t r;
    vm_address_t address;
    vm_size_t size;
    vm_prot_t protection, max_protection;
    vm_inherit_t inheritence;
    boolean_t shared;
    port_t object_name;
    vm_offset_t offset;

    static struct segment_command stack_segment = { 0 };

	address = (vm_address_t)&stack;
	if((r = vm_region(task_self(), &address, &size, &protection,
			  &max_protection, &inheritence, &shared, &object_name,
			  &offset)) != KERN_SUCCESS)
	    mach_fatal(r, "can't determine stack region");

	stack_segment.cmd = LC_SEGMENT;
	stack_segment.cmdsize = sizeof(struct segment_command);
	strcpy(stack_segment.segname, "UNIX STACK");
	stack_segment.vmaddr = address;
	stack_segment.vmsize = size;
	stack_segment.fileoff = 0;
	stack_segment.filesize = 0;
	stack_segment.maxprot = max_protection;
	stack_segment.initprot = protection;
	stack_segment.nsects = 0;
	stack_segment.flags = 0;
	new_segment(0, &stack_segment);
}

/*
 * process_segments() checks to see if all the segments are valid and can all
 * be mapped in without overlapping.  How this is really done in the kernel is
 * a mystery as there is little or no documentation on how the edge conditions
 * of vm_allocate() and map_fd() are suppose to behave.  So this is a best
 * guess.  Maybe some day a kernel person will fix this code to match the
 * kernel but until then this is better than nothing.
 */
static void
process_segments(
void)
{
    long pagesize, i, j;
    struct segment_t *segment, *segment1, *segment2;
    struct file_t *file;

	pagesize = getpagesize();
	for(i = 0; i < nsegments; i++){
	    segment = &(segments[i]);
	    file = &(files[segment->file_index]);
	    if(check_max_init_prot(segment->sg->maxprot,
				   segment->sg->initprot) == FAILURE)
		error_with_file(file, "maximum protection for "
				"segment: %0.16s doesn't include all initial "
				"protections", segment->sg->segname);
		/*
		 * Now this is not as simple as it appears but I think this is
		 * doing what the kernel is doing.  First the vmaddr is
		 * truncated to a pagesize then without reguard to the
		 * difference of the truncated address and the original address
		 * the vmsize is rounded to a pagesize.  This results in not
		 * getting all the address area requested, it will be at least
		 * as big but may fall short of covering all the address area
		 * at the end.  Then when the file is mapped it is mapped
		 * starting at the truncated address and not the original
		 * address.  So the bottom line is that if the original address
		 * is not the same as the truncated address it won't work with
		 * the way I think the kernel works.
		 */
		segment->kern_vmaddr = trunc(segment->sg->vmaddr, pagesize);
		segment->kern_vmsize = round(segment->sg->vmsize, pagesize);
		if(segment->kern_vmaddr != segment->sg->vmaddr)
		    error_with_file(file, "vmaddr (0x%x) for segment: "
				    "%0.16s is not equal to the address (0x%x) "
				    "truncated to the pagesize",
				    segment->sg->vmaddr, segment->sg->segname,
				    segment->kern_vmaddr);

		/*
		 * Now check to see if the filesize will fit in the size that
		 * the kernel will allocate for this segment.
		 */
		if(segment->kern_vmsize < segment->sg->filesize)
		    error_with_file(file, "filesize (%d) for segment: "
				    "%0.16s is greater than the page rounded "
				    "vmsize (%d)", segment->sg->filesize,
				    segment->sg->segname, segment->kern_vmsize);
		else if(segment->sg->vmsize < segment->sg->filesize)
		    warning_with_file(file, "filesize (%d) for "
				      "segment: %0.16s is greater than the "
				      "vmsize (%d)", segment->sg->filesize,
				      segment->sg->segname,segment->sg->vmsize);

		/*
		 * In the 1.0 versions of the kernel the last fraction of a page
		 * does not get zeroed correctly and will cause most programs to
		 * core dump if they expect their bss and common symbols to
		 * acctually have zero values.  Thus a warning for now (but they
		 * say it will be fixed for 2.0).
		 */
		if(segment->sg->filesize + segment->sg->filesize !=
		   round(segment->sg->filesize + segment->sg->filesize,
			 pagesize))
		    warning_with_file(file, "fileoff + filesize (%d) for "
				      "segment: %0.16s is not equal to the "
				      "fileoff + filesize rounded to the "
				      "pagesize (%d) (it may not work)",
				      segment->sg->fileoff +
				      segment->sg->filesize,
				      segment->sg->segname,
				      round(segment->sg->fileoff +
				            segment->sg->filesize, pagesize));
	}

	/* check for overlaping segments */
	for(i = 0; i < nsegments; i++){
	    segment1 = &(segments[i]);
	    for(j = i + 1; j < nsegments; j++){
		segment2 = &(segments[j]);
		check_overlap(segment1, segment2);
	    }
	}
}

/*
 * check_overlap() checks if the two segments passed to it overlap.
 */
static
void
check_overlap(
const struct segment_t *segment1,
const struct segment_t *segment2)
{
    struct file_t *file1, *file2;

	file1 = &(files[segment1->file_index]);
	file2 = &(files[segment2->file_index]);

	if(segment1->kern_vmsize == 0 || segment2->kern_vmsize == 0)
	    return;

	if(segment1->kern_vmaddr > segment2->kern_vmaddr){
	    if(segment2->kern_vmaddr + segment2->kern_vmsize <=
	       segment1->kern_vmaddr)
		return;
	}
	else{
	    if(segment1->kern_vmaddr + segment1->kern_vmsize <=
	       segment2->kern_vmaddr)
		return;
	}
	error("%0.16s segment (truncated address = 0x%x rounded size = 0x%x) "
	      "of %s file: %s overlaps with "
	      "%0.16s segment (truncated address = 0x%x rounded size = 0x%x) "
	      "of %s file: %s",
	      segment1->sg->segname, segment1->kern_vmaddr,
	      segment1->kern_vmsize, filetype_names[file1->filetype],
	      file1->filename,
	      segment2->sg->segname, segment2->kern_vmaddr,
	      segment2->kern_vmsize, filetype_names[file2->filetype],
	      file2->filename);
}

/*
 * check_max_init_prot() checks to make sure that all protections in the initial
 * protection are also in the maximum protection.
 */
static
enum return_t
check_max_init_prot(
const vm_prot_t maxprot,
const vm_prot_t initprot)
{
	if(((initprot & VM_PROT_READ)    && !(maxprot & VM_PROT_READ)) ||
	   ((initprot & VM_PROT_WRITE)   && !(maxprot & VM_PROT_WRITE)) ||
	   ((initprot & VM_PROT_EXECUTE) && !(maxprot & VM_PROT_EXECUTE)) )
	    return(FAILURE);
	return(SUCCESS);
}

/*
 * check_entry_point() checks to see that the unix thread has an entry point and
 * that that entry point is in some segment.
 */
static
void
check_entry_point(
void)
{
    char *p, *state;
    unsigned long flavor, count, i;
    struct NeXT_thread_state_regs *cpu;
    unsigned long entry_point, entry_point_found;

	entry_point = 0;
	entry_point_found = 0;
	state = (char *)unix_thread + sizeof(struct thread_command);
	if(cputype == CPU_TYPE_MC680x0 &&
	   (cpusubtype == CPU_SUBTYPE_MC68030 ||
	    cpusubtype == CPU_SUBTYPE_MC68040)){
	    p = (char *)unix_thread + unix_thread->cmdsize;
	    while(state < p){
		flavor = *((unsigned long *)state);
		state += sizeof(unsigned long);
		count = *((unsigned long *)state);
		state += sizeof(unsigned long);
		switch(flavor){
		case NeXT_THREAD_STATE_REGS:
		    cpu = (struct NeXT_thread_state_regs *)state;
		    if(entry_point_found == 0){
			entry_point = cpu->pc;
			entry_point_found = 1;
		    }
		    else{
			error("executable file: %s has more than one entry "
			      "point in it's LC_UNIXTHREAD command",executable);
			return;
		    }
		    break;
		}
	    }
	}
	else{
	    warning("executable file: %s has cputype and cpusubtype unknown "
		    "to %s and the entry point can't be checked to see if it "
		    "is in a segment", executable, progname);
	    return;
	}

	if(entry_point_found == 0){
	    error("executable file: %s does not contain an entry point in it's "
		  "LC_UNIXTHREAD command", executable);
	    return;
	}

	/*
	 * Now this just checks that the entry point is in any segment.  It does
	 * try to check that the segment is resonable (it is the executable's
	 * __TEXT segment).
	 */
	for(i = 0; i < nsegments; i++){
	    if(entry_point >= segments[i].kern_vmaddr &&
	       entry_point < segments[i].kern_vmaddr + segments[i].kern_vmsize){
		if(files[segments[i].file_index].filename != executable &&
		   strcmp(segments[i].sg->segname, SEG_TEXT))
		    warning("entry point (0x%x) not in executable's " SEG_TEXT
			    "segment", entry_point);
		return;
	    }
	}
	error("executable file: %s has an entry point (0x%x) that is not in "
	      "any segment", executable);
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
 * trunc() truncates the value 'v' to the power of two value 'r'.  If v is
 * less than zero it returns zero.
 */
static
long
trunc(
long v,
unsigned long r)
{
	if(v < 0)
	    return(0);
	return(v & ~(r - 1));
}

/*
 * allocate() is just a wrapper around malloc that prints and error message and
 * exits if the malloc fails.
 */
static
void *
allocate(
const long size)
{
    void *p;

	if((p = malloc(size)) == NULL)
	    system_fatal("virtual memory exhausted (malloc failed)");
	return(p);
}

/*
 * reallocate() is just a wrapper around realloc that prints and error message
 * and exits if the realloc fails.
 */
static
void *
reallocate(
void *p,
const long size)
{
	if((p = realloc(p, size)) == NULL)
	    system_fatal("virtual memory exhausted (realloc failed)");
	return(p);
}

/*
 * All printing of all messages goes through this function.
 */
static
void
vprint(
const char *format,
va_list ap)
{
	vprintf(format, ap);
}

/*
 * The print function that just calls the above vprint() function.
 */
static
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
 * Print the warning message.  This is does not set 'errors'.
 */
static
void
warning(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: warning ", progname);
	vprint(format, ap);
        print("\n");
	va_end(ap);
}

/*
 * Print the error message and set the 'error' indication.
 */
static
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
 * Print the filename and warning message.
 */
static
void
warning_with_file(
const struct file_t *file,
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: warning ", progname);
	print("%s file: %s ", filetype_names[file->filetype], file->filename);
	vprint(format, ap);
        print("\n");
	va_end(ap);
}

/*
 * Print the filename and error message, set the error indication.
 */
static
void
error_with_file(
const struct file_t *file,
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: ", progname);
	print("%s file: %s ", filetype_names[file->filetype], file->filename);
	vprint(format, ap);
        print("\n");
	va_end(ap);
	errors = 1;
}

/*
 * Print the error message along with the system error message, set the
 * error indication.
 */
static
void
system_error(
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: ", progname);
	vprint(format, ap);
	print(" (%s)\n", strerror(errno));
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
        print("%s: ", progname);
	vprint(format, ap);
	print(" (%s)\n", strerror(errno));
	va_end(ap);
	exit(EXIT_FAILURE);
}

/*
 * Print the fatal error message along with the mach error string, and exit
 * non-zero.
 */
static
void
mach_fatal(
const kern_return_t r,
const char *format,
...)
{
    va_list ap;

	va_start(ap, format);
        print("%s: ", progname);
	vprint(format, ap);
	print(" (%s)\n", mach_error_string(r));
	va_end(ap);
	exit(EXIT_FAILURE);
}

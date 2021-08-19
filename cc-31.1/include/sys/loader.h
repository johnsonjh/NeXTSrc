/*
 * This file describes the format of mach object files.
 */

/*
 * <sys/machine.h> is needed here for the cpu_type_t and cpu_subtype_t types
 * and contains the constants for the possible values of these types.
 */
#import <sys/machine.h>

/*
 * <vm/vm_prot.h> is needed here for the vm_prot_t type and contains the 
 * constants that are or'ed together for the possible values of this type.
 */
#import <vm/vm_prot.h>

/*
 * <machine/thread_status.h> is expected to define the flavors of the thread
 * states and the structures of those flavors for each machine.
 */
#import <machine/thread_status.h>

/*
 * The mach header appears at the very beginning of the object file.
 */
struct mach_header {
	unsigned long	magic;		/* mach magic number identifier */
	cpu_type_t	cputype;	/* cpu specifier */
	cpu_subtype_t	cpusubtype;	/* machine specifier */
	unsigned long	filetype;	/* type of file */
	unsigned long	ncmds;		/* number of load commands */
	unsigned long	sizeofcmds;	/* the size of all the load commands */
	unsigned long	flags;		/* flags */
};

/* Constant for the magic field of the mach_header */
#define	MH_MAGIC	0xfeedface	/* the mach magic number */

/*
 * The layout of the file depends on the filetype.  For all but the MH_OBJECT
 * file type the segments are padded out and aligned on a segment alignment
 * boundary for efficient demand pageing.  Both the MH_EXECUTE and the MH_FVMLIB
 * file types also have the headers included as part of their first segment.
 * 
 * The file type MH_OBJECT is a compact format intended as output of the
 * assembler and input (and possibly output) of the link editor (the .o format).
 * All sections are in one unnamed segment with no segment padding.  This format
 * is used as an executable format when the file is so small the segment padding
 * greatly increases it's size.
 *
 * The file type MH_PRELOAD is an executable format intended for things that
 * not executed under the kernel (proms, stand alones, kernels, etc).  The
 * format can be executed under the kernel but may demand paged it and not
 * preload it before execution.
 *
 * A core file is in MH_CORE format and can be any in an arbritray leagal
 * Mach-O file.
 *
 * Constants for the filetype field of the mach_header
 */
#define	MH_OBJECT	0x1		/* relocatable object file */
#define	MH_EXECUTE	0x2		/* demand paged executable file */
#define	MH_FVMLIB	0x3		/* fixed VM shared library file */
#define	MH_CORE		0x4		/* core file */
#define	MH_PRELOAD	0x5		/* preloaded executable file */

/* Constants for the flags field of the mach_header */
#define	MH_NOUNDEFS	0x1		/* the object file has no undefined
					   references, can be executed */
#define	MH_INCRLINK	0x2		/* the object file is the output of an
					   incremental link against a base file
					   and can't be link edited again */

/*
 * The load commands directly follow the mach_header.  The total size of all
 * of the commands is given by the sizeofcmds field in the mach_header.  All
 * load commands must have as their first two fields cmd and cmdsize.  The cmd
 * field is filled in with a constant for that command type.  Each command type
 * has a structure specifically for it.  The cmdsize field is the size in bytes
 * of the particular load command structure plus anything that follows it that
 * is a part of the load command (i.e. section structures, strings, etc.).  To
 * advance to the next load command the cmdsize can be added to the offset or
 * pointer of the current load command.  The cmdsize MUST be a multiple of
 * sizeof(long) (this is forever the maximum alignment of any load commands).
 * The padded bytes must be zero.  All tables in the object file must also
 * follow these rules so the file can be memory mapped.  Otherwise the pointers
 * to these tables will not work well or at all on some machines.  With all
 * padding zeroed like objects will compare byte for byte.
 */
struct load_command {
	unsigned long cmd;		/* type of load command */
	unsigned long cmdsize;		/* total size of command in bytes */
};

/* Constants for the cmd field of all load commands, the type */
#define	LC_SEGMENT	0x1	/* segment of this file to be mapped */
#define	LC_SYMTAB	0x2	/* link-edit stab symbol table info */
#define	LC_SYMSEG	0x3	/* link-edit gdb symbol table info (obsolete) */
#define	LC_THREAD	0x4	/* thread */
#define	LC_UNIXTHREAD	0x5	/* unix thread (includes a stack) */
#define	LC_LOADFVMLIB	0x6	/* load a specified fixed VM shared library */
#define	LC_IDFVMLIB	0x7	/* fixed VM shared library identification */
#define	LC_IDENT	0x8	/* object identification information(obsolete)*/

/*
 * A variable length string in a load command is represented by an lc_str
 * union.  The strings are stored just after the load command structure and
 * the offset is from the start of the load command structure.  The size
 * of the string is reflected in the cmdsize field of the load command.
 * Once again any padded bytes to bring the cmdsize field to a multiple
 * of sizeof(long) must be zero.
 */
union lc_str {
	unsigned long	offset;	/* offset to the string */
	char		*ptr;	/* pointer to the string */
};

/*
 * The segment load command indicates that a part of this file is to be
 * mapped into the task's address space.  The size of this segment in memory,
 * vmsize, maybe equal to or larger than the amount to map from this file,
 * filesize.  The file is mapped starting at fileoff to the beginning of
 * the segment in memory, vmaddr.  The rest of the memory of the segment,
 * if any, is allocated zero fill on demand.  The segment's maximum virtual
 * memory protection and initial virtual memory protection are specified
 * by the maxprot and initprot fields.  If the segment has sections then the
 * section structures directly follow the segment command and their size is
 * reflected in cmdsize.
 */
struct segment_command {
	unsigned long	cmd;		/* LC_SEGMENT */
	unsigned long	cmdsize;	/* includes sizeof section structures */
	char		segname[16];	/* segment name */
	unsigned long	vmaddr;		/* memory address of this segment */
	unsigned long	vmsize;		/* memory size of this segment */
	unsigned long	fileoff;	/* file offset of this segment */
	unsigned long	filesize;	/* amount to map from the file */
	vm_prot_t	maxprot;	/* maximum VM protection */
	vm_prot_t	initprot;	/* initial VM protection */
	unsigned long	nsects;		/* number of sections in segment */
	unsigned long	flags;		/* flags */
};

/* Constants for the flags field of the segment_command */
#define	SG_HIGHVM	0x1	/* the file contents for this segment is for
				   the high part of the VM space, the low part
				   is zero filled (for stacks in core files) */
#define	SG_FVMLIB	0x2	/* this segment is the VM that is allocated by
				   a fixed VM library, for overlap checking in
				   the link editor */
#define	SG_NORELOC	0x4	/* this segment has nothing that was relocated
				   in it and nothing relocated to it, that is
				   it maybe safely replaced without relocation*/

/*
 * A segment is made up of zero or more sections.  Non-MH_OBJECT files have
 * all of their segments with the proper sections in each, and padded to the
 * specified segment alignment when produced by the link editor.  The first
 * segment of a MH_EXECUTE and MH_FVMLIB format file contains the mach_header
 * and load commands of the object file before it's first section.  The zero
 * fill sections are always last in their segment (in all formats).  This allows
 * the zeroed segment padding to be mapped into memory where zero fill sections
 * might be.
 *
 * The MH_OBJECT format has all of it's sections in one segment for compactness.
 * There is no padding to a specified segment boundary and the mach_header and
 * load commands are not part of the segment.
 *
 * Sections with the same section name, sectname, going into the same segment,
 * segname, are combined by the link editor.  The resulting section is aligned
 * to the maximum alignment of the combined sections and is the new section's
 * alignment.  The combined sections are aligned to their original alignment in
 * the combined section.  Any padded bytes to get the specified alignment are
 * zeroed.
 *
 * The format of the relocation entries referenced by the reloff and nreloc
 * fields of the section structure for mach object files is described in the
 * header file <reloc.h>.
 */
struct section {
	char		sectname[16];	/* name of this section */
	char		segname[16];	/* segment this section goes in */
	unsigned long	addr;		/* memory address of this section */
	unsigned long	size;		/* size in bytes of this section */
	unsigned long	offset;		/* file offset of this section */
	unsigned long	align;		/* section alignment (power of 2)*/
	unsigned long	reloff;		/* file offset of relocation entries */
	unsigned long	nreloc;		/* number of relocation entries */
	unsigned long	flags;		/* flags (i.e. zero fill section, etc)*/
	unsigned long	reserved1;	/* reserved */
	unsigned long	reserved2;	/* reserved */
};
/* Constants for the flags field of a section structure */
#define	S_ZEROFILL		0x1	/* zero fill on demand section */
#define	S_CSTRING_LITERALS	0x2	/* section with only literal C strings*/
#define	S_4BYTE_LITERALS	0x3	/* section with only 4 byte literals */
#define	S_8BYTE_LITERALS	0x4	/* section with only 8 byte literals */
#define	S_LITERAL_POINTERS	0x5	/* section with only pointers to */
					/*  literals */

/*
 * The names of segments and sections in them are mostly meaningless to the
 * link-editor.  But there are few things to support traditional UNIX
 * executables that require the link-editor and assembler to use some names
 * agreed upon by convention.
 *
 * The initial protection of the "__TEXT" segment has write protection turned
 * off (not writeable).
 *
 * The link-editor will allocate common symbols at the end of the "__common"
 * section in the "__DATA" segment.  It will create the section and segment
 * if needed.
 */

/* The currently known segment names and the section names in those segments */

#define	SEG_PAGEZERO	"__PAGEZERO"	/* the pagezero segment which has no
					   protections and catches NULL
					   references for MH_EXECUTE files */


#define	SEG_TEXT	"__TEXT"	/* the tradition UNIX text segment */
#define	SECT_TEXT	"__text"	/* the real text part of the text
					   section no headers, and no padding */
#define SECT_FVMLIB_INIT0 "__fvmlib_init0"	/* the fvmlib initialization */
						/*  section */
#define SECT_FVMLIB_INIT1 "__fvmlib_init1"	/* the section following the */
					        /*  fvmlib initialization */
						/*  section */

#define	SEG_DATA	"__DATA"	/* the tradition UNIX data segment */
#define	SECT_DATA	"__data"	/* the real initialized data section
					   no padding, no bss overlap */
#define	SECT_BSS	"__bss"		/* the real uninitialized data section
					   no padding */
#define SECT_COMMON	"__common"	/* the section common symbols are allo-
					   cated in by the link editor */

#define	SEG_OBJC	"__OBJC"	/* objective-C runtime segment */
#define SECT_OBJC_SYMBOLS "__symbol_table"	/* symbol table */
#define SECT_OBJC_MODULES "__module_info"	/* module information */
#define SECT_OBJC_STRINGS "__selector_strs"	/* string table */
#define SECT_OBJC_REFS "__selector_refs"	/* string table */

#define	SEG_ICON	 "__ICON"	/* the NeXT icon segment */
#define	SECT_ICON_HEADER "__header"	/* the icon headers */
#define	SECT_ICON_TIFF   "__tiff"	/* the icons in tiff format */

#define	SEG_LINKEDIT	"__LINKEDIT"	/* the segment containing all structures
					   created and maintained by the link
					   editor.  Created with -seglinkedit 
					   option to ld(1) for MH_EXECUTE and
					   FVMLIB file types only  */
/*
 * Fixed virtual memory shared libraries are identified by two things.  The
 * target pathname (the name of the library as found for execution), and the
 * minor version number.  The address of where the headers are loaded is in
 * header_addr.
 */
struct fvmlib {
	union lc_str	name;		/* library's target pathname */
	unsigned long	minor_version;	/* library's minor version number */
	unsigned long	header_addr;	/* library's header address */
};

/*
 * A fixed virtual shared library (filetype == MH_FVMLIB in the mach header)
 * contains a fvmlib_command (cmd == LC_IDFVMLIB) to identify the library.
 * An object that uses a fixed virtual shared library also contains a
 * fvmlib_command (cmd == LC_LOADFVMLIB) for each library it uses.
 */
struct fvmlib_command {
	unsigned long	cmd;		/* LC_IDFVMLIB or LC_LOADFVMLIB */
	unsigned long	cmdsize;	/* includes pathname string */
	struct fvmlib	fvmlib;		/* the library identification */
};

/*
 * Thread commands contain machine-specific data structures suitable for
 * use in the thread state primitives.  The machine specific data structures
 * follow the struct thread_command as follows.
 * Each flavor of machine specific data structure is preceded by an unsigned
 * long constant for the flavor of that data structure, an unsigned long
 * that is the count of longs of the size of the state data structure and then
 * the state data structure follows.  This triple may be repeated for many
 * flavors.  The constants for the flavors, counts and state data structure
 * definitions are expected to be in the header file <machine/thread_status.h>.
 * These machine specific data structures sizes must be multiples of
 * sizeof(long).  The cmdsize reflects the total size of the thread_command
 * and all of the sizes of the constants for the flavors, counts and state
 * data structures.
 *
 * For executable objects that are unix processes there will be one
 * thread_command (cmd == LC_UNIXTHREAD) created for it by the link-editor.
 * This is the same as a LC_THREAD, except that a stack is automatically
 * created (based on the shell's limit for the stack size).  Command arguments
 * and environment variables are copied onto that stack.
 */
struct thread_command {
	unsigned long	cmd;		/* LC_THREAD or  LC_UNIXTHREAD */
	unsigned long	cmdsize;	/* total size of this command */
	/* unsigned long flavor		   flavor of thread state */
	/* unsigned long count		   count of longs in thread state */
	/* struct XXX_thread_state state   thread state for this flavor */
	/* ... */
};

/*
 * The symtab_command contains the offsets and sizes of the link-edit 4.3BSD
 * "stab" style symbol table information as described in the header files
 * <nlist.h> and <stab.h>.
 */
struct symtab_command {
	unsigned long	cmd;		/* LC_SYMTAB */
	unsigned long	cmdsize;	/* sizeof(struct symtab_command) */
	unsigned long	symoff;		/* symbol table offset */
	unsigned long	nsyms;		/* number of symbol table entries */
	unsigned long	stroff;		/* string table offset */
	unsigned long	strsize;	/* string table size in bytes */
};

/*
 * The symseg_command contains the offset and size of the GNU style
 * symbol table information as described in the header file <symseg.h>.
 * The symbol roots of the symbol segments must also be aligned properly
 * in the file.  So the requirement of keeping the offsets aligned to a
 * multiple of a sizeof(long) translates to the length field of the symbol
 * roots also being a multiple of a long.  Also the padding must again be
 * zeroed. (THIS IS OBSOLETE and no longer supported).
 */
struct symseg_command {
	unsigned long	cmd;		/* LC_SYMSEG */
	unsigned long	cmdsize;	/* sizeof(struct symseg_command) */
	unsigned long	offset;		/* symbol segment offset */
	unsigned long	size;		/* symbol segment size in bytes */
};

/*
 * The ident_command contains a free format string table following the
 * ident_command structure.  The strings are null terminated and the size of
 * the command is padded out with zero bytes to a multiple of sizeof(long).
 * (THIS IS OBSOLETE and no longer supported).
 */
struct ident_command {
	long cmd;		/* LC_IDENT */
	long cmdsize;		/* includes strings that follow this command */
};

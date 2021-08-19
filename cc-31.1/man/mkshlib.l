.TH MKSHLIB l "April 16, 1990" "NeXT, Inc."
.SH NAME
mkshlib \- create a shared library
.SH SYNOPSIS
.B mkshlib
\-s specfile \-t target \-h host [options] ...
.SH DESCRIPTION
The 
.I mkshlib
command builds both the host and target shared libraries.  A shared
library is similar in function to a normal, non-shared library,
except that programs that link with a shared library will share the library code
during execution. Programs that link with a non-shared library will get 
their own copies of each library routine used.
.PP
The host shared library is an archive that is used to link-edit user programs
with the shared library [see
.IR ar (5)
].  A host shared library can
be treated exactly like a non-shared library and should be included on
compiler,
.IR cc (1),
command lines in the usual way.  Further, 
all operations that can be performed on an archive can also be
performed on the host shared library.
.PP
The target shared library is an executable module that is attached to the 
user's process during execution of a program using 
the shared library.  The target shared library contains the code
for all the routines in the library and must be fully resolved.  
The target will be brought into memory during execution of a program
using the shared library, and subsequent processes that use the shared library will share
the copy of code already in memory.  The text of 
the target is always shared, but each process will get its own copy of the 
data.
.PP
The user interface to 
.I mkshilib
consists of command line options and a shared library specification file.
The shared library specification file describes the 
contents of the shared library.
.PP
The
.I mkshlib
command invokes other tools, such as the assembler,
.IR as (1),
and the link editor,
.IR ld (1).
These tools
are invoked through the use of
.IR execvp (3),
which searches directories
in the user's PATH.
.PP
The following command line arguments are recognized by
.I mkshlib:
.TP 15
.BI "\-s " specfile
Specifies the shared library specification file,
.I specfile.
This file contains the information necessary to build a shared library.  Its
contents include the branch table specifications for the target,
the pathname in which the target should be installed, the start addresses
of text and data for the target, the initialization specifications for the
host, and the list of object files to be included in the shared library (see
details below).
.TP 15
.BI "\-t " target
Specifies the name,
.I target,
of the target shared library 
produced on the host machine.  When
.I target
is moved to the target machine, it
should be installed at the location given in
the specification file (see the
.B #target
directive below).
.TP 15
.BI "\-h " host
Specifies the name of the host shared library,
.I host.
.TP 15
.B \-v
Set the verbose option.  This option prints the command lines it executes.
.TP 15
.B \-d
Sets the debugging option.  This option creates a directory ./hostdir and
leaves all the temporary files it creates in there and does not remove them.
.TP 15
.B \-f
Don't create the host library.
.TP 15
.BI \-minor_version " num"
Set the minor version number of the library.  This or the directive
.BI "#minor_version " num
can be used (see below).
.BI "\-sectcreate" " segname sectname file"
The section named,
.I sectname,
in the segment named,
.I segname,
is created from the contents of the file,
.I file.
This section name can not be the same as a section name in an input object file
in the same segment.  This option was previously
.B "\-segcreate"
and will continue to be recognized.  This option is passed to the link editor.
.TP 15
.BI "\-segaddr" " name addr"
Specifies the starting address of the named segment,
.I name,
to be
.I addr.
Where
.I addr
is a hexadecimal number and must be a multiple of the segment alignment.
.TP 15
.BI "\-segprot" " name max init"
Specifies the maximum and initial virtual memory protection of the named
segment,
.I name,
to be
.I max
and
.I init
respectfully.  The values for
.I max
and
.I init
are any combination of the characters `r' (for read), `w' (for write),
`x' (for execute) and '\-' (no access).  This option is passed to the link
editor.
.TP 15
.B \-seglinkedit
Pass this flag to the link editor so that the \_\|\_SEGLINKEDIT segment is
produced in the target shared library.
.PP
The shared library specification file contains all the information
necessary to build both the host and target shared libraries.  The
contents and format of the specification file are given
by the following directives:
.TP 15
.BI "#address " "segname address"
Specifies the start address,
.I address,
of the segment
.I segname
for the target.  This directive is used to specify the start addresses of
the __TEXT and __DATA segments.  Since the headers are part of the
text segment of a target shared library they are put on there own page.
The real text starts on the next page from where the text segment is specified
with this directive.
.TP 15
.BI "#target " pathname
Specifies the absolute pathname,
.I pathname,
of the target shared library on the target machine.  This pathname is copied to
.B a.out
files and is the location where the operating system will look for
the shared library when executing a file that uses it.
.TP 15
.B #branch
Specifies the start of the branch table specifications.  The lines
following this directive are taken to be branch
table specification lines.
.sp 1
Branch table specification lines have the following format:
.sp 1
.nf
.B
		funcname <white space> position
.r
.sp 1
.fi
where
.I funcname
is the name of the symbol given a branch table entry
and
.I position
specifies the position of
.I funcname's
branch table entry.
.I Position
may be a single integer integer or a range of integers of the form
.I position1-position2.
Each
.I position
must be greater than or equal to zero, the same position cannot be specified
more than once, and every position from one to the highest
given position must be accounted for.  The special
.I funcname
.I .empty_slot
allocates branch tables slot(s) but put no function in them.  These branch
table slots have a ``trapl #0xfeadface'' instruction in them.
.sp 1
If a symbol is given more than one branch table entry by associating 
a range of positions with the symbol or by
specifying the same symbol on more than one branch table specification
line, the symbol is defined to have the address
of the highest associated branch table entry.  All other branch table entries
for the symbol can be thought of as ``empty'' slots and can be
replaced by new entries in future versions of the shared library.
.sp 1
Finally, only functions should be given branch table entries, and
those functions must be external.
.sp 1
This directive can be specified only once per shared library specification file.
.TP 15
.B #objects
Specifies the names of the object files constituting the 
target shared library.  The lines following this directive are taken to
be the list of input object files in the order
they are to be loaded into the target.  The list simply
consists of each filename followed by white space.  This list is also used to 
determine the input object files for the host shared library.
.sp 1
This directive can be specified only once per shared library
specification file.
.TP 15
.BI "#init " object
Specifies that the object file,
.I object,
requires initialization
code. The lines following this directive are taken to be
initialization specification lines.
.sp 1
Initialization specification lines have the following format:
.sp 1
.nf
.B
		pimport <white space> import
.r
.sp 1
.fi
.I Pimport
is a pointer to the associated imported symbol,
.I import,
and must be defined in the current specified object
file,
.I object.
The initialization code generated for each such line can be thought of an
assignment statement of the form:
.sp 1
.nf 
.B
		pimport = &import;
.r
.sp 1
.fi
where
.I pimport
is the absolute address of
.I import.
.sp 1
The
.I import
may no longer be any legal assembly language expression with no white space (for
example _foo+4 is no longer legal after the 1.0 release).
.TP 15
.BI "#minor_version " num
Specifies the decimal number,
.I num, 
as the minor version of the library.
This directive (or the command line option
.B \-minor_version) must be specified and
.I num
must be greater than zero.  The operating system only allows executables to
execute with minor versions that are equal or less than the shared library
on the system.
.TP 15
.B #private_externs
Specifies a list of external symbols in the library that are not to be included
in the host library.  The lines following this directive are taken to be the
symbol names.  This prevents the user of a library to use these symbols.
.TP 15
.B #nobranch_text
Specifies a list of external text symbols in the library that are not symbol
names of routines but rather
.I const
data.
This avoids
.I mkshlib
printing a warning for these symbols complaining that they are no branch table
entries for these text symbols.
.TP 15
.B #undefineds
Specifies a list of symbols in the library that are expected to be undefined.
Other symbols that are undefined will cause an error message and the library
will not be built.
.TP 15
.B #alias
Specifies a list of pairs of symbols to ``alias'' to each other using the
.IR ld (1)
.BI \-i definition:indirect
option and automaticly causing
.I definition
to be a private extern.
Each of the following lines contains a pair of symbol names on the
same line separated by white space.  The first symbol name on the line is the
.I definition
symbol name and the second is the
.I indirect
symbol name the symbol will become after it is link edited.
.TP 15
\f3##\f1
Specifies a comment.  All information on a line following
this directive is ignored.
.PP
All directives that may be followed by multi-line specifications are
valid until the next directive or the end of the file.
.SH FILES
.nf
mkshlib_*		temporary files
.fi
.SH SEE ALSO
ar(1), as(1), cc(1), ld(1), Mach-O(5), ar(5), ranlib(1), otool(1)

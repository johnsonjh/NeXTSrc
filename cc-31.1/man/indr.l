.TH INDR l "March 1, 1989" "NeXT, Inc."
.SH NAME
indr \- add indirection to symbols in object files
.SH SYNOPSIS
.B indr
[\-nvd] list input output
.SH DESCRIPTION
.I Indr
builds the output file,
.I output,
by translating each symbol name listed in the file
.I list
to the same name with and underbar prepended to it in all the
objects in the input file,
.I input.
This is used in building the ANSI C library and ``hiding'' non-ANSI C library
symbols that are used by the ANSI C routines.
The input file can be either and object file or an archive and the output
file will be the same type as the input file.
Then it if the input file is an archive and the
.B \-n
flag is not specified then it creates an object for each of these
symbols with an indirect symbol (n_type == N_INDR) for the symbol name with an
underbar and adds that to the output archive.
.PP
Some or all of the following options may be specified:
.TP
.B \-n
Suppress creating the indirect objects when the output file is an archive.
This is assumed when the output file is an object file.
.TP
.B \-v
Sets the verbose option.  This prints the commands that are executed.
.TP
.B \-d
Sets the debugging option.  This option creates a directory ./bozodir and
leaves all the temporary files it creates in there and does not remove them.
.SH FILES
tempdir???/*	temporary files
.SH SEE ALSO
Mach-O(5)

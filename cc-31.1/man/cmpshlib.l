.TH CMPSHLIB l "June 25, 1990" "NeXT, Inc."
.SH NAME
cmpshlib \- compare two shared libraries for compatibility
.SH SYNOPSIS
.B cmpshlib
.B "[ -s spec_file ]"
old new
.SH DESCRIPTION
.I Cmpshlib
compares two target shared libraries to see if the changes from the old target
shared libaray,
.I old,
are compatible with the new target shared library,
.I new.
This currently only checks the following: that the all of the old branch table
targets have the same symbol in the new branch table and that all the old global
data symbols have the same addresses.
.PP
With the
.B "\-s spec_file"
global const data symbols (#nobranch_text and not #private_externs) are checked
for the same addresses.  Also with this option it will not report data symbols
that are #private_externs having different addresses.
.SH SEE ALSO
mkshlib(l)
.SH BUGS
There are lots of other things including things in the host libraries that
could be checked for that are not.  This only catches most of the major stuff.
Also any slot that was part of a range used for a symbol that was replaced by
a new symbol gets printed as a warning (this is not the case if .empty_slot
is used).

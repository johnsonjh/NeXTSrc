#! /bin/csh -f
#PROGRAM
#
# vers_string PROGRAM [STAMPED_NAME]
#
# Output a string suitable for use as a version identifier
#
set cflag=0 fflag=0 lflag=0 Bflag=0 nflag=0
while ( $#argv > 0 )
	if ( "$argv[1]" !~ -* ) break
	if ( "$argv[1]" =~ *c* ) set cflag=1
	if ( "$argv[1]" =~ *f* ) set fflag=1
	if ( "$argv[1]" =~ *l* ) set lflag=1
	if ( "$argv[1]" =~ *B* ) set Bflag=1
	if ( "$argv[1]" =~ *n* ) set nflag=1
	shift
end
if ( $#argv > 1) then
	set version=$argv[2]
	set rev=( `expr $version : '.*-\(.*\)'`)
	if ( $status != 0 ) then
		echo "${0}: No hyphen in project root $version" >/dev/tty
		exit(1)
	endif
else
	set curdir=`pwd`
	while ( $curdir:t != $curdir )
		set version=$curdir:t
		set rev=( `expr $version : '.*-\(.*\)'`)
		if ( $status == 0 ) break
		set curdir=$curdir:h
	end
	if ( $curdir:t == $curdir ) then
		set curdir=`pwd`
		echo "${0}: No hyphen in project root $curdir" >/dev/tty
		echo "${0}: Could not determine version" >/dev/tty
		set version=Unknown
		set rev=""
	endif
endif
if ( ! $?USER ) then
	set USER=`whoami`
endif
if ( $#argv > 0) then
	set PROG=$argv[1]
else
	set PROG=Unknown
endif
if ( $Bflag ) then
	set date="NO DATE SET (-B used)"
else
	set date=`date`
endif
if ( $lflag ) then
	echo "static const char SGS_VERS[160] =" '"'"@(#)LIBRARY:$PROG  PROJECT:${version}  DEVELOPER:${USER}  BUILT:${date}\n"'";'
else if ( $cflag ) then
	echo "const char SGS_VERS[160] =" '"'"@(#)PROGRAM:$PROG  PROJECT:${version}  DEVELOPER:${USER}  BUILT:${date}\n"'";'
	echo "const char VERS_NUM[10] =" '"'"${rev}"'";'
else if ( $fflag ) then
	echo $PROG-$rev
else if ( $nflag ) then
	echo $rev
else
	echo "PROGRAM:$PROG  PROJECT:${version}  DEVELOPER:${USER}  BUILT:${date}"
endif


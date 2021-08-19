#! /bin/csh -fb
# This shell script is called from nu to initialize the contents of a
# newly-created user's directory.
#
# it is named "nu2.sh" instead of something like "addfiles.sh" to discourage
# people from trying to run it standalone, without going through nu.
#
# Created:  25 Aug 84	Brian Reid

set logindir=$1
set uid=$2
set gid=$3
set wantMHsetup=$4
set debug=$5
set noglob; set path=(/etc /usr/etc $path); unset noglob
if ( $debug != 0 ) then
    set verbose
endif
cd $logindir
if ( -d /usr/template/user ) then
    foreach i (/usr/template/user/.[a-zA-Z]* /usr/template/user/*)
	if ( $i != /usr/template/user/Mail ) then
	    cp -rp $i .
	endif
    end
    if ( $debug == 0 ) then
	set files = `find . -print`
	chown $uid .[a-zA-Z]* $files
	chgrp $gid .[a-zA-Z]* $files
    endif
endif
if ($wantMHsetup) then
    mkdir Mail
    mkdir Mail/inbox
    if ( -d /usr/template/user/Mail/inbox ) then
	cp /usr/template/user/Mail/inbox/* Mail/inbox
    endif
    if ($debug == 0) chown $uid Mail Mail/* Mail/inbox/*
    if ($debug == 0) chgrp $gid Mail Mail/* Mail/inbox/*
    chmod 0711 Mail
endif

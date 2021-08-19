#! /bin/csh -fb
# This shell script is called from /etc/nu to create a new directory for a
# new user, and to make the necessary links and permissions for it.
# 
# it is named "nu1.sh" instead of something like "makeuser.sh" to discourage
# people from trying to run it standalone, without going through nu.

set uid=$1
set gid=$2
set logindir=$3
set linkdir=$4
set clobber=$5
set debug=$6
set username=$7
set noglob; set path=(/etc /usr/etc $path); unset noglob
if ( $debug != 0 ) then
    set verbose
endif
if ($clobber) then
    rm -rf $logindir
    mkdir $logindir
endif
if ("$logindir" != "$linkdir") then
    ln -s $logindir $linkdir
    chown $uid $linkdir
    chgrp $gid $linkdir
endif
if (($debug == 0) && $clobber) chown $uid $logindir
if (($debug == 0) && $clobber) chgrp $gid $logindir

# Mail to root that a new user was created....
if ($debug == 0) then
	mail -s "Welcome new user $username:t" root  << xxMAILxx
	The new user "$username:t" was added by $user.

	The Nu Program.
xxMAILxx
endif

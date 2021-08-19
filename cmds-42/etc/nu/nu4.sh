#! /bin/csh -fb
# This shell script is called from nu to purge one account without
# removing it from the /etc/passwd database (to preserve accounting info.)
#
# it is named "nu4.sh" instead of something like "deleteacct.sh" to discourage
# people from trying to run it standalone, without going through nu.
#
# Created: 8 Oct 84	Jeffrey Mogul
# Modified:  9 Jun 89   Lee Tucker

set exuser=$1
set logindir=$2
set linkdir=$3
set Logfile=$4
set debug=$5

set egrepstr = "^${exuser}\:"
if ($debug == 0) then
    rm -rf $logindir; echo rm -rf $logindir
    if ($logindir != $linkdir) then
	rm $linkdir
	echo rm $linkdir
    endif
    rm -f /usr/spool/mail/$exuser; echo rm -f /usr/spool/mail/$exuser
endif
echo $exuser deleted by $user `date` >> $Logfile

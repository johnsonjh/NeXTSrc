#! /bin/csh -f
#PROGRAM
# builddisk [-n] [-q] [-h HOSTNAME] [-l LABEL] [-t DISKTYPE] [-s SCRIPT] DEV
#
set path=( /usr/etc /bin /usr/bin /usr/ucb /etc $path )
set prog=$0
set hostname=`hostname`
set scriptdir=/etc
set exitstatus=0
set mounted=()
set mountlist=()
while ( $#argv > 0 )
	switch ( $argv[1] )
	case -x:
		set nofscks
		shift
		breaksw
	case -n:
		set noinit
		shift
		breaksw
	case -q:
		set noquestions
		shift
		breaksw
	case -h:
		if ( $#argv < 2 ) then
			echo ${prog:t}: -h requires hostname
			exit(1)
		endif
		set hostname=$argv[2]
		shift; shift
		breaksw
	case -l:
		if ( $#argv < 2 ) then
			echo ${prog:t}: -l requires disklabel
			exit(1)
		endif
		set label="$argv[2]"
		shift; shift
		breaksw
	case -t:
		if ( $#argv < 2 ) then
			echo ${prog:t}: -t requires disktab type
			exit(1)
		endif
		set dtype=$argv[2]
		shift; shift
		breaksw
	case -s:
		if ( $#argv < 2 ) then
			echo ${prog:t}: -s requires builddisk script name
			exit(1)
		endif
		set script=$argv[2]
		shift; shift
		breaksw
	case -*:
		echo ${prog:t}: Unknown option: $argv[1]
		exit(1)
	default:
		break
	endsw
end
set noglob
umask 022
if ( `whoami` != "root" ) then
	echo You must be root to run ${prog:t}
	exit(1)
endif
if ( $#argv != 1 ) then
	echo Usage: ${prog:t} [-n] [-q] [-h HOSTNAME] [-l LABEL] [-t DISKTYPE] [-s SCRIPT] DEV
	exit(1)
endif
switch ( $argv[1] )
case od[01]:
	set type=od
	breaksw
case sd[01234567]:
	set type=sd
	breaksw
default:
	echo ${prog:t}: Device must be of form \"od0\", \"sd0\", etc
	exit(1)
endsw
set dev="/dev/$argv[1]"
set rdev = "/dev/r"$argv[1]
switch ( $type )
case 'sd':
	set mb=( `scsimodes -C ${rdev}a` )
	if ( $status ) then
		echo ${prog:t}: Can\'t determine SCSI capacity
		set fstab=sd330
	else if ( $mb > 500 ) then
		set fstab=sd660
		set dscript=sd660
	else if ( $mb > 270 ) then
		set fstab=sd330
		set dscript=sd330
	else if ( $mb > 185 ) then
		set fstab=sd330
		set dscript=sd200app
#	else if ( $mb > 75 ) then
#		set fstab=sd330
#		set dscript=sd100
	else
		set fstab=sd330
		set dscript=swapdisk
		set label=swapdisk
	endif
	breaksw
case 'od':
	set fstab=od
	set dscript=od
	breaksw
case 'fd':
	set fstab=fd
	set dscript=fd
	breaksw
endsw
if ( ! $?script ) then
	if ( $?dscript ) then
		set script=$dscript
	else
		echo ${prog:t}: Use -s SCRIPT option to specify script
		echo Known scripts are:
		unset noglob
		ls $scriptdir/BLD.* | sed -e 's/.*\/BLD\.//'
		exit(1)
	endif
endif
if ( ! $?label ) then
	set label="MyDisk"
endif
if ( -e BLD.$script ) then
	set desc=BLD.$script
else if ( -e $scriptdir/BLD.$script ) then
	set desc=$scriptdir/BLD.$script
else
	echo ${prog:t}: can\'t find builddisk script BLD.$script
	echo Known scripts are:
	unset noglob
	ls $scriptdir/BLD.* | sed -e 's/.*\/BLD\.//'
	exit(1)
endif
set mtab=( `mount | grep ^${dev}` )
if ( $#mtab > 0 ) then
	mount | grep ^${dev}
	echo ${prog:t}: ${dev} is mounted, unmount it before running ${prog:t}
	exit(1)
endif
set mtab=( `mount | grep '/mnt[abc]' ` )
if ( $#mtab > 0 ) then
	mount | grep '/mnt[abc]'
	echo ${prog:t}: /mnta, /mntb, or /mntc is in use
	echo ${prog:t} must use these mount points
	exit(1)
endif
if ( ! $?noinit ) then
	if ( ! $?noquestions ) then
		echo -n "builddisk destroys previous contents of $dev, ok? "
		set rsp=$<
		if ( $rsp !~ [yY]* ) then
			echo ${prog:t} aborted
			exit(1)
		endif
	endif
else
	if ( ! $?noquestions ) then
		echo -n "builddisk overwrites contents of $dev, ok? "
		set rsp=$<
		if ( $rsp !~ [yY]* ) then
			echo ${prog:t} aborted
			exit(1)
		endif
	endif
	set initialized
endif
onintr abort
@ line = 1
while (1)
	set cmdline = ( `sed -n -e ${line}p $desc` )
	if ( $#cmdline < 1 ) goto done
	if ( "$cmdline[1]" =~ \#* ) then
		echo $cmdline
		goto nextline
	endif
	if ( $#cmdline < 2 ) then
		echo ${prog:t}: Bad command line \"$cmdline\"
		goto abort
	endif
	echo Doing: $cmdline
	set cmd = $cmdline[1]
	set part = $cmdline[2]
	if ( $cmd != newdisk && $?initialized == 0 ) then
		if ( $?dtype ) then
			disk -i -t $dtype -h $hostname -l "$label" ${rdev}a
			if ( $status != 0 ) then
				echo ${prog:t}: Disk initialization failed
				goto abort
			endif
		else
			disk -i -h $hostname -l "$label" ${rdev}a
			if ( $status != 0 ) then
				echo ${prog:t}: Disk initialization failed
				goto abort
			endif
		endif
		set initialized
	endif
	if ( $cmd != mount && $cmd != newdisk && "$mounted" !~ *\:$part\:* ) then
		if ( ! $?nofscks && ! { fsck ${rdev}$part } ) then
			echo ${prog:t}: fsck of ${rdev}$part failed
			goto abort
		endif
		umount /mnt$part >& /dev/null
		if ( ! { mount ${dev}$part /mnt$part } ) then
			echo ${prog:t}: mount ${dev}$part /mnt$part failed
			goto abort
		endif
		set mounted=($mounted \:$part\: )
		set mountlist=($mountlist $part)
	endif
	switch ( $cmd )
	case "load":
		if ( $#cmdline < 3 || $#cmdline > 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set local = $cmdline[3]
		if ( $#cmdline == 4 ) then
			set target = $cmdline[4]
		else
			set target = $local
		endif
		mkdirs /mnt$part/$target
		(cd $local; tar cf - .)|(cd /mnt$part/$target; tar xfpB - )
		if ( $status != 0 ) then
			echo ${prog:t}: load of $target from $local failed
			set exitstatus=1
		endif
		breaksw
	case "cload":
		if ( $#cmdline < 3 || $#cmdline > 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set local = $cmdline[3]
		if ( $#cmdline == 4 ) then
			set target = $cmdline[4]
		else
			set target = $local
		endif
		if ( -d $local ) then
			mkdirs /mnt$part/$target
			(cd $local; tar cf - .)|(cd /mnt$part/$target; tar xfpB - )
			if ( $status != 0 ) then
				echo ${prog:t}: load of $target from $local failed
				set exitstatus=1
			endif
		endif
		breaksw
	case "symlink":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set dest = $cmdline[3]
		set src = $cmdline[4]
		rm -f /mnt$part/$src
		ln -s $dest /mnt$part/$src
		if ( $status != 0 ) then
			echo ${prog:t}: symlink of $src to $dest failed
			set exitstatus=1
		endif
		breaksw
	case "link":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set dest = $cmdline[3]
		set src = $cmdline[4]
		rm -f /mnt$part/$src
		ln /mnt$part/$dest /mnt$part/$src
		if ( $status != 0 ) then
			echo ${prog:t}: link of $src to $dest failed
			set exitstatus=1
		endif
		breaksw
	case "copy":
		if ( $#cmdline < 3 || $#cmdline > 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set src = $cmdline[3]
		if ( $#cmdline == 4 ) then
			set dest = $cmdline[4]
		else
			set dest = $src
		endif
		rm -f /mnt$part/$dest
		unset noglob
		if ( ! { cp -p $src /mnt$part/$dest } ) then
			set noglob
			echo ${prog:t}: copy of $src to $dest failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "ccopy":
		if ( $#cmdline < 3 || $#cmdline > 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set src = $cmdline[3]
		if ( $#cmdline == 4 ) then
			set dest = $cmdline[4]
		else
			set dest = $src
		endif
		if ( -f $src ) then
			rm -f /mnt$part/$dest
			unset noglob
			if ( ! { cp -p $src /mnt$part/$dest } ) then
				set noglob
				echo ${prog:t}: copy of $src to $dest failed
				set exitstatus=1
			endif
			set noglob
		endif
		breaksw
	case "move":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set src = $cmdline[3]
		set dest = $cmdline[4]
		rm -f /mnt$part/$dest
		unset noglob
		if ( ! { mv /mnt$part/$src /mnt$part/$dest } ) then
			set noglob
			echo ${prog:t}: move of $src to $dest failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "mount":
		if ( $#cmdline != 3 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set src = $cmdline[2]
		set dest = $cmdline[3]
		umount $dest
		if ( ! { mount $src $dest } ) then
			echo ${prog:t}: mount of $src on $dest failed
			goto abort
		endif
		breaksw
	case "mkdirs":
		if ( $#cmdline != 3 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set dirs = $cmdline[3]
		if ( ! { mkdirs /mnt$part/$dirs } ) then
			echo ${prog:t}: mkdirs of $dirs failed
			set exitstatus=1
		endif
		breaksw
	case "rrm":
		if ( $#cmdline != 3 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set dirs = $cmdline[3]
		unset noglob
		if ( ! { rm -rf /mnt$part/$dirs } ) then
			set noglob
			echo ${prog:t}: rm -rf of $dirs failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "rm":
		if ( $#cmdline != 3 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set file = $cmdline[3]
		unset noglob
		if ( ! { rm /mnt$part/$file } ) then
			set noglob
			echo ${prog:t}: rm of $file failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "strip":
		if ( $#cmdline != 3 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set file = $cmdline[3]
		unset noglob
		strip /mnt$part/$file >& /dev/null
		set noglob
		breaksw
	case "chmod":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set mode = $cmdline[3]
		set file = $cmdline[4]
		unset noglob
		if ( ! { chmod $mode /mnt$part/$file } ) then
			set noglob
			echo ${prog:t}: chmod $mode $file failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "chown":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set owner = $cmdline[3]
		set file = $cmdline[4]
		unset noglob
		if ( ! { chown $owner /mnt$part/$file } ) then
			set noglob
			echo ${prog:t}: chown $owner $file failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "chgrp":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set group = $cmdline[3]
		set file = $cmdline[4]
		unset noglob
		if ( ! { chgrp $group /mnt$part/$file } ) then
			set noglob
			echo ${prog:t}: chgrp $group $file failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "rchmod":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set mode = $cmdline[3]
		set file = $cmdline[4]
		unset noglob
		if ( ! { chmod -R $mode /mnt$part/$file } ) then
			set noglob
			echo ${prog:t}: chmod -R $mode $file failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "rchown":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set owner = $cmdline[3]
		set file = $cmdline[4]
		unset noglob
		if ( ! { chown -R $owner /mnt$part/$file } ) then
			set noglob
			echo ${prog:t}: chown -R $owner $file failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "rchgrp":
		if ( $#cmdline != 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set group = $cmdline[3]
		set file = $cmdline[4]
		unset noglob
		if ( ! { chgrp -R $group /mnt$part/$file } ) then
			set noglob
			echo ${prog:t}: chgrp -R $group $file failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "exec":
		if ( $#cmdline < 4 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set dir = $cmdline[3]
		set command = ( $cmdline[4-] )
		( cd /mnt$part/$dir; $command )
		if ( $status != 0 ) then
			echo ${prog:t}: $command failed
			set exitstatus=1
		endif
		breaksw
	case "newclient":
		if ( $#cmdline < 3 || $#cmdline > 4) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set dir = $cmdline[3]
		if ( $#cmdline < 4 ) then
			set opt = "-p"
		else
			set opt = $cmdline[4]
		endif
		( cd /mnt$part/$dir; ./newclient $opt $fstab /mnta/usr/template/client /mnta/private )
		if ( $status != 0 ) then
			echo ${prog:t}: newclient failed
			set exitstatus=1
		endif
		breaksw
	case "touch":
		if ( $#cmdline != 3 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set file = $cmdline[3]
		if ( ! { touch /mnt$part/$file } ) then
			echo ${prog:t}: touch $file failed
			set exitstatus=1
		endif
		breaksw
	case "remove":
		if ( $#cmdline != 3 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		set file=$cmdline[3]
		unset noglob
		if ( ! { rm -f /mnt$part/$file } ) then
			set noglob
			echo ${prog:t}: rm -f $file failed
			set exitstatus=1
		endif
		set noglob
		breaksw
	case "newdisk":
		if ( $#cmdline != 2 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		if ( $type !~ od && $type !~ fd) then
			echo ${prog:t}: Can\'t use newdisk command with $type disks
			goto abort
		endif
		if ( $#mountlist > 0 ) then
			while ( $#mountlist > 0 )
				umount /mnt$mountlist[1]
				if ( ! $?nofscks && ! { fsck -y ${rdev}$mountlist[1] } ) then
					echo ${prog:t}: fsck of ${rdev}$mountlist[1] failed
					set exitstatus=1
				endif
				df ${rdev}$mountlist[1]
				shift mountlist
			end
			echo Label ejected disk with $label
			disk -e ${rdev}a
			set mounted=()
			set mountlist=()
		endif
		set label=$part
		echo Beginning build of $label
		disk -i -h $hostname -l "$label" ${rdev}a
		if ( $status != 0 ) then
			echo ${prog:t}: Disk initialization failed
			goto abort
		endif
		set initialized
		breaksw
	case "bom":
		if ( $#cmdline > 3 ) then
			echo ${prog:t}: Bad command \"$cmdline\"
			goto abort
		endif
		if ( $#cmdline == 3 ) then
			set bom=$cmdline[3]
		else
			set bom=$label
		endif
		(cd /mnt$part; du; ls -lgAR ) >${bom}.bom
		breaksw
	default:
		echo ${prog:t}: Unknown command \"$cmdline\"
		goto abort
	endsw
nextline:
	@ line++
end

abort:
echo Build of $dev aborted
while ( $#mountlist > 0 )
	umount /mnt$mountlist[1]
	shift mountlist
end
set exitstatus=1
goto ejectit

done:
while ( $#mountlist > 0 )
	umount /mnt$mountlist[1]
	if ( ! $?nofscks && ! { fsck -y ${rdev}$mountlist[1] } ) then
		echo ${prog:t}: fsck of ${rdev}$mountlist[1] failed
		set exitstatus=1
	endif
	df ${rdev}$mountlist[1]
	shift mountlist
end
if ( $exitstatus != 0 ) then
	echo ""
	echo ============= WARNING ==================
	echo ${prog:t}: BUILD OF $dev HAD ERRORS
	echo DISK MAY NOT BE USABLE, REVIEW ERROR LOG
	echo ========================================
else
	echo ""
	echo ==========================
	echo Build of $dev complete
	echo ==========================
endif

ejectit:
if ( $type =~ od ) then
	disk -e ${rdev}a
endif
exit($exitstatus)






#! /bin/csh -fx
#PROGRAM
# mkSingleUser SRCROOT DSTROOT
# -- a simple script to setup the /bootdisk/Unix/SingleUser
# directory
#
# SRCROOT and DSTROOT should point to the Unix directories in the
# read-only and read-write partitions respectively
#
switch ( $#argv )
case 0:
	set srcroot=/bootdisk/NeXT/Unix
	set dstroot=/bootdisk/Unix
	breaksw
case 1:
	set srcroot=$argv[1]
	set dstroot=/bootdisk/Unix
	breaksw
case 2:
	set srcroot=$argv[1]
	set dstroot=$argv[2]
	breaksw
default:
	echo Usage: $0 SRCROOT DSTROOT
	exit(1)
endsw
if ( `whoami` != root ) then
	echo  You must be root to run mkSingleUser
	exit(1)
endif
if ( ! -d ${srcroot}/etc ) then
	echo ${srcroot}/etc does not exist
	exit(1)
endif
if ( ! -d ${srcroot}/bin ) then
	echo ${srcroot}/bin does not exist
	exit(1)
endif
if ( ! -d ${dstroot}/SingleUser ) then
	echo ${dstroot}/SingleUser does not exist
	exit(1)
endif
mkdirs ${dstroot}/SingleUser/bin
mkdirs ${dstroot}/SingleUser/etc
foreach i ( bin/cat bin/chgrp bin/chmod bin/cmp bin/cp bin/csh \
    bin/date bin/df bin/diff bin/echo bin/grep bin/hostid bin/hostname \
    bin/kill bin/ln bin/ls bin/mkdir bin/mv bin/od bin/ps bin/rcp \
    bin/rm bin/sh bin/stty bin/sync bin/tar bin/test \
    etc/chown etc/disk etc/dump etc/fsck etc/halt etc/ifconfig \
    etc/init etc/mach_init etc/mkfs etc/mknod etc/mount \
    etc/newfs etc/ping etc/rdump etc/reboot etc/restore etc/rrestore \
    etc/umount)
	cp -p ${srcroot}/$i \
	    ${dstroot}/SingleUser/$i
end
cp -p ${srcroot}/usr/ucb/vi \
    ${dstroot}/SingleUser/bin/vi
rm -f ${dstroot}/SingleUser/ex
ln ${dstroot}/SingleUser/bin/vi \
    ${dstroot}/SingleUser/bin/ex
rm -f ${dstroot}'/SingleUser/['
ln ${dstroot}/SingleUser/bin/test \
    ${dstroot}'/SingleUser/bin/['
echo SingleUser built

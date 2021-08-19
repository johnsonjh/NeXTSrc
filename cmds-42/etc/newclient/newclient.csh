#! /bin/csh -f
#
# newclient
#
# NetBoot client building script.
#
# Copyright (c) 1988, NeXT, Inc.
#
# Usage:
#	"newclient -p DISKTYPE TEMPLATE DEST" Standalone Private
#	"newclient [-s SERVER] CLIENT-1 ... CLIENT-n" Clients
#

#
# Constants
#
set path=(/bin /usr/bin /usr/ucb /usr/etc /etc)
set TESTDIR
set CLIENTDIR=${TESTDIR}/clients
set template=${TESTDIR}/usr/template/client
set HOSTFILE=${TESTDIR}/etc/hosts
set BOOTPTAB=${TESTDIR}/etc/bootptab

# We are the default server
set server=`hostname`

# Are we root?
if ( `whoami` != root ) then
	echo "You must be root to run $0"
	exit(1)
endif

# Preliminary argument checkout
if ( $#argv < 1 ) then
	goto usage
endif

# Switches must be at argv[1]
foreach arg ( $argv[2-] )
	if ( x$arg =~ x-* ) then
		goto usage
	endif
end

# Parse the switches
if ( $argv[1] == -p || $argv[1] == -P ) then
	if ( $#argv != 4 ) then
		goto usage
	endif
	set PrivateMode
	set disktype=$argv[2]
	set template=$argv[3]
	set dest=$argv[4]
	if ( $argv[1] == -P ) then
		set swapsize=1b
	else
		set swapsize=16m
	endif

	# Check that the disk type is valid
	if ( ! -e $template/etc/fstab.$disktype ) then
		echo "Unknown disk type" $disktype
		exit(1)
	endif
else if ( $argv[1] == -s ) then
	if ( $#argv < 3 ) then
		goto usage
	endif
	set server=$argv[2]
	shift
	shift
else if ( $argv[1] =~ -* ) then
	goto usage
endif

# Make sure we have a template to work with
if ( ! -d $template ) then
	echo "New client template $template does not exist"
	exit(1)
endif

# Process the private partition if that is the mode we are in.
if ( $?PrivateMode ) then
	if ( -e $dest ) then
		echo "$dest already exists"
		exit(1)
	endif

	# Create the destination directory
	echo -n "Creating ${dest}..."
	mkdir $dest
	chmod 775 $dest
	chown root.staff $dest
	if ( $status != 0 ) then
		echo "failed"
		exit(1)
	endif
	echo "[OK]"

	# Copy the template into it
	echo -n "Copying ${template} into ${dest}..."
	(cd $template; tar cf - .)|(cd $dest; tar xpBf -)
	echo "[OK]"

	# Make the device files
	echo -n "Making devices in ${dest}/dev..."
	(cd ${dest}/dev; /usr/etc/MAKEDEV NeXT)
	echo "[OK]"

	# Customize the fstab file
	echo -n "Installing fstab.$disktype as ${dest}/etc/fstab..."
	cp -p $dest/etc/fstab.${disktype} $dest/etc/fstab
	if ( $status != 0 ) then
		echo "failed"
		exit(1)
	endif
	echo "[OK]"

	# Touch the first 16 Meg of the swapfile
	echo -n "Preallocating swapfile blocks..."
	(cd ${dest}/vm; /bin/rm -f swapfile; /usr/etc/mkfile $swapsize swapfile)
	echo "[OK]"

	exit(0)
endif

#
# If we get here, then we are building a list of clients
#

echo $argv[*]
foreach client ( $argv[*] )
	if ( $server == localhost ) then
		echo "This machine can't be a server until it is given a hostname"
		exit(1)
	endif
	set dest=$CLIENTDIR/$client

	if (! -f /private/tftpboot/mach) then
		echo Installing /private/tftpboot/mach
		cp -p /mach /private/tftpboot/mach
	endif
	if (! -f /private/tftpboot/boot) then
		echo Installing /private/tftpboot/boot
		cp -p /usr/standalone/boot /private/tftpboot/boot
	endif

	# Check for already existing client directory
	if ( -e $dest ) then
		echo $dest already exists
		continue
	endif

	# Create the destination directory
	echo -n "Creating $dest..."
	mkdirs -m 775 -o root -g staff $dest
	if ( $status != 0 ) then
		echo "failed"
		exit(1)
	endif
	echo "[OK]"

	# Copy the template into it.
	echo -n "Copying $template into ${dest}..."
	(cd $template; tar cf - .)|(cd $dest; tar xpBf -)
	echo "[OK]"

	# Make the device files
	echo -n "Making devices in ${dest}/dev..."
	(cd $dest/dev; /usr/etc/MAKEDEV NeXT)
	echo "[OK]"

	# Customize the fstab file
	echo -n "Installing fstab.client as ${dest}/etc/fstab..."
	cp -p $dest/etc/fstab.client $dest/etc/fstab
	sed -e "s/SERVER/$server/g" -e "s/CLIENT/$client/g" $dest/etc/fstab.client > $dest/etc/fstab
	if ( $status != 0 ) then
		echo $failed
		exit(1)
	endif
	echo "[OK]"

	# Set up a symbolic link to the kernel
	
	if (-f ${dest}/tftpboot/mach) then
		echo -n "Removing ${dest}/tftpboot/mach "
		rm ${dest}/tftpboot/mach
		echo "[OK]"
	endif
		
	echo -n "Linking /tftpboot/mach to /sdmach "
	ln -s /sdmach ${dest}/tftpboot/mach
	echo "[OK]"

	# Touch the first 16 Meg of the swapfile
	echo -n "Creating swapfile..."
	(cd ${dest}/vm; /bin/rm -f swapfile; /usr/etc/mkfile 8k swapfile)
	echo "[OK]"

end

exit(0)

usage:
	echo "Usage: $0 -p DISKTYPE TEMPLATE DEST"
	echo "       $0 [-s SERVER] CLIENT CLIENT ..."
	exit(1)



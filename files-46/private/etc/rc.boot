#
# This script sets up the machine enough to run single-user
#
# Please note that all "echo" commands are in parentheses so that
# the main shell does not open a terminal and get its process group set.
#
HOME=/; export HOME
PATH=/etc:/usr/etc:/bin:/usr/bin; export PATH

#
# Default swapfile
#
swapfile=/private/vm/swapfile

#
# We must fsck here before we touch anything in the filesystems
#
fsckerror=0

# Output the date for reference
date								>/dev/console

if [ $1x = singleuserx ]
then
	(echo "Singleuser boot -- fsck not done")		>/dev/console
	mount -o remount /					>/dev/console
	if [ ! -f $swapfile ]
	then
		(echo "No default swapfile present!")		>/dev/console
	fi
else
	fbshow -A -B -E -s 2 "Checking" "Disk..."
	(echo Checking disks)					>/dev/console
	fsck -p						 >/dev/console 2>&1
	case $? in
	    0)
		;;
	    4)
		(echo "Root fixed - rebooting.")		>/dev/console
		reboot -q -n
		;;
	    8)
		(echo "Reboot failed...help!")			>/dev/console
		fsckerror=1
		;;
	    12)
		(echo "Reboot interrupted.")			>/dev/console
		fsckerror=1
		;;
	    *)
		(echo "Unknown error in reboot fsck.")		>/dev/console
		fsckerror=1
		;;
	esac
	mount -o remount /					>/dev/console
	if [ ! -f $swapfile ]
	then
		(echo -n "Creating default swapfile")		>/dev/console
		touch $swapfile
		chmod 1600 $swapfile
		(echo " - rebooting")				>/dev/console
		reboot -q
	fi
fi

# Read in configuration information
. /etc/hostconfig
case $? in
    0)
	;;
    *)
	(echo "Error reading hostconfig")			>/dev/console
	exit 1
	;;
esac

# Fake mount entries for root and private filesystems and then mount up
# the read-only software distribution
(echo "Faking root mount entries")				>/dev/console
> /etc/mtab
mount -f /
mount -f /private


# Configure network interfaces
ifconfigerror=0
if [ "${INETADDR=-AUTOMATIC-}" != "-NO-" ]
then
	(echo "Configuring ethernet interface to $INETADDR")	>/dev/console
	# Figure out netmask and broadcast flags
	if [ -n "${IPNETMASK=}" ]
	then
		IFFLAGS="netmask $IPNETMASK"
	else
		IFFLAGS=
	fi
	if [ "${IPBROADCAST=-AUTOMATIC-}" != "-AUTOMATIC-" -a -n "${IPBROADCAST}" ]
	then
		IFFLAGS="$IFFLAGS broadcast $IPBROADCAST"
		(echo "Setting broadcast address to $IPBROADCAST.") > /dev/console
	else
		(echo "Using default broadcast address.") > /dev/console
	fi
	fbshow -A -B -E -s 3 "Checking" "for" "Network..."
	# ifconfig knows about -AUTOMATIC-
	ifconfig en0 $INETADDR $IFFLAGS -trailers up \
							>/dev/console 2>&1
	ifconfigerror=$?
	if [ $ifconfigerror -ne 0 -a "${HOSTNAME=-AUTOMATIC-}" = "-AUTOMATIC-" ]
	then
		HOSTNAME=-NO-
	fi
else
	(echo "Skipping ethernet interface configuration")	>/dev/console
fi
(echo "Configuring local interface")				>/dev/console
ifconfig lo0 127.0.0.1 up

hosterror=0
if [ "${HOSTNAME=-AUTOMATIC-}" != "-NO-" ]
then
	(echo "Configuring hostname to $HOSTNAME")		>/dev/console
	# hostname knows about -AUTOMATIC-
	hostname $HOSTNAME				>/dev/console 2>&1
	hosterror=$?
else
	# This is NOT the place to change your hostname
	(echo "Setting hostname to localhost")		>/dev/console
	/bin/hostname localhost
fi
sync

# Exit with error if failed above
# Note that ifconfig exits 1 if the error is a show stopper and -1 if it
# is not.
if [ $fsckerror -ne 0 -o $ifconfigerror -eq 1 -o $hosterror -ne 0 ]
then
	exit 1
fi
exit 0

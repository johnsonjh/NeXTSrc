#!/bin/csh -f
#
# This script currently only handles integer version numbers
# It needs to handle minor versions for our current environment.
#
/bin/rm -f /tmp/dirs /tmp/names
/bin/ls -l .. | grep drwx | grep libsys | tail -2 > /tmp/dirs
 
foreach i (`/bin/awk '{print $8}' < /tmp/dirs`)
	expr $i : '.*-\(.*\)' >> /tmp/names
end	

set newvers = `tail -1 /tmp/names`
set oldvers = `tail -2 /tmp/names | head -1`

echo "Comparing version $oldvers with $newvers"
if ( -d ../libsys-$oldvers && -d ../libsys-$newvers )  then
    echo "Both directories exist"
    if ( -f ../libsys-$oldvers/libsys_s.B.shlib && \
    	 -f ../libsys-$newvers/libsys_s.B.shlib )  then
    	echo "Both files exist"
	cmpshlib -s ../libsys-$oldvers/spec_sys \
		../libsys-$oldvers/libsys_s.B.shlib \
		../libsys-$newvers/libsys_s.B.shlib
    else
	echo "Could not find the libsys_s.B.shlib files"
    endif
else
	echo "Could not find libsys directories"
endif
/bin/rm -f /tmp/dirs /tmp/names
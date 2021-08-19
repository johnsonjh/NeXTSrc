#!/bin/csh -f
#PROGRAM
# strip symbols of the args

if ($#argv == 0) then
	echo "Usage: stripfile <files...>"
	exit -1
endif

foreach f ($argv)
echo "	stripping $f"
ld -x -r -o $f.stripped $f
/bin/mv -f $f.stripped $f
end




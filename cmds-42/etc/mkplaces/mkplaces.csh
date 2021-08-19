#! /bin/csh -f
#PROGRAM -- don't delete me, I'm here for vers_string
# mkplaces [ -r DSTROOT ] [ DIR_LIST ]
# mkplaces -- make .places files throughout a directory tree
#
set dstroot=""
while ( $#argv > 0 )
	switch ( $argv[1] )
	case -r:
		set dstroot=$argv[2]
		shift; shift
		breaksw
	case -*:
		echo ${0}: Unknown option: $argv[1]
		exit(1)
		breaksw
	default:
		break
	endsw
end
set makeplaces=${dstroot}/usr/local/bin/makeplaces
if ( $#argv == 0 ) then
	set dirlist=( . )
else
	set dirlist=( $argv )
endif
foreach i ( $dirlist )
	find $i ! -type l -type d -exec ${makeplaces} "{}" \;
end

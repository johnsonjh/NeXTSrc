#! /bin/sh
#
# @(#)_fixcrypt.awk	1.1 88/04/04 4.0NFSSRC SMI;  from 1.2 88/02/08 (C) 1987 SMI
#
# Convert the first ".data" line to a ".text" line.

awk '$0 ~ /^[ 	]*\.data$/ { if ( FIRST == 0 ) { FIRST = 1 ; print "\t.text" } else { print $0 } }
$0 !~ /^[ 	]*\.data$/ { print $0 }'

#! /bin/sh
#
#     @(#)ypxfr_1perhour.sh	1.1 88/03/07 4.0NFSSRC SMI
#PROGRAM
#
# ypxfr_1perhour.sh - Do hourly yp map check/updates
#

PATH=/bin:/usr/bin:/usr/etc:/usr/etc/yp:$PATH
export PATH

# set -xv
/usr/etc/yp/ypxfr passwd.byname
/usr/etc/yp/ypxfr passwd.byuid 

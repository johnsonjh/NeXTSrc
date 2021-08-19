#! /bin/sh
#
#    @(#)ypxfr_2perday.sh	1.1 88/03/07 4.0NFSSRC SMI
#PROGRAM
#
# ypxfr_2perday.sh - Do twice-daily yp map check/updates
#

PATH=/bin:/usr/bin:/usr/etc:/usr/etc/yp:$PATH
export PATH

# set -xv
/usr/etc/yp/ypxfr hosts.byname
/usr/etc/yp/ypxfr hosts.byaddr
/usr/etc/yp/ypxfr ethers.byaddr
/usr/etc/yp/ypxfr ethers.byname
/usr/etc/yp/ypxfr netgroup
/usr/etc/yp/ypxfr netgroup.byuser
/usr/etc/yp/ypxfr netgroup.byhost
/usr/etc/yp/ypxfr mail.aliases 

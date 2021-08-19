#! /bin/sh
#
#    @(#)ypxfr_1perday.sh	1.1 88/03/07 4.0NFSSRC SMI
#PROGRAM
#
# ypxfr_1perday.sh - Do daily yp map check/updates
#

PATH=/bin:/usr/bin:/usr/etc:/usr/etc/yp:$PATH
export PATH

# set -xv
/usr/etc/yp/ypxfr group.byname
/usr/etc/yp/ypxfr group.bygid 
/usr/etc/yp/ypxfr protocols.byname
/usr/etc/yp/ypxfr protocols.bynumber
/usr/etc/yp/ypxfr networks.byname
/usr/etc/yp/ypxfr networks.byaddr
/usr/etc/yp/ypxfr services.byname
/usr/etc/yp/ypxfr ypservers

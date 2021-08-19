#! /bin/sh
#
#
#	uucp.day [-x] [-l]
#
#	this script is, as most, continuously under developement. the
# main reason for this script is to keep stats on uucp traffic in order to 
# better understand who we talk to and to help justify alternate networking
# ideas (x.25). 
#	problems to Cal_Thixton@Next.COM
#		cal thixton
#
# crontab:  20 23 * * * su uucp -c /usr/lib/uucp/uucp.day
#
# automatic logging to some master log site on the net....
#
# aliases: uucpstats: "|/usr/lib/uucp/uucp.day -l"
#
#	-l	log this report to STATSDIR
#	-x	turn debugging on. do nothing damaging.
#	
#  
#                            next
#                       UUCP GENERAL ACCOUNTING
#                          Mon Jun 15 1987
# Site       Bytes   Time         Bytes      Calls    Cost ($ ATT)   Files  BPS
#            Total hh:mm:ss    Out     In   Out Tot  Today  To_Date Out In
# austsun    41191  0:06:37   24488   16703  0  3 $   2.25   65.80  10  10  103
# cipric       901  0:00:11     410     491  0  2 $   0.00    0.00   1   1   75
# convex   1391749  1:50:13 1390921     828  0  3 $   0.00    0.00 145   2  210
# edsr     4011514 10:03:58 4011514       0  0  0 $   0.00    0.00 281   0  110
# housun     20314  0:03:56   20314       0  0  4 $   1.57    1.57   8   0   85
# kcsun        839  0:00:03     406     433  0  1 $   0.02    0.02   1   0  209
# milano       919  0:00:15     410     509  0  2 $   0.09    0.09   1   1   57
# sunarch      914  0:00:09     414     500  0  2 $   0.05    4.22   1   1   91
# suntan      3109  0:00:30    1521    1588  0  2 $   0.00    0.00   2   3  100
# ti-csl     18211  0:02:57    1533   16678  0  7 $   0.00    0.00   2   3  102
# tulsun     76674  0:06:04   75215    1459  0 14 $   1.58   10.64  26   2  210
# tundra     62273  0:04:34   61773     500  0  2 $   1.51    1.51  26   1  226
# uokmax     81172  0:12:35   80334     838  0  4 $   0.00    0.00   3   2  107
# | Days:   9 $/day: $   9.32            Total:   $   7.05   83.84 Bytes: 5709780
#
#	most of the colums should be obvious.
# Call_Out: The number of calls we initiated.
# Call_Tot: Total number of calls (in and out). call_tot - call_out = the # of calls
#	we received.
# Cost_today: an estimate of ATT charges for that day based on L-costs. call att and
#	get a $/minute rate for day/direct_dial.
# Cost_Todate: a running sum of the estimated att costs.
# Files_Out: files sent
# Files_In: files received
# Days: number of days since the total was reset back to 0.
# $/Day: average cost per day in $.
# Total: first number is the estimated cost for today followed by running total for
#	x days.
# BPS - average bytes/sec
#
#	1/15/88 - add sed to clean up incoming email when logging.
#		delete unnecessary headers. tj
#	1/15/88 - do not run uucpanz. tj
#
# daily UUCP cleanup
#
PATH="/bin:/usr/bin:/usr/lib/uucp:/usr/ucb:/etc:/usr/local:" ; export PATH
HOSTNAME=`hostname`
LIBDIR=/usr/lib/uucp
UUCLEAN=${LIBDIR}/uuclean
CHOWN=/usr/etc/chown
SPOOLDIR=/usr/spool/uucp
STATSDIR=${SPOOLDIR}/STATS
#
#	if we are started up with -l, assume that we are being 
# given an account report from some site and that we need to save it.
#
if [ "$1"x = -xx ]; then
	DEBUG=true
	shift
fi
if [ "$1"x = -lx ]; then
	sed \
		-e "/^Received: /d" \
		-e "/^	id A[A-C][0-9]/d" \
		-e "/^Return-Path: /d" \
		-e "/^Posted-/d" \
		-e "/^Message-Id: /d" \
		-e "/^Status: /d" \
		-e '/^UUCP ANALYZER:/,$d' > /tmp/uucpstats.$$
	SUB=`grep '^Subject: .* UUCP GENERAL ACCOUNTING' /tmp/uucpstats.$$`
	set ${SUB:=none}
	if [ ! "$3" = UUCP -o ! "$4" = GENERAL -o ! "$5" = ACCOUNTING ]; then
		cat /tmp/uucpstats.$$ >> ${STATSDIR}/trash
		${CHOWN} -f uucp ${STATSDIR}/trash
		rm -f /tmp/uucpstats.$$
		exit 0
	fi
	if [ ! -d ${STATSDIR} ]; then
		mkdir ${STATSDIR}
		chmod 777 ${STATSDIR}
		${CHOWN} -f uucp ${STATSDIR}
	fi
	DIR=${STATSDIR}/`echo $2 | sed 's,/.*,,'`
	if [ ! -d ${DIR} ]; then
		mkdir ${DIR}
		chmod 777 ${DIR}
		${CHOWN} -f uucp ${DIR}
	fi
	FILE=`echo $2 | sed -e 's/Jan/01/' -e 's/Feb/02/' -e 's/Mar/03/' \
		-e 's/Apr/04/' -e 's/May/05/' -e 's/Jun/06/' \
		-e 's/Jul/07/' -e 's/Aug/08/' -e 's/Sep/09/' \
		-e 's/Oct/10/' -e 's/Nov/11/' -e 's/Dec/12/' \
		-e 's/\([0-9][0-9]\)\.\([0-9][0-9]\)\.\(19[0-9][0-9]\)/\3.\1.\2/'`
	if [ -f ${STATSDIR}/${FILE} ]; then
		mv /tmp/uucpstats.$$ ${STATSDIR}/${FILE}.$$
		${CHOWN} -f uucp ${STATSDIR}/${FILE}.$$
	else
		mv /tmp/uucpstats.$$ ${STATSDIR}/${FILE}
		${CHOWN} -f uucp ${STATSDIR}/${FILE}
	fi
	exit 0
fi
set `date`
DAY=$1
MON=$2
YEAR=$6
#
#	DAYMON: Day of the month 1-31
#
DAYMON=$3
#
#	MONTH: Change Month name into a number
#
MONTH=`echo ${MON} | sed -e 's/Jan/01/' -e 's/Feb/02/' -e 's/Mar/03/' \
	-e 's/Apr/04/' -e 's/May/05/' -e 's/Jun/06/' \
	-e 's/Jul/07/' -e 's/Aug/08/' -e 's/Sep/09/' \
	-e 's/Oct/10/' -e 's/Nov/11/' -e 's/Dec/12/' \
	-e 's/\([0-9][0-9]\)\.\([0-9][0-9]\)\.\(19[0-9][0-9]\)/\3.\1.\2/'`
#
#	prefix 1-9 days with 0 so that ls(1)'s work on listing the 
#	STATS directory and keep the files in chronological order.
#
if [ ${DAYMON} -lt 10 ]; then
	DAYMON=0${DAYMON}
fi
#
#	DATANAME: e.g. 1989.03.03       Year, Month, Day
#
DATENAME=${YEAR}.${MONTH}.${DAYMON}
HOURS=720
if [ ! -n "$DEBUG" ]; then
	SYS=${SPOOLDIR}/SYSLOG.$$
else
	SYS=${SPOOLDIR}/SYSLOG
fi

#
#	STATSPERSON - place to email these stats to. this is really not
#	a person but a archival script
#
STATSPERSON=uucpstats@localhost.com
#
#	L-costs has a list of sites and cost in $/minute. e.g.
#		sun             cost    0.34
#		housun          cost    0.40
#	keyword 'cost' is required.
#
if [ -f /etc/uucp/L-costs ]; then
	COSTS=/etc/uucp/L-costs
elif [ -f /usr/lib/uucp/L-costs ]; then
	COSTS=/usr/lib/uucp/L-costs
else
	COSTS=/dev/null
fi
#
#
#	LAST_REPORT - most recent report (yesterdays). used to 
#	general cost_to_date estimates.
#
LASTREPORT=${STATSDIR}/LAST_REPORT
if [ ! -f ${LASTREPORT} ]; then
	touch ${LASTREPORT}
fi
#
#	Figure out where the Logging directory is
#
echo hi
if [ -d ${SPOOLDIR}/OLD ]; then
	LOGDIR=${SPOOLDIR}/OLD
elif [ -d ${SPOOLDIR}/LOG ]; then
	LOGDIR=${SPOOLDIR}/LOG
else
	LOGDIR=/tmp
fi
LASTLOG=${LOGDIR}/LOGFILE.last
#
#	check to see if the stats directory is there and create it if not
#
if [ ! -d ${STATSDIR} ]; then
	mkdir ${STATSDIR}
	chmod 777 ${STATSDIR}
	${CHOWN} -f uucp ${STATSDIR}
fi
REPORTFILE=${DATENAME}
NEWREPORT=${STATSDIR}/${REPORTFILE}
if [ -f ${NEWREPORT} ]; then
	NEWREPORT=${STATSDIR}/${REPORTFILE}.$$
fi
#
#	UUclean a bunch of things
#
if [ ! -n "$DEBUG" ]; then 
	for i in \
		${SPOOLDIR}/AUDIT \
		${SPOOLDIR}/CORRUPT \
		${SPOOLDIR}/LCK \
		${SPOOLDIR}/STST \
		${SPOOLDIR}/TM. \
		${SPOOLDIR}/X. \
		${SPOOLDIR}/XTMP \
		${SPOOLDIR}/C. \
		${SPOOLDIR}/D. \
		${SPOOLDIR}/D.${HOSTNAME} \
		${SPOOLDIR}/D.${HOSTNAME}X
	do
		if [ -d ${i} ]; then
			$UUCLEAN -p -n${HOURS} -d${i}
		fi
	done
fi

if [ ! -n "$DEBUG" ]; then $UUCLEAN -pSTST. -pTM. -pLTMP. -pLOG. -pX. -n4 -d${SPOOLDIR} ; fi
if [ ! -n "$DEBUG" ]; then $UUCLEAN -pLCK. -n8 -d${SPOOLDIR} ; fi
if [ ! -n "$DEBUG" ]; then $UUCLEAN -d${LIBDIR}/.XQTDIR -p -n72 ; fi

#
# Begin new log at beginning of each month if either this is the first
# day of the month or uucp.startnewmonth has something in it. (since /usr/adm
# is owned by root and it is uncertain who cron runs as, then modify the file
# instead of deleting/creating it to indicate change).
#
#	if uucp.newmonth has the keywords 'newmonth <some_month>',
#	awk will assume this is a new month.
#
if [ ${DAYMON} = 01 ] ; then
	NEWMONTH="newmonth ${MON}"
else
	NEWMONTH="${MON}"
fi

# Old spool/log files are kept by the naming scheme:
# LOGFILE.${YEAR}.${MONTH}.${DAYMON} where ${DAY} is the day of the week,
# Sun to Sat, and ${DAYMON} is the numerical day of the month.  SYSLOG
# files are kept by the scheme:  SYSLOG.week: the current week's: totals;
# SYSLOG.month:  the current month's totals; SYSLOG.${month} where
# ${month} is the first three letters of the month: the totals for that
# month.

if [ ! -n "$DEBUG" ]; then
	touch ${SPOOLDIR}/LOGFILE
	${CHOWN} -f uucp ${SPOOLDIR}/LOGFILE
	touch ${LOGDIR}/LOGFILE.${DATENAME}
	${CHOWN} -f uucp ${LOGDIR}/LOGFILE.${DATENAME}
	mv ${SPOOLDIR}/LOGFILE ${LOGDIR}/LOGFILE.${DATENAME}
	rm -f ${LASTLOG} 
	ln ${LOGDIR}/LOGFILE.${DATENAME} ${LASTLOG}
	touch ${SPOOLDIR}/SYSLOG
	${CHOWN} -f uucp ${SPOOLDIR}/SYSLOG
	touch ${SPOOLDIR}/SYSLOG.$$
	${CHOWN} -f uucp ${SPOOLDIR}/SYSLOG.$$
	mv ${SPOOLDIR}/SYSLOG ${SPOOLDIR}/SYSLOG.$$
	touch ${SPOOLDIR}/SYSLOG
	${CHOWN} -f uucp ${SPOOLDIR}/SYSLOG
	touch ${SPOOLDIR}/LOGFILE
	${CHOWN} -f uucp ${SPOOLDIR}/LOGFILE
	cat ${SPOOLDIR}/SYSLOG.$$ >> ${LOGDIR}/SYSLOG.week
	if [ -f ${SPOOLDIR}/ERRLOG ]; then
		touch ${SPOOLDIR}/ERRLOG
		${CHOWN} -f uucp ${SPOOLDIR}/ERRLOG
		touch ${LOGDIR}/ERRLOG.${DATENAME}
		${CHOWN} -f uucp ${LOGDIR}/ERRLOG.${DATENAME}
		mv ${SPOOLDIR}/ERRLOG ${LOGDIR}/ERRLOG.${DATENAME}
		touch ${SPOOLDIR}/ERRLOG
		${CHOWN} -f uucp ${SPOOLDIR}/ERRLOG
	fi
fi
#
# make sure these are there when cat'ed below else the script could die
#
# send syslog stats to administrator.
#
# b - total bytes
# c - costs in $ ATT
# d - dialout to site
# f - failed attempts to call site
# o - total cost to date in $
# r - bytes recieved
# s - bytes sent
# t - total time in seconds
# v - conversations with site
# x - files sent
# y - files recieved
#
(
echo " "
echo "                             ${HOSTNAME}"
echo "                      UUCP GENERAL ACCOUNTING"
echo "                         ${DAY} ${MON} ${DAYMON} ${YEAR}"
echo "Site       Bytes   Time         Bytes      Calls    Cost ($ ATT)   Files  BPS Fail"
echo "           Total hh:mm:ss    Out     In   Out Tot  Today  To_Date Out In"
(
(
    echo hostname ${HOSTNAME}
    echo ${NEWMONTH}
    cat ${LASTREPORT} ${LASTLOG} ${COSTS} ${SYS} ) | awk ' { \
	  if ( $1 == "hostname" )           { hostname = $2 } \
	  if ( $1 == "newmonth" )           { newmonth = 1 } \
          if ( $6 == "data" )               { b[ $2 ] += $7 ; t[ $2 ] += $9 } \
	  if ( $5 == "sent" )               { s[ $2 ] += $7 ; x[ $2 ] += 1 } \
	  if ( $5 == "received" )           { r[ $2 ] += $7 ; y[ $2 ] += 1 } \
	  if ( $2 == "cost" && ( $4 == "" || $4 == hostname )) \
				            { c[ $1 ] = $3 } \
	  if ( $8 == "$" && newmonth == 0 ) { o[ $1 ] = $10 } \
	  if ( $1 == "|" && newmonth == 0 ) { days = $3 } \
	  if ( $4 == "SUCCEEDED" && $5 == "(call" ) { d[ $2 ] += 1 } \
	  if ( $4 == "FAILED" && $5 == "(call" ) { f[ $2 ] += 1 } \
	  if ( $5 == "(conversation" && $6 == "complete)" ) { v[ $2 ] += 1 } \
	}  \
	END { for ( i in b ) {  \
	printf("%-8.8s %7d %2d:%02d:%02d %7d %7d %2d %2d $ %6.2f %7.2f %3d %3d %4d %3d\n", \
	i, b[i], t[i]/3600, (t[i]%3600)/60, (t[i]%3600)%60, s[i], r[i], \
	d[i], v[i], (t[i]/60)*c[i], ((t[i]/60)*c[i])+o[i], x[i]/2, y[i]/2, \
	b[i]/(t[i]+1), f[i]) ; \
	totalbytes += b[i] ; day += (t[i]/60)*c[i] ; \
	todate += ((t[i]/60)*c[i])+o[i] } \
	printf("| Days: %3d $/day: $ %6.2f            Total:   $ %6.2f %7.2f Bytes: %d\n", \
	(days + 1), todate/(days + 1), day, todate, totalbytes ) }' ) | \
	sort ) > /tmp/.uucp.day.report

if [ ! -n "$DEBUG" ]; then
	mv /tmp/.uucp.day.report ${NEWREPORT}
	${CHOWN} -f uucp ${NEWREPORT}
	if [ -n "${STATSPERSON}" ]; then
		Mail -s "${HOSTNAME}/${REPORTFILE} UUCP GENERAL ACCOUNTING" ${STATSPERSON} < ${NEWREPORT}
	fi
	rm -f ${LASTREPORT}
	ln -s `basename ${NEWREPORT}` ${LASTREPORT}
	rm -f ${SYS}
fi

#
#  Remove LOGFILEs older than one week; save SYSLOG
#  files forever.  Remove them manually if desired.
#

if [ ! -n "$DEBUG" ]; then find ${LOGDIR} -name 'LOGFILE.*' -mtime +7 -exec rm -f {} \; ; fi

#
#  Poll morning systems
#
if [ ! -n "$DEBUG" ]; then
	for i in `uuname`
	do
		touch ${SPOOLDIR}/C./C.${i}n0000
	done
fi
exit 0



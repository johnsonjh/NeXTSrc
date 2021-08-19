/*  gtime  --  generate time from buffer
 *
 *  Usage:  t = gtime (buf);
 *	long t;
 *	struct tm *buf;
 *
 *  Gtime reconstructs the time from a buffer as produced by
 *  "localtime" using the algorithm of date (1).  Gtime is, in 
 *  fact, the inverse of "localtime".
 *
 *  HISTORY
 * 30-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Adapted for 4.2 BSD UNIX.  Changed to use new timezone conventions.
 *
 * 28-Dec-83  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fix one of many problems here.  Use 1969 as the time year base,
 *	since timezones allow a date of Dec 31, 1969.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Written by Mike Accetta.  If you don't like it, complain to him.
 *
 */

#include <sys/types.h>
#include <sys/time.h>

static	int	dmsize[12] =
{
	31,
	28,
	31,
	30,
	31,
	30,
	31,
	31,
	30,
	31,
	30,
	31
};

struct	tm *localtime();

long gtime(tm)
struct tm *tm;
{
	register int i;
	register secs, mins, day, hours, month, year;
	struct timeval tv;
	struct timezone tz;
	long	timbuf;

	secs = tm->tm_sec;
	mins = tm->tm_min;
	hours = tm->tm_hour;
	day = tm->tm_mday;
	month = tm->tm_mon + 1;
	year = tm->tm_year;
	if( month<1 || month>12 ||
	    day<1 || day>31 ||
	    hours<0 || hours>24 ||
	    mins<0 || mins>59 ||
	    secs<0 || secs>59)
		return(-1);
	if (hours==24) {
		hours=0; day++;
	}
	timbuf = 0;
	year += 1900;
	/* Use 1969 because of timezone effect */
	for(i=1969; i<year; i++)
		timbuf += dysize(i);
	/* Leap year */
	if (dysize(year)==366 && month >= 3)
		timbuf++;
	while(--month)
		timbuf += dmsize[month-1];
	timbuf += day-1;
	timbuf = 24*timbuf + hours;
	timbuf = 60*timbuf + mins;
	timbuf = 60*timbuf + secs;
	gettimeofday (&tv,&tz);
	timbuf += (tz.tz_minuteswest) * 60;
	/* now adjust back to 1970 since all time increments are done */
	timbuf -= dysize(1969) * 24 * 60 * 60;
	/* now fix up local daylight time */
	if(localtime(&timbuf)->tm_isdst)
		timbuf -= 60*60;
	return(timbuf);

}

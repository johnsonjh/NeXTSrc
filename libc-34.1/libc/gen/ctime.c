/*
 * Copyright (c) 1987 Regents of the University of California.
 * This file may be freely redistributed provided that this
 * notice remains attached.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)ctime.c	1.1 (Berkeley) 3/25/87";
#endif /* LIBC_SCCS and not lint */

#include "sys/param.h"
#include "sys/time.h"
#include "tzfile.h"

#include <stdio.h>
#include <string.h>

char *
ctime(const time_t *t) {
	struct tm	*localtime();
	char	*asctime();

	return(asctime(localtime(t)));
}

/*
** A la X3J11
*/

char *
asctime(register const struct tm *	timeptr) {
	static const char	wday_name[DAYS_PER_WEEK][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char	mon_name[MONS_PER_YEAR][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char	result[26] = {0};

	(void) sprintf(result, "%.3s %.3s%3d %02d:%02d:%02d %d\n",
		wday_name[timeptr->tm_wday],
		mon_name[timeptr->tm_mon],
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		TM_YEAR_BASE + timeptr->tm_year);
	return result;
}

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif /* !TRUE */

extern char *		getenv();
extern char *		strcpy();
extern char *		strcat();
static struct tm *offtime(const time_t *clock, long offset);

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
};

struct state {
	int		timecnt;
	int		typecnt;
	int		charcnt;
	time_t		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[TZ_MAX_CHARS + 1];
};

#if	NeXT
/*
 * Lazy allocate global variables for VM efficiency
 */
typedef struct _ctimeglobals {
	struct state	s;
	int		tz_is_set;
} ctimeglobals_t;

static ctimeglobals_t *
getctimeglobalsp(void)
{
	static ctimeglobals_t *ctimeglobalsp = 0;
	extern void *calloc(unsigned, unsigned);

	if (!ctimeglobalsp) {
		ctimeglobalsp = calloc(1, sizeof(*ctimeglobalsp));
	}
	return (ctimeglobalsp);
}

#define s getctimeglobalsp()->s
#define tz_is_set getctimeglobalsp()->tz_is_set

#else	NeXT
static struct state	s;
  
static int		tz_is_set;
#endif	NeXT

extern char *tzname[2];

#ifdef USG_COMPAT
time_t			timezone = 0;
int			daylight = 0;
#endif /* USG_COMPAT */

static long
detzcode(codep)
char *	codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

static
tzload(name)
register char *	name;
{
	register int	i;
	register int	fid;

	if (name == 0 && (name = TZDEFAULT) == 0)
		return -1;
	{
		register char *	p;
		register int	doaccess;
		char		fullname[MAXPATHLEN];

		doaccess = name[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == 0)
				return -1;
			if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
				return -1;
			(void) strcpy(fullname, p);
			(void) strcat(fullname, "/");
			(void) strcat(fullname, name);
			/*
			** Set doaccess if '.' (as in "../") shows up in name.
			*/
			while (*name != '\0')
				if (*name++ == '.')
					doaccess = TRUE;
			name = fullname;
		}
		if (doaccess && access(name, 4) != 0)
			return -1;
		if ((fid = open(name, 0)) == -1)
			return -1;
	}
	{
		register char *			p;
		register struct tzhead *	tzhp;
		char				buf[sizeof s];

		i = read(fid, buf, sizeof buf);
		if (close(fid) != 0 || i < sizeof *tzhp)
			return -1;
		tzhp = (struct tzhead *) buf;
		s.timecnt = (int) detzcode(tzhp->tzh_timecnt);
		s.typecnt = (int) detzcode(tzhp->tzh_typecnt);
		s.charcnt = (int) detzcode(tzhp->tzh_charcnt);
		if (s.timecnt > TZ_MAX_TIMES ||
			s.typecnt == 0 ||
			s.typecnt > TZ_MAX_TYPES ||
			s.charcnt > TZ_MAX_CHARS)
				return -1;
		if (i < sizeof *tzhp +
			s.timecnt * (4 + sizeof (char)) +
			s.typecnt * (4 + 2 * sizeof (char)) +
			s.charcnt * sizeof (char))
				return -1;
		p = buf + sizeof *tzhp;
		for (i = 0; i < s.timecnt; ++i) {
			s.ats[i] = detzcode(p);
			p += 4;
		}
		for (i = 0; i < s.timecnt; ++i)
			s.types[i] = (unsigned char) *p++;
		for (i = 0; i < s.typecnt; ++i) {
			register struct ttinfo *	ttisp;

			ttisp = &s.ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			ttisp->tt_abbrind = (unsigned char) *p++;
		}
		for (i = 0; i < s.charcnt; ++i)
			s.chars[i] = *p++;
		s.chars[i] = '\0';	/* ensure '\0' at end */
	}
	/*
	** Check that all the local time type indices are valid.
	*/
	for (i = 0; i < s.timecnt; ++i)
		if (s.types[i] >= s.typecnt)
			return -1;
	/*
	** Check that all abbreviation indices are valid.
	*/
	for (i = 0; i < s.typecnt; ++i)
		if (s.ttis[i].tt_abbrind >= s.charcnt)
			return -1;
	/*
	** Set tzname elements to initial values.
	*/
	tzname[0] = tzname[1] = &s.chars[0];
#ifdef USG_COMPAT
	timezone = -s.ttis[0].tt_gmtoff;
	daylight = 0;
#endif /* USG_COMPAT */
	for (i = 1; i < s.typecnt; ++i) {
		register struct ttinfo *	ttisp;

		ttisp = &s.ttis[i];
		if (ttisp->tt_isdst) {
			tzname[1] = &s.chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
			daylight = 1;
#endif /* USG_COMPAT */ 
		} else {
			tzname[0] = &s.chars[ttisp->tt_abbrind];
#ifdef USG_COMPAT
			timezone = -ttisp->tt_gmtoff;
#endif /* USG_COMPAT */ 
		}
	}
	return 0;
}

static
tzsetkernel()
{
	struct timeval	tv;
	struct timezone	tz;
	char	*tztab();

	if (gettimeofday(&tv, &tz))
		return -1;
	s.timecnt = 0;		/* UNIX counts *west* of Greenwich */
	s.ttis[0].tt_gmtoff = tz.tz_minuteswest * -SECS_PER_MIN;
	s.ttis[0].tt_abbrind = 0;
	(void)strcpy(s.chars, tztab(tz.tz_minuteswest, 0));
	tzname[0] = tzname[1] = s.chars;
#ifdef USG_COMPAT
	timezone = tz.tz_minuteswest * 60;
	daylight = tz.tz_dsttime;
#endif /* USG_COMPAT */
	return 0;
}

static
tzsetgmt()
{
	s.timecnt = 0;
	s.ttis[0].tt_gmtoff = 0;
	s.ttis[0].tt_abbrind = 0;
	(void) strcpy(s.chars, "GMT");
	tzname[0] = tzname[1] = s.chars;
#ifdef USG_COMPAT
	timezone = 0;
	daylight = 0;
#endif /* USG_COMPAT */
}

void
tzset()
{
	register char *	name;

	tz_is_set = TRUE;
#ifdef notdef
	/*
	 * This causes horrible pollution of the ANSI C Clib
	 * name space, because it pulls in getpwnam() which calls
	 * in NetInfo and YP, which call in RPC and XDR.... You
	 * get the picture.  This breaks the current "indir" tools
	 * because we get a command line much too long.
	 */
	name = GetDefaultValue( 0, "TimeZone" );
	if (!name)
		name = getenv("TZ");
#else
	name = getenv("TZ");
#endif notdef
	if (!name || *name) {			/* did not request GMT */
		if (name && !tzload(name))	/* requested name worked */
			return;
		if (!tzload((char *)0))		/* default name worked */
			return;
		if (!tzsetkernel())		/* kernel guess worked */
			return;
	}
	tzsetgmt();				/* GMT is default */
}

struct tm *
localtime(const time_t *timep)
{
	register struct ttinfo *	ttisp;
	register struct tm *		tmp;
	register int			i;
	time_t				t;

	if (!tz_is_set)
		(void) tzset();
	t = *timep;
	if (s.timecnt == 0 || t < s.ats[0]) {
		i = 0;
		while (s.ttis[i].tt_isdst)
			if (++i >= s.timecnt) {
				i = 0;
				break;
			}
	} else {
		for (i = 1; i < s.timecnt; ++i)
			if (t < s.ats[i])
				break;
		i = s.types[i - 1];
	}
	ttisp = &s.ttis[i];
	/*
	** To get (wrong) behavior that's compatible with System V Release 2.0
	** you'd replace the statement below with
	**	tmp = offtime((time_t) (t + ttisp->tt_gmtoff), 0L);
	*/
	tmp = offtime(&t, ttisp->tt_gmtoff);
	tmp->tm_isdst = ttisp->tt_isdst;
	tzname[tmp->tm_isdst] = &s.chars[ttisp->tt_abbrind];
	tmp->tm_zone = &s.chars[ttisp->tt_abbrind];
	return tmp;
}

struct tm *
gmtime(const time_t *clock)
{
	register struct tm *	tmp;

	tmp = offtime(clock, 0L);
	tzname[0] = "GMT";
	tmp->tm_zone = "GMT";		/* UCT ? */
	return tmp;
}

static const int	mon_lengths[2][MONS_PER_YEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const int	year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

/*
 * mktime converts a localtime back to a single
 * calendar time value, allowing alterations to
 * the tm_year, tm_mon, tm_mday, tm_hour, tm_min and tm_sec fields.
 * The values in these fields can be out of range, so long
 * as the resulting calendar time fits in a positive time_t
 *
 * NeXT, Inc, 6-Dec-88 CCH
 */
 
time_t
mktime(struct tm *tmp) {
	time_t stamp;
	register long days;
	register int month, year, y;
	register const int *ip;

	year = tmp->tm_year + TM_YEAR_BASE;
	month = tmp->tm_mon;
	if (month < 0) {
	    year -= (11-month) / 12;
	    month = 11 - ((11-month) % 12);
	} else {
            year += month / 12;
	    month = month % 12;
	}
	days = (long) tmp->tm_mday - 1;
	ip = mon_lengths[isleap(year)];
	while (month>0) {
	    month--;
	    days += ip[month];
	}
	y = EPOCH_YEAR;
	while (y < year) {
	      days += (long) year_lengths[isleap(y)];
	      y++;
	}
	while (y > year) {
	      y--;
	      days -= (long) year_lengths[isleap(y)];
	}
        stamp = tmp->tm_sec
	      + SECS_PER_MIN*tmp->tm_min
	      + SECS_PER_HOUR*tmp->tm_hour
	      + SECS_PER_DAY*days
	      - tmp->tm_gmtoff;
	if (stamp < 0) {
	    return (time_t)-1;
	} else {
            memcpy(tmp, offtime(&stamp, tmp->tm_gmtoff), sizeof(struct tm));
	    return stamp;
	}
}

static struct tm *
offtime(const time_t *clock, long offset) {
	register struct tm *	tmp;
	register long		days;
	register long		rem;
	register int		y;
	register int		yleap;
	register const int *		ip;
	static struct tm	tm = { 0 };

	tmp = &tm;
	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	rem += offset;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = (int) (rem / SECS_PER_MIN);
	tmp->tm_sec = (int) (rem % SECS_PER_MIN);
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYS_PER_WEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for ( ; ; ) {
			yleap = isleap(y);
			if (days < (long) year_lengths[yleap])
				break;
			++y;
			days = days - (long) year_lengths[yleap];
		}
	else do {
		--y;
		yleap = isleap(y);
		days = days + (long) year_lengths[yleap];
	} while (days < 0);
	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;
	ip = mon_lengths[yleap];
	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days = days - (long) ip[tmp->tm_mon];
	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
	tmp->tm_zone = "";
	tmp->tm_gmtoff = offset;
	return tmp;
}

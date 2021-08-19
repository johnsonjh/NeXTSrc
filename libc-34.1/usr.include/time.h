/*
 * Copyright (c) 1983, 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)time.h	1.2 (Berkeley) 3/4/87
 */

/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#ifndef _TIME_H
#define _TIME_H

#ifndef __STRICT_BSD__
#include <stddef.h>

#define CLK_TCK 64
typedef unsigned long int clock_t;
#ifndef _TIME_T
#define _TIME_T
typedef long time_t;
#endif /* _TIME_T */

#endif /* __STRICT_BSD__ */

/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	long	tm_gmtoff;
	char	*tm_zone;
};

#ifdef __STRICT_BSD__
extern	struct tm *gmtime(), *localtime();
extern	char *asctime(), *ctime();
#else /* __STRICT_BSD__ */
/* ANSI C functions */
clock_t clock(void);
double difftime(time_t time1, time_t time0);
time_t mktime(struct tm *timeptr);
size_t strftime(char *s, size_t maxsize,
	const char *format, const struct tm *timeptr);

/* BSD and ANSI C functions */
time_t time(time_t *timer);
char *asctime(const struct tm *timeptr);
char *ctime(const time_t *timer);
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
#endif /* __STRICT_BSD__ */

#endif /* _TIME_H */

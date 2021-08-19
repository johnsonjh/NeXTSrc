/*
	perfTimerInc.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson

	This file has private stuff for doing timings.  It is used by the
	kit and pbs via #include'ing it in other source files.
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#ifdef DEBUG

#import <objc/objc.h>
#import <libc.h>
#import <dpsclient/dpsclient.h>
#import <sys/time.h>
#import <sys/resource.h>
#ifndef NON_KIT_TIMING
#import "appkitPrivate.h"
#endif


typedef struct {
    float time;			/* wall time */
    float PStime;		/* time spent in PostScript */
    float utime;		/* user time */
    float stime;		/* system time */
    const char *str;
    short level;
    BOOL didPing;
} timeMarker;

#ifndef NON_KIT_TIMING
#define MAX_MARKS 682
#else
#define MAX_MARKS 341
#endif
static timeMarker *Markers = NULL;
static timeMarker *firstMarker = NULL;
static timeMarker *lastMarker = NULL;
static char *argBuf = NULL;
static char *argCurr = NULL;
static int ctxtEOF = 0;

static double getTime();

static double baseTime = 0.0;

#define TV_TO_DOUBLE(t) 	((t).tv_sec + ((double)((t).tv_usec))/1000000)

void _NXInitTimeMarker(double time)
{
    if (!Markers) {
	Markers = NXZoneMalloc(NXDefaultMallocZone(), MAX_MARKS * sizeof(timeMarker));
	firstMarker = Markers + 1;
	lastMarker = Markers;
    }
    if (time > 0) {
	if (baseTime == 0.0)
	    getTime();		/* be sure base is initted */
	Markers[0].time = time - baseTime;
    } else
	Markers[0].time = getTime();
    Markers[0].str = "Launch time";
    Markers[0].level = 0;
    Markers[0].didPing = NO;
    firstMarker = Markers;
    ctxtEOF = 0;

#ifndef NON_KIT_TIMING
{
    timeMarker *mark;
    int bufCount;
    struct timeval *tv;
    const char **s;
    static const char *crtStrings[] = { "mach_init", "cthread_init", "_init_shlibs", "_objcInit", "top of main", NULL};

    if (_NXTimeBuffers[0].tv_sec) {
	for (tv = _NXTimeBuffers, bufCount = 0; tv->tv_sec; tv++, bufCount++)
	    ;
	for (mark = lastMarker; mark > firstMarker; mark--)
	    *(mark + bufCount) = *mark;
	s = crtStrings;
	for (tv = _NXTimeBuffers, mark = Markers+1; tv->tv_sec; tv++, mark++) {
	    mark->time = TV_TO_DOUBLE(*tv);
	    mark->str = *s;
	    if (*s)
		s++;
	    mark->level = 0;
	    lastMarker++;
	}
    }
}
#endif
}

void _NXSetTimeMarker(const char *str, short level)
{
    struct rusage ru;

    if (!Markers) {
	Markers = NXZoneMalloc(NXDefaultMallocZone(), MAX_MARKS * sizeof(timeMarker));
	firstMarker = Markers + 1;
	lastMarker = Markers;
    }
    if (lastMarker < Markers+MAX_MARKS-1) {
#ifndef NON_KIT_TIMING
	int newCtxtEOF;
	DPSContext ctxt = DPSGetCurrentContext();

	if (lastMarker >= firstMarker && ctxt) {
	    newCtxtEOF = (*(NXStream **)((char *)ctxt-4))->eof;
	    if (newCtxtEOF != ctxtEOF) {
		lastMarker->PStime = getTime();
		NXPing();
		lastMarker->didPing = YES;
		ctxtEOF = newCtxtEOF;
	    } else
		lastMarker->didPing = NO;
	}
#endif
	lastMarker++;
	getrusage(0, &ru);
	lastMarker->utime = TV_TO_DOUBLE(ru.ru_utime);
	lastMarker->stime = TV_TO_DOUBLE(ru.ru_stime);
	lastMarker->time = getTime();
	lastMarker->str = str;
	lastMarker->level = level;
    }
}

#define MAX_MSG_BUF		(1024 * 32)

void _NXSetTimeMarkerWithArgs(const char *str, int arg1, int arg2, short level)
{
    if (!argBuf)
	argCurr = argBuf = NXZoneMalloc(NXDefaultMallocZone(), MAX_MSG_BUF);
    sprintf(argCurr, str, arg1, arg2);
    _NXSetTimeMarker(argCurr, level);
    argCurr += strlen(argCurr) + 1;
    if (argCurr > argBuf + MAX_MSG_BUF - 256)
	argCurr = argBuf + MAX_MSG_BUF - 256;
}


void _NXClearTimeMarkers(void)
{
    lastMarker = Markers;
    ctxtEOF = 0;
}


void _NXDumpTimeMarkers(void)
{
    timeMarker *start, *end;
    int i;

    _NXSetTimeMarker("Data dump", 0);
    for (start = firstMarker; start < lastMarker; start++) {
	for (end = start+1; end <= lastMarker; end++)
	    if (end->level <= start->level)
		break;
	if (end > lastMarker)
	    end = lastMarker;
	for (i = start->level; i--; )
	    fprintf(stderr, "    ");
	fprintf(stderr, "%8.4lf %8.4lf %8.4lf  ",
				end->time - start->time,
				(start+1)->time - start->time,
				end->time - Markers->time);
	fprintf(stderr, "%8.4lf %8.4lf ",
				end->utime - start->utime,
				(start+1)->utime - start->utime);
	fprintf(stderr, "%8.4lf %8.4lf  ",
				end->stime - start->stime,
				(start+1)->stime - start->stime);
	fprintf(stderr, "%c%s\n", start->didPing ? '*' : ' ', start->str);
    }
    fprintf(stderr, "\n");
}


#ifndef NON_KIT_TIMING
void _NXProfileMarker(char code, BOOL flag)
{
    if (_NXProfileString &&  strchr(_NXProfileString, code))
	moncontrol(flag);
}
#endif

/* returns the time in seconds.  A 52 bit mantissa
   in a IEEE double gives us 16-17 digits of accuracy.  Taking off six digits
   for the microseconds in a struct timeval, this leaves 10 digits of seconds,
   which is ~300 years.
*/
static double getTime()
{
    struct timeval now;
    double ret;

    gettimeofday(&now, NULL);
    ret = TV_TO_DOUBLE(now);
    if (baseTime == 0.0)
	baseTime = ret - 10;
    return ret - baseTime;
}


#endif


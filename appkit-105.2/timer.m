/*
	timer.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
	  
	DEFINED IN: The Application Kit
	HEADER FILES: timer.h
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "timer.h"
#import "nextstd.h"
#import <dpsclient/dpsNeXT.h>

#include "perfTimerInc.m"	/* shared code between kit and pbs */

extern NXHandler *_NXAddAltHandler(void (*proc)(void *data, int code,
					  void *data1, void *data2),
				   void *context);
extern void _NXRemoveAltHandler(NXHandler *errorData);

/* handles timed entry for Timer events */
static void doTimer(DPSTimedEntry te, double now, void *timerParam)
{
    NXEvent             ev;
    NXTrackingTimer    *timer = timerParam;
    extern NXEvent     *_DPSGetQEntry();	/* hack to get privates */

    bzero(&ev, sizeof(NXEvent));
    ev.type = NX_TIMER;
    ev.ctxt = DPSGetCurrentContext();
    if (!_DPSGetQEntry(ev.ctxt, NX_TIMERMASK)) {
	DPSPostEvent(&ev, FALSE);
	if (timer->firstTime) {
	    timer->firstTime = NO;
	    if (timer->delay != timer->period) {
		DPSRemoveTimedEntry(timer->te);
		timer->te = DPSAddTimedEntry(timer->period, doTimer, timer,
					     NX_MODALRESPTHRESHOLD + 1);
	    }
	}
    }
}


static void timerCleanup(void *data, int code, void *data1, void *data2)
{
    NXTrackingTimer *timer = (NXTrackingTimer *)data;

    DPSRemoveTimedEntry(timer->te);
    if (timer->freeMe)
	NX_FREE(timer);
    DPSDiscardEvents(DPSGetCurrentContext(), NX_TIMERMASK);
}


NXTrackingTimer *NXBeginTimer(NXTrackingTimer *timer, double delay, double period)
{
    if (!timer) {
	NX_ZONEMALLOC(NXDefaultMallocZone(), timer, NXTrackingTimer, 1);
	timer->freeMe = YES;
    } else
	timer->freeMe = NO;
    timer->delay = delay;
    timer->period = period;
    timer->firstTime = YES;
    timer->te = DPSAddTimedEntry(delay, doTimer, timer,
				 NX_MODALRESPTHRESHOLD + 1);
    timer->errorData = _NXAddAltHandler(&timerCleanup, timer);
    return timer;
}


void NXEndTimer(NXTrackingTimer *timer)
{
    _NXRemoveAltHandler(timer->errorData);
    timerCleanup(timer, 0, NULL, NULL);
}


/*
  
Modifications (starting at 0.8):
  
12/10/88 trey	File creation.  NXStartTimer and NXEndTimer added.

0.91
----
 5/19/89 trey	added time marking code for profiling with walltime
		made timer events not get posted is they are already in queue

0.92
----
 6/12/89 trey	use Alt Handlers to ensure timers are always turned off
		 on errors

83
--
 5/01/90 trey	mades pings for time markers automatic
		added _NXProfileMarker

85
--
 5/28/90 trey	timer events are bzero'ed before being posted

89
--
 7/19/90 trey	moved timer stuff into pefTimer.m for use by pbs

*/


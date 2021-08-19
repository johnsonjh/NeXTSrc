/*
	timer.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Application.h"
#import <objc/error.h>

/*
   Information used by the system between calls to NXBeginTimer and
   NXEndTimer.  This can either be allocated on the stack frame of
   the caller, or by NXBeginTimer.  The application should not
   access any of the elements of this structure.
*/

typedef struct _NXTrackingTimer {
    double delay;
    double period;
    DPSTimedEntry te;
    BOOL freeMe;
    BOOL firstTime;
    NXHandler *errorData;
    int reserved1;
    int reserved2;
} NXTrackingTimer;

/* used for initial delay and periodic behavior in tracking loops */

extern NXTrackingTimer *NXBeginTimer(NXTrackingTimer *timer, double delay, double period);
extern void NXEndTimer(NXTrackingTimer *timer);

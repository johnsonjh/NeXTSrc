/*
	NXJournalListener.m
	Copyright 1989, NeXT, Inc.
	Responsibility: Bill Tschumy
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXJournaler_Private.h"
#import "NXJournalListener.h"
#import "Application.h"
#import <dpsclient/dpsclient.h>
#import <stdlib.h>
#import <stddef.h>

@implementation  NXJournalListener : Listener

static NXRemoteMethod *remoteMethods = NULL;
#define REMOTEMETHODS 5


- (int)_activateApp
{
    [NXApp activateSelf:YES];
    return 0;
}


+ initialize 
{
    if (!remoteMethods) {
	remoteMethods =
	      (NXRemoteMethod *) malloc((REMOTEMETHODS + 1) * sizeof(NXRemoteMethod));
	remoteMethods[0].key = @selector(_playJournalEventType:
					 x:y:
					 time:
					 flags:
					 window:
					 subtype:
					 miscL0:miscL1:
					 ctxt:);
	remoteMethods[0].types = "iddiiiiiii";
	remoteMethods[1].key = @selector(_recordJournalEventType:
					 x:y:
					 time:
					 flags:
					 window:
					 subtype:
					 miscL0:miscL1:
					 ctxt:);
	remoteMethods[1].types = "iddiiiiiii";
	remoteMethods[2].key = @selector(_setStatus:);
	remoteMethods[2].types = "i";
	remoteMethods[3].key = @selector(_activateApp);
	remoteMethods[3].types = "";
	remoteMethods[4].key = @selector(_userAbort);
	remoteMethods[4].types = "";
	remoteMethods[REMOTEMETHODS].key = (SEL)NULL;
    }
    return nil;
}


- (int)performRemoteMethod:(NXRemoteMethod *) method paramList:(NXParamValue *) paramList
{
    switch (method - remoteMethods) {
    case 0:
	return[self _playJournalEventType:paramList[0].ival
	       x:paramList[1].dval y:paramList[2].dval
	       time:paramList[3].ival
	       flags:paramList[4].ival
	       window:paramList[5].ival
	       subtype:paramList[6].ival
	       miscL0:paramList[7].ival
	       miscL1:paramList[8].ival
	       ctxt:paramList[9].ival];
    case 1:
	return[self _recordJournalEventType:paramList[0].ival
	       x:paramList[1].dval y:paramList[2].dval
	       time:paramList[3].ival
	       flags:paramList[4].ival
	       window:paramList[5].ival
	       subtype:paramList[6].ival
	       miscL0:paramList[7].ival
	       miscL1:paramList[8].ival
	       ctxt:paramList[9].ival];
    case 2:
	return[self _setStatus:paramList[0].ival];
    case 3:
        return[self _activateApp];
    case 4:
        return[self _userAbort];
    default:
	return[super performRemoteMethod:method paramList:paramList];
    }
}


- (int)_playJournalEventType:(int)type
    x:(double)x y:(double)y
    time:(int)time
    flags:(int)flags
    window:(int)window
    subtype:(int)subtype
    miscL0:(int)miscL0
    miscL1:(int)miscL1
    ctxt:(int)context
{
    NXEvent event, *eventPtr = &event;
    
    eventPtr->type = type;
    eventPtr->location.x = x;
    eventPtr->location.y = y;
    eventPtr->time = time;
    eventPtr->flags = flags;
    eventPtr->window = window;
    eventPtr->data.compound.subtype = subtype;
    eventPtr->data.compound.misc.L[0] = miscL0;
    eventPtr->data.compound.misc.L[1] = miscL1;
    eventPtr->ctxt = (DPSContext)context;
    
    return [delegate _playJournalEvent:eventPtr];
}


- (int)_recordJournalEventType:(int)type
    x:(double)x y:(double)y
    time:(int)time
    flags:(int)flags
    window:(int)window
    subtype:(int)subtype
    miscL0:(int)miscL0
    miscL1:(int)miscL1
    ctxt:(int)context
{
    NXEvent event, *eventPtr = &event;
    
    eventPtr->type = type;
    eventPtr->location.x = x;
    eventPtr->location.y = y;
    eventPtr->time = time;
    eventPtr->flags = flags;
    eventPtr->window = window;
    eventPtr->data.compound.subtype = subtype;
    eventPtr->data.compound.misc.L[0] = miscL0;
    eventPtr->data.compound.misc.L[1] = miscL1;
    eventPtr->ctxt = (DPSContext)context;
    
    return [delegate _recordJournalEvent:eventPtr];
}


- (NXRemoteMethod *) remoteMethodFor:(SEL) aSel
{
    NXRemoteMethod *rm;

    if (rm = NXRemoteMethodFromSel(aSel, remoteMethods))
	return rm;
    return[super remoteMethodFor:aSel];
}


- (int)_setStatus:(int)newStatus
{
    return [delegate _setStatus:newStatus];
}


- (int)_userAbort
{
    return [delegate _userAbort];
}


@end


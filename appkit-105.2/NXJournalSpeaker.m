/*
	NXJournalSpeaker.m
  	Copyright 1989, NeXT, Inc.
	Responsibility: Bill Tschumy
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Application.h"
#import "Listener.h"
#import "NXJournaler.h"
#import "NXJournalSpeaker.h"

@implementation  NXJournalSpeaker :Speaker


- (int)_activateApp
{
    id              localReceiver;

    if (localReceiver = [self _localReceiver])
	return[localReceiver _activateApp];
    else
	return[self selectorRPC:"_activateApp" paramTypes:""];
}


- (int)_playJournalEvent:(NXEvent *)eventPtr
{
    id              localReceiver;

    if (localReceiver = [self _localReceiver])
	return[localReceiver _playJournalEvent:eventPtr];
    else
	return[self
	       selectorRPC:"_playJournalEventType:x:y:time:flags:window:subtype:miscL0:miscL1:ctxt:"
	       paramTypes:"iddiiiiiii",
	       eventPtr->type,
	       eventPtr->location.x, eventPtr->location.y,
	       eventPtr->time,
	       eventPtr->flags,
	       eventPtr->window,
	       eventPtr->data.compound.subtype,
	       eventPtr->data.compound.misc.L[0],
	       eventPtr->data.compound.misc.L[1],
	       eventPtr->ctxt];
}


- (int)_recordJournalEvent:(NXEvent *)eventPtr
{
    id              localReceiver;

    if (localReceiver = [self _localReceiver])
	return[localReceiver _recordJournalEvent:eventPtr];
    else
	return[self
	       selectorRPC:"_recordJournalEventType:x:y:time:flags:window:subtype:miscL0:miscL1:ctxt:"
	       paramTypes:"iddiiiiiii",
	       eventPtr->type,
	       eventPtr->location.x, eventPtr->location.y,
	       eventPtr->time,
	       eventPtr->flags,
	       eventPtr->window,
	       eventPtr->data.compound.subtype,
	       eventPtr->data.compound.misc.L[0],
	       eventPtr->data.compound.misc.L[1],
	       eventPtr->ctxt];
}


- (int)_setStatus:(int)newStatus
{
    id              localReceiver;

    if (localReceiver = [self _localReceiver])
	return[localReceiver _setStatus:newStatus];
    else
	return[self selectorRPC:"_setStatus:"
	       paramTypes:"i",
	       newStatus];
}


- (int)_userAbort
{
    id              localReceiver;

    if (localReceiver = [self _localReceiver])
	return[localReceiver _userAbort];
    else
	return[self selectorRPC:"_userAbort" paramTypes:""];
}


- _localReceiver
{
    id              slaveJournaler = [NXApp slaveJournaler];
    id              masterJournaler = [NXApp masterJournaler];

    if (slaveJournaler && masterJournaler) {
	if (sendPort == [[slaveJournaler listener] listenPort])
	    return slaveJournaler;
	else if (sendPort == [[masterJournaler listener] listenPort])
	    return masterJournaler;
    }
    return nil;
}

@end

/*

Modifications:

82
--
 5/4/90  wot	Changed all the speaker methods to deal with the case when the
 		masterJournaler and slaveJournaler are in the same process.  We
		bypasss the Listener/Speaker connection in this case.
*/	

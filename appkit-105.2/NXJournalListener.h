/*

	NXJournalListener.h
	Copyright 1989, NeXT, Inc.
	Responsibility: Bill Tschumy
  
	DEFINED IN: The Application Kit
	HEADER FILES: NXJournalListener.h
*/

#import "Listener.h"

@interface NXJournalListener : Listener
{
}

+ initialize;
- (int)_activateApp;
- (int)performRemoteMethod:(NXRemoteMethod *) method paramList:(NXParamValue *) paramList;
- (int)_playJournalEventType:(int)type    x:(double)x y:(double)y    time:(int)time    flags:(int)flags    window:(int)window    subtype:(int)subtype    miscL0:(int)miscL0    miscL1:(int)miscL1    ctxt:(int)context;
- (int)_recordJournalEventType:(int)type    x:(double)x y:(double)y    time:(int)time    flags:(int)flags    window:(int)window    subtype:(int)subtype    miscL0:(int)miscL0    miscL1:(int)miscL1    ctxt:(int)context;
- (NXRemoteMethod *) remoteMethodFor:(SEL) aSel;
- (int)_setStatus:(int)newStatus;
- (int)_userAbort;
@end


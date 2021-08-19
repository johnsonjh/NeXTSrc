/*

	NXJournalSpeaker.h
	Copyright 1989, NeXT, Inc.
	Responsibility: Bill Tschumy
  
	DEFINED IN: The Application Kit
	HEADER FILES: NXJournalSpeaker.h
*/

#import <dpsclient/dpsclient.h>
#import "Speaker.h"

@interface NXJournalSpeaker : Speaker
{
}

- (int)_activateApp;
- (int)_playJournalEvent:(NXEvent *)eventPtr;
- (int)_recordJournalEvent:(NXEvent *)eventPtr;
- (int)_setStatus:(int)newStatus;
- (int)_userAbort;
- _localReceiver;

@end

/*
	NXJournaler.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import <dpsclient/dpsclient.h>
#import	<sound/sound.h>

/* NX_JOURNALEVENT subtypes */

#define NX_WINDRAGGED		0
#define NX_MOUSELOCATION	1
#define NX_LASTJRNEVENT		2

/* Window encodings in .evt file */

#define NX_KEYWINDOW 		(-1)
#define NX_MAINWINDOW 		(-2)
#define NX_MAINMENU 		(-3)
#define NX_MOUSEDOWNWINDOW	(-4)
#define NX_APPICONWINDOW	(-5)
#define NX_UNKNOWNWINDOW	(-6)

/* Values for eventStatus and soundStatus */

#define NX_STOPPED	0
#define NX_PLAYING	1
#define NX_RECORDING	2

/* Values for recordDevice */

#define NX_CODEC	0
#define NX_DSP		1

#define NX_JOURNALREQUEST    "NXJournalerRequest"

typedef struct {
    int                 version;
    unsigned int        offsetToAppNames;
    unsigned int        lastEventTime;
    unsigned int        reserved1;
    unsigned int        reserved2;
}                   NXJournalHeader;

void NXJournalMouse(void);

@interface NXJournaler : Object
{

 /* Used exclusively by master journaler */
    long                _startTime;
    int                 _soundStatus;
    id                  _appsData;
    SNDSoundStruct     *_sndBuffer;
    char               *_soundfile;
    int                 _recordDevice;
    NXEvent             _nextEvent;
    NXStream           *_eventStream;
    DPSTimedEntry       _teNum;
    NXJournalHeader	_journalHeader;

 /* Used exclusively by slave journaler */
    id                  _mouseDownWindow;
    int                 _mouseDownWindowNum;
    int                 _applicationNum;
    int                 _dragWindowNum;
    unsigned int	_oldEventMask;
    DPSEventFilterFunc  _oldEventFilter;
    BOOL                _pendingStop;

 /* Used by both master and slave journalers */
    id                  _listener;
    id                  _speaker;
    id                  _delegate;
    int                 _eventStatus;
}

- init;

- free;
- journalerDidEnd:journaler;
- journalerDidUserAbort:journaler;
- delegate;
- setDelegate:anObject;
- getEventStatus:(int *)eventStatusPtr soundStatus:(int *)soundStatusPtr eventStream:(NXStream **)streamPtr soundfile:(char **)soundfilePtr;
- setEventStatus:(int)eventStatus soundStatus:(int)soundStatus eventStream:(NXStream *)stream soundfile:(const char *)soundfile;
- (int)recordDevice;
- setRecordDevice:(int)newRecordDevice;
- listener;
- speaker;

@end

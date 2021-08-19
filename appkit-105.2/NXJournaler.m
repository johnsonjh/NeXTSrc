/*
	NXJournaler.m
  	Copyright 1989, NeXT, Inc.
	Responsibility: Bill Tschumy
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXJournaler_Private.h"
#import "Application_Private.h"
#import "Listener_Private.h"
#import "NXJournalListener.h"
#import "NXJournalSpeaker.h"
#import "Window.h"
#import "errors.h"
#import "nextstd.h"
#import "privateWraps.h"
#import "publicWraps.h"
#import	"packagesWraps.h"
#import <soundkit/Sound.h>
#import <dpsclient/dpsclient.h>
#import <dpsclient/wraps.h>
#import <objc/Storage.h>
#import <streams/streams.h>
#import	<sys/time.h>
#import <cthreads.h>
#import <servers/netname.h>


int SNDStartRecordingFile(const char *file, SNDSoundStruct *s, int tag, int priority, int preempt, SNDNotificationFun beginFun, SNDNotificationFun endFun);

#define NO_EVENT_FILTER 	((DPSEventFilterFunc)-1)
#define NO_TIMED_ENTRY		((DPSTimedEntry)-1)
#define NO_PORT			((port_t)0)

#define	TICKS_PER_SECOND	(68.39)
#define ABORT_FLAGS_MASK 	(NX_ALTERNATEMASK)

#define MOUSE_EVENTS  	       (NX_LMOUSEDOWNMASK | \
				NX_LMOUSEUPMASK | \
				NX_LMOUSEDRAGGEDMASK | \
				NX_RMOUSEDRAGGEDMASK |	\
				NX_RMOUSEDOWNMASK | \
				NX_RMOUSEUPMASK | \
				NX_MOUSEMOVEDMASK)

typedef struct {
    char           *appName;
    port_t          sendPort;
}               AppData;
				
typedef struct { /* ??? wot -- This is a bit evil on my part */
    @defs (Window)
}            	*WindowId;


static port_t 	_connectToApp(const char *appName);
static void 	_convertLocation(id oldWindow, id newWindow, NXEvent * eventPtr);
static int 	_decodeWindowNum(unsigned int *numPtr, NXEvent * eventPtr, NXJournaler * self);
static void 	_encodeWindowNum(unsigned int *numPtr, NXEvent * eventPtr, NXJournaler * self);
static void 	_freeSndBuffer(NXJournaler *self);
static long     _getCurrentTime();
static long 	_getServerTime();
static int 	_lastJrnEvent(NXEvent *eventPtr);
static int 	_playEventFilter(NXEvent *eventPtr);
static void 	_playTEProc(DPSTimedEntry num, double now, NXJournaler * self);
static void 	_recordSndInit(NXJournaler *self);
static int 	_shouldRecord(NXEvent *eventPtr);
static void  	_sleepFor(int numSeconds);
static int 	_soundStarted(SNDSoundStruct *soundBuffer, int tag, int err);


static struct mutex soundStartedLock = MUTEX_INITIALIZER;	/* lock for accessing soundStarted */
static struct condition soundStartedCondition = CONDITION_INITIALIZER;
static BOOL soundStarted = NO;
static long	startOfTime = 0;
static NXJournalHeader defaultHeader =  {1 /* version */,
			  		 0 /* offsetToAppNames */,
			  		 0 /* lastEventTime */,
					 0 /* reserved1 */,
					 0 /* reserved2 */ };
static long playbackServerStartTime = 0;


@implementation  NXJournaler : Object

/********************************************************/
/******************* PUBLIC METHODS *********************/
/********************************************************/

- delegate
{
    return _delegate;
}


- free
{
    [_listener removePort];
    [_listener free];
    [_speaker free];
    [self _freeAppsData];
    if (_soundfile)
	free(_soundfile);
    _freeSndBuffer(self);
    if (self == [NXApp slaveJournaler])
	[NXApp _setSlaveJournaler:nil];
    if (self == [NXApp masterJournaler])
	[NXApp _setMasterJournaler:nil];
    return[super free];
}


- getEventStatus:(int *)eventStatusPtr
    soundStatus:(int *)soundStatusPtr
    eventStream:(NXStream **) streamPtr
    soundfile:(char **)soundfilePtr
{
    if (eventStatusPtr)
	*eventStatusPtr = _eventStatus;
    if (soundStatusPtr)
	*soundStatusPtr = _soundStatus;
    if (streamPtr)
	*streamPtr = _eventStream;
    if (soundfilePtr)
	*soundfilePtr = _soundfile;
    return self;
}


- journalerDidEnd:journaler
{
    return self;
}


- journalerDidUserAbort:journaler;
 {
    return self;
}


- listener
{
    return _listener;
}


- init
{
    [super init];
    [self _commonNew:NX_JOURNALREQUEST];
    NX_ASSERT (![NXApp masterJournaler], "NXJournaler -- creating more than one masterJournaler");
    [NXApp _setMasterJournaler:self];
    return self;
}


void NXJournalMouse()
{
#define JOURNAL_MOUSE_EVENTMASK (NX_JOURNALEVENTMASK | NX_MOUSEDRAGGEDMASK | NX_MOUSEUPMASK)

    NXJournaler    *slave;
    NXEvent	    junkEvent;

    slave = [NXApp slaveJournaler];
    if (slave && slave->_eventStatus != NX_STOPPED)
       (void)NXGetOrPeekEvent(DPSGetCurrentContext(),
			      &junkEvent,
			      JOURNAL_MOUSE_EVENTMASK,
			      0,
			      NX_MODALRESPTHRESHOLD,
			      0);
}


- (int)recordDevice
{
    return _recordDevice;
}


- setDelegate:anObject
{
    _delegate = anObject;
    return self;
}


- setEventStatus:(int)eventStatus
    soundStatus:(int)soundStatus
    eventStream:(NXStream *) stream
    soundfile:(const char *)soundfile;
{
#define SOUND_START_TIMEOUT	10	/* Time in seconds to wait for
					 * sound to start */

    int             numApps, i, err;
    AppData        *appDataPtr;
    NXEvent         theEvent;
    AppData         appData;
    char            appName[MAXPATHLEN + 1];
    NXZone         *zone = [self zone];

 /* First the events */
    if (eventStatus != _eventStatus) {
	switch (eventStatus) {

	case NX_PLAYING:
	    if (_eventStream = stream) {
		[self _initAppsData];
		NXSeek(_eventStream, 0, NX_FROMSTART);
		NXRead(_eventStream, &_journalHeader, sizeof(NXJournalHeader));
		NXSeek(_eventStream, _journalHeader.offsetToAppNames, NX_FROMSTART);
		NXRead(_eventStream, &numApps, sizeof(int));
		for (i = 0; i < numApps; i++) {
		    NXScanf(_eventStream, "%s", appName);
		    appData.appName = NXZoneMalloc(zone, strlen(appName) + 1);
		    strcpy(appData.appName, appName);
		    appData.sendPort = (port_t) 0;
		    [_appsData addElement:(void *)&appData];
		}
		NXSeek(_eventStream, sizeof(NXJournalHeader), NX_FROMSTART);
		if (![self _readEvent:&_nextEvent])
		    eventStatus = _eventStatus;	/* Set new eventStatus to
						 * current if there are no
						 * events to read. */
	    }
	    break;

	case NX_RECORDING:
	    if (_eventStream = stream) {
		[self _initAppsData];
		_journalHeader = defaultHeader;
		NXSeek(_eventStream, sizeof(NXJournalHeader), NX_FROMSTART);
	    }
	    break;

	case NX_STOPPED:
	    _NXUndoButtonDownOverride();
	    _NXSetJournalRecording(FALSE);
	    NXPing();
	    numApps = [_appsData count];
	/* Notify the slaves to stop. */
	    for (i = 0; i < numApps; i++) {
		appDataPtr = (AppData *)[_appsData elementAt:i];
		[_speaker setSendPort:appDataPtr->sendPort];
		[_speaker _setStatus:NX_STOPPED];
	    }
	    if (_eventStatus == NX_RECORDING) {
		theEvent.type = NX_JOURNALEVENT;
		theEvent.data.compound.subtype = NX_LASTJRNEVENT;
		theEvent.time = _getServerTime();
		[self _writeEvent:&theEvent];
		_journalHeader.offsetToAppNames = NXTell(_eventStream);
		NXWrite(_eventStream, &numApps, sizeof(int));
		for (i = 0; i < numApps; i++) {
		    appDataPtr = (AppData *)[_appsData elementAt:i];
		    NXPrintf(_eventStream, "%s\n", appDataPtr->appName);
		}
		NXSeek(_eventStream, 0, NX_FROMSTART);
		NXWrite(_eventStream, &_journalHeader, sizeof(NXJournalHeader));
	    } else {
		[self _clearTimedEntry];
	    }
	    break;
	}
    }
 /* Now the sound */
    soundStarted = YES;
    if (soundStatus != _soundStatus) {
	if (soundfile != _soundfile) {
	    if (_soundfile)
		free(_soundfile);
	    if (soundfile) {
		_soundfile = NXZoneMalloc(zone, strlen(soundfile) + 1);
		strcpy(_soundfile, soundfile);
	    } else 
		_soundfile = NULL;
	}
	switch (soundStatus) {
	case NX_PLAYING:
	    _freeSndBuffer(self);
	    if (_soundfile) {
	        soundStarted = NO;
		err = SNDReadSoundfile(_soundfile, &(_sndBuffer));
		if (err) {
		    NXLogError("NXJournaler -- Error %d calling SNDReadSoundfile.\n", err);
		    soundStarted = YES;
		}
		if (!err) {
		    if (_sndBuffer->dataFormat == SND_FORMAT_DSP_DATA_16)
			_sndBuffer->dataFormat = SND_FORMAT_LINEAR_16;
		    err = SNDStartPlaying(_sndBuffer, 1, 0, 0, _soundStarted, SND_NULL_FUN);
		    if (err) {
			NXLogError("NXJournaler -- Error %d calling SNDStartPlaying.\n", err);
			soundStarted = YES;
		    }
		}
	    } else
		soundStatus = NX_STOPPED;
	    break;
	case NX_RECORDING:
	    _freeSndBuffer(self);
	    if (_soundfile) {
	        soundStarted = NO;
		_recordSndInit(self);
		err = SNDStartRecordingFile(_soundfile, _sndBuffer,
					    1, 0, 0, _soundStarted,
					    SND_NULL_FUN);
		if (err) {
		    NXLogError("NXJournaler -- Error %d calling SNDStartRecordingFile.\n", err);
		    soundStarted = YES;
		}
	    } else
		soundStatus = NX_STOPPED;
	    break;
	case NX_STOPPED:
	    if (_soundStatus != NX_STOPPED) {
		err = SNDStop(1);
		if (err)
		    NXLogError("NXJournaler -- Error %d calling SNDStop.\n", err);
	    }
	    if (_soundStatus == NX_RECORDING)
	    /*
	     * We need to set the dataSize back to 0 so the buffer gets
	     * deallocated correctly 
	     */
		_sndBuffer->dataSize = 0;
	    _freeSndBuffer(self);
	    break;
	}
    }
    if (eventStatus != _eventStatus)
	if (stream) {
	    mutex_lock(&soundStartedLock);
	    if (!soundStarted)
	        cthread_detach(cthread_fork((any_t (*)())_sleepFor, (any_t)SOUND_START_TIMEOUT));
	    while (!soundStarted)
		condition_wait(&soundStartedCondition, &soundStartedLock);
	    mutex_unlock(&soundStartedLock);
	    if (eventStatus != _eventStatus) {
		switch (eventStatus) {
		case NX_PLAYING:
		    self->_startTime = _getCurrentTime();
		    playbackServerStartTime = _getServerTime();
		    _NXPrepareToOverride();
		    _NXOverrideButtonDown();
		    _NXFinishOverride();
		    _playTEProc(NO_TIMED_ENTRY, 0.0, self);	/* Call this directly to
								 * get things rolling */
		    break;
		case NX_RECORDING:
		    self->_startTime = _getServerTime();
		    _NXSetJournalRecording(TRUE);
		    NXPing();
		    break;
		}
	    }
	} else
	    eventStatus = NX_STOPPED;
    _eventStatus = eventStatus;
    _soundStatus = soundStatus;
    if (_eventStatus == NX_STOPPED &&
	_soundStatus == NX_STOPPED &&
	[_delegate respondsTo:@selector(journalerDidEnd:)])
	[_delegate journalerDidEnd:self];
    return self;
}


- setRecordDevice :(int)newRecordDevice
{
    _recordDevice = newRecordDevice;
    return self;
}


- speaker
{
    return _speaker;
}


/********************************************************/
/******************* PRIVATE METHODS ********************/
/********************************************************/

- _adjustNewEventMask:(int *)newMask for:window
 /*
  * Called from Window's setEventMask.  This adjustment is to make sure no
  * one removes NX_LMOUSEDRAGGEDMASK from the windows event mask while we
  * are recording and the mouse is down. 
  */
{
    if (_eventStatus == NX_RECORDING && window == _mouseDownWindow) {
        *newMask |= NX_LMOUSEDRAGGEDMASK;
    }
    return self;
}


- _clearEventFilter
 /*
  * Clears the current event filter used for event playback by the slave
  * journaler.  It restores any previous event filter that may have been
  * set before playback started. 
  */
{
    if (_oldEventFilter != NO_EVENT_FILTER) {
        DPSSetEventFunc([NXApp context], _oldEventFilter);
	_oldEventFilter = NO_EVENT_FILTER;
    }
    return self;
}


- _clearTimedEntry
 /*
  * Clears the current timed entry used by the master journaler to
  * dispatch events. 
  */
{
    if (_teNum != NO_TIMED_ENTRY) {
	DPSRemoveTimedEntry(_teNum);
	_teNum = NO_TIMED_ENTRY;
    }
    return self;
}


- _commonNew:(const char *)listenerName
 /*
  * Common code used by the factory  methods +new and +_newSlave. 
  * ListenerName is the name that the private listener should check in as. 
  * If the method is called to create a master journaler (i.e. +new), then
  * listenerName should be NX_JOURNALREQUEST.  If it is used to create a
  * slave journaler then NULL should be passed in listenerName and the
  * listener will register itself as a private port. 
  */
{
    NXZone *zone = [self zone];

    _listener = [[NXJournalListener allocFromZone:zone] init];
    if (listenerName)
	[_listener checkInAs:listenerName];
    else
	[_listener usePrivatePort];
    [_listener setDelegate:self];
    [_listener setPriority:NX_MODALRESPTHRESHOLD];
    [_listener addPort];
    _speaker = [[NXJournalSpeaker allocFromZone:zone] init];
    _oldEventFilter = NO_EVENT_FILTER;
    _teNum = NO_TIMED_ENTRY;

    return self;
}


static port_t _connectToApp(const char *appName)
{
    port_t          sport;
    int             i;

    for (i = 0; i < 10; i++) {
	if ((sport = NXPortNameLookup (appName,"")) != PORT_NULL) {
	    return sport;
	} else {
	    sleep(1);
	}
    }
    return NXPortFromName(appName, NULL);
}


static void _convertLocation(id oldWindow, id newWindow, NXEvent * eventPtr)
 /*
  * Converts the location of the event from oldWindow's coordinates to
  * that of newWindow's. If oldWindow is nil the location is assumed to
  * start in screen coords.  If newWindow is nil the location is converted
  * to screen coords.
  */
{
    NXRect          oldFrame, newFrame;

    oldFrame.origin.x = 0.0;
    oldFrame.origin.y = 0.0;
    newFrame.origin = oldFrame.origin;
    [oldWindow getFrame:&oldFrame];
    [newWindow getFrame:&newFrame];
    eventPtr->location.x += (oldFrame.origin.x - newFrame.origin.x);
    eventPtr->location.y += (oldFrame.origin.y - newFrame.origin.y);
}


- _decodeJournalEvent:(NXEvent *) eventPtr
 /*
  * This message is sent to the slave journaler to decode a journal event
  * before returning it from NXGetOrPeekEvent.  It decodes the window
  * number of the event and moves the cursor to the location of the event
  * if appropriate.  If the event is a LMOUSEDOWN the event is posted to
  * the packages so they can stay in synch. 
  */
{
    register WindowId drugWindow;
    float           junkX, junkY;
    int             eventType = eventPtr->type;
    int eventSubtype = eventPtr->data.compound.subtype;

    NX_ASSERT(eventPtr->flags & NX_JOURNALFLAGMASK, 
 	      "NXJournaler -- decoding non-journal event");
    if (eventType == NX_KITDEFINED && eventSubtype == NX_APPACT)
	_decodeWindowNum((unsigned int *)&eventPtr->data.compound.misc.L[1],
			 eventPtr, self);
    else if (eventType == NX_JOURNALEVENT) {
	if (eventSubtype == NX_WINDRAGGED) {
	    if (_decodeWindowNum(&eventPtr->window, eventPtr, self)) {
		drugWindow = (WindowId)[NXApp findWindow:eventPtr->window];
		_NXDragJnlWindow(eventPtr->window,
			      eventPtr->location.x, eventPtr->location.y,
				 &junkX, &junkY);
	    }
	} else if (eventSubtype == NX_MOUSELOCATION) {
	    if (_decodeWindowNum(&eventPtr->window, eventPtr, self))
		_NXSetMouse(eventPtr->window,
			    eventPtr->location.x,
			    eventPtr->location.y);
	}
    } else if (NX_EVENTCODEMASK(eventType) & MOUSE_EVENTS) {
	_dragWindowNum = 0;
	if (_decodeWindowNum(&eventPtr->window, eventPtr, self)) {
	    _NXSetMouse(eventPtr->window,
			eventPtr->location.x,
			eventPtr->location.y);
	    if (eventType == NX_LMOUSEDOWN) {
	    /* Post LMouseDown events to the packages. */
	        _NXSetJrnMDownEvtNum(eventPtr->data.mouse.eventNum);
		_NXlmdToPackages(eventType,
				 eventPtr->location.x,
				 eventPtr->location.y,
				 -1 /*eventPtr->time*/,
				 eventPtr->flags,
				 eventPtr->window,
				 eventPtr->data.compound.subtype,
				 eventPtr->data.compound.misc.L[0],
				 eventPtr->data.compound.misc.L[1]);
		if (_mouseDownWindowNum != eventPtr->window) {
		    _mouseDownWindowNum = eventPtr->window;
		    _mouseDownWindow = [NXApp findWindow:_mouseDownWindowNum];
		}
	    } else if (eventType == NX_LMOUSEUP)
	        _NXSetJrnMDownEvtNum(0);
	}
    }
    return self;
}


static int _decodeWindowNum(unsigned int *numPtr, NXEvent *eventPtr, NXJournaler *self)
 /*
  * Decodes the window number designation pointer to by numPtr to the
  * current window number for that designation.  Legal designations are
  * NX_KEYWINDOW, NX_MAINWINDOW, NX_MAINMENU, NX_MOUSEDOWNWINDOW,
  * NX_APPICONWINDOW, NX_UNKNOWNWINDOW. Returns the window number of the
  * window to recieve the event.  If the return value is 0, an appropriate
  * window couldn't be found. 
  */
{
    register id     mouseWindow;
    float           mouseX, mouseY;
    int             foundOne;

    switch (*numPtr) {
    case NX_KEYWINDOW:
	mouseWindow = [NXApp _keyWindow];
	break;
    case NX_MAINWINDOW:
	mouseWindow = [NXApp _mainWindow];
	break;
    case NX_MAINMENU:
	mouseWindow = [NXApp mainMenu];
	break;
    case NX_MOUSEDOWNWINDOW:
	mouseWindow = self->_mouseDownWindow;
	break;
    case NX_APPICONWINDOW:
        mouseWindow = [NXApp appIcon];
	break;
    case NX_UNKNOWNWINDOW:
	if (eventPtr->type == NX_JOURNALEVENT &&
	    eventPtr->data.compound.subtype == NX_WINDRAGGED) {
	    if (!(*numPtr = self->_dragWindowNum)) {
		_NXScreenMouse(&mouseX, &mouseY);
		PSfindwindow(mouseX, mouseY, NX_ABOVE, 0,
			     &mouseX, &mouseY, (int *)numPtr, &foundOne);
	    }
	} else
	    PSfindwindow(eventPtr->location.x, eventPtr->location.y, NX_ABOVE, 0,
			 &eventPtr->location.x, &eventPtr->location.y,
			 (int *)numPtr, &foundOne);
	if (foundOne && *numPtr != 14) {
	    NXConvertGlobalToWinNum((int)*numPtr, numPtr);
	    return *numPtr;
	} else
	    return 0;
    case 0:
        return 0;
    default:
	NX_ASSERT((eventPtr->type == NX_MOUSEDOWN || eventPtr->type == NX_MOUSEUP),
	         "NXJournaler -- Bad window designation in _decodeWindowNum");
	return 0;
    }
    return (*numPtr = [mouseWindow windowNum]);
}


- _disposeJournalEvents
 /*
  * Get rid of any events in the queue that have the JOURNALFLAG flag set. 
  */
{
    NXEvent         theEvent;
    BOOL            disposeEvent;

    do {
	disposeEvent = DPSPeekEvent([NXApp context], &theEvent,
				    NX_ALLEVENTS, 0.0, NX_BASETHRESHOLD);
	disposeEvent &= (theEvent.flags & NX_JOURNALFLAGMASK);
	if (disposeEvent)
	    DPSGetEvent([NXApp context], &theEvent,
			NX_ALLEVENTS, 0.0, NX_BASETHRESHOLD);
    } while (disposeEvent);
    return self;
}


static void _encodeWindowNum(unsigned int*numPtr, NXEvent * eventPtr, NXJournaler * self)
 /*
  * Encodes the window number pointed to by numPtr to the appropriate
  * designation.  Legal designations are NX_KEYWINDOW, NX_MAINWINDOW,
  * NX_MAINMENU, NX_MOUSEDOWNWINDOW, NX_APPICONWINDOW, NX_UNKNOWNWINDOW.  
  */
{
    register int    windowNum;

    if (windowNum = *numPtr) {
	if (windowNum == [[NXApp _keyWindow] windowNum])
	    windowNum = NX_KEYWINDOW;
	else if (windowNum == [[NXApp _mainWindow] windowNum])
	    windowNum = NX_MAINWINDOW;
	else if (windowNum == [[NXApp mainMenu] windowNum])
	    windowNum = NX_MAINMENU;
	else if (windowNum == self->_mouseDownWindowNum)
	    windowNum = NX_MOUSEDOWNWINDOW;
	else if (windowNum == [[NXApp appIcon] windowNum])
	    windowNum = NX_APPICONWINDOW;
	else {
	    if (!(eventPtr->type == NX_JOURNALEVENT &&
		  eventPtr->data.compound.subtype == NX_WINDRAGGED))
		_convertLocation([NXApp findWindow:windowNum], nil, eventPtr);
	    windowNum = NX_UNKNOWNWINDOW;
	}
	*numPtr = windowNum;
    }
}


- _freeAppsData
 /*
  * Frees the _appsData object.  It first frees the appName strings for
  * each element. 
  */
{
    int             numApps, i;
    AppData        *appDataPtr;

    if (_appsData) {
	numApps = [_appsData count];
	for (i = 0; i < numApps; i++) {
	    appDataPtr = (AppData *)[_appsData elementAt:i];
	    free(appDataPtr->appName);
	}
	[_appsData free];
    }
    _appsData = nil;
    return self;
}


static void _freeSndBuffer(NXJournaler *self)
 /*
  * Frees the SNDSoundStruct in the sndBuffer. replacing it with NULL.
  */
{
    if (self->_sndBuffer) {
	SNDFree(self->_sndBuffer);
	self->_sndBuffer = NULL;
    }
}


static long _getCurrentTime()
 /*
  * Gets the current time in 1/TICKS_PER_SECOND of seconds by calling
  * gettimeofday. The time returned is the difference between the current
  * time and the time at which getCurrentTime was first called. Since
  * getCurrentTime is called at the beginning of journaling, it is really
  * the amount of time sice journaling began. 
  */
{
    struct timeval  now;
    
    gettimeofday(&now, NULL);
    if (!startOfTime)
        startOfTime = now.tv_sec;
    return (TICKS_PER_SECOND * ((now.tv_sec - startOfTime) + (now.tv_usec / 1000000.0)));
}


static long _getServerTime()
 /*
  * Returns the time filled in by the server for an event that would
  * happen right now. The time is in units of 1/TICKS_PER_SECOND.
  */
{
    int            time;
    static BOOL	   serverTimeInited = NO;

    if (!serverTimeInited) {
        _NXInitGetServerTime();
	serverTimeInited = YES;
    }
    _NXGetServerTime(&time);
    return time;
}


- _initAppsData
 /*
  * Private method that frees any existing appsData and creates a new
  * empty one. 
  */
{
    [self _freeAppsData];
    _appsData = [[Storage allocFromZone:[self zone]] initCount:0
		 elementSize:sizeof(AppData)
		 description :"{*i}"];
    return self;
}


static int _lastJrnEvent(NXEvent *eventPtr)
 /* Returns TRUE iff the event is a NX_LASTJRNEVENT. */
{
    return (eventPtr->type == NX_JOURNALEVENT &&
	    eventPtr->data.compound.subtype == NX_LASTJRNEVENT);
}


- _initSlave
 /* Sent to create a new slave journaler for either recording of playback. */
{
    [super init];
    [self _commonNew:NULL];
    [NXApp _setSlaveJournaler:self];
    return self;
}

+ _newSlave
{
    return [[self allocFromZone:NXDefaultMallocZone()] _initSlave];
}

- (int)_playJournalEvent:(NXEvent *) eventPtr
 /*
  * This message is sent to the slave journaler. It receives events to be
  * played back in the controlled application. 
  */
{
    if (eventPtr->type != NX_JOURNALEVENT)
        _NXSetLastEventSentTime(eventPtr->time);
    eventPtr->ctxt = DPSGetCurrentContext();
    (void)DPSPostEvent(eventPtr, NO);
    return 0;
}


static int _playEventFilter(NXEvent *eventPtr)
 /*
  * This is the event filter that gets installed during event playback. It
  * discards some real events coming from the server and checks for abort
  * events. If an abort event is detected, the master journaler is sent
  * the _userAbort message. 
  */
{
    int             pitchIt = TRUE;
    int             type = eventPtr->type;
    NXJournaler	   *slaveJournaler;

    if (!(eventPtr->flags & NX_JOURNALFLAGMASK) && type == NX_KEYDOWN) {
	[[[NXApp slaveJournaler] speaker] _userAbort];
	NX_RAISE(NX_journalAborted, 0, 0);
    } else {
	if (type == NX_KITDEFINED && eventPtr->data.compound.subtype == NX_APPACT)
	    [NXApp _setCurrentActivation:eventPtr->data.compound.misc.L[0]];
	pitchIt = (NX_EVENTCODEMASK(type) & (MOUSE_EVENTS)) ||
	      (type == NX_KITDEFINED &&
	       (eventPtr->data.compound.subtype == NX_APPACT ||
		eventPtr->data.compound.subtype == NX_WINMOVED));
	if (pitchIt) {
	    slaveJournaler = [NXApp slaveJournaler];
	    if (slaveJournaler && slaveJournaler->_pendingStop)
	    pitchIt = FALSE;
	}
    }
    return !pitchIt;
}


static void _playTEProc(DPSTimedEntry num, double now, NXJournaler *self)
 /*
  * This is the time entry procedure that gets installed in the
  * controlling application during event playback.  Using the event in
  * _nextEvent it determines how much time must elapse before the event
  * should be dispatched to the slave journaler.  If the time is less than
  * or equal to zero, the event is dispatched. If the time is greater than
  * zero another timed entry is installed so this same routine will be
  * called after that amount of time has elapsed. After an event is
  * dispatched the next event is read from the event stream. 
  */
{
    long            timeToWait;
    long            currentTime;
    NXEvent        *nextEventPtr;
    int             moreEvents = TRUE;
    AppData        *appDataPtr;
    int             error;
    id		    requestee;
    port_t          remotePort;
    const char     *appName;

    nextEventPtr = &self->_nextEvent;
    currentTime = _getCurrentTime() - self->_startTime;
    timeToWait = nextEventPtr->time - currentTime;
    if (timeToWait <= 0) {
	do {
	    if (_lastJrnEvent(nextEventPtr))
		moreEvents = FALSE;
	    else {
		appDataPtr = (AppData *)[self->_appsData
				 elementAt:(unsigned)nextEventPtr->ctxt];
		if (!appDataPtr->sendPort) {
		    if (!strcmp(appDataPtr->appName, "Workspace"))
			appName = NX_WORKSPACEREQUEST;
		    else
			appName = appDataPtr->appName;
		    if (remotePort = _connectToApp(appName)) {
			if ([[NXApp appListener] listenPort] == remotePort)
			/*
			 * We are going to be sending events to ourself. 
			 * We can't initJournaling with Listener/Speaker
			 * since it blocks when there is returned data. 
			 */
			    requestee = [NXApp appListener];
			else {
			    requestee = self->_speaker;
			    [self->_speaker setSendPort:remotePort];
			}
			[requestee _initJournaling:(unsigned)nextEventPtr->ctxt
			 :[self->_listener listenPort]
			  :&(appDataPtr->sendPort)];
			[self->_speaker setSendPort:appDataPtr->sendPort];
			[self->_speaker _setStatus:NX_PLAYING];
		    } else
			NXLogError("NXJournaler -- Couldn't connect to app %s", appName);
   		    currentTime = _getCurrentTime() - self->_startTime;
   		    timeToWait = nextEventPtr->time - currentTime;
		}
		[self->_speaker setSendPort:appDataPtr->sendPort];
		nextEventPtr->time += playbackServerStartTime;
		error = [self->_speaker _playJournalEvent:nextEventPtr];
		NX_ASSERT(!error, "NXJournaler -- Error sending event for playing");
		do {
		    moreEvents = [self _readEvent:nextEventPtr];
		    timeToWait = nextEventPtr->time - currentTime;
		} while (timeToWait <= 0 &&
			 nextEventPtr->type == NX_JOURNALEVENT &&
		nextEventPtr->data.compound.subtype == NX_MOUSELOCATION);
	    }
	} while (moreEvents && timeToWait <= 0);
    }
    if (moreEvents) {
	[self _clearTimedEntry];
	self->_teNum = DPSAddTimedEntry(timeToWait / TICKS_PER_SECOND,
					(DPSTimedEntryProc) _playTEProc,
				    (void *)self, NX_MODALRESPTHRESHOLD);
    } else
	[self setEventStatus:NX_STOPPED
	 soundStatus:NX_STOPPED
	 eventStream:self->_eventStream
	 soundfile:self->_soundfile];
}


- (int)_readEvent:(NXEvent *)eventPtr
 /*
  * Reads the nextEvent from the event stream.  Return TRUE if and only if
  * there was another event to read. 
  */
{
    int             success;

    success = (sizeof(NXEvent) == NXRead(_eventStream, eventPtr, sizeof(NXEvent)));
    if (success && !_lastJrnEvent(eventPtr))
	eventPtr->flags |= NX_JOURNALFLAGMASK;	/* Set the JOURNALFLAG
						 * flag on all events read
						 * from the journal. */
    return success;
}


- (int)_recordJournalEvent:(NXEvent *)eventPtr
 /*
  * This method should only be received by the master journaler in the
  * controlling application. It receives events to be recorded from the
  * various apps.  
  */
{
    if (_eventStatus == NX_RECORDING)
	[self _writeEvent:eventPtr];
    return 0;
}


static void _recordSndInit(NXJournaler *self)
{
    int             err;
    int             format, rate, channels;
    int             bufSize;

   if (self->_recordDevice == NX_DSP) {
	bufSize = 2 * 8192;
	format = SND_FORMAT_DSP_DATA_16;
//	format = SND_FORMAT_COMPRESSED;
	rate = SND_RATE_LOW;
	channels = 1;
    } else {
	bufSize = 8192;
	format = SND_FORMAT_MULAW_8;
	rate = SND_RATE_CODEC;
	channels = 1;
    }
    err = SNDAlloc(&self->_sndBuffer, bufSize, format, rate, channels, 4);
    NX_ASSERT(!err, "NXJournaler -- _recordSndInit had error allocating sound buffers");
    /* Set dataSize to record at least an hour on the DSP */
    self->_sndBuffer->dataSize = 3600 * rate * 2;
}


- (int)_requestInitJournaling:(const char *)appName
     :(port_t) slavePort
     :(int *)appNumPtr
     :(port_t *) masterPortPtr
 /*
  * This method should be received only by a master journaler through a
  * Listener/Speaker connection.  It is sent when a slave needs to connect
  * to it for the purpose of sending events to be recorded. 
  */
{
    AppData         appData;

    if (_eventStatus == NX_RECORDING) {
	appData.appName = NXZoneMalloc([self zone], strlen(appName) + 1);
	strcpy(appData.appName, appName);
	appData.sendPort = slavePort;
	*appNumPtr = [_appsData count];
	*masterPortPtr = [_listener listenPort];
	[_appsData addElement:(void *)&appData];
	return 0;
    } else
	return -1;
}


- _sendEventForRecording:(NXEvent *)eventPtr
 /*
  * This method should be received only by slave journalers.  It converts
  * an event to a form that removes dependencies on the particular session
  * of the application being run.  In particular the window number for the
  * event is converted to one of NX_MOUSEDOWNWINDOW, NX_KEYWINDOW,
  * NX_MAINWINDOW, NX_MAINMENU, NX_APPICONWINDOW, NX_UNKNOWNWINDOW. After
  * conversion, the event is sent to the master journaler for writing to
  * the event stream. 
  *
  * If the event is one that causes aborting of script recording, the event
  * (not just a copy) is converted into a NX_JOURNALEVENT event with
  * subtype NX_LASTJRNEVENT and a _userAbort message is sent to the master
  * journaler. 
  */
{
    NXEvent         localEvent, *localEventPtr = &localEvent;
    id		    newWindowObj;
    int             newWindowNum;
    id              windowObj = nil;

    if (_eventStatus == NX_RECORDING) {
	if (eventPtr->type == NX_RMOUSEDOWN &&
	    ((eventPtr->flags & ABORT_FLAGS_MASK) == ABORT_FLAGS_MASK)) {
	    eventPtr->type = NX_JOURNALEVENT;
	    eventPtr->data.compound.subtype = NX_LASTJRNEVENT;
	    [self->_speaker _userAbort];
	} else if (_shouldRecord(eventPtr)) {
	    localEvent = *eventPtr;
	    if (NX_EVENTCODEMASK(eventPtr->type) & MOUSE_EVENTS) {
	        if (eventPtr->window == self->_mouseDownWindowNum)
		    windowObj = self->_mouseDownWindow;
		else
		    windowObj = [NXApp findWindow:eventPtr->window];
		if (eventPtr->type == NX_MOUSEMOVED) {
		    localEventPtr->type = NX_JOURNALEVENT;
		    localEventPtr->data.compound.subtype = NX_MOUSELOCATION;
		    if (newWindowObj = [NXApp _keyWindow])
			newWindowNum = NX_KEYWINDOW;
		    else if (newWindowObj = [NXApp _mainWindow])
			newWindowNum = NX_MAINWINDOW;
		    else if (newWindowObj = [NXApp mainMenu])
			newWindowNum = NX_MAINMENU;
		    else if (newWindowObj = self->_mouseDownWindow)
			newWindowNum = NX_MOUSEDOWNWINDOW;
		    else if (newWindowObj = [NXApp appIcon])
			newWindowNum = NX_APPICONWINDOW;
		    else {
			newWindowObj = nil;
			newWindowNum = NX_UNKNOWNWINDOW;
		    }
		    localEventPtr->window = newWindowNum;
		    _convertLocation(windowObj, newWindowObj, localEventPtr);
		} else {
		    _encodeWindowNum(&localEventPtr->window, localEventPtr, self);
		}
	    }
	    if (eventPtr->type == NX_JOURNALEVENT &&
		eventPtr->data.compound.subtype == NX_WINDRAGGED)
		_encodeWindowNum(&localEventPtr->window, localEventPtr, self);
	    else if (eventPtr->type == NX_KITDEFINED &&
		     eventPtr->data.compound.subtype == NX_APPACT)
		_encodeWindowNum((unsigned int *)&localEventPtr->data.compound.misc.L[1],
				 localEventPtr, self);
	    else if (eventPtr->type == NX_LMOUSEDOWN) {
		self->_oldEventMask =
		      [windowObj setEventMask:[windowObj eventMask] | NX_LMOUSEDRAGGEDMASK];
		self->_mouseDownWindowNum = eventPtr->window;
		self->_mouseDownWindow = windowObj;
	    } else if (eventPtr->type == NX_LMOUSEUP) {
		if (self->_oldEventMask)
		    [windowObj setEventMask:self->_oldEventMask];
		self->_oldEventMask = 0;
		self->_mouseDownWindowNum = 0;
		self->_mouseDownWindow = nil;
	    }
	    localEventPtr->ctxt = (DPSContext) self->_applicationNum;
	    [self->_speaker _recordJournalEvent:localEventPtr];
	}
    }
    return self;
}


- _setApplicationNum:(int)appNum
 /*
  * This method should be received only by slave journalers.  It sets the
  * _applicationNum instance variable that is used by the master journaler
  * to identify which application the event came from. 
  */
{
    _applicationNum = appNum;
    return self;
}


- (int)_setStatus:(int)newStatus
 /*
  * This message  should be recieved only by a slave journaler. NewStatus
  * may be NX_RECORDING, NX_PLAYING, or NX_STOPPED. If newStatus is
  * NX_RECORDING the application will begin sending it's events back to
  * the controlling application.  If newStatus is NX_PLAYING the
  * application prepares to start receiving events to play.  If newStatus
  * is NX_STOPPED, the application does whatever cleanup is necessary to
  * either stop receiving or sending events. 
  */
{
    NXEvent         theEvent;

    switch (newStatus) {
    case NX_RECORDING:
	[[NXApp mainMenu] addToEventMask:NX_MOUSEMOVEDMASK];
	break;
    case NX_PLAYING:
	_oldEventFilter = DPSSetEventFunc([NXApp context], _playEventFilter);
	break;
    case NX_STOPPED:
	if (_eventStatus == NX_RECORDING) {
	    [[NXApp mainMenu] removeFromEventMask:NX_MOUSEMOVEDMASK];
	    [self _disposeJournalEvents];
	    [self free];
	    return 0;
	} else if (_eventStatus == NX_PLAYING) {
	    if (_pendingStop) {
		[self _clearEventFilter];
		_pendingStop = NO;
	    } else {
	    /*
	     * Send yourself a NX_LASTJRNEVENT.  Upon getting this event
	     * in NXGetOrPeekEvent we can free the journaler as it will no
	     * longer be needed. 
	     */
		theEvent.type = NX_JOURNALEVENT;
		theEvent.data.compound.subtype = NX_LASTJRNEVENT;
		[self _playJournalEvent:&theEvent];
		_pendingStop = YES;
	    }

	}
	break;
    }
    _mouseDownWindow = nil;
    _mouseDownWindowNum = 0;
    if (!_pendingStop)
	_eventStatus = newStatus;
    return 0;
}


static int _shouldRecord(NXEvent *eventPtr)
{
    int type;
    
    type = eventPtr->type;
    if (type == NX_TIMER ||
        type == NX_CURSORUPDATE)
        return FALSE;
    else
        return TRUE;
}


static void  _sleepFor(int numSeconds)
{
 /*
  * Top level function of thread spawned to implement a time-out on
  * waiting for sound to start. 
  */
    sleep(numSeconds);
    mutex_lock(&soundStartedLock);
    soundStarted = YES;
    condition_signal(&soundStartedCondition);
    mutex_unlock(&soundStartedLock);
}


static int _soundStarted(SNDSoundStruct *soundBuffer, int tag, int err)
 /*
  * Called by the sound library when a sound buffer has started
  * being played or recorded.  This  routine simply sets a static 
  * global letting me know sound has begun. 
  */
{
    mutex_lock(&soundStartedLock);
    soundStarted = YES;
    condition_signal(&soundStartedCondition); 
    mutex_unlock(&soundStartedLock);
    return 0;
}


- (int)_userAbort
 /*
  * Sent to the journaler in the controlling application when one of the
  * controlled applications detects that the user has generated an abort
  * event during recording or playback.  This method has the controlling
  * journaler terminate. 
  */
{
    if ([_delegate respondsTo:@selector(journalerDidUserAbort:)])
        [_delegate journalerDidUserAbort:self];
    [self setEventStatus:NX_STOPPED
     soundStatus:NX_STOPPED
     eventStream:_eventStream
     soundfile:_soundfile];
    return 0;
}


- (int)_writeEvent:(NXEvent *)eventPtr
 /*
  * Writes the event to the eventStream, updating
  * _journalHeader.lastEventTime with the time. 
  */
{
#define TIME_FUDGE	((_recordDevice == NX_DSP) ? 12 : 0)

    _journalHeader.lastEventTime = eventPtr->time -= (_startTime - TIME_FUDGE);
    return NXWrite(_eventStream, eventPtr, sizeof(NXEvent));
}


@end

/*

Modifications:

82
--
 5/4/90  wot	Moved static "masterJournaler" to Application.m.  Removed the
		private method that returned the masterJournaler. 
 5/4/90	 wot	Fixed the call to _initJournaling to deal with the case where
		the master and slave journalers are in the same process.

85
--
 5/22/90 wot	Converted from using SoundObject and NXRecorder for sound to using
 		the sound library.
		
89
--
 7/30/90 wot	Changed sound recording to use SNDStartRecordingFile.
 7/30/90 wot	Added NXJournalMouse to allow correct mouse journaling when
 		you are not calling getNextEvent:..
		
91
--
 8/13/90 wot	Ripped out the code dealing with the overflow buffer.  DPSClient
 		no longer fills the queue.
		Added mutexes to synchronize the sound and events.
		
93
--
 8/28/90 wot    Added some error handling code for when the user aborted.  We now 
 		raise a NX_JOURNALABORTED error on user abort.  This fixed a problem
		where the slave app would get left in a wierd state if it was in some
		modal loop on abort
 9/6/90	wot	Changed _playEventFilter so we don't pitch the event if _pendingStop is
 		true.  This was to fix a bug where ShowAndTell didn't get the activate 
		event when the script stopped and ShowAndTell was participating in
		the journaling.
 9/6/90 wot	Moved the code that connects up to an app on playback.  It used to be
 		in the routine that read the next event.  Now I don't connect up until
		right before I need to send the event to the app.  This got around
		a problem where Workspace timed out trying to launch an app that was
		double-clicked on.  Journaling was already trying to launch it.

95
--
 9/15/90 wot	Added a thread to time-out on waiting for sound to start recording
 		or playing back.
 9/28/90 wot	Put in a TIME_FUDGE to fudge the timestamp of recorded events.  Without
 		this, scripts recorded with the DSP play back with events before the 
		sounds.
		
*/

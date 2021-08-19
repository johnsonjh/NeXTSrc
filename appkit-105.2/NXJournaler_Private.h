#import "NXJournaler.h"

@interface NXJournaler(Private)

- _adjustNewEventMask:(int *)newMask for:window;
- _clearTimedEntry;
- _commonNew:(const char *)listenerName;
- _decodeJournalEvent:(NXEvent *)eventPtr;
- _disposeJournalEvents;
- _freeAppsData;
- _initAppsData;
+ _newSlave;
- _initSlave;
- (int)_playJournalEvent:(NXEvent *) eventPtr;
- _postOverflowEvents;
- (int)_readEvent:(NXEvent *)eventPtr;
- (int)_recordJournalEvent:(NXEvent *)eventPtr;
- (int)_requestInitJournaling:(const char *)appName :(port_t) slavePort :(int *)appNumPtr :(port_t *) masterPortPtr;
- _sendEventForRecording:(NXEvent *)eventPtr;
- _setApplicationNum:(int)appNum;
- (int)_setStatus:(int)newStatus;
- (int)_userAbort;
- (int)_writeEvent:(NXEvent *)eventPtr;

@end

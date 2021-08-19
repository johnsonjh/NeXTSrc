#import "Listener.h"

@interface Listener(Private)

- (int)_initJournaling:(int)appNum :(port_t)masterPort :(port_t *)slavePort;
- (int)_requestInitJournaling:(const char *)appName  :(port_t) slavePort :(int *)appNumPtr :(port_t *) masterPortPtr;
- (int)_request:(const char *)requestName pb:(const char *)pasteboardName host:(const char *)host userData:(const char *)userData error:(char **)errorString;
- (int)_rmError:(const char *)menuEntry app:(const char *)application error:(const char *)error;
- (int)_request:(const char *)requestName pb:(const char *)pasteboardName host:(const char *)host userData:(const char *)userData app:(const char *)appName menu:(const char *)menuEntry error:(port_t)errorPort unhide:(int)flag;
- (int)_getWindowServerMemory:(int *)virtualMemory backing:(int *)backing;

@end

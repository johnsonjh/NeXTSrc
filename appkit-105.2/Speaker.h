/*
	Speaker.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import <sys/message.h>
#import <sys/port.h>

#define NX_ISFILE 0
#define NX_ISDIRECTORY 1
#define NX_ISAPPLICATION 2
#define NX_ISODMOUNT 3
#define NX_ISNETMOUNT 4
#define NX_ISSCSIMOUNT 5

@interface Speaker : Object
{
    port_t              sendPort;
    port_t              replyPort;
    int                 sendTimeout;
    int                 replyTimeout;
    id                  delegate;
    int                 _reservedSpeaker1;
    int                 _reservedSpeaker2;
}

- init;
- free;
- delegate;
- setDelegate:anObject;
- (port_t)sendPort;
- setSendPort:(port_t)aPort;
- (port_t)replyPort;
- setReplyPort:(port_t)aPort;
- (int)sendTimeout;
- setSendTimeout:(int)ms;
- (int)replyTimeout;
- setReplyTimeout:(int)ms;
- (int)sendOpenFileMsg:(const char *)fullPath ok:(int *)flag andDeactivateSelf:(BOOL)doDeact;
- (int)sendOpenTempFileMsg:(const char *)fullPath ok:(int *)flag andDeactivateSelf:(BOOL)doDeact;
- (int)openFile:(const char *)fullPath ok:(int *)flag;
- (int)openTempFile:(const char *)fullPath ok:(int *)flag;
- (int)launchProgram:(const char *)name ok:(int *)flag;
- (int)iconEntered:(int)windowNum at:(double)x :(double)y iconWindow:(int)iconWindowNum iconX:(double)iconX iconY:(double)iconY iconWidth:(double)iconWidth iconHeight:(double)iconHeight pathList:(const char *)pathList;
- (int)iconMovedTo:(double)x :(double)y;
- (int)iconReleasedAt:(double)x :(double)y ok:(int *)flag;
- (int)iconExitedAt:(double)x :(double)y;
- (int)registerWindow:(int)windowNum toPort:(port_t)aPort;
- (int)unregisterWindow:(int)windowNum;
- (int)getFileInfoFor:(char *)fullPath app:(char **)appname type:(char **)type ilk:(int *)ilk ok:(int *)flag;
- (int)getFileIconFor:(char *)fullPath TIFF:(char **)tiff TIFFLength:(int *)length ok:(int *)flag;
- (int)msgQuit:(int *)flag;
- (int)msgCalc:(int *)flag;
- (int)msgDirectory:(char * const *)fullPath ok:(int *)flag;
- (int)msgVersion:(char * const *)aString ok:(int *)flag;
- (int)msgFile:(char * const *)fullPath ok:(int *)flag;
- (int)msgPrint:(const char *)fullPath ok:(int *)flag;
- (int)msgSelection:(char * const *)bytes length:(int *)len asType:(const char *)aType ok:(int *)flag;
- (int)msgSetPosition:(const char *)aString posType:(int)anInt andSelect:(int)sflag ok:(int *)flag;
- (int)msgPosition:(char * const *)aString posType:(int *)anInt ok:(int *)flag;
- (int)msgCopyAsType:(const char *)aType ok:(int *)flag;
- (int)msgCutAsType:(const char *)aType ok:(int *)flag;
- (int)msgPaste:(int *)flag;
- (int)unmounting:(const char *)fullPath ok:(int *)flag;
- (int)powerOffIn:(int)ms andSave:(int)aFlag;
- (int)extendPowerOffBy:(int)requestedMs actual:(int *)actualMs;
- (int)performRemoteMethod:(const char *)msgSelector;
- (int)performRemoteMethod:(const char *)msgSelector with:(const char *)data length:(int)numBytes;
- (int)selectorRPC:(const char *)msgSelector paramTypes:(char *)params, ...;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;

@end

/*
	Application.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Responder.h"
#import "screens.h"
#import <objc/hashtable.h>

/* KITDEFINED subtypes */

#define NX_WINEXPOSED 		0
#define NX_APPACT 		1
#define NX_APPDEACT 		2
#define NX_WINRESIZED 		3
#define NX_WINMOVED 		4
#define NX_SCREENCHANGED	8

/* SYSDEFINED subtypes */

#define NX_POWEROFF		1

/* Additional flags */

#define NX_JOURNALFLAG	31
#define NX_JOURNALFLAGMASK	(1 << NX_JOURNALFLAG)

/* Thresholds passed to DPSGetEvent and DPSPeekEvent */

#define NX_BASETHRESHOLD	1
#define NX_RUNMODALTHRESHOLD	5
#define NX_MODALRESPTHRESHOLD	10

/*
 * Pre-defined return values for runModalFor: and runModalSession:
 * The system slso reserves all values BELOW these.
 */

#define NX_RUNSTOPPED		(-1000)
#define NX_RUNABORTED		(-1001)
#define NX_RUNCONTINUES		(-1002)

extern id NXApp;
extern int NXProcessID;
extern int NXNullObject;
extern const char *const NXSystemDomainName;

extern id NXGetNamedObject(const char *name, id owner);
extern const char *NXGetObjectName(id theObject);
extern int NXNameObject(const char *name, id theObject, id owner);
extern int NXUnnameObject(const char *name, id owner);

extern BOOL NXUserAborted(void);
extern void NXResetUserAbort(void);

extern const char *NXHomeDirectory(void);
extern const char *NXUserName(void);

extern int NXGetWindowServerMemory(DPSContext context, int *virtualMemory, int *windowBackingMemory, NXStream *windowDumpStream);

extern NXEvent *NXGetOrPeekEvent(DPSContext context, NXEvent * eventPtr, int mask, double timeout, int threshold, int peek);

/*
 * Information used by the system between beginModalSession:for:
 * and endModalSession: messages. This can either be allocated on
 * the stack frame of the caller, or by beginModalSession:for:.
 * The application should not access any of the elements of this
 * structure.
 */

typedef struct _NXModalSession {
    id                      app;
    id                      window;
    struct _NXModalSession *prevSession;
    int                     oldRunningCount;
    BOOL                    oldDoesHide;
    BOOL                    freeMe;
    int                     winNum;
    NXHandler              *errorData;
    int                     reserved1;
    int                     reserved2;
}                   NXModalSession;

@interface Application : Responder
{
    char               *appName;
    NXEvent             currentEvent;
    id                  windowList;
    id                  keyWindow;
    id                  mainWindow;
    id                  delegate;
    int                *hiddenList;
    int                 hiddenCount;
    const char         *hostName;
    DPSContext          context;
    int                 contextNum;
    id                  appListener;
    id                  appSpeaker;
    port_t              replyPort;
    NXSize              screenSize;
    short               running;
    struct __appFlags {
	unsigned int        hidden:1;
	unsigned int        autoupdate:1;
	unsigned int        active:1;
	unsigned int        _RESERVED:8;
	unsigned int        _doingUnhide:1;
	unsigned int	    _delegateReturnsValidRequestor:1;
	unsigned int	    _deactPending:1;
	unsigned int        _invalidState:1;
	unsigned int        _invalidEvent:1;
    }                   appFlags;
    id                  _reservedApp4;
    id                  _focusStack;
    id                  _freelist;
    id                  _pboard;
    id                  _mainMenu;
    id                  _appIcon;
    id                  _nameTable;
    id                  _printInfo;
    unsigned int        _reservedApp1;
    unsigned int        _reservedApp2;
    unsigned int        _reservedApp3;
}

+ initialize;
+ new;
+ allocFromZone:(NXZone *)zone;
+ alloc;

- free;
- setDelegate:anObject;
- delegate;
- (DPSContext)context;
- hide:sender;
- unhide:sender;
- unhideWithoutActivation:sender;
- findWindow:(int)windowNum;
- focusView;
- mainWindow;
- keyWindow;
- (port_t)replyPort;
- (const char *)appName;
- (const char *const *)systemLanguages;
- (const char *)appListenerPortName;
- appListener;
- setAppListener:aListener;
- appSpeaker;
- setAppSpeaker:aSpeaker;
- (int)unhide;
- (int)openFile:(const char *)fullPath ok:(int *)flag;
- (int)openTempFile:(const char *)fullPath ok:(int *)flag;
- (int)unmounting:(const char *)fullPath ok:(int *)flag;
- (int)powerOffIn:(int)ms andSave:(int)aFlag;
- (BOOL)isActive;
- (const char *)hostName;
- (BOOL)isHidden;
- (BOOL)isRunning;
- (int)activeApp;
- deactivateSelf;
- (int)activateSelf:(BOOL)flag;
- (int)activate:(int)contextNumber;
- run;
- (int)runModalFor:theWindow;
- stop:sender;
- stopModal;
- stopModal:(int)returnCode;
- (void)abortModal;
- (NXModalSession *)beginModalSession:(NXModalSession *)session for:theWindow;
- (int)runModalSession:(NXModalSession *)session;
- endModalSession:(NXModalSession *)session;
- setAutoupdate:(BOOL)flag;
- terminate:sender;
- (NXEvent *)getNextEvent:(int)mask waitFor:(double)timeout threshold:(int)level;
- (NXEvent *)peekNextEvent:(int)mask into:(NXEvent *)eventPtr waitFor:(float)timeout threshold:(int)level;
- (NXEvent *)getNextEvent:(int)mask;
- (NXEvent *)peekAndGetNextEvent:(int)mask;
- (NXEvent *)peekNextEvent:(int)mask into:(NXEvent *)eventPtr;
- (NXEvent *)currentEvent;
- powerOff:(NXEvent *)theEvent;
- applicationDefined:(NXEvent *)theEvent;
- rightMouseDown:(NXEvent *)theEvent;
- sendEvent:(NXEvent *)theEvent;
- becomeActiveApp;
- resignActiveApp;
- makeWindowsPerform:(SEL)aSelector inOrder:(BOOL)flag;
- appIcon;
- windowList;
- getWindowNumbers:(int **)list count:(int *)winCount;
- updateWindows;
- (BOOL)sendAction:(SEL)theAction to:theTarget from:sender;
- calcTargetForAction:(SEL)theAction;
- (BOOL)tryToPerform:(SEL)anAction with:anObject;
- printInfo;
- runPageLayout:sender;
- orderFrontColorPanel:sender;
- setPrintInfo:info;
- setMainMenu:aMenu;
- mainMenu;
- delayedFree:theObject;
- getScreens:(const NXScreen **)list count:(int *)count;
- (const NXScreen *)mainScreen;
- (const NXScreen *)colorScreen;
- getScreenSize:(NXSize *)theSize;
- loadNibFile:(const char *)fileName owner:anOwner withNames:(BOOL)aFlag fromZone:(NXZone *)zone;
- loadNibFile:(const char *)fileName owner:anOwner withNames:(BOOL)aFlag;
- loadNibFile:(const char *)fileName owner:anOwner;
- loadNibSection:(const char *)sectionName owner:anOwner withNames:(BOOL)aFlag;
- loadNibSection:(const char *)sectionName owner:anOwner withNames:(BOOL)aFlag fromZone:(NXZone *)zone;
- loadNibSection:(const char *)sectionName owner:anOwner withNames:(BOOL)flag fromHeader:(const struct mach_header *)mhp;
- loadNibSection:(const char *)sectionName owner:anOwner withNames:(BOOL)flag fromHeader:(const struct mach_header *)mhp fromZone:(NXZone *)zone;
- loadNibSection:(const char *)sectionName owner:anOwner;
- slaveJournaler;
- masterJournaler;
- (BOOL)isJournalable;
- setJournalable:(BOOL)newStatus;

@end

@interface Application(WindowsMenu)
- windowsMenu;
- setWindowsMenu:(id)menu;
- arrangeInFront:sender;
- removeWindowsItem:(id)win;
- addWindowsItem:(id)win title:(const char *)aString filename:(BOOL)isFilename;
- changeWindowsItem:(id)win title:(const char *)aString filename:(BOOL)isFilename;
- updateWindowsItem:(id)win;
@end

@interface Object(ApplicationDelegate)
- appWillInit:sender;
- appDidInit:sender;
- appDidHide:sender;
- appDidUnhide:sender;
- appDidBecomeActive:sender;
- appDidResignActive:sender;
- appDidUpdate:sender;
- appWillUpdate:sender;
- appWillTerminate:sender;
- (BOOL)appAcceptsAnotherFile:sender;
- (int)app:sender openFile:(const char *)filename type:(const char *)aType;
- (int)app:sender openTempFile:(const char *)filename type:(const char *)aType;
- app:sender powerOffIn:(int)ms andSave:(int)aFlag;
- (int)app:sender unmounting:(const char *)fullPath;
- applicationDefined:(NXEvent *)theEvent;
- powerOff:(NXEvent *)theEvent;
@end

@interface Application(ServicesMenu)
- servicesMenu;
- setServicesMenu:aMenu;
- registerServicesMenuSendTypes:(const char *const *)sendTypes andReturnTypes:(const char *const *)returnTypes;
@end

@interface Object(ServicesMenuResponders)
- (BOOL)writeSelectionToPasteboard:pboard types:(NXAtom *)types;
- readSelectionFromPasteboard:pboard;
@end

@interface Object(DelayedPerform)
- perform:(SEL)aSelector with:anArg afterDelay:(int)ms cancelPrevious:(BOOL)flag;
@end

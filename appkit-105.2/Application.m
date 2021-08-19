/*
	Application.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "FontManager_Private.h"
#import "Listener_Private.h"
#import "Menu_Private.h"
#import "NXCursor_Private.h"
#import	"NXJournaler_Private.h"
#import "Panel_Private.h"
#import "Font_Private.h"
#import "Pasteboard_Private.h"
#import "Window_Private.h"
#import "Font.h"
#import "FontManager.h"
#import	"PopUpList.h"
#import "Matrix.h"
#import "Menu.h"
#import "MenuCell.h"
#import "NameTable.h"
#import "NibData.h"
#import "Panel.h"
#import "PageLayout.h"
#import "PrintInfo.h"
#import "NXColorPanel.h"
#import "NXFaxPanel.h"
#import "Speaker.h"
#import "View.h"
#import "WindowTemplate.h"
#import "packagesWraps.h"
#import "publicWraps.h"
#import "privateWraps.h"
#import "cursorRect.h"
#import <defaults.h>
#import "errors.h"
#import "screens.h"
#import "nibprivate.h"
#import "nextstd.h"
#import "perfTimer.h"
#import "pbtypes.h"
#import "pbs.h"
#import <dpsclient/wraps.h>
#import <objc/List.h>
#import <objc/Storage.h>
#import <objc/StreamTable.h>
#import <objc/hashtable.h>
#import <objc/NXStringTable.h>
#import <libc.h>
#import <sys/time.h>
#import <sys/signal.h>
#import <sys/message.h>
#import <sys/param.h>
#import <sys/syslog.h>
#import <sys/file.h>
#import <sys/stat.h>
#import <sys/types.h>
#import <ctype.h>
#import <mach.h>
#import <servers/netname.h>
#import <math.h>
#import <stdio.h>
#import <locale.h>

#ifndef SHLIB
    /* this references category .o files, forcing them to be linked into an app which is linked against a archive version of the kit library. */
    asm(".reference .NXAppCategoryServicesMenu\n");
    asm(".reference .NXAppCategoryWindows\n");
    asm(".reference .NXAppCategoryCache\n");
    asm(".reference .NXObjectCategoryDelayedPerform\n");
    asm(".reference .NXObjectCategoryZone\n");
    #include <linkUnreferencedClasses>
#endif

#define _NX_APPUNHIDE	7
  /* kitdefined event telling us to unhide */

static void         initListener(Application *self);
#if defined(CHECK_PS_STACK) && defined(DEBUG)
static void	    checkStack(void);
#endif
static void         convertEvent(NXEvent *eventPtr);
static void	    uncaughtErrorProc(int code, const void *data1, const void *data2);
static void	    endKeyAndMain(Application *self);
static Application *whoIsResponsible(Application *self, SEL selector);
static void         handle_signal(int);
static void         handleMallocError(int code);
static void         handleCursorRect(Application *self, NXEvent *theEvent);
static void         makeAppIconBitmap(void);
static void	    runModalCleanup(void *data, int code, void *data1, void *data2);
static char 	   *getAppName(void);
static NXCursorRect *topOfCursorStack(void);
static void _getJournalStatus(int *status);

static NXCursorStack *cursorStack = 0;

static const char   packageName[] = "windowPackage";

static BOOL         abortRaised = NO;
static BOOL	    isJournalable = YES;
static int	    currentActivation = 0; /* used to sync cursors */
	/* a sync count incremented in packages whenever an app is activated */
static id	    hiddenWindows = nil;
/* this should ONLY be used between +initialize and +new.  After that,
   call the method.
 */
static char	   *appNameAtStartup;

/*
 * Number of screens. This information is obtained from the server on
 * the first invocation of getScreens:. Do not access these variables
 * directly as they might not be set.
 *
 * Note that having these as statics makes them yet another piece of
 * cruft that needs to be fixed if we want multiple contexts supported
 * in apps.
 */
static short _numScreens = -1;
static NXScreen *_screens = NULL;

static id _slaveJournaler = nil;
static id _masterJournaler = nil;
static NXZone *appZone = 0;


typedef struct {
    @defs (Window)
} WindowId;

#define CURSORS_INVALID(w) (!(((WindowId *)(w))->wFlags2._validCursorRects))

@interface Object(ObsoleteApplicationDelegates)
- (int)appOpenTempFile:(const char *)filename type:(const char *)type;
- (int)appOpenFile:(const char *)filename type:(const char *)type;
- (int)appUnmounting:(const char *)filename;
- (int)appPowerOffIn:(int)ms andSave:(BOOL)flag;
- appPowerOff:(NXEvent *)event;
@end

@implementation Application:Responder


/** Initializing **/

+ initialize
{
    const char	       *defValue;
  /* INDENT OFF */
    static const NXDefaultsVector AppKitDefaults = {
	{"DoLaunchTiming", NULL},	/* DFLTSTR - Do launch timing? */
	{"HomeDirectory", NULL},	/* DFLTSTR - user's home directory */
	{"LaunchTime", "0 0"},		/* DFLTSTR - launch time passed from WorkSpace */
	{"MachLaunch", NULL},		/* DFLTSTR - sequence # to reply on launch */	
	{"ServicesMenuTimings", NULL},	/* DFLTSTR - whether to time the SavePanel */
	{"SavePanelTiming", NULL},	/* DFLTSTR - whether to time the SavePanel */
	{"SavePanelDebug", NULL},	/* DFLTSTR - whether to debug the SavePanel */
	{"Uid", NULL},			/* DFLTSTR - user's user id */
	{"UserName", NULL},		/* DFLTSTR - user's user name */
	{"PBSName", NULL},		/* DFLTSTR - name to look up pbs with */
	{"NPDName", NULL},		/* DFLTSTR - name to look up npd with */
	{"ProfileString", NULL},	/* DFLTSTR - controls turning profiling on/off */
	{NULL }
    };
    static const NXDefaultsVector SystemDefaults = {
	{"BoldSystemFont","Helvetica-Bold"}, /* DFLTSTR - default bod font */
	{"ButtonDelay", "0.4"},		/* DFLTSTR - initial delay of buttons */
	{"ButtonPeriod", "0.075"},	/* DFLTSTR - period of buttons */
	{"BrowserSpeed", "400"},	/* DFLTSTR - speed of browser scrolling */
	{"BrowserColumnWidth", "100"},	/* DFLTSTR - width of an Open/SavePanel browser column */
	{"Printer", "Local_Printer"},	/* DFLTSTR - name of printer */
	{"PrinterHost", ""},		/* DFLTSTR - host of printer */
	{"PrinterResolution", "400"},	/* DFLTSTR - resolution in dpi of the printer */
	{"ScrollerButtonDelay", "0.5"},	/* DFLTSTR - initial delay of scroller buttons */
	{"ScrollerButtonPeriod", "0.075"}, /* DFLTSTR - period of scroller buttons */
	{"ScrollerKnobDelay", "0.001"}, /* DFLTSTR - wait time for another drag event */
	{"ScrollerKnobCount", "2"}, 	/* DFLTSTR - number of drag events to snarf */
	{"SystemFont","Helvetica"},	/* DFLTSTR - fonts used in apps */
	{"UnixExpert", "NO"},		/* DFLTSTR - is user unix facile */
	{NX_FAXORIGINS, "0.0 0.0 0.0 0.0" }, /* DFLTSTR - rtf and eps layout positions. */
	{NX_LIBRARYPATH,_NX_LIBRARYPATH}, /* DFLTSTR - library path for finding fax files */
	{NX_FAXWANTSCOVER,"YES"},	/* DFLTSTR - whether to generate cover */
	{NX_FAXWANTSNOTIFY,"YES"},	/* DFLTSTR - whether to notify on completion */
	{NX_FAXWANTSHIRES,"YES"},	/* DFLTSTR - whether to send fax hires */
	{"Language", NULL},		/* DFLTSTR - what language the system is in */
	{"Country", NULL},		/* DFLTSTR - what country or region the system is in */
	{"Fax", "Local_Fax"},		/* DFLTSTR - name of fax */
	{"FaxHost", ""},		/* DFLTSTR - host of fax */
	{"FaxResolution", "200"},	/* DFLTSTR - resolution in dpi of the fax */
	{"LargeFileSystem", NULL},	/* DFLTSTR - whether stats are VERY slow */
	{NULL }
    };
    static const NXDefaultsVector ApplicationDefaults = {
	{"NXAutoLaunch", "NO"},		/* DFLTSTR - is prog autolaunched at login? */
	{"NXAllWindowsRetained", NULL},	/* DFLTSTR - force all windows to be retained? */
	{"NXAllWindowsOneShot", NULL},	/* DFLTSTR - force all windows to be one shot? */
	{"NXCaseSensitiveBrowser", NULL},/* DFLTSTR - is browser case sensitive */
	{"NXHost", NULL},		/* DFLTSTR - host for socket to PS */
	{"NXMargins", "72 72 90 90"},	/* DFLTSTR - unprinted borders of a page */
	{"NXMenuX","-1.0"},		/* DFLTSTR - initial main menu x location */
	{"NXMenuY","1000000.0"},	/* DFLTSTR - initial main menu y location */
	{"NXOpen",NULL},		/* DFLTSTR - name of document file to open */
	{"NXOpenTemp",NULL},		/* DFLTSTR - name of temp file to open */
	{"NXPSDebug", NULL},		/* DFLTSTR - use alt PS handleerror proc? */
	{"NXPSName", NULL},		/* DFLTSTR - name to lookup PS server by */
	{"NXPaperType", "Letter"},	/* DFLTSTR - paper type for page layout */
	{"NXShowAllWindows",NULL},	/* DFLTSTR - debugging flag to show all windows*/
	{"NXShowPS", NULL},		/* DFLTSTR - postscript debugging output */
	{"NXTraceEvents", NULL},	/* DFLTSTR - event trace debugging output */
	{"NXFont", _NX_INITDEFAULTFONT},/* DFLTSTR - default text font */
	{"NXFontSize", _NX_INITDEFAULTFONTSIZE},/* DFLTSTR - default text font size */
	{"NXFixedPitchFont", "Ohlfs"},	/* DFLTSTR - default fixed width font */
	{"NXFixedPitchFontSize", "10"},	/* DFLTSTR - default fixed width font size */
	{"NXMallocDebug", "0"},		/* DFLTSTR - level passed to malloc_debug */
	{"NXWordTablesFile", NULL},	/* DFLTSTR - default type of word tables */
	{"NXNetTimeout", "60"},		/* DFLTSTR - sec to timeout for net ops */
	{"NXOptimizeDrawing", NULL},	/* DFLTSTR - controls optimized drawing */
	{"NXIsJournalable", "YES"},	/* DFLTSTR - default value of isJournalable */
	{"NXWindowDepthLimit", NULL},	/* DFLTSTR - depth limit for windows */
	{"NXDebugLanguages", NULL},	/* DFLTSTR - whether to emit language debugging info */
	{"_NXBypassNSC", NULL},		/* DFLTSTR - create secure contexts? */
	{"NXServiceLaunch", NULL},	/* DFLTSTR - launched from services */
	{NULL }
    };
 /* INDENT ON */

    if (self == [Application class]) {
	_NXSetupPriorities();
	[self setVersion:1];
    	appNameAtStartup = getAppName();
	MARKTIME(YES, "[Application initialize] register defaults", 0);
#ifdef SHLIB
       /* Attempt to fix the NXArgc headbutt with defaults. -Ali */
	asm("	movel	__libNeXT_NXArgc, 0x0401064c\n");
	asm("	movel	__libNeXT_NXArgv, 0x040105f4\n");
#endif
	NXRegisterDefaults(_NXAppKitDomainName, AppKitDefaults);
	NXRegisterDefaults(NXSystemDomainName, SystemDefaults);
	NXRegisterDefaults(appNameAtStartup, ApplicationDefaults);
	NXRegisterErrorReporter(NX_APPKITERRBASE, NX_APPKITERRBASE + 999,
				_NXAppkitReporter);
	NXRegisterErrorReporter(DPS_ERRORBASE, DPS_ERRORBASE + 999,
				_NXDpsReporter);
	NXRegisterErrorReporter(NX_STREAMERRBASE, NX_STREAMERRBASE + 999,
				_NXStreamsReporter);
	NXRegisterErrorReporter(TYPEDSTREAM_ERROR_RBASE,
				TYPEDSTREAM_ERROR_RBASE + 999,
				_NXTypedStreamsReporter);
	defValue = NXGetDefaultValue(appNameAtStartup, "NXMallocDebug");
	if (defValue)
	    malloc_debug(atoi(defValue));
	openlog(appNameAtStartup, LOG_PID, 0);
	defValue = NXGetDefaultValue(appNameAtStartup, "NXNetTimeout");
	if (defValue)
	    _NXNetTimeout = atoi(defValue) * 1000;
	[Pasteboard class];	/* cause type strings to get uniqued */
#ifdef DEBUG
	{
	    int sec, usec;
            const char *dstr;
	    
	    if (dstr = NXGetDefaultValue(_NXAppKitDomainName, "LaunchTime")) { 
		MARKTIME(YES, "[Application initialize] get defaults", 0);
		sscanf(dstr, "%d %d", &sec, &usec);
		INITMARKTIME((double)sec + ((double)usec/1000000));
		if (NXGetDefaultValue(_NXAppKitDomainName, "DoLaunchTiming"))
		    _NXLaunchTiming = YES;
		else
		    CLEARTIMES(YES);
		MARKTIME(_NXLaunchTiming, "other ObjC inits", 0);
	    }
	    _NXProfileString = NXGetDefaultValue(_NXAppKitDomainName, "ProfileString");
	    if (_NXProfileString)
		moncontrol(0);
	}
#endif
    }
    setlocale(LC_ALL, "");
  /* tell dpsclient that its a 1.0 app */
    if (_NXGetShlibVersion() <= MINOR_VERS_1_0) {
	extern char _DPSDo10Compatibility;
	_DPSDo10Compatibility = YES;
    }
    return self;
}


+ (void)_userLoggedOut
{
    [Font _freePBSData];
}


static void initGlobals()
{
 /* we must initialize certain globals before unarchiving the app */
    NXSystemFont = NXGetDefaultValue(NXSystemDomainName, "SystemFont");
    NXBoldSystemFont = NXGetDefaultValue(NXSystemDomainName, "BoldSystemFont");
}


/** Instantiation **/

+ (BOOL)_canAlloc { return NO; }

+ allocFromZone:(NXZone *)zone { return [self doesNotRecognize:_cmd]; }

+ alloc { return [self doesNotRecognize:_cmd]; }


+ new
{
    if (NXApp)
	return NXApp;
    if (!appZone) {
	appZone = NXCreateZone(vm_page_size, vm_page_size, YES);
	NXNameZone(appZone, "NXApp");
    }
    return [[super allocFromZone:appZone] _setUp];
}


- _setUp
{
    DPSContext          newContext;
    const char         *host;

    MARKTIME(_NXLaunchTiming, "[Application new] read defaults", 0);
    NX_DURING {
	NXProcessID = getpid();
	NXSetUncaughtExceptionHandler(uncaughtErrorProc);
	malloc_error(handleMallocError);
	host = NXGetDefaultValue(appNameAtStartup, "NXHost");
	if (NXGetDefaultValue(appNameAtStartup, "_NXBypassNSC")) {
	    extern char _DPSDoNSC;
	    _DPSDoNSC = NO;
	}
	if (NXGetDefaultValue(appNameAtStartup, "NXShowAllWindows"))
	    _NXShowAllWindows = YES;
	if (NXGetDefaultValue(appNameAtStartup, "NXAllWindowsRetained"))
	    _NXAllWindowsRetained = YES;
	if (NXGetDefaultValue(appNameAtStartup, "NXShowPS"))
	    DPSTraceContext(DPS_ALLCONTEXTS, TRUE);
	if (NXGetDefaultValue(appNameAtStartup, "NXTraceEvents")) {
	    _NXTraceEvents = YES;
	    DPSTraceEvents(DPS_ALLCONTEXTS, TRUE);
	}
	MARKTIME(_NXLaunchTiming, "[Application new] create PS connection", 0);
	newContext = DPSCreateContextWithTimeoutFromZone(host,
			NXGetDefaultValue(appNameAtStartup, "NXPSName"),
			NULL, NULL, _NXNetTimeout, [self zone]);
	_NXSetDepthLimitFromDepthName(NXGetDefaultValue(appNameAtStartup, "NXWindowDepthLimit"));
      /* ??? test success to see if package was loaded? */
	MARKTIME(_NXLaunchTiming, "[Application new] load packages", 0);
	if (_NXLoadPackage(packageName) < 0)
	    NXLogError("Package %s could not be loaded.\n", packageName);
	if (NXGetDefaultValue(appNameAtStartup, "NXPSDebug"))
	    DPSPrintf(DPSGetCurrentContext(), "_NXInstallDebugErrorHandler\n");
	DPSPrintf(DPSGetCurrentContext(), "/_NXKitVersion %d def\n", [Application version]);

	MARKTIME(_NXLaunchTiming, "[Application new] other initting", 0);
	initGlobals();
	_NXSetProcessID(NXProcessID);
	if (!NXApp) {
	    [super init];
	    appName = appNameAtStartup;
	    hostName = host;		/* host from defaults (can be NULL) */
	    context = newContext;
	    appFlags.hidden = NO;
	    windowList = [[List allocFromZone:[self zone]] init];
	    MARKTIME(_NXLaunchTiming, "[Application new] common init", 0);
	    [self _commonAppInit];
	}
	running = 1;
    } NX_HANDLER {
	(*_NXTopLevelErrorHandler) (&NXLocalHandler);
    } NX_ENDHANDLER
    return self;
}


- free
{
    register int        offset;
    NXZone *zone = [self zone];

    running = 0;
 /*
  * Must go through in reverse order since close may remove the window from
  * the List. 
  */
    if (offset = [windowList count])
	for (offset--; offset >= 0; offset--)
	    [[windowList objectAt:offset] close];

    [_focusStack free];
    _focusStack = nil;
    [_pboard free];
    _pboard = nil;
    DPSDestroyContext(context);
    [super free];
    if (appZone == zone) {
	NXDestroyZone(appZone);
	appZone = 0;
    }
    return nil;
}


- _commonAppInit
{
    NXApp = self;
    MARKTIME(_NXLaunchTiming, "[Application _commonAppInit] getting server info", 1);
    _NXScreenSize(&screenSize.width, &screenSize.height);
    PSnull();
    NXNullObject = DPSDefineUserObject(0);
    _focusStack = [[Storage allocFromZone:[self zone]] initCount:0 elementSize:sizeof(focusStackElem) description:"@*c"];
    PScurrentcontext(&contextNum);
    MARKTIME(_NXLaunchTiming, "[Application _commonAppInit] _NXBuildSharedWindows", 1);
    _NXBuildSharedWindows();
    MARKTIME(_NXLaunchTiming, "[Application _commonAppInit] _makeCursors", 1);
    [NXCursor _makeCursors];
    _appIcon = nil;
    MARKTIME(_NXLaunchTiming, "[Application _commonAppInit] makeAppIconBitmap", 1);
    makeAppIconBitmap();
    MARKTIME(_NXLaunchTiming, "[Application _commonAppInit] afterglue", 1);
    return self;
}


- setDelegate:anObject
{
    delegate = anObject;
    appFlags._delegateReturnsValidRequestor = [delegate respondsTo:@selector(validRequestorForSendType:andReturnType:)];
    return self;
}


- delegate
{
    return delegate;
}


- (DPSContext)context
{
    return context;
}


/** Window Management **/

- _removeHiddenWindow:(int)windowNum
{
    int i = hiddenCount;

    if (hiddenCount && hiddenWindows && hiddenList) {
	while (i--) {
	    if (hiddenList[i] == windowNum)
		break;
	}
	if (i >= 0) {
	    [hiddenWindows removeAt:i];
	    hiddenList = [hiddenWindows elementAt:0];
	    hiddenCount--;
	}
    }

    return self;
}


- hide:sender
{
    register id         win, key, main;
    register int        i;
    int                 ping, tempCount, *tempList, oldCount;

    if (!hiddenWindows)
	hiddenWindows = [[Storage allocFromZone:[self zone]] initCount:0 elementSize:sizeof(int) description:"i"];
    [Menu _repairBackups];
    key = keyWindow;
    main = mainWindow;
    _NXDeactivateSelf(&ping);
    [self getWindowNumbers:&tempList count:&tempCount];

    [_appIcon displayBorder];  /* ??? why is this display here */
    [_appIcon orderFront:sender];
    appFlags.hidden = YES;

    for (i = tempCount - 1; i >= 0; i--) {
	win = [self findWindow:tempList[i]];
	if (win != _appIcon && ([win style] != NX_TOKENSTYLE))
	    [win _tempHide:YES relWin:0];
    }
    mainWindow = main;
    keyWindow = key;
    oldCount = [hiddenWindows count];
    hiddenCount = oldCount + tempCount;
    [hiddenWindows setNumSlots:hiddenCount];
    if (oldCount) 
	bcopy([hiddenWindows elementAt:0], [hiddenWindows elementAt:tempCount],
	    oldCount * sizeof(int));
    bcopy(tempList, [hiddenWindows elementAt:0], tempCount * sizeof(int));
    hiddenList = [hiddenWindows elementAt:0];
    free(tempList);
    if ([delegate respondsTo:@selector(appDidHide:)])
	[delegate appDidHide:self];
    _NXActivateNextApp();
    return self;
}


- unhide:sender
{
    [self activateSelf:YES];
    return [self unhideWithoutActivation:sender];
}


- unhideWithoutActivation:sender
{
    register id         win;
    register int	lastWindowNum;
    register int        i;

    if (appFlags._doingUnhide)
	return self;
    if (!appFlags.hidden)
	_NXOrderKeyAndMain();
    else {
	lastWindowNum = 0;
      /* protects against recursive unhiding, which happens because orderwindow makes sure an app is unhidden when it brings windows onto the screen.  */
	 appFlags._doingUnhide = YES;
	 NX_DURING {
	    for (i = 0; i < hiddenCount; i++) {
		/* ??? wot - this scheme won't work after archiving */
		win = [self findWindow:hiddenList[i]];
		if (win != _appIcon && ([win style] != NX_TOKENSTYLE)) {
		    [win _tempHide:NO relWin:lastWindowNum];
		    lastWindowNum = [win windowNum];
		}
	    }
	} NX_HANDLER {
	    appFlags._doingUnhide = NO;
	    NX_RERAISE();
	} NX_ENDHANDLER
	appFlags._doingUnhide = NO;
	appFlags.hidden = NO;
	hiddenList = NULL;
	hiddenCount = 0;
	[hiddenWindows setNumSlots:0];
	_NXOrderKeyAndMain();
      /* removed - this will happen when we get activate event */
      /*_NXShowKeyAndMain();*/
	if ([delegate respondsTo:@selector(appDidUnhide:)])
	    [delegate appDidUnhide:self];
    }
    return self;
}


- findWindow:(int)windowNum
{
    return [self findWindowUsingCache:windowNum];
}


- focusView
{
    int count = [_focusStack count];
    
    if (count)
	return (((focusStackElem *)[_focusStack elementAt:(count-1)])->view);
    else
	return nil;
}


- _focusStack
{
    return (_focusStack);
}


- (BOOL)_flipState
{
    return ([[self focusView] isFlipped]);
}


- mainWindow
{
    return appFlags.active && !appFlags._invalidState ? mainWindow : nil;
}

- _mainWindow 
{
    return mainWindow;
}


- _setMainWindow:newMainWindow
{
    mainWindow = newMainWindow;
    return self;
}


- keyWindow
{
    return appFlags.active && !appFlags._invalidState ? keyWindow : nil;
}

- _keyWindow 
{
    return keyWindow;
}


- _setKeyWindow:newKeyWindow
{
    keyWindow = newKeyWindow;
    return self;
}

- _setInvalid:(BOOL)flag
{
    appFlags._invalidState = flag ? 1 : 0;
    return self;
}


- (BOOL)_isInvalidEvent
{
    return appFlags._invalidEvent;
}


- (BOOL)_isDeactPending
{
    return appFlags._deactPending;
}


- (BOOL)_isInvalid
{
    return appFlags._invalidState;
}


/* Inter-Application Messaging Support */


- (port_t)replyPort
{
    return replyPort;
}

- (const char *)appName
{
    if (!appName)
	appName = getAppName();
    return appName;
}

- setAppName:(char *)aName
{
  /* dont free this instance var if its the same as the storage we alloced
     at startup
   */
    if (appName && appName != appNameAtStartup)
	free (appName);
    appName = NXCopyStringBufferFromZone(aName, [self zone]);
    return self;
}

static BOOL systemHasLanguages = YES;
static char **systemLanguages = NULL;

- (const char *const *)systemLanguages
{
    int count, length;
    const char *language, *separator;

    if (systemHasLanguages && !systemLanguages) {
	language = NXGetDefaultValue(NXSystemDomainName, "Language");
	if (!language) language = NXGetDefaultValue(NXSystemDomainName, "NXLanguage");
	if (language && *language) {
	    count = 1;
	    separator = strchr(language, ';');
	    while (separator) {
		count++;
		separator = strchr(separator+1, ';');
	    }
	    systemLanguages = NXZoneMalloc([self zone], (count + 1) * sizeof(char *));
	    systemLanguages[count] = NULL;
	    for (count = 0; language; count++) {
		separator = strchr(language, ';');
		if (separator) {
		    length = separator - language;
		    separator++;
		} else {
		    length = strlen(language);
		}
		while (*language == ' ' || *language == '	') {
		    language++; length--;
		}
		while (language[length-1] == ' ' || language[length-1] == '	') length--;
		systemLanguages[count] = (char *)NXZoneMalloc([self zone], length+1);
		strncpy(systemLanguages[count], language, length);
		systemLanguages[count][length] = '\0';
		language = separator;
	    }
	} else {
	    systemHasLanguages = NO;
	}
    }

    return (const char *const *)systemLanguages;
}

const char *_NXKitString(const char *tableName, const char *key)
{
    id table = nil;
    const char *value;
    BOOL isEnglish = NO;
    const char *const *languages;
    char file[MAXPATHLEN+1], tableBuffer[MAXPATHLEN+1];
    static int debugLanguages = -1;
    static id kitStringTables = nil;

    languages = systemLanguages;
    if (!languages && systemHasLanguages) languages = [NXApp systemLanguages];
    if (!languages) return key;
    if (!tableName) tableName = "Common";
    if (tableName[0] == '"' && tableName[strlen(tableName)-1] == '"') {
	strcpy(tableBuffer, tableName+1);
	tableBuffer[strlen(tableBuffer)-1] = '\0';
	tableName = tableBuffer;
    }

    if (debugLanguages < 0) debugLanguages = NXGetDefaultValue([NXApp appName], "NXDebugLanguages") ? YES : NO;

    if (!kitStringTables) kitStringTables = [HashTable newKeyDesc:"*"];

    if (![kitStringTables isKey:tableName]) {
	while (!table && *languages) {
	    if (!strcmp(*languages, KIT_LANGUAGE)) {
		isEnglish = YES;
		break;	/* The kit is written in English */
	    }
	    sprintf(file, "%s/Languages/%s/%s.strings", _NXAppKitFilesPath, *languages, tableName);
	    table = [NXStringTable newFromFile:file];
	    languages++;
	}
	if (!table) {
	    if (debugLanguages && !isEnglish) NXLogError("Cannot parse '%s' strings.", tableName);
	    table = [NXStringTable new];
	}
	[kitStringTables insertKey:tableName value:table];
    } else {
	table = [kitStringTables valueForKey:tableName];
    }

    value = [table valueForStringKey:key];

    if (!value) {
	if (debugLanguages && !isEnglish) {
	    NXLogError("Cannot find value for string '%s' in table '%s'.", key, tableName);
	}
	[table insertKey:key value:(void *)key];
    }

    return value ? value : key;
}

const char *_NXKitString2(const char *tableName, const char *key, BOOL *inOrder)
{
    const char *retval = _NXKitString(tableName, key);
    if (inOrder) *inOrder = YES;
    if (*retval == '>' || *retval == '<') {
	if (inOrder && (*retval == '<')) *inOrder = NO;
	retval++;
    }
    return retval;
}

- (const char *)appListenerPortName
{
    return [self appName];
}

- appListener
{
    return appListener;
}

- setAppListener:aListener
{
    appListener = aListener;	/* ??? shouldn't this free the old one? */
    return self;
}

- appSpeaker
{
    return appSpeaker;
}

- setAppSpeaker:aSpeaker
{
    appSpeaker = aSpeaker;
    return self;
}

- (int)unhide
{
    [self unhide:self];
    return 0;
}


/* BIG HACK ALERT!  This global is set whenever we are working on an openFile message.  We use this fact way down in Window's -orderWindow... method to know to do some special window ordering.
In this special case, we will call a package proc that safely orders the window to the front (i.e., does not cover up a window that the user clicked on in the meantime).
To do this, we need to know the event timestamp of the event that initiated the openFile: message.  This is stored in a global in the WindowServer.  See, it really is a hack.  */

static BOOL doingOpenFile = NO;

- (BOOL)_isDoingOpenFile
{
    return doingOpenFile;
}

- (int)openFile:(const char *)fullPath ok:(int *)flag
{
    return [self _doOpenFile:fullPath ok:flag tryTemp:NO];
}

- (int)openTempFile:(const char *)fullPath ok:(int *)flag
{
    return [self _doOpenFile:fullPath ok:flag tryTemp:YES];
}

- (int)_doOpenFile:(const char *)fullPath ok:(int *)flag tryTemp:(BOOL)tryTemp
{
    char                fname[MAXPATHLEN + 1];
    int                 namelen;
    char               *theType;
    id                  rParty;

    if ((rParty = whoIsResponsible(self, @selector(appAcceptsAnotherFile:))) &&
	[rParty appAcceptsAnotherFile:self]) {
	*flag = YES;
	namelen = strlen(fullPath);
	if (namelen <= MAXPATHLEN) {
	    bcopy(fullPath, fname, namelen + 1);
	    if (theType = strrchr(fname, '.'))
		theType++;
	    else
		theType = "";
	    if (appFlags.hidden)
		[self unhideWithoutActivation:self];
	    [self activateSelf:NO];
	    NX_DURING {
		doingOpenFile = YES;
		if (tryTemp && (rParty = whoIsResponsible(self, @selector(app:openTempFile:type:))))
		    *flag = [rParty app:self openTempFile:fname type:theType];
		else if (tryTemp && (rParty = whoIsResponsible(self, @selector(appOpenTempFile:type:))))
		    *flag = [rParty appOpenTempFile:fname type:theType];
		else if (rParty = whoIsResponsible(self, @selector(app:openFile:type:)))
		    *flag = [rParty app:self openFile:fname type:theType];
		else if (rParty = whoIsResponsible(self, @selector(appOpenFile:type:)))
		    *flag = [rParty appOpenFile:fname type:theType];
	    } NX_HANDLER {
		doingOpenFile = NO;	/* be sure to reset this to clear */
		NX_RERAISE();
	    } NX_ENDHANDLER;
	    doingOpenFile = NO;
	} else
	    *flag = NO;
    } else
	*flag = rParty ? -1 : NO;	/* rParty -> app doesn't accept
					 * another file */
    return 0;
}

- (int)unmounting:(const char *)fullPath ok:(int *)flag
{
    char                fname[MAXPATHLEN + 1];
    const char	       *home;
    int                 namelen;
    id                  rParty;
    struct stat         sbud;
    struct stat         sbwd;


    *flag = NO;
    namelen = strlen(fullPath);
    if (namelen <= MAXPATHLEN) {
	bcopy(fullPath, fname, namelen + 1);
	if (rParty = whoIsResponsible(self, @selector(app:unmounting:)))
	    *flag = [rParty app:self unmounting:fname];
	else if (rParty = whoIsResponsible(self, @selector(appUnmounting:)))
	    *flag = [rParty appUnmounting:fname];
	else {
	    if (stat(".", &sbwd) >= 0 && stat(fname, &sbud) >= 0) {
		*flag = YES;
		if ((sbwd.st_dev & 0xFFF8) == (sbud.st_dev & 0xFFF8)) {
		    home = NXHomeDirectory();
		    if (chdir((home && home[0] == '/') ? home : "/") < 0)
			*flag = NO;
		}
	    }
	}
    }
    return 0;
}

- (int)powerOffIn:(int)ms andSave:(int)aFlag
{
    int *errorData;

    NXAllocErrorData( sizeof(int)*2, &(void *)errorData );
    errorData[0] = ms;
    errorData[1] = aFlag;
    NX_RAISE(NX_powerOff, errorData, 0);
    return 0;
}

- (BOOL)isActive
{
    return appFlags.active;
}


- (const char *)hostName
{
    return hostName;
}


- (BOOL)isHidden
{
    return appFlags.hidden;
}


- (BOOL)_isRunningModal
{
    return running > 1;
}

- (BOOL)isRunning
{
    return running;
}


- (int)activeApp
{
    int                 activeApp;

    _NXGetActiveApp(&activeApp);
    return activeApp;
}


- deactivateSelf
{
    int                 ping;

    endKeyAndMain(self);
    _NXDeactivateSelf(&ping);
    return self;
}


- (int)activateSelf:(BOOL)flag
{
    int                 ping;
    int                 oldActive = [self activeApp];

    _NXActivateSelf((int)flag, &ping);
    return oldActive;
}


- (int)activate:(int)contextNumber
{
    int                 activeApp = [self activeApp];

    _NXActivateContext(contextNumber);
    return activeApp;
}


/** Process Management **/

- _replyToLaunch
{
    const char         *launchStr;
    int                 mseq;	/* sequence number for authenticating reply
				 * to launch */
    int                 globalWinNum;	/* global window num for app's icon */
    int                 localWinNum;	/* local window num for app's icon */
    port_t              wsport;
    NXRect              iconFrame, cRect;
    extern const char  *NXWorkspaceName;
    char               *wsname;

    if (wsname = getenv(NXWorkspaceName))
	NXWorkspaceName = wsname;
    if (launchStr = NXGetDefaultValue(_NXAppKitDomainName, "MachLaunch")) {
    /* ??? this global win num is a headbutt for broadcasting PS code */
	sscanf(launchStr, "%d%d", &mseq, &globalWinNum);
	wsport = NXPortFromName(NX_WORKSPACEREPLY, NULL);
	NXSendAcknowledge(wsport, [self->appListener listenPort],
					mseq, 1, NX_SENDTIMEOUT);
	PSsendint(globalWinNum);
	localWinNum = DPSDefineUserObject(0);
	PScurrentwindowbounds(localWinNum,
				&iconFrame.origin.x, &iconFrame.origin.y,
				&iconFrame.size.width, &iconFrame.size.height);
	[Window getContentRect:&cRect forFrameRect:&iconFrame
						style:NX_TOKENSTYLE];
	_appIcon = [[Window allocFromZone:[self zone]] initContent:&cRect style:NX_TOKENSTYLE backing:NX_BUFFERED buttonMask:0 defer:YES];
	[_appIcon _wsmIconInitFor:localWinNum];
	_NXInitOtherOwner(localWinNum);
    } else {
	_NXGetIconFrame(&iconFrame);
	[Window getContentRect:&cRect forFrameRect:&iconFrame style:NX_MINIWORLDSTYLE];
	_appIcon = [[Window allocFromZone:[self zone]] initContent:&cRect style:NX_MINIWORLDSTYLE backing:NX_BUFFERED buttonMask:0 defer:NO];
	[_appIcon setTitle:[_mainMenu title]];
	_NXSetIcon([_appIcon windowNum]);
	[_appIcon display];
	[_appIcon orderFront:self];
    }
    return self;
}


- _replyToOpen:(int)ok
{
    const char         *launchStr;
    int                 mseq;	/* sequence number for authenticating reply */
    int                 wnum;	/* global window number for app's icon */
    port_t              wsport;

    if (launchStr = NXGetDefaultValue(_NXAppKitDomainName, "MachLaunch")) {
    /* ??? this global win num is a headbutt for broadcasting PS code */
	sscanf(launchStr, "%d%d", &mseq, &wnum);
	wsport = NXPortFromName(NX_WORKSPACEREPLY, NULL);
	NXSendAcknowledge(wsport,
			  [self->appListener listenPort],
			  mseq,
			  ok,
			  NX_SENDTIMEOUT);
    }
    return self;
}


static void workSpaceFeedBack(id appIcon)
{
    NXPoint iconOrigin;

    if ([appIcon _wsmOwnsWindow]) {
	[[appIcon contentView] lockFocus];
    /*	PSnewinstance(); */
	PSsetinstance(FALSE);
	PSsetgray(NX_LTGRAY);
	PSrectfill(2.0, 2.0, NX_TOKENWIDTH - 4.0, NX_TOKENHEIGHT - 4.0);
	iconOrigin.x = (NX_TOKENWIDTH - NX_ICONWIDTH)/2.0;
	iconOrigin.y = (NX_TOKENHEIGHT - NX_ICONHEIGHT)/2.0;
	[[NXImage findImageNamed:"app"] composite:NX_SOVER toPoint:&iconOrigin];
	[[appIcon contentView] unlockFocus];
	[appIcon display];
    }
}


- run
{
/* This loop has two DURING/HANDLERS next to each other due to the following
   constraints:  1) We want initialization code to be covered by handlers.
   2) You cant goto code inside a DURING, yet we need to restart the loop on
   an exception  3) We want this to really be the top level handler so
   NXDefaultTopLevelHandler can know not to re-raise.  Therefore, we cant
   nest handlers.
 */
    void                (*oldHandler)() = NULL;	/* initted for clean -Wall */
    static BOOL		firstRun = YES;		/* initted for clean -Wall */
    id                  rParty;
    const char	       *isJournalableStr;

    MARKTIME(_NXLaunchTiming, "[Application run] setup", 0);
    NX_DURING {
	oldHandler = signal(SIGINT, &handle_signal);
	if (firstRun) {
	    [self activateSelf:NO];
	    if (rParty = whoIsResponsible(self, @selector(appWillInit:)))
		[rParty appWillInit:self];
	    initListener(self);		/* calls openFile: */
	}
	running = 1;
    } NX_HANDLER {
	(*_NXTopLevelErrorHandler) (&NXLocalHandler);
    } NX_ENDHANDLER
  /* be sure we init the app, even if there are initial errors */
    NX_DURING {
	if (firstRun) {
	    workSpaceFeedBack(_appIcon);
	    if (rParty = whoIsResponsible(self, @selector(appDidInit:)))
		[rParty appDidInit:self];
	    [self _initServicesMenu];
	    if (isJournalableStr = NXGetDefaultValue(appNameAtStartup, "NXIsJournalable"))
	        isJournalable = strcmp (isJournalableStr, "NO") ? YES : NO;
	}
	firstRun = NO;
    } NX_HANDLER {
	(*_NXTopLevelErrorHandler) (&NXLocalHandler);
    } NX_ENDHANDLER
 /* These nested loops let us re-enter the run loop upon errors */
      while (running) {
	NX_DURING {		/* loop has its own handler, so its
				 * restartable */
	    while (running) {
		NXResetUserAbort();
#if defined(CHECK_PS_STACK) && defined(DEBUG)
		checkStack();
#endif
		if (_freelist && [_freelist count])
		    [_freelist freeObjects];
		if (keyWindow && CURSORS_INVALID(keyWindow))
		    [keyWindow resetCursorRects];
		MARKTIME(_NXLaunchTiming, "Getting next event", 0);
		NXGetOrPeekEvent(DPS_ALLCONTEXTS, &currentEvent, NX_ALLEVENTS,
			         NX_FOREVER, NX_BASETHRESHOLD, 0);
		appFlags._invalidEvent = NO;
		MARKTIME2(_NXLaunchTiming, "Sending event type %d, %d", currentEvent.type, currentEvent.data.compound.subtype, 0);
		[self sendEvent:&currentEvent];
		NXResetErrorData();
	    }
	} NX_HANDLER {
	    [self makeWindowsPerform:@selector(_resetDisableCounts) inOrder:NO];
	    if (NXLocalHandler.code == NX_powerOff) {
	    /*
	     * !!! cmf DO NOT RENAME TO powerOffIn:andSave: will conflict !!!
	     * powerOffIn:andSave: 
	     */
		if (rParty = whoIsResponsible(self, @selector(app:powerOffIn:andSave:)))
		    [rParty app:self powerOffIn:((int *)(NXLocalHandler.data1))[0]
		    		andSave:((int *)(NXLocalHandler.data1))[1]];
		else if (rParty = whoIsResponsible(self, @selector(appPowerOffIn:andSave:)))
		    [rParty appPowerOffIn:((int *)(NXLocalHandler.data1))[0]
		    		andSave:((int *)(NXLocalHandler.data1))[1]];
		return self;
	    } else if (NXLocalHandler.code != NX_journalAborted)
		(*_NXTopLevelErrorHandler) (&NXLocalHandler);
	    PSshowcursor();		/* to make sure it isnt left hidden */
	} NX_ENDHANDLER
    }
    signal(SIGINT, oldHandler);
    return self;
}


static void initListener(Application *self)
{
    int                 tempFlag;
    const char         *fname = NULL;
    const char	       *name;

    if (!self->appListener)
	self->appListener = [[Listener allocFromZone:[self zone]] init];
    [self->appListener setDelegate:self];
    if (name = [self appListenerPortName])
	[self->appListener checkInAs:name];
    if ([self->appListener listenPort] == PORT_NULL)
	[self->appListener usePrivatePort];
    [self->appListener addPort];
    [self _replyToLaunch];
    self->replyPort = PORT_NULL;
    (void)port_allocate(task_self(), &self->replyPort);
    if (!self->appSpeaker)
	self->appSpeaker = [[Speaker allocFromZone:[self zone]] init];
    if ([self->appSpeaker replyPort] == PORT_NULL)
	[self->appSpeaker setReplyPort:self->replyPort];
    [self->appSpeaker setDelegate:self];
    if (fname = NXGetDefaultValue(name, "NXOpen")) {
	(void)[self openFile:fname ok:&tempFlag];
	[self _replyToOpen:(tempFlag ? 3 : 2)];
    } else if (fname = NXGetDefaultValue(name, "NXOpenTemp")) {
	(void)[self openTempFile:fname ok:&tempFlag];
	[self _replyToOpen:(tempFlag ? 3 : 2)];
    }
}


#if defined(CHECK_PS_STACK) && defined(DEBUG)
static void checkStack(void)
{
    int                 stackCount;

    PScount(&stackCount);
    AK_ASSERT(stackCount == 0, "Extra tokens on operand stack:");
    PSpstack();			/* dump them back in ascii */
    PSflush();
}
#endif

/* used to pass return code from stopModal: to runModalFor: and
   runModalSession:
 */
static int runModalReturn = 0;

/* event mask for running modal panels */
#define MODAL_FILTER_MASK (NX_LMOUSEDOWNMASK | NX_LMOUSEUPMASK |\
			   NX_RMOUSEDOWNMASK | NX_RMOUSEUPMASK |\
			   NX_MOUSEMOVEDMASK |\
			   NX_LMOUSEDRAGGEDMASK | NX_RMOUSEDRAGGEDMASK)

extern NXHandler *_NXAddAltHandler(void (*proc)(void *data, int code,
					  void *data1, void *data2),
				   void *context);
extern void _NXRemoveAltHandler(NXHandler *errorData);

- (int)runModalFor:theWindow
{
    NXHandler		exception;
    volatile NXModalSession session;

    exception.code = 0;
    [self beginModalSession:(NXModalSession *)(&session) for:theWindow];
    NX_DURING {
	while (running > session.oldRunningCount) {
	    if (CURSORS_INVALID(theWindow))
		[theWindow resetCursorRects];
	    [self getNextEvent:NX_ALLEVENTS waitFor:NX_FOREVER
	     threshold:NX_RUNMODALTHRESHOLD];
	    if (((~MODAL_FILTER_MASK) & (1 << currentEvent.type)) ||
		(currentEvent.window == session.winNum) ||
		([[self findWindow:currentEvent.window] worksWhenModal]))
		[self sendEvent:&currentEvent];
	}
    } NX_HANDLER {
	exception = NXLocalHandler;
    } NX_ENDHANDLER
 /* restore window level, even on an error */
    [self endModalSession:(NXModalSession *)(&session)];
    if (exception.code == NX_abortModal)
	return NX_RUNABORTED;
    else if (exception.code != 0)
	NX_RAISE(exception.code, exception.data1, exception.data2);

    return runModalReturn;
}


- stop:sender
{
    running = 0;
    runModalReturn = NX_RUNSTOPPED;
    return self;
}


- stopModal
{
    return [self stopModal:NX_RUNSTOPPED];
}


- stopModal:(int)returnCode
{
    if (running > 1)
	running--;
    runModalReturn = returnCode;
    return self;
}


- (void)abortModal
{
    if (running > 1)
	running--;
    NX_RAISE(NX_abortModal, NULL, NULL);
}

#define MODAL_OFFSET	66

- _orderFrontModalWindow:theWindow
{
    if (![theWindow isVisible])		/* reposition only if not visible */
	if (!_NXLastModalSession || !_NXLastModalSession->window)
	    [theWindow center];
	else {
	    NXRect              lastFrame;
	    NXRect              currFrame;
	    float               newPosX, newPosY;
    
	    [_NXLastModalSession->window getFrame:&lastFrame];
	    [theWindow getFrame:&currFrame];
	    newPosX = lastFrame.origin.x + MODAL_OFFSET;
	    newPosY = NX_MAXY(&lastFrame) - MODAL_OFFSET;
	    if (newPosY - currFrame.size.height < 0)
		newPosY = currFrame.size.height;
	    if (newPosX + currFrame.size.width > screenSize.width)
		newPosX = screenSize.width - currFrame.size.width;
	    [theWindow moveTopLeftTo:newPosX :newPosY];
	}
    [theWindow _doOrderWindow:NX_ABOVE relativeTo:0 findKey:NO level:NX_MAINMENULEVEL+1];
    return self;
}

- (NXModalSession *)beginModalSession:(NXModalSession *)session for:theWindow
{
    if (!session) {
	NX_ZONEMALLOC([theWindow zone], session, NXModalSession, 1);
	session->freeMe = YES;
    } else
	session->freeMe = NO;
    session->app = self;
    session->window = theWindow;
    [self _orderFrontModalWindow:theWindow];
    session->winNum = [theWindow windowNum];
    [theWindow makeKeyWindow];
    session->prevSession = _NXLastModalSession;
    _NXLastModalSession = session;
    session->oldDoesHide = [theWindow doesHideOnDeactivate];
    [theWindow setHideOnDeactivate:NO];
    session->oldRunningCount = running;
    running++;
    session->errorData = _NXAddAltHandler(&runModalCleanup, session);
    return session;
}


- (int)runModalSession:(NXModalSession *)session
{
    if (running > session->oldRunningCount)
	runModalReturn = NX_RUNCONTINUES;
    while (running > session->oldRunningCount) {
	if (CURSORS_INVALID(session->window))
	    [session->window resetCursorRects];
	if ([self getNextEvent:NX_ALLEVENTS waitFor:0.0
				threshold:NX_RUNMODALTHRESHOLD]) {
	    if (((~MODAL_FILTER_MASK) & (1 << currentEvent.type)) ||
		(currentEvent.window == session->winNum))
		[self sendEvent:&currentEvent];
	} else
	    break;	/* no more events, return */
	}
    return runModalReturn;
}


- endModalSession:(NXModalSession *)session
{
    _NXRemoveAltHandler(session->errorData);
    runModalCleanup(session, 0, NULL, NULL);
    return self;
}


static void runModalCleanup(void *data, int code, void *data1, void *data2)
{
    NXModalSession *session = (NXModalSession *)data;
    Application *app;

    [session->window _setWindowLevel:NX_NORMALLEVEL];
    [session->window setHideOnDeactivate:session->oldDoesHide];
    _NXLastModalSession = session->prevSession;
    app = (Application *)(session->app);
    app->running = session->oldRunningCount;
    if (session->freeMe)
	NX_FREE(session);
}


- setAutoupdate:(BOOL)flag
{
    appFlags.autoupdate = flag;
    return self;
}


- terminate:sender
{
    id rParty;
 
    NX_DURING
	if (rParty = whoIsResponsible(self, @selector(appWillTerminate:)))
	    if (![rParty appWillTerminate:self])
		NX_VALRETURN(self);
	[Pasteboard _provideAllPromisedData];
	_NXActivateNextApp();
	NXPing();
	[self free];
	DUMPTIMES(_NXLaunchTiming);
    NX_HANDLER
      /* catch error to exit no matter what */
	NXReportError(&NXLocalHandler);
    NX_ENDHANDLER
    exit(0);
    return self; /* to quiet compiler warning */
}


/** Event Processing **/

static void convertEvent(NXEvent *eventPtr)
{
    if (NX_EVENTCODEMASK(eventPtr->type) &
		(NX_KEYDOWNMASK|NX_KEYUPMASK|NX_FLAGSCHANGEDMASK))
	eventPtr->window = [((Application *)NXApp)->keyWindow windowNum];
}


NXEvent *NXGetOrPeekEvent(DPSContext context, 
			  NXEvent *eventPtr,
			  int mask, 
			  double timeout, 
			  int threshold, 
			  int peek)
 /*
  * A cover function for DPSGetEvent and DPSPeekEvent that deals with
  * journaling the events.
  */
{
    id              appSpeaker, requestee;
    int             gotOne, appNum, journalStatus;
    BOOL            jrnEvent;
    NXEvent         junkEvent;
    port_t          thePort, masterPort;
    extern port_t   name_server_port;

    do {
	jrnEvent = NO;
	eventPtr->time = 0;
	eventPtr->window = 0;
	gotOne = _DPSGetOrPeekEvent(context, eventPtr, mask, timeout, threshold, peek);
    /*
     * We must re-get journalStatus since the process of
     * getting the event might have caused _setStatus: to be called which
     * could change this value. 
     */
	_getJournalStatus(&journalStatus);
	if (gotOne) {
	    ((Application *)NXApp)->appFlags._invalidEvent = NO;
	    if ((journalStatus != NX_PLAYING) || !(eventPtr->flags & NX_JOURNALFLAGMASK))
		convertEvent(eventPtr);
	    if ((journalStatus == NX_STOPPED) &&
		(eventPtr->flags & NX_JOURNALFLAGMASK) &&
		(eventPtr->flags != -1) &&
		isJournalable) {
	    /* Need to set up a journaler and start recording events. */
		appSpeaker = [NXApp appSpeaker];
		if (netname_look_up(name_server_port, "", NX_JOURNALREQUEST, &thePort) ==
		    NETNAME_SUCCESS) {
		    if (!_slaveJournaler)
			[NXJournaler _newSlave];	    
		    if (_masterJournaler)
		    /*
		     * We are going to be recording events from ourself.  We
		     * can't initJournaling with Listener/Speaker since it
		     * blocks when there is returned data. 
		     */
			requestee = _masterJournaler;
		    else {
			[appSpeaker setSendPort:thePort];
			requestee = appSpeaker;
		    }
		    if (![requestee _requestInitJournaling:[NXApp appName]
			   :[[_slaveJournaler listener] listenPort]
			   :&appNum
			  :&masterPort]) {
			[_slaveJournaler _setApplicationNum:appNum];
			[[_slaveJournaler speaker] setSendPort:masterPort];
			[_slaveJournaler _setStatus:NX_RECORDING];
			journalStatus = NX_RECORDING;
		    }
		}
		if (journalStatus == NX_STOPPED) {
		/*
		 * We were not successful in connecting to a master
		 * journaler. 
		 */
		    [_slaveJournaler _disposeJournalEvents];
		    _NXSetJournalRecording(FALSE);
		}
	    }
	    if (journalStatus != NX_STOPPED) {
	    /* Check to see if this is a special journal event. */
		if (eventPtr->type == NX_JOURNALEVENT ||
		    (eventPtr->type == NX_MOUSEMOVED &&
		     [[NXApp mainMenu] windowNum] == eventPtr->window))
		    jrnEvent = YES;
	    /*
	     * If we are playing the journal then decode the event. If we
	     * are recording then we only want to actually record the
	     * event if we are getting rather than peeking. 
	     */
		if (journalStatus == NX_PLAYING) {
		    if (eventPtr->flags & NX_JOURNALFLAGMASK)
			[_slaveJournaler _decodeJournalEvent:eventPtr];
		/*
		 * Check to see if we have a NX_LASTJRNEVENT.  We have
		 * dispatched this event to ourselves when the slave
		 * journaler got a _setStatus:NX_STOPPED message. This
		 * event serves as a sentinel, indicating that there will
		 * be no more journaled events coming.  We can now free
		 * the slave journaler. 
		 */
		    if (eventPtr->type == NX_JOURNALEVENT &&
		    eventPtr->data.compound.subtype == NX_LASTJRNEVENT) {
			[_slaveJournaler _setStatus:NX_STOPPED];
			[_slaveJournaler free];
		    }
		} else if (!peek || jrnEvent)
		    [_slaveJournaler _sendEventForRecording:eventPtr];
	    /*
	     * If we are peeking and we have a special journal event we
	     * need to get the event out of the queue. 
	     */
		if (peek && jrnEvent)
		    DPSGetEvent(context, &junkEvent, mask, timeout, threshold);
	    }
	}
    /*
     * If we actually got an event, but it was a journal event, do it
     * again. 
     */
    } while (gotOne && jrnEvent);
    if (gotOne) {
	if (journalStatus != NX_PLAYING)
	    eventPtr->flags &= ~NX_JOURNALFLAGMASK;
	return eventPtr;
    } else
	return NULL;
}
	
	

- (NXEvent *) getNextEvent:(int)mask waitFor:(double)timeout
    threshold:(int)level
{
    return NXGetOrPeekEvent(DPSGetCurrentContext(),
			    &currentEvent,
			    mask,
			    timeout,
			    level,
			    0);
}


- (NXEvent *) peekNextEvent:(int)mask into:(NXEvent *) eventPtr
    waitFor:(float)timeout threshold:(int)level
{
    return NXGetOrPeekEvent(DPSGetCurrentContext(),
			    eventPtr,
			    mask,
			    timeout,
			    level,
			    1);
}


- (NXEvent *) getNextEvent:(int)mask
{
    return NXGetOrPeekEvent(DPSGetCurrentContext(),
			    &currentEvent,
			    mask,
			    NX_FOREVER,
			    NX_MODALRESPTHRESHOLD,
			    0);
}


- (NXEvent *) peekAndGetNextEvent:(int)mask
{
    return NXGetOrPeekEvent(DPSGetCurrentContext(),
			    &currentEvent,
			    mask,
			    0.0,
			    NX_MODALRESPTHRESHOLD,
			    0);
}


- (NXEvent *) peekNextEvent:(int)mask into:(NXEvent *) eventPtr
{
    return NXGetOrPeekEvent(DPSGetCurrentContext(),
			    eventPtr,
			    mask,
			    0.0,
			    NX_MODALRESPTHRESHOLD,
			    1);
}


- (NXEvent *)currentEvent
{
    return (&currentEvent);
}


- powerOff:(NXEvent *)theEvent
{
    if (!NXGetDefaultValue(_NXAppKitDomainName, "MachLaunch")) {
	if (delegate != self && [delegate respondsTo:@selector(powerOff:)])
	    return[delegate powerOff:theEvent];
	else if ([delegate respondsTo:@selector(appPowerOff:)])
	    return[delegate appPowerOff:theEvent];
    }
    return self;
}


- applicationDefined:(NXEvent *)theEvent
{
    if ((delegate != self) && [delegate respondsTo:@selector(applicationDefined:)])
	return[delegate applicationDefined:theEvent];
    else
	return self;
}


- rightMouseDown:(NXEvent *)theEvent
{
    if (running == 1)	/* no pop-up during modal runs */
	[_mainMenu rightMouseDown:theEvent];
    return self;
}


static BOOL isAnotherDectEvent(void)
{
    NXEvent ev;

    if (DPSPeekEvent(DPSGetCurrentContext(), &ev, NX_KITDEFINEDMASK, 0.0, 31))
	if (ev.data.compound.subtype == NX_APPDEACT)
	    return YES;
    return NO;
}


- sendEvent:(NXEvent *)theEvent
{
    extern void	    _DPSPrintEvent(FILE *fp, NXEvent *evTo);
    id              win;
    int             windowNum;
    int		    i;
    BOOL	    updateOKWithThisEvent =
		    !(theEvent->type == NX_CURSORUPDATE ||
			theEvent->type == NX_MOUSEDRAGGED ||
			(theEvent->type == NX_KITDEFINED &&
			(theEvent->data.compound.subtype == _NX_APPUNHIDE ||
			theEvent->data.compound.subtype == NX_APPDEACT ||
			    (theEvent->data.compound.subtype == NX_APPACT &&
			    theEvent->data.compound.misc.L[1] != 0))));

    if (_NXTraceEvents) {
	fprintf(stderr, "In Application: ");
	_DPSPrintEvent(stderr, theEvent);
    }
    switch (theEvent->type) {
    case NX_KITDEFINED:
	switch (theEvent->data.compound.subtype) {
	case NX_APPACT:
	    windowNum = theEvent->data.compound.misc.L[1];
	  /*
	   * Check if we are activating because we clicked on a window that
	   * has since been hidden.  If so, we dont want to do the activation.
	   */
	    if (hiddenCount && hiddenWindows && hiddenList) {
		for (i = hiddenCount; --i >= 0; )
		    if (hiddenList[i] == windowNum)
			break;
		if (i >= 0)
		    break;
	    }

	    if (isAnotherDectEvent())
		appFlags._deactPending = YES;
	    {
		BOOL            keyWindowHit;
	      /* says if a key window was picked as part of the activate */
		id              newKeyWindow = nil;

		appFlags.active = YES;
		if (!(theEvent->flags & NX_JOURNALFLAGMASK))
		/*
		 * Written by pkgs.  In the case where we are playing a
		 * journal script, the current activation is set from the
		 * _playEventsFilter proc in NXJournaler.m 
		 */
		    currentActivation = theEvent->data.compound.misc.L[0];
		[NXArrow set];
		keyWindowHit = (windowNum != 0);
		if (keyWindowHit) {
		/* see if the window hit is in this app */
		    newKeyWindow = [self findWindow:windowNum];
		    keyWindowHit = (newKeyWindow != nil);
		}
		if (keyWindowHit) {
		/* see if the window is a window that takes keys */
		    keyWindowHit = [newKeyWindow _visibleAndCanBecomeKey];
		}
		if (!keyWindowHit || [self _isRunningModal]) {
		/*
		 * This is the case where the app was activated by some
		 * means other that a click in one of our windows, or else
		 * we dont want to change the key or main window based on
		 * such a click because we have a modal panel. Most likely
		 * a unhide from the workspace. They key and main windows
		 * were probably set up from the app itself. Otherwise,
		 * this stuff is set up by changeKeyAndMain in the Window
		 * sendEvent: method. 
		 */
		    _NXOrderKeyAndMain();
		    _NXShowKeyAndMain();
		} else {
		/*
		 * this is the case where a key window has been hit. The
		 * old main and key window need to be erased at this time 
		 */
		    appFlags._invalidState = YES;
		}
		if ([_mainMenu windowNum] == -1)
		    [_mainMenu _doOrderWindow:NX_ABOVE relativeTo:0 findKey:YES level:NX_MAINMENULEVEL];
		[self becomeActiveApp];
	    }
	    break;
	case NX_APPDEACT:
	    appFlags._deactPending = NO;
	    appFlags.active = NO;
	    endKeyAndMain(self);
	    [self resignActiveApp];
	    break;

	case NX_WINEXPOSED:
	    [[self findWindow:theEvent->window] windowExposed:theEvent];
	    break;

	case NX_SCREENCHANGED:
	    [[self findWindow:theEvent->window] screenChanged:theEvent];
	    break;

	case NX_WINRESIZED:
	    [[self findWindow:theEvent->window] windowResized:theEvent];
	    break;

	case NX_WINMOVED:
	    [[self findWindow:theEvent->window] windowMoved:theEvent];
	    break;

	case _NX_APPUNHIDE:
#ifdef AFTERWARP4
	    [self unhideWithoutActivation:self];	/* ??? do this */
#else
	    [self unhide:self];
#endif
	/*
	 * This code is a little test to see what PS dumps of windows is
	 * like case 13: [[self
	 * findWindow:theEvent->data.compound.misc.L[0]]
	 * _remotePrintWindow:theEvent]; break; case 14: [[self
	 * findWindow:theEvent->data.compound.misc.L[0]]
	 * _remoteCopyWindow:theEvent]; break; 
	 */

	}
	break;
    case NX_KEYUP:
	if (!([keyWindow eventMask] & NX_KEYUPMASK) || (theEvent->flags & NX_COMMANDMASK))
	    return self;
    case NX_KEYDOWN:
	if (theEvent->flags & NX_COMMANDMASK) {
	    register unsigned short c;
	    register BOOL   didIt = NO;
	    register int    numWindows;
	    register unsigned offset;
	    register id     theWindow;

	    if (theEvent->data.key.repeat)
		break;

	    if (theEvent->flags & NX_ALPHASHIFTMASK &&
		!(theEvent->flags & NX_SHIFTMASK)) {
		c = theEvent->data.key.charCode;
		if (isascii(c) && isupper(c))
		    theEvent->data.key.charCode = tolower(c);
	    }
	    numWindows = [windowList count];
	    for (offset = 0; offset < numWindows; offset++) {
		theWindow = [windowList objectAt:offset];
		if (didIt = [theWindow commandKey:theEvent])
		    break;
	    }
	    if (didIt)
		break;
	}
	if (keyWindow)
	    [keyWindow sendEvent:theEvent];
	else
	    NXBeep();
	break;
    case NX_FLAGSCHANGED:
	if (keyWindow)
	    [keyWindow sendEvent:theEvent];
	break;
    case NX_LMOUSEDOWN:
    case NX_LMOUSEUP:
    case NX_RMOUSEUP:
    case NX_MOUSEMOVED:
    case NX_LMOUSEDRAGGED:
    case NX_RMOUSEDRAGGED:
    case NX_MOUSEENTERED:
    case NX_MOUSEEXITED:
	win = [self findWindow:theEvent->window];
	[win sendEvent:theEvent];
	break;

    case NX_RMOUSEDOWN:
	win = [self findWindow:theEvent->window];
	if (win) {
	    [win sendEvent:theEvent];
	} else {
	    [self rightMouseDown:theEvent];
	}
	break;

    /*
     * ??? some temp hack to get convert events, but we have to deal with
     * app_defined events anyways somehow. 
     */
    case NX_SYSDEFINED:
	switch (theEvent->data.compound.subtype) {
	case NX_POWEROFF:
	    [self powerOff:theEvent];
	    break;
	}
	break;
    case NX_APPDEFINED:
	[self applicationDefined:theEvent];
	break;
    case NX_CURSORUPDATE:
	handleCursorRect(self, theEvent);
	break;
    case NX_JOURNALEVENT:
	return self;
	break;
    default:
	NXLogError( "Unrecognized event in [Application -sendEvent:], type = %d", theEvent->type);
	break;
    }

 /*
  * If its a cursor event, an unhide event, a deactivate event, or an
  * activate where the user clicked on a window (thus a mouse down is on
  * its way), we dont do menu updating. 
  */
    if (updateOKWithThisEvent) {
	if (appFlags.autoupdate) {
	    [self updateWindows];
	} else {
	    if ([[[FontManager new] getFontMenu:NO] isVisible]) [[FontManager new] _updateMenuItems];
	    if ([self _servicesMenuIsVisible]) [[self servicesMenu] update];
	}
    }
 /*
  * If we are not running any modal panels, after we have dispatched this
  * event it becomes stale.  This is done so that SafeOrderFront doesnt
  * use the time on this stale event to sync ordering windows with the
  * user's other clicks.  This is essentially marking that response to
  * this event is complete.  In 3.0, we might just want to NULL out
  * currentEvent once we're through with it. 
  */
    if (running == 1)
	appFlags._invalidEvent = YES;
    return self;
}


- becomeActiveApp
{
    if ([delegate respondsTo:@selector(appDidBecomeActive:)])
	[delegate appDidBecomeActive:self];
    return self;
}


- resignActiveApp
{
    if ([delegate respondsTo:@selector(appDidResignActive:)])
	[delegate appDidResignActive:self];
    return self;
}

/*
 * Get screen information from the server.  This method is pretty fast after
 * the first call. Two assumptions here: Screen configuration doesn't change
 * after window server startup, and an app only talks to one server at one
 * time.
 */
- getScreens:(const NXScreen **)list count:(int *)count
{
    if (!_screens) {
	int cnt;
	PScountframebuffers (&cnt);
	if (!(_numScreens = cnt)) {
#ifdef DEBUG
	    NXRect rect = {{0.0, 0.0}, {1120.0, 832.0}};
	    NXLogError ("Can't obtain screen configuration from server; guessing.\n");
	    NX_ZONEMALLOC([self zone], _screens, NXScreen, 1);
	    _screens[0].screenBounds = rect;
	    _screens[0].depth = NX_TwoBitGrayDepth;
	    _screens[0].screenNumber = 0;
	    _numScreens = 1;
#else
	    NXLogError ("Can't obtain screen configuration from server!\n");
#endif
	} else {
	    NX_ZONEMALLOC([self zone], _screens, NXScreen, _numScreens);
	    for (cnt = 0; cnt < _numScreens; cnt++) {
		NXRect *r = &(_screens[cnt].screenBounds);
		int depth;
		_NXGetScreenInfo (cnt,
				  &NX_X(r), &NX_Y(r), &NX_WIDTH(r), &NX_HEIGHT(r), &depth);
		_screens[cnt].depth = depth;
		_screens[0].screenNumber = cnt;
		/*
		 * Assure that _screens[0] is the "zero screen," ie, the
		 * screen whose origin is at 0,0.
		 */
		if ((NX_X(r) == 0.0) && (NX_Y(r) == 0.0) && (cnt != 0)) {
		    NXScreen tmp = _screens[cnt];
		    _screens[cnt] = _screens[0];
		    _screens[0] = tmp;
		}
	    }
#ifdef DEBUG
	    if ((NX_X(&_screens[0].screenBounds) != 0.0) ||
		(NX_Y(&_screens[0].screenBounds) != 0.0)) {
		NXLogError ("No zero screen!\n");
	    }
#endif
	}
    }
    if (list) {
	*list = _screens;
    }
    if (count) {
	*count = _numScreens;
    }
    return self;
}

/*
 * Return the main screen. This method is guaranteed to return a screen.
 */
- (const NXScreen *)mainScreen
{
    const NXScreen *screens, *screen;
    int screenCount;

    [self getScreens:&screens count:&screenCount];
    if (screenCount == 1) {	/* Special case one screen; lots of those. */
	screen = &screens[0];
    } else if ((!(screen = [[self _keyWindow] screen])) &&
	       (!(screen = [[self mainMenu] screen]))) {
	screen = &screens[0];
    }
    return screen;
}

/*
 * Return the screen whose origin is at 0,0.
 */
- (const NXScreen *)_zeroScreen
{
    const NXScreen *screens;

    [self getScreens:&screens count:NULL];
    return &screens[0];
}

/*
 * Return the screen that can best represent color. Try to find any screen if
 * no color screen exists. This method is guaranteed to return a screen.
 */
- (const NXScreen *)colorScreen
{
    const NXScreen *screens, *bestScreen = NULL;
    int screenCount, cnt;

    [self getScreens:&screens count:&screenCount];
    if (screenCount == 1) {	/* Special case one screen; lots of those. */
	bestScreen = &screens[0];
    } else {
	int bestBPS = 0;
	for (cnt = 0; cnt < screenCount; cnt++) {
	    /* Come up with a magic cookie, (color? << 8) + bps. */
	    int curBPS = (NXNumberOfColorComponents (
		NXColorSpaceFromDepth(screens[cnt].depth)) > 1) << 8 + 
		NXBPSFromDepth(screens[cnt].depth);
	    if (curBPS > bestBPS) {
		bestScreen = &screens[cnt];
		bestBPS = curBPS;
	    }
	}
    }
    return bestScreen;
}

- frontWindow
{
    int                 frontWinNumber;

    NXWindowList(1, &frontWinNumber);
    return [self findWindow:frontWinNumber];
}


- makeWindowsPerform:(SEL)aSelector inOrder:(BOOL)flag
{
    register int        i;
    register id         theWindow;
    int                 numWindows;
    int                *windowArray;

    _NXCheckSelector(aSelector, "Application -makeWindowsPerform:inOrder:");
    if (flag) {
	NXCountWindows(&numWindows);
	windowArray = (int *)alloca(numWindows * sizeof(int));
	NXWindowList(numWindows, windowArray);
	for (i = 0; i < numWindows; i++) {
	    theWindow = [self findWindow:windowArray[i]];
	    if ((BOOL)[theWindow perform:aSelector])
		return theWindow;
	}
    } else {
	numWindows = [windowList count];
	for (i = 0; i < numWindows; i++) {
	    theWindow = [windowList objectAt:i];
	    if ((BOOL)[theWindow perform:aSelector])
		return theWindow;
	}
    }
    return nil;
}

- appIcon
{
    return _appIcon;
}

- windowList
{
    return windowList;
}

- getWindowNumbers:(int **)list count:(int *)winCount
{
    int                 theCount;
    register int       *theList;

    NXCountWindows(&theCount);
    theList = NXZoneMalloc([self zone], theCount * 4);
    NXWindowList(theCount, theList);
    *winCount = theCount;
    *list = theList;
    return self;
}

- getScreenList:(int **)list count:(int *)winCount
{
    return [self getWindowNumbers:list count:winCount];
}


- (BOOL)_updateFontMenu:cell
{
    return NO;
}

- updateWindows
{
    if (delegate && [delegate respondsTo:@selector(appWillUpdate:)]) {
	[delegate appWillUpdate:self];
    }
    [self updateWindowUsingCache];
    if (delegate && [delegate respondsTo:@selector(appDidUpdate:)]) {
	[delegate appDidUpdate:self];
    }
    return self;
}

- (BOOL)tryToPerform:(SEL)anAction with:anObject
{
    if ([super tryToPerform:anAction with:anObject])
	return YES;
    if ([delegate respondsTo:anAction])
	if ([delegate perform:anAction with:anObject])
	    return YES;
    return NO;
}

- validRequestorForSendType:(NXAtom)sendType andReturnType:(NXAtom)returnType
{
    if (delegate != self && ((![delegate isKindOf:[Responder class]] || ![delegate nextResponder]) && appFlags._delegateReturnsValidRequestor)) {
	return [delegate validRequestorForSendType:sendType andReturnType:returnType];
    }
    return nil;
}

static id
findResponder(id obj, SEL action, BOOL tryFirst)
{
    id                  delegate;
    id                  responder;

    responder = tryFirst ?[obj firstResponder] : obj;
    for (; responder; responder = [responder nextResponder])
	if ([responder respondsTo:action])
	    return responder;

    delegate = [obj delegate];
    if (delegate && [delegate respondsTo:action])
	return delegate;

    return nil;
}


id _NXCalcTargetForAction(SEL theAction)
{
    register id         mainWindow;
    register id         keyWindow;
    id                  receiver = nil;

    if (theAction) {
	keyWindow = [NXApp keyWindow];
	receiver = findResponder(keyWindow, theAction, YES);
	if (!receiver) {
	    mainWindow = [NXApp mainWindow];
	    if (mainWindow != keyWindow)
		receiver = findResponder(mainWindow, theAction, YES);
	    if (!receiver)
		receiver = findResponder(NXApp, theAction, NO);
	}
    }
    return receiver;
}


- calcTargetForAction:(SEL)theAction;
{
    _NXCheckSelector(theAction, "Application -calcTargetForAction:");
    return _NXCalcTargetForAction(theAction);
}

static BOOL tryToPerform(id self, id obj, SEL action, BOOL tryFirst)
{
    id                 responder = obj;

    if (tryFirst)
	responder = [obj firstResponder];
    if (responder && [responder tryToPerform:action with:self]) {
	return YES;
    }
    return NO;
}

- (BOOL)sendAction:(SEL)theAction to:theTarget from:sender
{
    id		    theMainWindow, theKeyWindow, application;
    BOOL	    isView = [sender isKindOf:[View class]];

    _NXCheckSelector(theAction, "NXApp - sendAction:to:from:");
    if (theAction) {
	theKeyWindow = [self keyWindow];
	if ([self _isRunningModal]) {	/* worksWhenModal */
	    if (theTarget &&
		(![theTarget respondsTo:@selector(worksWhenModal)] || ![theTarget worksWhenModal]) &&
		(!isView || (([sender window] != (_NXLastModalSession ? _NXLastModalSession->window : nil)) &&
			    (![[sender window] isKindOf:[PopUpList class]]) && 
			    ([[sender window] delegate] != theTarget) &&
		            (![theTarget isKindOf:[View class]] || [theTarget window] != [sender window])))) {
		return NO;
	    }
	    theMainWindow = (_NXLastModalSession ? _NXLastModalSession->window : nil);
	    if (theKeyWindow == theMainWindow && isView && [sender window] == (_NXLastModalSession ? _NXLastModalSession->window : nil)) {
		theMainWindow = (_NXLastModalSession && _NXLastModalSession->prevSession) ? _NXLastModalSession->prevSession->window : [self mainWindow];
	    }
	    application = (theMainWindow == [self mainWindow]) ? self : nil;
	} else {
	    theMainWindow = [self mainWindow];
	    application = self;
	}
	if (theTarget) {
	    return [theTarget perform:theAction with:sender] ? YES : NO;
	} else {
	    if (tryToPerform(sender, theKeyWindow, theAction, YES))
		return YES;
	    if (theMainWindow != theKeyWindow) {
		if (tryToPerform(sender, theMainWindow, theAction, YES))
		    return YES;
	    }
	    if (tryToPerform(sender, application, theAction, NO))
		return YES;
	}
    }

    return NO;
}

- printInfo
{
    if (!_printInfo)
	_printInfo = [[PrintInfo allocFromZone:[self zone]] init];
    return _printInfo;
}


- setPrintInfo:info
{
    id                  oldInfo = _printInfo;

    _printInfo = info;
    return oldInfo;
}

- runPageLayout:sender
{
    [[PageLayout new] runModal];
    return self;
}

- orderFrontColorPanel:sender
{
    [[NXColorPanel newColorMask:NX_ALLMODESMASK] orderFront:self];
    return self;
}

 /* pasteboard support */

- pasteboard
{
    if (!_pboard)
	_pboard = [Pasteboard new];
    return _pboard;
 /*
  * Depending on how much we support multiple connections on one or more
  * machines, we may have have one of these per machine. 
  */
}


/* Menu support */

- setMainMenu:aMenu
{
    if (_mainMenu) {
	[_mainMenu close];
	[_mainMenu _setTempHidden:NO];
	if ([_mainMenu windowNum] >= 0)
	    [_mainMenu _setWindowLevel:NX_SUBMENULEVEL];
    }
    _mainMenu = aMenu;
    if ([aMenu windowNum] >= 0)
	[aMenu _setWindowLevel:NX_MAINMENULEVEL];
    return self;
}


- mainMenu
{
    return _mainMenu;
}


/* delayed freeing of objects */

- delayedFree:theObject
{
    if (!_freelist) {
	_freelist = [[List allocFromZone:[self zone]] init];
    }
    [_freelist addObject:theObject];
    return self;
}


- getScreenSize:(NXSize *)theSize
{
    const NXScreen *screen = [NXApp mainScreen];
    *theSize = screen->screenBounds.size;
    return self;
}

/* Names */


static id nameTable(Application *self)
{
    if (!self->_nameTable)
	self->_nameTable = [[NameTable allocFromZone:[self zone]] init];
    return self->_nameTable;
}


id NXGetNamedObject(name, owner)
    id                  owner;
    const char         *name;
{
    return[nameTable(NXApp) getObjectForName :name owner:owner];
}

int NXNameObject(const char *name, id object, id owner)
{
    if ([nameTable(NXApp) addName:name owner:owner forObject:object])
	return 1;
    else
	return 0;
}

int NXUnnameObject(const char *name, id owner)
{

    if ([nameTable(NXApp) removeName:name owner:owner])
	return 1;
    else
	return 0;
}

const char *NXGetObjectName(id theObject)
{
    id                  theOwner;

    return[nameTable(NXApp) getObjectName :theObject andOwner:&theOwner];
}

- _removeWindow:aWindow
{
    [windowList removeObject:aWindow];
    if (keyWindow == aWindow)
	keyWindow = nil;
    if (mainWindow == aWindow)
	mainWindow = nil;
    [self removeWindowFromCache:aWindow];
    return self;
}

- _addWindow:aWindow
{
    BOOL overridesUpdate = _NXSubclassSel([aWindow class], @selector(update));
    BOOL doesUpdate = [((WindowId *)aWindow)->delegate respondsTo:@selector(windowDidUpdate:)];
  /*
   * We look at the instance var directly here because that parallels the
   * test made in Window's update method.  If you dont do this, classes like
   * Tile in WSM crash because they override -delegate, yet arent ready for the
   * message when the window is added to the list.
   */

    [windowList addObjectIfAbsent:aWindow];
    if (!overridesUpdate && [aWindow isKindOf:[Menu class]])
	overridesUpdate = YES;
    [self setOverridesUpdate:overridesUpdate forWindow:aWindow];
    [self setDelegateUpdates:doesUpdate forWindow:aWindow];
    return self;
}


void _NXGetIconFrame(register NXRect *frame)
{
    int                 iconNumber;
    static short        maxIconsAcross = 0;
#define maxIconsUp  2		/* as long as its a constant... */

    if (maxIconsAcross == 0) {
	NXSize              screen;

	[NXApp getScreenSize:&screen];
	maxIconsAcross = (int)floor(screen.width / NX_TOKENWIDTH) - 1;
    }
    _NXGetIconNumber(&iconNumber);
    NX_WIDTH(frame) = NX_TOKENWIDTH;
    NX_HEIGHT(frame) = NX_TOKENHEIGHT;
    NX_X(frame) = (iconNumber % maxIconsAcross) * NX_TOKENWIDTH;
    NX_Y(frame) = ((iconNumber / maxIconsAcross) % maxIconsUp) * NX_TOKENHEIGHT;
}


/* handles errors raised outside of any DURING/HANDLER construct */
static void uncaughtErrorProc(int code, const void *data1, const void *data2)
{
    NXHandler           myHandler;

    NXLogError("An uncaught exception was raised\n");
    bzero(&myHandler, sizeof(NXHandler));
    myHandler.code = code;
    myHandler.data1 = data1;
    myHandler.data2 = data2;
    (*_NXTopLevelErrorHandler) (&myHandler);
}


static void handle_signal(int i)
{
    abortRaised = YES;
}


/* returns whether user has hit cmd-. */
BOOL NXUserAborted()
{
    return abortRaised;
}


/* resets the cmd-. flag */
void NXResetUserAbort()
{
    abortRaised = NO;
}


/* NIB methods */

#import "nibprivate.h"

/* given a source, a StreamTable or an Archiver, this function retrieves the MAINDATAENTRY, and generates the appropriate side-effects for the other entries */
/* this function also closes source */
/* Due to complex reasons, the structure of the files is a 2 level tier: */
/* The entry MAINDATAENTRY contains a table mapping entry number to entries. */
static id getNibData(id source)
{
    id                  nibData;
    id                  table = [source valueForStreamKey:(void *)MAINDATAENTRY];

/*
 * the entries MAINIMAGEENTRY and MAINSOUNDENTRY of table are read for
 * side effect 
 */
    nibData = [table valueForKey:(void *)MAINDATAENTRY];
    [((id)[table valueForKey:(void *)MAINSOUNDENTRY]) free];
    [((id)[table valueForKey:(void *)MAINIMAGEENTRY]) free];
    [table free];
    [source free];
    return nibData;
}


static id doNibLoad(NXTypedStream *s, id *nameTable, id owner, BOOL namesFlag, NXZone *zone)
{
    id nibData, firstWindow;
    id table;

    MARKTIME(_NXLaunchTiming, "[Application _loadNibSegment] getNibData", 0);
    PROFILEMARK('L', YES);		/* profile loading nib data */
    NXSetTypedStreamZone(s, zone);
    table = NXReadObject(s);
    nibData = getNibData(table);
    PROFILEMARK('L', NO);
    MARKTIME(_NXLaunchTiming, "*[Application _loadNibSegment] instantiate", 0);
    NXCloseTypedStream(s);

    if (namesFlag && !*nameTable)
	*nameTable = [[NameTable allocFromZone:[NXApp zone]] init];
    PROFILEMARK('N', YES);            /* profile whole instantiation */
    firstWindow = [nibData nibInstantiateIn:(namesFlag ? *nameTable : nil) owner:owner];
    PROFILEMARK('N', NO);
    MARKTIME(_NXLaunchTiming, "[Application _loadNibSegment] glue", 0);
    [nibData free];
    return firstWindow;
}


- loadNibFile:(const char *)fileName owner:anOwner
{
    return [self loadNibFile:fileName owner:anOwner withNames:YES];
}

- loadNibFile:(const char *)fileName owner:anOwner withNames:(BOOL)flag
{
    return [self loadNibFile:fileName owner:anOwner withNames:flag fromZone:NXDefaultMallocZone()];
}

- loadNibFile:(const char *)fileName owner:anOwner withNames:(BOOL)flag fromZone:(NXZone *)zone
{
    NXTypedStream *s;
    id ret = nil;

    PROFILEMARK('U', YES);		/* profile whole unarchiving */
    if (access(fileName, R_OK) == 0) {
	s = NXOpenTypedStreamForFile(fileName, NX_READONLY);
	if (s)
	    ret = doNibLoad(s, &_nameTable, anOwner, flag, zone);
    }
    PROFILEMARK('U', NO);
    return ret;
}

- loadNibSection:(const char *)sectionName owner:anOwner
{
    return [self _loadNibSegment:"__NIB" section:sectionName owner:anOwner withNames:YES fromShlib:NO fromHeader:NULL fromZone:NXDefaultMallocZone()];
}

- loadNibSection:(const char *)sectionName owner:anOwner withNames:(BOOL)flag
{
    return [self _loadNibSegment:"__NIB" section:sectionName owner:anOwner withNames:flag fromShlib:NO fromHeader:NULL fromZone:NXDefaultMallocZone()];
}

- loadNibSection:(const char *)sectionName owner:anOwner withNames:(BOOL)flag fromZone:(NXZone *)zone
{
    return [self _loadNibSegment:"__NIB" section:sectionName owner:anOwner withNames:flag fromShlib:NO fromHeader:NULL fromZone:zone];
}

- loadNibSection:(const char *)sectionName owner:anOwner withNames:(BOOL)flag fromHeader:(const struct mach_header *)mhp
{
    return [self _loadNibSegment:"__NIB" section:sectionName owner:anOwner withNames:flag fromShlib:NO fromHeader:mhp fromZone:NXDefaultMallocZone()];
}

- loadNibSection:(const char *)sectionName owner:anOwner withNames:(BOOL)flag fromHeader:(const struct mach_header *)mhp fromZone:(NXZone *)zone
{
    return [self _loadNibSegment:"__NIB" section:sectionName owner:anOwner withNames:flag fromShlib:NO fromHeader:mhp fromZone:zone];
}

/*
 * This is the big one. Either fromShlib is YES, or fromHeader has a value, 
 * or we look in the actual MachO of the app and the launch dir.
 */
- _loadNibSegment:(const char *)segmentName section:(const char *)sectionName owner:anOwner withNames:(BOOL)flag fromShlib:(BOOL)fromShlib fromHeader:(const struct mach_header *)mhp fromZone:(NXZone *)zone
{
    id                  firstWindow = nil;
    NXTypedStream      *s;
    char               *data;
    int                 size;
    NXStream           *stream = NULL;

    MARKTIME(_NXLaunchTiming, "[Application _loadNibSegment] setup", 0);
    PROFILEMARK('U', YES);		/* profile whole unarchiving */
    if (fromShlib) {
	if (data = getsectdatafromlib("libNeXT_s", (char *)segmentName, (char *)sectionName, &size))
	    stream = NXOpenMemory(data, size, NX_READONLY);
    } else if (mhp) {
	if (data = getsectdatafromheader((struct mach_header *)mhp, (char *)segmentName, (char *)sectionName, &size))
	    stream = NXOpenMemory(data, size, NX_READONLY);
    } else
	stream = _NXOpenStreamOnSection(segmentName, sectionName);

    if (stream) {
	s = NXOpenTypedStream(stream, NX_READONLY);
	if (s)
	    firstWindow = doNibLoad(s, &_nameTable, anOwner, flag, zone);
	NXClose(stream);
    }
    PROFILEMARK('U', NO);
    return firstWindow;
}


- slaveJournaler
{ 
   return _slaveJournaler;
}

  
- _setSlaveJournaler:newJournaler
{
    NX_ASSERT (!(newJournaler && _slaveJournaler), "Application -- slave Journaler already exists");
    _slaveJournaler = newJournaler;
    return self;
}


- masterJournaler
{
    return _masterJournaler;
}


- _setMasterJournaler:newJournaler
{
    NX_ASSERT (!(newJournaler && _masterJournaler), "Application -- master Journaler already exists");
    _masterJournaler = newJournaler;
    return self;
}


- setJournalable:(BOOL)newStatus
{
    isJournalable = newStatus;
    return self;
}

- (BOOL) isJournalable
{
    return isJournalable;
}

static void _getJournalStatus(int *status)
{
    *status = NX_STOPPED;
    [_slaveJournaler getEventStatus:status
     soundStatus:NULL
     eventStream:NULL
     soundfile:NULL];
}

static void endKeyAndMain(Application *self)
{
    id                  key = self->keyWindow;
    id                  main = self->mainWindow;

    _NXSendWindowNotification(key, @selector(resignKeyWindow));
    _NXSendWindowNotification(main, @selector(resignMainWindow));
    [key _displayTitle];
    if (key != main)
	[main _displayTitle];
    if (cursorStack)
	cursorStack->chunk.used = 0;
}


static Application *whoIsResponsible(Application *self, SEL selector)
{

    if (self->delegate && [self->delegate respondsTo:selector])
	return self->delegate;
    if ([self respondsTo:selector])
	return self;
    return nil;
}


static void handleMallocError(int code)
{
     NXHandler fakeError;

  /* ??? Use NX_RAISE when this doesnt happen all the time */
     fakeError.code = NX_mallocError;
     fakeError.data1 = (void *)code;
     fakeError.data2 = NULL;
     NXReportError(&fakeError);
     /* NX_RAISE(NX_mallocError, (void *)code, (void *)0); */
}

static void popCursorRect()
{
    if (cursorStack && cursorStack->chunk.used > 0)
	cursorStack->chunk.used -= sizeof(NXCursorRect *);
}

#define AREA(x) ( (x)->size.width * (x)->size.height )

static void bigEnough()
{
    if (!cursorStack)
	cursorStack = (NXCursorStack *)
	NXChunkZoneMalloc(0, 2 * sizeof(NXCursorRect *), [NXApp zone]);
    if (cursorStack->chunk.used == cursorStack->chunk.allocated)
	cursorStack = (NXCursorStack *)NXChunkZoneRealloc(&cursorStack->chunk, [NXApp zone]);
}


static void pushCursorRect(NXCursorRect * cRect)
{
    int                 index, pos, i;
    register NXCursorRect **list;
    NXCoord             area;

    bigEnough();
    index = cursorStack->chunk.used / sizeof(NXCursorRect *);
    pos = index;
    list = cursorStack->rects;
    if (index) {
	area = AREA(&cRect->cursorRect);
	for (i = 0; i < index; i++) {
	    register NXRect    *rect = &(list[i]->cursorRect);

	    if (area > AREA(rect)) {
		pos = i;
		break;
	    }
	}
    }
    for (i = index; i > pos; i--) {
	list[i] = list[i - 1];
    }
    list[pos] = cRect;
    cursorStack->chunk.used += sizeof(NXCursorRect *);
}


static NXCursorRect *topOfCursorStack(void)
{
    int                 index;

    if (cursorStack && cursorStack->chunk.used > 0) {
	index = (cursorStack->chunk.used - sizeof(NXCursorRect *)) /
	  sizeof(NXCursorRect *);
	return cursorStack->rects[index];
    } else
	return NULL;
}


static void handleCursorRect(Application *self, NXEvent *theEvent)
{
    NXCursorRect       *cRect;
    NXEvent             event;
    int                 wnum = theEvent->window;
    id                  window = [self findWindow:wnum];
    int                 index;

    if (!window || window != self->keyWindow)
	return;
    while (1) {
	index = -1 - theEvent->data.tracking.trackingNum;
	cRect = _NXIndexToCursorRect(window, index);
	switch (theEvent->flags & ~NX_JOURNALFLAGMASK) {
	    case NX_MOUSEENTERED:
		if (cRect)
		    pushCursorRect(cRect);
		break;
	    case NX_MOUSEEXITED:
		popCursorRect();
		break;
	    default:
		NXLogError("Bad cursor rect event, flags = %d", theEvent->flags);
		continue;
	}
	if (![self peekNextEvent:NX_CURSORUPDATEMASK into:&event
		waitFor:0.0 threshold:31])
	    break;
	if (event.window != wnum)
	    break;
	theEvent = [self getNextEvent:NX_CURSORUPDATEMASK
		    waitFor:NX_FOREVER threshold:31];
    }
    cRect = topOfCursorStack();
    if (self->appFlags.active && window == self->keyWindow) {
	if (cRect)
	    [cRect->cursor set];
	else
	    [NXArrow set];
    } else if (cursorStack)
	cursorStack->chunk.used = 0;
}

extern void _NXResetCursorState(int windowNum, NXCursorRemove which, BOOL show)
{
    NXEvent             event, *theEvent;
	while ([NXApp peekNextEvent:NX_CURSORUPDATEMASK into:&event
		waitFor:0.0 threshold:31]) {
	    switch (which) {
		case eventsFor:
		    if (event.window != windowNum)
			goto done;
		    break;
		case otherEvents:
		    if (event.window == windowNum)
			goto done;
		    break;
		case allEvents:
		    break;
	    }
	    theEvent = [NXApp getNextEvent:NX_CURSORUPDATEMASK
			waitFor:NX_FOREVER threshold:31];
	}
  done:
    if (cursorStack)
	cursorStack->chunk.used = 0;
    if (show)
	[NXArrow set];
}


static void makeAppIconBitmap(void)
{
#if 0
    id                  bitmap;

    bitmap = [Bitmap findBitmapFor:"app"];
    if (!bitmap) {
	bitmap = _NXBuildBitmapFromMacho("__ICON", "__tiff", [Bitmap class]);
	if (!bitmap) bitmap = [Bitmap findBitmapFor:"defaulticon"];
	[Bitmap addName:"app" bitmap:bitmap];
    }
#endif
}


- _restoreCursor
{
    NXCursorRect *cRect = topOfCursorStack();

    [(cRect ? cRect->cursor : NXArrow) set];
    return self;
}


- (int) _currentActivation 
{
    return currentActivation;
}


- _setCurrentActivation:(int)activationCount
{
    currentActivation = activationCount;
    return self;
}


/* reads app name from MachO, or if that fails uses argv[0] */
static char *getAppName(void)
{
    NXStream *st;
    char nameBuf[MAXPATHLEN+1];
    char *name = nameBuf;
    int newChar;

    *name = '\0';
    st = _NXOpenStreamOnMacho("__ICON", "__header");
    if (st) {
	do {
	    if (NXGetc(st) == 'F') {
		NXScanf(st, "%*[ 	]%*s%*[ 	]%s", name);
		break;
	    } else {
		NXUngetc(st);	/* in case of line with only a newline */
		do {
		    newChar = NXGetc(st);
		} while (newChar != '\n' && newChar != EOF);
	    }
	} while (newChar != EOF);
	NXClose(st);
    }
    if (!*name) {
	name = strrchr(NXArgv[0], '/');
	if (!name)
	    name = NXArgv[0];
	else
	    name++;
    }
    return NXCopyStringBuffer(name);
}

- _sendRequestServerData:(char *)data length:(int)length
{
    int akserverVersion;
    _NXUpdateRequestServers(_NXLookupAppkitServer(&akserverVersion, NULL, NULL), (char *)NXUserName(), strlen(NXUserName())+1, data, length);
    return self;
}

- _sendFileExtensionData:(char *)data length:(int)length
{
    int akserverVersion;
    _NXUpdateFileExtensions(_NXLookupAppkitServer(&akserverVersion, NULL, NULL), data, length);
    return self;
}

@end

/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the new List object
12/13/88 trey	added code to set malloc error proc to raise an error
		  (#ifdef'ed out until 0.81 C library)
		removed peekNextEvent:waitFor:threshold: (was only for
		  back compatibility)
12/13/88 bgy	converted to the new List object;
01/03/89 bgy	added appDidUpdate: delegate method to updateWindows
    		 added new method, deactivateSelf
01/19/89 cmf    launch feedback in _appIcon, clear instance drawing
01/26/89 trey	appkit installs a handler with malloc_error to catch out
		 of memory exceptions
02/03/89 trey	added _loadNibSegment:section:owner as a private entry
		 point to read nibs out of the mach-o.
02/05/89 bgy	added support for cursor tracking. Both the run & runModalFor:
		 now check every time though their loop for a keywindow
		 with invalid cursorRects. sendEvent: has a new arm for 
		 NX_CURSORUPDATE, which will go through the event queue 
		 looking for cursor rect events. A stack is maintained of 
		 the nesting of the cursor rects. It is necessary to keep 
		 this stack sorted since the tracking based events are not
		 guaranteed to return in any particular order.
02/11/89 trey	removed methods to get and set Page Layout and Print Panels
		 use factory methods of those classes instead
 2/13/89 trey	changed error: message to NX_ASSERT when sendEvent has an
 		 unrecognized event
 2/17/89 trey	made runModalFor: center the panel its running, with nested
		 calls relatively offsetting successive panels
 2/16/89 trey	create appIcon named bitmap at start-up
02/09/89 cmf	added support for unmounting:ok: message sent by WM to
		application to get it to enable unmounting of Optical disks. 
		default behavior is to chdir() if working dir. is on disk to
		be unmounted. Added another application delegate method
		appUnmounting:.
02/13/89 cmf    added code to take name of workspace request port from
		 workspace environment variable passed by workspace.
 2/16/89 pah	add _isRunningModal so that we can figure that out easily
  
0.83
----
03/07/89 cmf	initialize ourLastPanel in runModalFor, so that runModal can be
		 called the first time with running != 1.
03/10/89 cmf	change to standard reply to openFile:ok: to distinquish between
		 OK,Not OK and application doesn't want another file.
 3/16/89 pah	add calcTargetForAction: method
 3/18/89 pah	Make _focusView public
03/21/89 wrp	changed all defaults to capitalized.
03/21/89 wrp	put all kit defaults in Application.
03/22/89 wrp	added const where needed

0.91
----
 5/19/89 trey	minimized static data
		added profiling MARKTIME code
		made showps be enabled before the connection is created,
		 so that all PostScript is shown

0.92
----
 5/19/89 trey	made runModal: loop use NX_RUNMODALTHRESHOLD for a threshold
 5/25/89 pah	add hook to update the Font menu
 5/30/89 pah	add NXHomeDirectory() and NXUserName()
 6/06/89 trey	runModal: doesnt reposition the panel if its already visible
		hostName instance variable and method made const
		-abortModal added
		support for modal sessions added: -beginModalSession:for:,
		 -runModalSession:, -endModalSession:session
		printerHost default now registered to support ChoosePrinter
		 sharing the notion of the current printer
		_NXHostName added
 6/6/89 pah	make NXHomeDirectory() and NXUserName() return NULL if
		 they can't be determined
 6/11/89 wrp	changed displayBorder to _displayTitle where possible
 6/13/89 pah	add defaults stuff for HomeDirectory optimization from WSM
 6/13/89 wrp	converted to use NXCopyStringBuffer()

0.93
----
 6/14/89 pah	add -HomeDirectory and -UserName command-line defaults as
		 optimization from Workspace to get those values
 6/14/89 wrp	center-scanned events to occur in the center of the pixel 
 		 rather than the top-left corner. This makes hit-testing work.
 6/14/89 wrp	changed name of Application icon from appIcon to app
 6/15/89 pah	make appName const char *
 6/17/89 pah	add default for CaseSensitiveBrowser
 6/21/89 trey	all support of old Archiver class removed
		-appName now looks in the MachO for appName, or at argv[0]
		 if that fails
		_NXLookupAppkitServer added to get port to pbs
 6/21/89 wrp	converted defaults to use _NXAppKitName as domain rather 
 		  than NULL
 6/28/89 wrp	changed fprintf's to NXLogError
 6/28/89 trey	default printer changed from np to Local_Printer
 6/30/89 wrp	moved window closing from terminate into free to fix bug #53

0.94
----
 7/07/89 trey	Pasteboard asked to provide outstanding data upon termination
 7/09/89 pah	made NXHomeDirectory() and NXUserName() always return the
		 most sensible non-NULL value available instead of returning
		 NULL if no info can be found (at worst this means / and
		 nobody)
 7/10/89 wrp	changed _focusStack from List to Storage to support error 
 		 recovery
 7/11/89 trey	install typed streams error reporter
		NXLoadPackage made private
 7/16/89 trey	error handling at the start of run: made more robust
 7/21/89 trey	hostName instance var set to name of local host if
		 connection is local (instead of being NULL).
 7/24/89 wrp	fixed sendEvent to early exit if app is hidden and event is a 
 		 user event (key, mouse). This fixes bug #1706

 7/24/89 trey	terminate catches all errors to ensure we exit
 7/25/89 trey	added openTempFile:ok: and NXOpenTemp default
 8/01/89 trey	undid recent -hostname changes
		prevented appNameAtStartup from being freed in -setAppName,
		 fixing apps whose syslog didnt have the app name printed
 8/07/89 trey	changed NXDebugging to _NXShowAllWindows
 8/22/89 wrp	changed NXFixedPitchFontSize to default to 10 (was 12)

76
--
10/29/89 trey	_NXLookupAppkitServer changed to return version
 1/03/90 trey	PBSName default added
		NXWordTablesFile default added
 1/05/90 trey	bumped Application's class version to 1, which represents the
		 version of the kit as a whole
77
--
 1/30/90 trey	nuked _NXInitBaseGState
 2/01/90 trey	nuked unused -_activate method
 2/01/90 trey	fixed key and main bugs
		 hide no longer calls endKeyAndMain (which is done on deact)
		 in unhide, key and main not redrawn if already unhidden
		 nuked showKeyAndMain
		   uses _NXOrderKeyAndMain and _NXOrderKeyAndMain instead
 2/13/90 trey	fixed bug where app icon's Window always had origin of 0,0
		nuked _markBorders
		packages unhide apps, bypassing workspace
		 added _NX_APPUNHIDE subevent
 2/26/90 trey	added NXAllWindowsRetained, NXPSDebug defaults
 2/27/90 trey	windows unhide from front to back instead of back to front
		hiding and showing panels no longer done here (now in packages)
		suppressed meun updates for act/deact/cursor events
 3/01/90 trey	beginModal session relies on orderWindow to createRealWindow
		 to facilitate incremental flush
 3/05/90 trey	added NX_SCREENCHANGED constant
 3/05/90 king	Added NPDName as an AppKit default for npd testing.
 3/05/90 trey	nuked unused displayAllWindows
 3/08/90 trey	kit explicitly redraws app icon to end launch feedback
 3/10/90 trey	added ActivateOnHide, ActivateOnQuit defaults
		new active app is now selected when you quit or hide
		NXPing added in -terminate to ensure all PS is completed
		main menu only ordered front if app is active
79
--
 3/21/90 cmf	added defaults for support of Fax printing.
 4/03/90 pah	changed _NXLastModalPanel to _NXLastModalSession throughout
		added _updateModalMenus to keep the menus up to date as
		 the user transitions between modal sessions
		added tryToPerform: so that when a target/action message is
		 sent down the responder chain, the Application's delegate
		 gets hit.
		added additional test to runModalSession: so that windows
		 that worksWhenModal can get events while the user is running
		 a modal panel.

80
--
  4/4/90 wot	Removed unneeded argument self to convertEvent.
		Added NXAllowJournaling default which controls whether an
		 application participates in Journaling.  Also added a static
		 BOOL allowJournaling which caches that state.  The BOOL
		 allowJournaling can also be set from a new method
		 setAllowJournaling:, allowing runtime control of this
		 behavior.
		The event retrieving stuff has changed substantially
		 internally.  I have added a static cover function for
		 DPSGetEvent and DPSPeekEvent called NXGetOrPeekEvent.  This
		 cover function is now used where the DPS functions used to
		 be.  The new cover deals with journal recording
		 and playback.  It also calls convertEvent for you.
		A new compound event type has been added: NX_JOURNALEVENT.
		Added new private method _setCurrentActivation: to set the 
		 currentActivation instance variable from NXJournaler.m
 4/05/90 trey	added NXNetTimeout, support for settable net timeouts
		rewrote _NXLookupAppkitServer to support looking on other
		 hosts.  Since packages are incompatible, we no longer try to 
		 find old versions of pbs
		_NXHostName caches hostname as a NXAtom
 4/09/90 trey	nuked -pasteboard method, use [Pasteboard new]
 4/09/90 pah	added support for Request Menu
		added sendAction:to:from:
		moved _NXCalcTargetForAction() from Control to here
		don't call updateWindows on NX_MOUSEDRAGGED events
 4/06/90 aozer	Added support for the NX_SCREENCHANGED subevent: window now
		gets a screenChanged:.
 4/08/90 aozer	getScreens:count: to get screen information from the server
 4/09/90 aozer	mainScreen, colorScreen, getBounds:forScreen: to make sense
		of the screen information. getScreenSize: changed to return 
		the size of the main screen only.
 4/09/90 aozer	Added NXColorAccuracy and NXMonochromeAccuracy defaults
 4/09/90 aozer	Made loadNibSection: look at the app launch directory after the
		__NIB segment.  For apps launched from the Shell it looks at
		the current working directory.
81
--
 4/12/90 aozer	Call NXCursor _makeCursors instead of Cursor _makeCursors.
 4/12/90 aozer	Added version of loadNibSection:... to load .nib files
		from a mach header (and not the default one).
82
--
 4/17/90 chris	Removed most of FAX defaults, per trey's request. Moved rest of
 		defaults to system defaults (were in appkit defaults).
83
--
 4/26/90 chris	Added default to store whether user wants to be notified on fax
 		transmission.
 4/29/90 trey	Changed default NXMallocDebug level to 0
		rearranged NXLinkUnreferencedClasses
  5/3/90 pah	fixed request menu submenu code
		ripped out automatic menu disabling when modal
		added request-specific pasteboard name
		added freeing of named pasteboard after sync request
 4/24/90 aozer	Added _zeroScreen to return the window server zero screen.
  5/4/90 wot	Changed call to requestInitJournaling to bypass
		Listener/Speaker when the slaveJournaler and masterJournaler
		are in the same process.
  5/4/90 wot	Added private methods to get and set the masterJournaler.
		Changed the journaler method name to slaveJournaler.
  5/4/90 wot	Added statics for _masterJournaler and _slaveJournaler.
		Remove the journaler instance variable.
  5/4/90 wot	Moved the place where we look at the NXAllowJournaling variable
		from the new method to the run method.  This allows other
		classes to modify the default before it gets cached.
84
--
 5/14/90 pah	Fixed request menu updating to recurse through submenus.
		Put in _ hook to support Request menu working in Workspace.

85
--
 5/15/90 trey	Added NXBrowser and NXSplitView to linkUnreferencedClasses
 5/15/90 aozer	Replaced calls to temporary private funcs in privateWraps
		with the real wraps (accuracy & frame buffer operators)
 6/02/90 trey	remaining hacks in events as sent fromt the window server
		 removed from convertEvent
		Added -NXTraceEvents
		moved some utility functions to systemUtils.m

86
--
 6/12/90 trey	Added NXAllWindowsOneShot
		added  +_userLoggedOut
 6/12/90 pah	Added System defaults Language and Country
		Changed BrowserSpeed default to 400ms
		Moved Request Menu stuff to appRequest.m category
		
87
--
 7/09/90 trey	nuked two flavors of _loadNibSegment
		factored some code into doNibLoad
  7/12/90 glc	Leak plug. Stream wasn't being closed.	
 6/27/90 chris	Added defaults for Fax,FaxHost,FaxResolution
 6/15/90 aozer	Fixed workspaceFeedback to unhighlight one more pixel at top
 6/19/90 aozer	Renamed some methods & a default to fix bug 6419.
		    allowJournaling -> isJournalable
		    setAllowJournaling -> setJournalable:
		    NXAllowJournaling -> NXIsJournalable
 7/11/90 aozer	Removed accuracy stuff
 7/11/90 aozer	Removed call to makeAppIcon; it's now an NXImage

89
--
 7/23/90 chris	Added NX_FAXWANTSHIRES to record hires button state.
 7/20/90 aozer	Added default depth limit setting from NXWindowDepthLimit.

90
--
 8/6/90 aozer	Changed workspaceFeedback() to unhighlight one less pixel
 		 (on JMH's request). Leaves 2 pixels on all sides now.

91
--
 8/6/90 trey	exceptions cause window disable counts to be reset (bug #7383).
 8/7/90 aozer	Made getScreens:count: more robust by making it sure that the
		assumptions are true (that there is at least one screen & 
		one screen is at 0,0).
 8/8/90 trey	yanked weird code in -run to hide the app if no windows 
		visible at launch
 8/9/90 trey	added appWillTerminate: delegate method
		added appWillInit: delegate method
		changed unhighlighting of app tile to happen before appDidInit:
 8/10/90 pah	Moved Object(DelayedPerform) category its own file

92
--
 8/20/90 trey	cursor is shown in top level bombout to prevent it from getting
		 stuck in a hidden state (bug 7460).
 8/20/90 gcockrft  Named the application malloc zone
 8/19/90 bryan	changed findWindow: and updateWindows to use the app cache to 
 		implement their respective functions. The app cache will not
		read all of memory to do these functions.  Also added 
		_addWindow, used by the Window class, to add a new window to
		the system. Both _addWindow and _removeWindow store information
		into the apps cache.

93
--
 9/4/90 trey	prevent recursive unhides
		override clip operator to not have errors, fixing Wingz
		inform dpsclient that we have a 1.0 app so it can override
		 orderwindow for old Below 0 semantics

94
--
 9/25/90 trey	ignore activation clicks on hidden windows.  Fixes the race
		 when clicking on windows as they hide.

95
--
 9/28/90 trey	nuked set -setAppName from the API
		removed NOP'ing of clip errors for Wingz benefit

96
--
 10/8/90 trey	added support for nextstepcontext security operator

98
--
 10/18/90 aozer	Added NXServiceLaunch default (fix 10892)

100
---
 10/24/90 aozer	Changed default for Button & ScrollerButtonPeriod to 0.075.
		On the 030 we never noticed how fast 0.025 really was.

*/

/* Required for compatiblity with 1.0 to turn off .const and .cstring */
#pragma CC_NO_MACH_TEXT_SECTIONS

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

/*
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
	  
	DEFINED IN: The Application Kit
	HEADER FILES: appkit.h
*/

#import "Font.h"
#import "Application.h"
#import "Listener.h"
#import "View.h"
#import "Text.h"
#import "errors.h"
#import "appkitPrivate.h"
#import "objc/hashtable.h"
#import "sys/time.h"

/* from Application.m */

id                  NXApp = nil;/* the global id for the single */
short               NXDrawingStatus = NX_DRAWING;  /* Are we drawing,
						    * printing, or copying? */
const char         *NXSystemFont = 0;
const char         *NXBoldSystemFont = 0;
id                  NXSharedCursors = 0;
id                  NXSharedIcons = 0;
short               garbageFiller = 0;
int                 NXProcessID = 0;
int                 NXNullObject = 0;

BOOL		    _NXLaunchTiming = NO;
BOOL		    _NXAllWindowsRetained = NO;
BOOL		    _NXTraceEvents = NO;
BOOL		    _NXAppkitUnused3 = 0;
const char	   *_NXProfileString = NULL;
id                  _NXAppkitUnused4 = {0};	/* was NXsizingCursors */

extern const char   _literal15[];
NXAtom		    NXSelectionPboard = _literal15;
extern const char   _literal16[];
NXAtom		    NXFindPboard = _literal16;
extern const char   _literal17[];
NXAtom		    NXFontPboard = _literal17;
extern const char   _literal18[];
NXAtom		    NXRulerPboard = _literal18;

int		    _NXNetTimeout = 60000;	/* ms to timeout for net ops */
NXModalSession	   *_NXLastModalSession = NULL;
			/* used to chain running modal sessions together */

/* from Cursor.m */
id                  NXArrow = 0;
id                  NXIBeam = 0;
id                  NXWait = 0;

/* from textUtil.m */

BOOL                NXScreenDump = 0;	
/*
 * Determines if while printing objects draw themselves as they look on the
 * screen (ie, selections get highlighted, isOnColorScreen returns YES or NO
 * depending on window location, etc) 
 */

BOOL                _NXAppkitUnused5 = 0;

/* global proc called in appkit top-level handlers */
void                (*_NXTopLevelErrorHandler) () = NXDefaultTopLevelErrorHandler;

/* from Listener */
NXMessage          *NXInputMessage = (NXMessage *)0;
NXRemoteMethod     *NXInputMethod = (NXRemoteMethod *)0;

/* from Speaker */
int                 NXMustFreeRPCVM = 0;
			/* TRUE iff must free VM and ports in NXRPCMessage */
NXMessage           NXRPCMessage = {0};
			/* Message that holds last RPC result message */
char                NXParamTypes[NX_MAXMSGPARAMS + 1] = {0};
			/* types of current messages parameters */

extern const char   _literal6[];
const char         *NXWorkspaceName = _literal6;

BOOL                _NXShowAllWindows = 0;
BOOL                _NXAppkitUnused6[3] = {0};

struct timeval	    _NXTimeBuffers[10] = {{0,0}};

/* global strings for NeXT standard pasteboard types  These were moved here
   from the text segment so they could be atoms.  The old versions were renamed
   with an _ and left where they were for compatibility.
 */
extern const char   _literal1[];
NXAtom		NXAsciiPboardType = _literal1;
extern const char   _literal3[];
NXAtom		NXPostScriptPboardType = _literal3;
extern const char   _literal4[];
NXAtom		NXTIFFPboardType = _literal4;
extern const char   _literal5[];
NXAtom		NXRTFPboardType = _literal5;
extern const char   _literal11[];
NXAtom		NXFilenamePboardType = _literal11;
extern const char   _literal12[];
NXAtom		NXTabularTextPboardType = _literal12;
extern const char   _literal13[];
NXAtom		NXFontPboardType = _literal13;
extern const char   _literal14[];
NXAtom		NXRulerPboardType = _literal14;

/* CHECK FOR UNUSED FIELDS ABOVE before adding more data down here. */
char                _appkit_data_pad[1814] = {0};


/* global strings for NeXT standard pasteboard types. */
extern const char   _literal1[];
const char         *const _NXAsciiPboardType = _literal1;
extern const char   _literal2[];
const char         *const _NXSmartPaste = _literal2;
extern const char   _literal3[];
const char         *const _NXPostScriptPboardType = _literal3;
extern const char   _literal4[];
const char         *const _NXTIFFPboardType = _literal4;
extern const char   _literal5[];
const char         *const _NXRTFPboardType = _literal5;
extern const char   _literal7[];
const char         *const NXWorkspaceReplyName = _literal7;
extern const char   _literal8[];
const char         *const _NXAppKitFilesPath = _literal8;
extern const char   _literal9[];
const char         *const _NXAppKitDomainName = _literal9;
extern const char   _literal10[];
const char         *const NXSystemDomainName = _literal10;

extern const unsigned char _NXEnglishSmartLeftChars[];
const unsigned char * const NXEnglishSmartLeftChars = _NXEnglishSmartLeftChars;
extern const unsigned char _NXEnglishSmartRightChars[];
const unsigned char * const NXEnglishSmartRightChars=_NXEnglishSmartRightChars;
extern const unsigned char _NXEnglishCharCatTable[];
const unsigned char * const NXEnglishCharCatTable = _NXEnglishCharCatTable;
extern const NXFSM _NXEnglishBreakTable[];
const NXFSM * const NXEnglishBreakTable = _NXEnglishBreakTable;
const int NXEnglishBreakTableSize = 30;
extern const NXFSM _NXEnglishNoBreakTable[];
const NXFSM * const NXEnglishNoBreakTable = _NXEnglishNoBreakTable;
const int NXEnglishNoBreakTableSize = 30;
extern const NXFSM _NXEnglishClickTable[];
const NXFSM * const NXEnglishClickTable = _NXEnglishClickTable;
const int NXEnglishClickTableSize = 80;

extern const unsigned char _NXCSmartLeftChars[];
const unsigned char * const NXCSmartLeftChars = _NXCSmartLeftChars;
extern const unsigned char _NXCSmartRightChars[];
const unsigned char * const NXCSmartRightChars = _NXCSmartRightChars;
extern const unsigned char _NXCCharCatTable[];
const unsigned char * const NXCCharCatTable = _NXCCharCatTable;
extern const NXFSM _NXCBreakTable[];
const NXFSM * const NXCBreakTable = _NXCBreakTable;
const int NXCBreakTableSize = 66;
extern const NXFSM _NXCClickTable[];
const NXFSM * const NXCClickTable = _NXCClickTable;
const int NXCClickTableSize = 484;

extern const char   _literal11[];
const char         *const _NXFilenamePboardType = _literal11;
extern const char   _literal12[];
const char         *const _NXTabularTextPboardType = _literal12;
extern const char   _literal13[];
const char         *const _NXFontPboardType = _literal13;
extern const char   _literal14[];
const char         *const _NXRulerPboardType = _literal14;
extern const char   _literal19[];
const char         *const NXFindPboardType = _literal19;

static const char   _appkit_constdata_pad1[392] = {0};

static const char   _literal1[] = "NeXT plain ascii pasteboard type";
static const char   _literal2[] = "NeXT smart paste pasteboard type";
static const char   _literal3[] = "NeXT Encapsulated PostScript v1.2 pasteboard type";
static const char   _literal4[] = "NeXT TIFF v4.0 pasteboard type";
static const char   _literal5[] = "NeXT Rich Text Format v1.0 pasteboard type";
static const char   _literal6[] = "Workspace";
static const char   _literal7[] = "WorkspaceReply";
static const char   _literal8[] = "/usr/lib/NextStep/";
static const char   _literal9[] = "ApplicationKit";
static const char   _literal10[] = "System";
static const char   _literal11[] = "NeXT filename pasteboard type";
static const char   _literal12[] = "NeXT tabular text pasteboard type";
static const char   _literal13[] = "NeXT font pasteboard type";
static const char   _literal14[] = "NeXT ruler pasteboard type";
static const char   _literal15[] = "NeXT selection pasteboard name";
static const char   _literal16[] = "NeXT find info pasteboard name";
static const char   _literal17[] = "NeXT font pasteboard name";
static const char   _literal18[] = "NeXT ruler pasteboard name";
static const char   _literal19[] = "NeXT obsolete  pasteboard type";

static const char   _appkit_constdata_pad2[1025] = {0};


/*
  
Modifications (starting after 1.0):
  
76
--
12/21/89 trey	English and C word tables exported

77
--
 2/23/90 trey	added _NXAllWindowsRetained
 3/10/90 trey	added _NXLastModalPanel

79
--
  4/3/90 pah	changed _NXLastModalPanel to _NXLastModalSession

83
--
  5/2/90 aozer	Renamed NXPrintSelection to NXScreenDump

*/

/*
	appkitPrivate.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Bill Parkhurst
*/

#import <objc/objc.h>
#import "nextstd.h"
#import "graphics.h"
#import "Application.h"
#import "Font.h"
#import "Window.h"
#import "color.h"
#import <objc/error.h>
#import <sys/time.h>
#import <libc.h>
#import <zone.h>

/* various libNeXT versions.  Call _NXGetShlibVersion() to find the version of libNeXT that the app was linked against. */
#define CURRENT_MINOR_VERS	(atoi(MINOR_VERS_STRING))
#define MINOR_VERS_1_0		(22)

/* The language the Kit assumes */
#define KIT_LANGUAGE "English"

/* kit internal endian management */
#define _NX_BIGENDIAN			0
#define _NX_LITTLEENDIAN		1
/* Configure for your machine to either _NX_BIGENDIAN or _NX_LITTLEENDIAN */
#define _NX_BYTEORDER			_NX_BIGENDIAN

/* Whether Kit panels should be one-shot */
#define NX_KITPANELSONESHOT YES
/* Whether drawing should be optimized in Kit panels */
#define NX_KITPANELSOPTIMIZED YES

/* Kit File Paths  - put here to isolate file system dependencies. */
#define _NX_SPOOLERPATH		"/usr/spool/appkit/"
#define _NX_FONTPATH		"/NextLibrary/Fonts/:"	\
				"~/Library/Fonts/:"	\
				"/LocalLibrary/Fonts/"

/* GLOBAL PATH in appkit_globals.h */
extern const char * const _NXAppKitFilesPath;

extern const char * const _NXAppKitDomainName;

extern BOOL		_NXLaunchTiming;
extern BOOL		_NXShowAllWindows;
extern BOOL		_NXAllWindowsRetained;
extern BOOL		_NXTraceEvents;
extern int		_NXNetTimeout;

/* initial default vals for font and font size.  Used for default
   registration and for rejecting bozo defaults.
 */
#define _NX_INITDEFAULTFONT		"Helvetica"
#define _NX_INITDEFAULTFONTSIZE		"12"

typedef struct _focusStackElem {
    id         view;
    NXHandler *errorData;
    BOOL       valid;
} focusStackElem;


#ifdef AKDEBUG
#define AK_ASSERT(exp, str)		{if(!(exp)) fprintf( stderr, "Appkit Assertion failed: %s\n", str );}
#else AKDEBUG
#define AK_ASSERT(exp, str) {}
#endif AKDEBUG

/* string localization routines */

#define KitString(table, key, comment) _NXKitString(#table, key)
#define KitString2(table, key, inOrder, comment) _NXKitString2(#table, key, inOrder)
extern int _NXKitAlert(const char *table, const char *title, const char *s, const char *f, const char *x, const char *t, ...);
extern const char *_NXKitString(const char *table, const char *key);
const char *_NXKitString2(const char *tableName, const char *key, BOOL *inOrder);

/* private kit function prototypes */

extern BOOL _NXCanAllocInZone(id factory, SEL newSel, SEL initSel);
extern BOOL _NXSubclassSel(id factory, SEL sel);
extern void _NXGetIconFrame(NXRect *frame);
extern void _NXBuildSharedWindows(void);
extern int _NXGetButtonParam(id self, int n);
extern void _NXSetButtonParam(id self, int n, int val);
extern int _NXGetCellParam(id self, int n);
extern void _NXSetCellParam(id self, int n, int val);
extern void _NXDrawTextCell(id cell, id view, const NXRect* rect, BOOL clip);
extern BOOL _NXExtractDouble(const char* string, double* theDouble);
extern id _NXLoadNibPanel(const char *name);
extern id _NXLoadNib(const char *segName, const char *sectName, id owner, NXZone *zone);
extern void _NXCreateRealWindow(Window *self);
extern void _NXGetSharedWindows(int *cursors, int *icons);
extern void _NXInitBaseGState(void);
extern void _NXInitGState(int window, int *gState);
extern void _NXInitGStateWithClip(int window, int *gState, NXRect *clipRect);
extern void _NXSetOtherWindows(int windowNum, int size, int windows[]);
extern int _NXWsmIconGStateCreate(int wnum);
extern int _NXCoverRect(NXRect *aRect, NXRect *bRect);
extern void _NXWindow(char *package, int type, float x, float y, float w, float h, int *num, BOOL isBitmap);
extern void _NXQEraseRect(NXCoord x, NXCoord y, NXCoord width, NXCoord height, float gray);
extern void _NXGetMenuLocation(NXPoint *p);
extern id _NXCalcTargetForAction(SEL theAction);
extern NXCoord _NXtMargin(int style);
extern void _NXImageBitmapWithFlip(const NXRect *rect, int width, int height, int bps, int spp, int planarConfig, int photoInt, const void *data1, const void *data2, const void *data3, const void *data4, const void *data5, BOOL useFlipState);
#ifndef OLD_SPOOLER
extern void _NXNpdSend(int msgID, id pInfo);
#endif
extern id _NXBuildBitmapFromMacho( const char *segmentName,
				const char *sectionName, id factory);
extern NXFontMetrics *
_NXReadFontInfo(NXFontMetrics *stuff, int flags, int file);
extern const char *_NXFindPaperName(const NXSize *paperSize);
extern const char *_NXHostName(void);
extern id _NXCallPanelSuper(id factory, SEL factoryMethod,
		  const NXRect *contentRect, int aStyle, int bufferingType,
		  int mask, BOOL flag);
extern NXStream *_NXOpenStreamOnMacho(const char *segmentName,
					const char *sectionName);
extern NXStream *_NXOpenStreamOnSection(const char *segName,
					const char *sectName);
extern BOOL _NXIsMemoryStream(NXStream *stream);
extern port_t _NXLookupAppkitServer(int *version, const char *hostArg, char *hostFound);
#ifdef DEBUG
extern void _NXCheckSelector(SEL aSelector, const char *classAndMethod);
#else
#define _NXCheckSelector(aSelector, classAndMethod)
#endif
extern void _NXComplement(const unsigned char *src, unsigned char *dest, int size);
extern void _NXAppkitReporter(NXHandler *errorState);
extern void _NXDpsReporter(NXHandler *errorState);
extern void _NXStreamsReporter(NXHandler *errorState);
extern void _NXTypedStreamsReporter(NXHandler *errorState);
extern int _NXLoadPackage(const char *packName);
extern void _NXPureConvertSize(id src, id dest, NXSize *aSize);
extern void _NXTextFontInfoInView(id self, id fid,
		NXCoord *ascender, NXCoord *descender, NXCoord *lineHt);
extern void _NXCheckWordTableSizes(void);
extern void _NXDoDragWindow(float x, float y, int eventNum, int windowNum, 
			    float *newX, float *newY);
extern void _NXOrderKeyAndMain(void);
extern void _NXShowKeyAndMain(void);
extern void _NXSafeOrderFront(int windowNum);
extern void _NXSafeOrderFrontDuringOpenFile(int windowNum);
extern NXModalSession *_NXLastModalSession;
extern void _NXRGBToHSB (float red, float green, float blue, float *hue, float *satuartion, float *brightness);
extern void _NXHSBToRGB (float hue, float saturation, float brightness, float *red, float *green, float *blue);
extern void _NXDrawIcon(id icon, NXRect *toRect, id view);
extern BOOL _NXIconHasAlpha(id icon);
extern id _NXFindIconNamed(const char *name);
extern BOOL _NXIsValidColor(NXColor color);
extern BOOL _NXIsPureGray(NXColor color, float gray);
extern NXColor _NXNoColor();
extern BOOL _NXRemoveAlpha(unsigned char *destData[5],
		unsigned char *srcData[5],
		int width, int height, int bps, int spp,
		int rowBytes, BOOL isPlanar);
extern const char *_NXProfileString;
extern struct timeval _NXTimeBuffers[];
extern const char *_NXGetFileSuffix(const char *filename);
extern int _NXGetShlibVersion(void);
extern void _NXDrawButton(const NXRect *aRect, const NXRect *clipRect, id view);
extern void _NXDrawGrayBezel(const NXRect *aRect, const NXRect *clipRect, id view);
extern void _NXDrawGroove(const NXRect *aRect, const NXRect *clipRect, id view);
extern void _NXDrawGrooveExceptTop(const NXRect *aRect, const NXRect *clipRect, id view);
extern NXRect *_NXDrawTiledRects(NXRect *boundsRect, const NXRect *clipRect,
		const int *sides, const float *grays, int count, id view);
extern void _NXDrawWhiteBezel(const NXRect *aRect, const NXRect *clipRect, id view);
extern void _NXJustFillRects(float gray, float *coords, int num);
extern BOOL _NXEPSGetBoundingBox(NXStream *stream, NXRect *boundingBox);
extern BOOL _NXEPSDeclareDocumentFonts(NXStream *stream);
extern void _NXEPSBeginDocument(const char *docName);
extern void _NXEPSEndDocument(void);
extern void _NXSetGrayUsingPattern (float gray);
extern void _NXSetDepthLimitFromDepthName (const char *name);
extern void _NXGetDepthLimitOfCurrentWindow(int *depth);
extern int _NXSendFileToPS(const char *fileName);
extern int _NXGetDetailedWindowServerMemory(DPSContext context, int *virtualMem, int *bufferedMem, int *retainedMem, int *fixedMem, int *otherMem, NXStream *windowDumpStream);
void _NXSetupPriorities(void);
extern id _NXCallPanelSuperFromZone(id instance, SEL factoryMethod, NXZone *zone);
extern void _NXSendWindowNotification(Window *win, SEL sel);
extern void _NXActivateNextApp(void);
extern void _NXSetLastOpenFileTime(void);
extern BOOL _NXExecDetached(const char *host, const char *fullpath);


/* libc prototypes (still not in libc.h) */

extern void setpwent(void);
extern void endpwent(void);
extern int map_fd();
extern int _filbuf();
extern int _flsbuf();
extern void prnStack();
extern char *getsectdata();
extern char *getsectdatafromheader();
extern const struct section *getsectbynamefromheader();

/* make sure these messages come up regardless of whether debug is on */
#ifndef DEBUG
#undef NX_PSDEBUG()
#define NX_PSDEBUG()		{if ((DPSGetCurrentContext())->chainChild) DPSPrintf( DPSGetCurrentContext(),"\n%% *** Debug *** Object:%d Class:%s Method:%s\n", ((int)self), ((char *)[self name]), SELNAME(_cmd));}
#endif


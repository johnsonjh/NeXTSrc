/*
	Font.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Font_Private.h"
#import <objc/List.h>
#import "Application.h"
#import "View.h"
#import "privateWraps.h"
#import "pbtypes.h"
#import <defaults.h>
#import "errors.h"
#import "nextstd.h"
#import "perfTimer.h"
#define c_plusplus 1		/* for prototypes */
#include "pbs.h"
#undef c_plusplus
#import <dpsclient/wraps.h>
#import <objc/hashtable.h>
#import <stdio.h>
#import <sys/file.h>
#import <sys/param.h>
#import <mach.h>
#import <string.h>
#import <stdlib.h>

static BOOL matricesEqual(const float *m, const float *n);
static void relocate(char *base, void *field);
static void *receiveShmem(int tag, void *data, int length, int maxSize);
static NXZone *FontZone(void);

/* constants describing the max size of the shmems that pbs allocs */
/* must be in sync with pbs */
#define WIDTHS_MAXSIZE		(1024*8)
#define FULLDATA_MAXSIZE	(1024*64)
#define CC_MAXSIZE		(1024*64)
#define STRINGS_MAXSIZE		(1024*64)

#define SCREENPREFIX	"Screen-"	/* String that all screen fonts begin
					 * with */
#define	AFMSUFFIX	".afm"
#define	DFLTGETMASK	(NX_FONTHEADER|NX_FONTMETRICS|NX_FONTWIDTHS)

#define FIRSTFONT ((id *)NX_ADDRESS(fontCltn))

static const char   fontAFMPath[] = _NX_FONTPATH;

static id           fontCltn = nil;	/* An List containing all font
					 * objects */
static NXFaceInfo  *faceList = (NXFaceInfo *)0;	/* Head of linked list of
						 * NXFaceInfos */
static const float  identityMatrix[] = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
static const float  flippedMatrix[] = {1.0, 0.0, 0.0, -1.0, 0.0, 0.0};


@implementation Font

+ (BOOL)_canAlloc { return NO; }

+ allocFromZone:(NXZone *)zone { return [self doesNotRecognize:_cmd]; }

+ alloc { return [self doesNotRecognize:_cmd]; }


+ initialize
{
    if (self == [Font class])
	fontCltn = [[List allocFromZone:FontZone()] init];
    return self;
}


+ (void)_freePBSData
{
    int akServerVersion;
    port_t server;

    if (server = _NXLookupAppkitServer(&akServerVersion, NULL, NULL))
	_NXFreeFontData(server);
}


+ newFont:(const char *)fontName size:(float)fontSize style:(int)fontStyle matrix:(const float *)fontMatrix
{
    register id         fontId;

    if (fontMatrix == NX_IDENTITYMATRIX)
	fontMatrix = identityMatrix;
    else if (fontMatrix == NX_FLIPPEDMATRIX)
	fontMatrix = flippedMatrix;
    if (self == [Font class] && (fontId = [self _findFont:fontName size:fontSize style:fontStyle matrix:fontMatrix]))
	return fontId;
    else {
	self = [super allocFromZone:FontZone()];
	name = NXCopyStringBufferFromZone(fontName, FontZone());
	size = fontSize;
	style = fontStyle;
	if (fontMatrix == identityMatrix || fontMatrix == flippedMatrix) {
	    matrix = (float *)fontMatrix;
	} else {
	    matrix = NXZoneMalloc(FontZone(), 6 * sizeof(float));
	    bcopy(fontMatrix, matrix, 6 * sizeof(float));
	}
	if (self = [self _commonFontInit]) {
	    [fontCltn insertObject:self at:0];
	} else {
	    return nil;
	}
	if (strncmp(fontName, SCREENPREFIX, strlen(SCREENPREFIX))) {
	    char                screenName[1024];
	    Font *other;

	    strcpy(screenName, SCREENPREFIX);
	    strcat(screenName, fontName);
	    otherFont = [Font newFont:screenName size:fontSize
			 style:fontStyle matrix:fontMatrix];
	    other = otherFont;
	    if (other) {
		other->otherFont = self;
		other->fFlags.isScreenFont = YES;
	    }
	} else if (self)
	    fFlags.isScreenFont = YES;
	return self;
    }
}


+ newFont:(const char *)fontName size:(float)fontSize
{
    return [self newFont:fontName size:fontSize style:0 matrix:NX_FLIPPEDMATRIX];
}


+ newFont:(const char *)fontName size:(float)fontSize matrix:(const float *)fontMatrix
{
    return [self newFont:fontName size:fontSize style:0 matrix:fontMatrix];
}


- awake
{
    [super awake];
    fFlags.usedByWS = NO;
    return self;
}


- _realFree
{
    if (matrix != identityMatrix && matrix != flippedMatrix) {
	free(matrix);
    }
    free(name);
    return [super free];
}

- free
{
    return self;
}


- (float)pointSize
{
    return size;
}


- (const char *)name
{
    return name;
}


- (int)fontNum
{
    return fontNum;
}

- (int)style
{
    return style;
}


- setStyle:(int)aStyle
{
    if (style != aStyle && [self class] == [Font class] &&
	[Font _findFont:name size:size style:style matrix:matrix] == self) {
	return [Font newFont:name size:size style:aStyle matrix:matrix];
    } else {
	style = aStyle;
	return self;
    }
}


- (const float *)matrix
{
    return matrix;
}


- (NXFontMetrics *)metrics
{
    return faceInfo->fontMetrics;
}


- (NXFontMetrics *)readMetrics:(int)flags
{
    int newFlags;

    newFlags = flags & ~faceInfo->flags;
    if (newFlags)
	if (![self _read:newFlags for:name size:size])
	    return NULL;
    return faceInfo->fontMetrics;
}


- (BOOL)hasMatrix
{
    return (!matricesEqual(identityMatrix, matrix));
}


- set
{
    BOOL                doStoreFont = NO;

    if (NXDrawingStatus != NX_DRAWING) {
	faceInfo->fontFlags.usedInPage = YES;
	faceInfo->fontFlags.usedInSheet = YES;
	faceInfo->fontFlags.usedInDoc = YES;
	if (!fFlags.usedByPrinter) {
	    doStoreFont = YES;
	    fFlags.usedByPrinter = YES;
	}
    } else if (!fFlags.usedByWS) {
	fFlags.usedByWS = YES;
	doStoreFont = YES;
    }
    if (doStoreFont) {
	if ([self hasMatrix])
	    _NXSelectFontNSM(faceInfo->fontMetrics->name, size, matrix);
	else
	    _NXSelectFontNS(faceInfo->fontMetrics->name, size);
	fontNum = DPSDefineUserObject(fontNum);
    }
    PSsetfont(fontNum);
    return self;
}


- (float)getWidthOf:(const char *)string
{
    register float      w, *wp;

    if (!faceInfo->fontMetrics->widths)
	[self readMetrics:NX_FONTWIDTHS];
    wp = faceInfo->fontMetrics->widths;
    w = 0.0;
    while (*string)
	w += wp[(unsigned char)*string++];
    if (!faceInfo->fontMetrics->isScreenFont)
	w *= size * matrix[0];
    return w;
}

#ifdef NEED_TO_THINK_ABOUT_THIS
- (int)countCharsOf:(const char *)string withinWidth:(float)maxWidth
{
    float	totalWidth, *wp;
    int		numChars;

    if (!faceInfo->fontMetrics->widths)
	[self readMetrics:NX_FONTWIDTHS];
    wp = faceInfo->fontMetrics->widths;
    totalWidth = 0.0;
    numChars = 0;
    if (!faceInfo->fontMetrics->isScreenFont)
	maxWidth = ABS(maxWidth / (size * matrix[0]));
    while (*string) {
	totalWidth += wp[(unsigned char)*string++];
	if (totalWidth <= maxWidth)
	    numChars++;
	else
	    break;
    }
    return numChars;
}
#endif

- screenFont
{
    return fFlags.isScreenFont ? self : otherFont;
}

typedef struct _funnyFace {
    struct _funnyFace *next;
    BOOL usedInPage;
    BOOL usedInSheet;
    BOOL usedInDoc;
    char name[256];
} funnyFace;

static funnyFace *fontsUsed = NULL;

+ useFont:(char*) fontName
{
    funnyFace *ff;
    
    ff = fontsUsed;
    while (ff) {
	if (!strcmp(ff->name,fontName))
	    goto fontFound;
        ff = ff->next;
    }
    ff = NXZoneMalloc(FontZone(), sizeof(funnyFace)-255+strlen(fontName));
    ff->next = fontsUsed;
    fontsUsed = ff;
    strcpy(ff->name,fontName);
fontFound:
    ff->usedInPage = YES;
    ff->usedInSheet = YES;
    ff->usedInDoc = YES;
    return self;
}

/* Private methods */

+ _findFont:(const char *)fontName size:(float)fontSize style:(int)fontStyle matrix:(const float *)fontMatrix
 /*
  * Attempts to locate a font object with the specified name, size, style,
  * and matrix in the fontCltn.  If found, it is returned.  Otherwise nil
  * is returned. 
  */
{
    register int        fontCount;
    register int        i;
    Font               *fontId;
    Font	      **fontIdPtr;

    fontIdPtr = FIRSTFONT;
    fontCount = (int)[fontCltn count];
    for (i = 0; i < fontCount; i++) {
	fontId = *fontIdPtr++;
	if (strcmp((fontId->name), fontName) == 0
	    && fontId->size == fontSize
	    && fontId->style == fontStyle
	    && matricesEqual(fontId->matrix, fontMatrix)
	    && ([fontId class] == self))
	    /* the above line replaced that below. wrp: new runtime conversion */
	    /*&& (strcmp(NAMEOF(fontId), NAMEOF(self) + 1) == 0))*/
	    return fontId;
    }
    return nil;
}


+ _clearDocFontsUsed
 /*
  * Clears all the document fontUsed flags for each font in the list. 
  */
{
    register NXFaceInfo *f;
    register int        fontCount;
    register int        i;
    Font        **fontIdPtr;
    register funnyFace *ff;

    for (ff = fontsUsed;ff != NULL; ff = ff->next)
	ff->usedInDoc = NO;
    for (f = faceList; f != NULL; f = f->nextFInfo)
	f->fontFlags.usedInDoc = NO;
    fontIdPtr = FIRSTFONT;
    fontCount = (int)[fontCltn count];
    for (i = 0; i < fontCount; i++, fontIdPtr++)
	(*fontIdPtr)->fFlags.usedByPrinter = NO;
    return self;
}


+ _clearSheetFontsUsed
 /*
  * Clears all the document fontUsed flags for each font in the list. 
  */
{
    register NXFaceInfo *f;
    register funnyFace *ff;

    for (ff= fontsUsed; ff != NULL; ff = ff->next)
	ff->usedInSheet = NO;
    for (f = faceList; f != NULL; f = f->nextFInfo)
	f->fontFlags.usedInSheet = NO;
    return self;
}


+ _clearPageFontsUsed
 /*
  * Clears all the page fontUsed flags for each font in the list.  Used to
  * know whether to generate user object definitions for a given font.  The
  * userInPage bit itself is no longer used, but hangs around for the sake of
  * compatibility.
  */
{
    register NXFaceInfo *f;
    register int        fontCount;
    register int        i;
    Font        **fontIdPtr;
    register funnyFace *ff;
    
    for (ff= fontsUsed; ff != NULL; ff = ff->next)
        ff->usedInPage = NO; 
    for (f = faceList; f != NULL; f = f->nextFInfo)
	f->fontFlags.usedInPage = NO;
    fontIdPtr = FIRSTFONT;
    fontCount = (int)[fontCltn count];
    for (i = 0; i < fontCount; i++, fontIdPtr++)
	(*fontIdPtr)->fFlags.usedByPrinter = NO;
    return self;
}

static int doAFont(int firstTime, char *name)
{
    if (firstTime) {
	firstTime = FALSE;
	DPSPrintf(DPSGetCurrentContext(), "%%%%DocumentFonts: ");
    } else
	DPSPrintf(DPSGetCurrentContext(), "%%%%+ ");
    DPSPrintf(DPSGetCurrentContext(), "%s\n", name);
    return firstTime;
}

+ _writeDocFontsUsed
 /*
  * Writes the fonts used for generating conforming PostScript for a whole
  * document. 
  */
{
    register NXFaceInfo *f;
    register int        firstTime = TRUE;
    register funnyFace *ff;
  
    for (ff = fontsUsed; ff != NULL; ff = ff->next) {
        if (ff->usedInDoc) {
	    for (f = faceList; f != NULL; f = f->nextFInfo) {
		if (!f->fontFlags.usedInDoc && 
		    !strcmp(ff->name, f->fontMetrics->name))
		    goto continueLoop;
	    }
	    firstTime = doAFont(firstTime, ff->name);
	}
    continueLoop:
        /* NULL */;
    }
    for (f = faceList; f != NULL; f = f->nextFInfo)
	if (f->fontFlags.usedInDoc)
	    firstTime = doAFont(firstTime, f->fontMetrics->name);
    return self;
}


+ _writePageFontsUsed
 /*
  * Writes the fonts used for generating conforming PostScript for a page
  * of a document. 
  */
{
    register NXFaceInfo *f;
    register int        firstTime = TRUE;
    register funnyFace *ff;
  
    for (ff = fontsUsed; ff != NULL; ff = ff->next) {
        if (ff->usedInSheet) {
	    for (f = faceList; f != NULL; f = f->nextFInfo) {
		if (!(f->fontFlags.usedInSheet) && 
		    !strcmp(ff->name,f->fontMetrics->name))
		    goto continueLoop;
	    }
	    firstTime = doAFont(firstTime, ff->name);
	}
    continueLoop:
        /* NULL */;
    }
    for (f = faceList; f != NULL; f = f->nextFInfo)
	if (f->fontFlags.usedInSheet)
	    firstTime = doAFont(firstTime, f->fontMetrics->name);
    return self;
}


- _commonFontInit
 /*
  * Called by both newFont:size:style:matrix and finishUnarchiving to do
  * some common initialization of the fontObject. 
  */
{
    register float     *widths;
    register NXFaceInfo *facePtr;
    char                mappedName[256];

 /* See if an appropriate faceInfo exists */
    [self _mapFont:name size:size style:style newName:mappedName];
    for (facePtr = faceList; facePtr; facePtr = facePtr->nextFInfo)
	if ((strcmp(mappedName, facePtr->fontMetrics->name) == 0)
	    && ((!facePtr->fontMetrics->isScreenFont) ||
		(facePtr->fontMetrics->screenFontSize == (short)size)))
	    break;
    if (!(faceInfo = facePtr)) {	/* Create appropriate faceInfo */
    /* Read in the global info and widths */
	faceInfo = facePtr = NXZoneCalloc(FontZone(), 1, sizeof(NXFaceInfo));
	if (![self _read:DFLTGETMASK for:mappedName size:size]) {
	    free(facePtr);
	    [self _realFree];
	    return nil;
	}
	widths = facePtr->fontMetrics->widths;
	facePtr->nextFInfo = faceList;
	faceList = facePtr;
    }
    return self;
}


- _read:(int)reqMask for:(const char *)fontName size:(float)thisSize
 /* Reads information from .afm files. */
{
#define MAX_BUF	1000
    char                paramBuffer[MAX_BUF];
    int			pathLength;
    int			userLength;
    int			paramLength;
    char	       *param;
    const char	       *user;
    char		fontSuffix[12] = {'\0'};
    int			ret;
    char	       *globalInfo, *widths, *fullData, *cc, *strings;
    unsigned int	globalInfoLength, widthsLength, fullDataLength, ccLength, stringsLength;
    int			widthsTag, fullDataTag, ccTag, stringsTag;
    port_t		server;
    NXFontMetrics      *metrics;
    kern_return_t	kr;
    int			akServerVersion;
    int			newMask;

    if (!(server = _NXLookupAppkitServer(&akServerVersion, NULL, NULL)))
	return nil;
    newMask = reqMask;
    user = NXUserName();
    if (strncmp(fontName, SCREENPREFIX, strlen(SCREENPREFIX)) == 0)
	sprintf(fontSuffix, ".%d", (int)size);
    pathLength = strlen(fontAFMPath);
    userLength = strlen(user);
    paramLength = pathLength + userLength + strlen(fontName) +
    						strlen(fontSuffix) + 3;
    if (paramLength > MAX_BUF)
	param = NXZoneMalloc(FontZone(), paramLength);
    else
	param = paramBuffer;
    strcpy(param, fontAFMPath);
    strcpy(param + pathLength + 1, user);
    strcpy(param + pathLength + userLength + 2, fontName);
    strcat(param + pathLength + userLength + 2, fontSuffix);
    MARKTIME2(_NXLaunchTiming, "[Font _read] %s", (int)fontName, 0, 2);
#if (_NX_BYTEORDER == _NX_BIGENDIAN)
    ret = _NXGetFullAFMInfoBigE(MAXINT, server, &newMask,
			param, paramLength,
			&globalInfo, &globalInfoLength,
			&widths, &widthsLength, &widthsTag,
			&fullData, &fullDataLength, &fullDataTag,
			&cc, &ccLength, &ccTag,
			&strings, &stringsLength, &stringsTag);
#else
    ret = _NXGetFullAFMInfoLittleE(MAXINT, server, &newMask,
			param, paramLength,
			&globalInfo, &globalInfoLength,
			&widths, &widthsLength, &widthsTag,
			&fullData, &fullDataLength, &fullDataTag,
			&cc, &ccLength, &ccTag,
			&strings, &stringsLength, &stringsTag);
#endif
    MARKTIME(_NXLaunchTiming, "[Font _read] afterglue", 2);
    if (paramLength > MAX_BUF)
	free(param);
    if (ret == AFM_OK) {
	faceInfo->flags |= newMask;
	metrics = NXZoneMalloc(FontZone(), sizeof(NXFontMetrics));
	faceInfo->fontMetrics = metrics;
	bcopy(globalInfo, metrics, sizeof(NXFontMetrics));
	if (widthsLength)
	    widths = receiveShmem(widthsTag, widths, widthsLength, WIDTHS_MAXSIZE);
	if (fullDataLength)
	    fullData = receiveShmem(fullDataTag, fullData, fullDataLength, FULLDATA_MAXSIZE);
	if (ccLength)
	    cc = receiveShmem(ccTag, cc, ccLength, CC_MAXSIZE);
	if (stringsLength)
	    strings = receiveShmem(stringsTag, strings, stringsLength, STRINGS_MAXSIZE);
	relocate(widths, &metrics->widths);
	metrics->widthsLength = 256 + sizeof(float);
	metrics->strings = strings;
	metrics->stringsLength = stringsLength;
	relocate(metrics->strings, &metrics->formatVersion);
	relocate(metrics->strings, &metrics->name);
	relocate(metrics->strings, &metrics->fullName);
	relocate(metrics->strings, &metrics->familyName);
	relocate(metrics->strings, &metrics->weight);
	relocate(metrics->strings, &metrics->version);
	relocate(metrics->strings, &metrics->notice);
	relocate(metrics->strings, &metrics->encodingScheme);
	relocate(fullData, &metrics->encoding);
	relocate(fullData, &metrics->yWidths);
	relocate(fullData, &metrics->charMetrics);
	relocate(fullData, &metrics->ligatures);
	relocate(fullData, &metrics->encLigatures);
	relocate(fullData, &metrics->kerns.kernPairs);
	relocate(fullData, &metrics->trackKerns);
	relocate(fullData, &metrics->compositeChars);
	relocate(fullData, &metrics->compositeCharParts);
	kr = vm_deallocate(task_self(), (vm_address_t)globalInfo, globalInfoLength);
	if (kr != KERN_SUCCESS)
	    NX_RAISE(NX_appkitVMError, (void *)kr, "vm_deallocate() in Font");
	return self;
    } else
	return nil;
}


static void relocate(char *base, void *field)
{
    int offset = *(int *)field;
    char **dest = field;

    *dest = (offset >= 0) ? base + offset : NULL;
}


- finishUnarchiving
{
    register id tempId;

    if (tempId = [Font _findFont:name size:size style:style matrix:matrix]) {
	[self _realFree];
	return tempId;
    } else {
	if ([self _commonFontInit]) {
	    [fontCltn insertObject:self at:0];
	    return nil;
	} else {
	  /* _commonFontInit frees the bad font object */
	    tempId = [Font newFont:NXSystemFont size:size style:style matrix:matrix];
	    return tempId;
	}
    }
}


- _mapFont:(const char *)fontName size:(float)fontSize style:(int)fontStyle newName:(char *)mappedName
 /*
  * Maps the specified font, size, and style into a postscript fontname.
  * CURRENTLY DOESN'T DO A THING. 
  */
{
    strcpy(mappedName, fontName);
    return self;
}


static BOOL
matricesEqual(const float *m, const float *n)
{
    int                 i;

    for (i = 0; i < 6; i++)
	if (*m++ != *n++)
	    return NO;
    return YES;
}

- write:(NXTypedStream *) s
{
    short               aShort = 0;

    [super write:s];
    fFlags._hasStyle = style ? 1 : 0;
    fFlags._matrixIsIdentity = (matrix == identityMatrix);
    fFlags._matrixIsFlipped = (matrix == flippedMatrix);
    NXWriteTypes(s, "*fss", &name, &size, &aShort, &fFlags);
    if (!fFlags._matrixIsIdentity && !fFlags._matrixIsFlipped) {
	NXWriteArray(s, "f", 6, matrix);
    }
    if (fFlags._hasStyle) {
	NXWriteTypes(s, "i", &style);
    }
    return self;
}

- read:(NXTypedStream *) s
{
    short               aShort = 0;
    float		stackMatrix[6];

    [super read:s];
    NXReadTypes(s, "*fss", &name, &size, &aShort, &fFlags);
    if (fFlags._matrixIsIdentity) {
	matrix = (float *)identityMatrix;
    } else if (fFlags._matrixIsFlipped) {
	matrix = (float *)flippedMatrix;
    } else {
	NXReadArray(s, "f", 6, stackMatrix);
	if (matricesEqual(stackMatrix, identityMatrix)) {
	    matrix = (float *)identityMatrix;
	} else if (matricesEqual(stackMatrix, flippedMatrix)) {
	    matrix = (float *)flippedMatrix;
	} else {
	    matrix = NXZoneMalloc(FontZone(), 6 * sizeof(float));
	    bcopy(stackMatrix, matrix, 6 * sizeof(float));
	}
    }
    if (fFlags._hasStyle) {
	NXReadTypes(s, "i", &style);
    } else {
	style = 0;
    }
    [Font newFont:name size:size style:style matrix:matrix];
    return self;
}

@end

typedef struct {
    int tag;
    void *data;
    int maxSize;
} Shmem;

static Shmem *ShmemList = NULL;
static int NumShmems = 0;

static void *receiveShmem(int tag, void *data, int length, int maxSize)
{
    Shmem *sm;
    int i;
    kern_return_t kret;

    for (sm = ShmemList, i = 0; i < NumShmems; i++, sm++)
	if (sm->tag == tag)
	    break;
    if (i == NumShmems) {	/* if coundnt find matching shmem */
	NumShmems++;
	ShmemList = NXZoneRealloc(FontZone(), ShmemList, NumShmems * sizeof(Shmem));
	sm = ShmemList + NumShmems - 1;
	sm->tag = tag;
	kret = vm_allocate(task_self(), (vm_address_t *)&sm->data, maxSize, TRUE);
	if (kret != KERN_SUCCESS)
	    /*error*/;
	sm->maxSize = maxSize;
    } else
	vm_protect(task_self(), (vm_address_t)sm->data, sm->maxSize, FALSE, VM_PROT_READ|VM_PROT_WRITE);
    vm_copy(task_self(), (vm_address_t)data, ((length + (vm_page_size - 1)) / vm_page_size) * vm_page_size, (vm_address_t)sm->data);
    vm_protect(task_self(), (vm_address_t)sm->data, sm->maxSize, FALSE, VM_PROT_READ);
    vm_deallocate(task_self(), (vm_address_t)data, length);
    return sm->data;
}


static NXZone *FontZone(void)
{
    NXZone *OurZone;

    OurZone = [NXApp zone];
    if (!OurZone)
	OurZone = NXDefaultMallocZone();
    return OurZone;
}


/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the new List object
01/12/89 trey	added _clearSheetFontsUsed method and the flag
		  NXFaceInfo.fontFlags.usedInSheet
 1/27/89 bs	added read: write:
 2/13/89 trey	added screenFont method to return otherFont
  
0.83
----
 3/19/89 pah	make style an int
  
0.91
----
 5/19/89 trey	minimized static data
		add launch profiling code
  
0.92
----
 6/20/89 trey	AFM data now returned by pbs server
		support for per character data, composite chars, and kerning
		 info has been dropped
		fontMetrics element of NXFaceInfo structure changed to be a
		 pointer
 7/05/89 trey	code removed which hard-wired widths for various non-breaking
		 spaces

0.96
----
 7/23/89 pah	added error checking for vm calls

0.97
----
 7/24/89 pah	make finishUnarchiving really free (i.e. don't call [self free]
		 which does nothing)

10/29/89 trey	support for little endian machines getting AFM data
		_NXLookupAppkitServer now takes an argument
11/07/89 trey	fixed bug preventing Fonts from being properly accumulated when
		writing out EPS files.

77
--
 2/28/90 king	fixed comment extension prelude.  was emitting %+ should
		have been %%+
		also cleaned up some vm_deallocate typecast headbutts.

79
--
 3/21/90 cmf	added useFont: and modified _write*FontsUsed & _clear*FontsUsed
		 to take advantage of useFont: info.  This code supports 
		 generating proper docfonts info for embedded postscript or 
		 other postscript using fonts without calling - set.

80
--
 4/08/90 trey	supports getting AFM info on per character info, kerning,
		 ligatures, and composite chars

86
--
 6/08/90 trey	finished receving of composite character data
		added +_freePBSData
 6/12/90 pah	Fixed _setStyle: and made it public
 
87
--
 7/12/90 glc	fixed leak. facePtr not freed on error.

89
--
 7/29/90 trey	reordered font path to put /NextLibrary first

95
--
 9/28/90 trey	when we unarchive an unknown font, substitute the system font

*/


/*
	textprivate.h
  	Copyright 1988, 1989, 1990 NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifndef TEXT_H
#import "Text.h"
#endif TEXT_H

#import "Window.h"
#import "Font.h"
#import <objc/hashtable.h>
#import "rtfstructs.h"

extern void _NXTextBackgroundRect(Text *self, NXRect *rectp, BOOL useCache, BOOL colorScreen);
extern NXColor _NXFromTextRGBColor(int color);
extern int _NXToTextRGBColor(NXColor color);

@interface Text(Event)
- _pastefrompboard:pboard;
- _copytopboard:pboard writeRTF:(BOOL)writeRTF;
@end

@interface Text(Private)
- _initFrame:(const NXRect *)frameRect text:(const char *)theText alignment:(int)mode services:(BOOL)flag;
- _bringSelToScreen:(NXCoord) pad;
-  (int)_readText:(NXStream *)stream;
- (BOOL) _canDraw;
- _fixFontPanel;
- _convertFont:(NXRun *)run with:(void *)data;
- _convertRun:(NXRun *)run selector:(SEL)aSel with:(void *)data;
- _changeTypingRun:(SEL)aSel with:(void *)data;
@end

 /*
  * various font extraction macros, make them as fast or as
  * object-oriented as you want 
  */

typedef struct {
	@defs (Font)
} _IDFont ;

#define FONTTOMETRICS(fid) (((_IDFont *) fid)->faceInfo->fontMetrics)
#define FONTTOWIDTHS(fid) (((_IDFont *) fid)->faceInfo->fontMetrics->widths)
#define FONTTOSIZE(fid) (((_IDFont *) fid)->size)
#define FONTTOSTYLE(fid) (((_IDFont *) fid)->style)
#define FONTTONAME(fid) (((_IDFont *) fid)->name)
#define FONTTOMATRIX(fid) (((_IDFont *) fid)->matrix)
#define FONTTOSCREEN(fid) (((_IDFont *) fid)->otherFont)
#define ISSCREENFONT(fid) (((_IDFont *) fid)->fFlags.isScreenFont)

typedef struct {
	@defs (Window)
} WindowClass;

#define VALID_WINDOW(w) (w && (((WindowClass *) (w))->windowNum > 0))
#define FLUSHWINDOW(w) (VALID_WINDOW(w) && ((WindowClass *) w)->_flushDisabled <= 0 )

#define TXYESCHANGE(LD) (LD | (int)0x8000)
#define TXNOCHANGE(LD) (LD & (int)0x7FFF)
#define TXISCHANGE(LD) (LD < 0)
#define TXMCHANGE 0x8000

#define TXYESPARA(LD) (LD | (int)0x4000)
#define TXNOPARA(LD) (LD & (int)0xBFFF)
#define TXISPARA(LD) (LD & (int)0x4000)
#define TXMPARA ((int)0x4000)

#define TXGETCHARS(LD) (LD & (int)0x3FFF)
#define TXSETCHARS(LD, NC) ((LD & (int)0xC000) | NC)
#define TXMCHARS (int)0x3FFF
#define TX_LINEAT(BASE, OFFSET) ((NXLineDesc *) ((char *) (BASE) + (OFFSET)))
#define TX_HTCHANGE(BASE, OFFSET) ((NXHeightChange *)((char *)(BASE) + (OFFSET)))
#define TX_HTINFO(BASE, OFFSET) ((NXHeightInfo *)((char *)(BASE) + (OFFSET)))
#define TX_LAYAT(BASE, OFFSET) ((NXLay *) ((char *)(BASE) + (OFFSET)))
#define TX_RUNAT(BASE, OFFSET) ((NXRun *) ((char *)(BASE) + (OFFSET)))
#define TX_NEXTSTATE(BASE, OFFSET) ((NXFSM *) ((char *)(BASE) + (OFFSET)))
#define TX_BYTES(PTR1, PTR2) ((char *)(PTR1) - (char *)(PTR2))
#define HTCHANGE(ps) ((NXHeightChange *)(ps))
/* targetMethods supported */
#define TXWILLGROW 		0
#define TXGROWACTION 		1
#define TXWILLCHANGE 		2
#define TXCHANGEACTION 		3
#define TXWILLEND 		4
#define TXENDACTION 		5
#define TXISEMPTY 		6
#define TXDIDGETKEYS		7

#define TXMWILLGROW		(1 << TXWILLGROW)
#define TXMGROWACTION		(1 << TXGROWACTION)
#define TXMWILLCHANGE		(1 << TXWILLCHANGE)
#define TXMCHANGEACTION		(1 << TXCHANGEACTION)
#define TXMWILLEND		(1 << TXWILLEND)
#define TXMENDACTION		(1 << TXENDACTION)
#define TXMISEMPTY		(1 << TXISEMPTY)
#define TXMDIDGETKEYS		(1 << TXDIDGETKEYS)


/* caret blinking states */
#define CARETDISABLED 0
#define CARETON 1
#define CARETOFF 2

/* left and right fudge factor */

#define HFUDGE 2.0

#define NX_MAXCPOS ((int)0x7FFFFFFF)
#define NX_YHYSTERESIS 3.0
#define NX_XHYSTERESIS 1.0



 /*
  * pingrate defines the number of lines per ping 
  */
#define PINGRATE 1

 /**********************************************************/


#define MOVECHAR(c) ((c) == '\t' || ((c) == NX_FIGSPACE))

#define GROWARRAYBY 100
#define INITARRAY 100
#define GROWLINESBY 10
#define SINotTracking 0
#define SIMove 1
#define VISBIGFUDGE 30.0
#define VISLITTLEFUDGE 5.0
#define READONLY 1
#define READWRITE 2
#define SELECTABLE 1
#define NOTSELECTABLE 2
#define GROWABLE 1
#define NOTGROWABLE 2
#define UPARROW 173
#define DOWNARROW 175
#define LEFTARROW 172
#define RIGHTARROW 174
#define SYMBOLFONT 1
#define NPBTYPES	3
#define NX_ENTER 3

#define ISNULLSEL ((sp0.cp >= 0) && (sp0.cp == spN.cp) && (sp0.line == spN.line) && (sp0.x == spN.x))

#define PROLOG(info) if (!(((NXTextInfo *)(info))->globals.refCount++)) \
    [Text initTextInfo:info]
#define EPILOG(info) if (!(--(((NXTextInfo *)(info))->globals.refCount))) \
    [Text finishTextInfo:info]

#define EQUALRUN(r1, r2) (((r1)->font == (r2)->font) && \
	((r1)->paraStyle == (r2)->paraStyle) && \
	((r1)->superscript == (r2)->superscript) && \
	((r1)->subscript == (r2)->subscript) && \
	((r1)->textRGBColor == (r2)->textRGBColor) && \
	((r1)->textGray == (r2)->textGray) && \
	((r1)->info == (r2)->info) && \
	(!memcmp(&(r1)->rFlags, &(r2)->rFlags, sizeof(NXRunFlags))))
	
#define HAVE_RULER(info) (((NXTextInfo *)info)->globals.ruler)
typedef struct {
    id font;
    NXCoord Gray;
    int	Color;
} NXDrawCache;

typedef struct {
    int		    runs;
    int             chars;
    int             words;
} NXRunPboardHeader;

typedef struct {
    int             chars;
    NXCoord         size;
    short	    fontStyle;
    short           namelen;
}               NXRunPboardFixed;

typedef struct {
    NXLayArray *lays;
    NXWidthArray *widths;
    NXCharArray *chars;
    NXBreakArray *savedBreaks;
    NXWidthArray *xPos;
} NXGlobalArrays;

typedef struct {
    id cell;
    NXPoint origin;
} NXTextCellInfo;

@interface NXUniqueTabs:Object
{
    NXHashTable *tabHash;
}
- free;
- (NXTabStop *)getTabs:(NXTabStop *)tabs num:(int)numTabs;
@end

/*
 * State info which must be unique to each text object during RTF IO.
 */
typedef struct {
    NXProps      	*gProps;
    NXRunArray  	*globalRuns;
    NXCharArray   	*textBuf;
    NXRun       	*curRun,*lRun;
    unsigned char 	*curText,*lText;
    NXProps      	lastProps;	/* property cache */
/*
 * The Font lookup code is slow, so we keep around the last 2 returns, 
 * since alot of the same fonts are used in a row.  32 bytes of storage.
 */
    NXFontTraitMask 	face1,face2;
    int			font1,font2;
    int			size1,size2;
    id			ret1,ret2;
} NXRtfIO;

typedef struct {
    NXRect hitRect;
    NXBreakArray *savedBreaks;
    NXDrawCache drawCache;
    NXGlobalArrays *arrays;
    NXHashTable *rtfStyles;
    NXTabStop *tabs;
    NXTabStop *tabsEnd;
    NXTextStyle *defaultStyle;
    NXTextStyle *uniqueStyle;
    NXUniqueTabs *tabTable;
    NXWidthArray *xPos;
    NXCoord caretXPos;
    NXCoord lastCharWidth;
    NXCoord minBodyHeight;
    NXCoord minBodyWidth;
    id ruler;
    int inReplaceSel;
    int refCount;
    int tabLay;
    int tabsPending;
    int	textRGBColor;  		/* negative if not used. 10 bits of RGB */
    int backgroundRGBColor;  	/* negative if not used. 10 bits of RGB */
    NXHashTable *cellInfo;
    unsigned short didEndChar;
    NXRtfIO	rtf;
    struct {
	unsigned int clipLeft:1;
	unsigned int clipRight:1;
	unsigned int vertOrHoriz:1;
	unsigned int plusMinus:1;
	unsigned int willConvertFont:1;
	unsigned int willSetSel:1;
	unsigned int willWritePaperSize:1;
	unsigned int didReadPaperSize:1;
	unsigned int willWriteRichText:1;
	unsigned int willReadRichText:1;
	unsigned int willStartRichText:1;
	unsigned int willFinishRichText:1;
	unsigned int changingFont:1;
	unsigned int movingCaret:1;
	unsigned int hasTabs:1;
	unsigned int inDrawSelf:1;
	unsigned int canUseXYShow:1;
	unsigned int xyshowOk:1;
	unsigned int retainedWhileDrawing:1;
	unsigned int reserved:13;
    } gFlags;
} NXTextGlobals;

typedef struct {
    NXLayInfo layInfo;
    NXTextGlobals globals;
} NXTextInfo;


typedef struct {
    id sender;
    void *paraStyle;
} SenderAndStyle;

typedef struct {
    id font;
    void *paraStyle;
} FontAndStyle;

typedef enum {
    archive_io,
    pasteboard_io,
    ruler_io,
    font_io
} RichTextOp;

#define CACHE(info) (&(((NXTextInfo *)(info))->layInfo.cache))
#define DRAWCACHE(info) (&(((NXTextInfo *)(info))->globals.drawCache))
#define TEXTGLOBALS(info) (&(((NXTextInfo *)(info))->globals))

#define CANUSEXYSHOW(info) ((((NXTextInfo *)(info))->globals.gFlags.canUseXYShow) && \
			    (((NXTextInfo *)(info))->globals.gFlags.xyshowOk))
 
extern NXTextStyle *_NXMakeDefaultStyle(
    NXZone *zone, int newJust, NXCoord newHeight, NXCoord newDescent, 
    id nFont, NXTextStyle * ps);
extern void _NXQuickCaret(id self);
extern void _NXBlinkCaret(
    DPSTimedEntry te, double time, id _whoToBlink, int quick);
extern void _NXBlinkCaretTE(DPSTimedEntry te, double time, void *whoToBlink);
extern int _NXPosNewline(
    id self, NXTextInfo *info, NXSelPt *sp, NXSelPt *synch);
extern void _NXClearClickState(id self);
extern NXCoord _NXSetDrawBounds(id self, NXCoord *pTop);
extern NXRunArray *_NXSetDefaultRun(
    id self, int iLen, NXRunArray *dfltRun);
extern void _NXMinSize(id self, NXSize *theSize);
extern NXStream *_NXOpenTextStream(id self);
extern void _NXResetDrawCache(id self);
extern void _NXRestoreDrawCache(id self);

@interface Text(Replace)

- (int)_replaceSel:pboardRep :(const char *)insertText :(int)iLen :(NXRunArray *)insertRun :(BOOL)abortable :(BOOL)smartCut :(BOOL)smartPaste;
- _replaceSel:(const char *)aString length:(int)length smartPaste:(BOOL)flag;
- _replaceAll:(const char *)insertText :(int)iLen :(NXRunArray *)insertRun;


/* static methods */

@end

@interface Text(ReplaceUtil)
- (int) doAbortion:(int *)newSelN info:(NXTextInfo *)info
    runs:(NXRunArray *)insertRun;
- (int) replaceRangeAt:(int)cp1st end:(int) cpN
    info: (NXTextInfo *) info
    text:(const char *)insertText length:(int *) insertLength
    runs:(NXRunArray *)insertRun smartPaste:(int)smartPaste
    error:(int *) errorNumber;
- reSel:(int)sel line:(int)line info:(NXTextInfo *)info pos:(int)cp
    height:(NXCoord)ht yPos:(NXCoord)yp selection:(NXSelPt *)sp;
- (BOOL) wordPunct:(NXTextInfo *)info pos:(int)cp
    table:(const unsigned char *)punctTable;
- freeCellsFrom:(NXRun *)start to:(NXRun *)last;
- pasteRunAt:(int)cp1st end:(int) cpN info:(NXTextInfo *)info 
    length:(int) iLen runs:(NXRunArray *)insertRun
    left:(int)leftBlank right:(int)rightBlank;
- (NXTextBlock *) mergePrior:(NXTextBlock *)block;
- oncePerGrow:(NXRect *)boundsSave maxBounds:(NXRect *)boundsMax;
- (BOOL) perLineGrow:(NXTextInfo *)info maxBounds:(NXRect *)boundsMax;
- smartSelect:(NXTextInfo *)info;
@end

@interface Text(Stream)
- invalidateStream;
- validateStream;

@end

@interface Text(Edit)
- replaceRangeFrom:(int)cp0 to:(int)cpN selector:(SEL)aSel with:(void *)data;
- replaceRunFrom:(int)cp0 to:(int)cpN selector:(SEL)aSel with:(void *)data;

@end

@interface Text (SelectUtil)
- (int)posToLine:(int) cp sel:(int)sel line:(int)line info:(NXTextInfo *)info
    height:(NXCoord)ht yPos:(NXCoord)yp selection:(NXSelPt *)sp 
    left:(BOOL)left;
- posToPoint:(int)cp sel:(int)sel line:(int)line info:(NXTextInfo *)info
    height:(NXCoord)ht yPos:(NXCoord)yp selection:(NXSelPt *)sp
    left:(BOOL)left;
-  _mouseTrack:(NXPoint *) thePoint;
- _ptToHit: (NXPoint *)thePoint: (NXSelPt *) _downP0: (NXSelPt *) _downPN;
- _hilite: (int)extendCR:(NXSelPt *) _p0 : (NXSelPt *) _pN :(const NXRect *)clipLineRect;
- _setSelPrologue: (BOOL) isCaret;
- _setSelPostlogue;
- _setSel: (NXSelPt *)sel;
- _setSel:(int)start :(int)end;
- selectParagraph;
- updateRuler;
- findEndParagraph:(NXSelPt *)sel;
- findStartParagraph:(NXSelPt *)sel;

@end

@interface Text (RTF)
- readRTF:(NXStream *)stream op:(RichTextOp)operation;
- writeRTFAt:(int)startPos end:(int)endPos into:(NXStream *)stream
    op:(RichTextOp)operation;
- (NXTabStop *)getTabs:(NXTabStop *)tabs num:(int)numTabs;
- (NXTextStyle *)getUniqueStyle:(NXTextStyle *)style;
- removeUniqueStyle:(NXTextStyle *)style;

@end

@interface Text (Util)
- freeBList:(NXTextBlock *)pb;
- trimRuns;
- copyText:(const char *)newText;
- (int)delegateFeatures;
+ initTextInfo:(NXTextInfo *) info;
+ finishTextInfo:(NXTextInfo *) info;
- _setFont:fontObj paraStyle:(void *)paraStyle;

@end

@interface Text (Tabs)
+ sortTabs:(NXTabStop *)tabs num:(int)numTabs;
@end


/*

Modifications (starting at 0.8):

01/26/89 trey	nuked #define DYNAMIC, since the misfeature it was
		 associated with was removed.

84
--
05/08/90 chris	added _selectPrologue,_selectPostlogue,_select:
05/09/90 chris  added NX_ENTER constant
05/11/90 chris  removed parameter to _fixFontPanel: and remove use of bucky
		bits to suppress setting fontPanel. as per discussion with
		bryan, Copy Font removes necessity for this "feature" which
		interfers with mixing clicking and command key setting of
		bold, etc.
		
85
--
05/17/90 chris	removed readChars parameter from _readText:: as it is only
		called in one place now (archiving uses readRichText), and
		-1 is passed as the argument.
06/04/90 gcockrft new textReplace.m method _replaceAll:::

87
--
7/12/90	glc	changes to support color text.

99
--
10/17/90 glc    Changes to support reentrant RTF

*/

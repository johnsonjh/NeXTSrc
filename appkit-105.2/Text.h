/*
	Text.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#ifndef TEXT_H
#define TEXT_H
#import "View.h"
#import "chunk.h"
#import "color.h"
#import "FontManager.h"

#define NX_TEXTPER 490

typedef struct _NXTextBlock {
    struct _NXTextBlock *next;		/* next text block in link list */
    struct _NXTextBlock *prior;		/* previous text block in link list */
    struct _tbFlags {
	unsigned int    malloced:1;	/* true if block was malloced */
	unsigned int    PAD:15;
    } tbFlags;
    short           chars;		/* number of chars in this block */
    unsigned char  *text;		/* the text */
} NXTextBlock;

/*
 *  NXRun represents a single run of text w/ a given format
 */
  
typedef struct {
    unsigned int underline:1;
    unsigned int dummy:1;		/* unused */
    unsigned int subclassWantsRTF:1;
    unsigned int graphic:1;
    unsigned int RESERVED:12;
} NXRunFlags;

typedef struct _NXRun {
    id              font;	/* Font id */
    int             chars;	/* number of chars in run */
    void           *paraStyle;	/* implementation dependent paraStyle
				 * sheet info. */
    float	    textGray;	/* text gray of current run */
    int		    textRGBColor;	/* text color negative if not set */
    unsigned char   superscript;/* superscript in points */
    unsigned char   subscript;	/* subscript in points */
    id  	    info;	/* available for subclasses of Text */
    NXRunFlags rFlags;
} NXRun;

/*
 *  NXRunArray is a NXChunk that holds the set of formats for a Text object
 */

typedef struct _NXRunArray {
    NXChunk         chunk;
    NXRun           runs[1];
} NXRunArray;

/*
 * NXBreakArray is a NXChunk that holds line break information for a Text
 * Object. it is mostly an array of line descriptors.  each line
 * descriptor contains 3 fields: 
 *
 * 	1) line change bit (sign bit), set if this line defines a new height 
 * 	2) paragraph end bit (next to sign bit), set if the end of this 
 *	   line ends the paragraph 
 * 	3) numbers of characters in the line (low order 14 bits) 
 *
 * if the line change bit is set, the descriptor is the first field of a
 * NXHeightChange. since this record is bracketed by negative short
 * values, the breaks array can be sequentially accessed backwards and
 * forwards. 
 */

typedef short NXLineDesc;

typedef struct _NXHeightInfo {
    NXCoord         newHeight;	/* line height from here forward */
    NXCoord         oldHeight;	/* height before change */
    NXLineDesc      lineDesc;	/* line descriptor */
} NXHeightInfo;

typedef struct _NXHeightChange {
    NXLineDesc      lineDesc;	/* line descriptor */
    NXHeightInfo    heightInfo;
} NXHeightChange;

typedef struct _NXBreakArray {
    NXChunk         chunk;
    NXLineDesc      breaks[1];
} NXBreakArray;

/*
 * NXLay represents a single run of text in a line and records
 * everything needed to select or draw that piece.
 */
 
typedef struct {
    unsigned int mustMove:1;	/* unimplemented */
    unsigned int isMoveChar:1;
    unsigned int RESERVED:14;
} NXLayFlags;

typedef struct _NXLay {
    NXCoord         x;		/* x coordinate of moveto */
    NXCoord         y;		/* y coordinate of moveto */
    short           offset;	/* offset in line array for text */
    short           chars;	/* number of characters in lay */
    id              font;	/* font id */
    void           *paraStyle;	/* implementation dependent fontStyle
				 * sheet info. */
    NXRun *run;			/* run for lay */
    NXLayFlags	    lFlags;
} NXLay;

/*
 *  NXLayArray is a NXChunk that holds the layout for the current line
 */

typedef struct _NXLayArray {
    NXChunk         chunk;
    NXLay           lays[1];
} NXLayArray;

/*
 *  NXWidthArray is a NXChunk that holds the widths for the current line
 */

typedef struct _NXWidthArray {
    NXChunk         chunk;
    NXCoord         widths[1];
} NXWidthArray;

/*
 *  NXCharArray is a NXChunk that holds the chars for the current line
 */

typedef struct _NXCharArray {
    NXChunk         chunk;
    unsigned char   text[1];
} NXCharArray;

/*
 *  Word definition Finite State Machine transition struct
 */
typedef struct _NXFSM {
    const struct _NXFSM  *next;	/* state to go to, NULL implies final state */
    short           delta;	/* if final state, this undoes lookahead */
    short           token;	/* if final state, < 0 word is newline,
				 * = is dark, > is white space */
} NXFSM;

/*
 *  Represents one end of a selection
 */

typedef struct _NXSelPt {
    int             cp;		/* character position */
    int             line;	/* offset of LineDesc in break table */
    NXCoord         x;		/* x coordinate */
    NXCoord         y;		/* y coordinate */
    int             c1st;	/* character position of first character
				 * on the line */
    NXCoord         ht;		/* line height */
} NXSelPt;

/*
 *  describes tabstop
 */

typedef struct _NXTabStop {
    short           kind;	/* only NX_LEFTTAB implemented*/
    NXCoord         x;		/* x coordinate for stop */
} NXTabStop;

typedef struct _NXTextCache {
    int curPos;			/* current position in text stream */
    NXRun *curRun;		/* cache current block of text and */
    int runFirstPos;		/* character pos that corresponds */
    NXTextBlock *curBlock;	/* cache current block of text and */
    int blockFirstPos;		/* character pos that corresponds */
} NXTextCache;

typedef struct _NXLayInfo {
    NXRect rect;		/* bounds rect for current line. */
    NXCoord descent;		/* descent for current line, can be reset
				 * by scanFunc */
    NXCoord width;		/* width of line */
    NXCoord left;		/* left side visible coordinate */
    NXCoord right;		/* right side visible coordinate */
    NXCoord rightIndent;	/* how much white space is left at right
				 * side of line */
    NXLayArray *lays;		/* scanFunc fills with NXLay items */
    NXWidthArray *widths;	/* scanFunc fills with character widths */
    NXCharArray *chars;		/* scanFunc fills with characters */
    NXTextCache cache;		/* cache of current block & run */
    NXRect *textClipRect;	/* if non-nil, the current clip for drawing */
    struct _lFlags {
	unsigned int horizCanGrow:1;/* 1 if scanFunc should perform dynamic
				 * growing of x margins */
	unsigned int vertCanGrow:1;/* 1 if scanFunc should perform dynamic
				 * growing of y margins */
	unsigned int erase:1;	/* used to tell drawFunc to erase before
				 * drawing line  */
	unsigned int ping:1;	/* used to tell drawFunc to ping server */
	unsigned int endsParagraph:1;/* true if line ends the paragraph, eg
				 * ends in newline */
	unsigned int resetCache:1;/* used in scanFunc to reset local caches */
	unsigned int RESERVED:10;
    } lFlags;
} NXLayInfo;

/*
 *  Gives a paragraph fontStyle
 */

typedef struct _NXTextStyle {
    NXCoord         indent1st;	/* how far first line in paragraph is
				 * indented */
    NXCoord         indent2nd;	/* how far second line is indented */
    NXCoord         lineHt;	/* line height */
    NXCoord         descentLine;/* distance to ascent line from bottom of
				 * line */
    short           alignment;	/* justification */
    short           numTabs;	/* number of tab stops */
    NXTabStop      *tabs;	/* array of tab stops */
} NXTextStyle;

/* justification modes */

#define NX_LEFTALIGNED 0
#define NX_RIGHTALIGNED 1
#define NX_CENTERED 2
#define NX_JUSTIFIED 3

/* tab stop fontStyles */

#define NX_LEFTTAB 0

#define NX_BACKSPACE  8
#define NX_CR 13
#define NX_DELETE ((unsigned short)0x7F)
#define NX_BTAB 25

/* movement codes for movement between fields */

#define NX_ILLEGAL 	0
#define NX_RETURN 	((unsigned short)0x10)
#define NX_TAB 		((unsigned short)0x11)
#define NX_BACKTAB 	((unsigned short)0x12)
#define NX_LEFT 	((unsigned short)0x13)
#define NX_RIGHT 	((unsigned short)0x14)
#define NX_UP 		((unsigned short)0x15)
#define NX_DOWN 	((unsigned short)0x16)

typedef enum {
    NX_LEFTALIGN = NX_LEFTALIGNED,
    NX_RIGHTALIGN = NX_RIGHTALIGNED,
    NX_CENTERALIGN = NX_CENTERED,
    NX_JUSTALIGN = NX_JUSTIFIED,
    NX_FIRSTINDENT,
    NX_INDENT,
    NX_ADDTAB,
    NX_REMOVETAB,
    NX_LEFTMARGIN,
    NX_RIGHTMARGIN
} NXParagraphProp;

/* 
    Word tables for various languages.  The SmartLeft and SmartRight arrays
    are suitable as arguments for the messages setPreSelSmartTable: and
    setPostSelSmartTable.  When doing a paste, if the character to the left
    (right) of the new word is not in the left (right) table, an extra space
    is added on that side.  The CharCats tables define the character classes
    used in the word wrap or click tables.  The BreakTables are finite state
    machines that determine word wrapping.  The ClickTables are finite state
    machines that determine which characters are selected when the user
    double clicks.
*/

extern const unsigned char * const NXEnglishSmartLeftChars;
extern const unsigned char * const NXEnglishSmartRightChars;
extern const unsigned char * const NXEnglishCharCatTable;
extern const NXFSM * const NXEnglishBreakTable;
extern const int NXEnglishBreakTableSize;
extern const NXFSM * const NXEnglishNoBreakTable;
extern const int NXEnglishNoBreakTableSize;
extern const NXFSM * const NXEnglishClickTable;
extern const int NXEnglishClickTableSize;

extern const unsigned char * const NXCSmartLeftChars;
extern const unsigned char * const NXCSmartRightChars;
extern const unsigned char * const NXCCharCatTable;
extern const NXFSM * const NXCBreakTable;
extern const int NXCBreakTableSize;
extern const NXFSM * const NXCClickTable;
extern const int NXCClickTableSize;

typedef int (*NXTextFunc) (id self, NXLayInfo *layInfo);
typedef unsigned short (*NXCharFilterFunc) (
    unsigned short charCode, int flags, unsigned short charSet);
typedef char  *(*NXTextFilterFunc) (
    id self, unsigned char * insertText, int *insertLength, int position);

@interface Text : View
{
    const NXFSM        *breakTable;
    const NXFSM        *clickTable;
    const unsigned char *preSelSmartTable;
    const unsigned char *postSelSmartTable;
    const unsigned char *charCategoryTable;
    char                delegateMethods;
    NXCharFilterFunc    charFilterFunc;
    NXTextFilterFunc    textFilterFunc;
    char               *_compilerErrorSpacer;
    NXTextFunc          scanFunc;
    NXTextFunc          drawFunc;
    id                  delegate;
    int                 tag;
    DPSTimedEntry       cursorTE;
    NXTextBlock        *firstTextBlock;
    NXTextBlock        *lastTextBlock;
    NXRunArray         *theRuns;
    NXRun               typingRun;
    NXBreakArray       *theBreaks;
    int                 growLine;
    int                 textLength;
    NXCoord             maxY;
    NXCoord             maxX;
    NXRect              bodyRect;
    NXCoord             borderWidth;
    char                clickCount;
    NXSelPt             sp0;
    NXSelPt             spN;
    NXSelPt             anchorL;
    NXSelPt             anchorR;
    float               backgroundGray;
    float               textGray;
    float               selectionGray;
    NXSize              maxSize;
    NXSize              minSize;
    struct _tFlags {
	unsigned int        _editMode:2;
	unsigned int        _selectMode:2;
	unsigned int        _caretState:2;
	unsigned int        changeState:1;
	unsigned int        charWrap:1;
	unsigned int        haveDown:1;
	unsigned int        anchorIs0:1;
	unsigned int        horizResizable:1;
	unsigned int        vertResizable:1;
	unsigned int        overstrikeDiacriticals:1;
	unsigned int        monoFont:1;
	unsigned int        disableFontPanel:1;
	unsigned int        inClipView:1;
    }                   tFlags;
    void               *_info;
    NXStream           *textStream;
    unsigned int        _reservedText1;
    unsigned int        _reservedText2;
}

+ initialize;
+ excludeFromServicesMenu:(BOOL)flag;
+ getDefaultFont;
+ setDefaultFont:anObject;

- initFrame:(const NXRect *)frameRect text:(const char *)theText alignment:(int)mode;
- initFrame:(const NXRect *)frameRect;

- free;
- renewRuns:(NXRunArray *)newRuns text:(const char *)newText frame:(const NXRect *)newFrame tag:(int)newTag;
- renewFont:newFontId text:(const char *)newText frame:(const NXRect *)newFrame tag:(int)newTag;
- renewFont:(const char *)newFontName size:(float)newFontSize style:(int)newFontStyle text:(const char *)newText frame:(const NXRect *)newFrame tag:(int)newTag;
- setEditable:(BOOL)flag;
- (BOOL)isEditable;
- adjustPageHeightNew:(float *)newBottom top:(float)oldTop bottom:(float)oldBottom limit:(float)bottomLimit;
- getParagraph:(int)prNumber start:(int *)startPos end:(int *)endPos rect:(NXRect *)paragraphRect;
- (int)textLength;
- (int)byteLength;
- (int)getSubstring:(char *)buf start:(int)startPos length:(int)numChars;
- (NXTextBlock *)firstTextBlock;
- setText:(const char *)aString;
- readText:(NXStream *)stream;
- readRichText:(NXStream *)stream;
- writeText:(NXStream *)stream;
- writeRichText:(NXStream *)stream;
- writeRichText:(NXStream *)stream from:(int)start to:(int)end;
- setOverstrikeDiacriticals:(BOOL)flag;
- (int)overstrikeDiacriticals;
- setScanFunc:(NXTextFunc)aFunc;
- (NXTextFunc)scanFunc;
- setDrawFunc:(NXTextFunc)aFunc;
- (NXTextFunc)drawFunc;
- setCharFilter:(NXCharFilterFunc)aFunc;
- (NXCharFilterFunc)charFilter;
- setTextFilter:(NXTextFilterFunc)aFunc;
- (NXTextFilterFunc)textFilter;
- (const unsigned char *)preSelSmartTable;
- setPreSelSmartTable:(const unsigned char *)aTable;
- (const unsigned char *)postSelSmartTable;
- setPostSelSmartTable:(const unsigned char *)aTable;
- (const unsigned char *)charCategoryTable;
- setCharCategoryTable:(const unsigned char *)aTable;
- (const NXFSM *)breakTable;
- setBreakTable:(const NXFSM *)aTable;
- (const NXFSM *)clickTable;
- setClickTable:(const NXFSM *)aTable;
- setTag:(int)anInt;
- (int)tag;
- setDelegate:anObject;
- delegate;
- setBackgroundGray:(float)value;
- (float)backgroundGray;
- setBackgroundColor:(NXColor)color;
- (NXColor)backgroundColor;
- setTextGray:(float)value;
- setTextColor:(NXColor)color;
- (float)textGray;
- (NXColor)textColor;
- (float)selGray;
- windowChanged:newWindow;
- (NXStream *)stream;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;
- writeRichText:(NXStream *)stream forRun:(NXRun *)run atPosition:(int)runPosition emitDefaultRichText:(BOOL *)writeDefaultRTF;
- readRichText:(NXStream *)stream atPosition:(int)position;
- finishReadingRichText;
- startReadingRichText;
- setRetainedWhileDrawing:(BOOL)aFlag;
- (BOOL) isRetainedWhileDrawing;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect text:(const char *)theText alignment:(int)mode;
+ new;
+ newFrame:(const NXRect *)frameRect;

@end

@interface Text(FrameRect)
- getMaxSize:(NXSize *)theSize;
- getMinSize:(NXSize *)theSize;
- (BOOL)isHorizResizable;
- (BOOL)isVertResizable;
- moveTo:(NXCoord)x :(NXCoord)y;
- resizeText:(const NXRect *)oldBounds :(const NXRect *)maxRect;
- setMaxSize:(const NXSize *)newMaxSize;
- setMinSize:(const NXSize *)newMinSize;
- setHorizResizable:(BOOL)flag;
- setVertResizable:(BOOL)flag;
- sizeTo:(NXCoord)width :(NXCoord)height;
- sizeToFit;
@end

@interface Text(Layout)
- (int)alignment;
- (int)calcLine;
- (void *)calcParagraphStyle:fontId:(int)alignment;
- (BOOL)charWrap;
- (NXCoord)descentLine;
- getMarginLeft:(NXCoord *)leftMargin right:(NXCoord *)rightMargin top:(NXCoord *)topMargin bottom:(NXCoord *)bottomMargin;
- getMinWidth:(NXCoord *)width minHeight:(NXCoord *)height maxWidth:(NXCoord)widthMax maxHeight:(NXCoord)heightMax;
- (void *)defaultParaStyle;
- (NXCoord)lineHeight;
- setAlignment:(int)mode;
- setCharWrap:(BOOL)flag;
- setDescentLine:(NXCoord)value;
- setLineHeight:(NXCoord)value;
- setMarginLeft:(NXCoord)leftMargin right:(NXCoord)rightMargin top:(NXCoord)topMargin bottom:(NXCoord)bottomMargin;
- setNoWrap;
- setParaStyle:(void *)paraStyle;
@end

@interface Text(LinePosition)
- (int)lineFromPosition:(int)position;
- (int)positionFromLine:(int)line;
@end

@interface Text(Drawing)
- drawSelf:(const NXRect *)rects:(int)rectCount;
@end

@interface Text(Event)
- (BOOL)acceptsFirstResponder;
- becomeFirstResponder;
- becomeKeyWindow;
- clear:sender;
- copy:sender;
- cut:sender;
- delete:sender;
- keyDown:(NXEvent *)theEvent;
- mouseDown:(NXEvent *)theEvent;
- moveCaret:(unsigned short)theKey;
- paste:sender;
- pasteRuler:sender;
- pasteFont:sender;
- resignFirstResponder;
- resignKeyWindow;
- selectAll:sender;
- selectText:sender;
- copyRuler:sender;
- copyFont:sender;
@end

@interface Text(Ruler)
- toggleRuler:sender;
-(BOOL)isRulerVisible;
@end

@interface Text(Selection)
- getSel:(NXSelPt *)start :(NXSelPt *)end;
- hideCaret;
- (BOOL)isSelectable;
- replaceSel:(const char *)aString;
- replaceSel:(const char *)aString length:(int)length;
- replaceSel:(const char *)aString length:(int)length runs:(NXRunArray *)insertRuns;
- replaceSelWithRichText:(NXStream *)stream;
- scrollSelToVisible;
- selectError;
- selectNull;
- setSel:(int)start :(int)end;
- setSelectable:(BOOL)flag;
- setSelGray:(float)value;
- setSelColor:(NXColor) color;
- showCaret;
- subscript:sender;
- superscript:sender;
- underline:sender;
- unscript:sender;
- validRequestorForSendType:(NXAtom)sendType andReturnType:(NXAtom)returnType;
- readSelectionFromPasteboard:pboard;
- (BOOL)writeSelectionToPasteboard:pboard types:(NXAtom *)types;
@end

@interface Text(Font)
- changeFont:sender;
- font;
- (BOOL)isFontPanelEnabled;
- (BOOL)isMonoFont;
- setFont:fontObj;
- setFont:fontObj paraStyle:(void *)paraStyle;
- setSelFontFamily:(const char *)fontName;
- setSelFontSize:(float)size;
- setSelFontStyle:(NXFontTraitMask)traits;
- setSelFont:fontId;
- setSelFont:fontId paraStyle:(void *)paraStyle;
- setFontPanelEnabled:(BOOL)flag;
- setMonoFont:(BOOL)flag;
- changeTabStopAt:(NXCoord)oldX to:(NXCoord)newX;
- setSelProp:(NXParagraphProp)prop to:(NXCoord)val;
- alignSelLeft:sender;
- alignSelRight:sender;
- alignSelCenter:sender;
@end

@interface Text(SpellChecking)
- showGuessPanel:sender;
- checkSpelling:sender;
@end

@interface Text(Graphics)
- replaceSelWithCell:cell;
- replaceSelWithView:view;
- getLocation:(NXPoint *)origin ofCell:cell;
- setLocation:(NXPoint *)origin ofCell:cell;
- getLocation:(NXPoint *)origin ofView:view;
+ registerDirective:(const char *)directive forClass:class;
@end

@interface Object(TextDelegate)
- textWillResize:textObject;
- textDidResize:textObject oldBounds:(const NXRect *)oldBounds invalid:(NXRect *)invalidRect;
- (BOOL)textWillChange:textObject;
- textDidChange:textObject;
- (BOOL)textWillEnd:textObject;
- textDidEnd:textObject endChar:(unsigned short)whyEnd;
- textDidGetKeys:textObject isEmpty:(BOOL)flag;
- textWillSetSel:textObject toFont:font;
- textWillConvert:textObject fromFont:from toFont:to;
- textWillWriteRichText:textObject stream:(NXStream *)stream forRun:(NXRun *)run atPosition:(int)runPosition emitDefaultRichText:(BOOL *)writeDefaultRichText;
- textWillReadRichText:textObject stream:(NXStream *)stream atPosition:(int)runPosition;
- textWillStartReadingRichText:textObject;
- textWillFinishReadingRichText:textObject;
- textWillWrite:textObject paperSize:(NXSize *)paperSize;
- textDidRead:textObject paperSize:(NXSize *)paperSize;
@end

@interface Object(TextCell)
/*
 * Any object added to the Text object via replaceSelWithCell: must
 * respond to all of the following messages:
 */
- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- (BOOL)trackMouse:(NXEvent *)theEvent inRect:(const NXRect *)cellFrame ofView:controlView;
- calcCellSize:(NXSize *)theSize;
- readRichText:(NXStream *)stream forView:view;
- writeRichText:(NXStream *)stream forView:view;
@end

extern void NXTextFontInfo(
    id fid, NXCoord *ascender, NXCoord *descender, NXCoord *lineHt);
extern int NXScanALine(id self, NXLayInfo *layInfo);
extern int NXDrawALine(id self, NXLayInfo *layInfo);
extern unsigned short NXFieldFilter(
    unsigned short theChar, int flags, unsigned short charSet);
extern unsigned short NXEditorFilter(
    unsigned short theChar, int flags, unsigned short charSet);
extern void NXSetTextCache(id self, NXTextCache *cache, int pos);
extern int NXAdjustTextCache(id self, NXTextCache *cache, int pos);
extern void NXFlushTextCache(id self, NXTextCache *cache);
extern void NXWriteWordTable(NXStream *st, const unsigned char *smartLeft,
		const unsigned char *smartRight,
		const unsigned char *charClasses,
		const NXFSM *wrapBreaks, int wrapBreaksCount,
		const NXFSM *clickBreaks, int clickBreaksCount, BOOL charWrap);
extern void NXReadWordTable(NXZone *zone, NXStream *st,
		unsigned char **smartLeft, unsigned char **smartRight,
		unsigned char **charClasses,
		NXFSM **wrapBreaks, int *wrapBreaksCount,
		NXFSM **clickBreaks, int *clickBreaksCount,
		BOOL *charWrap);

typedef struct {
    unsigned char primary[256];
    unsigned char secondary[256];
    unsigned char primaryCI[256];
    unsigned char secondaryCI[256];
} NXStringOrderTable;

extern int NXOrderStrings(const unsigned char *s1, const unsigned char *s2, BOOL caseSensitive, int length, NXStringOrderTable *table);
extern NXStringOrderTable *NXDefaultStringOrderTable(void);


#endif TEXT_H


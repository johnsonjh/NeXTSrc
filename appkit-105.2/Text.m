/*
	Text.m
  	Copyright 1987, 1988, 1989, 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
  
	This file defines the Text Class of the AppKit
  
	Modified:
	01May87	cmf	file creation
*/


 /*
  * Need to do: 
  *
  * 8/04/87 test shift mouse to grow selection 
  *
  * 8/24/87 tell documentation about makeSubViewOf before doing anything that
  * draws 
  *
  * 9/4/87  think up some way to merge _replaceSel and _replaceRun.  at the
  * least carefully scrutinize them both for deviations (like bug fixes
  * applied only to one) 
  *
  * 9/17/87 optimize drawing to trim right and left of line when drawing.  be
  * sure to draw a character more than you think on left and right. 
  *
  * 9/18/87 optimize scanALine for tabs - currently it slows from 19Kcps to
  * 12.5Kcps when you mix in some tabs. 
  *
  * 9/29/87 optimize scanALine where it checks that we don't have too tall of a
  * line, for the case where all runs fit inside lineHt, this is especially
  * easy for mono fontStyle text. 
  *
  * 10/14/87 put in error handling code for readSelf,writeSelf 
  */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "View_Private.h"
#import "Application.h"
#import "Font.h"
#import "FontManager.h"
#import "Pasteboard.h"
#import "ScrollView.h"
#import "Text.h"
#import "appkitPrivate.h"
#import "publicWraps.h"
#import "ClipView.h"
#import "errors.h"
#import <defaults.h>
#import "nextstd.h"
#import "textprivate.h"
#import "textWraps.h"
#import <dpsclient/wraps.h>
#import <errno.h>
#import <mach.h>
#import <math.h>
#import <stdio.h>
#import <sys/param.h>
#import <sys/time.h>
#import <sys/types.h>
#import <sys/file.h>

#ifndef SHLIB
    /* this references category .o files, forcing them to be linked into an app which is linked against a archive version of the kit library. */
    asm(".reference .NXTextCategoryEdit\n");
    asm(".reference .NXTextCategoryEvent\n");
    asm(".reference .NXTextCategoryLayout\n");
    asm(".reference .NXTextCategoryRTF\n");
    asm(".reference .NXTextCategoryReplace\n");
    asm(".reference .NXTextCategoryReplaceUtil\n");
    asm(".reference .NXTextCategoryScanDraw\n");
    asm(".reference .NXTextCategorySelection\n");
    asm(".reference .NXTextCategorySelectUtil\n");
    asm(".reference .NXTextCategorySpell\n");
    asm(".reference .NXTextCategoryStream\n");
    asm(".reference .NXTextCategoryTabs\n");
    asm(".reference .NXTextCategoryUtil\n");
#endif

static BOOL ignoreServicesMenu = NO;
static BOOL servicesMenuDone = NO;
static id textDefaultFont = 0;
static void assignDefaultWordTables(Text *self);
static void validateEndRun(Text *self);

#define FIELDFILTER 1
#define EDITORFILTER 2
#define CLIENTFILTER 3

@implementation Text:View

+ initialize
{
#ifdef DEBUG
    _NXCheckWordTableSizes();
#endif
    return self;
}

+ excludeFromServicesMenu:(BOOL)flag
{
    if ((flag && !ignoreServicesMenu) || (!flag && ignoreServicesMenu)) {
	ignoreServicesMenu = flag ? YES : NO;
	/* should unregister if (ignoreServicesMenu && servicesMenuDone) */
    }
    return self;
}

+ ignoreRequestMenu:(BOOL)flag { return [self excludeFromServicesMenu:flag]; }

+ getDefaultFont
{
    if (!textDefaultFont) {
	const char *appName = [NXApp appName];
	const char *name = NXGetDefaultValue(appName, "NXFont");
	const char *size = NXGetDefaultValue(appName, "NXFontSize");
	float realSize = 0.0;
	static const char defName[] = _NX_INITDEFAULTFONT;

	if (size && *size)
	    sscanf(size, "%f", &realSize);
	if (realSize <= 0.0) {
	    realSize = atof(_NX_INITDEFAULTFONTSIZE);
	    NXLogError("Invalid default font size: %s", size);
	}
	if (!name) {
	    name = defName;
	    NXLogError("NULL default font name");
	}
	textDefaultFont = [Font newFont:name size:realSize];
	if (!textDefaultFont) {
	    NXLogError("Invalid default font name: %s", name);
	    name = defName;
	    textDefaultFont = [Font newFont:name size:realSize];
	}
    }
    return textDefaultFont;
}

+ setDefaultFont:anObject
{
    return textDefaultFont = anObject;
}

+ newFrame:(const NXRect *)frameRect text:(const char *)theText alignment:(int)mode
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect 
	text:theText alignment:mode];
}

- _initFrame:(const NXRect *)frameRect text:(const char *)theText alignment:(int)mode services:(BOOL)flag
{
    NXCoord             newAscent;
    NXCoord             newDescent;
    NXCoord             newHeight;
    id         dFont;
    NXRun     *pr;
    NXTextGlobals *globals;
    NXZone *zone = [self zone];

    dFont = [Text getDefaultFont];
    NXTextFontInfo(dFont, &newAscent, &newDescent, &newHeight);

 /* This leading stuff should go away */
    newHeight = ceil(MAX(newHeight, FONTTOSIZE(dFont)));

    [super initFrame:frameRect];	/* depends on error handling */
    [self notifyToInitGState:YES];
    _info = NXZoneMalloc(zone, sizeof(NXTextInfo));
    bzero(_info, sizeof(NXTextInfo));
    globals = TEXTGLOBALS(_info);
    globals->caretXPos = -1.0;
    PROLOG(_info);
    _NXResetDrawCache(self);
    [self setClipping:NO];
    [self setFlipped:YES];
    [self drawInSuperview];
    bodyRect = bounds;
    bodyRect.origin.x += HFUDGE;
    bodyRect.size.width -= (HFUDGE + HFUDGE);

    charFilterFunc = NXEditorFilter;
    scanFunc = NXScanALine;
    drawFunc = NXDrawALine;
    assignDefaultWordTables(self);
    textLength = 1;
    tFlags._editMode = READWRITE;
    tFlags._selectMode = SELECTABLE;
    tFlags.haveDown = SINotTracking;
    tFlags._caretState = CARETDISABLED;
    tFlags.overstrikeDiacriticals = YES;
    tFlags.monoFont = YES;
    sp0.cp = -1;
    backgroundGray = 1.0;
    globals->backgroundRGBColor = -1;
    selectionGray = NX_DKGRAY;
    firstTextBlock = (NXTextBlock *)NXZoneMalloc(zone, sizeof(NXTextBlock));
    firstTextBlock->text = (unsigned char *)NXZoneMalloc(zone, NX_TEXTPER);
    lastTextBlock = firstTextBlock;
    firstTextBlock->next = firstTextBlock->prior = NULL;
    firstTextBlock->chars = 1;
    firstTextBlock->text[0] = '\n';
    if (theText)
	[self copyText:theText];	/* depends on error handling */
 /* depends on error handling */
    theRuns = (NXRunArray *)NXChunkZoneMalloc(0, sizeof(NXRun), zone);
    theRuns->chunk.used = sizeof(NXRun);
    pr = theRuns->runs;
    bzero(pr, sizeof(NXRun));
    pr->font = dFont;
    pr->textGray = NX_BLACK;
    pr->textRGBColor = -1;
    globals->defaultStyle = _NXMakeDefaultStyle(
	zone, mode, newHeight, newDescent, dFont, 0);
    globals->uniqueStyle = [self getUniqueStyle:globals->defaultStyle];
    pr->paraStyle = globals->uniqueStyle;
    pr->chars = NX_MAXCPOS;
    [self trimRuns];
 /* depends on error handling */
    theBreaks = (NXBreakArray *)NXChunkZoneMalloc(
	0, 10 * sizeof(NXLineDesc), zone);
    [self calcLine];
    [self _setSel:-1:0];
    EPILOG(_info);
    if (flag && !ignoreServicesMenu && !servicesMenuDone) {
	const char *sendTypes[3];
	const char *returnTypes[3];
 	sendTypes[0] = NXAsciiPboardType;
	sendTypes[1] = NXRTFPboardType;
	sendTypes[2] = NULL;
 	returnTypes[0] = NXAsciiPboardType;
	returnTypes[1] = NXRTFPboardType;
	returnTypes[2] = NULL;
	[NXApp registerServicesMenuSendTypes:sendTypes andReturnTypes:returnTypes];
	servicesMenuDone = YES;
    }
    return (self);
}

- initFrame:(const NXRect *)frameRect text:(const char *)theText alignment:(int)mode
{
    return [self _initFrame:frameRect text:theText alignment:mode services:YES];
}


+ new
{
    NXRect              newR;

    newR.origin.x = newR.origin.y = newR.size.width = newR.size.height = 0.0;
    return ([self newFrame:&newR text:NULL alignment:NX_LEFTALIGNED]);
}

+ newFrame:(const NXRect *)frameRect
{
    return [self newFrame:frameRect text:NULL alignment:NX_LEFTALIGNED];
}

- initFrame:(const NXRect *)frameRect
{
    return [self initFrame:frameRect text:NULL alignment:NX_LEFTALIGNED];
}

- (BOOL)_canDraw
{
    BOOL validWindow = [super canDraw];
    return !vFlags.disableAutodisplay && validWindow;
}

- _changeTypingRun:(SEL)aSel with:(void *)data
{

    NXRunArray tempRunArray;
    if (!typingRun.chars) {
	_NXSetDefaultRun(self, 1, &tempRunArray);
	typingRun = tempRunArray.runs[0];
    }
    [self _convertRun:&typingRun selector:aSel with:data];
    return self;
}

- _convertRun:(NXRun *)run selector:(SEL)aSel with:(void *)data
{
    objc_msgSend(self, aSel, run, data);
    return self;
}

- _setParaStyle:(void *)paraStyle
{
    NXRun *run = theRuns->runs;
    NXRun *last = run + (theRuns->chunk.used / sizeof(NXRun));
    for (;run < last; run++) {
	if (paraStyle)
	    run->paraStyle = paraStyle;
    }
    return self;
}

- renewRuns:(NXRunArray *)newRuns
    text:(const char *)newText
    frame:(const NXRect *)newFrame
    tag:(int)newTag
{
    NXSize              tSize;

    [self selectNull];
    tag = newTag;
    validateEndRun(self);
    if (newRuns)
	self->theRuns = (NXRunArray *)NXChunkCopy(
				    &newRuns->chunk, &self->theRuns->chunk);
    if (newText)
	[self copyText:newText];	/* depends on error proc */
    if (newRuns || newText)
	[self trimRuns];
    [self moveTo:newFrame->origin.x:newFrame->origin.y];
    tSize = newFrame->size;
    _NXMinSize(self, &tSize);
    [self sizeTo:tSize.width:tSize.height];
    [self calcLine];
    return (self);
}

- renewFont:newFontId
    text:(const char *)newText
    frame:(const NXRect *)newFrame
    tag:(int)newTag
{
    theRuns->runs[0].font = newFontId;
    theRuns->runs[0].chars = NX_MAXCPOS;
    theRuns->chunk.used = sizeof(NXRun);
    [self trimRuns];
    [self calcParagraphStyle:newFontId :[self alignment]];
    return[self renewRuns:NULL
	   text:newText
	   frame:newFrame
	   tag:newTag];
}

- renewFont:(const char *)newFontName
    size:(float)newFontSize
    style:(int)newFontStyle
    text:(const char *)newText
    frame:(const NXRect *)newFrame
    tag:(int)newTag
{
    return[self renewFont:[Font newFont:newFontName
			   size:newFontSize
			   style:newFontStyle
			   matrix:NX_FLIPPEDMATRIX]
	   text:newText
	   frame:newFrame
	   tag:newTag];
}

- free
{
    NXTextStyle *style;
    NXTextInfo *info = _info;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    [self hideCaret];
    if (firstTextBlock)
	[self freeBList:firstTextBlock];
    [info->globals.tabTable free];
    if (info->globals.rtfStyles) {
	NXFreeHashTable(info->globals.rtfStyles);
	info->globals.rtfStyles = NULL;
    }
    if (info->globals.cellInfo) {
	NXFreeHashTable(info->globals.cellInfo);
	info->globals.cellInfo = NULL;
    }
    if (theRuns) {
	style = globals->defaultStyle;
	free(style->tabs);
	free(style);
	free(theRuns);
    }
    if (textStream) {
	NXClose(textStream);
	textStream = 0;
    }
    if (theBreaks)
	free(theBreaks);
    free(info);
    return[super free];
}

/** View Frame Geometry **/

 /*
  * !!!! I must figure out how to maintain relationship between bounds and
  * bodyRect in the translated, !!!! rotated world 
  */

- superviewSizeChanged:(const NXSize *)oldSize
{
    NXSize oldTextSize = frame.size;
    NXRect superFrame;
	    
    [super superviewSizeChanged:oldSize];
    /* in the special case where the text object is a docview of a clipView
     * rewrap and fix the line table and reset the min size. The
     * neat thing here is that when Interface builder resizes its text
     * object, this code will automatically fix the min size.
     */
    if (tFlags.inClipView) { 
	if(tFlags.vertResizable && !tFlags.horizResizable &&
	(oldTextSize.width != frame.size.width)) {
	    [self calcLine];
	}
    [superview getFrame:&superFrame];
    [self setMinSize:&superFrame.size];
    [self sizeToFit];
    }
    return self;
}

- setEditable:(BOOL)flag
{
    if (sp0.cp == spN.cp) {
	if (!flag) {
	    if ([self _canDraw]) 
		[self hideCaret];
	} else if (self == [window firstResponder] && [window isKeyWindow]) {
	    if ([self _canDraw]) 
		[self showCaret];
	}
    }
    tFlags._editMode = flag ? READWRITE : READONLY;
    if (flag && ![self isSelectable])
	[self setSelectable:YES];
    return self;
}

- (BOOL)isEditable
{
    return tFlags._editMode != READONLY;
}

- adjustPageHeightNew:(float *)newBottom top:(float)oldTop
    bottom:(float)oldBottom limit:(float)bottomLimit
{
    int        cw;
    NXLineDesc *ps;
    NXCoord    yPos, newPos, ht = 0.0;
    int        stopLine, curLine;

    yPos = bodyRect.origin.y;
    ps = theBreaks->breaks;
    curLine = 0;
    stopLine = theBreaks->chunk.used;
    while (curLine < stopLine) {
	cw = *(TX_LINEAT(ps, curLine));
	curLine += sizeof(NXLineDesc);
	if (cw < 0) {
	    ht = TX_HTINFO(ps, curLine)->newHeight;
	    curLine += sizeof(NXHeightInfo);
	}
	newPos = yPos + ht;
	if (newPos > oldBottom)
	    break;
	yPos = newPos;
    }
    *newBottom = yPos;
    return self;
}

- (int) _countRuns:(NXTextCache *)cache
{
    int nRuns = theRuns->chunk.used / sizeof(NXRun);
    NXRun *lastRun = theRuns->runs + nRuns;
    NXRun *curRun = cache->curRun;
    int count = 0, curPos = cache->runFirstPos, lastPos = spN.cp;
    id lastFont = curRun->font;
    for (; curRun < lastRun; curRun++) {
	if (curRun->font != lastFont) {
	    count++;
	    lastFont = curRun->font;
	}
	curPos += curRun->chars;
	if (curPos >= lastPos)
	    break;
    }
    return count;
}
- _convertFont:(NXRun *)run with:(void *)data
{
    id font, newfont;
    SenderAndStyle *ss = data;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    if (!run)
	return self;
    font = run->font;
    newfont = [ss->sender convertFont:font];
    if (delegate && globals->gFlags.willConvertFont)
	newfont = [delegate textWillConvert:self fromFont:font toFont:newfont];
    run->font = newfont;
    if (ss->paraStyle)
	run->paraStyle = ss->paraStyle;
    return self;
}

- _fixFontPanel
{
    id fontManager, fontPanel, fontMenu;
    id font = nil;
    BOOL multiple = NO;
    NXLayInfo *layInfo = _info;
    NXTextCache *cache = &layInfo->cache;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    if (tFlags.disableFontPanel || 
	![self _canDraw] || sp0.cp < 0 || globals->gFlags.changingFont)
	return self;
    fontManager = [FontManager new];
    fontPanel = [fontManager getFontPanel:NO];
    fontMenu = [fontManager getFontMenu:NO];
    if (!fontPanel && !fontMenu)
	return self;
    if (tFlags.monoFont) 
	font = theRuns->runs[0].font;
    else if (sp0.cp == spN.cp) {
	if (typingRun.chars)
	    font = typingRun.font;
	else {
	    int charindex;
	    charindex = sp0.cp;
	    if(charindex >= [self textLength])
		charindex--;
	    NXAdjustTextCache(self, cache, charindex);
	    font = cache->curRun->font;
	}
    } else {
	NXAdjustTextCache(self, cache, sp0.cp);
	font = cache->curRun->font;
	multiple = [self _countRuns:cache] ? YES : NO;
    }
    if (font) {
	if (delegate && globals->gFlags.willSetSel)
	    font = [delegate textWillSetSel: self toFont:font];
	if (font && [font isKindOf:[Font class]])
	    [fontManager setSelFont:font isMultiple:multiple];
    }
    return self;
}

- getParagraph:(int)prNumber start:(int *)startPos length:(int *)numChars
    rect:(NXRect *)paragraphRect
{
/*  this is the old bogus sematics, left here for compatibility  */
    return [self getParagraph:prNumber start:startPos end:numChars 
	rect:paragraphRect];
}

- getParagraph:(int)prNumber start:(int *)startPos end:(int *)endPos
    rect:(NXRect *)paragraphRect
{
    int        cp;
    int        cplast;
    NXCoord    ht = 0.0;
    NXCoord    yp;
    NXCoord    yplast;

    {
	NXLineDesc *ps;
	NXLineDesc *lps;
	int        nLines;
	int        cw;
	int        pNumber;

	ps = theBreaks->breaks;
	lps = TX_LINEAT(ps, theBreaks->chunk.used);
	cplast = cp = 0;
	nLines = 0;
	pNumber = prNumber;
	yp = yplast = bodyRect.origin.y;
	while (ps < lps) {
	    cw = *ps++;
	    if (TXISPARA(cw)) {
		if (pNumber-- < 0)
		    break;
		yplast = yp;
		cplast = cp;
	    }
	    if (cw < 0) {
		yp += (NXCoord)nLines *ht;

		ht = ((NXHeightInfo *)ps)->newHeight;
		ps = TX_LINEAT(ps, sizeof(NXHeightInfo));
		nLines = 0;
	    }
	    cw &= TXMCHARS;
	    cp += cw;
	    nLines++;
	}
	yp += (NXCoord)(--nLines) * ht;
    }
    {
	NXRect    *pr;

	*startPos = cplast;
	*endPos = cp;
	pr = paragraphRect;
	pr->origin.x = bodyRect.origin.x;
	pr->size.width = bodyRect.size.width;
	pr->origin.y = yplast;
	pr->size.height = yp + ht - yplast;
    }
    return self;
}

- (int)textLength
{
    return (textLength - 1);
}

- (int)byteLength
{
    return (textLength - 1);
}

- (int)getSubstring:(char *)buf start:(int)startPos length:(int)numChars
{
    int        fromPos;
    int        nChars;
    NXTextBlock *pb;
    int        totalChars;
    int        xfer;

    fromPos = startPos;
    if (fromPos > textLength)
	return (-1);
    nChars = numChars;
    pb = firstTextBlock;
    totalChars = 0;
    while (pb) {
	xfer = pb->chars;
	fromPos -= xfer;
	if (fromPos < 0) {
	    fromPos += xfer;
	    xfer -= fromPos;
	    if (xfer > nChars)
		xfer = nChars;
	    totalChars += xfer;
	    nChars -= xfer;
	    bcopy(pb->text + fromPos, buf, xfer);
	    pb = pb->next;
	    goto breakFirst;
	}
	pb = pb->next;
    }

breakFirst:
    while (pb && nChars) {
	xfer = pb->chars;
	if (xfer > nChars)
	    xfer = nChars;
	bcopy(pb->text, buf + totalChars, xfer);
	totalChars += xfer;
	nChars -= xfer;
	pb = pb->next;
    }
    if ((totalChars + startPos) == textLength)
	buf[totalChars - 1] = '\0';
    return (totalChars);
}

- (NXTextBlock *)firstTextBlock
{
    return (firstTextBlock);
}

- setText:(const char *)aString
{
    [self selectNull];
    validateEndRun(self);
    [self copyText:aString];	/* depends on error stuff */
    [self trimRuns];
    [self calcLine];
    return (self);
}

- (int)_readText:(NXStream *)stream
 /*
  * this method reads new text from \fIstream\fB 
  */
{
    char      *t;
    NXTextBlock *pb;
    NXTextBlock *npb;
    int        ncs;
    int        ncsRead;
    int        nciBlock;
    int        ncsMax;
    int        readError;

    validateEndRun(self);
    NXFlushTextCache(self, CACHE(_info));
    ncs = 0;
    ncsMax = 0x7FFFFFFF;
    pb = self->firstTextBlock;
    nciBlock = NX_TEXTPER;
    t = (char *)pb->text;
    readError = 0;
    while (1) {
	if (ncs > ncsMax - nciBlock)
	    nciBlock = ncsMax - ncs;
	ncsRead = NXRead(stream, t, nciBlock);
	if (ncsRead < 0) {
	    readError++;
	    ncsRead = 0;
	}
	ncs += ncsRead;
	if (ncsRead < nciBlock || ncs == ncsMax) {
	    if (ncs == ncsMax) { /* exceeded MAXCHARS */
	        ncs--;
		ncsRead--;
	    }
	    t[ncsRead] = '\n';
	    ncsRead++;
	    ncs++;
	    pb->chars = ncsRead;
	    if (npb = pb->next)
		[self freeBList:npb];
	    pb->next = NULL;
	    self->textLength = ncs;
	    break;
	}
	pb->chars = nciBlock;
	if (!(npb = pb->next)) {
	/* depends on error handling */
	    NXZone *zone = [self zone];
	    npb = (NXTextBlock *)NXZoneMalloc(zone, sizeof(NXTextBlock));
	    npb->text = (unsigned char *)NXZoneMalloc(zone, NX_TEXTPER);
	    pb->next = npb;
	    npb->next = NULL;
	    npb->prior = pb;
	    self->lastTextBlock = npb;
	}
	pb = npb;
	t = (char *)pb->text;
    }
    [self trimRuns];
    return (readError);
}

- readText:(NXStream *)stream
{
    int                 readError;

    [self selectNull];
    readError = [self _readText:stream];
    [self calcLine];
    if (readError)
	NX_RAISE(NX_textBadRead, 0, 0);
    return (self);
}

- readRichText:(NXStream *)stream
{
    if (!stream) {
	return self;
    } {
	NXSetTextCache(self, CACHE(_info), 0);
	[self _setSel:0 :textLength - 1];
	[self readRTF:stream op:archive_io];
	NXFlushTextCache(self, CACHE(_info));
    }

    return self;
}

- writeText:(NXStream *)stream
{
    NXTextBlock *pb;
    int        xfer;

    pb = firstTextBlock;
    while (pb) {
	xfer = pb->chars;
	if (pb == lastTextBlock) 
	    xfer--;
	if (xfer != NXWrite(stream, pb->text, xfer)) {
	    NX_RAISE(NX_textBadWrite, 0, 0);
	    break;
	}
	pb = pb->next;
    }
    return self;
}

- writeRichText:(NXStream *)stream

{
    [self writeRTFAt:0 end:textLength - 1 into:stream op:archive_io];
    NXFlush(stream);
    return self;
}

- writeRichText:(NXStream *)stream from:(int)start to:(int)end
{
    [self writeRTFAt:start end:end into:stream op:pasteboard_io];
    return self;
}

- setOverstrikeDiacriticals:(BOOL)flag
{
    tFlags.overstrikeDiacriticals = flag ? YES : NO;
    return self;
}

- (int)overstrikeDiacriticals
{
    return tFlags.overstrikeDiacriticals;
}

- setBorderWidth:(NXCoord)value
{
    borderWidth = value;
    return self;
}

- (NXCoord)borderWidth
{
    return (borderWidth);
}

- setScanFunc:(NXTextFunc)aFunc
{
    scanFunc = aFunc;
    return self;
}

- (NXTextFunc)scanFunc
{
    return (scanFunc);
}

- setDrawFunc:(NXTextFunc)aFunc
{
    drawFunc = aFunc;
    return self;
}

- (NXTextFunc)drawFunc
{
    return (drawFunc);
}

- setCharFilter:(NXCharFilterFunc) aFunc
{
    charFilterFunc = aFunc;
    return self;
}

- (NXCharFilterFunc) charFilter
{
    return (charFilterFunc);
}

- setTextFilter:(NXTextFilterFunc)aFunc
{
    textFilterFunc = aFunc;
    return self;
}

- (NXTextFilterFunc)textFilter
{
    return (textFilterFunc);
}

- (const unsigned char *)preSelSmartTable
{
    return (preSelSmartTable);
}

- setPreSelSmartTable:(const unsigned char *)aTable
{
    preSelSmartTable = aTable;
    return self;
}

- (const unsigned char *)postSelSmartTable
{
    return (postSelSmartTable);
}

- setPostSelSmartTable:(const unsigned char *)aTable
{
    postSelSmartTable = aTable;
    return self;
}

- (const unsigned char *)charCategoryTable
{
    return (charCategoryTable);
}

- setCharCategoryTable:(const unsigned char *)aTable
{
    charCategoryTable = aTable;
    return self;
}

- (const NXFSM *)breakTable
{
    return (breakTable);
}

- setBreakTable:(const NXFSM *)aTable
{
    breakTable = aTable;
    return self;
}


- (const NXFSM *)clickTable
{
    return (clickTable);
}

- setClickTable:(const NXFSM *)aTable
{
    clickTable = aTable;
    return self;
}

- setTag:(int)anInt
{
    tag = anInt;
    return self;
}

- (int)tag
{
    return (tag);
}

- setDelegate:anObject
{
    delegate = anObject;
    [self delegateFeatures];
    return self;
}

- delegate
{
    return (delegate);
}

- setBackgroundGray:(float)value
{
    backgroundGray = value;
    return self;
}

- (float)backgroundGray
{
    return (backgroundGray);
}

- setBackgroundColor:(NXColor)color
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);

    globals->backgroundRGBColor = _NXToTextRGBColor(color);
    return self;
}

- (NXColor)backgroundColor
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);

    return (_NXFromTextRGBColor(globals->backgroundRGBColor));
}

- setTextGray:(float)value
{
    NXRun *run, *last;
    textGray = value;
    run = theRuns->runs;
    last = run + (theRuns->chunk.used / sizeof(NXRun));
    for (; run < last; run++) {
	run->textGray = value;
    }
    return self;
}

- (NXColor)textColor
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);

    return (_NXFromTextRGBColor(globals->textRGBColor));
}

/*
 * Set all the runs to a particular color.
 * If the color is a gray value, remove the color from the runs & just set gray
 * value. Normally when setting color the gray value is still left inplace, and
 * used when a window is not on a color display. Might as well just use the gray
 * value if the color is gray. 
 */
- setTextColor:(NXColor)color
{
    NXRun *run, *last;
    float	red,green,blue,alpha;
    int		RGBColor;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    
    globals->textRGBColor = _NXToTextRGBColor(color); /* set instance var */
    
    NXConvertColorToRGBA (color, &red, &green, &blue, &alpha);
    run = theRuns->runs;
    last = run + (theRuns->chunk.used / sizeof(NXRun));
    
    if(red == blue && red == green) {
	for (; run < last; run++) {
	    run->textGray = red;
	    run->textRGBColor = -1;
	}
    }
    else {
	RGBColor = _NXToTextRGBColor(color);
	for (; run < last; run++) {
	    run->textRGBColor = RGBColor;
	}
    }
    return self;
}

- (float)textGray
{
    return (textGray);
}

- (float)selGray
{
    return (selectionGray);
}

- windowChanged:newWindow
{
    if (tFlags._caretState != CARETDISABLED)
	[self hideCaret];
    return [super windowChanged:newWindow];
}

- (NXStream *)stream
{
    if (!textStream)
	textStream = _NXOpenTextStream(self);
    [self validateStream];
    return textStream;
}

- initGState
{
    _NXRestoreDrawCache(self);
    return self;
}

- write:(NXTypedStream *) stream
{

    int                 items;
    NXStream           *auxstream;
    int                 length;
    char               *streambuf;
    int                 streamlen, streammax;

    [super write:stream];
    NXWriteObjectReference(stream, delegate);
    NXWriteRect(stream, &bodyRect);
    NXWriteTypes(stream, "ciifffcfffs", &delegateMethods, &tag, &growLine, &maxY, &maxX, &borderWidth, &clickCount, &backgroundGray, &textGray, &selectionGray, &tFlags);
    NXWriteSize(stream, &maxSize);
    NXWriteSize(stream, &minSize);
 /* We do not save: cursorTE sp0 spN anchorL anchorR */
    items = (breakTable == NXEnglishNoBreakTable) ? 1 : 0;
    NXWriteType(stream, "i", &items);
    items = (charFilterFunc == NXEditorFilter) ? EDITORFILTER :
      ((charFilterFunc == NXFieldFilter) ? FIELDFILTER : CLIENTFILTER);
    NXWriteType(stream, "i", &items);
    auxstream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
    [self writeRichText:auxstream];
    length = NXTell(auxstream);
    NXWriteTypes(stream, "i", &length);
    NXGetMemoryBuffer(auxstream, &streambuf, &streamlen, &streammax);
    NXWriteArray(stream, "c", streamlen, streambuf);
    NXCloseMemory(auxstream, NX_FREEBUFFER);
    return self;
}

/*
 *  once I get this working, we may need to check getDone before we try to read
 *  these things in, so that sub classes of text can do the use us to read in
 *  most of this stuff. talk to Tshumi about this!!!
 *
 */

- read:(NXTypedStream *) stream
{
    NXRun              *pr;
    id                  dFont;
    float               newAscent, newDescent, newHeight;
    int                 items;
    NXStream           *auxstream;
    int                 length;
    char               *streambuf;
    NXTextGlobals *globals;
    NXZone *zone = [self zone];

    [super read:stream];
    [self notifyToInitGState:YES];
    delegate = NXReadObject(stream);
    NXReadRect(stream, &bodyRect);
    _info = NXZoneMalloc(zone, sizeof(NXTextInfo));
    bzero(_info, sizeof(NXTextInfo));
    globals = TEXTGLOBALS(_info);
    globals->caretXPos = -1.0;
    PROLOG(_info);
    _NXResetDrawCache(self);
    NXReadTypes(stream, "ciifffcfffs", &delegateMethods, &tag, &growLine, &maxY, &maxX, &borderWidth, &clickCount, &backgroundGray, &textGray, &selectionGray, &tFlags);
    NXReadSize(stream, &maxSize);
    NXReadSize(stream, &minSize);
 /* We do not save: cursorTE sp0 spN anchorL anchorR */
    dFont = [Font newFont:"Helvetica" size:12.0];
    _NXTextFontInfoInView(self, dFont, &newAscent, &newDescent, &newHeight);
    firstTextBlock = (NXTextBlock *)NXZoneMalloc(zone, sizeof(NXTextBlock));
    firstTextBlock->text = (unsigned char *)NXZoneMalloc(zone, NX_TEXTPER);
    lastTextBlock = firstTextBlock;
    firstTextBlock->next = firstTextBlock->prior = NULL;
    firstTextBlock->chars = 1;
    firstTextBlock->text[0] = '\n';
    theBreaks = (NXBreakArray *)NXChunkZoneMalloc(
	0, 10 * sizeof(NXLineDesc), zone);

    typingRun.chars = 0;
    charFilterFunc = NXEditorFilter;
    scanFunc = NXScanALine;
    drawFunc = NXDrawALine;
    assignDefaultWordTables(self);
    textLength = 1;
    theRuns = (NXRunArray *)NXChunkZoneMalloc(0, sizeof(NXRun), zone);
    theRuns->chunk.used = sizeof(NXRun);
    pr = theRuns->runs;
    bzero(pr, sizeof(NXRun));
    pr->font = dFont;
    pr->textGray = textGray;
    pr->textRGBColor = -1;
    globals->backgroundRGBColor = -1;
    globals->defaultStyle = _NXMakeDefaultStyle(
	zone, NX_LEFTALIGNED, newHeight, newDescent, pr->font, 0);
    globals->uniqueStyle = [self getUniqueStyle:globals->defaultStyle];
    pr->paraStyle = globals->uniqueStyle;
    pr->chars = NX_MAXCPOS;
    [self trimRuns];

    NXReadType(stream, "i", &items);
    if (items) {
	charCategoryTable = NXEnglishCharCatTable;
	breakTable = NXEnglishNoBreakTable;
	clickTable = NXEnglishClickTable;
    }
    NXReadType(stream, "i", &items);
    switch (items) {
    case FIELDFILTER:
	charFilterFunc = NXFieldFilter;
	break;
    case EDITORFILTER:
	charFilterFunc = NXEditorFilter;
	break;
    default:
	break;
    };
    sp0.cp = -1;
    [self calcLine];
    [self _setSel:0 :0];
    NXReadTypes(stream, "i", &length);
    streambuf = (char *)NXZoneMalloc(zone, length);
    NXReadArray(stream, "c", length, streambuf);
    auxstream = NXOpenMemory(streambuf, length, NX_READONLY);
    [self readRichText:auxstream];
    NXCloseMemory(auxstream, NX_SAVEBUFFER);
    free(streambuf);
    [self delegateFeatures];
    tFlags.inClipView = [superview isKindOf:[ClipView class]];
    EPILOG(_info);
    if (!ignoreServicesMenu && !servicesMenuDone) {
	const char *sendTypes[3];
	const char *returnTypes[3];
 	sendTypes[0] = NXAsciiPboardType;
	sendTypes[1] = NXRTFPboardType;
	sendTypes[2] = NULL;
 	returnTypes[0] = NXAsciiPboardType;
	returnTypes[1] = NXRTFPboardType;
	returnTypes[2] = NULL;
	[NXApp registerServicesMenuSendTypes:sendTypes andReturnTypes:returnTypes];
	servicesMenuDone = YES;
    }
    return self;
}

- writeRichText:(NXStream *)stream forRun:(NXRun *)run 
    atPosition:(int)runPosition emitDefaultRichText:(BOOL *) writeDefaultRTF
{
    NXTextGlobals *globals = TEXTGLOBALS(self->_info);
    if (globals->gFlags.willWriteRichText)
	[delegate textWillWriteRichText:self stream:stream forRun:run
	    atPosition:runPosition emitDefaultRichText:writeDefaultRTF];
    return self;
}

- readRichText:(NXStream *)stream atPosition:(int)runPosition
{
    NXTextGlobals *globals = TEXTGLOBALS(self->_info);
    if (globals->gFlags.willReadRichText)
	[delegate textWillReadRichText:self stream:stream 
	    atPosition:runPosition];
    return self;
}

- finishReadingRichText
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    if (globals->gFlags.willFinishRichText)
	[delegate textWillFinishReadingRichText:self];
    return self;
}

- startReadingRichText
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    if (globals->gFlags.willStartRichText)
	[delegate textWillStartReadingRichText:self];
    return self;
}

- _bringSelToScreen:(NXCoord)pad
 /*
  * Brings the start of the selection (plus a little) onto the screen. 
  */
{
    NXRect              sRect;
    NXCoord    x;
    NXCoord    z;
    NXCoord    right;
    NXCoord    mo;

    if (sp0.cp >= 0) {
	sRect.origin.y = sp0.y;
	sRect.size.height = sp0.ht;
	x = sp0.x - pad;
	mo = bodyRect.origin.x;
	if (x < mo)
	    x = mo;
	z = x + 2 * pad;
	right = mo + bodyRect.size.width;
	if (z > right)
	    z = right;
	sRect.origin.x = x;
	sRect.size.width = z - x;
	[self scrollRectToVisible:&sRect];
    }
    return self;
}

- _setSuperview:sView
 /* over-ride View method to cache info about superview */
{
    tFlags.inClipView = [sView isKindOf:[ClipView class]];
    return ([super _setSuperview:sView]);
}

static void assignDefaultWordTables(Text *self)
{
    /* default word tables */
    static const unsigned char *smartLeft = NULL, *smartRight = NULL;
    static const unsigned char *charClasses = NULL;
    static const NXFSM *breakTable = NULL, *clickTable = NULL;
    static BOOL charWrap;
    const char *filename;
    int dum1, dum2;
    int fd = -1;
    NXStream *st = NULL;
    const char *const *languages;
    char path[MAXPATHLEN+1];

    if (!smartLeft) {
	filename = NXGetDefaultValue([NXApp appName], "NXWordTablesFile");
	if (!filename) {
	    languages = [NXApp systemLanguages];
	    if (languages) {
		while (!filename && *languages) {
		    if (!strcmp(*languages, KIT_LANGUAGE)) break;
		    sprintf(path, "%sLanguages/%s/WordTables", _NXAppKitFilesPath, *languages);
		    if (!access(path, R_OK)) {
			filename = path;
			break;
		    }
		    languages++;
		}
	    }
	}
	if (!filename || !strcmp(filename, "English")) {
	  CallItEnglishAfterAll:
	    smartLeft = NXEnglishSmartLeftChars;
	    smartRight = NXEnglishSmartRightChars;
	    charClasses = NXEnglishCharCatTable;
	    breakTable = NXEnglishBreakTable;
	    clickTable = NXEnglishClickTable;
	    charWrap = YES;
	} else if (!strcmp(filename, "C")) {
	    smartLeft = NXCSmartLeftChars;
	    smartRight = NXCSmartRightChars;
	    charClasses = NXCCharCatTable;
	    breakTable = NXCBreakTable;
	    clickTable = NXCClickTable;
	    charWrap = YES;
	} else if (*filename == '/') {
	    if ((fd = open(filename, O_RDWR, 0)) >= 0)
		if (st = NXOpenFile(fd, NX_READONLY))
		    NXReadWordTable([self zone], st, &smartLeft, &smartRight,
			    &charClasses, &breakTable, &dum1,
			    &clickTable, &dum2, &charWrap);
	    if (st)
		NXClose(st);
	    if (fd >= 0)
		close(fd);
	    if (!smartLeft && !charClasses)
		NXLogError("Text: couldnt open word table file %s", filename);
	    /* fill in unassigned fields */
	    if (!smartLeft) {
		smartLeft = NXEnglishSmartLeftChars;
		smartRight = NXEnglishSmartRightChars;
	    }
	    if (!charClasses) {
		charClasses = NXEnglishCharCatTable;
		breakTable = NXEnglishBreakTable;
		clickTable = NXEnglishClickTable;
		charWrap = YES;
	    }
	} else {
	    NXLogError("Text: unrecognized word table name %s", filename);
	    goto CallItEnglishAfterAll;
	}
    }
    self->charCategoryTable = charClasses;
    self->preSelSmartTable = smartLeft;
    self->postSelSmartTable = smartRight;
    self->breakTable = breakTable;
    self->clickTable = clickTable;
    self->tFlags.charWrap = charWrap;
}

/*  Makes sure that all runs fields are valid for trailing newline run. Also
    removes graphics runs. */

static void validateEndRun(Text *self)
{
    int nRuns;
    NXRun *last;
    NXRun *penultimate;
    int chars;
    
    nRuns = self->theRuns->chunk.used / sizeof(NXRun);
    last = self->theRuns->runs + (nRuns-1);
    [self freeCellsFrom:self->theRuns->runs to:last];
    nRuns = self->theRuns->chunk.used / sizeof(NXRun);
    last = self->theRuns->runs + (nRuns-1);
    if (nRuns > 1) { /* if there is more than one run */
        chars = last->chars;
	penultimate = last-1;
	*last = *penultimate;
	last->chars = chars;
    }
}

- setRetainedWhileDrawing:(BOOL)aFlag
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    globals->gFlags.retainedWhileDrawing = aFlag ? YES : NO;
    return self;
}

- (BOOL) isRetainedWhileDrawing
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    return globals->gFlags.retainedWhileDrawing ? YES : NO;
}

@end

/*
  
Modifications (starting at 0.8):
  
12/06/88 trey	Added conditional support for printing the selection.
12/08/88 bgy	added return self to all methods that didn't have 
		 it already.
		Fixed a bug in setSelFont: where the scroll view didn't get
		 updated when the frame grew larger.
12/14/88 bgy	Added a new type to Text.h, called NXLineDesc.  Line table
		 entries used to be declared as shorts, but this causes
		 problems for RISC architectures.  NXLineDesc is still a short
		 but changing it to a different type is much easier now.
		Took the code out of the text block copying procedures to
		 copy text a character at a time, versus a 4 byte word at
		 a time. The new code is slower, but much more machine
		 independent.
0.82
----
 1/06/89 bgy	Implemented sizeToFit and scrollSelToVisible
 1/12/89 pah	get rid of selectText (without the colon)
 1/16/89 bgy	massive prototyping, including some API changes. 
		 converting paraStyle from int to void *
		 Introduced the following types:
		    typedef int (*NXTextFunc) (Text *self); 
		    typedef int (*NXCharFilterFunc) (	
			unsigned short theChar, int flags);
		    typedef char  *(*NXTextFilterFunc) (	
			Text *self, unsigned char * insertText, 
			int *insertLength, int position);
 1/18/89 cmf    Fixed free to free tabs from default style.
 1/27/89 bs    	added read: and write:.
 2/01/89 trey	added initialEditableCheck() to factor out code
		 for sending textWillChange: on keyDown:, delete:, paste:.
		 textWillChange: is only called ONCE after text is
		 made firstResp.
		made tFlags.changeState be cleared in becomeFirstResp,
		 and set in initialEditableCheck
		selectNull always hides the caret
		tFlags._editMode defaults to READWRITE.
		tFlags._selectMode defaults to SELECTABLE
		isSelectable just consults the flag, and no longer calls
		 textWillChange:
		resignFirstResp no longer locks/unlocks focus
		timer events added to do auto scroll in mouseDown:
		in keyDown:, delete:, paste: textDidChange: is only called
		 ONCE after text is made firstResp, using tFlags.changeState
		 as a dirty bit
  3/15/89 cmf    Fixed free to free _info.

0.91
----
 5/19/89 trey	minimized static data
 6/12/89 bgy	fixed draw cache to emit the printer fonts while printing
 		 fixed replaceSel: to not emit and postscript while 
		 display is disabled .  
0.93
----
 6/17/89 wrp	change to NXIntersectionRect() to clear resulting rect if no 
 		 intersection showed up a bug in _hilite:::: where the end of 
		 the selection was set after doing the NXIntersectionRect().  
		 The fix was to do it before and adjust it afterwards only if 
		 necessary.
 6/17/89 wrp	renamed isScroll to isClipView. Changed code which checked for 
 		 ScrollView to check for ClipView instead.
 6/17/89 trey	fixed caret timed entry so caret flashes in modal panels 
 6/22/89 trey	generate beep whenever charFilterFunc erjects the char
 6/22/89 bgy	caches information about being in a clipview and not doing 
 		 clips 

0.94
----
 7/13/89 pah	changeFont: must return nil if disableFontPanel is true
 7/10/89 trey	paste returns nil if it doesnt accept the available data types
		_NXNotChangeable() code moved into initialEditableCheck()
		delete returns self if initialEditableCheck() fails, since it
		 shouldnt pass the action up the resp chain
		beep issued on editing read-only text instead of exception
		 raising
 7/24/89 trey	robustified against bogus default font and font size
 8/18/89 trey	fixed tracking loop to pitch key events in the loop,
		 not using DPSDiscardEvents

12/14/89 trey	fixed Text to pass NULL as Pasteboard owner
12/21/89 trey	Text uses new word table default to get default wrap tables

 3/09/90 bgy	Rearranged Text.m, textUtil.m, rtfText.m into the following:
 		 Text.m, textEdit.m, textEvent.m, textLayout.m, textPublic.m, 
		 textRTF.m, textReplace.m, textReplaceUtil.m, textScanDraw.m, 
		 textSelect.m, textSelectUtil.m, textStream.m, textUtil.m

appkit-80
---------
  4/9/90 pah	made Text register its Request Menu types in + newFrame:

84
--
 5/10/90 chris	added set of theRuns->chunk.used whereever
 		theRuns->runs[0].chars = NX_MAXCPOS used
		
85
--
 5/17/90 chris	_readText: is modified to ALWAYS append trailing newLine, which
 		writeText: NEVER writes.
86
--
 6/12/90 pah	updated Text object to new Request Menu registration scheme
		added + ignoreRequestMenu:(BOOL)flag
87
--
 6/21/90 chris	readText: has redundant call to trimRuns - _readText: does it,
 		too
 6/21/90 chris	fixed reference to bogus trailing newline run in trimRuns, 
 		where
 		text is being loaded. ie:
		- (int)_readText:(NXStream *)stream
		- renewRuns:(NXRunArray *)newRuns
		    text:(const char *)newText
		    frame:(const NXRect *)newFrame
		    tag:(int)newTag
		- setText:(const char *)aString
 7/12/90 glc	new methods to support color text.	

89
--
 7/27/90 glc	setTextColor: wasn't setting color instance var.	
 7/29/90 trey	nuked awake method, removing redundant calcLine:
 
91
--
 8/11/90 glc	free info->globals.tabTable

92
--
 8/20/90 gcockrft Fixed the false font which showed at end of text after 
 		  changing all fonts with select all and a font change.
 8/20/90 bryan	add setRetainedWhileDrawing: and isRetainedWhileDrawing

94
--
 9/25/90 gcockrft	added byteLength.
 			Redo minsize when clipview changes size.
*/

/*
	textUtil.m
  	Copyright 1988, 1989, 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Text.h"
#import "textprivate.h"
#import "textWraps.h"
#import "errors.h"
#import "nextstd.h"
#import <stdio.h>
#import <math.h>
#import <dpsclient/wraps.h>
#import <stdio.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryUtil=0\n");
    asm(".globl .NXTextCategoryUtil\n");
#endif

typedef struct {
    @defs (Text)
} TextId;

static int arrayCount = 0;
static NXGlobalArrays **arrayStack = 0;
#define STACKSIZE 10

@implementation Text (Util)
 /*
  * think about how to represent selection at end of line vs start of next
  * line. applies to anchor,too 
  */

- freeBList:(NXTextBlock *)pb
{
    NXTextBlock *npb;

    if (pb->prior)
	pb->prior->next = pb->next;
    if (!(lastTextBlock = pb->prior))
	firstTextBlock = NULL;
    while (pb) {
	npb = pb;
	pb = pb->next;
	free(npb->text);
	free(npb);
    }
    return self;
}

 /*
  * Internal routine that copies newText into theText. 
  */

- copyText:(const char *)newText
{
    int nciBlock;
    char *t, nada[1];
    NXTextBlock *pb, *npb;
    const char *newTextSave;
    NXZone *zone = [self zone];

    if (!newText) {
	nada[0] = '\0';
	newText = nada;
    }
    NXFlushTextCache(self, CACHE(_info));
    newTextSave = newText;
    pb = firstTextBlock;
    nciBlock = NX_TEXTPER;
    t = (char *)pb->text;
    while (1) {
	if (!nciBlock) {
	    nciBlock = NX_TEXTPER;
	    pb->chars = nciBlock;
	    if (!(npb = pb->next)) {
	    /* depends on error handling */
		npb = (NXTextBlock *)NXZoneMalloc(zone, sizeof(NXTextBlock));
		npb->text = (unsigned char *)NXZoneMalloc(zone, NX_TEXTPER);
		pb->next = npb;
		npb->next = NULL;
		npb->prior = pb;
		lastTextBlock = npb;
	    }
	    pb = npb;
	    t = (char *)pb->text;
	}
	nciBlock--;
	if (!(*t++ = *newText++)) {
	    *(--t) = '\n';
	    pb->chars = NX_TEXTPER - nciBlock;
	    if (npb = pb->next)
		[self freeBList:npb];
	    pb->next = NULL;
	    textLength = newText - newTextSave;
	    return self;
	}
    }
    return self;
}

 /*
  * this proc trims theRuns or pads the lastRun so that theText is all
  * covered by a run. Also removes redundant runs (possibly added by renew:*)
  */

- trimRuns
{
    int runFirstPos, last, nc, delta, nextChars;
    NXRun *curRun, *lastRun, *oldRun, *endRun, *nextRun;

    runFirstPos = 0;
    last = textLength - 1;
    oldRun = curRun = theRuns->runs;
    lastRun = TX_RUNAT(curRun, theRuns->chunk.used);
    endRun = TX_RUNAT(curRun, theRuns->chunk.allocated);
    nextRun = curRun+1;
    delta = 0;
    while (curRun < lastRun) {
	nc = curRun->chars;
	if (nc) {
	    delta = 0;
	    while (nextRun < lastRun) {
	        nextChars = nextRun->chars;
		if ((!nextChars) || EQUALRUN(curRun,nextRun)) {
		    nc += nextChars;
		    nextRun->chars = 0;
		    delta += sizeof(NXRun);
		} else
		    break;
		nextRun++;
	    }
	    curRun->chars = nc;
	    theRuns->chunk.used -= delta;
	} else {
	    if (nextRun < lastRun) {
		*curRun = *nextRun++;
		nc = curRun->chars;
	    } else
	        break;
	}
	if (last < (runFirstPos + nc)) {
	    curRun->chars = last - runFirstPos;
	    if (curRun->chars && last) {
		if (++curRun == endRun) {
		    theRuns = (NXRunArray *)NXChunkRealloc(&theRuns->chunk);
		    curRun = theRuns->runs + (curRun - oldRun);
		    bzero(curRun, theRuns->chunk.allocated - 
			TX_BYTES(curRun, theRuns->runs));
		}
		*(curRun) = *(curRun - 1);
	    }
	    curRun->chars = 1;
	    curRun->info = 0;
	    curRun++;
	    theRuns->chunk.used = TX_BYTES(curRun, theRuns->runs);
	    return self;
	}
	curRun++;
	if (nextRun <= curRun)
	    nextRun = curRun+1;
	runFirstPos += nc;
    }
    curRun--;
    curRun->chars += last - runFirstPos;
    curRun++;
    if (last) {
	if (curRun == endRun) {
	    theRuns = (NXRunArray *)NXChunkRealloc(&theRuns->chunk);
	    curRun = theRuns->runs + (curRun - oldRun);
	    bzero(curRun, theRuns->chunk.allocated - 
		TX_BYTES(curRun, theRuns->runs));
	}
	*curRun = *(curRun - 1);
    } 
    curRun->chars = 1;
    curRun->info = 0;
    theRuns->chunk.used += sizeof(NXRun);
    return self;
}


extern NXTextStyle *_NXMakeDefaultStyle(
	NXZone *zone, int newJust, NXCoord newHeight, NXCoord newDescent,
	id nFont, NXTextStyle *ps)
{
    NXTabStop *pts, *lpts;
    NXCoord eightBlanks, x;

    eightBlanks = 8.0 * FONTTOWIDTHS(nFont)[' '] * FONTTOSIZE(nFont);
    if (!ps) {
	pts = (NXTabStop *)(NXZoneMalloc(zone, 10 * sizeof(NXTabStop)));
	ps = (NXTextStyle *)(NXZoneMalloc(zone, sizeof(NXTextStyle)));
	ps->tabs = pts;
	ps->numTabs = 10;
    } else {
	pts = ps->tabs;
    }
    ps->alignment = newJust;
    ps->lineHt = newHeight;
    ps->descentLine = newDescent;
    ps->indent1st = ps->indent2nd = 0.0;
    lpts = pts + ps->numTabs;
    x = eightBlanks;
    while (pts < lpts) {
	pts->kind = NX_LEFTTAB;
	pts->x = x;
	x += eightBlanks;
	pts++;
    }
    return (ps);
}



extern int _NXPosNewline(
	  id _self, NXTextInfo * info, NXSelPt *sp, NXSelPt *synch)
{
    TextId             *self = (TextId *) _self;
    int        nLines;
    int        cw;
    int        cp;
    int        state;
    NXLineDesc *oldps = 0;
    int        line;
    NXBreakArray *theBreaks;
    NXCoord    ht;
    NXCoord    yp;

    {
	NXSelPt   *s;

	s = sp;
	line = sp->line;
	ht = sp->ht;
	yp = sp->y;
	cp = sp->c1st;
    }

    {
	NXLineDesc *ps;
	NXLineDesc *lps;

	theBreaks = self->theBreaks;
	ps = theBreaks->breaks;
	lps = TX_LINEAT(ps, theBreaks->chunk.used);
	ps = TX_LINEAT(ps, line);
	cw = TXGETCHARS(*ps);
	if (sp->cp >= (cp + cw)) {
	    yp += ht;
	    if (*ps < 0)
		ps = TX_LINEAT(ps, sizeof(NXHeightChange));
	    else
		ps = TX_LINEAT(ps, sizeof(NXLineDesc));
	    cp += cw;
	    if (*ps < 0)
		ht = HTCHANGE(ps)->heightInfo.newHeight;
	}
	state = 0;
	nLines = 0;
	while (ps < lps) {
	    oldps = ps;
	    cw = *ps++;
	    if (cw < 0) {
		yp += (NXCoord)nLines *ht;

		ht = ((NXHeightInfo *)ps)->newHeight;
		ps = TX_LINEAT(ps, sizeof(NXHeightInfo));
		nLines = 0;
	    }
	    cw &= TXMCHARS;
	    cp += cw;
	    nLines++;
	    if (!state) {
		if (TXISPARA(*oldps))
		    state++;
	    } else if (state > 0) {
		state++;
		break;
	    }
	}
	yp += (NXCoord)(--nLines) * ht;
	cp -= cw;
    }

    {
	NXSelPt   *sp;

	sp = synch;
	sp->cp = -1;
	sp->y = yp + ht;
	if (state == 2) {
	    sp->cp = cp;
	    sp->line = TX_BYTES(oldps, theBreaks->breaks);
	    sp->y = yp;
	    sp->c1st = cp;
	    sp->ht = ht;
	}
    }

    return (cp);
}



 /*
  * internal procedure.  whenever the anchor should be invalidated, call
  * _NXClearClickState(), which will set self up as for single click on end
  * of selection indicated by anchorIs0. 
  */

extern void _NXClearClickState(id _self)
{
    TextId    *self = (TextId *) _self;

    self->clickCount = 1;
    self->anchorR = self->anchorL =
      (self->tFlags.anchorIs0 ? self->sp0 : self->spN);
    self->growLine = self->spN.line;
}

extern NXCoord _NXSetDrawBounds(id _self, NXCoord *pTop)
{
    TextId    *self = (TextId *) _self;
    NXCoord    top;
    NXCoord    bottom;
    NXRect              brect;
    NXCoord    BBottom;

    top = self->bodyRect.origin.y;
    bottom = top + self->bodyRect.size.height;
    if ([(id)self getVisibleRect:&brect]) {
	BBottom = brect.origin.y + brect.size.height;
	if (bottom < brect.origin.y)
	    bottom = brect.origin.y;
	if (bottom > BBottom)
	    bottom = BBottom;
	if (top < brect.origin.y)
	    top = brect.origin.y;
	if (top > BBottom)
	    top = BBottom;
    } else
	bottom = top;
    *pTop = top;
    return (bottom);
}


 /*
  * Internal routine that asks the new delegate what methods it
  * delegateMethods.  This information is cached by the text object to avoid
  * asking every time a delegate method is required.  The TextObject does the
  * reasonable thing as a default if the delegate does not respond to any
  * method. 
  *
  */

- (int)delegateFeatures
{
    int mask;
    NXTextGlobals *globals = TEXTGLOBALS(_info);

    mask = 0;
    if ([delegate respondsTo:@selector(textWillResize:)])
	mask |= TXMWILLGROW;
    if ([delegate respondsTo:@selector(textDidResize:oldBounds:invalid:)])
	mask |= TXMGROWACTION;
    if ([delegate respondsTo:@selector(textWillChange:)])
	mask |= TXMWILLCHANGE;
    if ([delegate respondsTo:@selector(textDidChange:)])
	mask |= TXMCHANGEACTION;
    if ([delegate respondsTo:@selector(textWillEnd:)])
	mask |= TXMWILLEND;
    if ([delegate respondsTo:@selector(textDidEnd:endChar:)])
	mask |= TXMENDACTION;
    if ([delegate respondsTo:@selector(textDidGetKeys:isEmpty:)])
	mask |= TXMDIDGETKEYS;
    else if ([delegate respondsTo:@selector(text:isEmpty:)])
	mask |= TXMISEMPTY;
    globals->gFlags.willConvertFont = 
	[delegate respondsTo:@selector(textWillConvert:fromFont:toFont:)] ?
	YES : NO;
    globals->gFlags.willSetSel = 
	[delegate respondsTo:@selector(textWillSetSel:toFont:)] ? YES : NO;
    globals->gFlags.willWritePaperSize = 
	[delegate respondsTo:@selector(textWillWrite:paperSize:)] ? YES : NO;
    globals->gFlags.didReadPaperSize = 
	[delegate respondsTo:@selector(textDidRead:paperSize:)] ? YES : NO;
    globals->gFlags.willWriteRichText = 
	[delegate respondsTo:
@selector(textWillWriteRichText:stream:forRun:atPosition:emitDefaultRichText:)]
	? YES : NO;
    globals->gFlags.willReadRichText = 
	[delegate respondsTo:
	    @selector(textWillReadRichText:stream:atPosition:)] ? YES : NO;
    globals->gFlags.willFinishRichText = 
	[delegate respondsTo:
	    @selector(textWillFinishReadingRichText:text:runs:)] ? YES : NO;
    globals->gFlags.willStartRichText = 
	[delegate respondsTo:
	    @selector(textWillStartReadingRichText:text:runs:)] ? YES : NO;
    delegateMethods = mask;
    return (mask);
}

extern NXRunArray  *_NXSetDefaultRun(id _self, int iLen, NXRunArray *dfltRun)
{
    TextId *self = (TextId *) _self;
    int runcp;
    NXTextCache *cache = CACHE(self->_info);
    NXLineDesc *prevLine;

    dfltRun->chunk.used = dfltRun->chunk.allocated = sizeof(NXRun);
    if (!self->typingRun.chars) {	/* if there isn't a typingRun, get
					 * one */
	prevLine = TX_LINEAT(self->theBreaks->breaks,
			self->sp0.line - sizeof(NXLineDesc));
	runcp = self->sp0.cp;	/* assume run from char to right of sp0 */
	if (runcp && runcp == self->spN.cp &&
	    (runcp != self->sp0.c1st ||
		!self->sp0.line ||
		!TXISPARA(*prevLine)))
	    runcp--;
	if (runcp == self->textLength - 1)
	    runcp--;
	NXAdjustTextCache(_self, cache, runcp);
	self->typingRun = *cache->curRun;
	self->typingRun.info = 0;
	self->typingRun.rFlags.graphic = 0;
	self->typingRun.rFlags.subclassWantsRTF = 0;
	/* 
	 * Now make sure we have the correct paragraph style.
	 * Matches the code in updateRuler used to set ruler from selection.
	 */
	 if(self->sp0.cp >= 0) {
	    NXSelPt temp;
	    temp = self->sp0;
	    [_self findStartParagraph:&temp];
	    NXAdjustTextCache(_self, cache, temp.cp);
	    self->typingRun.paraStyle = cache->curRun->paraStyle;
	 }
    }
    dfltRun->runs[0] = self->typingRun;
    dfltRun->runs[0].chars = iLen;
    return (dfltRun);
}

extern void _NXMinSize(id _self, NXSize *theSize)
{
    TextId    *self = (TextId *) _self;
    NXSize    *minSize = &self->minSize;

    if (theSize->width < minSize->width)
	theSize->width = minSize->width;
    if (theSize->height < minSize->height)
	theSize->height = minSize->height;
}

static NXGlobalArrays *getGlobalArrays(void)
{
    NXGlobalArrays     *result;
    NXZone	       *zone;

    if (!arrayStack) {
	zone = NXApp ? [NXApp zone] : NXDefaultMallocZone();
	arrayStack = NXZoneMalloc(zone, STACKSIZE * sizeof(NXGlobalArrays *));
    }
    if (arrayCount > 0) {
	arrayCount--;
	result = arrayStack[arrayCount];
    } else {
	zone = NXApp ? [NXApp zone] : NXDefaultMallocZone();
	result = NXZoneMalloc(zone, sizeof(NXGlobalArrays));
	result->lays = (NXLayArray *)NXChunkMalloc(0, 10 * sizeof(NXLay));
	result->widths = (NXWidthArray *)NXChunkMalloc(0,
					       INITARRAY * sizeof(NXCoord));
	result->xPos =
	  (NXWidthArray *)NXChunkMalloc(0, INITARRAY * sizeof(NXCoord));
	result->chars = (NXCharArray *)NXChunkMalloc(0, INITARRAY);
	result->savedBreaks =
	  (NXBreakArray *)NXChunkMalloc(0, 10 * sizeof(NXLineDesc));
	AK_ASSERT((arrayCount == 0), "text global arrays allocated");
    }
    return result;
}

static void returnGlobalArrays(NXGlobalArrays * array)
{
    if (arrayCount >= STACKSIZE) {
	free(array->lays);
	free(array->widths);
	free(array->xPos);
	free(array->chars);
	free(array->savedBreaks);
	free(array);
	AK_ASSERT((arrayCount <= 1), "text global arrays freed");
    } else {
	arrayStack[arrayCount] = array;
	arrayCount++;
    }
}


+ initTextInfo:(NXTextInfo *) info
{
    NXGlobalArrays     *array;

    array = getGlobalArrays();
    info->globals.arrays = array;
    info->layInfo.lays = array->lays;
    info->layInfo.widths = array->widths;
    info->layInfo.chars = array->chars;
    info->globals.savedBreaks = array->savedBreaks;
    info->globals.xPos = array->xPos;
    return self;
}

+ finishTextInfo:(NXTextInfo *) info
{
    NXGlobalArrays     *array;

    array = info->globals.arrays;
    array->lays = info->layInfo.lays;
    array->widths = info->layInfo.widths;
    array->chars = info->layInfo.chars;
    array->savedBreaks = info->globals.savedBreaks;
    array->xPos = info->globals.xPos;
    returnGlobalArrays(array);
    info->globals.arrays = 0;
    info->layInfo.lays = 0;
    info->layInfo.widths = 0;
    info->layInfo.chars = 0;
    info->globals.savedBreaks = 0;
    info->globals.xPos = 0;
    return self;
}

- _setFont:fontObj paraStyle:(void *)paraStyle
{
    NXTextStyle *unique;
    theRuns->runs[0].chars = NX_MAXCPOS;
    theRuns->chunk.used = sizeof(NXRun);
    if (fontObj)
	theRuns->runs[0].font = fontObj ;
    if (paraStyle) {
	unique = [self getUniqueStyle:paraStyle];
	theRuns->runs[0].paraStyle = unique;
    }
    typingRun.chars = 0;
    [self trimRuns];
    fontObj = theRuns->runs[0].font;
    if (fontObj && !paraStyle)
	[self calcParagraphStyle:fontObj :[self alignment]];
    [self calcLine];
    return self;
}

@end

/*
  
Modifications (starting at 0.8):
  
12/06/88 trey	Added conditional support for printing the selection.
01/16/89 bgy	prototyped all C functions, add protos to textprivate.h,
		 Text.h, and to the beginning of this file.
01/26/89 trey	_NXNotChangeable no longer calls willChange:.

0.91
----
 5/19/89 trey	minimized static data
 5/26/89 wrp	added code to blinkCaret to not showps when showps enabled
 7/10/89 trey	_NXNotChangeable nuked, distributed into Text.m
 7/11/89 trey	added _NXTextFontInfoInView to get correct ascent and decent
		 when scaled
 7/17/89 bgy	fix smart paste when pboard has white space at start or end


12/21/89 trey	nuked old word tables, new ones in wordTables.m
		nuked support for NX_EMSPACE, NX_ENSPACE, NX_THINSPACE
84
--
 5/10/90 chris	modified trimRuns to remove redundant runs (made necessary by
 		addition of run at end for trailing newline.

87
--
7/12/90	glc	moved the drawcache and fontcache routines into the textScanDraw.m

94
--
 9/25/90 gcockrft	Get the correct para style when inserting text in
 			_NXSetDefaultRun.	
 
*/












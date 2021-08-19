/*
	textPublic.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Text.h"
#import "textprivate.h"
#import "textWraps.h"
#import "appkitPrivate.h"
#import <stdlib.h>
#import <string.h>
#import <math.h>

typedef struct {
    @defs (Text)
} TextId;

/* rounds size in base coord system.  roundup says which way to round
   in BASE COORDS.  YES means round up, towards the top of the window.
 */
static float roundSize(id self, float size, BOOL roundUp)
{
    NXSize tempSize;

    tempSize.width = 0.0;
    tempSize.height = size;
    _NXPureConvertSize(self, nil, &tempSize);
    if (roundUp)
	tempSize.height = ceil(tempSize.height);
    else
	tempSize.height = floor(tempSize.height);
    _NXPureConvertSize(nil, self, &tempSize);
    return tempSize.height;
}

extern NXChunk *NXChunkMalloc(int growBy, int initUsed)
{
    return NXChunkZoneMalloc(growBy, initUsed, NXDefaultMallocZone());
}


extern NXChunk *NXChunkZoneMalloc(int growBy, int initUsed, NXZone *zone)
{
    NXChunk   *pc;


    if (!initUsed)
	initUsed = growBy;
    pc = (NXChunk *)NXZoneMalloc(zone, initUsed + sizeof(NXChunk));
    pc->growby = growBy;
    pc->used = 0;
    pc->allocated = initUsed;
    return (pc);
}

extern NXChunk *NXChunkRealloc(NXChunk *pc)
{
    return NXChunkZoneRealloc(pc, NXZoneFromPtr(pc));
}

extern NXChunk *NXChunkZoneRealloc(NXChunk *pc, NXZone *zone)
{
    int                 growby = pc->growby ? pc->growby : pc->allocated;

    pc = (NXChunk *)NXZoneRealloc(
	zone, pc, pc->allocated + growby + sizeof(NXChunk));
    pc->allocated += growby;
    return (pc);
}


extern NXChunk *NXChunkGrow(NXChunk *pc, int newUsed)
{
    return NXChunkZoneGrow(pc, newUsed, NXZoneFromPtr(pc));
}

extern NXChunk *NXChunkZoneGrow(NXChunk *pc, int newUsed, NXZone *zone)
{
    int        iby;
    int        gby;

    gby = pc->growby;
    iby = gby ? ((newUsed + gby - 1) / gby) * gby : newUsed;
    if (iby <= pc->allocated)
	return (pc);
    pc = (NXChunk *)NXZoneRealloc(zone, pc, iby + sizeof(NXChunk));
    pc->allocated = iby;
    return (pc);
}


extern NXChunk *NXChunkCopy(NXChunk *pc, NXChunk *dpc)
{
    return NXChunkZoneCopy(pc, dpc, NXZoneFromPtr(pc));
}


extern NXChunk *NXChunkZoneCopy(NXChunk *pc, NXChunk *dpc, NXZone *zone)
{
    NXChunk   *lc;

    lc = (NXChunk *)NXZoneRealloc(zone, dpc, pc->allocated + sizeof(NXChunk));
    bcopy(pc, lc, pc->allocated + sizeof(NXChunk));
    return (lc);
}

extern void NXFlushTextCache(id _self, NXTextCache *cache)
{
    TextId    *self = (TextId *) _self;

    cache->curPos = 0;
    cache->blockFirstPos = 0;
    cache->curBlock = self->firstTextBlock;
    cache->curRun = self->theRuns->runs;
    cache->runFirstPos = 0;
    if (self->textStream)
	[_self invalidateStream];
}

extern void NXSetTextCache(id _self, NXTextCache *cache, int pos)
{
    TextId    *self = (TextId *) _self;
    NXRun     *CurRun;
    NXTextBlock *CurBlock;
    int        TFirstPos;
    int        RFirstPos;

    cache->curPos = pos;
    TFirstPos = 0;
    CurBlock = self->firstTextBlock;
    CurRun = self->theRuns->runs;
    RFirstPos = 0;
    while (pos >= (TFirstPos + CurBlock->chars)) {
	TFirstPos += CurBlock->chars;
	if (!(CurBlock = CurBlock->next))
	    goto endBreak;
    }
    while (pos >= (RFirstPos + CurRun->chars)) {
	RFirstPos += CurRun->chars;
	CurRun++;
    }
  endBreak:
    cache->blockFirstPos = TFirstPos;
    cache->curBlock = CurBlock;
    cache->runFirstPos = RFirstPos;
    cache->curRun = CurRun;
    if (self->textStream)
	[_self invalidateStream];
}

 /*
  * this wants to be a macro perhaps 
  */
extern int NXAdjustTextCache(id _self, NXTextCache *cache, int pos)
{
    TextId    *self = (TextId *) _self;
    NXRun     *CurRun;
    NXTextBlock *CurBlock;
    int        TFirstPos;
    int        RFirstPos;

    if (pos < 0) {
	cache->blockFirstPos = 0;
	cache->curBlock = self->firstTextBlock;
	cache->runFirstPos = 0;
	cache->curRun = self->theRuns->runs;
	return pos;
    }
    cache->curPos = pos;
    TFirstPos = cache->blockFirstPos;
    CurBlock = cache->curBlock;
    RFirstPos = cache->runFirstPos;
    CurRun = cache->curRun;
    if (pos < TFirstPos) {
	do {
	    CurBlock = CurBlock->prior;
	    TFirstPos -= CurBlock->chars;
	} while (pos < TFirstPos);
    } else {

	while (pos >= (TFirstPos + CurBlock->chars)) {
	    TFirstPos += CurBlock->chars;
	    if (!(CurBlock = CurBlock->next))
		return (-1);
	}
    }
    if (pos < RFirstPos) {
	do {
	    CurRun--;
	    RFirstPos -= CurRun->chars;
	} while (pos < RFirstPos);
    } else {
	while (pos >= (RFirstPos + CurRun->chars)) {
	    RFirstPos += CurRun->chars;
	    CurRun++;
	}
    }
    cache->blockFirstPos = TFirstPos;
    cache->curBlock = CurBlock;
    cache->runFirstPos = RFirstPos;
    cache->curRun = CurRun;
    if (self->textStream)
	[_self invalidateStream];
    return (pos);
}

/* the old dumb version that doesnt take into accound what view its in */
extern void NXTextFontInfo(
    id fid, NXCoord *ascender, NXCoord *descender, NXCoord *lineHt)
{
    _NXTextFontInfoInView(nil, fid, ascender, descender, lineHt);
}


/* calcs the ascent and decent, doing the rounding in the base coord system */
extern void _NXTextFontInfoInView(id self,
    id fid, NXCoord *ascender, NXCoord *descender, NXCoord *lineHt)
{
    NXFontMetrics *pf;
    NXCoord    thesize;

    pf = FONTTOMETRICS(fid);
 /*
  * We always use only flipped fonts, so matrix is constant, so we can
  * premultiply matrix[3] == -1.0 into thesize. 
  */
    thesize = -FONTTOSIZE(fid);
    if (!self) {
	*ascender = floor(pf->fontBBox[3] * thesize);
	*descender = ceil(pf->fontBBox[1] * thesize);
    } else {
	*ascender = roundSize(self, pf->fontBBox[3] * thesize, YES);
	*descender = roundSize(self, pf->fontBBox[1] * thesize, NO);
    }
    *lineHt = *descender - *ascender;
}


 /*
  * charFilterFunc allows filtering of characters, mapping to new characters
  * and implementation of tab and return as necessary. returns a character in
  * the set: 
  *
  * 0 == NX_ILLEGAL - illegal character, ignore NX_BACKSPACE '\t' '\n' 
  *
  * NX_RETURN 0x10 - carriage return move NX_TAB 0x11 - tab move NX_BACKTAB 0x12
  * - backtab move NX_LEFT 0x13 - left arrow move NX_RIGHT 0x14 - right arrow
  * move NX_UP 0x15 - up arrow move NX_DOWN 0x16 - down arrow move 
  *
  * ' '...0xFF 
  *
  * note that default filter maps tab to NX_TAB 25 to NX_BACKTAB CR to
  * NX_RETURN shift CR to '\n' '\n' to '\n', NX_DELETE (0x7f) to NX_BACKSPACE
  * NX_BACKSPACE to NX_BACKSPACE 
  *
  * everything else below ' ' to 0 and leaves rest of the characters unmodified. 
  *
  * charFilterFunc SHOULD NEVER RETURN CTRL CHARACTERS EXCEPT FOR
  * '\n',BACKSPACE, and '\t' and the NX_ILLEGAL (== 0) & NX_RETURN to NX_DOWN
  * movement codes. 
  *
  */

#define ANYBUCKEY (NX_COMMANDMASK || NX_SHIFTMASK || NX_CONTROLMASK)

extern unsigned short NXFieldFilter(
	      unsigned short theChar, int flags, unsigned short charSet)
{
    if (flags & NX_COMMANDMASK) {
	theChar = 0;
    } else {
        if (theChar == NX_ENTER)
            return NX_RETURN;
	else if (theChar == NX_BTAB)
	    return NX_BACKTAB;
	else if (theChar == NX_DELETE)
	    theChar = NX_BACKSPACE;
	else if (theChar == NX_CR) {
	    if (flags & NX_ALTERNATEMASK)
		theChar = '\n';
	    else
		theChar = NX_RETURN;
	} else if (theChar == '\t') {
	    if (flags & NX_ALTERNATEMASK)
		theChar = '\t';
	    else
		theChar = NX_TAB;
	} else if (charSet == SYMBOLFONT && !(flags & ANYBUCKEY)) {
	    switch (theChar) {
	    case LEFTARROW:
		return NX_LEFT;
	    case RIGHTARROW:
		return NX_RIGHT;
	    case UPARROW:
		return NX_UP;
	    case DOWNARROW:
		return NX_DOWN;
	    }
	} else if ((theChar < ' ') && (theChar != '\n') &&
		   (theChar != NX_BACKSPACE))
	    theChar = 0;
    }
    return (theChar);
}

extern unsigned short NXEditorFilter(
	       unsigned short theChar, int flags, unsigned short charSet)
{
    if (flags & NX_COMMANDMASK) {
	theChar = 0;
    } else {
        if (theChar == NX_ENTER)
            return NX_RETURN;
	else if (theChar == NX_BTAB)
	    return NX_BACKTAB;
	else if (theChar == NX_DELETE)
	    theChar = NX_BACKSPACE;
	else if (theChar == NX_CR) {
	    theChar = '\n';
	} else if (theChar == '\t') {
	    ;
	} else if (charSet == SYMBOLFONT && !(flags & ANYBUCKEY)) {
	    switch (theChar) {
	    case LEFTARROW:
		return NX_LEFT;
	    case RIGHTARROW:
		return NX_RIGHT;
	    case UPARROW:
		return NX_UP;
	    case DOWNARROW:
		return NX_DOWN;
	    }
	} else if ((theChar < ' ') && (theChar != '\n') &&
		   (theChar != NX_BACKSPACE))
	    theChar = 0;
    }
    return (theChar);
}

 /*
  * Internal routine that shows caret during typing. 
  *
  */

extern void _NXQuickCaret(id _self)
{
    TextId             *self = (TextId *) _self;
    NXCoord             x, y, height, mBottom, right;
    NXRect             *bodyRect = &self->bodyRect;

    if (![_self _canDraw])
	return;
    x = self->sp0.x - 0.5;
    right = NX_MAXX(bodyRect);
    if (x <= right) {
	if (x < NX_X(bodyRect))
	    x = NX_X(bodyRect);
	mBottom = NX_MAXY(bodyRect);
	y = self->sp0.y;
	if (y < mBottom) {
	    height = self->sp0.ht;
	    if (height > (mBottom - y))
		height = mBottom - y;
	_NXResetDrawCache(_self);	    
	_NXquickcaret(x, y, height);
	}
    }
}


 /*
  * Internal routine that blinks the caret. 
  *
  */

extern void _NXBlinkCaret(
	      DPSTimedEntry te, double time, id text, int quick)
{
    TextId    *self = (TextId *) text;
    NXCoord             x, y, height, right, mBottom;
    id  window = [text window];
    NXRect              theRect, *vRect = &theRect, *bodyRect = &self->bodyRect;

    if (![text _canDraw] || !FLUSHWINDOW(window) || 
	text != [window firstResponder] || ![window isKeyWindow])
	return;
    if (self->tFlags._caretState == CARETON) {
	[text lockFocus];
	if ([text getVisibleRect:vRect])
	    _NXhidecaret(
	       NX_X(vRect), NX_Y(vRect), NX_WIDTH(vRect), NX_HEIGHT(vRect));
	self->tFlags._caretState = CARETOFF;
	[text unlockFocus];
    } else {
	x = self->sp0.x - 0.5;
	if (!quick)
	    [text lockFocus];
	right = NX_MAXX(bodyRect);
	if (x <= right) {
	    if (x < NX_X(bodyRect))
		x = NX_X(bodyRect);
	    mBottom = NX_MAXY(bodyRect);
	    y = self->sp0.y;
	    if (y < mBottom) {
		height = self->sp0.ht;
		if (height > (mBottom - y))
		    height = mBottom - y;
		if (quick)
		    _NXquickcaret(x, y, height);
		else {
		    if ([text getVisibleRect:vRect]) {
			[window flushWindow];
			_NXshowcaret(x, y, height,
				     NX_X(vRect), NX_Y(vRect),
				     NX_WIDTH(vRect), NX_HEIGHT(vRect));
		    }
		}
	    }
	}
	self->tFlags._caretState = CARETON;
	if (!quick)
	    [text unlockFocus];
    }
}

 /*
  * Routine called by timed entry to blink caret. 
  *
  */
extern void _NXBlinkCaretTE(DPSTimedEntry te, double time, void *whoToBlink)
{
    if (NXDrawingStatus == NX_DRAWING) {

#ifdef DEBUG
	int showingps = (int)((DPSGetCurrentContext())->chainChild);
	DPSTraceContext(DPSGetCurrentContext(), 0);
#endif DEBUG

	(void)_NXBlinkCaret(te, time, (id)whoToBlink, 0);

#ifdef DEBUG
	DPSTraceContext(DPSGetCurrentContext(), showingps);
#endif DEBUG
    }
}

/*

84
--

05/09/90 chris	modified char filters to have enter key return NX_RETURN

86
--
05/18/90 chris	modified filters to use the new char mapping for backtab

87
--
7/12/90	glc	make sure caret flashing clears draw cache.
*/


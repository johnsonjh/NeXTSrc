/*
	textReplaceUtil.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Application.h"
#import "Text.h"
#import "textprivate.h"
#import <stdlib.h>
#import <string.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryReplaceUtil=0\n");
    asm(".globl .NXTextCategoryReplaceUtil\n");
#endif

static BOOL isWhite(char ch);
static int isSpace(int c);


@implementation Text (ReplaceUtil)

- (int) replaceRangeAt:(int)cp1st end:(int) cpN
    info: (NXTextInfo *) info
    text:(const char *)insertText length:(int *) insertLength
    runs:(NXRunArray *)insertRun smartPaste:(int)smartPaste
    error:(int *) errorNumber
{
    char dribble[NX_TEXTPER];
    NXTextBlock *CurBlock, *block1;
    int TFirstPos1, dribbleChars, blocks, rightBlank, leftBlank, newSelN;
    int iLen = *insertLength;
    NXZone *zone = [self zone];

    rightBlank = leftBlank = 0;
    if (smartPaste) {
	if (cp1st) {
	    leftBlank = [self wordPunct:info pos:cp1st - 1
		table:preSelSmartTable];
	    if (leftBlank && insertText && iLen > 0 && isWhite(*insertText))
		leftBlank = 0;
	}
	if (cpN < (textLength - 1)) {
	    rightBlank = [self wordPunct:info pos:cpN table:postSelSmartTable];
	    if (rightBlank && insertText && iLen > 0 && 
		isWhite(*(insertText + iLen - 1)))
		rightBlank = 0;
	}
    }
    [self pasteRunAt:cp1st end:cpN info:info length:iLen runs:insertRun
	   left:leftBlank right:rightBlank];
    NXAdjustTextCache(self, CACHE(_info), cp1st);
    textLength += iLen - cpN + cp1st + leftBlank + rightBlank;
    newSelN = cp1st + iLen + leftBlank;
    *insertLength = iLen + leftBlank + rightBlank;
    {				/* find begin and end blocks of selection */

	int        TFirstPos;
	int        cPos;

	TFirstPos1 = TFirstPos = info->layInfo.cache.blockFirstPos;
	block1 = CurBlock = info->layInfo.cache.curBlock;
	cPos = cpN;
	blocks = -1;
	while (1) {
	    TFirstPos += CurBlock->chars;
	    if (cPos < TFirstPos) {
		TFirstPos -= CurBlock->chars;
		break;
	    }
	    blocks++;
	    CurBlock = CurBlock->next;
	}
	if (blocks < 0)
	    blocks = 0;
	cPos -= TFirstPos;
	if (dribbleChars = CurBlock->chars - cPos)
	    bcopy(CurBlock->text + cPos, dribble, dribbleChars);
	block1->chars = cp1st - TFirstPos1;
	if (block1 != CurBlock)
	    CurBlock->chars = 0;
    }

    {				/* allocate or free NXTextBlocks to contain
				 * new selection */

	int        available;
	NXTextBlock *npb;
	NXTextBlock *nextpb;

	available = NX_TEXTPER - cp1st + TFirstPos1;
	available = (block1 != CurBlock) ? available + (NX_TEXTPER - dribbleChars) :
	  available - dribbleChars;
	available += (blocks * NX_TEXTPER - iLen - leftBlank - rightBlank);
	if (available < 0) {	/* need more blocks */
	    CurBlock = block1;
	    nextpb = CurBlock->next;
	    while (available < 0) {
	    /* depends on error handler */
		npb = (NXTextBlock *)NXZoneMalloc(zone, sizeof(NXTextBlock));
		npb->text = (unsigned char *)NXZoneMalloc(zone, NX_TEXTPER);
		npb->chars = 0;
		CurBlock->next = npb;
		npb->prior = CurBlock;
		CurBlock = npb;
		available += NX_TEXTPER;
	    }
	    if (CurBlock->next = nextpb)
		nextpb->prior = CurBlock;
	    else
		lastTextBlock = CurBlock;
	} else {		/* possibly need fewer block */
	    CurBlock = block1->next;
	    while (available >= NX_TEXTPER) {
		npb = CurBlock;
		CurBlock = CurBlock->next;
		free(npb->text);
		free(npb);
		available -= NX_TEXTPER;
	    }
	    if (block1->next = CurBlock)
		CurBlock->prior = block1;
	    else
		lastTextBlock = block1;
	}
    }

    {				/* copy text to blocks, then squirt in
				 * dribble */

	int        len;
	const char *c;
	int        cPos;
	int        xfer;

	len = iLen + leftBlank + rightBlank;
	CurBlock = block1;
	c = insertText;
	cPos = cp1st - TFirstPos1;

	while (1) {
	    xfer = NX_TEXTPER - cPos;
	    if (xfer > len)
		xfer = len;
	    if (CurBlock != block1)
		CurBlock->chars = 0;
	    if (xfer) {
		CurBlock->chars += xfer;
		if (leftBlank) {
		    leftBlank = 0;
		    CurBlock->text[cPos++] = ' ';
		    xfer--;
		    len--;
		}
		if (xfer == len && rightBlank) {
		    rightBlank = 0;
		    xfer--;
		    len--;
		    CurBlock->text[cPos + xfer] = ' ';
		}
		if (xfer) {
		    bcopy(c, CurBlock->text + cPos, xfer);
		    c += xfer;
		}
	    }
	    len -= xfer;
	    if (!len) {

	    /*
	     * dribble need not be split across blocks because 1) it comes
	     * last, 2) it fits in one block 
	     */

		if (dribbleChars) {
		    cPos = CurBlock->chars;
		    if (dribbleChars > (NX_TEXTPER - cPos)) {
			CurBlock = CurBlock->next;
			cPos = 0;
		    }
		    bcopy(dribble, CurBlock->text + cPos, dribbleChars);
		    CurBlock->chars = cPos + dribbleChars;
		}
		break;
	    }
	    cPos = 0;
	    CurBlock = CurBlock->next;
	}
    }

    if (block1 != CurBlock)
	block1 = [self mergePrior:block1];
    CurBlock = [self mergePrior:CurBlock];
    if (CurBlock = CurBlock->next)
	CurBlock = [self mergePrior:CurBlock];
    return (newSelN);
}

- reSel:(int)sel line:(int)line info:(NXTextInfo *)info pos:(int)cp
    height:(NXCoord)ht yPos:(NXCoord)yp selection:(NXSelPt *)sp
{
    NXLay *pl, *plb;
    int nciLay;
    NXCoord *pw, hitX;
    NXLayInfo *layInfo = &info->layInfo;
    NXRect *layRect = &layInfo->rect;

    sp->y = yp;
    sp->ht = ht;
    sp->line = line;
    sp->c1st = cp;
    sp->cp = sel;

    pl = layInfo->lays->lays;
    plb = TX_LAYAT(pl, layInfo->lays->chunk.used);
    pw = layInfo->widths->widths;
    hitX = NX_X(layRect) + pl->x;
    while (pl < plb) {
	nciLay = pl->chars;
	if (pl->x != 0.0)
	    hitX = NX_X(layRect) + pl->x;
	while (nciLay) {
	    nciLay--;
	    if (cp >= sel)
		goto breakWhile;
	    hitX += *pw++;
	    cp++;
	}
	pl++;
    }
  breakWhile:

    sp->x = hitX;
    sp0 = *sp;
    spN = *sp;
    growLine = line;
/*  reset state indicating selection arose from single/double click */				 
    _NXClearClickState(self);
    return self;
}

-(BOOL) wordPunct:(NXTextInfo *)info pos:(int)cp
    table:(const unsigned char *)punctTable
{
    int        c;

    NXAdjustTextCache(self, &info->layInfo.cache, cp);
    c = info->layInfo.cache.curBlock->text[
	cp - info->layInfo.cache.blockFirstPos];
    while (*punctTable)
	if (*punctTable++ == c)
	    return NO;
    return YES;
}

#ifdef DEBUG
- printRuns
{
    NXRun *run, *last;
    int curPos = 0;
    run = theRuns->runs;
    last = run + (theRuns->chunk.used / sizeof (NXRun));
    for (; run < last; run++) {
	printf("%d	%d	%s\n", curPos, run->chars, [run->font name]);
	curPos += run->chars;
    }
    return self;
}

#endif

- freeCellsFrom:(NXRun *)start to:(NXRun *)last
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    for (; start < last; start++) {
	if (start->rFlags.graphic) {
	    NXTextCellInfo temp, *ptr;
	    temp.cell = start->info;
	    ptr = NXHashRemove(globals->cellInfo, &temp);
	    if (ptr) {
		[ptr->cell free];
		free(ptr);
	    }
	    start->rFlags.graphic = NO;
	    start->info = nil;
	}
    }
    return self;
}

- fixStyles:(NXRun *)run count:(int)runsToAdd
{
    NXRun *last = run + runsToAdd;
    for (; run < last; run++) {
	run->paraStyle = [self getUniqueStyle:run->paraStyle];
    }
    return self;
}


- pasteRunAt:(int)cp1st end:(int) cpN info:(NXTextInfo *)info 
    length:(int) iLen runs:(NXRunArray *)insertRun
    left:(int)leftBlank right:(int)rightBlank
{
    NXRun *lptr, *rptr, *first, *last, *lastRun, *oldRuns, left, right, *real;
    NXTextCache *cache = CACHE(_info), saved;
    int nRuns = iLen ? (insertRun->chunk.used / sizeof(NXRun)) : 0;
    int runsToRemove, runsToAdd = nRuns, newSize, delta;
    NXAdjustTextCache(self, cache, cp1st);		/* get left side */
    real = cache->curRun;
    if (cache->runFirstPos == cp1st && cp1st) {
	cache->curRun--;
	cache->runFirstPos -= cache->curRun->chars;
    }
    oldRuns = theRuns->runs;
    lptr = cache->curRun;
    left = *lptr;
    lastRun = TX_RUNAT(oldRuns, theRuns->chunk.used - sizeof(NXRun));
    if (lptr == lastRun)
	left.chars--;				/* check for last \n */
    left.chars = cp1st - cache->runFirstPos + leftBlank;
    saved = *cache;
    NXAdjustTextCache(self, cache, cpN);		/* get right side */
    rptr = cache->curRun;
    right = *rptr;
    right.chars = (cache->runFirstPos + right.chars) - cpN + rightBlank;
    [self freeCellsFrom:real to:rptr];
    first = insertRun->runs;
    last = first + (nRuns - 1);
    if (nRuns && EQUALRUN(&left, first)) {	/* check if left is equal */
	left.chars += first->chars;
	first++;
	runsToAdd--;
    }
    if (nRuns && last >= first) {
	if (rptr != lastRun && EQUALRUN(last, &right)) {/* is right equal */
	    right.chars += last->chars;
	    last--;
	    runsToAdd--;
	}
    }
    if (lptr == rptr && rptr != lastRun && !runsToAdd) {/* no font change */
	lptr->chars = left.chars + right.chars;
	return self;
    }
    runsToRemove = (rptr - lptr) - 1;
    if (lptr != rptr && !runsToAdd && rptr != lastRun &&
	EQUALRUN(&left, &right)) {
	left.chars += right.chars;		/* merge end runs */
	right.chars = 0;
	runsToRemove++;
    }
    if (!left.chars)
	runsToRemove += 1;
    delta = (runsToAdd - runsToRemove);
    newSize = theRuns->chunk.used + (delta * sizeof(NXRun));
    if (newSize > theRuns->chunk.allocated)
	theRuns = (NXRunArray *)NXChunkGrow(&theRuns->chunk, newSize);
    lptr = theRuns->runs + (lptr - oldRuns);
    rptr = theRuns->runs + (rptr - oldRuns);
    lastRun = theRuns->runs + (theRuns->chunk.used / sizeof(NXRun));
    if (lastRun - rptr)
	bcopy(rptr, rptr + delta, (lastRun - rptr) * sizeof(NXRun));
    *lptr = left;
    if (runsToAdd) {
	NXRun *starting = lptr + (left.chars ? 1:0);
	bcopy(first, starting, runsToAdd * sizeof(NXRun));
	[self fixStyles:starting count:runsToAdd];
    }
    if (right.chars)
	*(rptr + delta) = right;
    theRuns->chunk.used = newSize;
    *cache = saved;
    cache->curRun = theRuns->runs + (cache->curRun - oldRuns);
    return self;
}

#define ABORTSIZE 40
- (int) doAbortion:(int *)newSelN info:(NXTextInfo *)info
    runs:(NXRunArray *)insertRun
{
    NXEvent *tempEv, tempEvSpace;
    int insertLen, errorNumber = 0;
    char iChars[ABORTSIZE];
    unsigned short newChar;

    insertLen = 0;
    while (insertLen < ABORTSIZE) {
	tempEv = [NXApp peekNextEvent:NX_ALLEVENTS into:&tempEvSpace];
	if (!tempEv || !(NX_EVENTCODEMASK(tempEv->type) &
			 (NX_KEYUPMASK | NX_KEYDOWNMASK)))
	    break;
	if (tempEv->type == NX_KEYUP) {
	    [NXApp getNextEvent:NX_KEYUPMASK];
	    continue;
	}
	if (!(newChar = (*charFilterFunc) (
				   tempEv->data.key.charCode, tempEv->flags,
					       tempEv->data.key.charSet)) ||
	    ((newChar == NX_BACKSPACE) && !insertLen) ||
	    ((newChar >= NX_RETURN) && (newChar <= NX_DOWN)))
	    break;
	if(tempEv->data.key.charSet == NX_SYMBOLSET)
	    break;
	[NXApp getNextEvent:NX_KEYDOWNMASK];
	if (newChar == NX_BACKSPACE) {
	    insertLen--;
	    continue;
	}
	iChars[insertLen++] = newChar;
    }

    if (insertLen) {
	char               *insertText = iChars;

	if (textFilterFunc) {
	    int                 oldLength = insertLen;

	    insertText = (char *)(*textFilterFunc) (
		   self, (unsigned char *)insertText, &insertLen, *newSelN);
	    insertRun->runs[0].chars += (insertLen - oldLength);
	}
	insertRun->runs[0].chars = insertLen;

/*	!!! errorNumber is no longer set and should be removed.  cmf.
 */

	*newSelN = [self replaceRangeAt:*newSelN end:*newSelN info:info
	     text:insertText length:&insertLen runs:insertRun
	     smartPaste:NO error:&errorNumber];
	if (errorNumber)
	    return -1;
    }
    return insertLen;
}

- (NXTextBlock *) mergePrior:(NXTextBlock *)block
{
    NXTextBlock *pblock;
    int pchars, chars, xfer;

    if (pblock = block->prior) {
	chars = block->chars;
	pchars = pblock->chars;
	if ((pchars + chars) <= NX_TEXTPER) {
	    pblock->chars = pchars + chars;
	    bcopy(block->text, pblock->text + pchars, chars);
	    if (pblock->next = block->next)
		block->next->prior = pblock;
	    else
		lastTextBlock = pblock;
	    free(block->text);
	    free(block);
	    block = pblock;
	} else if (chars < (NX_TEXTPER / 2)) {
	    xfer = (chars + pchars) / 2 - chars;
	    if (xfer > 0) {
		bcopy(block->text, block->text + xfer, chars);
		bcopy(pblock->text + pchars - xfer, block->text, xfer);
		pblock->chars -= xfer;
		block->chars += xfer;
	    }
	} else if (pchars < (NX_TEXTPER / 2)) {
	    xfer = (chars + pchars) / 2 - pchars;
	    if (xfer > 0) {
		bcopy(block->text, pblock->text + pchars, xfer);
		bcopy(block->text + xfer, block->text, chars - xfer);
		pblock->chars += xfer;
		block->chars -= xfer;
	    }
	}
    }
    return (block);
}

- smartSelect:(NXTextInfo *)info
{
    int cp, c, line;
    NXLineDesc *ps;
    BOOL extended = NO;
    BOOL leftBlank = YES;
    
    if (sp0.cp) {
	int                 offset;
	cp = spN.cp;
	if (cp >= (textLength-1) ||
	    ![self wordPunct:info pos:cp table:postSelSmartTable]) {
	    cp = sp0.cp-1;
	    NXAdjustTextCache((id)self, &info->layInfo.cache, cp);
	    offset = cp - info->layInfo.cache.blockFirstPos;
	    if (isSpace(info->layInfo.cache.curBlock->text[offset])) {
		extended = YES;
		sp0.cp = cp;
		if (cp < sp0.c1st) {
		    line = sp0.line;
		    ps = TX_LINEAT(theBreaks->breaks, line);
		    if (*ps < 0)
			sp0.ht = HTCHANGE(ps)->heightInfo.oldHeight;
		    line -= sizeof(NXLineDesc);
		    if ((c = *--ps) < 0)
			line -= sizeof(NXHeightInfo);
		    sp0.line = line;
		    sp0.c1st -= TXGETCHARS(c);
		    sp0.y -= sp0.ht;
		}
	    } else
		leftBlank = ![self wordPunct:info pos:cp table:preSelSmartTable];
	}
    }
    if (!extended && leftBlank) {
	cp = spN.cp;
	NXAdjustTextCache((id)self, &info->layInfo.cache, cp);
	if (isSpace(info->layInfo.cache.curBlock->text[cp -
					info->layInfo.cache.blockFirstPos]))
	    spN.cp++;

    }
    return self;
}

 /*
  * Internal routine which is called to grow frame & bounds of text
  * dynamically during rewrap.  Called from _replaceSel and _replaceRun. 
  *
  * - oncePerGrow:(NXRect *)boundsSave maxBounds:(NXRect *)boundsMax shrinks the height of bounds to fit at the end of the calling
  * routine (ie _replaceSel or _replaceRun).  design of scanALine insures
  * that width is shrunk if possible already. 
  *
  * perhaps, we should add padding so that caret is always visible? 
  */


- oncePerGrow:(NXRect *)boundsSave maxBounds:(NXRect *)boundsMax
{
    NXCoord deltaHeight, newheight;
    BOOL sized = NO;

    newheight = maxY;
    if (newheight > maxSize.height)
	newheight = maxSize.height;
    deltaHeight = newheight - bodyRect.size.height;
    if (deltaHeight != 0.0) {
	NXSize              tSize;

	tSize.width = frame.size.width;
	tSize.height = frame.size.height + deltaHeight;
	_NXMinSize(self, &tSize);
	[self sizeTo:tSize.width:tSize.height];
	sized = YES;
    }
    newheight = bounds.size.height;
    if (newheight > boundsMax->size.height)
	boundsMax->size.height = newheight;
    if (!NXEqualRect(&bounds, boundsSave) || 
	!NXEqualRect(boundsSave, boundsMax)) {
	[self resizeText:boundsSave :boundsMax];
	sized = YES;
    }
    if (sized)
	_NXResetDrawCache(self);
    return self;
}


 /*
  * Internal routine which is called to grow frame & bounds of text
  * dynamically, line by line during rewrap.  Called from _replaceSel and
  * _replaceRun. 
  *
  */


- (BOOL) perLineGrow:(NXTextInfo *)info maxBounds:(NXRect *)boundsMax
{
    NXSize tSize;
    NXCoord deltaHeight, deltaX, deltaWidth, newheight;
    NXCoord maxOriginOld, newRight, newOrigin;
    BOOL restore = NO;
    NXLayInfo oldLayInfo, *oldInfo = 0;

    newheight = info->layInfo.rect.origin.y +
	info->layInfo.rect.size.height - bodyRect.origin.y;
    if (newheight > maxSize.height)
	newheight = maxSize.height;
    deltaHeight = newheight - bodyRect.size.height;
    if (deltaHeight < 0.0)
	deltaHeight = 0.0;

    deltaX = info->layInfo.rect.origin.x - bodyRect.origin.x;
    deltaWidth = info->layInfo.rect.size.width - bodyRect.size.width;
    if (deltaHeight == 0.0 && deltaX == 0.0 && deltaWidth == 0.0)
	return NO;
    tSize.width = frame.size.width + deltaWidth;
    tSize.height = frame.size.height + deltaHeight;
    if (tSize.width < info->globals.minBodyWidth)
	return NO;
    [window disableFlushWindow];
    if (deltaX != 0.0) {
	BOOL disableAutodisplay = vFlags.disableAutodisplay;
	vFlags.disableAutodisplay = YES;
	oldInfo = &oldLayInfo;
	bcopy(&info->layInfo, oldInfo, sizeof(NXLayInfo));
	[(id)self moveTo:frame.origin.x + deltaX:
	    frame.origin.y];
	vFlags.disableAutodisplay = disableAutodisplay;
	restore = YES;
    }
    if (deltaWidth != 0.0 || deltaHeight != 0.0) {
	_NXMinSize((id)self, &tSize);
	[self sizeTo:tSize.width:tSize.height];
	restore = YES;
    }
    if (oldInfo) {
	oldInfo->lays = info->layInfo.lays;
	oldInfo->widths = info->layInfo.widths;
	oldInfo->chars = info->layInfo.chars;
	bcopy(oldInfo, &info->layInfo, sizeof(NXLayInfo));
    }
    if (restore) 
	_NXResetDrawCache(self);
    [window reenableFlushWindow];
    newheight = bounds.size.height;
    if (newheight > boundsMax->size.height)
	boundsMax->size.height = newheight;
    maxOriginOld = boundsMax->origin.x;
    newOrigin = bounds.origin.x;
    newRight = newOrigin + bounds.size.width;
    if (newOrigin < maxOriginOld)
	boundsMax->origin.x = newOrigin;
    if (newRight > (maxOriginOld + boundsMax->size.width))
	boundsMax->size.width = newRight - boundsMax->origin.x;
    return (deltaHeight != 0.0);
}


@end

static BOOL isWhite(char ch)
{
    return (isSpace(ch) || ch == '\n' || ch == '\t');
}

static int isSpace(int c)
{
    return (c == ' ' || c == NX_FIGSPACE);
}

/*

84
--
 5/07/90 chris	Fixed smartSelect to follow correct rules.
 5/10/90 chris	Move _NXClearClickState() call from _replaceSel:::::: to
 		reSel:line:info:pos: so that anchor isn't set while sp0.cp==-1
		
97
--
10/10/90 glc	Leave batching if we see symbol font character	
*/






/*
	textSelectUtil.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Cell.h"
#import "Text.h"
#import "ScrollView.h"
#import "NXRulerView.h"
#import "textprivate.h"
#import <math.h>
#import <dpsclient/wraps.h>
#import "publicWraps.h"

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategorySelectUtil=0\n");
    asm(".globl .NXTextCategorySelectUtil\n");
#endif

@implementation ScrollView (Ruler)

- toggleRuler:sender
{
    BOOL isTextObject = [sender isKindOf:[Text class]];
    if (_sFlags.rulerInstalled) {
	_sFlags.rulerInstalled = NO;
	[_ruler removeFromSuperview];
    } else {
	if (isTextObject) {
	    NXZone *zone = [self zone];
	    _sFlags.rulerInstalled = YES;
	    if (!_ruler)
		_ruler = [[NXRulerView allocFromZone:zone] init];
	    [self addSubview:_ruler];
	}
    }
    [window disableDisplay];
    [self tile];
    [window reenableDisplay];
    [self display];
    if (isTextObject)
	return _sFlags.rulerInstalled ? _ruler : nil;
    return self;
}

- ruler 
{
    return _ruler;
}

@end


@implementation Text(SelectUtil)

- findStartParagraph:(NXSelPt *)sel
{
    NXTextInfo *info = _info;
    int line = sel->line;
    NXLineDesc *cur, *start, desc;
    NXCoord curHeight, lastHeight;
    sel->x = NX_X(&info->layInfo.rect);
    sel->cp = sel->c1st;
    curHeight = lastHeight = sel->ht;
    if (line) {
	start = theBreaks->breaks;
	cur = TX_LINEAT(start, line);
	desc = *cur;
	while (cur > start) {
	    if (desc < 0) 
		curHeight = HTCHANGE(cur)->heightInfo.oldHeight;
	    cur--;
	    if (*cur < 0) 
		cur = TX_LINEAT(cur, (-sizeof(NXHeightInfo)));
	    desc = *cur;
	    if (TXISPARA(desc))
		break;
	    sel->cp -= TXGETCHARS(desc);
	    sel->y -= curHeight;
	    sel->line -= ((desc < 0) ? 
		sizeof(NXHeightChange) : sizeof(NXLineDesc));
	    lastHeight = curHeight;
	}
	sel->c1st = sel->cp;
	if (line != sel->line)
	    sel->ht = lastHeight; 
    }
    return self;
}

- findEndParagraph:(NXSelPt *)sel
{
    NXTextInfo *info = _info;
    int line = sel->line, newCp;
    NXLineDesc *cur, *start, *last, desc, oldDesc;
    NXCoord curHeight, lastHeight;

    curHeight = lastHeight = sel->ht;
    start = theBreaks->breaks;
    last = TX_LINEAT(start, theBreaks->chunk.used);
    cur = TX_LINEAT(start, line);
    desc = *cur;
    newCp = sel->c1st + TXGETCHARS(desc);
    if (newCp == sel->cp && TXISPARA(desc))
	return self;
    sel->x = NX_MAXX(&info->layInfo.rect);
    sel->cp = newCp;
    while (cur < last) {
	if (TXISPARA(desc))
	    break;
	if (desc < 0)
	    cur = TX_LINEAT(cur, sizeof(NXHeightChange));
	else
	    cur++;
	oldDesc = desc;
	desc = *cur;
	if (desc < 0) 
	    curHeight = HTCHANGE(cur)->heightInfo.newHeight;
	sel->c1st = sel->cp;
	sel->cp += TXGETCHARS(desc);
	sel->y += lastHeight;
	lastHeight = curHeight;
	sel->line += (oldDesc < 0) ? 
	    sizeof(NXHeightChange) : sizeof(NXLineDesc);
    }
    sel->ht = curHeight;
    return self;
}

- selectParagraph
{
    BOOL                canDraw = [self _canDraw];
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    
    NXSelPt oldSp0 = sp0;
    NXSelPt oldSpN = spN;
    
    if (sp0.cp < 0)
	return nil;
    if (self == [window firstResponder])
	if (canDraw) {
	    [self lockFocus];
	    [self hideCaret];
	    if (!ISNULLSEL)
		[self _hilite:NO :&sp0:&spN:NULL];
	    [self unlockFocus];
	} else
	    [self hideCaret];
    else {
	sp0.cp = -1;
	if (window && ![window makeFirstResponder:self]) {
	    return nil;	/* error */
	}
    }
    globals->caretXPos = -1.0;
    typingRun.chars = 0;
    [self findStartParagraph:&oldSp0];
    [self findEndParagraph:&oldSpN];
    sp0 = oldSp0;
    spN = oldSpN;
    if (canDraw) {
	[self lockFocus];
	[self _hilite:YES :&sp0:&spN:NULL];
	if (FLUSHWINDOW(window))
	    PSflushgraphics();
	[self unlockFocus];
	if (!tFlags.monoFont && [window isKeyWindow])
	    [self _fixFontPanel];
    }
    if (!globals->gFlags.movingCaret)
	globals->caretXPos = -1.0;
    return self;
}

- updateRuler
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXTextInfo *info = _info;
    NXTextCache *cache = &info->layInfo.cache;
    NXSelPt temp;
    if (!sp0.cp < 0)
	return nil;
    temp = sp0;
    [self findStartParagraph:&temp];
    NXAdjustTextCache(self, cache, temp.cp);
    [globals->ruler setRulerStyle:cache->curRun->paraStyle for:self];
    return self;
}

- toggleRuler:sender
{
    id view;
    id scrollView = nil;
    NXTextGlobals *globals = TEXTGLOBALS(_info);

    if (tFlags.monoFont) {
	NXBeep();
	return self;
    }
    for (view = superview; view; view = [view superview]) {
	if ([view isKindOf:[ScrollView class]])
	    scrollView = view;
    }
    if (!scrollView)
	return nil;
    globals->ruler = [scrollView toggleRuler:self];
    if (globals->ruler)
	[self updateRuler];

    return self;
}

-(BOOL)isRulerVisible
{
    return(HAVE_RULER(_info) != 0);
}


- clickBounds:(NXTextInfo *)info hitPos:(int)hitPos p0:(NXSelPt *)_downP0
    pN:(NXSelPt *)_downPN
{
    int cPos, wordPos;
    NXCoord leftX = 0, hitX;

    if (clickCount >= 3) {/* triple click */
	NXSelPt   *downP;

	info->globals.hitRect.origin.x = info->layInfo.rect.origin.x;
	info->globals.hitRect.size.width = info->layInfo.rect.size.width;
	downP = _downP0;
	[self findStartParagraph:downP];
	downP = _downPN;
	[self findEndParagraph:downP];
    } else {			/* double click */
	{
	    unsigned char *cp;
	    const unsigned char *chClass;
	    const NXFSM     *state;
	    const NXFSM     *trans;
	    unsigned char *wordp;
	    unsigned char *nca;
	    unsigned char *ncaHit;

	    state = clickTable;
	    chClass = charCategoryTable;
	    cp = (unsigned char *)info->layInfo.chars->text;
	    wordp = cp;
	    nca = cp + info->layInfo.chars->chunk.used;
	    ncaHit = cp + hitPos - _downP0->c1st;
	    while (cp < nca) {
		trans = TX_NEXTSTATE(state, chClass[*cp++]);
		if (!(state = trans->next)) {
		    state = clickTable;
		    cp -= trans->delta;
		    if (trans->token < 0)
			return self;
		    if (cp > ncaHit)
			break;
		    wordp = cp;
		}
	    }
	    wordPos = wordp - info->layInfo.chars->text;
	    cPos = cp - info->layInfo.chars->text;
	}
	{

	    NXCoord   *pw;
	    NXLay     *pl;
	    NXLay     *plb;
	    int        nciLay;
	    int        cp;
	    int        haveWord;
	    NXCoord    zero;

	    cp = textLength - _downP0->c1st - 1;
	    if (wordPos > cp)
		wordPos = cp;
	    if (cPos > cp)
		cPos = cp;
	    haveWord = 0;
	    pl = info->layInfo.lays->lays;
	    plb = TX_LAYAT(pl, info->layInfo.lays->chunk.used);
	    pw = info->layInfo.widths->widths;
	    cp = 0;
	    zero = 0.0;
	    hitX = info->layInfo.rect.origin.x + pl->x;
	    while (pl < plb) {
		nciLay = pl->chars;
		if (pl->x != zero)
		    hitX = info->layInfo.rect.origin.x + pl->x;
		while (nciLay) {
		    nciLay--;
		    if (cp >= wordPos && !haveWord) {
			haveWord++;
			leftX = hitX;
		    }
		    if (cp >= cPos)
			goto breakWhile;
		    hitX += *pw++;
		    cp++;
		}
		pl++;
	    }
    breakWhile:
	    ;
	}
	{
	    NXSelPt   *downP;

	    downP = _downP0;
	    downP->x = leftX;
	    downP->cp = downP->c1st + wordPos;
	    downP = _downPN;
	    downP->x = hitX;
	    downP->cp = downP->c1st + cPos;
	    info->globals.hitRect.origin.x = leftX;
	    info->globals.hitRect.size.width = hitX - leftX;
	}
    }
    return self;
}

- (int)posToLine:(int) cp sel:(int)sel line:(int)line info:(NXTextInfo *)info
    height:(NXCoord)ht yPos:(NXCoord)yp selection:(NXSelPt *)sp left:(BOOL)left
{
    int nLines, cw = 0;
    NXLineDesc *ps, *oldps = 0, *lps;
    
    ps = theBreaks->breaks;
    lps = TX_LINEAT(ps, theBreaks->chunk.used);
    ps = TX_LINEAT(ps, line);

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
	if (cp > sel || (cp == sel && !left))
	    break;
    }
    yp += (NXCoord)(--nLines) * ht;
    if (sel > cp)
	sel = cp;
    cp -= cw;
    sp->cp = sel;
    sp->line = TX_BYTES(oldps, theBreaks->breaks);
    info->layInfo.lFlags.endsParagraph = 1;
    if ((sp->line = TX_BYTES(oldps, theBreaks->breaks)) && !TXISPARA(*--oldps))
	info->layInfo.lFlags.endsParagraph = 0;
    sp->y = yp;
    sp->c1st = cp;
    sp->ht = ht;
    return (cp);
}
- posToPoint:(int)cp sel:(int)sel line:(int)line info:(NXTextInfo *)info
    height:(NXCoord)ht yPos:(NXCoord)yp selection:(NXSelPt *)sp left:(BOOL)left
{
    NXCoord *pw;
    NXLay *pl, *plb;
    int nciLay;
    NXCoord hitX, zero;

    cp = [self posToLine:cp sel:sel line:line info:info height:ht yPos:yp
	selection:sp left:left];
    NXAdjustTextCache(self, &info->layInfo.cache, cp);
    info->layInfo.rect = bodyRect;
    info->layInfo.rect.origin.y = sp->y;
    info->layInfo.rect.size.height = ht;
    info->globals.gFlags.vertOrHoriz = 0;
    info->globals.gFlags.plusMinus = 0;
    info->layInfo.lFlags.horizCanGrow = info->layInfo.lFlags.vertCanGrow = 0;
    info->layInfo.lFlags.resetCache = 1;
    (void)scanFunc(self, &info->layInfo);
    pl = info->layInfo.lays->lays;
    plb = TX_LAYAT(pl, info->layInfo.lays->chunk.used);
    pw = info->layInfo.widths->widths;
    zero = 0.0;
    hitX = info->layInfo.rect.origin.x + pl->x;
    while (pl < plb) {
	if (pl->x != zero)
	    hitX = info->layInfo.rect.origin.x + pl->x;
	nciLay = pl->chars;
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
    sp->x = floor(hitX);
    return self;
}

- _ptToHit:(NXPoint *)thePoint :(NXSelPt *)_downP0 :(NXSelPt *)_downPN
 /*
  * this is an internal method that given a point returns the x coordinate
  * and line break that are selected.  also returns neighborhood rectangle
  * for determining when mouse has moved away from the vicinity of this
  * point, characterwise. 
  */
{
    NXCoord             hitY = 0.0;
    NXCoord             hitHt = 0.0;
    NXCoord    hitX = 0.0;
    int                 hitPos = 0;
    int                 hitLine;
    int                 pastBottom;
    NXTextInfo *info = _info;
    pastBottom = 0;
    {
	NXCoord    ht = 0;
	NXCoord    yPos;
	NXCoord    testY;
	NXCoord    yOld = 0;

	NXLineDesc *ps;
	int        cPos;
	int        curLine;
	int        cw = 0;
	int        oldLine = 0;
	int        stopLine;
	int                 seenGrow;

	seenGrow = growLine < 0;
	testY = thePoint->y;
	cPos = 0;
	yPos = bodyRect.origin.y;
	curLine = 0;
	ps = theBreaks->breaks;
	stopLine = theBreaks->chunk.used;
	hitLine = -1;
	while (curLine < stopLine) {
	    oldLine = curLine;
	    cw = *(TX_LINEAT(ps, curLine));
	    curLine += sizeof(NXLineDesc);
	    if (cw < 0) {
		ht = TX_HTINFO(ps, curLine)->newHeight;
		curLine += sizeof(NXHeightInfo);
	    }
	    cw &= TXMCHARS;
	    yOld = yPos;
	    yPos += ht;
	    if (oldLine == growLine) {	/* hysteresis */
		if (testY >= (yOld - NX_YHYSTERESIS) && testY <= (yPos + NX_YHYSTERESIS)) {
		    hitLine = oldLine;
		    hitY = yOld;
		    hitHt = ht;
		    hitPos = cPos;
		    break;
		}
		if (hitLine >= 0)
		    break;
		seenGrow++;
	    }
	    if (hitLine < 0 && testY <= yPos) {
		hitLine = oldLine;
		hitY = yOld;
		hitHt = ht;
		hitPos = cPos;
		if (seenGrow)
		    break;
	    }
	    cPos += cw;
	}
	if (hitLine < 0) {
	    hitLine = oldLine;
	    hitY = yOld;
	    hitHt = ht;
	    hitPos = cPos - cw;
	    pastBottom++;
	}
	_downP0->c1st = hitPos;
	info->layInfo.lFlags.endsParagraph = 1;
	if (hitLine && !TXISPARA(*(TX_LINEAT(ps, hitLine
					     - sizeof(NXLineDesc)))))
	    info->layInfo.lFlags.endsParagraph = 0;
    }
    {

	NXCoord   *pw;
	NXCoord    testX;
	NXCoord    cwid;
	int        nciLay;
	NXLay     *pl;
	NXLay     *plb;
	NXCoord    lastCw = 0.0;
	NXCoord    zero;

	NXAdjustTextCache(self, &info->layInfo.cache, hitPos);
	info->layInfo.rect = bodyRect;
	info->layInfo.rect.origin.y = hitY;
	info->layInfo.rect.size.height = hitHt;
	info->globals.gFlags.vertOrHoriz = 0;
	info->globals.gFlags.plusMinus = 0;
	info->layInfo.lFlags.horizCanGrow = info->layInfo.lFlags.vertCanGrow = 0;
	info->layInfo.lFlags.resetCache = 1;
	(void)scanFunc(self, &info->layInfo);

	if (clickCount <= 2) {
	    pl = info->layInfo.lays->lays;
	    plb = TX_LAYAT(pl, info->layInfo.lays->chunk.used);
	    pw = info->layInfo.widths->widths;
	    testX = thePoint->x;
	    if (thePoint->y < bodyRect.origin.y)
		testX = bodyRect.origin.x;
	    if (pastBottom || (thePoint->y > (bodyRect.origin.y + bodyRect.size.height)))
		testX = info->layInfo.rect.origin.x + info->layInfo.rect.size.width;
	    zero = lastCw = 0.0;
	    hitX = info->layInfo.rect.origin.x + pl->x;
	    if (sp0.cp >= 0) {	/* growing selection */
		while (pl < plb) {
		    if (pl->x != zero)
			hitX = info->layInfo.rect.origin.x + pl->x;
		    nciLay = pl->chars;
		    if (textLength == (nciLay + hitPos))	/* don't include final
								 * '\n' */
			nciLay--;
		    while (nciLay) {
			nciLay--;
			cwid = *pw++;
			if (testX < hitX + cwid) {
			    info->globals.hitRect.origin.x = hitX;
			    if (testX - NX_XHYSTERESIS < hitX)
				info->globals.hitRect.origin.x = testX - NX_XHYSTERESIS;
			    info->globals.hitRect.size.width = hitX + cwid;
			    if (testX + NX_XHYSTERESIS > info->globals.hitRect.size.width)
				info->globals.hitRect.size.width = testX + NX_XHYSTERESIS;
			    info->globals.hitRect.size.width -= info->globals.hitRect.origin.x;
			    if (testX >= hitX && clickCount == 1) {
				if (hitLine == anchorL.line &&
				    testX >= anchorL.x - NX_XHYSTERESIS &&
				    testX <= anchorL.x + NX_XHYSTERESIS) {
				    hitPos = anchorL.cp;
				    hitX = anchorL.x;
				    info->globals.hitRect.origin.x = hitX - NX_XHYSTERESIS;
				    info->globals.hitRect.size.width = 2.0 * NX_XHYSTERESIS;
				} else if (hitPos >= anchorR.cp) {
				    hitPos++;
				    hitX += cwid;
				}
			    }
			    goto exitHit1;
			}
			lastCw = cwid;
			hitX += cwid;
			hitPos++;
		    }
		    pl++;
		}
		info->globals.hitRect.origin.x = hitX - lastCw - NX_XHYSTERESIS;
		info->globals.hitRect.size.width = info->layInfo.rect.origin.x +
		  info->layInfo.rect.size.width -
		  info->globals.hitRect.origin.x;
	exitHit1:
		;
	    } else {
		while (pl < plb) {
		    if (pl->x != zero)
			hitX = info->layInfo.rect.origin.x + pl->x;
		    nciLay = pl->chars;
		    if (textLength == (nciLay + hitPos))	/* don't include final
								 * '\n' */
			nciLay--;
		    while (nciLay) {
			nciLay--;
			cwid = *pw++;
			if (testX < hitX + cwid / 2.0) {
			    info->globals.hitRect.origin.x = hitX - lastCw / 2.0;
			    info->globals.hitRect.size.width = (lastCw + cwid) / 2.0;
			    goto exitHit2;
			}
			lastCw = cwid;
			hitX += cwid;
			hitPos++;
		    }
		    pl++;
		}
		info->globals.hitRect.origin.x = hitX - lastCw / 2.0;
		info->globals.hitRect.size.width = info->layInfo.rect.origin.x + info->layInfo.rect.size.width -
		  info->globals.hitRect.origin.x;
	exitHit2:
		;
	    }
	}
	info->globals.lastCharWidth = lastCw;
    }

    {
	NXSelPt   *downP;

	info->globals.hitRect.origin.y = hitY - NX_YHYSTERESIS;
	info->globals.hitRect.size.height = hitHt + (2.0 * NX_YHYSTERESIS);

	downP = _downP0;
	downP->cp = hitPos;
	downP->line = hitLine;
	downP->x = hitX;
	downP->y = info->layInfo.rect.origin.y;
	downP->ht = info->layInfo.rect.size.height;
	*_downPN = *downP;

	if (clickCount > 1) {	/* word or line */
	    if (clickCount == 2 && thePoint->x > NX_X(&info->layInfo.rect) && hitPos &&
		thePoint->x < downP->x) 
		hitPos--;
	    [self clickBounds:info hitPos:hitPos p0:downP pN:_downPN];
	}
    /*
     * here we check the end of paragraph condition. If we are not tracking,
     * we want to slip the hit point back one character before the end of
     * line character.  If are tracking, we want to increase to size of the
     * selection to hit the end of the lay rectangle. if we are tracking and
     * we are dragging selection back over linebreak, we also want to include
     * end of line.
     */
	if ((downP->cp == _downPN->cp) && clickCount <= 2) {
	    NXLineDesc          ps = *(TX_LINEAT(theBreaks->breaks, downP->line));

	    if (downP->cp == (downP->c1st + TXGETCHARS(ps)) && TXISPARA(ps)) {
		if (tFlags.haveDown == SINotTracking) {
		    downP->cp -= 1;
		    _downPN->cp -= 1;
		    downP->x -= info->globals.lastCharWidth;
		} else {
		    if (downP->line < spN.line) {
			downP->cp -= 1;
			downP->x -= info->globals.lastCharWidth;
		    }
		    _downPN->x = NX_MAXX(&info->layInfo.rect);
		    info->globals.hitRect.size.width = _downPN->x - downP->x;
		}
	    }
	}
	downP->x = floor(downP->x);
	_downPN->x = floor(_downPN->x);
    }
    return self;
}


#define SELPOS(x, y) ((width * (y)) + (x))
- _mouseTrack:(NXPoint *)thePoint
 /*
  * <<call>>ed in response to mouseDragged event in the mouseDown modal
  * responder. grows the selection. 
  */
{
    NXSelPt             spMse0;
    NXSelPt             spMseN;
    NXSelPt             oSp0;
    NXSelPt             oSpN;
    char                downState;
    NXCoord    width, p0, pN, m0, mN;
    NXTextInfo *info = _info;

    downState = tFlags.haveDown;

    if ((NXMouseInRect(thePoint, &info->globals.hitRect, YES)))
	goto exitSelf;

    oSp0 = sp0;
    oSpN = spN;

    [self _ptToHit:thePoint :&spMse0:&spMseN];
    width = bounds.size.width;
    p0 = SELPOS(sp0.x, sp0.y);
    pN = SELPOS(spN.x, spN.y);
    m0 = SELPOS(spMse0.x, spMse0.y);
    mN = SELPOS(spMseN.x, spMseN.y);

    growLine = spMse0.line;
    if (tFlags.anchorIs0) {	/* anchored at sp0 */
	if (mN <= p0) {		/* wipes selection (except for anchor) */
	    spN = anchorR;
	    sp0 = spMse0;
	    tFlags.anchorIs0 = NO;
	    [self _hilite:NO :&anchorR:&oSpN:NULL];
	    if (m0 != p0)
		[self _hilite:NO :&spMse0:&oSp0:NULL];
	} else {
	    spN = spMseN;

	/* !!! verify that this works with cross eol selections */

	    if (mN < pN)
		[self _hilite:NO :&spMseN:&oSpN:NULL];
	    else
		[self _hilite:YES :&oSpN:&spN:NULL];
	}
    } else {			/* anchored at spN */
	if (m0 >= pN) {		/* wipes selection (except for anchor) */
	    sp0 = anchorL;
	    spN = spMseN;
	    tFlags.anchorIs0 = YES;
	    [self _hilite:NO :&oSp0:&sp0:NULL];
	    if (mN != pN)
		[self _hilite:YES :&oSpN:&spN:NULL];
	} else {
	    sp0 = spMse0;

	/* !!! verify that this works with cross eol selections */

	    if (mN > p0)
		[self _hilite:NO :&oSp0:&sp0:NULL];
	    else
		[self _hilite:NO :&spMse0:&oSp0:NULL];
	}
    }

    if (FLUSHWINDOW(window))
	PSflushgraphics();

  exitSelf:
    return self;
}

- redrawCells:(int) start: (int)stop
{
    NXTextCache saved, *cache = CACHE(_info);
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXRun *run, *last;
    if (start == stop)
	return self;
    saved = *cache;
    NXAdjustTextCache(self, cache, start);
    run = cache->curRun;
    NXAdjustTextCache(self, cache, stop);
    last = cache->curRun + 1;
    for (; run < last; run++) {
	if (run->rFlags.graphic) {
	    NXTextCellInfo temp, *ptr;
	    NXRect rect;
	    temp.cell = run->info;
	    ptr = NXHashGet(globals->cellInfo, &temp);
	    rect.origin = ptr->origin;
	    [run->info calcCellSize:&rect.size];
	    [run->info drawSelf:&rect inView:self];
	}
    }
    *cache = saved;
    return self;
}


- _hilite:(int)extendCR :(NXSelPt *)_p0 :(NXSelPt *)_pN :(const NXRect *)clipLineRect
 /*
  * you never call this routine.  it hilites selection. 
  */
{
    int lPos0, lPosN, endOfN, existsPlus1;
    NXCoord yPos0, yPosN, xPos0, xPosN, ht0, htN;
    NXLineDesc *ps;
    NXSelPt *p;

    {
	int        lPos1;

	p = _p0;
	lPos0 = p->line;
	ht0 = p->ht;
	yPos0 = p->y;
	xPos0 = p->x;

	p = _pN;
	lPosN = p->line;
	htN = p->ht;
	yPosN = p->y;
	xPosN = p->x;

	ps = TX_LINEAT(theBreaks->breaks, 0);
	lPos1 = (*(TX_LINEAT(ps, lPos0)) > 0) ?
	  lPos0 + sizeof(NXLineDesc) : lPos0 + sizeof(NXHeightChange);
	existsPlus1 = (lPos0 == lPosN || lPos1 == lPosN) ? 0 : 1;
    }
    {
	int        cw;

	cw = TXGETCHARS(*(TX_LINEAT(ps, lPosN)));
	endOfN = ((cw + p->c1st) == p->cp);
    }

    {
	NXRect              eRect;
	char       saveErase;
	int        on;

/* dealing with being at end of line !!! */

	saveErase = vFlags.opaque;
	vFlags.opaque = 1;
	on = extendCR;

	eRect.size.height = ht0;
	eRect.origin.y = yPos0;
	eRect.origin.x = xPos0;
	eRect.size.width = bodyRect.origin.x + bodyRect.size.width - xPos0;
	if (lPos0 == lPosN)
	    eRect.size.width = xPosN - xPos0;
	p->x = NX_MAXX(&eRect);
	if (!clipLineRect || (NXIntersectionRect(clipLineRect, &eRect))) {
// greg bug 8403  p->x = NX_MAXX(&eRect);
	    if (NXIntersectionRect(&bodyRect, &eRect)) {
// greg bug 8403  p->x = NX_MAXX(&eRect);
		NXHighlightRect(&eRect);
	    }
	}
	if (lPos0 != lPosN) {
	    if (existsPlus1) {
		eRect.origin.x = bodyRect.origin.x;
		eRect.size.width = bodyRect.size.width;
		eRect.origin.y = yPos0 + ht0;
		eRect.size.height = yPosN - eRect.origin.y;
		if (!clipLineRect || (NXIntersectionRect(
						   clipLineRect, &eRect))) {
		    if (NXIntersectionRect(&bodyRect, &eRect))
			NXHighlightRect(&eRect);
		}
	    }
	    eRect.origin.x = bodyRect.origin.x;
	    eRect.origin.y = yPosN;
	    eRect.size.height = htN;
	    eRect.size.width = bodyRect.size.width;
	    if (!endOfN || !on)
		eRect.size.width = xPosN - eRect.origin.x;
	    p->x = NX_MAXX(&eRect);
	    if (!clipLineRect || (NXIntersectionRect(clipLineRect, &eRect))) {
		if (NXIntersectionRect(&bodyRect, &eRect))
		    NXHighlightRect(&eRect);
	    }
	}
	vFlags.opaque = saveErase;
    }
    [self redrawCells:_p0->cp:_pN->cp];
    return self;
}



- _setSel:(int)start :(int)end
 /*
  * You never <<call>> this method.  It computes the necessary internal
  * information to record the new selection range. <<Call>> setSel to set
  * the selection range. This method does not do any displaying. 
  */
{
    int                 sel;
    id                  retval = nil;
    NXLineDesc *ps;
    NXLineDesc *lps;
    NXTextInfo *info = _info;

    PROLOG(info);

    if (end >= textLength)
	end = textLength - 1;
    if (start >= textLength)
	start = textLength - 1;
    sel = start;
    if (end < sel)
	end = sel;
    if ((sp0.cp = sel) < 0)
	goto nope;
    if (end > textLength)
	end = textLength;
    ps = TX_LINEAT(theBreaks->breaks, 0);
    lps = TX_LINEAT(ps, theBreaks->chunk.used);
    [self posToPoint:0 sel:sel line:0 info:info height:0.0 
	yPos:bodyRect.origin.y selection:&sp0 left:YES];
    if (start == end) {
	spN = sp0;
    } else {
	ps = TX_LINEAT(ps, sp0.line);
	sel = end;
	[self posToPoint:sp0.c1st sel:end line:sp0.line info:info height:sp0.ht 
	    yPos:sp0.y selection:&spN left:NO];
    }
    growLine = spN.line;
    tFlags.anchorIs0 = YES;
    _NXClearClickState(self);
    retval = self;
  nope:
    EPILOG(info);
    return retval;
}

- _setSelPrologue: (BOOL) isCaret
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);

    if (self == [window firstResponder])
	[self selectNull];
    else {
	sp0.cp = -1;
	if (window && ![window makeFirstResponder:self]) {
	    return nil;	/* error */
	}
    }
    if (!isCaret)
	globals->caretXPos = -1.0;
    typingRun.chars = 0;
    return self;
}

- _setSelPostlogue
{
    BOOL                canDraw = [self _canDraw];
    NXTextGlobals *globals = TEXTGLOBALS(_info);

    if (canDraw) {
	[self lockFocus];
	[self _hilite:YES :&sp0:&spN:NULL];
	if (FLUSHWINDOW(window))
	    PSflushgraphics();
	if (sp0.cp >= 0)
	    [self showCaret];
	[self unlockFocus];
	if (!tFlags.monoFont && [window isKeyWindow]) {
	    [self _fixFontPanel];
	    if (HAVE_RULER(_info))
		[self updateRuler];
	}
    }
    if (!globals->gFlags.movingCaret)
	globals->caretXPos = -1.0;
    return self;
}

- _setSel:(NXSelPt *)sel
 /*
  * This establishes an exact caret selection for arrow movements.  this
  * allows specifying whether a end of line selection goes with prior or
  * following line by taking a NXSelPt instead of computing NXSelPt from
  * character position.  Otherwise, this routines is like setSel::, NOT
  * _setSel::!  EG, it draws as necessary.
  */
{
    [self _setSelPrologue : YES];
    sp0 = *sel;
    spN = *sel;
    growLine = spN.line;
    tFlags.anchorIs0 = YES;
    _NXClearClickState(self);
    [self _setSelPostlogue];
    return self;
}


@end


/*

Modifications (starting at 1.0a):

 3/20/90 bgy	Added support for ruler. This involves adding a new 
 		 category of ScrollView that interacts with the Text 
		 object. The toggleRuler: method toggles the ruler in
		 the scrollview;
83
--
 4/25/90 bgy	fixed selectParagraph to hide caret if necessary. changed both
 		 posToPoint & posToLine to take an extra arg that specifies
		 whether this is the left or right part of the selection.

84
--
 5/04/90 chris	Modified _ptToHit: to deal with dragging backwards across
 		line break.
 5/08/90 chris	Added _setSelPrologue,_setSelPostlogue,_setSel:.  Goal is to
 		allow selecting programatically end of prior line || start of
		following line so that arrow keys can work. setSel:: always
		selects start of following line.
 5/11/90 chris  removed parameter to _fixFontPanel: and remove use of bucky
		bits to suppress setting fontPanel. as per discussion with
		bryan, Copy Font removes necessity for this "feature" which
		interfers with mixing clicking and command key setting of
		bold, etc.
		
91
--
 8/11/90 glc	changed ruler instance variable to _ruler
 		Somehow toggleRuler got all messed up in a merge. Fixed this.
		

94
--
 9/25/90 gcockrft	Yanked ruler method. Added isRulerVisible.
 
 */






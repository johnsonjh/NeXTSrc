/*
	textEdit.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Text.h"
#import "textprivate.h"
#import "errors.h"
#import "textWraps.h"
#import <dpsclient/wraps.h>
#import <string.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryEdit=0\n");
    asm(".globl .NXTextCategoryEdit\n");
#endif

@implementation Text(Edit)

- changeRunFrom:(int) cp0 to:(int)cpN selector:(SEL)aSel with:(void *)data
{
    NXRun *curRun, *insp, leftIns[3], rightIns[3];
    int runPos, nchars, rpl0, rplN, used;
    int rpos0, rposN, newUsed, amt;
    NXTextInfo *info = _info;

    cpN--;
    if (cp0 > cpN)
	return self;
    NXSetTextCache(self, &info->layInfo.cache, cp0);
    runPos = info->layInfo.cache.runFirstPos;
    curRun = info->layInfo.cache.curRun;

    rpos0 = TX_BYTES(curRun, theRuns->runs);
    used = theRuns->chunk.used;

    nchars = curRun->chars;
    if (rpos0 == (used - sizeof(NXRun)) && cpN >= (runPos + nchars))
	cpN = runPos + nchars - 1;
    insp = leftIns;
    if (cp0 != runPos) {
	*insp = *curRun;
	insp->chars = cp0 - runPos;
	insp++;
    }
    *insp = *curRun;
    [self _convertRun:insp selector:aSel with:data];
    insp->chars = (cpN < (runPos + nchars)) ? 
	(cpN + 1 - cp0) : (runPos + nchars - cp0);
    insp++;
    if (cpN < (runPos + nchars - 1)) {
	*insp = *curRun;
	insp->chars = runPos + nchars - cpN - 1;
	insp++;
    }
    rpl0 = TX_BYTES(insp, leftIns);
    rplN = 0;
    if (cpN >= (runPos + nchars)) {
	rposN = rpos0 + sizeof(NXRun);
	curRun++;
	runPos += nchars;
	nchars = curRun->chars;
	if (rposN == (used - sizeof(NXRun)) && cpN >= (runPos + nchars))
	    cpN = runPos + nchars - 1;

	while (cpN >= runPos + nchars) {
	    [self _convertRun:curRun selector:aSel with:data];
	    runPos += nchars;
	    curRun++;
	    rposN += sizeof(NXRun);
	    nchars = curRun->chars;
	    if (rposN == (used - sizeof(NXRun)) && cpN >= (runPos + nchars)) {
		cpN = runPos + nchars - 1;
		break;
	    }
	}
	insp = rightIns;
	*insp = *curRun;
	[self _convertRun:insp selector:aSel with:data];
	insp->chars = cpN - runPos + 1;
	insp++;
	if (cpN < (runPos + nchars - 1)) {
	    *insp = *curRun;
	    insp->chars = runPos + nchars - cpN - 1;
	    insp++;
	}
	rplN = TX_BYTES(insp, rightIns);
    }
    rposN = TX_BYTES(curRun, theRuns->runs);
    newUsed = used + rpl0 + rplN - (rplN ? 2 * sizeof(NXRun) : sizeof(NXRun));
    if (newUsed > theRuns->chunk.allocated)
	theRuns = (NXRunArray *)NXChunkGrow(
					      &theRuns->chunk, newUsed);
    theRuns->chunk.used = newUsed;
    curRun = theRuns->runs;
    if ((amt = (used - rposN - sizeof(NXRun))) > 0)
	bcopy(
	      TX_RUNAT(curRun, used - amt),
	      TX_RUNAT(curRun, newUsed - amt), amt);
    if (rplN) {
	bcopy(rightIns, TX_RUNAT(curRun, rposN + rpl0 - sizeof(NXRun)), rplN);
	if ((amt = rposN - rpos0 - sizeof(NXRun)) > 0)
	    bcopy(
		  TX_RUNAT(curRun, rpos0 + sizeof(NXRun)),
		  TX_RUNAT(curRun, rpos0 + rpl0), amt);
    }
    bcopy(leftIns, TX_RUNAT(curRun, rpos0), rpl0);
    if (rpl0)
	rpl0 -= sizeof(NXRun);
    rposN += newUsed - used + sizeof(NXRun);
    if (rposN >= newUsed)
	rposN -= sizeof(NXRun);

    {
	NXRun *lastRun, *copyRun;
	int runLength;

	lastRun = TX_RUNAT(curRun, rposN);
	curRun = TX_RUNAT(curRun, rpos0);
	copyRun = curRun;
	runLength = 0;
	while (++curRun <= lastRun) {
	    if (EQUALRUN(copyRun, curRun))
		runLength += curRun->chars;
	    else {
		copyRun->chars += runLength;
		if (++copyRun != curRun) {
		    runLength = 0;
		    *copyRun = *curRun;
		}
	    }
	}
	copyRun->chars += runLength;
	if (amt = TX_BYTES(lastRun, copyRun)) {
	    theRuns->chunk.used -= amt;
	    if ((amt = newUsed - TX_BYTES(curRun, theRuns->runs)) > 0)
		bcopy(curRun, copyRun + 1, amt);
	}
    }
    return self;
}
- replaceRangeFrom:(int)cp0 to:(int)cpN selector:(SEL)aSel with:(void *)data
{
    NXRect boundsMax, boundsSave;
    NXSelPt synch, ins, end;
    NXCoord bottom, bailHt, bailY, lastHt, top, mbottom;
    int bailEndPos, bailLine, bailChars, synchChars = 0;
    NXLineDesc *ps, *sps;
    int lastCPos, saveLine, curLine, allocated, oldUsed;
    int clipped, saveSel0, saveSelN, cw;
    NXChunk   *ta;
    int needs, saveAllocated, backedUp, backWidth = 0;
    int cp, tooWide, hitBottom;
    BOOL canDraw = [self _canDraw];
    BOOL canFlush = FLUSHWINDOW(window);
    NXTextInfo *info = _info;

    if (info->globals.inReplaceSel)
	return self;
    else 
	info->globals.inReplaceSel++;
    boundsSave = bounds;
    boundsMax = boundsSave;
    NXAdjustTextCache(self, &info->layInfo.cache, cp0);
    saveSel0 = sp0.cp;
    saveSelN = spN.cp;
    [self posToPoint:0 sel:cp0 line:0 info:info height:0.0 
	yPos:bodyRect.origin.y selection:&ins left:YES];
    if (cp0 == cpN)
	end = ins;
    else 
	[self posToPoint:ins.c1st sel:cpN line:ins.line info:info height:ins.ht 
	    yPos:ins.y selection:&end left:NO];
    if (saveSel0 >= 0 && spN.line >= ins.line) {
	sp0.cp = -1;
	if (ins.line > sp0.line)
	    ins = sp0;
	if (end.line < spN.line)
	    end = spN;
    }
    cw = *(TX_LINEAT(theBreaks->breaks, end.line));
    bailEndPos = end.c1st + TXGETCHARS(cw);
    bailLine = end.line;
    bailHt = end.ht;
    bailChars = cw;
    bailY = end.y;
    _NXPosNewline(self, info, &end, &synch);
    if (synch.cp >= 0)
	synchChars = *(TX_LINEAT(theBreaks->breaks, synch.line));
    [self changeRunFrom:cp0 to:cpN selector:aSel with:data];
    if (delegateMethods & TXMWILLGROW) 
	[delegate textWillResize:self];
    info->layInfo.lFlags.horizCanGrow = tFlags.horizResizable;
    info->layInfo.lFlags.vertCanGrow = tFlags.vertResizable;

    mbottom = bodyRect.origin.y + bodyRect.size.height;
    info->globals.gFlags.clipRight = 0;
    info->globals.gFlags.clipLeft = 0;
    info->layInfo.lFlags.erase = 1;
    info->layInfo.lFlags.ping = 0;
    oldUsed = theBreaks->chunk.used;
    clipped = 0;
    while (1) {
	info->globals.savedBreaks->chunk.used = 0;
	info->layInfo.cache.curRun = theRuns->runs;
	info->layInfo.cache.runFirstPos = 0;
	info->layInfo.rect = bodyRect;
	info->layInfo.rect.origin.y = ins.y;
	lastHt = -1;
	if (curLine = ins.line) {
	    lastHt = ins.ht;
	    ps = TX_LINEAT(theBreaks->breaks, curLine);
	    if (*ps < 0)
		lastHt = HTCHANGE(ps)->heightInfo.oldHeight;
	}
	info->layInfo.rect.size.height = ins.ht;
	info->globals.gFlags.vertOrHoriz = 0;
	info->globals.gFlags.plusMinus = 0;
	lastCPos = ins.c1st;
	hitBottom = 0;

	backedUp = 0;
	if (curLine) {
	    ps = TX_LINEAT(
		       theBreaks->breaks, curLine - sizeof(NXLineDesc));
	    if (!TXISPARA(*ps)) {
		backedUp++;
		info->layInfo.rect.origin.y -= lastHt;
		if (ps[1] < 0)
		    info->layInfo.rect.size.height =
		      HTCHANGE(ps + 1)->heightInfo.oldHeight;
		backWidth = *ps;
		if (backWidth < 0) {
		    ps = TX_LINEAT(ps, -sizeof(NXHeightInfo));
		    lastHt = HTCHANGE(ps)->heightInfo.oldHeight;
		    curLine -= sizeof(NXHeightChange);
		} else {
		    curLine -= sizeof(NXLineDesc);
		}
		lastCPos -= (backWidth = TXGETCHARS(backWidth));
	    }
	}
	info->layInfo.lFlags.endsParagraph = 1;
	if (curLine &&
	    !TXISPARA(*(TX_LINEAT(theBreaks->breaks,
				  curLine - sizeof(NXLineDesc)))))
	    info->layInfo.lFlags.endsParagraph = 0;
    /*
     * make this test go against the sect of bodyRect's bottom & pane's
     * edge 
     */
	bottom = _NXSetDrawBounds(self, &top);
	allocated = theBreaks->chunk.allocated;
	saveLine = 0;
	saveAllocated = info->globals.savedBreaks->chunk.allocated;
	(void)NXAdjustTextCache(self, &info->layInfo.cache, lastCPos);
	info->layInfo.lFlags.resetCache = 1;
	while (1) {
	    tooWide = scanFunc(self, &info->layInfo);
	    info->layInfo.lFlags.resetCache = 0;
	    if ((cw = info->layInfo.cache.curPos - lastCPos) > TXMCHARS)
		NX_RAISE(NX_longLine, 0, 0);
	    if (tFlags.horizResizable || tFlags.vertResizable) {
		if ([self perLineGrow:info maxBounds:&boundsMax]) {
		/* redo bottom */
		    bottom = _NXSetDrawBounds(self, &top);
		    mbottom = bodyRect.origin.y + bodyRect.size.height;
		    if (clipped) {
			clipped = 0;
			if (canDraw)
			    PSgrestore();
		    }
		}
	    }
	    needs = sizeof(NXLineDesc);
	    if (info->layInfo.rect.size.height != lastHt)
		needs += sizeof(NXHeightInfo);
	    if ((saveLine + needs) >= saveAllocated) {
		info->globals.savedBreaks->chunk.used = saveLine;
	    /* depends on error handler */
		info->globals.savedBreaks = (NXBreakArray *)
		  (ta = NXChunkRealloc(&info->globals.savedBreaks->chunk));
		saveAllocated = ta->allocated;
	    }
	    if ((curLine + needs) >= allocated) {
		theBreaks->chunk.used = curLine;
	    /* depends on error handler */
		theBreaks = (NXBreakArray *)
		  (ta = NXChunkRealloc(&theBreaks->chunk));
		allocated = ta->allocated;
	    }
	    ps = TX_LINEAT(theBreaks->breaks, curLine);
	    if (info->layInfo.lFlags.endsParagraph)
		cw |= TXMPARA;
	    if (needs == sizeof(NXLineDesc)) {
		sps = TX_LINEAT(info->globals.savedBreaks->breaks, saveLine);
		*sps = *ps;
		*ps = cw;
	    } else {
		NXHeightChange     *htChange = (NXHeightChange *)ps;
		NXHeightChange *sht;
		
		sht = TX_HTCHANGE(info->globals.savedBreaks->breaks, saveLine);
		*sht = *htChange;
		htChange->lineDesc =
		  htChange->heightInfo.lineDesc = TXYESCHANGE(cw);
		htChange->heightInfo.oldHeight = lastHt;
		lastHt = info->layInfo.rect.size.height;
		htChange->heightInfo.newHeight = lastHt;
	    };
	    if (NX_MAXY(&info->layInfo.rect) > top) {
		if (NX_Y(&info->layInfo.rect) < bottom) { /* still visible */
		    if (!backedUp || 
		    (backWidth != (info->layInfo.cache.curPos - lastCPos))) {
			if (!clipped && (tooWide ||
			    NX_MAXY(&info->layInfo.rect) > mbottom)) {
			    clipped++;
			    if (canDraw) {
				PSgsave();
				PSrectclip(
				   bounds.origin.x, bounds.origin.y,
				 bounds.size.width, bounds.size.height);
			    }
			}
			if (canDraw)
			    (void)drawFunc(self, &info->layInfo);
		    }
		} else	/* not visible, if we've wrapped to synch,
			     * then we're done! */
		    hitBottom++;
	    }
	    backedUp = 0;

	    lastCPos = info->layInfo.cache.curPos;
	    curLine += needs;
	    saveLine += needs;

	    cp = NXAdjustTextCache(self, &info->layInfo.cache, lastCPos);
	    if ((curLine - needs) == bailLine && bailEndPos == lastCPos) {
		if (lastHt == bailHt && info->layInfo.rect.origin.y == bailY &&
		    ((needs == sizeof(NXLineDesc) && bailChars >= 0) ||
		     (needs == sizeof(NXHeightChange) && bailChars < 0)))
		    goto bailedOut;
	    }
	    info->layInfo.rect.origin.y += lastHt;

	    if (cp < 0 ||
		(hitBottom && synch.cp >= 0 && lastCPos >= synch.cp)) {
		theBreaks->chunk.used = curLine;
		info->globals.savedBreaks->chunk.used = saveLine;
		goto doneWrapping;
	    }
	}
	info->layInfo.lFlags.resetCache = 1;
    }
  doneWrapping:
    ;

 /*
  * here we patch the remainder of theBreaks & info->globals.savedBreaks into
  * theBreaks after wrapped part. we also erase anything necessary at bottom
  * (if the whole mess has shrunk) 
  */
    {
	int        used;

	int        amt;
	int                 delta;
	int                 curLineOld;
	int                 offset;
	NXCoord    y;
	NXCoord    deltaY;
	int                 isNewHt;
	NXRect	erase;

	y = info->layInfo.rect.origin.y;
	if (synch.cp >= 0) {
	    curLineOld = curLine;
	    ps = TX_LINEAT(theBreaks->breaks, curLine);
	    while (lastCPos > synch.cp) {
		y -= lastHt;
		amt = *--ps;
		if (amt < 0) {
		    curLine -= sizeof(NXHeightChange);
		    ps = TX_LINEAT(ps, -sizeof(NXHeightInfo));
		    lastHt = HTCHANGE(ps)->heightInfo.oldHeight;
		} else {
		    curLine -= sizeof(NXLineDesc);
		}
		lastCPos -= TXGETCHARS(amt);
	    }
	    delta = 0;
	    if ((isNewHt = (lastHt != synch.ht)) && synchChars >= 0)
		delta += sizeof(NXHeightInfo);
	/*
	 * newsize = untouched breaks + lines up to new break for synch.cp
	 * possible NXHeightChange for synch) + saved breaks 
	 */

	    used = oldUsed - synch.line + curLine + delta;
	/* depends on error handler */
	    if (used > allocated)
		theBreaks = (NXBreakArray *)
		  NXChunkGrow(&theBreaks->chunk, used);
	    theBreaks->chunk.used = used;
	    ps = TX_LINEAT(theBreaks->breaks, 0);

	/*
	 * copy up/down untouched breaks 
	 */
	    if (used != oldUsed) {
		if (oldUsed > curLineOld) {
		    offset = curLineOld;
		    if (curLineOld < synch.line)
			offset = synch.line;
		    if ((amt = oldUsed - offset) > 0) {
			bcopy(TX_LINEAT(ps, offset),
			      TX_LINEAT(ps, used - amt),
			      amt);
		    }
		} else {
		    amt = curLineOld - oldUsed;
		    curLineOld -= amt;
		    saveLine -= amt;
		}
	    }
	/*
	 * copy over saved breaks from info->globals.savedBreaks 
	 */
	    amt = curLineOld - synch.line -
	      ((synchChars < 0) ? sizeof(NXHeightChange) : sizeof(NXLineDesc));
	    if (amt > 0)
		bcopy(TX_LINEAT(info->globals.savedBreaks->breaks, 
		    saveLine - amt), TX_LINEAT(ps, curLine +
		    ((isNewHt || synchChars < 0) ?
		    sizeof(NXHeightChange) : sizeof(NXLineDesc))), amt);

	/*
	 * fill in break for synch line 
	 */
	    ps = TX_LINEAT(ps, curLine);
	    if (isNewHt || synchChars < 0) {
		synchChars |= TXMCHANGE;
		HTCHANGE(ps)->lineDesc = synchChars;
		HTCHANGE(ps)->heightInfo.lineDesc = synchChars;
		HTCHANGE(ps)->heightInfo.oldHeight = lastHt;
		HTCHANGE(ps)->heightInfo.newHeight = synch.ht;
	    } else {
		synchChars &= ~TXMCHANGE;
		*ps = synchChars;
	    }
	}
	if ((deltaY = synch.y - y) > 0) {	/* have to erase at bottom */
	    y = maxY + bodyRect.origin.y - deltaY;
	    if (y < bottom && canDraw) {
		NXSetRect(&erase, bounds.origin.x, y,
		    bounds.size.width, ((y + deltaY > bottom) ? 
		    bottom - y : deltaY));
		_NXTextBackgroundRect(self, &erase, NO, [self shouldDrawColor]);
		[window flushWindow];
	    }
	}
	maxY -= deltaY;
    }

  bailedOut:

    {
	int        mustGrow;

	mustGrow = tFlags.horizResizable || tFlags.vertResizable;

	if (saveSel0 >= 0 && sp0.cp < 0) {
	    [self _setSel:saveSel0 :saveSelN];
	    if (saveSel0 != saveSelN) {
		[self _hilite:YES :&sp0:&spN:&bodyRect];
		if (canFlush && canDraw)
		    PSflushgraphics();
	    }
	}
	if (clipped > 0 && canDraw)
	    PSgrestore();
	if (canDraw) {
	    if (canFlush)
		PSflushgraphics();
	    [self unlockFocus];
	}
	if (mustGrow)
	    [self oncePerGrow:&boundsSave maxBounds:&boundsMax];
    }
    info->globals.inReplaceSel--;
    return self;
}

- replaceRunFrom:(int)cp0 to:(int)cpN selector:(SEL)aSel with:(void *)data
{
    NXTextInfo *info = _info;
    BOOL canDraw = [self _canDraw];
    PROLOG(info);
    [self suspendNotifyAncestorWhenFrameChanged:YES];
    if (canDraw)
	_NXResetDrawCache(self);
    if (textLength == 1 || tFlags.monoFont) {
	[self _convertRun:&theRuns->runs[0] selector:aSel with:data];
	[self setFont: theRuns->runs[0].font];
    } else if (cp0 == cpN) {
	if (sp0.cp == spN.cp && sp0.cp == cp0) 
	    [self _changeTypingRun:aSel with:data];
    } else {
	if (canDraw) {
	    [self _bringSelToScreen:VISBIGFUDGE];
	    [self lockFocus];
	    _NXResetDrawCache(self);
	}
	[self replaceRangeFrom:cp0 to:cpN selector:aSel with:data];
    }
    [self suspendNotifyAncestorWhenFrameChanged:NO];
    EPILOG(info);
    return self;
}


- _replaceRunFrom:(int)cp0 to:(int)cpN selector:(SEL)aSel with:(void *)data
{
    return [self replaceRunFrom:cp0 to:cpN selector:aSel with:data];
}


@end


/* 
83
--
  4/25/90 bgy	use new API for posToPoint:...

87
--
 7/12/90 glc	added _NXTextBackgroundRect utility call for erasing 
		rectangles.

91
--
 8/12/90 aozer	Changed isOnColorScreen -> shouldDrawColor

*/





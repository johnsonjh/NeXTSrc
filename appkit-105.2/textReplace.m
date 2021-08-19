/*
	textReplace.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Text.h"
#import "errors.h"
#import "textprivate.h"
#import "textWraps.h"
#import <string.h>
#import <stdlib.h>
#import <dpsclient/wraps.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryReplace=0\n");
    asm(".globl .NXTextCategoryReplace\n");
#endif

extern NXHashTablePrototype cellProto;

@implementation Text (Replace)

- registerCellInfo:(NXRunArray *)insertRun
{
    NXRun *run, *last;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXZone *zone = [self zone];

    if (!insertRun)
	return self;
    run = insertRun->runs;
    last = TX_RUNAT(run, insertRun->chunk.used);
    for (; run < last; run++) {
	if (run->rFlags.graphic) {
	    NXTextCellInfo temp, *ptr;
	    if (!globals->cellInfo)
		globals->cellInfo = NXCreateHashTableFromZone(
		    cellProto, 0, 0, zone);
	    temp.cell = run->info;
	    if (NXHashGet(globals->cellInfo, &temp))
		continue;
	    ptr = (NXTextCellInfo *)NXZoneMalloc(zone, sizeof(NXTextCellInfo));
	    ptr->cell = run->info;
	    NXHashInsert(globals->cellInfo, ptr);
	}
    }
    return self;
}

- (int)_replaceSel:pboardRep :
    (const char *)insertText :
    (int)iLen :
    (NXRunArray *)insertRun :
    (BOOL)abortable :
    (BOOL)smartCut :
    (BOOL)smartPaste
{
    int newSelN, saveTFirstPos, bailEndPos, bailLine, bailChars;
    int synchChars = 0, saveLine, curLine, allocated, oldUsed, insChars;
    int clipped, pingAgain, errorNumber, lastCPos;
    NXRect boundsMax, boundsSave;
    NXTextBlock *saveBlock;
    NXSelPt synch, ins;
    NXCoord bottom, bailHt, bailY, lastHt;
    NXLineDesc *ps;
    NXBreakArray *saved;
    BOOL canDraw = [self _canDraw], sawEnd = NO;
    BOOL wasBlinking = (tFlags._caretState != CARETDISABLED);
    BOOL canFlush = FLUSHWINDOW(window), canBail = YES;
    register NXTextInfo *info = _info;
    NXLayInfo *layInfo = &info->layInfo;
    NXTextCache *cache = &layInfo->cache;

    if (info->globals.inReplaceSel)
	return 0;
    else 
	info->globals.inReplaceSel++;
    info->globals.caretXPos = -1.0;
    PROLOG(info);
    if (wasBlinking)
	[self hideCaret];
    [self registerCellInfo:insertRun];  /* check for new graphic types */
    clipped = 0;
    layInfo->lFlags.horizCanGrow = layInfo->lFlags.vertCanGrow = 0;
    errorNumber = 0;
    [self suspendNotifyAncestorWhenFrameChanged:YES];
    if (canDraw)
	_NXResetDrawCache(self);

    {
	register int        cw;

    /* prevent deletion of trailing newline */
	if (spN.cp == textLength) {
	    spN.cp--;
	    if (sp0.cp > spN.cp)
		sp0.cp--;
	}
	if (smartCut)
	    [self smartSelect:info];
	NXAdjustTextCache(self, cache, sp0.c1st);
	
/*	save block ptr and offset to reset TextCache after abort */

	if (saveBlock = cache->curBlock->prior)
	    saveTFirstPos =
	      cache->blockFirstPos - saveBlock->chars;
	else {
	    saveTFirstPos = 0;
	    saveBlock = cache->curBlock;
	}
	
/*	ins is edge of selection where insertion will occur.  this is
 *	normally the right edge spN, unless we have zero length selection
 *      spanning end of previous line and start of next. bail* are variables
 *      cached so that we can detect an operation that only affects one line.
 */
	ins = (spN.line != sp0.line && sp0.cp == spN.cp) ? spN : sp0;
	cw = *(TX_LINEAT(theBreaks->breaks, ins.line));
	bailEndPos = ins.c1st + TXGETCHARS(cw);
	bailLine = ins.line;
	bailHt = ins.ht;
	bailChars = cw;
	bailY = ins.y;
	
/*	synch is edge of line following first newline below right edge of 
 *	selection. synchChars is first word of breakTable for synch line. 
 */
	_NXPosNewline(self, info, &spN, &synch);
	if (synch.cp >= 0)
	    synchChars = *(TX_LINEAT(theBreaks->breaks, synch.line));
	    
/*	apply textFilterFunc to all text entered by typing (abortable == YES).
 *	modify insertRun (only one when typing) as necessary.
 */
	if (textFilterFunc && abortable) {
	    insertText = (char *)(*textFilterFunc) (
			  self, (unsigned char *)insertText, &iLen, sp0.cp);
	    insertRun->runs[0].chars = iLen;
	}
	
/*	modify text blocks to replace selection with insertText. note, iLen can
 *	be negative if backspaces typed.  modify run array to reflect run(s) of
 *	inserted text, deletion of selection and insertion of insertText. at this
 *      point, text blocks hold new text, runs reflect new text, but breakTable
 *      reflects old text.  NOTE, iLen can be modified by replaceRangeAt to
 * 	smart cut or paste.
 *      !!! errorNumber is hysterical baggage and should be zapped.
 */
	newSelN = [self replaceRangeAt:sp0.cp end:spN.cp info:info
	      text:insertText length:&iLen runs:insertRun
	      smartPaste:smartPaste error:&errorNumber];
	if (errorNumber)
	    goto bailedOut;
	    
/*	insChars is net change in characters. */

	insChars = sp0.cp - spN.cp + iLen;
	
/* 	prevents drawAProc from hiliting old selection */
	sp0.cp = -1;		
    }
    {
	int                 needs, saveAllocated, backedUp;
	int		    priorBackedUp,newPriorBackedUp;
	int                 backWidth = 0, cp, tooWide;
	int                 hitBottom, cw;

	register NXCoord    mbottom, top;
	NXCoord             _top;
	NXSelPt             saveIns;
	NXRect             *layRect;

/*	cache various info used below for future reference, resetting after
 *	abort or testing.  certain scanALine,drawALine comm. vars that won't
 *	change are set.
 *	    saveIns is used to reset ins on abort
 *	    mbottom is used to test for exceeding bounds of text object
 *	    clipRight & leftLeft tell drawALine whether left & right of line
 *		are off screen (an optimization not used currently)
 *          erase tells drawALine to erase before drawing line
 *	    oldUsed remembers size of old breakTable.
 *	    boundsSave & boundsMax are used to record original and maximum
 *	    	bounds of text object for refreshing display at exit
 *	    horizCanGrow and vertCanGrow control whether scanALine will compute
 *		line layout knowing that bounds of text can grow or not
 *	    layRect points to rectangle used to hold bounds of line being layed out
 *	    saved points to array into which old contents of breakTable will be
 *		saved.  these old contents are used for 2 purposes: fixup after
 *		abortion and construction of final breakTable at exit.
 */
	saveIns = ins;
	mbottom = bodyRect.origin.y + bodyRect.size.height;
	info->globals.gFlags.clipRight = 0;
	info->globals.gFlags.clipLeft = 0;
	layInfo->lFlags.erase = 1;
	oldUsed = theBreaks->chunk.used;
	boundsSave = bounds;
	boundsMax = boundsSave;
	if (delegateMethods & TXMWILLGROW)
	    [delegate textWillResize:self];
	layInfo->lFlags.horizCanGrow = tFlags.horizResizable;
	layInfo->lFlags.vertCanGrow = tFlags.vertResizable;
	layRect = &layInfo->rect;
	saved = info->globals.savedBreaks;
	priorBackedUp = newPriorBackedUp = 0;
	while (1) {
	
/*	    This loop is executed once per abort.  At this point we (again)
 *	    have a valid set of text blocks and runs, with the breakTable
 *	    containing its original contents. here we :
 *		reset saved array to be empty
 *		reset textCache
 *		reset layRect to rectangle of ins's line
 *		reset lastHt to height of line preceding ins
 */
 
	    saved->chunk.used = 0;
	    cache->blockFirstPos = saveTFirstPos;
	    cache->curBlock = saveBlock;
	    cache->curRun = theRuns->runs;
	    cache->runFirstPos = 0;
	    *layRect = bodyRect;
	    NX_Y(layRect) = ins.y;
	    lastHt = -1.0;
	    if (curLine = ins.line) {
		lastHt = ins.ht;
		ps = TX_LINEAT(theBreaks->breaks, curLine);
		if (*ps < 0)
		    lastHt = HTCHANGE(ps)->heightInfo.oldHeight;
	    }
	    NX_HEIGHT(layRect) = ins.ht;
	    
/*	    vertOrHoriz & plusMinus control left to right, top to bottom
 *	    layout.  these values are for european languages.
 */
	    info->globals.gFlags.vertOrHoriz = 0;
	    info->globals.gFlags.plusMinus = 0;
	    
/*	    lastCPos is start of current line being laid out.
 *	    hitBottom becomes true when we've drawn last visible line.
 */
	    lastCPos = ins.c1st;
	    hitBottom = 0;
	    
/*	    decide whether we need to backup a line to detect flow back (as
 *	    when a break character is typed or backspace).  backepUp is true
 *	    if CURRENT line being laid out is a backed up to line.  we back
 *          up unless this is first line or previous line ended in a newline.
 *	    if we back up, we set backedUp and
 *		set layRect to previous line's origin and height
 *		set backWidth to first word of breakTable entry of previous 
 *		line back up curLine, lastCPos and lastHt
 */
	    backedUp = 0;
	    if (curLine) {
		ps = TX_LINEAT(
		    theBreaks->breaks, curLine - sizeof(NXLineDesc));
		if (!TXISPARA(*ps)) {
		    backedUp++;
		    NX_Y(layRect) -= lastHt;
		    if (ps[1] < 0)
			NX_HEIGHT(layRect) =
			  HTCHANGE(ps + 1)->heightInfo.oldHeight;
		    backWidth = *ps;
		    if (backWidth < 0) {
			ps = TX_LINEAT(ps, (-sizeof(NXHeightInfo)));
			lastHt = HTCHANGE(ps)->heightInfo.oldHeight;
			curLine -= sizeof(NXHeightChange);
		    } else
			curLine -= sizeof(NXLineDesc);
		    lastCPos -= (backWidth = TXGETCHARS(backWidth));
		}
	    }
	    
/*	    set endsParagraph if this is first line or previous line ended in 
 *	    newline.  scanALine uses this for paragraph indentation.
 */
 
	    layInfo->lFlags.endsParagraph = 1;
	    if (curLine &&
		!TXISPARA(*(TX_LINEAT(theBreaks->breaks, curLine -
				      sizeof(NXLineDesc)))))
		layInfo->lFlags.endsParagraph = 0;

/*	    compute top and bottom of visible region */
	    bottom = _NXSetDrawBounds(self, &_top);
	    top = _top;
	    
/*	    allocated is bytes allocated in breakTable, used for testing
 *	    whether we have to grow breakTable.  saveAllocated is same for
 *	    save break table.  saveLine is curLine equivalent for save
 *	    table.
 */
 
	    allocated = theBreaks->chunk.allocated;
	    saveLine = 0;
	    saveAllocated = saved->chunk.allocated;
	    
	    NXAdjustTextCache(self, cache, lastCPos);
	    pingAgain = 0;
	    
/*	    !!! I've looked at this and I can figure out what resetCache's
 *	    !!! purpose is.  Must talk to Bryan.
 */
	    layInfo->lFlags.resetCache = 1;
	    while (1) {
	    
/*		We go through this loop once per line being wrapped.  
 *		We stop if:
 *		    a) we abort due to more chars being typed.
 *		    b) we wrap the ins line and see that it doesn't require
 *			any more wrapping
 *		    c) we wrap and draw all visible text below ins and wrap up 
 *			to synch.
 */
 
/*		Tell drawALine to ping if abortable and this is 
 *		PINGRATE'th line 
 */
		layInfo->lFlags.ping = 0;
		if (abortable && ++pingAgain >= PINGRATE) {
		    layInfo->lFlags.ping = 1;
		    pingAgain = 0;
		}
		
/*		ScanALine to layout next line.  Returns YES if line laid out 
 *		is too wide and we must clip.  ScanALine fills in global 
 *		arrays of lays, chars and widths.  Also advances the 
 *		textCache to 1st char of next line.  Returns rect. for 
 *		this line in layRect.   If text can grow horizontally, 
 *		both origin and size can change, else only height.
 */
		tooWide = scanFunc(self, &info->layInfo);
		
/*		sawEnd is YES if line for new insertion point has been wrapped. */

		if (newSelN >= lastCPos && newSelN < cache->curPos)
		    sawEnd = YES;
		layInfo->lFlags.resetCache = 0;
		
/*		compute number of chars in this line by subtracting lastCPos 
 *		from position of textCache.  detect too long line and 
 *		raise error.
 */
		if ((cw = cache->curPos - lastCPos) > TXMCHARS) {
		    errorNumber = NX_longLine;
		    goto bailedOut;
		}
		
/*		if text object is resizeable and layRect exceeds current 
 *		bodyRect, perLineGrow does minimal stuff to resize 
 *		object so that new line can be drawn, recording maximum 
 *		bounds in boundsMax.  If bounds changes, recompute bottom,
 *		top & mbottom.  if we've clipped reset clipped flag and 
 *		grestore.
 *		!!! Bryan should verify that his drawcache works here.
 */
		if (tFlags.horizResizable || tFlags.vertResizable) {
		    if ([self perLineGrow:info maxBounds:&boundsMax]) {
		    /* redo bottom */
			bottom = _NXSetDrawBounds(self, &_top);
			top = _top;
			mbottom = bodyRect.origin.y + bodyRect.size.height;
			if (clipped) {
			    clipped = 0;
			    if (canDraw)
				PSgrestore();
			}
		    }
		}
		
/*		compute size of new breakTable entry.  if new line ht (height
 *		from layRect) differs from lastHt, then contains NXLineDesc +
 *		NXHeightInfo, else just NXLineDesc.  adjust saved array and
 *		breakTable array if necessary.  recompute cached pointers &
 *		allocated counts.
 */
 
		needs = sizeof(NXLineDesc);
		if (NX_HEIGHT(layRect) != lastHt)
		    needs += sizeof(NXHeightInfo);
		if ((saveLine + needs) >= saveAllocated) {
		    saved->chunk.used = saveLine;
		    saved = (NXBreakArray *)NXChunkRealloc(&saved->chunk);
		    info->globals.savedBreaks = saved;
		    saveAllocated = saved->chunk.allocated;
		}
		if ((curLine + needs) >= allocated) {
		    theBreaks->chunk.used = curLine;
		    theBreaks = (NXBreakArray *)
		      NXChunkRealloc(&theBreaks->chunk);
		    allocated = theBreaks->chunk.allocated;
		}
		
/*		save breakTable bytes to be overwritten in saved array and
 *		then create new breakTable entry.
 */
 
		ps = TX_LINEAT(theBreaks->breaks, curLine);
		if (layInfo->lFlags.endsParagraph)
		    cw |= TXMPARA;
		if (needs == sizeof(NXLineDesc)) {
		    *(TX_LINEAT(saved->breaks, saveLine)) = *ps;
		    *ps = cw;
		} else {
		    NXHeightChange     *htChange = (NXHeightChange *)ps;

		    *TX_HTCHANGE(saved->breaks, saveLine) = *htChange;
		    htChange->lineDesc =
		      htChange->heightInfo.lineDesc = TXYESCHANGE(cw);
		    htChange->heightInfo.oldHeight = lastHt;
		    
		    lastHt = NX_HEIGHT(layRect);
		    htChange->heightInfo.newHeight = lastHt;
		    
		}
		
/*		if the new position for the caret falls on this line,
 *		recompute sp0 and spN and repeg ins to this point. we have
 *		previously cached ins (saveIns) so that we can restore ins
 *		if we abort.  If the selection is between lines, and
 *		the line ends in a carriage return, you want the selection to 
 *		go on the next line.  If you're wrapping a long line, you 
 *		want the selection on the current line.  If there are a lot
 *		of spaces on the end of this line, you wont see the caret.
*/
		if (newSelN >= lastCPos &&
		    ((newSelN < cache->curPos) || 
		    (!layInfo->lFlags.endsParagraph &&
			newSelN <= cache->curPos)))
		    [self reSel:newSelN line:curLine info:info pos:lastCPos
			height:NX_HEIGHT(layRect) yPos:NX_Y(layRect)
			selection:&ins];
			     
/*		if this line is visible, draw the line and show the caret.
 *		as necessary.  if this line must be clipped set clipping. if we
 *		are responding to typing (abortable) we flush on every line
 *		and ping at PINGRATE.  if we have drawn past end of visible
 *		area, we set hitbottom.
 *
 *		!!! BRYAN should think about his drawcache here.
 *		!!! BRYAN apparently removed pinging at ping rate and pings
 * 		!!! at every line.  if this is supposed to be so, he should
 *		!!! also remove logic for implemeting ping rate.
 */
		if (NX_MAXY(layRect) > top) {
		    if (NX_Y(layRect) < bottom) {	/* still visible */
			if (priorBackedUp || (!backedUp) || (backWidth !=
					  (cache->curPos - lastCPos))) {
			    newPriorBackedUp = newPriorBackedUp | backedUp;
			    if (!clipped && (tooWide ||
					     NX_MAXY(layRect) > mbottom)) {
				clipped++;
				if (canDraw) {
				    PSgsave();
				    PSrectclip(bounds.origin.x,
				    		bounds.origin.y,
						bounds.size.width,
						bounds.size.height);
				}
			    }
			    if (canDraw)
				drawFunc(self, &info->layInfo);
			    if (abortable && canDraw) {
				if (canFlush) {
				    PSflushgraphics();
				    if (sp0.cp >= 0 && sp0.line == curLine)
					_NXQuickCaret(self);
				}
				NXPing();
			    } else if (sp0.cp >= 0 && sp0.line == curLine
				&& canFlush)
				_NXQuickCaret(self);
			}
		    } else	/* not visible, if we've wrapped to synch,
				 * then we're done! */
			hitBottom++;
		}
		
/*		reset backedUp, since we have finished line (if any) we
 *		backed up to relayout to check for wrap back.  update lastCPos
 *		and advance offsets into breakTable and saved array.
 */
 		priorBackedUp = 0;
		backedUp = 0;
		lastCPos = cache->curPos;
		curLine += needs;
		saveLine += needs;
		
/*		if we have wrapped line with caret and we are responding to
 *		typing, see if we have some keydowns.  if we do, we update
 *		text blocks and run array, adjust newSelN (char. pos for new
 *		caret).  we restore breakTable with saved array.  restore ins
 *		and set sp0.cp so as to suppress drawing selection until we
 *		have again wrapped caret line.  insertLen is number of chars
 *		newly inserted, and used to come back < 0 if there was an
 *		error.  NOTE: due to complexity, we do not deal with aborting
 *		on backspaces here, but attempt to glom all of them in keyDown.
 *		this can lead to latency problems, but seems to work ok.
 *
 *		!!! no error status is returned anymore and this logic
 *		!!! should be cleaned up (cmf 5/26/89).
 */
		if (abortable && sawEnd) {
		    int insertLen = [self doAbortion:&newSelN info:info
			runs:insertRun];

		    if (insertLen < 0)
			goto bailedOut;
		    else if (insertLen) {
			insChars += insertLen;
			sawEnd = NO;
			bcopy(saved->breaks,
			      TX_LINEAT(theBreaks->breaks,
					curLine - saveLine), saveLine);
			ins = saveIns;
			sp0.cp = -1;
			priorBackedUp = newPriorBackedUp;
			goto abortWrapping;
		    }
		}

/*		advance the text cache to lastCPos.  if new cpos (cp) is
 *		negative, we've scanned all the text!!
 */		
		cp = NXAdjustTextCache(self, cache, lastCPos);
		
/*		if we wrapped the line on which we modified the text and
 *		no changes flowed to the next line, the line height didn't
 *		change and the origin of the line is the same we can recompute
 *		the selection and bail out.
 */
		if (canBail && (curLine - needs) == bailLine) {
		    if ((bailEndPos + insChars) == lastCPos &&
			newSelN <= lastCPos) {
			if (lastHt == bailHt && NX_Y(layRect) == bailY &&
			    ((needs == sizeof(NXLineDesc) && bailChars >= 0) ||
			    (needs == sizeof(NXHeightChange) &&
			    bailChars < 0))) {
			    if (sp0.cp < 0) {	/* newSelN is between this
						 * line and next */
				ps = TX_LINEAT(theBreaks->breaks, curLine);
				if (*ps < 0)
				    NX_HEIGHT(layRect) =
					HTCHANGE(ps)->heightInfo.newHeight;
				[self reSel:newSelN line:curLine info:info
				    pos:lastCPos height:NX_HEIGHT(layRect)
				    yPos:NX_Y(layRect)+ lastHt selection:&ins];
			    }
			    goto bailedOut;
			} else
			    canBail = NO;
		    } else
			canBail = NO;
		}
		NX_Y(layRect) += lastHt;

/*		we're done if we've scanned all the text or we've both drawn
 *		all the visible lines and wrapped to synch line's new location.
 */		
		if (cp < 0 ||
		    (hitBottom && synch.cp >= 0 && lastCPos >= (synch.cp + insChars))) {
		    theBreaks->chunk.used = curLine;
		    saved->chunk.used = saveLine;
		    goto doneWrapping;
		}
	    }
    abortWrapping:
	    layInfo->lFlags.resetCache = 1;
	}
  doneWrapping:
	;
    }

 /*
  * here we patch the remainder of theBreaks & info->globals.savedBreaks into
  * theBreaks after wrapped part. we also erase anything necessary at bottom
  * (if the whole mess has shrunk) 
  */

    {
	register int        used;

	register int        amt;
	int                 delta;
	int                 curLineOld;
	int                 offset;
	register NXCoord    y;
	register NXCoord    deltaY;
	int                 isNewHt;
	NXRect		    erase;

	y = layInfo->rect.origin.y;

/*	if we didn't have a synch point, then we wrapped whole damn thing. */

	if (synch.cp >= 0) {
	
/*	    back up curLine,lastHt,y and lastCPos to new position of synch line.
 *	    remember, we wrap past synch if necessary to draw all visible lines.
 */
	    curLineOld = curLine;
	    ps = TX_LINEAT(theBreaks->breaks, curLine);
	    insChars += synch.cp;
	    while (lastCPos > insChars) {
		y -= lastHt;
		amt = *--ps;
		if (amt < 0) {
		    curLine -= sizeof(NXHeightChange);
		    ps = TX_LINEAT(ps, (-sizeof(NXHeightInfo)));
		    lastHt = HTCHANGE(ps)->heightInfo.oldHeight;
		} else {
		    curLine -= sizeof(NXLineDesc);
		}
		lastCPos -= TXGETCHARS(amt);
	    }
	    
/*	    compute size of breakTable entry for synch line.  as a simplification
 *	    we do not shrink entry size in the case that synch line used to be
 *	    line height change and no longer is.
 */
	    delta = 0;
	    if ((isNewHt = (lastHt != synch.ht)) && synchChars >= 0)
		delta += sizeof(NXHeightInfo);
/*
 * 	    newsize = untouched breaks + lines up to new break for synch.cp
 * 	    possible NXHeightChange for synch) + saved breaks 
*/
	    used = oldUsed - synch.line + curLine + delta;
	    if (used > allocated)	/* depends on error handler */
		theBreaks = (NXBreakArray *)
		  NXChunkGrow(&theBreaks->chunk, used);
	    theBreaks->chunk.used = used;
	    ps = TX_LINEAT(theBreaks->breaks, 0);

/*
 * 	    copy up/down untouched breaks.  if there are no untouched breaks
 *	    adjust saveLine and curLineOld back to where old breakTable stopped.
*/
	    if (used != oldUsed) {
		if (oldUsed > curLineOld) {
		    offset = curLineOld;
		    if (curLineOld < synch.line)
			offset = synch.line;
		    if ((amt = oldUsed - offset) > 0) {
			bcopy(TX_LINEAT(ps, offset), TX_LINEAT(ps, used - amt),
			      amt);
		    }
		} else {
		    amt = curLineOld - oldUsed;
		    curLineOld -= amt;
		    saveLine -= amt;
		}
	    }
	    
/*          copy over saved breaks from info->globals.savedBreaks  */

	    amt = curLineOld - synch.line -
	      ((synchChars < 0) ? sizeof(NXHeightChange) : sizeof(NXLineDesc));
	    if (amt > 0)
		bcopy(TX_LINEAT(saved->breaks, saveLine - amt),
		      TX_LINEAT(ps, curLine +
				((isNewHt || synchChars < 0) ?
			      sizeof(NXHeightChange) : sizeof(NXLineDesc))),
		      amt);

/* 	    fill in break for synch line  */

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
	
/* 	if we have to, erase old text at bottom. */

	if ((deltaY = synch.y - y) > 0) {
	    y = maxY + bodyRect.origin.y - deltaY;
	    if (y < bottom && canDraw) {
		NXSetRect(&erase, bounds.origin.x, y,
		    bounds.size.width, ((y + deltaY > bottom) ? 
		    bottom - y : deltaY));
		_NXTextBackgroundRect(self, &erase, NO, [self shouldDrawColor]);
		[window flushWindow];
	    }
	}
	
/*	adjust total height of all lines */

	maxY -= deltaY;
    }

  bailedOut:
    if (clipped > 0 && canDraw)
	PSgrestore();
	
/*  if text object could have grown, check if it did and take care of any drawing
 *  necessary to redraw exposed areas.
 */
    if (!errorNumber && (tFlags.horizResizable || tFlags.vertResizable))
	[self oncePerGrow:&boundsSave maxBounds:&boundsMax];
    [self suspendNotifyAncestorWhenFrameChanged:NO];
    if (wasBlinking)
	[self showCaret];
    EPILOG(info);
    info->globals.inReplaceSel--;
    return (errorNumber);
}

- _replaceSel:(const char *)aString length:(int)length smartPaste:(BOOL)flag
{
    NXRun              *pr;
    NXRunArray          oneRun;
    NXTextInfo *info = _info;
    BOOL canDraw = [self _canDraw];
    NXTextGlobals *globals = TEXTGLOBALS(_info);

    if (sp0.cp < 0)
	return self;
    oneRun.chunk.used = sizeof(NXRun);
    oneRun.chunk.growby = sizeof(NXRun);
    oneRun.chunk.allocated = sizeof(NXRun);
    NXAdjustTextCache(self, &info->layInfo.cache, sp0.cp);
    pr = &oneRun.runs[0];
    bzero(pr, sizeof(NXRun));
    pr->paraStyle = globals->uniqueStyle;
    pr->font = info->layInfo.cache.curRun->font;
    pr->chars = length;
    pr->textGray = textGray;
    pr->textRGBColor = globals->textRGBColor;
    if (canDraw)
	[self lockFocus];
    [self _replaceSel:NULL :aString :length :&oneRun:0 :0 :flag];
    if (canDraw) {
	if (FLUSHWINDOW(window))
	    PSflushgraphics();
	[self unlockFocus];
    }
    return self;
}

/*
 * This gets called from rtf reading when we want to completely replace the 
 * existing text within the text object;
 */
- _replaceAll:
    (const char *)insertText :
    (int)iLen :
    (NXRunArray *)insertRun
{
    int errorNumber = 0;

    
/*	modify text blocks to replace selection with inserted.  modify run array 
 *	to reflect run(s) of
 *	inserted text, deletion of selection and insertion of insertText. at this
 *      point, text blocks hold new text, runs reflect new text, but breakTable
 *      reflects old text.  NOTE, iLen can be modified by replaceRangeAt to
 * 	smart cut or paste.
 *      !!! errorNumber is hysterical baggage and should be zapped.
 */
	[self replaceRangeAt:sp0.cp end:spN.cp info:_info
	      text:insertText length:&iLen runs:insertRun
	      smartPaste:NO error:&errorNumber];
	if (errorNumber)
	return self;
	
    [self registerCellInfo:insertRun];

	/* Now relayout all the text */
    [self selectNull];
    [self calcLine];
		
    return self;
}
@end

static unsigned hashCell (const void *info, const void *data)
{
    const NXTextCellInfo *cellInfo = data;
    return NXPtrHash(info, cellInfo->cell);
}

static int cellEqual (const void *info, const void *data1, const void *data2)
{
    const NXTextCellInfo *cellInfo1 = data1;
    const NXTextCellInfo *cellInfo2 = data2;
    return NXPtrIsEqual(info, cellInfo1->cell, cellInfo2->cell);

}

static void freeCell(const void *info, void *data)
{
    NXTextCellInfo *cellInfo = data;
    [cellInfo->cell free];
    free(cellInfo);
}

static NXHashTablePrototype cellProto = {
    hashCell,
    cellEqual,
    freeCell,
    0
};


/*

83
--

5/3/90 chris	Fixed overoptimistic (during abort) back wrap draw code

84
--
 5/10/90 chris	moved _NXClearClickState() in replaceSel:::::: to 
 		reSel:line:info:pos: so that anchor is not set while
		sp0.cp == -1
85
--
 6/4/90 glc	new method _replaceAll::: for when you want to replace alot of 
 		lines of text fast. Use in RTF reading.
		
87
--
7/12/90	glc	changes to support color text. Added new background erasing 
		utility call.		

91
--
 8/12/90 aozer	Changed isOnColorScreen -> shouldDrawColor

*/


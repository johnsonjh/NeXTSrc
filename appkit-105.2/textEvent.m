/*
	textEvent.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Application.h"
#import "Cell.h"
#import "Pasteboard.h"
#import "Text.h"
#import "Window.h"
#import "errors.h"
#import "nextstd.h"
#import "publicWraps.h"
#import "textprivate.h"
#import "timer.h"
#import <dpsclient/wraps.h>
#import <mach.h>
#import <math.h>
#import <stdlib.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryEvent=0\n");
    asm(".globl .NXTextCategoryEvent\n");
#endif

extern const char *const _NXSmartPaste;
static const char *RulerType = "NeXT ruler pasteboard type";
static const char *FontType = "NeXT font pasteboard type";

@interface Object(ObsoleteTextDelegate)
- text:textObject isEmpty:(BOOL)flag;
@end

@implementation Text(Event)

/* performs check first time text is edited after becoming first responder.
   Called byt keyDown:, paste:, delete:, ...  Returns YES if text can be
   edited, NO if the delegate refuses, or RAISES an error id the text
   is read only.
 */

- (BOOL) initialEditableCheck
{
    if (sp0.cp < 0 || tFlags._editMode == READONLY) {
	NXBeep();
	return NO;
    } else if (!tFlags.changeState &&
	       delegateMethods & TXMWILLCHANGE &&
	       [delegate textWillChange:self]) {
	[self _bringSelToScreen:VISBIGFUDGE];
	[window makeFirstResponder:window];
	return NO;
	NXBeep();
    } else
	return YES;
}

- _chooseAnchor:(NXPoint *)thePoint
{
    NXSelPt mouse0, mouseN;
    NXTextInfo *info = _info;
    NXRect hitRect = info->globals.hitRect;
    if ((NXMouseInRect(thePoint, &hitRect, YES)))
	return self;

    [self _ptToHit:thePoint :&mouse0:&mouseN];
    if (mouse0.cp >=  spN.cp)
	tFlags.anchorIs0 = YES;
    else if (mouseN.cp <= sp0.cp)
	tFlags.anchorIs0 = NO;
    else {
	int left = mouse0.cp - sp0.cp;
	int right = spN.cp - mouseN.cp;
	if (left > right)
	    tFlags.anchorIs0 = YES;
	else 
	    tFlags.anchorIs0 = NO;
    }
    if (tFlags.anchorIs0)
	anchorL = anchorR = sp0;
    else 
	anchorL = anchorR = spN;
    info->globals.hitRect = hitRect;
    return self;
}

- (BOOL)acceptsFirstResponder
{
    return tFlags._selectMode == SELECTABLE;
}

- becomeFirstResponder
{
    tFlags.changeState = NO;	/* next edit will cause willChange/didChange */
    return self;
}

- becomeKeyWindow
{
    if ([self _canDraw]) {
	[self lockFocus];
	[self showCaret];
	[self unlockFocus];
    } else
	[self showCaret];
    [self _fixFontPanel];
    return self;
}

- resignKeyWindow
{
    [self hideCaret];
    PSrevealcursor();
    return self;
}

- clear:sender
{
    [self delete:sender];
    return self;
}

- copyFont:sender
{
    const char *types[NPBTYPES];
    int streamlen, streammax, count = 0;
    id pboard = [Pasteboard newName:NXFontPboard];
    NXStream *stream = NULL;
    char *streambuf = 0;

    if (!pboard)
	return self;
    types[count++] = FontType;
    [pboard declareTypes:types num:count owner:NULL];
    stream = NXOpenMemory(NULL, 0, NX_READWRITE);
    [self writeRichText:stream from:sp0.cp to:sp0.cp];
    NXGetMemoryBuffer(stream, &streambuf, &streamlen, &streammax);
    [pboard writeType:FontType data:streambuf length:streamlen];
    if (stream)
	NXCloseMemory(stream, NX_FREEBUFFER);
    return self;
}

- copyRuler:sender
{
    const char *types[NPBTYPES];
    int streamlen, streammax, count = 0;
    id pboard = [Pasteboard newName:NXRulerPboard];
    NXStream *stream = NULL;
    char *streambuf = 0;

    [self selectParagraph];
    if (!pboard)
	return self;
    types[count++] = RulerType;
    [pboard declareTypes:types num:count owner:NULL];
    stream = NXOpenMemory(NULL, 0, NX_READWRITE);
    [self writeRichText:stream from:sp0.cp to:sp0.cp];
    NXGetMemoryBuffer(stream, &streambuf, &streamlen, &streammax);
    [pboard writeType:RulerType data:streambuf length:streamlen];
    if (stream)
	NXCloseMemory(stream, NX_FREEBUFFER);
    return self;
}

- copy:sender
{
    BOOL writeRTF = tFlags.monoFont ? NO : YES;
    id pboard = [Pasteboard new];

    return([self _copytopboard:pboard writeRTF:writeRTF]);
}
    
- _copytopboard:pboard writeRTF:(BOOL)writeRTF
{
    const char *types[NPBTYPES];
    int start, stop, streamlen, streammax, count = 0;
    NXStream *stream = NULL, *tempStream;
    char *streambuf = 0;
    BOOL smartPaste = (clickCount == 2) ? YES : NO;
    NXZone *zone = [self zone];

    start = sp0.cp;
    stop = spN.cp;
    if (stop >= textLength)
	stop = textLength -1;
    if (start < 0 || stop <= start || !pboard)
	return self;
    types[count++] = NXAsciiPboardType;
    if (writeRTF)
	types[count++] = NXRTFPboardType;
    if (smartPaste)
	types[count++] = _NXSmartPaste;
    [pboard declareTypes:types num:count owner:NULL];
 /*
  * Only ascii is written if monoFont.  Otherwise RTF and ascii are written.
  * If both types are written, share the streambuf buffer between both writes.  
  */
    if (writeRTF) {
	stream = NXOpenMemory(NULL, 0, NX_READWRITE);
	[self writeRichText:stream from:start to:stop];
	NXGetMemoryBuffer(stream, &streambuf, &streamlen, &streammax);
	[pboard writeType:NXRTFPboardType data:streambuf length:streamlen];
    }
 /*
  * write ascii next, using the same buffer, since length(rtf) < length(ascii)      
  */
    if (!streambuf) 
	streambuf = NXZoneMalloc(zone, stop - start); 
    tempStream = [self stream];
    NXSeek(tempStream, start, NX_FROMSTART);
    NXRead(tempStream, streambuf, stop - start);
    [pboard writeType:NXAsciiPboardType data:streambuf length:stop - start];
    if (stream)
	NXCloseMemory(stream, NX_FREEBUFFER);
    else 
	free(streambuf);
    if (smartPaste) 
	[pboard writeType:_NXSmartPaste data:0 length:0];
    return self;
}

- cut:sender
{
    [self copy:sender];
    [self delete:sender];
    return self;
}

- delete:sender
{
    int        cp;
    int        lp;
    int        errorNumber = 0;
    BOOL                canDraw = [self _canDraw];

    cp = sp0.cp;
    if (![self initialEditableCheck])
	return self;
    if ((lp = spN.cp) > cp) {
	[self _bringSelToScreen:VISBIGFUDGE];
	if (canDraw)
	    [self lockFocus];
	if (!tFlags.changeState) {
	    tFlags.changeState = YES;
	    if (delegateMethods & TXMCHANGEACTION)
		[delegate textDidChange:self];
	}
	errorNumber = [self _replaceSel:NULL :NULL :0 :NULL :0 :clickCount == 2:0];
	[self showCaret];
	if (canDraw) {
	    if (FLUSHWINDOW(window))
		PSflushgraphics();
	    [self unlockFocus];
	}
    }
    if (delegateMethods & TXMDIDGETKEYS)
	[delegate textDidGetKeys:self isEmpty:(textLength <= 1)];
    else if (delegateMethods & TXMISEMPTY)
	[delegate text:self isEmpty:(textLength <= 1)];
    if (errorNumber)
	NX_RAISE(errorNumber, 0, 0);
    return self;
}


/*
 * Processing of keyboard events.
 *
 * 2 seperate loops are used, depending on whether the key is a backspace or not.
 * Peeking in the event queue is used to bunch up groups of chars or backspace chars.
 * We can accept symbol character set keys as the event passed in theEvent. We will
 * not batch up symbol set keys with normal keys. Instead we flush the current
 * accumulated characters, and will be called again with the symbol set key.
 */
- keyDown:(NXEvent *)theEvent
{
    char                cA[11];
    unsigned short theChar;
    NXRunArray          dfltRun;
    NXEvent             _geek;
    NXEvent   *geek;

    int        iLen;
    int                 lockedOwner;
    int        insert;
    int                 smartCut;
    int                 errorNumber = 0;
    id                  safeDelegate, retval = nil;
    int		symbol;		/* Whether we should insert as symbol font */
    NXTextInfo *info = _info;
    BOOL emptySelection;

    emptySelection = (sp0.cp == spN.cp);
    PROLOG(info);
    lockedOwner = 0;
    symbol = 0;
    if (!(theChar = (*charFilterFunc) (theEvent->data.key.charCode,
				       theEvent->flags,
				       theEvent->data.key.charSet))) {
	retval = self;
	NXBeep();
	goto nope;
    }
    if ((theChar >= NX_RETURN) && (theChar <= NX_DOWN)) {
	if (theChar >= NX_LEFT && theChar <= NX_DOWN) {
	    retval = [self moveCaret:theChar];
	    goto nope;
	}
	info->globals.didEndChar = theChar;	/* implicit param to
						 * resignFirstResp */
    /*
     * In case resignFirstResponder disposes of self, we save the delegate
     * and whether the delegate responds to didEnd. 
     */
	safeDelegate = (delegateMethods & TXMENDACTION) ? delegate : NULL;
	if ([window makeFirstResponder:window])
	    [safeDelegate textDidEnd:self endChar:theChar];
	info->globals.didEndChar = 0;
	goto nope;
    }
    if (![self initialEditableCheck])
	goto nope;

    PSobscurecursor();
    [self hideCaret];
    if ((sp0.cp == spN.cp) && (sp0.line != spN.line)) {
	[self lockFocus];
	lockedOwner++;
	[self _hilite:NO :&sp0:&spN:NULL];
	sp0 = spN;
    }
    cA[0] = theChar;
    iLen = 1;
    insert = 1;
    smartCut = 0;
    if (theChar == NX_BACKSPACE) {
	if (sp0.cp == spN.cp) {
	    if (sp0.cp == 0) {
	        NXBeep();
		goto nopExit;
	    }
	    iLen = 1;
	    while (1) {
		geek = [NXApp peekNextEvent:NX_ALLEVENTS into:&_geek];
		if (!geek)
		    break;
		if (geek->type == NX_KEYUP) {
		    [NXApp getNextEvent:NX_KEYUPMASK];
		    continue;
		}
		if (geek->type != NX_KEYDOWN)
		    break;
		theChar = (*charFilterFunc) (
				       geek->data.key.charCode, geek->flags,
					     geek->data.key.charSet);
		if (theChar != NX_BACKSPACE)
		    break;
		[NXApp getNextEvent:NX_KEYDOWNMASK];
		iLen++;
	    }
	    [self _setSel:(sp0.cp < iLen ? 0 : sp0.cp - iLen) :sp0.cp];
	} else
	    smartCut = (clickCount == 2);
	iLen = 0;
    } else {
    	if(theEvent->data.key.charSet == NX_SYMBOLSET) {
	    symbol = 1;
	}
	else {
	    while (iLen < 11) {
		geek = [NXApp peekNextEvent:NX_ALLEVENTS into:&_geek];
		if (!geek)
		    break;
		if (geek->type == NX_KEYUP) {
		    [NXApp getNextEvent:NX_KEYUPMASK];
		    continue;
		}
		if (geek->type != NX_KEYDOWN)
		    break;
		if (geek->data.key.charSet == NX_SYMBOLSET)
		    break;
		if (!(theChar = (*charFilterFunc) (
					    geek->data.key.charCode, geek->flags,
						    geek->data.key.charSet)) ||
		    ((theChar == NX_BACKSPACE) && !iLen) ||
		    ((theChar >= NX_RETURN) && (theChar <= NX_DOWN)))
		    break;
		[NXApp getNextEvent:NX_KEYDOWNMASK];
		if (theChar == NX_BACKSPACE) {
		    iLen--;
		    continue;
		}
		cA[iLen++] = theChar;
	    }
	}
	insert = iLen;
    }

    if (insert) {
	[self _bringSelToScreen:VISBIGFUDGE];
	if (!lockedOwner)
	    [self lockFocus];
	lockedOwner++;
	if (!tFlags.changeState) {
	    tFlags.changeState = YES;
	    if (delegateMethods & TXMCHANGEACTION)
		[delegate textDidChange:self];
	}
	if (iLen == 0)		/* backspace always resets typingRun */
	    typingRun.chars = 0;
	    
	_NXSetDefaultRun(self, iLen, &dfltRun);
	if(symbol && ![self isMonoFont]) {
	    float	size;
	    
	    size = [dfltRun.runs[0].font pointSize];
	    dfltRun.runs[0].font = [Font newFont:"Symbol" size:size];
	}
	errorNumber = [self _replaceSel:NULL :cA :iLen :&dfltRun :1 :smartCut :0];
    }
  nopExit:
    if (!lockedOwner)
	[self lockFocus];
    [self showCaret];
    if (FLUSHWINDOW(window))
	PSflushgraphics();
    NXPing();
    [self unlockFocus];
    [self _bringSelToScreen:VISLITTLEFUDGE];
    if (!emptySelection && (sp0.cp == spN.cp)) [self _fixFontPanel];
    anchorL = sp0;
    anchorR = spN;
    NX_X(&info->globals.hitRect) = sp0.x;
    NX_Y(&info->globals.hitRect) = sp0.y;
    NX_WIDTH(&info->globals.hitRect) = 0.0;
    NX_HEIGHT(&info->globals.hitRect) = 0.0;
    if (delegateMethods & TXMDIDGETKEYS)
	[delegate textDidGetKeys:self isEmpty:(textLength <= 1)];
    else if (delegateMethods & TXMISEMPTY)
	[delegate text:self isEmpty:(textLength <= 1)];
    if (errorNumber)
	NX_RAISE(errorNumber, 0, 0);
    retval = self;
  nope:
    EPILOG(info);
    return retval;
}

- trackCell:cell rect:(NXRect *)rect event:(NXEvent *)event
{
    [window disableFlushWindow];
    [cell highlight:rect inView:self lit:YES];
    [window reenableFlushWindow];
    [window flushWindow];
    [cell trackMouse:event inRect:rect ofView:self];
    [window disableFlushWindow];
    [cell highlight:rect inView:self lit:NO];
    [window reenableFlushWindow];
    [window flushWindow];
    return self;
}

- trackCell:(NXEvent *)event location:(NXPoint *)loc
{
    NXLay *lay, *last;
    NXLayInfo *info = (NXLayInfo *)_info;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    lay = info->lays->lays;
    last = TX_LAYAT(lay, info->lays->chunk.used);
    for (; lay < last; lay++) {
	NXRun *run = lay->run;
	if (run && run->rFlags.graphic) {
	    NXTextCellInfo temp, *ptr;
	    temp.cell = run->info;
	    ptr = NXHashGet(globals->cellInfo, &temp);
	    if (ptr) {
		NXRect rect;
		if (loc->x < ptr->origin.x)
		    return self;
		rect.origin = ptr->origin;
		[run->info calcCellSize:&rect.size];
		if (NXPointInRect(loc, &rect)) {
		    [self trackCell:run->info rect:&rect event:event];
		    return nil;
		}
	    }
	}
    }
    return self;
}

#define SEL_EV_MASK (NX_MOUSEDRAGGEDMASK | NX_MOUSEUPMASK | NX_KEYDOWNMASK | NX_KEYUPMASK | NX_MOUSEDOWNMASK | NX_TIMERMASK)

- mouseDown:(NXEvent *)theEvent
{
    char		shifty;
    NXEvent		*cevent;
    NXEvent             lastRealEvent;	/* last non-timer event (for
					 * autoScroll) */
    int			emask;
    NXPoint             mouseLocation, lastLocation;
    NXRect              vRect;
    NXTrackingTimer     timer;	/* timer events used for auto scroll */
    id                  retval = nil;
    NXTextGlobals	*globals = TEXTGLOBALS(_info);
    int			mouseDownEvNum = theEvent->data.mouse.eventNum;

    PROLOG(_info);
    globals->caretXPos = -1.0;
    tFlags.haveDown = SINotTracking;

    if (![self acceptsFirstResponder])
	goto errExit1;
    cevent = theEvent;
    mouseLocation = cevent->location;
    [self convertPoint:&mouseLocation fromView:nil];
    shifty = (((NX_SHIFTMASK | NX_ALTERNATEMASK) & cevent->flags) != 0);

    if (!NXMouseInRect(&mouseLocation, &bounds, YES))
	goto nope;

    if (!shifty) {
	NXSelPt t0 = sp0;
	NXSelPt tN = spN;
	[self _ptToHit:&mouseLocation:&t0:&tN];
	if (![self trackCell:cevent location:&mouseLocation])
	    goto nope;
    }
    [self lockFocus];
    typingRun.chars = 0;

    [self hideCaret];
    if (shifty && (sp0.cp >= 0)) {	/* grow old */
    /*
     * here we don't zap clickCount,anchorL,anchorR & anchorIs0 and then we
     * fake a mouseDragged to start us off followed by the normal tracking
     * loop.  we set info->globals.hitRect so that mouse is outside of it. 
     */
        globals->hitRect.origin.x = mouseLocation.x+2.0;
	globals->hitRect.origin.y = mouseLocation.y+2.0;
	globals->hitRect.size.width = 0.0;
	globals->hitRect.size.height = 0.0;
	tFlags.haveDown = SIMove;
	[self _chooseAnchor:&mouseLocation];
	[self _mouseTrack:&mouseLocation];
    } else {			/* start new */
	growLine = -1;
	clickCount = cevent->data.mouse.click;

	if (![self selectNull])
	    goto errExit;
	[self _ptToHit:&mouseLocation:&sp0:&spN];
	if ((sp0.cp >= 0) && (sp0.cp == spN.cp) && (sp0.line == spN.line))
	    spN = sp0;
	anchorL = sp0;
	anchorR = spN;
	if (clickCount >= 2) {
	    [self _hilite:YES :&sp0:&spN:NULL];
	    if (FLUSHWINDOW(window))
		PSflushgraphics();
	}
	growLine = sp0.line;
	tFlags.anchorIs0 = YES;
    }
    tFlags.haveDown = SIMove;

    emask = [window addToEventMask:NX_MOUSEDRAGGEDMASK];

    [self getVisibleRect:&vRect];
    mouseLocation = *(NXPoint *)&cevent->location;
    [self convertPoint:&mouseLocation fromView:nil];
    lastLocation = mouseLocation;
    lastRealEvent = *cevent;
    (void)NXBeginTimer(&timer, 0.1, 0.1);
    while (1) {
	NXEvent evSpace;
	int isStillDown;

	cevent = [NXApp peekNextEvent:SEL_EV_MASK into:&evSpace waitFor:NX_FOREVER threshold:NX_MODALRESPTHRESHOLD];
	if (cevent->type == NX_KEYDOWN || cevent->type == NX_KEYUP) {
	    PSstilldown(mouseDownEvNum, &isStillDown);
	    if (!isStillDown) {
		tFlags.haveDown = SINotTracking;
		goto endTrack;
	    }
	}
	cevent = [NXApp getNextEvent:SEL_EV_MASK];
	switch (cevent->type) {
	case NX_MOUSEUP:
	case NX_MOUSEDOWN:
	    tFlags.haveDown = SINotTracking;
	    goto endTrack;
	case NX_MOUSEDRAGGED:
	    lastRealEvent = *cevent;
	    mouseLocation = lastRealEvent.location;
	    [self convertPoint:&mouseLocation fromView:nil];
	    if (lastLocation.x != mouseLocation.x ||
		lastLocation.y != mouseLocation.y) {
		[self _mouseTrack:&mouseLocation];
		lastLocation = mouseLocation;
	    }
	    break;
	case NX_TIMER:
	    if (!NXMouseInRect(&mouseLocation, &vRect, YES)) {
		[self autoscroll:&lastRealEvent];
		NXPing();
		mouseLocation = lastRealEvent.location;
		[self convertPoint:&mouseLocation fromView:nil];
		[self getVisibleRect:&vRect];
		[self _mouseTrack:&mouseLocation];
	    }
	    break;
	case NX_KEYDOWN:
	case NX_KEYUP:
	    break;	/* pitch these during tracking (but not after!) */
	}
    }

  endTrack:
    NXEndTimer(&timer);
    [window setEventMask:emask];
    if (ISNULLSEL)
	spN = sp0;
    if (sp0.cp < 0)
	[window makeFirstResponder:window];
    else
	[self showCaret];
    [self _fixFontPanel];
    if (HAVE_RULER(_info))
	[self updateRuler];
  errExit:
    [self unlockFocus];
  errExit1:
    retval = self;
  nope:
    EPILOG(_info);
    return retval;
}

- moveCaret:(unsigned short)theKey
{
    NXLineDesc *ps = TX_LINEAT(theBreaks->breaks, 0);
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    PROLOG(_info);
    globals->gFlags.movingCaret = YES;
    switch (theKey) {
    case NX_LEFT:
	globals->caretXPos = -1.0;
	if (sp0.cp >= 0) {
	    if (sp0.cp == spN.cp) {
		if (sp0.cp)
		    [self setSel:sp0.cp - 1:sp0.cp - 1];
	    } else
		[self setSel:sp0.cp:sp0.cp];
	}
	break;
    case NX_RIGHT:
	globals->caretXPos = -1.0;
	if (sp0.cp >= 0) {
	    if (sp0.cp == spN.cp) {
		if (spN.cp < textLength - 1)
		    [self setSel:spN.cp + 1:spN.cp + 1];
	    } else
		[self setSel:spN.cp:spN.cp];
	}
	break;
    case NX_UP:
	if (sp0.c1st == 0)
	    break;
	{
	    NXSelPt             sel0, selN;
	    NXPoint             thePoint;
	    if (globals->caretXPos < 0.0) {
		thePoint.x = sp0.x + 1.0;
		globals->caretXPos = sp0.x;
	    } else 
		thePoint.x = globals->caretXPos + 1.0;
	    thePoint.y = sp0.y - ceil(sp0.ht / 2);
	    [self _ptToHit:&thePoint:&sel0:&selN];
	    [self _setSel:&sel0];
	}
	break;
    case NX_DOWN:
	{
	    NXLineDesc          lineLength = *(TX_LINEAT(ps, spN.line));
	    NXSelPt             sel0, selN;
	    NXPoint             thePoint;

	    lineLength &= TXMCHARS;
	    if ((spN.c1st + lineLength) > (textLength - 1))
		break;
	    if (globals->caretXPos < 0.0) {
		thePoint.x = spN.x - 1.0;
		globals->caretXPos = spN.x;
	    } else 
		thePoint.x = globals->caretXPos - 1.0;
	    thePoint.y = spN.y + (1.5 *spN.ht);
	    [self _ptToHit:&thePoint:&sel0:&selN];
	    [self _setSel:&sel0];
	}
	break;
    }
    [self _bringSelToScreen:VISBIGFUDGE];
    if (!tFlags.monoFont)
	[self _fixFontPanel];
    if (HAVE_RULER(_info))
	[self updateRuler];
    globals->gFlags.movingCaret = NO;
    EPILOG(_info);
    return self;
}

- paste:sender
{
    return([self _pastefrompboard:[Pasteboard new]]);
}    

- _pastefrompboard:pboard
{
    NXStream *stream;
    char *streambuf;
    int slen;
    const char * const *types;
    BOOL hasRichText = NO, hasAscii = NO, hasSmartPaste = NO;
    kern_return_t kr;

    types = [pboard types];
    while (*types) {
	const char	*thetype = *types;

	if (!strcmp(thetype, NXRTFPboardType))
	    hasRichText = YES;
	if (!strcmp(thetype, NXAsciiPboardType))
	    hasAscii = YES;
	if (!strcmp(thetype, _NXSmartPaste))
	    hasSmartPaste = YES;
	types++;
    }
    if (hasRichText || hasAscii)
        if (![self initialEditableCheck])
	    return self;
    if (!tFlags.changeState) {
	tFlags.changeState = YES;
	if (delegateMethods & TXMCHANGEACTION)
	    [delegate textDidChange:self];
    }
    if (hasRichText) {
	if ([pboard readType:NXRTFPboardType data:&streambuf length:&slen] == nil)
	    return self;
	stream = NXOpenMemory(streambuf, slen, NX_READONLY);
	stream->flags &= ~NX_USER_OWNS_BUF;
	[self replaceSelWithRichText:stream];
	NXCloseMemory(stream, NX_FREEBUFFER);
    } else if (hasAscii) {
	if ([pboard readType:NXAsciiPboardType data:&streambuf length:&slen] == nil)
	    return self;
	[self _replaceSel:streambuf length:slen smartPaste:hasSmartPaste];
	kr = vm_deallocate(task_self(), (vm_address_t)streambuf, slen);
	if (kr != KERN_SUCCESS) {
	    NX_RAISE(NX_appkitVMError, (void *)kr, "vm_deallocate() in Text");
	}
    } else 
	return nil;
    [self showCaret];
    [self _bringSelToScreen:VISBIGFUDGE];
    if (delegateMethods & TXMDIDGETKEYS)
	[delegate textDidGetKeys:self isEmpty:(textLength <= 1)];
    else if (delegateMethods & TXMISEMPTY)
	[delegate text:self isEmpty:(textLength <= 1)];
    return self;
}

- pasteFont:sender
{
    NXStream *stream;
    const char * const *types;
    BOOL hasFont = NO;
    id pboard; 
    char *streambuf;
    int slen;
    
    pboard = [Pasteboard newName:NXFontPboard];
    types = [pboard types];
    while (*types) {
	if (!strcmp(*types++, FontType))
	    hasFont = YES;
    }
    
    if(!hasFont)
	return nil;	
    if (![self initialEditableCheck])
	return self;
    if (!tFlags.changeState) {
	tFlags.changeState = YES;
	if (delegateMethods & TXMCHANGEACTION)
	    [delegate textDidChange:self];
    }

	if (tFlags.monoFont) {
	    NXBeep();
	} else {
	    if ([pboard readType:FontType data:&streambuf length:&slen] == nil)
	        return self;
	    stream = NXOpenMemory(streambuf, slen, NX_READONLY);
	    stream->flags &= ~NX_USER_OWNS_BUF;
	    [self readRTF:stream op:font_io];
	    NXCloseMemory(stream, NX_FREEBUFFER);
	    [self _fixFontPanel];
	}
	
    [self showCaret];
    [self _bringSelToScreen:VISBIGFUDGE];
    if (delegateMethods & TXMDIDGETKEYS)
	[delegate textDidGetKeys:self isEmpty:(textLength <= 1)];
    return self;
}    

- pasteRuler:sender
{
    NXStream *stream;
    const char * const *types;
    BOOL hasRuler = NO;
    id pboard; 
    char *streambuf;
    int slen;
    
    pboard = [Pasteboard newName:NXRulerPboard];
    types = [pboard types];
    while (*types) {
	if (!strcmp(*types++, RulerType))
	    hasRuler = YES;
    }
    
    if(!hasRuler)
	return nil;	
    if (![self initialEditableCheck])
	return self;
    if (!tFlags.changeState) {
	tFlags.changeState = YES;
	if (delegateMethods & TXMCHANGEACTION)
	    [delegate textDidChange:self];
    }

	if (tFlags.monoFont) {
	    NXBeep();
	} else {
	    BOOL autodisplay = vFlags.disableAutodisplay; 
	    if ([pboard readType:RulerType data:&streambuf length:&slen] == nil)
	        return self;
	    stream = NXOpenMemory(streambuf, slen, NX_READONLY);
	    stream->flags &= ~NX_USER_OWNS_BUF;
	    vFlags.disableAutodisplay = YES;
	    [self selectParagraph];
	    vFlags.disableAutodisplay = autodisplay;
	    [self readRTF:stream op:ruler_io];
	    NXCloseMemory(stream, NX_FREEBUFFER);
	    if (HAVE_RULER(_info))
		[self updateRuler];
	}
		
    [self showCaret];
    [self _bringSelToScreen:VISBIGFUDGE];
    if (delegateMethods & TXMDIDGETKEYS)
	[delegate textDidGetKeys:self isEmpty:(textLength <= 1)];
    return self;
}    


    
- resignFirstResponder
{
    NXTextInfo *info = _info;
    BOOL                canDraw = [self _canDraw];

    if (tFlags.changeState && (delegateMethods & TXMWILLEND)) {
	if ([delegate textWillEnd:self]) {
	    [self selectError];
	    return NULL;
	}
    }
    [self selectNull];
    if (canDraw && FLUSHWINDOW(window)) {
	[self lockFocus];
	PSflushgraphics();
	[self unlockFocus];
    }
 /*
  * If we are ending firstResp because of a keystroke, keyDown will send the
  * didEnd message to prevent an infinite loop (since the target may set the
  * firstResp in his didEnd (e.g., tabbing between fields)). 
  */
    if ((delegateMethods & TXMENDACTION) && !info->globals.didEndChar)
	[delegate textDidEnd:self endChar:0];
    return (self);
}

- selectAll
{
    return[self selectAll:self];
}

- selectAll:sender
{
    if ([window makeFirstResponder:self]) {
	[self setSel:0:MAXINT];
    }
    return self;
}

- selectText:sender
{
    [self selectAll:sender];
    return self;
}

- selectText
{
    [self selectText:self];
    return self;
}

@end

/*

84
--

 5/08/90 chris	modified moveCaret: to deal with case where selection wants to
 		to be on the end of line of the previous line. (setSel::, always
		puts the selection on the start of the 2nd line)
 5/08/90 chris	made keyDown: beep if backspace at beginning of file.
 5/09/90 chris  made paste call text:isEmpty: or textDidGetKeys:isEmpty:
 5/10/90 chris  fixed mouseDown: so that hitRect is ignored for shift click
 5/11/90 chris  removed parameter to _fixFontPanel: and remove use of bucky
		bits to suppress setting fontPanel. as per discussion with
		bryan, Copy Font removes necessity for this "feature" which
		interfers with mixing clicking and command key setting of
		bold, etc.
		
86
--
 5/17/90 chris	resignKeyWindow modified to reveal cursor (so wait cursor will
 		show when you close window, etc.)
 5/22/90 chris  modified initialEditableCheck to only beep on null sel.
 6/11/90 gcockrft 
 		Added pasteFont & pasteRuler. Copying and pasting of Fonts and Rulers
		now uses separate pasteboards.

92
--
 8/20/90 gcockrft  Made copy & paste generic to share code with services stuff
 
97
--
10/10/90 glc	Added auto symbol font changing.
 
*/

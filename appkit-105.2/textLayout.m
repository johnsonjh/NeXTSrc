/*
	textLayout.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Text.h"
#import "appkitPrivate.h"
#import "textprivate.h"
#import "textWraps.h"
#import "errors.h"
#import <math.h>
#import <dpsclient/wraps.h>
#import <dpsclient/dpsNeXT.h>
#import "PSMatrix.h"

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryLayout=0\n");
    asm(".globl .NXTextCategoryLayout\n");
#endif

@implementation Text (Layout)

- removeDefaultStyle
{
    NXRun *first, *last, *run;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXTextStyle *unique = globals->uniqueStyle;
    first = theRuns->runs;
    last = first + (theRuns->chunk.used / sizeof(NXRun));
    for (run = first; run < last; run++) {
	if (run->paraStyle == unique)
	    run->paraStyle = 0;
    }
    globals->uniqueStyle = 0;
    return self;
}

- addDefaultStyle
{
    NXRun *first, *last, *run;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    first = theRuns->runs;
    last = first + (theRuns->chunk.used / sizeof(NXRun));
    globals->uniqueStyle = [self getUniqueStyle:globals->defaultStyle];
    for (run = first; run < last; run++) {
	if (!run->paraStyle)
	    run->paraStyle = globals->uniqueStyle;
    }
    return self;
}

- (int)alignment
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    return globals->defaultStyle->alignment;
}

- (int)calcLine
{
    NXCoord             lastHt, top = 0.0, bottom = 0.0, cp0, cpN;
    int                 lastCPos, needs, used, cw, clipped, retval = 0;
    NXLineDesc *ps;
    NXRect              vRect, eRect, *layRect;
    BOOL                canDraw = [self _canDraw];
    NXTextInfo *info = _info;

    PROLOG(info);
    NXFlushTextCache(self, &info->layInfo.cache);
    layRect = &info->layInfo.rect;
    *layRect = bodyRect;
    info->layInfo.lFlags.erase = 0;
    if (_NXGetShlibVersion() <= MINOR_VERS_1_0) {
	info->layInfo.lFlags.horizCanGrow = 0;
	info->layInfo.lFlags.vertCanGrow = 0;
    }
    else { 
	info->layInfo.lFlags.horizCanGrow = tFlags.horizResizable;
	info->layInfo.lFlags.vertCanGrow = tFlags.vertResizable;
    }
    info->layInfo.lFlags.resetCache = 1;
    info->globals.gFlags.vertOrHoriz = 0;
    info->globals.gFlags.plusMinus = 0;
    cp0 = sp0.cp;
    cpN = spN.cp;
    sp0.cp = -1;
    theBreaks->chunk.used = 0;
    lastHt = -1.0;
    lastCPos = 0;
    maxX = 0.0;
    clipped = 0;
    ((NXTextInfo *)(_info))->globals.gFlags.xyshowOk = YES;	/* optimization - see NXDrawALine() */
    if (canDraw) {
	if ([self getVisibleRect:&vRect]) {
	    eRect = vRect;
	    NXIntersectionRect(&bodyRect, &vRect);
	    [self lockFocus];
	    top = vRect.origin.y;
	    bottom = top + vRect.size.height;
	    if (vFlags.opaque) {
	        NXIntersectionRect(&bounds, &eRect);
		_NXTextBackgroundRect(self, &eRect, NO, [self shouldDrawColor]);
	    }
	} else
	    canDraw = NO;
    }
    if(canDraw)
	_NXResetDrawCache(self);
    while (1) {
	NXCoord newBottom = NX_MAXY(layRect);
	if (bottom < newBottom)
	    layRect->size.height -= (newBottom - bottom);
	if (scanFunc(self, &info->layInfo) && canDraw && !clipped) {
	    clipped++;
	    PSgsave();
	    PSrectclip(vRect.origin.x, vRect.origin.y,
		       vRect.size.width, vRect.size.height);
	    ((NXTextInfo *)(_info))->globals.gFlags.xyshowOk = NO;
	}
	info->layInfo.lFlags.resetCache = 0;
	if ((cw = info->layInfo.cache.curPos - lastCPos) > TXMCHARS) {
	    if (canDraw) {
		[self unlockFocus];
		if (clipped > 0)
		    PSgrestore();
	    }
	    NX_RAISE(NX_longLine, 0, 0);
	}
	if (canDraw && (layRect->origin.y < bottom)
	    && (NX_MAXY(layRect) > top))
	    drawFunc(self, &info->layInfo);
	if (info->layInfo.width > maxX)
	    maxX = info->layInfo.width;
	needs = sizeof(NXLineDesc);
	if (layRect->size.height != lastHt)
	    needs += sizeof(NXHeightInfo);
	used = theBreaks->chunk.used;
	if ((used + needs) >= theBreaks->chunk.allocated)
	/*
	 * depends on error handler 
	 */
	    theBreaks = (NXBreakArray *)NXChunkRealloc(&theBreaks->chunk);
	ps = TX_LINEAT(theBreaks->breaks, used);
	if (info->layInfo.lFlags.endsParagraph)
	    cw |= TXMPARA;
	if (needs == sizeof(NXLineDesc))
	    *ps = cw;
	else {
	    HTCHANGE(ps)->lineDesc =
	      HTCHANGE(ps)->heightInfo.lineDesc = TXYESCHANGE(cw);
	    HTCHANGE(ps)->heightInfo.oldHeight = lastHt;
	    lastHt = layRect->size.height;
	    HTCHANGE(ps)->heightInfo.newHeight = lastHt;
	};
	layRect->origin.y += lastHt;
	theBreaks->chunk.used += needs;
	lastCPos = info->layInfo.cache.curPos;
	if (NXAdjustTextCache(self, &info->layInfo.cache, lastCPos) < 0)
	    break;
    }
    maxY = layRect->origin.y - bodyRect.origin.y;
    if (cp0 >= 0)
	[self setSel:cp0:cpN];
    if (canDraw) {
	if (clipped > 0)
	    PSgrestore();
	if (FLUSHWINDOW(window))
	    PSflushgraphics();
	[self unlockFocus];
    }
    retval = 1;
    if (superview && tFlags.inClipView &&
	tFlags.vertResizable && !tFlags.horizResizable) {
	    NXRect superFrame;
	    [superview getFrame:&superFrame];
	    [self setMinSize:&superFrame.size];
	    [self sizeToFit];
    }

    EPILOG(info);
    return retval;
}

- (void *)calcParagraphStyle:fontId : (int)alignment
{

    NXCoord newAscent, newDescent, newHeight;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXZone *zone = [self zone];

    _NXTextFontInfoInView(self, fontId, &newAscent, &newDescent, &newHeight);
    [self removeDefaultStyle];
    globals->defaultStyle = _NXMakeDefaultStyle(zone, 
	alignment, newHeight, newDescent, fontId, globals->defaultStyle);
    [self addDefaultStyle];
    return globals->uniqueStyle;
}

- (BOOL)charWrap
{
    return (tFlags.charWrap);
}

- (NXCoord)descentLine
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    return globals->defaultStyle->descentLine;
}



- getMarginLeft:(NXCoord *)leftMargin right:(NXCoord *)rightMargin 
    top:(NXCoord *)topMargin bottom:(NXCoord *)bottomMargin
{
    *leftMargin = bodyRect.origin.x - bounds.origin.x;
    *topMargin = bodyRect.origin.y - bounds.origin.y;
    *rightMargin = bounds.size.width - (bodyRect.size.width + *leftMargin);
    *bottomMargin = bounds.size.height - (bodyRect.size.height + *topMargin);
    return self;
}


- getMinWidth:(NXCoord *)width minHeight:(NXCoord *)height
    maxWidth:(NXCoord)widthMax maxHeight:(NXCoord)heightMax
{
    NXCoord    maxWidth;
    NXCoord    left = 0;
    NXCoord    right;
    NXCoord    twidth;
    NXCoord    tw;
    NXCoord    tx;
    NXCoord    th;
    NXRect              newFrame;
    NXSize              tSize;
    int        alignment;

    [self suspendNotifyAncestorWhenFrameChanged:YES];
    th = bounds.size.height - bodyRect.size.height;

    tw = bounds.size.width - bodyRect.size.width;
    tx = bounds.origin.x - bodyRect.origin.x;

    newFrame.size.width = frame.size.width;
    newFrame.origin.x = frame.origin.x;

    if (width) {
	twidth = maxX;
	maxWidth = ceil(twidth);
	alignment = [self alignment];
	while (1) {
	    right = maxWidth;
	    switch (alignment) {
	    case NX_LEFTALIGNED:
		left = 0.0;
		right -= twidth;
		break;
	    case NX_RIGHTALIGNED:
		left = right - twidth;
		right = 0.0;
		break;
	    case NX_CENTERED:
		left = floor((right - twidth) / 2.0);
		right -= (twidth + left);
		break;
	    case NX_JUSTIFIED:
		break;
	    }
	    if (left >= 0.0 && right >= 0.0)
		break;
	    maxWidth += 1.0;
	}

	maxWidth += alignment == NX_CENTERED ? 4.0 : 1.0;
	if (maxWidth + tw > widthMax)
	    maxWidth = widthMax - tw;
	left = bodyRect.origin.x;
	switch (alignment) {
	case NX_RIGHTALIGNED:
	    break;
	case NX_CENTERED:
	    left -= ceil((maxWidth - bodyRect.size.width) / 2.0);
	    break;
	case NX_JUSTIFIED:
	    break;
	}
	newFrame.size.width = maxWidth + tw;
	newFrame.origin.x = frame.origin.x + (left + tx - bounds.origin.x);
    }
    newFrame.origin.y = frame.origin.y;
    newFrame.size.height = maxY + th;
    if (newFrame.size.height > heightMax)
	newFrame.size.height = heightMax;

    if (newFrame.origin.x != frame.origin.x)
	[self moveTo:newFrame.origin.x:newFrame.origin.y];
    tSize = newFrame.size;
    _NXMinSize(self, &tSize);
    [self sizeTo:tSize.width:tSize.height];
    *height = bounds.size.height;
    if (width)
	*width = bounds.size.width;
    [self suspendNotifyAncestorWhenFrameChanged:NO];
    [self updateRuler];
    return self;
}

- (NXCoord)lineHeight
{
 /*
  * cheapo implementation for case where there is just one paragraph
  * paraStyle 
  */
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    return globals->defaultStyle->lineHt;
}


- (void *)defaultParaStyle
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    return globals->defaultStyle;
}


- setAlignment:(int)mode
{
 /*
  * cheapo implementation for case where there is just one paragraph
  * paraStyle 
  */
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    [self removeDefaultStyle];
    globals->defaultStyle->alignment = mode;
    [self addDefaultStyle];
    return self;
}

- setCharWrap:(BOOL)flag
{
    tFlags.charWrap = flag ? YES : NO;
    return self;
}

- setDescentLine:(NXCoord)value
{
 /*
  * cheapo implementation for case where there is just one paragraph
  * paraStyle 
  */
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    [self removeDefaultStyle];
    globals->defaultStyle->descentLine = value;
    [self addDefaultStyle];
    return self;
}


- setLineHeight:(NXCoord)value
{
 /*
  *  cheapo implementation for case where there is just one paragraph paraStyle
  */
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    [self removeDefaultStyle];
    globals->defaultStyle->lineHt = value;
    [self addDefaultStyle];
    return self;
}


- setNoWrap
{
    charCategoryTable = NXEnglishCharCatTable;
    breakTable = NXEnglishNoBreakTable;
    clickTable = NXEnglishClickTable;
    tFlags.charWrap = NO;
    [self setAlignment:NX_LEFTALIGNED];
    return self;
}

- setMarginLeft:(NXCoord)leftMargin right:(NXCoord)rightMargin 
    top:(NXCoord)topMargin bottom:(NXCoord)bottomMargin
{
    NXCoord dw = leftMargin + rightMargin;
    NXCoord dh = topMargin + bottomMargin;
    bodyRect.origin.x = bounds.origin.x + leftMargin;
    bodyRect.origin.y = bounds.origin.y + topMargin;
    bodyRect.size.width = bounds.size.width - dw;
    bodyRect.size.height = bounds.size.height - dh;
    return self;
}

- setParaStyle:(void *)paraStyle
{
    return [self _setFont:nil paraStyle:paraStyle];
}

@end

@implementation Text (Drawing)

- drawSelf:(const NXRect *)rects :(int)rectCount
{

    int cPos;
    NXRect cRect, vRect;
    NXCoord top, bottom, yPos, ht = 0.0, mbnd;
    BOOL mustClip = NO;
    NXTextInfo *info = _info;
    
    PROLOG(info);
    info->globals.gFlags.inDrawSelf = YES;
    if (info->globals.gFlags.retainedWhileDrawing && NXDrawingStatus == NX_DRAWING)
	PSsetwindowtype(NX_RETAINED, [window windowNum]);
    _NXResetDrawCache(self);
    {
	int        cw;
	NXLineDesc *ps;
	NXLineDesc *plb;

	cRect = rects[0];
	if (!NXIntersectionRect(&bounds, &cRect))
	    goto exit1;
	[self getVisibleRect:&vRect];
	if (!NXIntersectionRect(&cRect, &vRect))
	    goto exit1;
	cRect = vRect;
	if (tFlags._caretState != CARETDISABLED &&
	    NXDrawingStatus == NX_DRAWING)
	    [self hideCaret];
	top = cRect.origin.y;
	bottom = top + cRect.size.height;

	cPos = 0;
	yPos = bodyRect.origin.y;
	ps = theBreaks->breaks;
	plb = TX_LINEAT(ps, theBreaks->chunk.used);
	while (1) {
	    cw = *ps++;
	    if (cw < 0) {
		ht = ((NXHeightInfo *)ps)->newHeight;
		ps = TX_LINEAT(ps, sizeof(NXHeightInfo));
	    }
	    yPos += ht;
	    if (top < yPos || ps >= plb) {
		yPos -= ht;
		ps--;
		if (cw < 0)
		    ps = TX_LINEAT(ps, -sizeof(NXHeightInfo));
		info->layInfo.lFlags.endsParagraph = 1;
		if (ps != theBreaks->breaks && !TXISPARA(*--ps))
		    info->layInfo.lFlags.endsParagraph = 0;
		break;
	    }
	    cPos += TXGETCHARS(cw);
	}
    }

    {
	int        clipped;
	NXRect             *layRect;
	NXCoord             newBottom;

	clipped = (vFlags.noClip) ? 0 : -1;
	if (tFlags.inClipView)
	    clipped = -1;
	info->layInfo.left = cRect.origin.x;
	info->layInfo.right = info->layInfo.left + cRect.size.width;
	info->globals.gFlags.clipRight = 0;
	info->globals.gFlags.clipLeft = 0;
	info->layInfo.lFlags.erase = 0;
	info->layInfo.lFlags.horizCanGrow = info->layInfo.lFlags.vertCanGrow = 0;

	if (bodyRect.origin.x < info->layInfo.left) {
	    info->globals.gFlags.clipLeft++;
	    info->layInfo.left -=.5;
	    if (info->layInfo.left < bodyRect.origin.x)
		info->layInfo.left = bodyRect.origin.x;
	}
	if ((bodyRect.origin.x + bodyRect.size.width) > info->layInfo.right) {
	    info->globals.gFlags.clipRight++;
	    info->layInfo.right +=.5;
	    if (info->layInfo.right > bodyRect.origin.x + bodyRect.size.width)
		info->layInfo.right = bodyRect.origin.x + bodyRect.size.width;
	}
	if (vFlags.opaque) {
	    _NXTextBackgroundRect(self, &vRect, NO, [self shouldDrawColor]);
	}
    /*
     * if (maxY > bodyRect.size.height) { if (clipped >= 0) { clipped++;
     * PSgsave(); PSrectclip(vRect.origin.x, vRect.origin.y,
     * vRect.size.width, vRect.size.height); } } 
     */
	NXIntersectionRect(&bodyRect, &cRect);
	bottom = NX_MAXY(&cRect);
	top = cRect.origin.y;
	mbnd = cRect.origin.y + cRect.size.height;
	NXAdjustTextCache(self, &info->layInfo.cache, cPos);
	layRect = &info->layInfo.rect;
	*layRect = bodyRect;
	layRect->origin.y = yPos;
	layRect->size.height = ht;
	info->globals.gFlags.vertOrHoriz = 0;
	info->globals.gFlags.plusMinus = 0;
	info->layInfo.textClipRect = &cRect;
	info->layInfo.lFlags.resetCache = 1;
	while (1) {
	    newBottom = NX_MAXY(layRect);
	    if (bottom < newBottom)
		layRect->size.height -= (newBottom - bottom);
	    mustClip = (scanFunc(self, &info->layInfo)
			|| (NX_MAXY(layRect) > mbnd)
			|| (layRect->origin.y < top));
	    info->layInfo.lFlags.resetCache = 0;
	    if (mustClip && !clipped) {
		clipped++;
		PSgsave();
		PSrectclip(cRect.origin.x, cRect.origin.y,
			   cRect.size.width, cRect.size.height);
		((NXTextInfo *)(_info))->globals.gFlags.xyshowOk = NO;
	    }
	    (void)drawFunc(self, &info->layInfo);
	    layRect->origin.y += layRect->size.height;
	    if (layRect->origin.y >= bottom)
		break;
	    if (NXAdjustTextCache(
		self, &info->layInfo.cache, info->layInfo.cache.curPos) < 0)
		break;
	}
	info->layInfo.textClipRect = NULL;
	if (clipped > 0)
	    PSgrestore();
    }

  exit1:
    if (info->globals.gFlags.retainedWhileDrawing && NXDrawingStatus == NX_DRAWING)
	PSsetwindowtype(NX_BUFFERED, [window windowNum]);
    if (sp0.cp >= 0 && (NXDrawingStatus == NX_DRAWING || NXScreenDump))
	[self showCaret];
    info->globals.gFlags.inDrawSelf = NO;
    EPILOG(info);
    return (self);
}

@end

@implementation Text (LinePosition)

- (int)positionFromLine:(int)line
{
    NXLineDesc *ps = theBreaks->breaks;
    NXLineDesc *lps = TX_LINEAT(ps, theBreaks->chunk.used);
    int        pos = 0;
    int        nLines = 1;
    int        cw;
    int                 isPara;

    while (ps < lps) {
	cw = *ps++;
	if (cw < 0)
	    ps = TX_LINEAT(ps, sizeof(NXHeightInfo));
	isPara = TXISPARA(cw);
	cw &= TXMCHARS;
	if (nLines == line) {
	    return pos;
	}
	if (isPara)
	    nLines++;
	pos += cw;
    }
    return -1;

}

- (int)lineFromPosition:(int)position
{
    NXLineDesc *ps = theBreaks->breaks;
    NXLineDesc *lps = TX_LINEAT(ps, theBreaks->chunk.used);
    int        pos = 0;
    int        nLines = 1;
    short               cw;
    int                 isPara = 0;
    int	       eot = textLength;
    
    while (ps < lps) {
	if (pos == position)
	    break;
	cw = *ps++;
	if (pos > position) {
	    if (isPara)
		nLines--;
	    break;
	}
	isPara = TXISPARA(cw);
	pos += (cw & TXMCHARS);
	if (isPara && pos < eot)
	    nLines++;
	if (cw < 0)
	    ps = TX_LINEAT(ps, sizeof(NXHeightInfo));
    }
    return nLines;

}

@end

@implementation Text (FrameRect)

- setMaxSize:(const NXSize *)newMaxSize
{
    maxSize = *newMaxSize;
    return self;
}

- getMaxSize:(NXSize *)theSize
{
    *theSize = maxSize;
    return self;
}


- setMinSize:(const NXSize *)newMinSize
{
    NXTextInfo *info = _info;
    minSize = *newMinSize;
    info->globals.minBodyWidth = minSize.width -
	(bounds.size.width - bodyRect.size.width);
    info->globals.minBodyHeight = minSize.width -
	(bounds.size.height - bodyRect.size.height);
    return self;
}

- getMinSize:(NXSize *)theSize
{
    *theSize = minSize;
    return self;
}

- setVertResizable:(BOOL)flag
{
    tFlags.vertResizable = (flag) ? YES : NO;
    return self;
}

- (BOOL)isVertResizable
{
    return (tFlags.vertResizable);
}

- setHorizResizable:(BOOL)flag
{
    tFlags.horizResizable = (flag) ? YES : NO;
    return self;
}

- (BOOL)isHorizResizable
{
    return (tFlags.horizResizable);
}

- moveTo:(NXCoord)x :(NXCoord)y
{
    NXCoord    tw, tx, th, ty, deltaX, deltaY;
    NXTextGlobals *globals = TEXTGLOBALS(_info);

/* !!! how to do this if not vFlags.drawInSuperview? */

    tw = bounds.size.width - bodyRect.size.width;
    tx = bodyRect.origin.x - bounds.origin.x;
    th = bounds.size.height - bodyRect.size.height;
    ty = bodyRect.origin.y - bounds.origin.y;
    deltaX = x - frame.origin.x;
    deltaY = y - frame.origin.y;
    sp0.x += deltaX;
    sp0.y += deltaY;
    spN.x += deltaX;
    spN.y += deltaY;
    if (globals->caretXPos >= 0.0)
	globals->caretXPos += deltaX;
    [super moveTo:x :y];
    bodyRect = bounds;
    bodyRect.origin.x += tx;
    bodyRect.size.width -= tw;
    bodyRect.origin.y += ty;
    bodyRect.size.height -= th;
    return self;
}

- resizeText:(const NXRect *)oldBounds :(const NXRect *)maxRect
{
    NXRect    *Bounds;
    const NXRect *maxBounds;
    NXRect    *pRs;
    NXRect              uRects[5];
    NXRect              invalRect;
    NXCoord    x;
    NXCoord    width;
    int        nRects;
    int                 i;

    nRects = 0;
    pRs = &uRects[1];
    if (delegateMethods & TXMGROWACTION) {
	if ((int)[delegate textDidResize:self oldBounds:oldBounds invalid:&invalRect]) {
	    nRects++;
	    *pRs = invalRect;
	    pRs++;
	}
    }
    maxBounds = maxRect;
    Bounds = &bounds;
    if (maxBounds->size.height > Bounds->size.height) {
	nRects++;
	*pRs = *Bounds;
	pRs->origin.y += Bounds->size.height;
	pRs->size.height = (maxBounds->size.height - Bounds->size.height);
	pRs++;
    }
    x = Bounds->origin.x - maxBounds->origin.x;
    if (x > 0.0) {
	nRects++;
	*pRs = *maxBounds;
	pRs->size.width = x;
	pRs++;
    }
    x = Bounds->origin.x + Bounds->size.width;
    width = maxBounds->origin.x + maxBounds->size.width - x;
    if (width > 0.0) {
	nRects++;
	*pRs = *maxBounds;
	pRs->origin.x = x;
	pRs->size.width = width;
	pRs++;
    }
    if (!vFlags.drawInSuperview) {
	for (i = 1; i <= nRects; i++) {
	    [_drawMatrix transformRect:&uRects[i]];
	    [_frameMatrix transformRect:&uRects[i]];
	}
    }
    if (nRects) {
	pRs = &uRects[1];
	if (nRects != 1) {
	    nRects++;
	    pRs--;
	    pRs[0] = pRs[1];
	    (void)NXUnionRect(pRs + 2, pRs);
	}
	[superview displayFromOpaqueAncestor:pRs :(nRects > 3) ? 3 : nRects :NO];
	nRects -= 3;
	if (nRects >= 1) {
	    pRs = &uRects[3];
	    if (nRects != 1) {
		nRects++;
		pRs--;
		pRs[0] = pRs[1];
		(void)NXUnionRect(pRs + 2, pRs);
	    }
	    [superview displayFromOpaqueAncestor:pRs :nRects :NO];
	}
    }
    return self;
}

- sizeTo:(NXCoord)width :(NXCoord)height
{
    NXCoord    tw, tx, th, ty;
    NXTextGlobals *globals = TEXTGLOBALS(_info);

/* !!! how to do this if _drawMatrix? */

    tw = bounds.size.width - bodyRect.size.width;
    tx = bodyRect.origin.x - bounds.origin.x;
    th = bounds.size.height - bodyRect.size.height;
    ty = bodyRect.origin.y - bounds.origin.y;
    globals->caretXPos = -1.0;

    [super sizeTo:width :height];
    bodyRect = bounds;
    bodyRect.origin.x += tx;
    bodyRect.size.width -= tw;
    bodyRect.origin.y += ty;
    bodyRect.size.height -= th;
    return self;
}

- sizeToFit
{
    NXCoord maxW = tFlags.horizResizable ? 1.0e37 : bounds.size.width;
    NXCoord maxH = tFlags.vertResizable ? 1.0e37 : bounds.size.height;
    NXCoord minW, minH;

    [self getMinWidth:&minW minHeight:&minH maxWidth:maxW maxHeight:maxH];
    return self;
}




@end

/* 
83
--
  4/25/90 bgy	removed the early exit in calcLine if there was no visible rect

86
--
  5/17/90 chris	modified calcLine to erase bounds, not bodyRect.  this is to
  		ensure erasing overhanging characters.

87
--
 7/12/90 glc	added _NXTextBackgroundRect utility call for erasing 
		rectangles.

91
--
 8/12/90 aozer	Changed isOnColorScreen -> shouldDrawColor

92
--
 8/20/90 gcockrft  Don't messup position when changing to right aligned.
 		   update the ruler on a resize.
 8/20/90 bryan	support for retainedWhileDrawing

93
--
9/2/90 trey	dont use PSsetwindowtype when printing

96
--
10/10/90 glc	Set correct resizing flags in calcLine

106
---
11/13/90 glc	Only do above change if not 1.0.


*/

/*
	textScanDraw.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Cell.h"
#import "Text.h"
#import "textWraps.h"
#import "View_Private.h"
#import "textprivate.h"
#import <math.h>
#import <dpsclient/wraps.h>
#import "nextstd.h"

extern void _NXTextBackgroundRect(Text *self, NXRect *rectp, BOOL useCache, BOOL colorScreen);

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryScanDraw=0\n");
    asm(".globl .NXTextCategoryScanDraw\n");
#endif

@interface Text(ScanDraw)
- drawSelection:(NXLayInfo *)layInfo layRect:(NXRect *)layRect;
- (NXCoord)moveCharWidth:(NXLayInfo *)layInfo globals:(NXTextGlobals *)globals
    pos:(int *)pos wordPos:(int)wordPos width:(NXCoord)width maxwidth:(NXCoord)maxwidth;
- (int) feedMe:(NXLayInfo *)layInfo globals:(NXTextGlobals *)globals
    pos:(int)cPos arrayCount:(int) nciArray blockCount:(int)nciBlock;
- (NXCoord) fixScreenWidths:(NXLayInfo *)layInfo globals:(NXTextGlobals *)globals;
- safeSetFont:font;
@end

    static NXLay *addLay(NXLayInfo *layInfo, int runPos, int cPos, 
	NXZone *zone);
typedef struct {
    @defs (Text)
} TextId;

#define HAVE_TABS(style) (((NXTextStyle *)style)->tabs)
#define LEFT_JUST(style) (((NXTextStyle *)style)->alignment == NX_LEFTALIGNED)
#define VALID_TABS(run) (HAVE_TABS(run->paraStyle) && LEFT_JUST(run->paraStyle))
@implementation Text(ScanDraw)

- (BOOL)_canOptimizeDrawing
{
    return CANUSEXYSHOW(_info) && [super _canOptimizeDrawing];
}

- _allowXYShowOptimization:(BOOL)flag
{
    ((NXTextInfo *)(_info))->globals.gFlags.canUseXYShow = flag ? YES : NO;
    return self;
}

#define CBITS 10
#define CVAL ((1 << CBITS) - 1)
#define VAL(color,n) ((color & (CVAL << (CBITS*n))) >> (CBITS*n))
#define RED(color) VAL(color,2)
#define GREEN(color) VAL(color,1)
#define BLUE(color) VAL(color,0)

/* Invalidates a color before use if we are not on colorscreen */
#define COL(Color, colorscreen) (colorscreen ? Color : -1)

#define TOFLOAT(v)  ((float)v / CVAL)

/* 
 * Convert out from the packed RGB format to a NXColor.
 */
NXColor _NXFromTextRGBColor(int color)
{
    return(NXConvertRGBAToColor(TOFLOAT(RED(color)), TOFLOAT(GREEN(color)),
			    TOFLOAT(BLUE(color)),1.0));
}

/* 
 * Convert to the packed RGB format from an NXColor.
 */
int _NXToTextRGBColor(NXColor color)
{
    float	red,green,blue,alpha;
    int		ired,igreen,iblue,RGBColor;
    
    NXConvertColorToRGBA (color, &red, &green, &blue, &alpha);
    ired = red * CVAL;
    igreen = green * CVAL;
    iblue = blue * CVAL;
    RGBColor = (ired << (CBITS*2)) | (igreen << CBITS) | iblue;
    return(RGBColor);
}

/*
 * Clear a rectangle with the Text background color or gray.
 * Check and update the cache if so desired 
 */
void _NXTextBackgroundRect(Text *self, NXRect *rectp, BOOL useCache, BOOL colorScreen)
{
NXTextGlobals *globals = &((NXTextInfo *)(((TextId *)self)->_info))->globals;
NXDrawCache *drawCache = &globals->drawCache;
int		color = 0;

    if(useCache) {
	if(colorScreen && globals->backgroundRGBColor >= 0) {
	    if(drawCache->Color != globals->backgroundRGBColor) {
		color = drawCache->Color = globals->backgroundRGBColor;
		drawCache->Gray = -1;
	    		PSsetrgbcolor (TOFLOAT(RED(color)), TOFLOAT(GREEN(color)),
				TOFLOAT(BLUE(color)));
	    }
	}
	else {
	    if(drawCache->Gray != ((TextId*)self)->backgroundGray) {
		drawCache->Color = -1;
		drawCache->Gray = ((TextId*)self)->backgroundGray;
		PSsetgray(drawCache->Gray);
	    }
	}
    }
    else {
	/* blowup cache */
	drawCache->Color = -1;
	drawCache->Gray = -1;
	if(colorScreen && globals->backgroundRGBColor >= 0) {
	    color = globals->backgroundRGBColor;
	    PSsetrgbcolor (TOFLOAT(RED(color)), TOFLOAT(GREEN(color)),
			    TOFLOAT(BLUE(color)));
	}
	else
	    PSsetgray(((TextId *)self)->backgroundGray);
    }
    NXRectFill(rectp);		
}

/*
 * Set the current ink to the passed gray or color. Update cache of ink 
 */
void static cacheInk(Text *self, float gray, int color) 
{
NXTextGlobals *globals = &((NXTextInfo *)(((TextId *)self)->_info))->globals;
NXDrawCache *drawCache = &globals->drawCache;

    if(color >= 0) {
	if(drawCache->Color != color) {
	    drawCache->Color = color;
	    drawCache->Gray = -1;
	    PSsetrgbcolor (TOFLOAT(RED(color)), TOFLOAT(GREEN(color)),
			    TOFLOAT(BLUE(color)));
	}
    }
    else {
	if(drawCache->Gray != gray) {
	    drawCache->Color = -1;
	    drawCache->Gray = gray;
	    PSsetgray(gray);
	}
    }
}

void static cacheFont(Text *self,Font *font)
{
    NXDrawCache *drawCache = DRAWCACHE(((TextId *)self)->_info);

    if (font == drawCache->font)
	return;
    [self safeSetFont:font];
    drawCache->font = font;
}

extern void _NXResetDrawCache(id _self)
{
    TextId    *self = (TextId *) _self;
    NXDrawCache        *drawCache = DRAWCACHE(self->_info);

    drawCache->font = nil;
    drawCache->Gray = -1.0;
    drawCache->Color = -2;  /* invalid color */
}

- safeSetFont:font
{
    BOOL drawing = (NXDrawingStatus == NX_DRAWING);
    id otherFont;
    if (!font)
	return self;
    if (drawing == ISSCREENFONT(font)) {
	[font set];
	return self;
    }
    otherFont = FONTTOSCREEN(font);
    if (otherFont)
	[otherFont set];
    else if (drawing)
	[font set];
    else 
	[[Text getDefaultFont] set];
    return self;
}


extern void _NXRestoreDrawCache(id _self)
{
    TextId    *self = (TextId *) _self;
    NXDrawCache        *drawCache = DRAWCACHE(self->_info);
    int		color;

    if (drawCache->font)
	[_self safeSetFont:drawCache->font];
	
    if(drawCache->Color >= 0) {
	color = drawCache->Color;
	PSsetrgbcolor (TOFLOAT(RED(color)), TOFLOAT(GREEN(color)),
			TOFLOAT(BLUE(color)));
	}
    else if(drawCache->Gray >= 0)
	    PSsetgray(drawCache->Gray);
}


- drawUnderline:(NXLayInfo *)layInfo layRect:(NXRect *)layRect colorscreen:(BOOL) colorScreen
{
    NXLay *lay, *last, *first;
    NXRect rect;
    NXCoord lastGray = -1.0;
    int		lastColor = -2; /* invalid color */
    NXRun *run;
    unsigned char lastSubscript;
    
    rect.size.height = 1.0;
    lay = layInfo->lays->lays;
    last = TX_LAYAT(lay, layInfo->lays->chunk.used);
    for (; lay < last; lay++) {
	if (lay->run->rFlags.underline) {
	    first = lay;
	    run = lay->run;
	    lastGray = run->textGray;
	    lastColor = run->textRGBColor;
	    lastSubscript = run->subscript;
	    for (;lay < (last - 1); lay++) {
		run = lay->run;
		if (!run->rFlags.underline || run->textGray != lastGray ||
				run->textRGBColor != lastColor || 
					run->subscript != lastSubscript)
		    break;
	    }
	    if (lay > first) {
		rect.origin.x = NX_X(layRect) + first->x;
		rect.origin.y = 
		    NX_MAXY(layRect) - layInfo->descent + 1.0 + lastSubscript;
		rect.size.width = lay->x - first->x;
		cacheInk(self, lastGray, COL(lastColor, colorScreen));
		NXRectFill(&rect);
		if(lay->run->rFlags.underline && lay < (last - 1))
		    lay--;  /* Still need to process this guy color change */
	    }
	}
    }
    return self;
}

- drawSelection:(NXLayInfo *)layInfo layRect:(NXRect *)layRect
{
 /* draws the current selection, if its not just an insertion point */
    NXRect              hRect;
    NXCoord             xPos, rtPos;

    if ((sp0.cp >= 0) &&
	((sp0.cp != spN.cp) ||
	 (sp0.line != spN.line))) {
	xPos = layInfo->lays->lays->x;
	rtPos = NX_MAXX(layRect) - layInfo->rightIndent;
	hRect = *layRect;
	hRect.origin.x = (xPos < layInfo->left) ?
	  xPos : layInfo->left;
	hRect.size.width = (rtPos > layInfo->right ?
			    rtPos : layInfo->right) - hRect.origin.x;
	if (layInfo->textClipRect)
	    NXIntersectionRect(layInfo->textClipRect, &hRect);
	[self _hilite:NO :&sp0:&spN:&hRect];
    }
    return self;
}


- (NXCoord)moveCharWidth:(NXLayInfo *)layInfo globals:(NXTextGlobals *)globals
    pos:(int *)pos wordPos:(int)wordPos width:(NXCoord)width maxwidth:(NXCoord)maxwidth
{

    NXCoord *pwa, *pxa, newwidth, tabWidth;
    int c, cPos = *pos;
    unsigned char *cp;
    NXLay *pl = 0;
    NXLay *lays;
    NXTabStop *pt;

    pwa = layInfo->widths->widths + wordPos;
    if (globals->xPos->chunk.allocated < layInfo->widths->chunk.allocated) 
	globals->xPos = (NXWidthArray *) NXChunkGrow(
	    &globals->xPos->chunk, layInfo->widths->chunk.allocated);
    pxa = globals->xPos->widths + wordPos;
    cp = layInfo->chars->text;
    lays = layInfo->lays->lays;

    while (wordPos < cPos) {
	c = cp[wordPos];
	if (globals->tabsPending) {
	    pl = TX_LAYAT(lays, globals->tabLay);
	    while (pl->offset < wordPos)
		pl++;
	}
	if (MOVECHAR(c)|| (pl->offset == wordPos && pl->run->rFlags.graphic)) {
	    globals->tabsPending--;
	    globals->tabLay = TX_BYTES(pl, lays);
	    tabWidth = 0.0;
	    if (c == '\t' && VALID_TABS(pl->run)) {
		for (pt = globals->tabs; pt < globals->tabsEnd; pt++) {
		    if (width < pt->x) {
			newwidth = pt->x;
			tabWidth = newwidth - width;
			*pwa++ = tabWidth;
			width = newwidth;
		   /*   tabs are past margin, and we'll consume at least 1 char. */
			if (newwidth > maxwidth && wordPos) { 
			    *pos = wordPos;
			    globals->tabsPending = 0;
			    layInfo->lays->chunk.used = TX_BYTES(pl, lays);
			    cPos = wordPos;
			}
			goto breakWhile;
		    }
		}
		/* no more tabs left, so introduce a line break */
		if (!wordPos) {
		    width += *pwa;
		    *pos = 1;
		} else {
#define BIGNUMBER 1.0e30			
		    width = maxwidth + BIGNUMBER;
		    *pos = wordPos;
		}
		layInfo->lays->chunk.used = TX_BYTES(pl, lays);
		globals->tabsPending = 0;
		cPos = wordPos;
	breakWhile:
		globals->tabs = pt;
		pl->x = width;
	    } else {
		width += *pwa++;
		pl->x = width;
	    }
	    pl->lFlags.isMoveChar = YES;
	} else
	    width += *pwa++;
	*pxa++ = width;
	wordPos++;
    }
    return (width);
}

- (int) feedMe:(NXLayInfo *)layInfo globals:(NXTextGlobals *)globals
    pos:(int)cPos arrayCount:(int) nciArray blockCount:(int)nciBlock
{
    unsigned char *cp;

    {
	NXCharArray *ta;

	layInfo->chars->chunk.used = cPos;
    /* depends on error handler */
	layInfo->chars = ta =
	  (NXCharArray *)NXChunkRealloc(&layInfo->chars->chunk);
	cp = (unsigned char *)ta->text;
	nciArray = ta->chunk.allocated;
    }


    layInfo->widths->chunk.used = cPos * sizeof(NXCoord);
    globals->xPos->chunk.used = cPos * sizeof(NXCoord);
 /* depends on error handler */
    layInfo->widths = (NXWidthArray *)NXChunkRealloc(
	&layInfo->widths->chunk);
    if (globals->xPos->chunk.allocated < layInfo->widths->chunk.allocated) 
	globals->xPos = (NXWidthArray *) NXChunkGrow(
	    &globals->xPos->chunk, layInfo->widths->chunk.allocated);

    {
	unsigned char *sp;
	int        c;
	int        cc;

	c = cPos;
	cp += cPos;
	while (c < nciArray) {
	    if (!nciBlock) {
		if (!layInfo->cache.curBlock->next) {
		    nciArray = c;
		    break;
		}
		layInfo->cache.blockFirstPos += layInfo->cache.curBlock->chars;
		layInfo->cache.curBlock = layInfo->cache.curBlock->next;
		nciBlock = layInfo->cache.curBlock->chars;
	    }
	    sp = (unsigned char *)layInfo->cache.curBlock->text +
	      layInfo->cache.curBlock->chars - nciBlock;
	    cc = nciBlock;
	    if (cc > nciArray - c)
		cc = nciArray - c;
	    c += cc;
	    nciBlock -= cc;
	    while (cc) {
		cc--;
		*cp++ = *sp++;
	    }
	}
    }
    return (nciBlock);
}

- (NXCoord) fixScreenWidths:(NXLayInfo *)layInfo 
	            globals:(NXTextGlobals *)globals
{
    NXLay *pl, *plast,*pend;
    NXCoord *pwa, *pxa, *pw, size, width, curWidth = 0.0, ypos, lastGray = 0.;
    int	lastRGBColor = -2;  /* invalid color */
    unsigned char lastSubscript = 0;
    unsigned char *cp, ch;
    int isScreen, count;
    id font;
    NXRun *run;
    BOOL sawUnderline = NO;
    NXCoord trueWidth;
    
    ypos = NX_MAXY(&layInfo->rect) - layInfo->descent;
    pwa = layInfo->widths->widths;
    pxa = globals->xPos->widths;
    cp = layInfo->chars->text;
    pl = layInfo->lays->lays;
    plast = TX_LAYAT(layInfo->lays->lays, layInfo->lays->chunk.used);
    pend = plast-1;
    curWidth = pl->x;
    trueWidth = curWidth;
    while (pl != plast) {
        if (pl == pend)
	    trueWidth = curWidth-trueWidth;
	font = pl->font;
	run = pl->run;
	if (pl->lFlags.mustMove)
	    curWidth = pl->x;
	if (sawUnderline && 
	    (!run->rFlags.underline || pl == (plast - 1) ||
	    lastGray != run->textGray || lastRGBColor != run->textRGBColor) ||
		lastSubscript != run->subscript) {
	    pl->x = curWidth;
	    lastGray = run->textGray;
	    lastRGBColor = run->textRGBColor;
	    lastSubscript = run->subscript;
	}
	if (!sawUnderline && run->rFlags.underline) {
	    pl->x = curWidth;
	    lastGray = run->textGray;
	    lastRGBColor = run->textRGBColor;
	    lastSubscript = run->subscript;
	}
	if (run->rFlags.graphic) {
	    pl->x = curWidth;
	    curWidth += *pwa++;
	    pxa++;
	    cp++;
	    if (globals->cellInfo) {
		NXTextCellInfo temp, *ptr;
		temp.cell = run->info;
		ptr = NXHashGet(globals->cellInfo, &temp);
		if (ptr) {
		    NXSize size;
		    [run->info calcCellSize:&size];
		    ptr->origin.x = pl->x + NX_X(&layInfo->rect);
		    ptr->origin.y = ypos + pl->y - ceil(size.height);
		}
	    }
	} else {
	    isScreen = ISSCREENFONT(font);
	    pw = FONTTOWIDTHS(font);
	    size = FONTTOSIZE(font);
	    count = pl->chars;
	    while (count) {
		ch = *cp++;
		if (ch == '\t' && VALID_TABS(run))
		    width = (*pxa > curWidth) ? 
			(*pxa - curWidth):(curWidth - *pxa);
		else 
		    width = isScreen ? pw[ch] : size * pw[ch];
		*pwa++ = width;
		curWidth += width;
		pxa++;
		count--;
	    }
	}
	sawUnderline = run->rFlags.underline;
	pl++;
    }
    return trueWidth;
}


@end

/*  Talk to bryan to make sure I understand fixScreenWidths and
    why he didn't use justification in lay adjustment loop.
 */
/*  Computes the left and right offsets for given justification,
 *  returns same result as NXScanALine:
 *  returns 1 if line spills over left or right end of line 
 */
static int doJustification(NXCoord width,
			   NXCoord maxwidth,
			   NXCoord padBy,
			   NXCoord indent,
			   int justification,
			   NXLayInfo *layInfo)
{
    NXCoord right,left = 0.0,delta;
    NXLay *pl, *plast;
    
    width = ceil(width);
    right = maxwidth + padBy;
    switch (justification) {
    case NX_LEFTALIGNED:
	left = indent;
	right -= width;
	break;
    case NX_RIGHTALIGNED:
	left = right - width;
	right = 0.0;
	break;
    case NX_CENTERED:
	left = floor((right - width) / 2.0);
	right -= (width + left);
	break;
    case NX_JUSTIFIED:
	break;
    }

    pl = layInfo->lays->lays;		/* correct lays X positions*/
    delta = left - pl->x;
    pl->x = left;
    if (!(LEFT_JUST(pl->run->paraStyle))) {
	plast = TX_LAYAT(pl, layInfo->lays->chunk.used);
	for (pl = pl + 1; pl < plast; pl++) {
	    if (pl->x != 0.0)
		pl->x += delta;
	}
    }
    layInfo->rightIndent = right;
    return ((left < 0.0 || right < 0) ? 1 : 0);
}

 /*
  * Our default line layout routine 
  *
  * cursors are already established into NXRunArray and NXTextBlock structure
  * for currentPos 
  *
  * returns 1 if line spills over left or right end of line 
  *
  */

extern int NXScanALine(id _self, NXLayInfo *layInfo)
{
    TextId *self = (TextId *) _self;
    int result, justification;
    NXTextGlobals *globals = TEXTGLOBALS(self->_info);
    NXRect *layRect = &layInfo->rect;
    NXTextCache *cache = &layInfo->cache;
    NXZone *zone = [_self zone];
    NXCoord padBy, indent;
    
    {
	int cPos, runPos, whitePos, wordPos = 0;
	int charWrap = 0;
	BOOL hadEscape, growHorizontally,textUsedUp = 0;
	NXCoord width, maxwidth;
	NXCoord oldwidth;
	id font, screenFont;

	{
	    NXTextStyle *ps;

	    ps = (NXTextStyle *)cache->curRun->paraStyle;
	    justification = ps->alignment;
	    oldwidth = width = indent =
	      (layInfo->lFlags.endsParagraph ?
	       ps->indent1st : ps->indent2nd);
	    globals->tabs = ps->tabs;
	    globals->tabsEnd = globals->tabs + ps->numTabs;
	}

	charWrap = self->tFlags.charWrap ? YES : NO;
	padBy = 1.0;
	if (justification == NX_CENTERED)
	    padBy = 4.0;
	maxwidth = ceil(NX_WIDTH(layRect) - padBy);
	growHorizontally = NO;
	if (layInfo->lFlags.horizCanGrow && !cache->curPos) {
	    growHorizontally = YES;
	    maxwidth = ceil(self->maxSize.width - padBy);
	} 
	{
	    const NXFSM     *state;
	    NXCoord   *pwa, psize, ssize;
	    unsigned char *cp;
	    int        cc;
	    int        nciArray;
	    int        nciRun;
	    int        lookAhead;
	    int                 nciBlock;
	    const unsigned char *chClass;
	    NXCoord            *pw, *psw = 0;

	    NXCoord    size;

	/*
	 * This is main loop : 1) If necessary, advance cursor in NXRunArray
	 * and cache run info.  At same time, write run info in
	 * layInfo->lays. 2) If necessary, advance cursor in NXTextBlock list
	 * and cache info 3) If necessary expand layInfo->chars and
	 * layInfo->widths. 4) Get next character and copy into
	 * layInfo->chars. 5) Get width of character and copy into
	 * layInfo->widths. 6) If adding this character exceeds
	 * layRect->size.width, break 7) Get state transition for current
	 * state and class of current character. 8) If this is a final state:
	 * a)  if this is newLine then break b)  New state in state 0 c)
	 * Start new word here  
	 *
	 */

	    font = cache->curRun->font;
	    screenFont = FONTTOSCREEN(font);
	    pw = FONTTOWIDTHS(font);
	    if (screenFont)
		psw = FONTTOWIDTHS(screenFont);
	    size = FONTTOSIZE(font);
	    pwa = layInfo->widths->widths;
	    nciRun = cache->runFirstPos + 
		cache->curRun->chars - cache->curPos;


	    state = self->breakTable;
	    chClass = self->charCategoryTable;
	    nciArray = layInfo->chars->chunk.allocated;
	    cp = layInfo->chars->text;

	    {
		unsigned char *sp;
		unsigned char *cpl;

		sp = (unsigned char *)cache->curBlock->text +
		    cache->curPos - cache->blockFirstPos;
		cPos = nciBlock = cache->blockFirstPos +
		    cache->curBlock->chars - cache->curPos;
		if (cPos > nciArray)
		    cPos = nciArray;
		nciBlock -= cPos;
		cpl = cp + cPos;
		while (cp < cpl) {
		    *cp++ = *sp++;
		}
		while (cPos < nciArray) {
		    if (!cache->curBlock->next) {
			nciArray = cPos;
			break;
		    }
		    cache->blockFirstPos += cache->curBlock->chars;
		    cache->curBlock = cache->curBlock->next;
		    sp = (unsigned char *)cache->curBlock->text;
		    cc = nciBlock = cache->curBlock->chars;
		    if (cc > nciArray - cPos)
			cc = nciArray - cPos;
		    cPos += cc;
		    nciBlock -= cc;
		    cpl = cp + cc;
		    while (cp < cpl) {
			*cp++ = *sp++;
		    }
		}
	    }
	    cp = layInfo->chars->text;
	    layInfo->lays->chunk.used = 0;
	    lookAhead = wordPos = whitePos = runPos = cPos = 0;
	    globals->tabsPending = 0;
	    globals->tabLay = 0;
	    hadEscape = NO;

	    while (1) {
		if (lookAhead) {
		    lookAhead--;
		    pwa++;
		    cc = cp[cPos];
		} else {
		    if (!nciRun) {
			if (runPos != cPos) {
			    addLay(layInfo, runPos, cPos, zone);
			    }
			cache->runFirstPos += cache->curRun->chars;
			cache->curRun++;
			font = cache->curRun->font;
			screenFont = FONTTOSCREEN(font);
			pw = FONTTOWIDTHS(font);
			if (screenFont)
			    psw = FONTTOWIDTHS(screenFont);
			size = FONTTOSIZE(font);
			nciRun = cache->curRun->chars;
			runPos = cPos;
		    }
		    nciRun--;
		    if (nciArray == cPos) {
			nciBlock = [_self feedMe:layInfo globals:globals
			    pos:cPos arrayCount:nciArray blockCount:nciBlock];
			cp = (unsigned char *)layInfo->chars->text;
			nciArray = layInfo->chars->chunk.allocated;
			pwa = layInfo->widths->widths + cPos;
		    }
		    cc = cp[cPos];
		    if (cache->curRun->rFlags.graphic) {
			NXSize size;
			[cache->curRun->info calcCellSize:&size];
			*pwa++ = ceil(size.width);
		    } else {
			psize = pw[cc] * size;
			ssize = psw ? psw[cc] : 0.0;
		    /* take the larger of the screen font and printer font */
			*pwa++ = MAX(psize, ssize);
		    }
		    if (MOVECHAR(cc) || cache->curRun->rFlags.graphic) {
			globals->tabsPending++;
			if (runPos != cPos) {
			    addLay(layInfo, runPos, cPos, zone);
			    }
			addLay(layInfo, cPos, cPos + 1, zone);
			runPos = cPos + 1;
			hadEscape = YES;
		    }
       		}
		cPos++;

		{
		    const NXFSM     *trans;

		    trans = TX_NEXTSTATE(state, chClass[cc]);
		    if (!(state = trans->next)) {
			state = self->breakTable;
			lookAhead = trans->delta;
			cPos -= lookAhead;
			if (globals->tabsPending) {
			    int	new_cPos;
			    new_cPos = cPos;
			    width = [_self moveCharWidth:layInfo 
				globals:globals pos:&new_cPos wordPos:wordPos 
				width:width maxwidth:maxwidth];
			    pwa = layInfo->widths->widths + cPos;
			    if(cPos != new_cPos) {
		    		whitePos = wordPos = cPos = new_cPos;
			    	break;
			    }
			} else {
			    cc = wordPos;
			    pwa = layInfo->widths->widths + cc;
			    while (cc < cPos) {
				width += *pwa++;
				cc++;
			    }
			}
			if (trans->token < 0)	/* newline */
			    break;
			if (width > maxwidth && (trans->token || whitePos || charWrap)) {
			    if (!trans->token) {
				if (whitePos)
				    cPos = wordPos;
				else {	/* must be charWrap */
				    charWrap++;
				    oldwidth = width;
				}
			    }
			    break;
			}
			wordPos = cPos;
			if (!trans->token) {	/* non-white space word */
			    whitePos = cPos;
			    oldwidth = width;
			}
		    }
		}
	    }

	    if(width >= BIGNUMBER) 
	    	width = maxwidth;
		
            textUsedUp = (cPos == self->textLength);
	    
	    if (charWrap > 1) {	/* char wrap a big one */
		pwa = &layInfo->widths->widths[cPos];
		width = 0.0;
		while (cPos > 1) {
		    width += *(--pwa);
		    cPos--;
		    if ((oldwidth - width) < maxwidth)
			break;
		}
		whitePos = cPos;
		oldwidth -= width;
		width = oldwidth;	/* !!! cmf 030788 */
	    }
	    layInfo->chars->chunk.used = cPos;
	    layInfo->widths->chunk.used = cPos * sizeof(NXCoord);
	    layInfo->lFlags.endsParagraph = 0;
	    if (layInfo->chars->text[cPos - 1] == '\n')
		layInfo->lFlags.endsParagraph++;


/*
 *  break only on line end.  we have to close off last entry in NXLayArray.
 *  however, we also want to include all white space up to and including
 *  newline on this line.  if we run out of space in the middle of a non-white
 *  range of characters or by a newline, that white space (if it exists) will 
 *  already be in NXLayArray so we need only adjust width.  if we run out of 
 *  space in the middle of white space, we need to trim     
 *       a)  remove sequence starting with current word from layInfo->lays including
 *           any preceding tabs and blanks (usually involves adjusting NXLay
 *           entry where start of word lies).  adjust width accordingly.
 *       b)  If this character is space or tab, keep scanning until see non-
 *           space, non- tab.  If it's newline include it, too.  Write this
 *           sequence of characters into layInfo->lays as last NXLay entry.
 *       c)  Adjust first and last x's in layInfo->lays to record justification
 */

	}

	{

	    NXLay *pl, *plast;
	    NXCoord left = 0.0, right, oldCenter;
	    int endPos, curPos, startPos = cache->curPos;

	    cache->curPos += cPos;
	    layInfo->width = (cPos == self->textLength) ?
	      ceil(width - layInfo->widths->widths[cPos - 1]) : ceil(width);
	    width = oldwidth;
	    endPos = cPos;
	    cPos = whitePos;
	    curPos = startPos + MIN(runPos, cPos);
	    while (curPos < layInfo->cache.runFirstPos) {
		layInfo->cache.curRun--;
		layInfo->cache.runFirstPos -= layInfo->cache.curRun->chars;
	    }
	    if (runPos < cPos) {/* finish run */
		addLay(layInfo, runPos, cPos, zone);
	    } else {
		pl = layInfo->lays->lays;
		plast = TX_LAYAT(layInfo->lays->lays, layInfo->lays->chunk.used);
		while (pl != plast) {
		    if ((pl->offset + pl->chars) >= cPos) {
			layInfo->lays->chunk.used =
			  TX_BYTES(pl, layInfo->lays->lays) + sizeof(NXLay);
			pl->chars = cPos - pl->offset;
			break;
		    }
		    pl++;
		}
	    }
	    addLay(layInfo, cPos, endPos, zone);

	    if (hadEscape) {
		left = 0.0;
		pl = layInfo->lays->lays;
		plast = TX_LAYAT(layInfo->lays->lays,
				 layInfo->lays->chunk.used - sizeof(NXLay));
		while (pl != plast) {
		    right = left;
		    left = pl->x;
		    pl->x = right;
		    if (pl->chars && pl->lFlags.isMoveChar && pl < plast)
			(pl + 1)->lFlags.mustMove = YES;
		    pl++;
		}
		pl->x = 0.0;
	    }
	/*
	 * the use of ceil here may not be correct 
	 */

	    if (growHorizontally) {
		if (textUsedUp || layInfo->lFlags.endsParagraph)
		    maxwidth = layInfo->width;
		while (1) {
		    right = maxwidth + padBy;
		    switch (justification) {
		    case NX_LEFTALIGNED:
			left = indent;
			right -= layInfo->width;
			break;
		    case NX_RIGHTALIGNED:
			left = right - layInfo->width;
			right = 0.0;
			break;
		    case NX_CENTERED:
			left = floor((right - layInfo->width) / 2.0);
			right -= (layInfo->width + left);
			break;
		    case NX_JUSTIFIED:
			break;
		    }
		    if (left >= 0.0 && right >= 0.0)
			break;
		    maxwidth += 1.0;
		}
	    }
	    
	    result = doJustification(width,maxwidth,padBy,indent,
	    			     justification,layInfo);
	    
	    if (growHorizontally) {
	    /*
	     * it's an bug if maxwidth is now bigger than self->maxSize.width
	     * - there should be enough slop so that the maxwidth computed
	     * above is less. 
	     */

		maxwidth += padBy;
		switch (justification) {
		case NX_RIGHTALIGNED:
		    NX_X(layRect) += (NX_WIDTH(layRect) - maxwidth);
		    break;
		case NX_CENTERED:
		/* cmf 010688 next few lines */
		    oldCenter = floor(
				 NX_X(layRect) + (NX_WIDTH(layRect) / 2.0));
		    NX_X(layRect) -= ceil(
				      (maxwidth - NX_WIDTH(layRect)) / 2.0);
		    if (oldCenter !=
			floor(NX_X(layRect) + maxwidth / 2.0))
			NX_X(layRect) += 1.0;
		    break;
		case NX_JUSTIFIED:
		    break;
		}
		NX_WIDTH(layRect) = maxwidth;
	    }
	}
    }
    {
	NXLay *pl, *plast;
	NXCoord maxA, maxD, x, size = 0.0, *pwa;
	id lastFont;
	NXFontMetrics *pf = 0;
	unsigned char *cp;

	pl = layInfo->lays->lays;
	plast = TX_LAYAT(layInfo->lays->lays, layInfo->lays->chunk.used);
	lastFont = NULL;
	maxD = maxA = 0;
	cp = layInfo->chars->text;
	pwa = layInfo->widths->widths;
	/*
	 * If the only run in layinfo is a \n then use its font, otherwise use the largest font
	 * in layinfo and ignore the font of the newline at the end.
	 */
	while (pl < plast) {
	    if ((pl->font != lastFont || pl->y != 0.0) &&
	    	!(pl->chars == 1 && cp[pl->offset] == '\n' && pl != layInfo->lays->lays)) {
		if (pl->font != lastFont) {
		    lastFont = pl->font;
		    if (ISSCREENFONT(lastFont))
			lastFont = FONTTOSCREEN(lastFont);
		    pf = FONTTOMETRICS(lastFont);
		    size = -FONTTOSIZE(lastFont);
		}
/*
 * We always use only flipped fonts, so matrix is constant, so we can premultiply
 * matrix[3] == -1.0 into thesize.
 */
		x = floor(pf->fontBBox[3] * size + pl->y);
		if (x < maxA)
		    maxA = x;
		x = ceil(pf->fontBBox[1] * size + pl->y);
		if (x > maxD)
		    maxD = x;
	    }
	    if (pl->run->rFlags.graphic) {
		NXSize cellSize;
		[pl->run->info calcCellSize:&cellSize];
		cellSize.height = ceil(cellSize.height);
		if (pl->y > maxD)
		    maxD = pl->y;
		if (cellSize.height - pl->y > -maxA)
		    maxA = pl->y - cellSize.height;
	    }
	    pl++;
	}
	layInfo->descent = maxD;
	NX_HEIGHT(layRect) = maxD - maxA;
    }
    
    {
	NXCoord width;
	
	width = [_self fixScreenWidths:layInfo globals:globals];
	result = doJustification(width,
				 NX_WIDTH(layRect)-padBy,
				 padBy,
				 indent,
			         justification,
				 layInfo);

    }
    return (result);
}

/*
 *  example drawFunc
 */
extern int NXDrawALine(Text *_self, NXLayInfo *layInfo)
{
    TextId             *self = (TextId *) _self;
    NXRect theRect, *layRect, *erase = &theRect, graphicRect;
    NXLay *pl, *pll;
    unsigned char *text, ch;
    NXCoord xPos, yPos;
    int numChars = 1;
    BOOL printedSelection, drawCell = NO, drewSomething, optimizedXYShowOk;
    float yShift = 0.0;
    BOOL colorScreen = [_self shouldDrawColor];
 /* draws each line that intersects layrect  */
    layRect = &layInfo->rect;
    pl = layInfo->lays->lays;
    pll = pl + (layInfo->lays->chunk.used / sizeof(NXLay));
    if (pll == pl || !((pll-1)->run->rFlags.graphic))
	pll--;
    yPos = NX_MAXY(layRect) - layInfo->descent;
    *erase = *layRect;
    NX_X(erase) = self->bounds.origin.x;
    NX_WIDTH(erase) = self->bounds.size.width;
    drewSomething = printedSelection = NO;
    if (NXDrawingStatus != NX_DRAWING && NXScreenDump) {
	if (layInfo->lFlags.erase) {
	    _NXTextBackgroundRect(_self, erase, YES, colorScreen);
	    [self->window flushWindow];
	}
	[_self drawSelection:layInfo layRect:layRect];
	printedSelection++;
    }
    for (; pl < pll; pl++) {
	text = layInfo->chars->text + pl->offset;
	ch = *text;
	numChars = pl->chars;
	yShift = pl->y;
	if (pl->run->rFlags.graphic) {
	    [pl->run->info calcCellSize:&graphicRect.size];
	    graphicRect.origin.x = pl->x + NX_X(layRect);
	    graphicRect.origin.y = yPos + yShift - ceil(graphicRect.size.height);
	    drawCell = YES;
	} else if (!(MOVECHAR(ch))) {
	    if (numChars < /*2*/0 && (pl == pll-1) && [_self _canOptimizeDrawing] &&
	        (pl->lFlags.mustMove || !drewSomething) && (drewSomething || !layInfo->lFlags.erase) && COL(pl->run->textRGBColor, colorScreen) < 0 &&
		[_self _setOptimizedXYShowFont:pl->font gray:pl->run->textGray]) {
		optimizedXYShowOk = YES;
	    } else {
		optimizedXYShowOk = NO;
		cacheFont(_self,pl->font);
		cacheInk(_self, pl->run->textGray, 
			    COL(pl->run->textRGBColor, colorScreen));
	    }
	    xPos = pl->x;
	    if (pl->lFlags.mustMove || !drewSomething) {
		xPos += NX_X(layRect);
		if (!drewSomething && layInfo->lFlags.erase) {
		    _NXTextBackgroundRect(_self, erase, YES, colorScreen);
		    cacheInk(_self, pl->run->textGray, 
			    COL(pl->run->textRGBColor, colorScreen));
		    _NXshowat(xPos, yPos+yShift, numChars, (char *)text);
		} else {
		    if (optimizedXYShowOk) {
			if (![_self _optimizedXYShow:(char *)text numChars:numChars at:xPos :yPos+yShift]) {
			    cacheFont(_self,pl->font);
			    cacheInk(_self, pl->run->textGray, 
					COL(pl->run->textRGBColor, colorScreen));
			    _NXshowat(xPos, yPos+yShift, numChars, (char *)text);
			}
		    } else {
			_NXshowat(xPos, yPos+yShift, numChars, (char *)text);
		    }
		    ((NXTextInfo *)(self->_info))->globals.gFlags.xyshowOk = NO;
		}
	    } else {
		if (yShift != 0.0)
		    PSrmoveto(0.0, yShift);
		_NXshow(numChars, (char *)text);
	    }
	    drewSomething = YES;
	    if (yShift != 0.0)
		PSrmoveto(0.0, -yShift);
	}
	if (drawCell) {
	    drawCell = NO;
	    if (!drewSomething && layInfo->lFlags.erase) {
		_NXTextBackgroundRect(_self, erase, YES, colorScreen);
		drewSomething = YES;
	    }
	    [pl->run->info drawSelf:&graphicRect inView:_self];
	}
    }
    if (!drewSomething && !printedSelection && layInfo->lFlags.erase) {
	_NXTextBackgroundRect(_self, erase, YES, colorScreen);
 	[self->window flushWindow];
	}
    if (NXDrawingStatus == NX_DRAWING)
	[_self drawSelection:layInfo layRect:layRect];
    [_self drawUnderline:layInfo layRect:layRect colorscreen:colorScreen];
    return 0;
}

static NXLay *addLay(NXLayInfo *layInfo, int runPos, int cPos, NXZone *zone)
{
    NXLay *pl;
    id screenFont;
    NXRun *run;
    
    if (layInfo->lays->chunk.used == layInfo->lays->chunk.allocated)
	layInfo->lays =
	  (NXLayArray *)NXChunkRealloc(&layInfo->lays->chunk);
    pl = TX_LAYAT(layInfo->lays->lays, layInfo->lays->chunk.used);
    layInfo->lays->chunk.used += sizeof(NXLay);
    pl->x = 0.0;
    pl->chars = cPos - runPos;
    pl->offset = runPos;
    run = layInfo->cache.curRun;
    screenFont = FONTTOSCREEN(run->font);
    pl->font = (screenFont && (NXDrawingStatus == NX_DRAWING)) ?
      screenFont : run->font;
    pl->y = run->superscript ? -((float)(run->superscript)) :
	(run->subscript ? ((float)run->subscript): 0.0);
    pl->paraStyle = run->paraStyle;
    pl->run = run;
    pl->lFlags.mustMove = NO;
    pl->lFlags.isMoveChar = NO;
    return (pl);
}




/*

Modifications (starting at 1.0a):

 3/16/90 bgy	Changed tabs to work like writenow when there are no
 		 more tab stops left. Tabs will now cause lines to 
		 wrap.
 3/19/90 bgy	Made tabs work in lieu of there not being any tab stops.
 3/20/90 bgy	Fixed the behavior of tabs when text is center or right
 		 aligned.  Tabs are now treated as spaces under these
		 conditions.
84
--
 5/04/90 chris	changes NXScanALine to always try to expand horizontally
 		to max allowed before expanding vertically (when wrapping
		from first line of text)

85
--
 6/04/90 pah	Added optimized RectFill and XYShow stuff.
 6/04/90 gcockrft  performace change in fixScreenWidths. Avoids float mul.

87
--
 7/12/90 glc	Color drawing support. Rework of draw cache.

91
--
 8/11/90 glc	fixed printing of underlines
 		Fix crashed which showed up in underlines when reading certain RTF files.
 8/12/90 aozer	Changed isOnColorScreen -> shouldDrawColor


92
--
 8/20/90 gcockrft  Removed some debugging code. 
 			Do the right thing for underlining subscripts.
			Fixes for graphic cells.
 8/16/90 chris	fixed moveCharWidth:* to always guarantee consuming one char.
 8/21/90 gcockrft Backed out graphic cell fixes. If you can call them that.
 8/22/90 gcockrft Put back the one graphic cell fix that was correct. 
 		  Fixed rtf autoindent tab bug 8032,8033

94
--
 9/25/90 gcockrft	Fix for tab hang in Draw.
 
95
--
 9/28/90 chris	fixed right & center justification bug arising from
 		fixScreenWidths. 

96
--
 10/9/90 glc	fix for bug 10323 crash in Bug56. Tab handling moved back cPos causing
 		scanline code to run off end of runs array.		
 */

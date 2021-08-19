#ifndef NOBETTER
#import <appkit/appkit.h>
#import <appkit/Text.h>
#import <appkit/Font.h>
#import <dpsclient/wraps.h>
#import "textprivate.h"
#import "textps.h"

typedef struct {
	@defs (Text)
} textId;

#define STYLE(f) (((_IDFont *)(f))->style)
#define TX_LAYAT(BASE, OFFSET) ((NXLay *) ((char *)(BASE) + (OFFSET)))
#define PFont(f) (NXDrawingStatus == NX_PRINTING? f : ((_IDFont *)(f))->otherFont? ((_IDFont *)(f))->otherFont : f)

static NXLay *addLay(NXLayInfo *layInfo, int runPos, int cPos);
static fixScreenWidths(
    textId * self, NXLayInfo *layInfo, NXTextGlobals *globals);
static int feedMe(
    NXLayInfo *layInfo, NXTextGlobals *globals, int cPos, 
    int nciArray, int nciBlock);
static NXCoord moveCharWidth(
    textId *self, NXLayInfo *layInfo, NXTextGlobals *globals,
    int cPos, int wordPos, NXCoord width);
static void drawSelection(textId *self, NXLayInfo *info, NXRect *layRect);

static NXTextGlobals realGlobal;
static BOOL globalInited = NO;
static void initGlobal()
{
    globalInited = YES;
    bzero(&realGlobal, sizeof(NXTextGlobals));
    realGlobal.savedBreaks = (NXBreakArray *)
	NXChunkMalloc(0, 10 * sizeof(NXLineDesc));
    realGlobal.xPos = (NXWidthArray *)
	NXChunkMalloc(0, INITARRAY * sizeof(NXCoord));
}

BetterScanALine(id _self, NXLayInfo *layInfo)
{
    textId *self = (textId *) _self;
    int result, justification;
    NXTextGlobals *globals = &realGlobal;
    NXRect *layRect = &layInfo->rect;
    if (!globalInited) 
	initGlobal();
    {
	int cPos, runPos, whitePos, wordPos, hadEscape, tryOneLiner;
	int seenMinLine, whitePosMin, cPosMin, charWrap, charWrapMin;
	NXCoord oldwidthMin, maxwidthMin, width, maxwidth, oldwidth;
	NXCoord padBy, indent;
	id font, screenFont;

	{
	    register NXTextStyle *ps;

	    ps = (NXTextStyle *) layInfo->cache.curRun->paraStyle;
	    justification = ps->alignment;
	    NX_HEIGHT(layRect) = ps->lineHt;
	    layInfo->descent = ps->descentLine;
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
	tryOneLiner = 0;
	if (layInfo->lFlags.horizCanGrow && !layInfo->cache.curPos) {
	    seenMinLine = 0;
	    tryOneLiner++;
	    maxwidthMin = maxwidth;
	    maxwidth = ceil(self->maxSize.width - padBy);
	} 
	
	{
	    register NXFSM *state;
	    register NXCoord *pwa, psize, ssize;
	    register unsigned char *cp;
	    register int    cc;
	    register int    nciArray;
	    register int    nciRun;
	    register int    lookAhead;
	    int             nciBlock;
	    unsigned char  *chClass;
	    NXCoord        *pw, *psw = 0;
	    NXLay          *pl;
	    register NXCoord size;

 /*
  * This is main loop : 
  * 1) If necessary, advance cursor in NXRunArray and
  *    cache run info.  At same time, write run info in layInfo->lays. 
  * 2) If necessary, advance cursor in NXTextBlock list and cache info 
  * 3) If necessary expand layInfo->chars and layInfo->widths. 
  * 4) Get next character and copy into layInfo->chars. 
  * 5) Get width of character and copy into layInfo->widths. 
  * 6) If adding this character exceeds layRect->size.width, break 
  * 7) Get state transition for current state and class of current character. 
  * 8) If this is a final state: 
  * 	a)  if this is newLine then break 
  * 	b)  New state in state 0 
  * 	c)  Start new word here  
  *
  */

	    font = layInfo->cache.curRun->font;
	    screenFont = FONTTOSCREEN(font);
	    pw = FONTTOWIDTHS(font);
	    if (screenFont) 
		psw = FONTTOWIDTHS(screenFont);
	    size = FONTTOSIZE(font);
	    pwa = layInfo->widths->widths;
	    nciRun = layInfo->cache.runFirstPos + layInfo->cache.curRun->chars - layInfo->cache.curPos;


	    state = self->breakTable;
	    chClass = self->charCategoryTable;
	    nciArray = layInfo->chars->chunk.allocated;
	    cp = layInfo->chars->text;

	    {
		register unsigned char *sp;
		register unsigned char *cpl;

		sp = (unsigned char *)layInfo->cache.curBlock->text + layInfo->cache.curPos - layInfo->cache.blockFirstPos;
		cPos = nciBlock = layInfo->cache.blockFirstPos + layInfo->cache.curBlock->chars - layInfo->cache.curPos;
		if (cPos > nciArray)
		    cPos = nciArray;
		nciBlock -= cPos;
		cpl = cp + cPos;
		while (cp < cpl) {
		    *cp++ = *sp++;
		}
		while (cPos < nciArray) {
		    if (!layInfo->cache.curBlock->next) {
			nciArray = cPos;
			break;
		    }
		    layInfo->cache.blockFirstPos += layInfo->cache.curBlock->chars;
		    layInfo->cache.curBlock = layInfo->cache.curBlock->next;
		    sp = (unsigned char *)layInfo->cache.curBlock->text;
		    cc = nciBlock = layInfo->cache.curBlock->chars;
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
	    hadEscape = 0;

	    while (1) {
		if (lookAhead) {
		    lookAhead--;
		    pwa++;
		    cc = cp[cPos];
		} else {
		    if (!nciRun) {
			if (runPos != cPos)
			    addLay(layInfo, runPos, cPos);
			layInfo->cache.runFirstPos += layInfo->cache.curRun->chars;
			layInfo->cache.curRun++;
			font = layInfo->cache.curRun->font;
			screenFont = FONTTOSCREEN(font);
			pw = FONTTOWIDTHS(font);
			if (screenFont) psw = FONTTOWIDTHS(screenFont);
			size = FONTTOSIZE(font);
			nciRun = layInfo->cache.curRun->chars;
			runPos = cPos;
		    }
		    nciRun--;
		    if (nciArray == cPos) {
			nciBlock = feedMe(
			    layInfo, globals, cPos, nciArray, nciBlock);
			cp = (unsigned char *)layInfo->chars->text;
			nciArray = layInfo->chars->chunk.allocated;
			pwa = layInfo->widths->widths + cPos;
		    }
		    cc = cp[cPos];
		    psize = pw[cc] * size;
		    ssize = psw ? psw[cc] : 0.0;
		    if (DIACRITICAL(cc)){  /* diacriticals are zero width */
		    	psize = ssize = 0.0;
			if (cc=='\b' && cPos > 0) pwa[-1] = 0.0;
		    }
		    /* take the larger of the screen font and printer font */
		    *pwa++ = MAX(psize, ssize);
		    if (MOVECHAR(cc)) {
			globals->tabsPending++;
			if (runPos != cPos)
			    addLay(layInfo, runPos, cPos);/* depends on error
							 * handler */
			addLay(layInfo, cPos, cPos + 1);
			runPos = cPos + 1;
			hadEscape = 1;
		    }
		}
		cPos++;

		{
		    register NXFSM *trans;

		    trans = TX_NEXTSTATE(state, chClass[cc]);
		    if (!(state = trans->next)) {
			state = self->breakTable;
			lookAhead = trans->delta;
			cPos -= lookAhead;
			if (globals->tabsPending) {
			    width = moveCharWidth(
				self, layInfo, globals, cPos, 
				wordPos, width);
			    pwa = layInfo->widths->widths + cPos;
			} else {
			    cc = wordPos;
			    pwa = layInfo->widths->widths + cc;
			    while (cc < cPos) {
				width += *pwa++;
				cc++;
			    }
			}
			if ((cc = trans->token) < 0)	/* newline */
			    break;
			if (tryOneLiner && !seenMinLine) {
			    if (width > maxwidthMin && 
				(cc || whitePos || charWrap)) {
				seenMinLine++;
				cPosMin = cPos;
				whitePosMin = whitePos;
				oldwidthMin = oldwidth;
				charWrapMin = charWrap;
				if (!cc) {
				    if (whitePos)
					cPosMin = wordPos;
				    else {	/* must be char wrap */
					oldwidthMin = width;
					charWrapMin = 2;
				    }
				}
			    }
			}
			if (width > maxwidth && (cc || whitePos || charWrap)) {
			    if (!cc) {
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
			if (!cc) {	/* non-white space word */
			    whitePos = cPos;
			    oldwidth = width;
			}
		    }
		}
	    }

	    if (tryOneLiner && cPos != self->textLength) {
		/* didn't fit on 1 line */
		tryOneLiner = 0;
		maxwidth = maxwidthMin;
		if (seenMinLine) {
		    charWrap = charWrapMin;
		    cPos = cPosMin;
		    whitePos = whitePosMin;
		    oldwidth = oldwidthMin;
		}
	    }
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
		width = oldwidth; /* !!! cmf 030788 */
	    }
	    layInfo->chars->chunk.used = cPos;
	    layInfo->widths->chunk.used = cPos * sizeof(NXCoord);
	    layInfo->cache.curPos += cPos;


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

	    register NXLay *pl;
	    register NXCoord left;
	    register NXCoord right;
	    register NXLay *plast;
	    register int    endPos;
	    register NXCoord oldCenter; /* cmf 010688 */

	    layInfo->width = (cPos == self->textLength) ?
	      ceil(width - layInfo->widths->widths[cPos - 1]) : ceil(width);

	    width = oldwidth;
	    endPos = cPos;
	    cPos = whitePos;
	    if (runPos < cPos) {/* finish run */
		addLay(layInfo, runPos, cPos);
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
	    addLay(layInfo, cPos, endPos);

	    if (hadEscape) {
		left = 0.0;
		pl = layInfo->lays->lays;
		plast = TX_LAYAT(layInfo->lays->lays, 
		    layInfo->lays->chunk.used - sizeof(NXLay));
		while (pl != plast) {
		    right = left;
		    left = pl->x;
		    pl->x = right;
		    pl++;
		}
		pl->x = 0.0;
	    }
	/*
	 * the use of ceil here may not be correct 
	 */

	    if (tryOneLiner) {
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

	    pl = layInfo->lays->lays;
	    pl->x = left;
	    if (tryOneLiner) {
	    /*
	     * it's an bug if maxwidth is now bigger than self->maxSize.width -
	     * there should be enough slop so that the maxwidth computed
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
	    layInfo->rightIndent = right;
	    layInfo->lFlags.endsParagraph = 0;
	    if (layInfo->chars->text[layInfo->chars->chunk.used - 1] == '\n')
		layInfo->lFlags.endsParagraph++;
	    result = ((left < 0.0 || right < 0) ? 1 : 0);
	}
    }
    {
	register NXLay *pl;
	register NXCoord maxA;
	register NXCoord maxD;
	register NXCoord x;
	register NXCoord size;
	register NXLay *plast;
	register id    lastFont;
	register NXFontMetrics *pf;
	NXCoord *pwa;
	register unsigned char *cp;

	pl = layInfo->lays->lays;
	plast = TX_LAYAT(layInfo->lays->lays, layInfo->lays->chunk.used);
	lastFont = NULL;
	maxD = layInfo->descent;
	maxA = maxD - NX_HEIGHT(layRect);
	cp = layInfo->chars->text;
	pwa = layInfo->widths->widths;
	while (pl < plast) {
	    if (pl->font != lastFont) {
		lastFont = pl->font;
		if (ISSCREENFONT(lastFont)) lastFont = FONTTOSCREEN(lastFont);
    		pf = FONTTOMETRICS(lastFont);
    		size = -FONTTOSIZE(lastFont);
    		
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
	    pl++;
	}
	layInfo->descent = maxD;
	NX_HEIGHT(layRect) = maxD - maxA;
    }
    fixScreenWidths(self, layInfo, globals);
    return (result);
}

BetterDiacriticals(id _self, NXLayInfo *layInfo)
{
    textId         *self = (textId *) _self;
    NXRect          theRect, *layRect, *erase = &theRect;
    NXLay          *pl, *pll;
    unsigned char  *text, ch = 0, prevc = 0;
    NXCoord         xPos, yPos;
    int             numChars = 1, drewSomething;
    BOOL            printedSelection;
    float           yShift = 0.;

 /* draws each line that intersects layrect  */
    layRect = &layInfo->rect;
    pl = layInfo->lays->lays;
    pll = pl + (layInfo->lays->chunk.used / sizeof(NXLay)) - 1;
    yPos = NX_MAXY(layRect) - layInfo->descent;
    *erase = *layRect;
    NX_X(erase) = self->bounds.origin.x;
    NX_WIDTH(erase) = self->bounds.size.width;
    drewSomething = printedSelection = 0;
    if (NXDrawingStatus != NX_DRAWING && NXScreenDump) {
	if (layInfo->lFlags.erase)
	    _NXFQEraseRect(NX_X(erase), NX_Y(erase), NX_WIDTH(erase),
			   NX_HEIGHT(erase), self->backgroundGray);
	drawSelection(self, layInfo, layRect);
	printedSelection++;
    }
    for (; pl < pll; pl++) {
	text = layInfo->chars->text + pl->offset;
	ch = *text;
	numChars = pl->chars;
    /* move super/subscript amount, if necessary */
        yShift = pl->y;

	if (DIACRITICAL(ch) || !MOVECHAR(ch)) {
	    if (ch != '\b')
		[pl->font set];
	    if (DIACRITICAL(ch)) {
		float           x, y, *w;
		int             size = [pl->font pointSize];

		w = [PFont(pl->font) metrics]->widths;
	    /*
	     * hack: Most of the fonts proportion the diacritical
	     * characters as having width .333, which is also the width of
	     * an 'r'. This will -- more or less -- center the diacritical
	     * over the previous character.  NB - to really do this we
	     * should use character bounding boxes, which currently are
	     * not commonly available, and also attend accurately to
	     * placement of cedilla and ogonek. 
	     */
		x = -((w[prevc] + w[ch == 0310 || ch == 0305 ? 
		    'i' : 'r']) / 2.0) * size;
		if (ch == 0310)
		    x -=.04 * size;
		else if (ch == 0305)
		    x -=.02 * size;
		else if (ch == 0302)
		    x +=.04 * size;
		else if (ch == 0304 || ch == 0303 || ch == 0304 || ch == 0317)
		    x +=.08 * size;
		if ((prevc >= '0' && prevc <= '9') ||
		    (prevc == ' ' || prevc == '\'' || 
		    prevc == '`' || prevc == '\"'))
		    x = 0;
		y = (prevc >= 'A' && prevc <= 'Z' &&
		     ch != 0313 && ch != 0316) ? -.2 : 0;
		y *= size;
		if (drewSomething){
		    if (ch == '\b') {
			_NXBackspace(-(w[prevc] * size));
		    } else {
			_NXDiacritical(x, y, 1, (char *)text, self->textGray);
			drewSomething++;
		    }
		}
	    } else if (((xPos = pl->x) != 0.0) || !drewSomething) {
		xPos += NX_X(layRect);
		if (!drewSomething && layInfo->lFlags.erase) {
		    _NXLineStart(xPos, yPos+yShift, NX_X(erase), NX_Y(erase),
			     NX_WIDTH(erase), NX_HEIGHT(erase), numChars,
		     (char *)text, self->backgroundGray, self->textGray);
		    if (yShift != 0.)
			_NXsuper(-yShift);
		} else
		    _NXshowat(
		     xPos, yPos+yShift, numChars, (char *)text, self->textGray);
		if (yShift != 0.) _NXsuper(-yShift);
		drewSomething++;
	    } else {
		if (yShift != 0.) _NXsuper(yShift);
		_NXshow(numChars, (char *)text);
		if (yShift != 0.) _NXsuper(-yShift);
		drewSomething++;
	    }
	    prevc = text[numChars - 1];
	}
    }
    if (!drewSomething && !printedSelection && layInfo->lFlags.erase)
	_NXFQEraseRect(NX_X(erase), NX_Y(erase), NX_WIDTH(erase),
		       NX_HEIGHT(erase), self->backgroundGray);

    if (NXDrawingStatus == NX_DRAWING)
	drawSelection(self, layInfo, layRect);
}



static NXLay *addLay(NXLayInfo *layInfo, int runPos, int cPos)
{
    register NXLay * pl;
    register NXRun *r;
    id screenFont;
    if (layInfo->lays->chunk.used == layInfo->lays->chunk.allocated) 
	layInfo->lays = 
	    (NXLayArray *) NXChunkRealloc(&layInfo->lays->chunk);
    pl = TX_LAYAT(layInfo->lays->lays, layInfo->lays->chunk.used);
    layInfo->lays->chunk.used += sizeof(NXLay);
    pl->x = 0.0;
    pl->chars = cPos - runPos;
    pl->offset = runPos;
    r = layInfo->cache.curRun;
    screenFont = FONTTOSCREEN(r->font);
    pl->font = (screenFont && (NXDrawingStatus == NX_DRAWING)) ? 
	screenFont : r->font;
    pl->y = r->superscript? - (float)(r->superscript) :
    	    r->subscript?     (float)(r->subscript) : 0.0;
    pl->paraStyle = r->paraStyle;
    pl->run = r;
    return(pl);
}


static fixScreenWidths(
    textId * self, NXLayInfo *layInfo, NXTextGlobals *globals)
{
    register NXLay *pl;
    register NXLay *plast;
    NXCoord *pwa, *pxa, *pw, size, width, curWidth = 0.0;
    register unsigned char *cp, ch;
    int isScreen, count;
    id font;
    pwa = layInfo->widths->widths;
    pxa = globals->xPos->widths;
    cp = layInfo->chars->text;
    pl = layInfo->lays->lays;
    plast = TX_LAYAT(layInfo->lays->lays, layInfo->lays->chunk.used);
    while (pl != plast) {
	font = pl->font;
	isScreen = ISSCREENFONT(font);
	pw = FONTTOWIDTHS(font);
	size = FONTTOSIZE(font);
	count = pl->chars;
	while (count) {
	    ch = *cp++;
	    if (ch == '\t') 
	        width = *pxa - curWidth;
	    else if (!*pwa) {
		width = 0.;
	    }
	    else width = isScreen ? pw[ch] : size * pw[ch];
	    *pwa++ = width;
	    curWidth += width;
	    pxa++;
	    count--;
	}
	pl++;
    }
}

static int feedMe(
    NXLayInfo *layInfo, NXTextGlobals *globals, int cPos, 
    int nciArray, int nciBlock)
{
    register unsigned char *cp;
    register NXCoord *pwa;

    {
	register NXCharArray *ta;

	layInfo->chars->chunk.used = cPos;
    /* depends on error handler */
	layInfo->chars = ta = 
	    (NXCharArray *) NXChunkRealloc(&layInfo->chars->chunk);
	cp = (unsigned char *)ta->text;
	nciArray = ta->chunk.allocated;
    }


    layInfo->widths->chunk.used = cPos * sizeof(NXCoord);
    /* depends on error handler */
    layInfo->widths = (NXWidthArray *) NXChunkRealloc(&layInfo->widths->chunk);
    globals->xPos = (NXWidthArray *) NXChunkRealloc(&globals->xPos->chunk);
    
    {
	register unsigned char *sp;
	register int    c;
	register int    cc;

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

static NXCoord moveCharWidth(
    textId *self, NXLayInfo *layInfo, NXTextGlobals *globals,
    int cPos, int wordPos, NXCoord width)
{

    NXCoord        *pwa, *pxa, newwidth, tabWidth;
    int             c;
    unsigned char  *cp;
    NXLay          *pl, *lays;
    NXTabStop      *pt;


    pwa = layInfo->widths->widths + wordPos;
    pxa = globals->xPos->widths + wordPos;
    cp = layInfo->chars->text;
    lays = layInfo->lays->lays;

    while (wordPos < cPos) {
	c = cp[wordPos];
	if (MOVECHAR(c)) {
	    globals->tabsPending--;
	    pl = TX_LAYAT(lays, globals->tabLay);
	    while (pl->offset != wordPos)
		pl++;
	    globals->tabLay = TX_BYTES(pl, lays);

	    tabWidth = 0.0;
	    if (c == '\t') {
		for (pt = globals->tabs;pt < globals->tabsEnd; pt++) {
		    if (width < pt->x) {
			newwidth = pt->x;
			tabWidth = newwidth - width;
			*pwa++ = tabWidth;
			width = newwidth;
			goto breakWhile;
		    }
		}
		width += *pwa;
	breakWhile:
		globals->tabs = pt;
		pl->x = width;
	    } else if (DIACRITICAL(c)) {
		pl->x = 0.;
		pwa++;
	    } else {
		width += *pwa++;
		pl->x = width;
	    }
	} else
	    width += *pwa++;
	*pxa++ = width;
	wordPos++;
    }
    return (width);
}

static void drawSelection(textId *self,  NXLayInfo *layInfo, NXRect *layRect)
{
    /* draws the current selection, if its not just an insertion point */
    NXRect hRect;
    NXCoord xPos, rtPos;

    if ((self->sp0.cp >= 0) &&
		((self->sp0.cp != self->spN.cp) ||
		 (self->sp0.line != self->spN.line))) {
	xPos = layInfo->lays->lays->x;
	rtPos = NX_MAXX(layRect) - layInfo->rightIndent;
	hRect = *layRect;
	hRect.origin.x = (xPos < layInfo->left) ? 
	    xPos : layInfo->left;
	hRect.size.width = (rtPos > layInfo->right ? 
	    rtPos : layInfo->right) - hRect.origin.x;
	if (layInfo->textClipRect)
	    NXIntersectionRect(layInfo->textClipRect, &hRect);
	[(id)self _hilite:NO :&self->sp0 :&self->spN :&hRect];
    }
}
#endif

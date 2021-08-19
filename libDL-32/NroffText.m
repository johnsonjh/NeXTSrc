#import <appkit/appkit.h>
#import <text/text.h>
#import <string.h>
#import "NroffText.h"
#import "TXText.h"
#import "QueryField.h"	 /* text objects target */
#import "findFile.h"
#import "confirm.h"
#import "super.h"

#define PAGESFORCOPYRIGHT 5
#define COPYRIGHT ".index/Copyright.ps"

BOOL WebsterHack = NO;

/* these should be instance vars, but we only 
 * have one instance and I don't
 * want to have a method to access them
 */
id textFont[fNUM];
static unsigned char textCClasses[256];

@implementation NroffText

+ newFrame:(NXRect *) r {

	float	defaultSize = 12.0;
	char	*defaultFontStr = NXGetDefaultValue([NXApp appName], "NXFont");
	char	*defaultSizeStr = NXGetDefaultValue([NXApp appName], "NXFontSize");
	if( defaultSizeStr && *defaultSizeStr)
		sscanf( defaultSizeStr, "%f", &defaultSize);
	
	
	if (!textFont[fPlain]) {
		if( defaultFontStr && *defaultFontStr)
			textFont[fPlain] = [Font newFont: defaultFontStr size: defaultSize];
		else
			textFont[fPlain] = [Font newFont: "Courier" size: 12.0];
		textFont[fBold] =
			[[FontManager new] convert: textFont[fPlain] toHaveTrait: NX_BOLD];
	}

	[Text setDefaultFont:textFont[fPlain]];
	self = [super newFrame:r];
	[self setDoAlert:NO];
	[self setAlignment:NX_LEFTALIGNED];
	[self setMargins];
	[self setCharFilter:NXEditorFilter];
	/* treat backspace like alphanumerics */
	bcopy([self charCategoryTable], textCClasses, sizeof(textCClasses));
	textCClasses['\b'] = textCClasses['a'];
	[self setCharCategoryTable:textCClasses];
	
	[self setBackgroundGray:NX_WHITE];
	[self setCopyrightNeeded:PAGESFORCOPYRIGHT];
	[self setMonoFont:NO];
	[self setCustomLineFuncs: YES];
	return self;
}

- setWrapAround:(BOOL)flag {
	noWrap = !flag;
	return self;
}

- setDoAlert:(BOOL)flag {
	doAlert = flag;
	return self;
}

- setMargins {
	float           l, r, t, b;

	[self getMarginLeft:&l right:&r top:&t bottom:&b];
	l += 4.;
	r = 0;
	t += 4.;
	b += 2.;
	[self setMarginLeft:l right:r top:t bottom:b];
	return self;
}

- resetMargins {
	[self	setMarginLeft:	4
		right:			0
		top:				4
		bottom:			2];
}

- setTarget:(id) theTarget {
	target = theTarget;
	return self;
}

- _setMinMax {
	NXRect          mBounds;

	[self getBounds:&mBounds];
	mBounds.size.height = 0.;
	[self setMinSize:&mBounds.size];

	mBounds.size.height = 1.0e38;
	[self setMaxSize:&mBounds.size];
	return self;
}

- setResizeableText {
	[self setVertResizable:YES];
	[self setHorizResizable:NO];
	[self _setMinMax];
	[[self superview] setAutoresizeSubviews:YES];
	return self;
}

- setFonts:(id) plain :(id) bold
/*
 * Set the default textFont[fPlain] and textFont[fBold] to 'plain' and 'bold',
 * respectively.
 */
{
	NXRect r;
	[self getFrame:&r];
	if (plain) textFont[fPlain] = plain;
	if (bold)  textFont[fBold] = bold;
	[Text setDefaultFont:textFont[fPlain]];
	[self renewFont:textFont[fPlain] text:NULL frame:&r tag:tag];
	return self;
}

/* The following goop was mercilessly plundered from editapp
 * and implements pattern searching.
 */
- rectInView:(NXRect *) theRect {
	register NXRect *rect = theRect;
	register NXRect *bnds = &bounds;
	register NXCoord maxHeight = NX_HEIGHT(bnds);
	register NXCoord maxy = NX_MAXY(rect);

	if (maxy <= maxHeight)
		return self;
	if (NX_HEIGHT(rect) >= maxHeight) {
		*rect = *bnds;
		return self;
	}
	NX_Y(rect) -= maxy - maxHeight;
	return self;
}

- bringSelToTop {
	NXRect _vis, _selRect;
	register NXRect *vis = &_vis;
	register NXRect *selRect = &_selRect;
	BOOL            visible = NO;

	selRect->origin.x = 0.0;
	selRect->origin.y = sp0.y;
	selRect->size.width = bounds.size.width;
	selRect->size.height = (spN.y - sp0.y) + spN.ht;
	[self getVisibleRect:vis];
	if (selRect->origin.y >= vis->origin.y &&
	    selRect->origin.y <= (vis->origin.y + vis->size.height) &&
	    (selRect->origin.y + selRect->size.height) >= vis->origin.y &&
	    (selRect->origin.y + selRect->size.height) <=
	    (vis->origin.y + vis->size.height))
		visible = YES;
	if (visible)
		return self;
	selRect->size.height = vis->size.height - 50.0;
	[self rectInView:selRect];
	[self scrollRectToVisible:selRect];
	return self;
}

- bringSelToBottom
{
	NXRect _vis, _selRect;
	register NXRect *vis = &_vis;
	register NXRect *selRect = &_selRect;
	BOOL            visible = NO;

	selRect->origin.x = 0.0;
	selRect->origin.y = spN.y;
	selRect->size.width = bounds.size.width;
	selRect->size.height = spN.ht;
	[self getVisibleRect:vis];
	if (selRect->origin.y >= vis->origin.y &&
	    selRect->origin.y <= (vis->origin.y + vis->size.height) &&
	    (selRect->origin.y + selRect->size.height) >= vis->origin.y &&
	    (selRect->origin.y + selRect->size.height) <=
	    (vis->origin.y + vis->size.height))
		visible = YES;
	if (visible)
		return self;
	[self rectInView:selRect];
	[self scrollRectToVisible:selRect];
	return self;
}

typedef struct _posRec {
	int             firstPos, lastPos;
	NXTextBlock    *block;
	char           *cur, *first, *last;
}               PosRec;

static          PosRec _NXgPosRec;
static PosRec  *_NXg = &_NXgPosRec;
static int      loc1, loc2;
static char     _NXtCh;

#define GETCHAR(q) (_NXtCh = ((q < 0)?EOF:((q >= _NXg->firstPos && q < _NXg->lastPos)?_NXg->first[q-(_NXg->firstPos)]:positionText(self, _NXg, q))), ((_NXtCh >= 'A' && _NXtCh <= 'Z')?('a'+(_NXtCh - 'A')):_NXtCh))
#define POSTINC(x) ((x < 0)?(EOF):((x < _NXStopPos)?x++: (EOF)))
#define DECR(x) ((x < 0)?(EOF):((x >= _NXStopPos)?(--x): (EOF)))
static int      _NXStopPos;

static char
advanceText(self, pos)
	id              self;
	PosRec         *pos;
{
	register NXTextBlock *CurBlock;
	char            result;

	CurBlock = pos->block;
	pos->firstPos += CurBlock->chars;
	CurBlock = CurBlock->next;
	if (!(CurBlock)) {
		result = EOF;
		goto nope;
	}
	pos->first = pos->cur = (char *)&(CurBlock->text[0]);
	pos->last = pos->first + CurBlock->chars;
	pos->block = CurBlock;
	pos->lastPos = pos->firstPos + CurBlock->chars;
	result = (*(pos->cur++));
nope:
	/* malloc_verify(); */
	return result;
}

static char
positionText(self, pos, CPos)
	register id     self;
	PosRec         *pos;
	register int    CPos;
{
	register NXTextBlock *CurBlock;
	register int    TFirstPos;
	char            result;

	TFirstPos = pos->firstPos;
	CurBlock = pos->block;
	if (CPos == (TFirstPos + CurBlock->chars)) {
		result = advanceText(self, pos);
		goto nope;
	}
	if (CPos < TFirstPos) {
		do {
			CurBlock = CurBlock->prior;
			if (!CurBlock) {
				result = EOF;
				goto nope;
			}
			TFirstPos -= CurBlock->chars;
		} while (CPos < TFirstPos);
		result = CurBlock->text[CurBlock->chars - 1];
	}
	else {

		while (CPos >= (TFirstPos + CurBlock->chars)) {
			TFirstPos += CurBlock->chars;
			if (!(CurBlock = CurBlock->next)) {
				result = EOF;
				goto nope;
			}
		}
		result = CurBlock->text[0];
	}
	pos->firstPos = TFirstPos;
	pos->lastPos = pos->firstPos + CurBlock->chars;
	pos->first = (char *)&(CurBlock->text[0]);
	pos->last = pos->first + CurBlock->chars;
	pos->cur = pos->first + CPos - TFirstPos;
	pos->block = CurBlock;
nope:
	/* malloc_verify(); */
	return (result);
}

static int 
_NXGetCPos(self, CPos, posRec)
	register id     self;
	register int    CPos;
	PosRec         *posRec;
{
	register NXTextBlock *CurBlock;
	register int    TFirstPos;

	TFirstPos = 0;
	CurBlock = self->firstTextBlock;
	while (CPos >= (TFirstPos + CurBlock->chars)) {
		TFirstPos += CurBlock->chars;
		if (!(CurBlock = CurBlock->next)) {
			/*	fprintf(stderr, "position %d not found \n", CPos);	*/
			return;
		}
	}
endBreak:
	posRec->firstPos = TFirstPos;
	posRec->block = CurBlock;
	posRec->first = posRec->cur = (char *)&(CurBlock->text[0]);
	posRec->last = posRec->cur + CurBlock->chars;
	posRec->lastPos = posRec->firstPos + CurBlock->chars;
}

static
doFindText(self, start, stop, pattern)
	id              self;
	int             start, stop;
	char           *pattern;
{
	int             temp, save;
	char           *ppat;
	register char   ch, patChar, lastCh = '\0';

	_NXGetCPos(self, start, _NXg);
	_NXStopPos = stop;
	patChar = *pattern;
	ppat = pattern;
	temp = POSTINC(start);
	while ((ch = GETCHAR(temp)) != EOF) {
		if (ch == patChar) {
			loc1 = save = temp;
			while (1) {
				patChar = *(++ppat);
				if (!patChar) {
					loc2 = start;
					return (1);
				}
				lastCh = ch;
				temp = POSTINC(start);
				if ((ch = GETCHAR(temp)) == EOF)
					return (0);

				/* skip over underscores that aren't part of
				 * the pattern, and backspaces that follow
				 * underscores.  These are nroff underlines
				 */
				if ((ch == '_' && patChar != '_') || 
					(lastCh == '_' && ch == '\b')) {
					patChar = *(--ppat);
				}
				else if (!(ch == patChar)) {
					temp = save;
					patChar = *(ppat = pattern);
					goto nope;
				}
			}
		}
nope:
		temp = POSTINC(start);
		lastCh = ch;
	}
	return (0);
}

static
doBackFindText(self, start, stop, pattern)
	id              self;
	int             start, stop;
	char           *pattern;
{
	int             temp, save;
	char           *ppat, *endStr;
	register char   ch, patChar, lastCh = '\0';

	_NXStopPos = start;
	temp = DECR(stop);
	_NXGetCPos(self, stop, _NXg);
	endStr = ppat = pattern + strlen(pattern) - 1;
	patChar = *ppat;
	while ((ch = GETCHAR(temp)) != EOF) {
		if (ch == patChar) {
			save = temp;
			loc2 = stop + 1;
			while (1) {
				if (ppat <= pattern) {
					loc1 = temp;
					return (1);
				}
				patChar = *(--ppat);
				lastCh = ch;
				temp = DECR(stop);
				if ((ch = GETCHAR(temp)) == EOF)
					return (0);

				/* skip over backspaces that aren't part of
				 * the pattern, and backspaces that follow
				 * underscores.  These are nroff underlines
				 */
				if ((ch == '\b' && patChar != '\b') || 
					(lastCh == '\b' && ch == '_')) {
					patChar = *(++ppat);
				}
				else if (!(ch == patChar)) {
					stop = save;
					patChar = *(ppat = endStr);
					goto nope;
				}
			}
		}
nope:
		temp = DECR(stop);
		lastCh = ch;
	}
	return (0);
}

- setSelEnd {
	[self setSel:textLength:textLength];
	return self;
}

- (int) _notFound:(char *)word:(int)startSel:(int)endSel
{
	[self setSel:startSel :endSel];
	if (doAlert)
		error("\"%s\" not found.", word);
	else
		NXBeep();		
	return NO;
}

- (int)findText:(const char *)s forward:(BOOL) findFwd
{
	char           *retval = 0;
	static char     prev[128] = "";
	int             startSel, endSel;

	if (s)	strncpy(prev, s, sizeof(prev)-1);
	if (!*prev) {
		return NO;
	}
	strtolower(prev);

	/* 
	 * these have to be saved because 
	 * resignFirstResponder sets sel to -1
	 */
	startSel = sp0.cp;
	endSel = spN.cp;
	if (findFwd) {
		if (!doFindText(self, endSel, textLength, prev) &&
		    (noWrap || !doFindText(self, 0, startSel, prev))) {
			return [self _notFound:prev:startSel:endSel];
		}
	}
	else {
		if (!doBackFindText(self, 0, startSel, prev) && 
		    (noWrap || !doBackFindText(self, endSel, textLength, prev))) {
			return [self _notFound:prev:startSel:endSel];
		}
	}
	[self setSel:loc1 :loc2];

	if (findFwd)
		[self bringSelToTop];
	else {
		[self bringSelToBottom];
	}
	[self hideCaret];
	return YES;
}

/********* end of finding goop *********/

- paste:sender {
	[super paste:sender];
	[self calcLine];
	if ([delegate respondsTo:@selector (textDidChange:)])
		[delegate textDidChange:self];
	return self;
}

- (NXRunArray *)getTheRuns
{
	return theRuns;
}

- readRichText:(NXStream *)f
{
	if ([delegate respondsTo:@selector(setFontChangeable:)])
		[delegate setFontChangeable:NO];

	return [super readRichText:f];
}

- readText:(NXStream *)f
{
	if ([delegate respondsTo:@selector(setFontChangeable:)])
		[delegate setFontChangeable:YES];

	return [super readText:f];
}

- setText:(char const *)s {
	if (s && *s && [delegate respondsTo:@selector(setFontChangeable:)])
		[delegate setFontChangeable:YES];

	if (WebsterHack) {
		extern NXRunArray *TXnroffRuns(void);
		NXRunArray *websterRuns = TXnroffRuns();
		if (websterRuns) {
			NXTextStyle *p;
			[self setParaStyle];
			p = (NXTextStyle *)(websterRuns->runs[0].paraStyle);
			p->indent2nd = 65.;	/* hack; depends on the font */
		}
		[self renewRuns:websterRuns text:s frame:&frame tag:tag];
		[self getMinWidth:NULL minHeight:&(bounds.size.height) 
			maxWidth:0 maxHeight:100000000.0];
	}
	else {
		[super setText:s];
	}

	[self setParaStyle];
	if (currFile) {
		free(currFile);
		currFile = NULL;
	}
	[self setSel:0:0];
	return self;
}

#define MAXSTR 1024

- (char *)getSelString:(char *)s maxLen:(int)len
/* Copy the selected string into 's'. */
{
	int             n = spN.cp - sp0.cp;

	if (spN.cp < 0 || sp0.cp < 0)  {
		n = 0;
	}

	if (n >= len)
		n = len - 1;
	if (n != 0)
		[self getSubstring:s start:sp0.cp length:n];
	s[n] = '\0';
	return s;
}

- save:sender {
	if (delegate && [delegate respondsTo:@selector (save:)])
		[delegate save:sender];
	return self;
}

- mouseDown:(NXEvent *) e
/*
 * Calls the default mousedown method for the textobject,
 * and if the user has passed a 'target', that object
 * recieves a 'click :(id)text :(char *) t : (NXEvent *)e' message.
 */
{
	char            t[MAXSTR + 1];

	*t = '\0';
	[super mouseDown:e];
	[self getSelString:t maxLen:MAXSTR];
	if (target && [target respondsTo:@selector (click:::)]) {
			[target click:self :t :e];
	}
	return self;
}

- setParaStyle {
	int n, N;
	float size=0.;
	
	for (n=0;textFont[n];n++){
		float s;
		s = [textFont[n] pointSize];
		if (size < s){
			size = s;
			N = n;
		}
	}
	if (size>0.) {
		[self setDfltParaStyle:N];
	}
	return self;
}

- setDfltParaStyle:(int)fontNum {
	[self calcParagraphStyle:textFont[fontNum]:NX_LEFTALIGNED];
	TXsetDfltParaStyle(theRuns->runs[0].paraStyle);
	return self;
}

/* printing stuff */

static int numPages=0;

/* this is called after all of the setup is done and before
 * any pages are done.
 */
- endSetup  {
	numPages=0;
	return [super endSetup];
}

/* this is called each time a page is printed */
- endPage {
	numPages++;
	return [super endPage];
}

/* this prints the copyright page if it needs to.
 * This page should really be added to the end of the text object
 * to do it properly.  But this is easier and probably rigth
 * most of the time.
 */
- printCopyrightPage {
	char *copyrightPS;
	NXStream *strm;
	char *buf;
	int count, maxLen;
	
	if (currFile && copyrightNeeded && (numPages > copyrightNeeded)) {
		if (copyrightPS = findFileFromPath(currFile, COPYRIGHT)) { 
		    if (strm = NXMapFile(copyrightPS, NX_READONLY)) {
			    NXGetMemoryBuffer(strm, &buf, &count, &maxLen);

			    DPSPrintf(DPSGetCurrentContext(),
				"\n%% Add the copyright page\n");
			    DPSPrintf(DPSGetCurrentContext(),
				"%%%%BeginDocument: %s\n", copyrightPS);

			    DPSWriteData(DPSGetCurrentContext(), buf, count);
			    DPSPrintf(DPSGetCurrentContext(),
				"%%%%EndDocument\n\n");

			    NXCloseMemory(strm, NX_FREEBUFFER);
		    }
		    else {
			    error("Couldn't open \"%s\"\n", copyrightPS);
		    }
		    free(copyrightPS);
		}
	}
}

/* print the copyright page at the end before 
 * the document trailer so that we know that 
 * there are enough pages printed to make it worthwhile.
 */
- beginTrailer {
	[self printCopyrightPage];
	return [super beginTrailer];
}

/* 
 * do this so that there is a decent title for the spooler app
 */
- beginPrologueBBox:(NXRect *)boundingBox creationDate:(char *)dateCreated
		createdBy:(char *)anApplication fonts:(char *)fontNames
		forWhom:(char *)user pages:(int)numPages title:(char *)aTitle 
{
	 return [super beginPrologueBBox:boundingBox
			creationDate:dateCreated
			createdBy:anApplication
			fonts:fontNames
			forWhom:user
			pages:numPages
			title:currFile? currFile:""];
}

/*
 * the app subclass should set this when it does a readText or readRichText
 * so that we know whether there is an important file to print (and what it's
 * title is.)
 */
- setCurrFile:(char *)file {
	if (currFile) free(currFile);
	currFile = strsave(file);
	return self;
}

/*
 * sets the bit that we need to print a copyright page if there is one, and
 * the number of pages that we can print without printing a copyright
 * defaults to PAGESFORCOPYRIGHT.
 */
- setCopyrightNeeded:(int)pages {
	copyrightNeeded = pages;
	return self;
}

- (int) copyrightNeeded {
	return copyrightNeeded;
}

- setCustomLineFuncs: (BOOL) yes {
	if( yes)
#ifndef NOBETTER
	{
	extern BetterDiacriticals(), BetterScanALine();
		[self setDrawFunc:BetterDiacriticals];
		[self setScanFunc:BetterScanALine];
	}
	else {
#else
	|| if( !yes) {
#endif
		[self setDrawFunc:NXDrawALine];
		[self setScanFunc:NXScanALine];
	}
}

@end
		

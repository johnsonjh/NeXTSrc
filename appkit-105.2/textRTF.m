/*
	textRTF.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

/*
 * TODO - change fontList to remove assumption that font nubmering will
 * start at zero. 
 *
 * - add stylesheet reader - add paragraph style changes 
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <objc/hashtable.h>
#import <objc/HashTable.h>
#import <objc/Storage.h>
#import "Font.h"
#import "Text.h"
#import "rtfstructs.h"
#import "rtfdefs.h"
#import "rtfdata.h"
#import "textprivate.h"
#import "nextstd.h"
#import "errors.h"
#import <dpsclient/wraps.h>
#import <stdio.h>
#import <ctype.h>
#import <math.h>
#import "appkitPrivate.h"

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryRTF=0\n");
    asm(".globl .NXTextCategoryRTF\n");
#endif

typedef struct {
    @defs (Text)
} TextId;

#define DEFAULT_MASK (NX_UNBOLD | NX_UNITALIC)
#define TX_FONTAT(BASE, OFFSET) ((NXFontEntry *) ((char *)(BASE) + (OFFSET)))
#define ISALPHA(c) isalpha(c)
#define ISNUM(c) (c >= '0' && c <= '9')   /* appears to beat isdigit(c) */

#define HEX(ch)		((ch >= 'a' && ch <= 'e') ? (10 + ch - 'a'):\
			 ((ch >= 'A' && ch <= 'E') ? (10 + ch - 'A'):\
			  ((ch >= '0' && ch <= '9') ? (ch - '0') : 0)))

/*
 * This data is shared among all text objects.
 */
static NXSymbolTable oneChar = {0, SPECIAL, 0};

static NXHashTable *symHashTab = 0;
static NXHashTable *timesHashTab = 0;
static NXHashTable *rtfFamilyTab = 0;
static NXHashTable *fontNumTab = 0;
static int currentFontNum = 0;
static id defaultFont = nil;
static int defaultWeight = 0, currentSeen = 0;
static const char        *tail = "}\n";
static const char *timesName = 0;


    static void adjustMargin(id self, int lmargin, int rmargin);
    static NXHashTable *getFonts(NXStream *stream, NXZone *zone);
    static void clearBlock(NXStream *stream);
    static void initProp(NXProps * p);
    static void setRunFromProps(Text *self,NXProps *prop, NXHashTable* fontList, 
		Storage *colorList, NXRun *run);
    static void flushBuf(Text *self, unsigned char *buf, 
	int nchars, NXProps * prop, NXTextStyle *textStyle, 
	NXHashTable * fontList, Storage *colorList);
    static void adjustFrame(id self, NXCoord paperWidth, NXCoord paperHeight);
    static NXTextStyle *getStyle(id self, NXProps * prop);
    static void initTable(void);
    static NXFontEntry *getFontEntry(NXHashTable *fontTable, id font);
    static void replaceText(id self, NXCharArray *textBuf, NXRunArray *runs, BOOL smartPaste, RichTextOp operation);
    static int getFontNum(NXAtom family, BOOL *print);
    static NXAtom getRichTextFamily(NXAtom family);
    static NXStyleSheetEntry *findRTFStyle(NXHashTable *styleList, int entry);
    static void getRTFStyles(
	NXStream *stream, NXTextStyle *initStyle, NXHashTable *styleList,
	NXZone *zone);
    static void clearWhite(NXStream *stream);
    static void setProp(NXProps *prop, NXSymbolTable *st, int param);
    static NXSymbolTable *getToken(
	NXStream *stream, int *param, unsigned char *token, BOOL haveFirstChar);
    static void setupTabs(id _self, NXProps *prop);

extern NXHashTablePrototype symProto, timesProto, rtfFamilyProto, styleProto, 
    styleSheetProto, fontProto, writeFontProto, fontNumProto;
    static id getRunFont(NXTextGlobals *globals, NXProps *prop, NXHashTable *fontList);
    

#define MAXPROP 20
#define TSTYLE(x) ((x) ? &(x)->prop.textStyle: 0)



@implementation Text(RTF)

- sendText:(unsigned char *)buf start:(int)startPos end:(int)endPos
    into:(NXStream *)stream
{
    int charsToRead, remaining;

    remaining = endPos - startPos;
    while (remaining) {
	charsToRead = MIN(remaining, BSIZE);
	[self getSubstring:(char *)buf start:startPos length:charsToRead];
	{
	    unsigned char *last, *s;
	    BOOL       escape;

	    s = buf;
	    last = s + charsToRead;
	    while (s < last) {
		escape = NO;
		switch (*s) {
		case '{':
		case '}':
		case '\\':
		case '\n':
		    escape = YES;
		}
		if (escape)
		    NXPutc(stream, '\\');
		NXPutc(stream, *s++);
	    }
	}
	startPos += charsToRead;
	remaining -= charsToRead;
    }
    return self;
}

typedef struct _RTFstate {
    float      	fsize;
    short       bold;
    short       italic;
    short       underline;
    short	alignment;
    int        	fontnum;
    float      	lindent;
    float      	findent;
    float	gray;
    int		color;
    int		subscript;
    int		superscript;
    NXTabStop	*tabs;
} RTFstate;

/*
 * Clear state. So that all attributes are written.
 */
static void clearstate(RTFstate *statep)
{
    statep->fsize = -1;
    statep->bold = -1;
    statep->italic = -1;
    statep->underline = -1;
    statep->alignment = -1;
    statep->fontnum = -1;
    statep->lindent = -1;
    statep->findent = -1;
    statep->gray = -1;
    statep->color = -1;
    statep->subscript = -1;
    statep->superscript = -1;
    statep->tabs = NULL;
}

/*
 * After a \pard. Bring paragraph state back to defaults.
 */
static void initrtfstate(RTFstate *statep)
{
    statep->color = -1;
    statep->tabs = NULL;
    statep->alignment = NX_LEFTALIGNED;
    statep->subscript = 0;
    statep->superscript = 0;
    statep->lindent = 0;
    statep->findent = 0;
    statep->gray = 0;
}

- (int) printOneRun:(int) firstPos end:(int)lastPos into:(NXStream *)stream
    fonts:(NXHashTable *)fontTable colors:(HashTable *)colorTable run:(NXRun *)run
     runPos:(int)runPosition buf:(unsigned char *)buf state:(RTFstate *)statep
{
    char faces[30], *just = 0;
    NXFontEntry *entry = getFontEntry(fontTable, run->font);
    NXTextStyle *textStyle = (NXTextStyle *)run->paraStyle;
    NXTabStop *tab, *lastTab;
    BOOL writeDefaultRTF;
    int	colornum,startpos,item;
    
    if(run->rFlags.graphic || run->rFlags.subclassWantsRTF)
	NXWrite(stream, "{", 1);
    	
    if (run->rFlags.subclassWantsRTF) {
	writeDefaultRTF = YES;
	NXPrintf(stream, "{\\attachment%d ", runPosition);
	[self writeRichText:stream forRun:run atPosition:runPosition
	    emitDefaultRichText:&writeDefaultRTF];
	NXWrite(stream, tail, strlen(tail));
	clearstate(statep);
	if (!writeDefaultRTF)
	    return 0;
    }
    startpos = NXTell(stream);
    if(statep->tabs != textStyle->tabs) {
    	initrtfstate(statep);   /* have to reset all paragraph attributes */
	statep->tabs = textStyle->tabs;
	NXPrintf(stream,"\\pard");
	if (textStyle->numTabs) {
	    lastTab = textStyle->tabs + textStyle->numTabs;
	    for (tab = textStyle->tabs; tab < lastTab; tab++) {
		NXPrintf(stream, "\\tx%d", (int)(tab->x * CONVERSION));
	    }
	}
    }

    if(statep->fontnum != entry->fontNum) {
	statep->fontnum = entry->fontNum;
	NXPrintf(stream, "\\f%d",statep->fontnum);
    }
    
    
    *faces = 0;
    
    item = 0;
    if (entry->face & NX_BOLD)
	item = 1;
    if(statep->bold != item) {
	statep->bold = item;
	if(statep->bold)
	    strcat(faces, "\\b");
	else
	    strcat(faces,"\\b0");
    }
    
    item = 0;
    if (entry->face & NX_ITALIC)
	item = 1;
    if(statep->italic != item) {
	statep->italic = item;
	if(statep->italic)
	    strcat(faces, "\\i");
	else
	    strcat(faces,"\\i0");
    }
    
    if(statep->underline != run->rFlags.underline) {
	statep->underline = run->rFlags.underline;
	if(statep->underline)
	    strcat(faces, "\\ul");
	else
	    strcat(faces,"\\ul0");
    }
    
    if(statep->alignment != textStyle->alignment) {
    	statep->alignment = textStyle->alignment;
	switch (statep->alignment) {
	    case NX_LEFTALIGNED:
		just = "l";
		break;
	    case NX_RIGHTALIGNED:
		just = "r";
		break;
	    case NX_CENTERED:
		just = "c";
		break;
	    case NX_JUSTIFIED:
		just = "j";
		break;
	}
	strcat(faces,"\\q");
	strcat(faces,just);
    }
    NXPrintf(stream, "%s",faces);
    
    if(statep->fsize != [run->font pointSize]) {
	statep->fsize = [run->font pointSize];
	NXPrintf(stream, "\\fs%d",(int)(statep->fsize * 2.0));
    }
    if(statep->findent != (textStyle->indent1st - textStyle->indent2nd)) {
	statep->findent = (textStyle->indent1st - textStyle->indent2nd);
	NXPrintf(stream, "\\fi%d",(int)(statep->findent * CONVERSION));
    }
    if(statep->lindent != textStyle->indent2nd) {
	statep->lindent = textStyle->indent2nd;
	NXPrintf(stream, "\\li%d",(int)(statep->lindent * CONVERSION));
    }
    
    if (statep->gray != run->textGray) {
    	statep->gray = run->textGray;
	NXPrintf(stream, "\\gray%d", (int) (statep->gray * 1000.));
    }
    if (run->textRGBColor >= 0) {
	colornum = (int) [colorTable valueForKey:(const void *)run->textRGBColor];
	if(statep->color != colornum) {
	    statep->color = colornum;
	    NXPrintf(stream, "\\fc%d", colornum);
	}
    }
    if (statep->superscript != run->superscript) {
	statep->superscript = run->superscript;
	NXPrintf(stream, "\\up%d", run->superscript * 2);
    }
    if (statep->subscript != run->subscript) {
	statep->subscript = run->subscript;
	NXPrintf(stream, "\\dn%d", run->subscript * 2);
    }

    if (run->rFlags.graphic && !run->rFlags.subclassWantsRTF) {
	const char *directive = [[self class] directiveForClass:
	    [run->info class]];
	if (directive) {
	    NXPrintf(stream, "{\\%s%d ", directive, runPosition);
	    [run->info writeRichText:stream forView:self];
	    NXWrite(stream, tail, strlen(tail));
	    clearstate(statep);
	    startpos = NXTell(stream);
	}
    }
    if(startpos != NXTell(stream)) 
	NXWrite(stream, " ", 1);
    
    [self sendText:buf start:firstPos end:lastPos into:stream];
    if(run->rFlags.graphic || run->rFlags.subclassWantsRTF)
	NXWrite(stream, "}", 1);
    else
    	NXWrite(stream, "\n", 1);
    return lastPos - firstPos;
}


- printRuns:(int) startPos end:(int) endPos into:(NXStream *)stream
    fonts:(NXHashTable *)fontTable colors:(HashTable *)colorTable 
    op:(RichTextOp) operation
{
    NXRun *run = &theRuns->runs[0];
    NXRun *lastRun = TX_RUNAT(run, theRuns->chunk.used);
    NXZone *zone = [self zone];
    unsigned char *buf = (unsigned char *)NXZoneMalloc(zone, BSIZE);
    int currentPos, runPosition = 0;
    BOOL noText = (startPos == endPos) ? YES: NO;
    RTFstate	rtfstate;

    currentPos = 0;
    clearstate(&rtfstate);
    
    for (; run < lastRun; run++) {
	int                 firstPos, lastPos;
	int                 endRunPos = currentPos + run->chars;

	if (endRunPos <= startPos)
	    goto next;
	if (currentPos > endPos)
	    break;
	firstPos = (startPos > currentPos) ? startPos : currentPos;
	lastPos = (endPos < endRunPos) ? endPos : endRunPos;
	if (firstPos > lastPos)
	    break;
	if (firstPos == lastPos) {		
	    if (noText)				/* copy ruler */
		noText = NO;
	    else 
		break;
	}
	runPosition += [self printOneRun:firstPos end:lastPos
	    into:stream fonts:fontTable colors:colorTable run:run runPos:runPosition
	    buf:buf state:&rtfstate];
  next:
	currentPos = endRunPos;
    }
    free(buf);
    return self;
}

- (NXHashTable *)printFonts:(NXStream *)stream op:(RichTextOp) operation
{
    char               *fontHeader = "{\\fonttbl";
    NXHashTable        *fontTable;
    float size, lmargin, rmargin, tmargin, bmargin;
    int weight;
    id fontManager = [FontManager new];
    BOOL print = NO;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXZone *zone = [self zone];

    currentSeen++;
    fontTable = NXCreateHashTable(writeFontProto, 0, 0);
    NXWrite(stream, fontHeader, strlen(fontHeader));
    {
	NXRun     *run = &theRuns->runs[0];
	NXRun     *lastRun = TX_RUNAT(run, theRuns->chunk.used);
	id	  runFont;
	for (;run < lastRun; run++) {
	    NXFontEntry temp, *entry;
	    temp.font = run->font;
	    entry = NXHashGet(fontTable, &temp);
	    if (entry)
		continue;
	    NX_ZONEMALLOC(zone, entry, NXFontEntry, 1);
	    [fontManager getFamily:&entry->family traits:&entry->face
		weight:&weight size:&size ofFont:run->font];
		/* Use unbolded & italic fontname */
	    runFont = run->font;
	    runFont = [fontManager convert:runFont toNotHaveTrait:NX_ITALIC];
	    runFont = [fontManager convert:runFont toNotHaveTrait:NX_BOLD];
	    entry->family = [runFont name];
	    entry->family = NXUniqueString(entry->family);
	    entry->fontNum = getFontNum(entry->family, &print);
	    entry->rtfFamily = getRichTextFamily(entry->family);
	    entry->font = run->font;
	    NXHashInsert(fontTable, entry);
	    if (print)
		NXPrintf(stream, "\\f%d\\f%s %s;\000", entry->fontNum, 
		    entry->rtfFamily, entry->family);
	}
    }
    NXWrite(stream, tail, strlen(tail));
    switch (operation) {
	case archive_io:
	    if (globals->gFlags.willWritePaperSize) {
		NXSize paperSize;
		[delegate textWillWrite:self paperSize:&paperSize];
		NXPrintf(stream, "\\paperw%d\n", 
		    (int)(paperSize.width * CONVERSION));
		NXPrintf(stream, "\\paperh%d\n", 
		    (int)(paperSize.height * CONVERSION));
	    }
	    [self getMarginLeft:&lmargin right:&rmargin 
		top:&tmargin bottom:&bmargin];
	    NXPrintf(stream, "\\margl%d\n", (int)(lmargin * CONVERSION));
	    NXPrintf(stream, "\\margr%d\n", (int)(rmargin * CONVERSION));
	    break;
	case pasteboard_io:
	    NXPrintf(stream, "\\smartcopy%d\n", 
		(clickCount == 2) ? 1 : 0);
	    break;
	default:
	    break;
    }
    return (fontTable);
}

- (HashTable *)printColors:(NXStream *)stream op:(RichTextOp) operation
{
    char               *colorHeader = "{\\colortbl";
    HashTable        *colorTable;
    float	red,green,blue,alpha;
    int		colors = 0;
    NXColor	color;
    NXRun     *run = &theRuns->runs[0];
    NXRun     *lastRun = TX_RUNAT(run, theRuns->chunk.used);
    NXZone *zone = [self zone];
    
    colorTable = [[HashTable allocFromZone:zone] initKeyDesc:"i"];
    for (;run < lastRun; run++) {
	if(run->textRGBColor < 0)
	    continue;
	if([colorTable isKey:(const void *)run->textRGBColor])
	    continue;
   
	if(colors == 0) 
	    NXWrite(stream, colorHeader, strlen(colorHeader));

	color = _NXFromTextRGBColor(run->textRGBColor);
	NXConvertColorToRGBA (color, &red, &green, &blue, &alpha);
	[colorTable insertKey:(const void *)run->textRGBColor value:(void *)colors];
#define CON(i) ((int)(i*255))
	NXPrintf(stream, "\\red%d\\green%d\\blue%d;",CON(red),CON(green),CON(blue));
	if((colors & 7) == 7)
	    NXWrite(stream, "\n", 1);	    
	colors++;
    }
    if(colors > 0)
	NXWrite(stream, tail, strlen(tail));
    return (colorTable);
}


static void initProp(NXProps * p)
{
    if(p->buildTabs && !p->tabsCopied) 
	[p->buildTabs free];
	
    bzero(p, sizeof(NXProps));
    p->font = -1;
    p->size = 12;
    p->textStyle.alignment = NX_LEFTALIGNED;
    p->face = DEFAULT_MASK;
}


static void clearWhite(NXStream *stream)
{
    int ch;

    ch = NXGetc(stream);
    while (isspace(ch))
	ch = NXGetc(stream);
    if (ch != EOF)
	NXUngetc(stream);
}



static void clearBlock(NXStream *stream)
{
    unsigned char ch;
    int        match = 1;

    while (match) {
	ch = NXGetc(stream);
	switch (ch) {
	case '{':
	    match++;
	    break;
	case '}':
	    match--;
	    break;
	case '\\':
	    NXGetc(stream);
	    break;
	}
    }

}



/*
 * This function gets called be the lexical analyser upon ecountering a
 * \fonttbl token.  It is a mini parser that expects a font table that
 * looks something like this: 
 *
 * [{]\fxxx(\froman|\fmodern|\fswiss|\fscript|\fdecor|\ftech) Name;[}] 
 *
 * The function is called with the file pointing to the first character after
 * the \fonttbl token. 
 */
static NXHashTable *getFonts(NXStream *stream, NXZone *zone)
{
    NXHashTable *fontList;
    NXFontItem *entry,temp;
    unsigned char fontName[100];
    unsigned char ch, *s;
    int                 param, offset;
    NXSymbolTable      *st;

    int                 inFontTable = YES;
    int                 fontOpen = NO;

 /*
  * Allocate space for a font table. 
  */
    fontList = NXCreateHashTableFromZone(fontProto, 0, 0, zone);
    while (inFontTable) {
	clearWhite(stream);
	ch = NXGetc(stream);

	switch (ch) {
	case LBRACE:
	    fontOpen = YES;
	    break;

	case '\\':

	    s = fontName;
	/*
	 * We should get a \f followed by a paramter. 
	 */
	    st = getToken(stream, &offset, fontName, YES);
	    if (!st || st->value != FONTPROP)
		NX_RAISE(NX_badRtfFontTable, 
		    "expected font number", 0);

	/*
	 * Get the font type, i.e. roman, modern, swiss. 
	 */
	    clearWhite(stream);
	    st = getToken(stream, &param, fontName, NO);

	/*
	 * Get the font name, this should be terminated by a ';'. 
	 */
	    clearWhite(stream);
	    ch = NXGetc(stream);
	    while (ch != ';') {
		*s++ = ch;
		ch = NXGetc(stream);
	    }

	    *s++ = 0;

	/*
	 * Copy the font name into the fontList table. 
	 */
	    temp.fontName = NXUniqueString((char *)fontName);
	    if(NXHashGet(fontList, &temp))
		break;
	    NX_ZONEMALLOC(zone, entry, NXFontItem, 1);
	    entry->num = offset;
	    entry->fontName = temp.fontName;
	    NXHashInsert(fontList, entry);
	    break;

	case RBRACE:
	    if (!fontOpen)
		inFontTable = NO;
	    fontOpen = NO;
	    break;

	default:
	    NX_RAISE(NX_badRtfFontTable, "invalid font table format", 0);
	    break;
	}
    }

    clearWhite(stream);
    return (fontList);
}
/*
 * This function gets called by the lexical analyser upon ecountering a
 * \colortbl token.  It is a mini parser that expects a color table that
 * looks something like this: 
 *
 * [{]\red0\green0\blue0;;\red255\green0\blue0;[}] 
 *  semis bump the colortable index
 
 * The function is called with the file pointing to the first character after
 * the \colortbl token. 
 */
static Storage *getColors(NXStream *stream, NXZone *zone)
{
    Storage *colorList;
    unsigned char token[100];
    unsigned char ch;
    int                 param;
    NXSymbolTable      *st;
    float		red,green,blue;
    int			colorindex = 0;
    int			inColorTable = YES;
    int			RGBColor;
    NXColor		color;


 /*
  * Allocate space for a color table. 
  */
    colorList = [[Storage allocFromZone:zone]
	initCount:0 elementSize:sizeof(int) description:"i"];
    while (inColorTable) {
	clearWhite(stream);
	ch = NXGetc(stream);

	switch (ch) {
	case '\\':

	/*
	 * We should get a \red followed by a paramter. 
	 */
	    st = getToken(stream, &param, token, YES);
	    if (!st || st->value != REDPROP)
		NX_RAISE(NX_badRtfColorTable, 
		    "expected red number", 0);
	    red = param / 255.0;
	    
	    clearWhite(stream);
	    st = getToken(stream, &param, token, NO);
	    if (!st || st->value != GREENPROP)
		NX_RAISE(NX_badRtfColorTable, 
		    "expected green number", 0);
	    green = param / 255.0;
	    clearWhite(stream);
	    
	    st = getToken(stream, &param, token, NO);
	    if (!st || st->value != BLUEPROP)
		NX_RAISE(NX_badRtfColorTable, 
		    "expected blue number", 0);
	    blue = param / 255.0;
	    
	    color = NXConvertRGBAToColor(red,green,blue,1.0);
	    RGBColor = _NXToTextRGBColor(color);
	    [colorList insert:(void *)&RGBColor at:colorindex+1]; /* bumped by 1 */
	    clearWhite(stream);
	    break;
	case RBRACE:
	    inColorTable = NO;
	    break;
	case ';':
	    colorindex++;
	    break;

	default:
	    NX_RAISE(NX_badRtfColorTable, "invalid color table format", 0);
	    break;
	}
    }

    clearWhite(stream);
    return (colorList);
}

static void cleanupTabs(NXProps *prop)
{
    if (prop->buildTabs && !prop->tabsCopied) 
	[prop->buildTabs free];
    prop->buildTabs = NULL;
    prop->tabsCopied = NO;
}

static void setTab(NXProps *prop, NXCoord val, NXZone *zone)
{
    if (!prop->buildTabs || prop->tabsCopied) {
	prop->buildTabs = [[NXTabStopList allocFromZone:zone] init];
	prop->tabsCopied = NO;
    }
    [prop->buildTabs addTabStop:val kind:NX_LEFTTAB];
}

- (NXTextStyle *)getUniqueStyle:(NXTextStyle *)style
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXTextStyle *result, temp;
    NXZone *zone = [self zone];
    if (!style)
	return style;
    if (!globals->rtfStyles)
	globals->rtfStyles = NXCreateHashTable(styleProto, 0, self);
    temp = *style;
    if (temp.numTabs < 0)
	temp.numTabs = 0;
    if (temp.tabs) 
	temp.tabs = [self getTabs:temp.tabs num:temp.numTabs];
    result = NXHashGet(globals->rtfStyles, &temp);
    if (result)
	return result;
    result = (NXTextStyle *)NXZoneMalloc (zone, sizeof(NXTextStyle));
    *result = temp;
    NXHashInsert(globals->rtfStyles, result);
    return result;
}

- removeUniqueStyle:(NXTextStyle *)style
{
    /* does not delete tabs. this is not a space leak, since
       they are used again to create another style.
     */
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    if (!globals->rtfStyles)
	return self;
    NXHashRemove(globals->rtfStyles, style);
    free(style);
    return self;
}

- readRTF:(NXStream *)stream op:(RichTextOp)operation
{
    unsigned char buf[BSIZE], *last, *ptr, ch, token[100];
    int param, lmargin = -1, rmargin = -1, stack = 0, smartCopy = 0, thechar;
    int maxStack = 0;
    NXSymbolTable *st;
    NXHashTable *fontList = 0;
    Storage *colorList = 0;
    NXStyleSheetEntry  *curStyle = 0;
    NXHashTable *styleList = NULL;
    NXCoord paperWidth = -1.0, paperHeight = -1.0;
    NXProps *curProp = 0;
    NXProps maxProps;
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXTextStyle *pasteStyle = 0;
    id pasteFont = 0;
    NXZone *zone = [self zone];
    NXProps      *gProps;

 /*
  * Initialize the table of recognized RTF directives.  
  */
    if (!symHashTab)
	initTable();
	
    NX_ZONEMALLOC(zone, globals->rtf.gProps, NXProps, MAXPROP);
    bzero(globals->rtf.gProps, sizeof(NXProps) * MAXPROP);
    globals->rtf.globalRuns = (NXRunArray *)NXChunkZoneMalloc(10 * sizeof(NXRun), 0, zone);
    globals->rtf.textBuf = (NXCharArray *) NXChunkZoneMalloc(8 * 1024, 0, zone);
    globals->gFlags.hasTabs = NO;
    gProps = globals->rtf.gProps;
 /*
  * Initialize the stack of formatting properties. 
  */
    initProp(&gProps[stack]);
    /* 
     * Init the variables with state.
     */
    globals->rtf.lastProps.size = -1;
    globals->rtf.curRun = &globals->rtf.globalRuns->runs[0];
    globals->rtf.lRun = 
    	TX_RUNAT(globals->rtf.curRun, globals->rtf.globalRuns->chunk.allocated);
    globals->rtf.globalRuns->chunk.used = 0;
    globals->rtf.curText = &globals->rtf.textBuf->text[0];
    globals->rtf.lText = globals->rtf.curText + globals->rtf.textBuf->chunk.allocated;
    globals->rtf.textBuf->chunk.used = 0;
    globals->rtf.face1 = globals->rtf.font1 = globals->rtf.size1 = 0;
    globals->rtf.face2 = globals->rtf.font2 = globals->rtf.size2 = 0;
    globals->rtf.ret1 = globals->rtf.ret2 = (id)0;
     
    maxProps.size = -1;
    [self startReadingRichText];
    ptr = buf;
    last = ptr + BSIZE;

 /*
  * This lexical analyser looks for grouping tokens and for `\` delimited RTF
  * tokens. 
  *
  * Any level of nested groups are allowed and within nested groups any
  * combination of RTF directives are allowed.  Groups need not only bracket
  * directives they can also bracket text.  This provides structure to a
  * document but adds no formatting information. 
  */
    thechar = NXGetc(stream);
    ch = thechar;
    while (thechar != EOF) {
	curProp = gProps + stack;
	if(isalnum(ch))
	    goto handleChar;
	switch (ch) {

	/*
	 * Look for an open group token. 
	 */
	case LBRACE:
	/*
	 * flush out the stuff with the current properties 
	 */
	    flushBuf(self, buf, ptr - buf, &gProps[stack],
		     TSTYLE(curStyle), fontList, colorList);
	    ptr = buf;

	/*
	 * Push the previous group state and copy it to the top of stack.  
	 */
	    curProp++;
	    *curProp = gProps[stack];
	    if (curProp->buildTabs)
		curProp->tabsCopied = YES;
	    stack++;

	/* Check for stack overflow. */
	    if (stack >= MAXPROP)
		fprintf(stderr, "prop stack overflow\n");

	    break;

	case RBRACE:
	    flushBuf(self, buf, ptr - buf, curProp,
		     TSTYLE(curStyle), fontList, colorList);
	    if (stack > maxStack) {
		switch (operation) {
		    case ruler_io:
			setupTabs(self, curProp);
			pasteStyle = getStyle(self, curProp);
			break;
		    case font_io:
			pasteFont = getRunFont(globals, curProp, fontList);
			break;
		    case archive_io:
		        maxProps = *curProp;
			break;
		    default: break;
		}
		maxStack = stack;
	    }
	    cleanupTabs(curProp);
	    stack--;
	    if (stack < 0) {
		fprintf(stderr, "prop stack underflow\n");
		stack = 0;
	    }
	    ptr = buf;
	    if(stack == 0)
		goto leavewhile;
	    break;

	case '\\':
	/*
	 * A RTF directive token should follow the '\' delimiter 
	 */
	    st = getToken(stream, &param, token, YES);
	    if (!st)
		break;		/* some unknown property */

	    switch (st->type) {
	    case PROP:
		flushBuf(self, buf, ptr - buf,
			 curProp, TSTYLE(curStyle),
			 fontList, colorList);
		ptr = buf;
		setProp(curProp, st, param);
		break;

	    case SPECIAL:
		switch (st->value) {
		case HEX_SPECIAL:
		    {
			unsigned char gotc;

			gotc = NXGetc(stream);
			ch = HEX(gotc) * 16;
			gotc = NXGetc(stream);
			ch += HEX(gotc);
			goto handleChar;
		    }
		    break;
		case PAPERW_SPECIAL:
		    paperWidth = CONVERT(param);
		    break;
		case PAPERH_SPECIAL:
		    paperHeight = CONVERT(param);
		    break;
		case MARGL_SPECIAL:
		    lmargin = CONVERT(param);
		    break;
		case MARGR_SPECIAL:
		    rmargin = CONVERT(param);
		    break;
		case SMARTCOPY_SPECIAL:
		    if (operation == pasteboard_io)
			smartCopy = param;
		    break;
		case SETTAB_SPECIAL:
		    setTab(curProp, CONVERT(param), zone);
		    globals->gFlags.hasTabs = YES;
		    break;
		case NEXTSTYLE_SPECIAL:
		case BASEDONSTYLE_SPECIAL:
		case STYLE_SPECIAL:
		case 0:
		/*
		 * Don't do anything if we get a paper height directive. 
		 */
		    break;
		default:
		    ch = st->value;
		    goto handleChar;
		}
		break;

	    case DEST:
		switch (st->value) {
		case COLORTABLE_GROUP:
		    colorList = getColors(stream, zone);
		    stack--;
		    if (stack < 0) {
			fprintf(stderr, "prop stack underflow\n");
			stack = 0;
		    }
		    break;

		case FONTTABLE_GROUP:
		    fontList = getFonts(stream, zone);
		    stack--;
		    if (stack < 0) {
			fprintf(stderr, "prop stack underflow\n");
			stack = 0;
		    }
		    break;

		case STYLESHEET_GROUP:
		    styleList = NXCreateHashTable(styleSheetProto, 0, 0);
		    getRTFStyles(
			stream, globals->uniqueStyle, styleList, zone);
		    stack--;
		    if (stack < 0) {
			fprintf(stderr, "prop stack underflow\n");
			stack = 0;
		    }
		    break;

		case STYLE_GROUP:
		    {
			NXStyleSheetEntry  *tempStyle = 
				findRTFStyle(styleList, param);
			if (tempStyle)
			    curStyle = tempStyle;
			break;
		    }
		case DOCUMENT_GROUP:
		    break;

		case SPECIAL_GROUP:
		    stack--;
		    gProps[stack].info = [self readCellRichText:stream sym:st];
		    clearBlock(stream);
		    break;
		case ATTACHMENT_GROUP:
		    [self readRichText:stream atPosition:param];
		default:
		    clearBlock(stream);
		    stack--;
		    if (stack < 0) {
			fprintf(stderr,
				"prop stack underflow\n");
			stack = 0;
		    }
		}
		break;

	    default:
		NX_RAISE(NX_badRtfDirective, "invalid tableEntry", 0);
		stack--;
		if (stack < 0) {
		    fprintf(stderr,
			    "prop stack underflow\n");
		    stack = 0;
		}
		break;
	    }
	    break;

	case '\n':
	    break;
	case '\r':
	    break;

	default:
    handleChar:
	    if (ptr >= last) {
		flushBuf(self, buf, ptr - buf,
			 &gProps[stack], TSTYLE(curStyle),
			 fontList, colorList);
		ptr = buf;
	    }
	    *ptr++ = ch;
	    break;
	} /* switch */

	thechar = NXGetc(stream);
	ch = thechar;
    } /* while */
leavewhile:    
    
    switch (operation) {
	case ruler_io:
	    [self setSelFont:nil paraStyle:pasteStyle];
	    break;
	case font_io:
	    [self setSelFont:pasteFont paraStyle:0];
	    break;
	default:
	    flushBuf(self, buf, ptr - buf, &gProps[stack], TSTYLE(curStyle),
			fontList, colorList);
	    if (lmargin >= 0 || rmargin >= 0)
		adjustMargin(self, lmargin, rmargin);
	    if ((operation == archive_io) && 
		(paperWidth > 0.0 || paperHeight > 0.0))
		adjustFrame(self, paperWidth, paperHeight);
	/*  if we had a property, but empty text, make sure we read font info */
	    if (globals->rtf.textBuf->chunk.used == 0 && maxProps.size != -1)
	        setRunFromProps(self,&maxProps,fontList,colorList,&theRuns->runs[0]);
	    if ([self _canDraw]) {
		[self lockFocus];
		[self hideCaret];
		replaceText(self, globals->rtf.textBuf, globals->rtf.globalRuns, 
							smartCopy, operation);
		[self showCaret];
		[[self window] flushWindow];
		[self unlockFocus];
	    } else 
		replaceText(self, globals->rtf.textBuf, globals->rtf.globalRuns, 
							smartCopy, operation);
    }
    [self finishReadingRichText];
    if (fontList)
	NXFreeHashTable(fontList);
    if(colorList)
	[colorList free];
    if (styleList)
	NXFreeHashTable(styleList);
     
    free(globals->rtf.gProps);
    free(globals->rtf.globalRuns);
    free(globals->rtf.textBuf);
    return self;
}

static void replaceText(id _self, NXCharArray *textBuf, NXRunArray *runs, BOOL smartPaste, RichTextOp operation)
{	
    NXRunArray oneRun;
    TextId *self = (TextId *) _self;
    NXTextGlobals *globals = TEXTGLOBALS(self->_info);
    NXRun *pr, *last;
    if (self->tFlags.monoFont) {
	oneRun.chunk.used = sizeof(NXRun);
	oneRun.chunk.growby = sizeof(NXRun);
	oneRun.chunk.allocated = sizeof(NXRun);
	pr = &oneRun.runs[0];
	bzero(pr, sizeof(NXRun));
	pr->paraStyle = globals->uniqueStyle;
	if (operation == pasteboard_io)
	    pr->font = self->theRuns->runs[0].font;
	else
	    pr->font = runs->runs[0].font;
	pr->chars = textBuf->chunk.used;
	pr->textGray = self->textGray;
	pr->textRGBColor = globals->textRGBColor;
	runs = &oneRun;
    }
    
	/*
	 * If we are pasting and the selection start is at the start of a paragraph,
	 * we use the paragraph styles of the pasted text.
	 * If we are starting in the middle of a paragraph, we use this paragraphs 
	 * paragraph style. Ideally you would want to switch to the pasting text
	 * paragraph styles as soon as you hit a newline. This is a little more
	 * difficult to implement, because you might have to splice in runs versus
	 * just using different styles.
	 */
    if(!self->tFlags.monoFont && operation == pasteboard_io) {
	NXSelPt temp;
	NXTextCache *cache = CACHE(self->_info);
	    
	temp = self->sp0;
	[_self findStartParagraph:&temp];
	if(self->sp0.cp != temp.cp) {
	    NXAdjustTextCache(_self, cache, temp.cp);
	    pr = &runs->runs[0];
	    last = pr + (runs->chunk.used / sizeof(NXRun));
	    for (; pr < last; pr++) {
		    pr->paraStyle = cache->curRun->paraStyle;
		}
	}
    }

    if (!globals->gFlags.hasTabs) {
	Font	*ft;
	NXTextStyle *style;
	
	ft = [Font newFont:"Ohlfs" size:10.0];
		/* Just want good tabs from this guy */
	style = _NXMakeDefaultStyle([_self zone], 0, 0.0, 0.0, ft, 0);
	pr = &runs->runs[0];
	last = pr + (runs->chunk.used / sizeof(NXRun));
	if(textBuf->chunk.used == 0) {
	    pr = &self->theRuns->runs[0];
	    last = pr + 1;
	}
	for (; pr < last; pr++) {
	    if (!pr->paraStyle)
		pr->paraStyle = style;
	    if (!((NXTextStyle *)pr->paraStyle)->tabs) {
		NXTextStyle temp;
		temp = *((NXTextStyle *)pr->paraStyle);
		temp.tabs = style->tabs;
		temp.numTabs = style->numTabs;
		pr->paraStyle = [_self getUniqueStyle:&temp];
	    }
	}
	free(style->tabs);
	free(style);
    }
	if(operation == archive_io)
    	[_self _replaceAll:(char *)&textBuf->text[0] :textBuf->chunk.used :runs];
	else
    	[_self _replaceSel:nil :(char *)&textBuf->text[0] :textBuf->chunk.used
	 		:runs :NO :NO :smartPaste];
}

static BOOL equalProps(NXProps *p1, NXProps *p2)
{
 /*
  * if same ptr, they are the same. this catches two NULLS as well 
  */
    if (p1 == p2)
	return YES;
 /*
  * not the same if one doesn't exist 
  */
    if (!p1 || !p2)
	return NO;

    if (p1->font != p2->font)
	return NO;
    if (p1->size != p2->size)
	return NO;
    if (p1->group != p2->group)
	return NO;
    if (p1->textGray != p2->textGray)
	return NO;
    if (p1->textRGBColor != p2->textRGBColor)
	return NO;
    if (p1->face != p2->face)
	return NO;
    if (p1->superscript != p2->superscript)
	return NO;
    if (p1->subscript != p2->subscript)
	return NO;
    if (p1->textStyle.alignment != p2->textStyle.alignment)
	return NO;
    if (p1->textStyle.indent1st != p2->textStyle.indent1st)
	return NO;
    if (p1->textStyle.indent2nd != p2->textStyle.indent2nd)
	return NO;
    if (p1->tabs != p2->tabs)
	return NO;
    if (p1->underline != p2->underline)
	return NO;
    if (p1->info != p2->info)
	return NO;
    return YES;
}


static NXSymbolTable *specialChar(unsigned char ch)
{
    oneChar.value = ch;
    return &oneChar;
}


/*
 * Convert the character stream starting with ch from ascii to decimal. 
 * This now takes into account negative numbers. 
 */
#define GETNUM(stream, ch, param)\
{\
    int        val = 0;\
\
	if(ch == '-')\
		{\
		ch = NXGetc(stream);\
     	while (ISNUM(ch))\
			{\
			val = (10 * val) + ch - '0';\
			ch = NXGetc(stream);\
			}\
		*param = -val;\
		}\
	else\
		{\
     	do\
			{\
			val = (10 * val) + ch - '0';\
			ch = NXGetc(stream);\
			} while(ISNUM(ch));\
		*param = val;\
		}\
	\
	if(ch != ' ')\
		NXUngetc(stream);\
}

static NXSymbolTable *getToken(
    NXStream *stream, int *param, unsigned char *token, BOOL haveFirstChar)
{
    unsigned char ch;
    NXSymbolTable *st,temp;
    unsigned char *s = token;

    temp.name = (char *)s;    

    *s = 0;
    *param = -1;

    if (!haveFirstChar) {
	ch = NXGetc(stream);
	if (ch != '\\')
	    return (specialChar(ch));
    }
    ch = NXGetc(stream);

 /*
  * If the first character isn't alphabetical then it should either be in the
  * lookup table or it shoud be a special character. 
  */
    if (!ISALPHA(ch)) {
	*s++ = ch;
	*s++ = 0;
    	st = NXHashGet(symHashTab, &temp);
	if (st)
	    return st;
	st = (NXSymbolTable *)[Text symbolForDirective:(const char *)token];
	if (st)
	    return st;
    /*
     * If the token is in the symbol table then return it's symbol entry. If
     * not return treat it as a special character. 
     */
	return (specialChar(ch));
    }
 /*
  * Get a token. 
  */
    do {
	*s++ = ch;
	ch = NXGetc(stream);
    } while (ISALPHA(ch));

 /*
  * If the token is followed be a numerical parameter then we should convert
  * the ascii to decimal. 
  */
    if (ISNUM(ch) || ch == '-')
		{
		GETNUM(stream, ch, param);
		}
    else if (!isspace(ch))	/* discard any white space. */
	NXUngetc(stream);

    *s++ = 0;			/* null-terminate the token */

 /*
  * Try to find this token in the lookup table. 
  */
	st = NXHashGet(symHashTab, &temp);
	if (st)
		return st;
	st = (NXSymbolTable *)[Text symbolForDirective:(const char *)token];

#ifdef DEBUG_MISSING
    if (!st)
	fprintf(stderr, "Can't find token %s\n", token);
#endif

    return (st);
}



static void setProp(NXProps *prop, NXSymbolTable *st, int param)
{
    switch (st->value) {
    case INITPROP:		/* set the current stack to the default */
	{
	    int                 oldFont = prop->font;
	    int                 oldSize = prop->size;
	    int			oldFace = prop->face;

	    initProp(prop);
	    prop->font = oldFont;
	    prop->size = oldSize;
	    prop->face = oldFace;
	}
	break;
    case PLAINPROP:
	prop->face = DEFAULT_MASK;
	break;
    case FONTPROP:
	prop->font = param;
	break;
    case BOLDPROP:
	if (param == 0) {	/* turn it off */
	    prop->face &= ~NX_BOLD;
	    prop->face |= NX_UNBOLD;
	} else {		/* no parameter, so turn it on */
	    prop->face |= NX_BOLD;
	    prop->face &= ~NX_UNBOLD;
	}
	break;
    case UNDERLINEPROP:
	if (param == 0) {	/* turn it off */
	    prop->underline = NO;
	} else {		/* no parameter, so turn it on */
	    prop->underline = YES;
	}
	break;
    case STOPUNDERLINEPROP:
	prop->underline = NO;
	break;
    case ITALICPROP:
	if (param == 0) {	/* turn it off */
	    prop->face &= ~NX_ITALIC;
	    prop->face |= NX_UNITALIC;
	} else {		/* no parameter, so turn it on */
	    prop->face |= NX_ITALIC;
	    prop->face &= ~NX_UNITALIC;
	}
	break;
    case SIZEPROP:
	prop->size = param / 2;
	break;
    case SUPERSCRIPTPROP:
	if (param > 255 * 2)
	    param = 255 * 2;
	prop->superscript = (param < 0) ? 0 : param / 2;
	break;
    case SUBSCRIPTPROP:
	if (param > 255 * 2)
	    param = 255 * 2;
	prop->subscript = (param < 0) ? 0 : param / 2;
	break;
    case FIRSTINDENTPROP:
	prop->textStyle.indent1st = CONVERT(param);
	break;
    case LEFTINDENTPROP:
	prop->textStyle.indent2nd = CONVERT(param);
	break;
    case LEFTJUSTPROP:
	prop->textStyle.alignment = NX_LEFTALIGNED;
	break;
    case RIGHTJUSTPROP:
	prop->textStyle.alignment = NX_RIGHTALIGNED;
	break;
    case JUSTIFYPROP:
	prop->textStyle.alignment = NX_LEFTALIGNED;
	break;
    case CENTERPROP:
	prop->textStyle.alignment = NX_CENTERED;
	break;
    case GRAYPROP:
	prop->textGray = param;
	break;
    case COLORPROP:
	prop->textRGBColor = param + 1; /* bump by 1 to tell apart from 0 default */
	break;

    default:
	fprintf(stderr, "unknown property %s\n", st->name);
	break;
    }
}


static void createFamily(void)
{
    const RichTextFamily *rtf;
    RichTextFamily *entry,temp;

    rtfFamilyTab = NXCreateHashTable(rtfFamilyProto, 0, 0);
    for (rtf = richTextFamilies; rtf->family; rtf++) {
	temp.family = NXUniqueString(rtf->family);
	if(NXHashGet(rtfFamilyTab,&temp))
	    continue;
	NX_MALLOC(entry, RichTextFamily, 1);
	entry->family = temp.family;
	entry->rtf = NXUniqueString(rtf->rtf);
	NXHashInsert(rtfFamilyTab, entry);
    }
}


- writeRTFAt:(int)startPos end:(int)endPos into:(NXStream *)stream
    op:(RichTextOp)operation
{
    NXHashTable *fontTable;
    HashTable	*colorTable;
    char *header = "{\\rtf0\\ansi";

    if (!rtfFamilyTab)
	createFamily();
    NXWrite(stream, header, strlen(header));
    fontTable = [self printFonts:stream op:operation];
    colorTable = [self printColors:stream op:operation];
    [self printRuns:startPos end:endPos into:stream 
	fonts:fontTable colors:colorTable op:operation];
    NXWrite(stream, tail, strlen(tail));
    NXFlush(stream);
    [colorTable free];
    NXFreeHashTable(fontTable);
    return self;
}

static int getFontNum(NXAtom family, BOOL *print)
{
    NXFontItem temp, *item;
    *print = NO;
    if (!fontNumTab)
	fontNumTab = NXCreateHashTable(fontNumProto, 0, 0);
    temp.fontName = family;
    item = NXHashGet(fontNumTab, &temp);
    if (item) {
	if (item->seen != currentSeen) {
	    item->seen = currentSeen;
	    *print = YES;
	}
	return item->num;
    }
    NX_MALLOC(item, NXFontItem, 1);
    item->fontName = family;
    item->num = currentFontNum++;
    item->seen = currentSeen;
    *print = YES;
    NXHashInsert(fontNumTab, item);
    return item->num;
}

static NXAtom getRichTextFamily(NXAtom family)
{
    RichTextFamily temp, *rtf;
    temp.family = family;
    rtf = NXHashGet(rtfFamilyTab, &temp);
    if (rtf)
	return rtf->rtf;
    return "nil";
}

static NXFontEntry *getFontEntry(NXHashTable *fontTable, id font)
{
    NXFontEntry temp;
    temp.font = font;
    return NXHashGet(fontTable, &temp);
}



static void adjustMargin(id self, int lmargin, int rmargin)
{
    NXCoord             left, right, top, bottom;

    [self getMarginLeft:&left right:&right top:&top bottom:&bottom];
    if (lmargin >= 0)
	left = lmargin;
    if (rmargin >= 0)
	right = rmargin;
    [self setMarginLeft:left right:right top:top bottom:bottom];
}

static NXTextStyle *getStyle(id _self, NXProps * prop)
{
    TextId *self = (TextId *)_self;
    NXTextGlobals *globals = TEXTGLOBALS(self->_info);
    NXTextStyle *defaultStyle = globals->uniqueStyle;
    NXTextStyle tstyle;
    
    tstyle = *defaultStyle;
    tstyle.indent1st = prop->textStyle.indent1st;
    tstyle.indent2nd = prop->textStyle.indent2nd;
    tstyle.indent1st += tstyle.indent2nd;
    tstyle.alignment = prop->textStyle.alignment;
    tstyle.numTabs = prop->numTabs;
    tstyle.tabs = prop->tabs;
    return [_self getUniqueStyle:&tstyle];
}

static void adjustFrame(id _self, NXCoord paperWidth, NXCoord paperHeight)
{
    NXSize size;
    NXRect rect;
    TextId *self = (TextId *)_self;
    NXTextGlobals *globals = TEXTGLOBALS(self->_info);
    [_self getFrame:&rect];
    size.width = paperWidth > 0.0 ? paperWidth : rect.size.width;
    size.height = paperHeight > 0.0 ? paperHeight : rect.size.height;
    if (globals->gFlags.didReadPaperSize)
	[self->delegate textDidRead:_self paperSize:&size];
}

- (NXTabStop *)getTabs:(NXTabStop *)tabs num:(int)numTabs
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    if (!globals->tabTable) {
	NXZone *zone = [self zone];
	globals->tabTable = [NXUniqueTabs allocFromZone:zone];
    }
    return [globals->tabTable getTabs:tabs num:numTabs];
}

@end

static void setStyle(NXStyleSheetEntry * style, NXSymbolTable * st, int param)
{
    switch (st->value) {
	case NEXTSTYLE_SPECIAL:
	style->nextStyle = param;
	break;
    case BASEDONSTYLE_SPECIAL:
	style->basedOnStyle = param;
	break;
    case SETTAB_SPECIAL:
    /* Deal with this later. */
	break;
    case STYLE_SPECIAL:
	style->num = param;
	break;
    default:
	setProp(&style->prop, st, param);
	break;
    }
}

/*
 *  This function gets called be the lexical analyser upon ecountering
 * a \stylesheet token.  Each style in the sheet should be bracketed
 * and may optionally contain \sbasedon and/or \snext tokens.  The
 * first style doesn't necessarily need a style number and if one
 * is absent then it is assumed to be zero.
 *
 * The function is called with the file pointing to the first character after
 * the \stylesheet token.
 */
static void getRTFStyles(
    NXStream *stream, NXTextStyle *initStyle, NXHashTable *styleList,
    NXZone *zone)
{
    NXStyleSheetEntry *style = 0;
    unsigned char styleName[100], ch, *s;
    int param, styleNum,firststyle;
    BOOL inStyleSheet = YES, styleOpen = NO;
    NXSymbolTable  *st;
    
    firststyle = YES;

    while (inStyleSheet) {
	clearWhite(stream);

	ch = NXGetc(stream);
	switch (ch) {
	case LBRACE:
	    styleOpen = YES;
	    break;

	case '\\':
	    s = styleName;

	/*
	 * We should get a \s followed by a paramter. 
	 */
	    st = getToken(stream, &styleNum, styleName, YES);
	    if (!st || st->value != STYLE_GROUP) {
		styleNum = 0;
	    }

	/*
	 * Allocate space for a symbol table. 
	 */

	    style =(NXStyleSheetEntry *)NXZoneMalloc(
		zone, sizeof(NXStyleSheetEntry));
	    bzero(style, sizeof(NXStyleSheetEntry));
	    style->num = styleNum;
	    NXHashInsert(styleList, style);
	/*
	 * Initialize the NXTextStyle. 
	 */
	    style->prop.textStyle = *(initStyle);

	/*
	 * Get the style directives. 
	 */
	    clearWhite(stream);
	    ch = NXGetc(stream);
	    while (ch == '\\') {
		st = getToken(stream, &param, styleName, YES);
		if (st) {
		    setStyle(style, st, param);
		}
		clearWhite(stream);
		ch = NXGetc(stream);
	    }

	/*
	 * Get the style name, this should be terminated by a ';'. 
	 */
	    while (ch != ';') {
		*s++ = ch;
		ch = NXGetc(stream);
	    }

	    *s++ = 0;

	/*
	 * Copy the font name into the fontList table. 
	 */
	    style->name = NXCopyStringBufferFromZone((char *)styleName, zone);
	    firststyle = NO;
	    break;

	case RBRACE:
	    if (!styleOpen)
		inStyleSheet = NO;

	    styleOpen = NO;
	    break;

	default:
	    NX_RAISE(NX_badRtfFontTable, "invalid font table format", 0);
	    break;
	}
    }

    clearWhite(stream);
}

static NXStyleSheetEntry *findRTFStyle(NXHashTable *styleList, int entry)
{
    NXStyleSheetEntry temp;
    temp.num = entry;
    return NXHashGet(styleList, &temp);
}

static void setupTabs(id _self, NXProps *prop)
{
    if (!prop->buildTabs || !prop->buildTabs->numTabs) {
	prop->tabs = 0;
	prop->numTabs = 0;
	return;
    }
    prop->numTabs = prop->buildTabs->numTabs;
    if (prop->numTabs < 0)
	prop->numTabs = 0;
    prop->tabs = [_self getTabs:[prop->buildTabs tabs] num:prop->numTabs];
}



static id getRunFont(NXTextGlobals *globals, NXProps *prop, NXHashTable *fontList)
{			/* get the font */
    const char *fontBase = 0;
    id runFont = nil;
	
		/* Check to see if we recently returned this font */
	if(prop->size == globals->rtf.size1 && prop->font == globals->rtf.font1 && 
				prop->face == globals->rtf.face1)
		return(globals->rtf.ret1);
	if(prop->size == globals->rtf.size2 && prop->font == globals->rtf.font2 && 
				prop->face == globals->rtf.face2)
		return(globals->rtf.ret2);
	
    if (prop->font < 0) {
	fontBase = NULL;
    } else {
	NXFontItem temp, *found;
	temp.num = prop->font;
	found = NXHashGet(fontList, &temp);
	if (found)
	    fontBase = found->fontName;
    }
    if (fontBase) {
	char *newFont = NXHashGet(timesHashTab, fontBase);
	if (newFont)
	    fontBase = timesName;
	runFont = [Font newFont:fontBase size:(float)prop->size];
	if(prop->face & NX_BOLD)
	    runFont = [[FontManager new] convert:runFont toHaveTrait:NX_BOLD];
	if(prop->face & NX_ITALIC)
	    runFont = [[FontManager new] convert:runFont toHaveTrait:NX_ITALIC];
    }
    
    globals->rtf.font2 = globals->rtf.font1; globals->rtf.face2 = globals->rtf.face1; 
    globals->rtf.size2 = globals->rtf.size1; globals->rtf.ret2 = globals->rtf.ret1;
    globals->rtf.font1 = prop->font; globals->rtf.face1 = prop->face; 
    globals->rtf.size1 = prop->size; globals->rtf.ret1 = runFont;
	
    return runFont;
}

static void setRunFromProps(Text *self,NXProps *prop, NXHashTable* fontList, 
		Storage *colorList, NXRun *run)
{
int	*cptr;
NXTextGlobals *globals = TEXTGLOBALS(self->_info);

    run->font = getRunFont(globals, prop, fontList);
    if (!run->font)
	run->font = defaultFont;

    run->paraStyle = getStyle(self, prop);
    run->superscript = prop->superscript;
    run->subscript = prop->subscript;
    run->textGray = ((float)prop->textGray) / 1000.;
    run->textRGBColor = -1;
    if(colorList && prop->textRGBColor > 0) {
	cptr = (int *) [colorList elementAt:(unsigned int) prop->textRGBColor];
	if(cptr)
	    run->textRGBColor = *cptr;
    }
    run->rFlags.underline = prop->underline ? YES : NO;
    run->info = prop->info;
    if (prop->info)
	run->rFlags.graphic = YES;
}

static void flushBuf(
    Text *self, unsigned char *buf, int nchars,
    NXProps * prop, NXTextStyle *textStyle, NXHashTable* fontList, Storage *colorList)
{
    NXRun        run;
    NXTextGlobals *globals = TEXTGLOBALS(self->_info);
    NXRunArray  	*globalRuns;
    NXCharArray   	*textBuf;

    if (!nchars)
	return;
    globalRuns = globals->rtf.globalRuns;
    textBuf = globals->rtf.textBuf;
    if (globals->rtf.curRun == globals->rtf.lRun) {
	int                 offset = TX_BYTES(globals->rtf.curRun, &globalRuns->runs[0]);

	globalRuns = (NXRunArray *)NXChunkZoneGrow(
		       &globalRuns->chunk, 2 * globalRuns->chunk.allocated,[self zone]);
	globals->rtf.curRun = &globalRuns->runs[0];
	globals->rtf.lRun = TX_RUNAT(globals->rtf.curRun, globalRuns->chunk.allocated);
	globals->rtf.curRun = TX_RUNAT(globals->rtf.curRun, offset);
    }
    if ((globals->rtf.curText + nchars) > globals->rtf.lText) {
	int                 offset = TX_BYTES(globals->rtf.curText, &textBuf->text[0]);
	int                 allocate = 2 * textBuf->chunk.allocated + nchars;

	textBuf = (NXCharArray *) NXChunkZoneGrow(&textBuf->chunk, allocate, [self zone]);
	globals->rtf.curText = &textBuf->text[0];
	globals->rtf.lText = globals->rtf.curText + textBuf->chunk.allocated;
	globals->rtf.curText = globals->rtf.curText + offset;
    }
    bzero(&run, sizeof(NXRun));
    run.chars = nchars;
    setupTabs(self, prop);
    if (!equalProps(&globals->rtf.lastProps, prop)) {
	setRunFromProps(self,prop,fontList,colorList,&run);
	globals->rtf.lastProps = *prop;
	*globals->rtf.curRun = run;
	globals->rtf.curRun++;
	globalRuns->chunk.used += sizeof(NXRun);
    } else {
	NXRun              *lastRun = globals->rtf.curRun - 1;	/* dangerous, need to
							 * check start cond */
	lastRun->chars += nchars;
    }
    bcopy(buf, globals->rtf.curText, nchars);
    globals->rtf.curText += nchars;
    textBuf->chunk.used += nchars;
    globals->rtf.globalRuns = globalRuns;
    globals->rtf.textBuf = textBuf;
}

static void initTable(void)
{
    const NXSymbolTable *st;
    const char **times, *family;
    NXFontTraitMask trait;
    float size; 
    symHashTab = NXCreateHashTable(symProto, 55, NULL);
    /* load unhashed symbols into new hash table */
    for (st = unhashedSymbols; st->name; st++) 
	NXHashInsert(symHashTab, st);
    timesHashTab = NXCreateHashTable(timesProto, 0, NULL);
    for (times = TimesMap; *times; times++)
	NXHashInsert(timesHashTab, NXUniqueString(*times));
    defaultFont = [Font newFont:"Helvetica" size:12.0];
    [[FontManager new] getFamily:&family traits:&trait
	weight:&defaultWeight size:&size ofFont:defaultFont];
    timesName = NXUniqueString("Times-Roman");
}

  /* these functions are necessary for the shlibs */
static void dontFree(const void *info, void *data)
{ }

static unsigned symHash(const void *info, const void *data)
{
    const NXSymbolTable *real = data;
    return NXStrHash(info, real->name);
}

static int symEqual(const void *info, const void *data1, const void *data2)
{
    const NXSymbolTable *real1 = data1;
    const NXSymbolTable *real2 = data2;
    return NXStrIsEqual(info, real1->name, real2->name);
}

static unsigned doStyleHash(const void *info, const void *data)
{
    const unsigned char *first = data;
    const unsigned char *last = first + sizeof(NXTextStyle);
    unsigned hash = 0;
    while (first < last)
	hash += *first++;
    return hash;
}

static int styleEqual(const void *info, const void *data1, const void *data2)
{
    const unsigned char *real1 = data1;
    const unsigned char *real2 = data2;
    const unsigned char *last = real1 + sizeof(NXTextStyle);
    while (real1 < last) {
	if (*real1++ != *real2++)
	    return 0;
    }
    return 1;
}

static void styleFree(const void *info, void *data)
{
    const TextId *self = info;
    NXTextGlobals *globals = TEXTGLOBALS(self->_info);
    if (self->theRuns && (data != globals->defaultStyle))
	free(data);
}

static unsigned doStyleSheetHash(const void *info, const void *data)
{
    const NXStyleSheetEntry *style = data;
    return (unsigned) style->num;
}


static int styleSheetEqual(const void *info, const void *data1, const void *data2)
{
    const NXStyleSheetEntry *style1 = data1;
    const NXStyleSheetEntry *style2 = data2;
    return (style1->num == style2->num);
}

static void dofree(const void *info, void *data)
{
    free(data);
}


static void styleSheetFree(const void *info, void *data)
{
    NXStyleSheetEntry *style = data;
    free(style->name);
    free(style);
}

static unsigned dofontTableHash(const void *info, const void *data)
{
    const NXFontItem *font = data;
    return (unsigned) font->num;
}


static int fontTableEqual(const void *info, const void *data1, const void *data2)
{
    const NXFontItem *font1 = data1;
    const NXFontItem *font2 = data2;
    return (font1->num == font2->num);
}


static unsigned rtfFamilyHash(const void *info, const void *data)
{
    const RichTextFamily *rtf = data;
    return NXPtrHash(info, rtf->family);
}

static int rtfFamilyEqual(const void *info, const void *data1, const void *data2)
{
    const RichTextFamily *rtf1 = data1;
    const RichTextFamily *rtf2 = data2;
    return (rtf1->family == rtf2->family);
}

static unsigned writeFontHash(const void *info, const void *data)
{
    const NXFontEntry *fontEntry = data;
    return NXPtrHash(info, fontEntry->font);
}

static int writeFontEqual(const void *info, const void *data1, const void *data2)
{
    const NXFontEntry *fontEntry1 = data1;
    const NXFontEntry *fontEntry2 = data2;
    return (fontEntry1->font == fontEntry2->font);
}

static unsigned fontNumHash(const void *info, const void *data)
{
    const NXFontItem *item = data;
    return NXPtrHash(info, item->fontName);
}

static int fontNumEqual(const void *info, const void *data1, const void *data2)
{
    const NXFontItem *item1 = data1;
    const NXFontItem *item2 = data2;
    return (item1->fontName == item2->fontName);
}

static NXHashTablePrototype symProto = {
    symHash, symEqual, dontFree, 0
};
static NXHashTablePrototype timesProto = {
    0, 0, dontFree, 0
};
static NXHashTablePrototype rtfFamilyProto = {
    rtfFamilyHash, rtfFamilyEqual, dontFree, 0
};
static NXHashTablePrototype styleProto = {
    doStyleHash, styleEqual, styleFree, 0
};
static NXHashTablePrototype styleSheetProto = {
    doStyleSheetHash, styleSheetEqual, styleSheetFree, 0
};
static NXHashTablePrototype fontProto = {
    dofontTableHash, fontTableEqual, dofree, 0
};
static NXHashTablePrototype writeFontProto = {
    writeFontHash, writeFontEqual, dofree, 0
};
static NXHashTablePrototype fontNumProto = {
    fontNumHash, fontNumEqual, dofree, 0
};

/*

84
--

 5/11/90 chris	modified readRTF:op: to deal with empty text with run info
 
85
--

 6/4/90 gcockrft	various performance tweaks to tokenizer. Put a 2 level deep cache
    in getRunFont. Bug fixes for 5726 & 5777. Added a faster call for replacing text 
	if readRTF op is archive_io.

87
--
7/12/90	glc	Changes to support color. Color table is read and written.
		Fixed leaks in the hashtables. Added dofree's
		
89
--
7/27/90	glc	Setup tabs correctly for rtf files without tabs. (mainly 1.0 RTF)
		Optimized RTF file writing.
		
91
--
 8/11/90 glc	Fix for RTF of imbedded graphics broken in optimization.
 		Fix handling of \pard. It should not change font style info.
		Fix for crash when reading in Mac RTF.
		

94
--
 9/25/90 gcockrft	Fix for bug when RTF files had \r instead \n.
 			Used isspace()

99
--
10/17/90 glc    Changes to support reentrant RTF

100
---
10/11/90 glc	Fix for 9689 defaults in BugTracker submit window
		Fix for 10814 correct handling of plus pack font names
		
10/24/90 glc	Make sure getToken return is always checked.
		Let styles get by without a number. 
 
 */




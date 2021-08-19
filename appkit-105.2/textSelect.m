/*
	textSelect.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Application.h"
#import "Pasteboard.h"
#import "Text.h"
#import "nextstd.h"
#import "textprivate.h"
#import "textWraps.h"
#import <math.h>
#import <string.h>
#import <dpsclient/wraps.h>
#import <mach.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategorySelection=0\n");
    asm(".globl .NXTextCategorySelection\n");
#endif

static const char *asciitype = NULL;
static const char *rtftype = NULL;

typedef struct {
    int first;
    int underline;
} UnderlineInfo;

@implementation Text (Selection)

- getSel:(NXSelPt *)start :(NXSelPt *)end
{
    *start = sp0;
    *end = spN;
    return self;
}

- hideCaret
{
    NXRect              vRect;
    BOOL canDraw = [self _canDraw];

    if (tFlags._editMode == READONLY)
	return self;
    if (tFlags._caretState != CARETDISABLED) {
	if (cursorTE) {
	    DPSRemoveTimedEntry(cursorTE);
	    cursorTE = NULL;
	}
	if (tFlags._caretState == CARETON && canDraw) {
	    [self lockFocus];
	    if ([self getVisibleRect:&vRect])
		_NXhidecaret(vRect.origin.x, vRect.origin.y,
			     vRect.size.width, vRect.size.height);
	    [self unlockFocus];
	}
	tFlags._caretState = CARETDISABLED;
    };
    return self;
}

- (BOOL)isSelectable
{
    return ([self acceptsFirstResponder] != 0);
}

- replaceSel:(const char *)aString
{
    int                 length;

    length = aString ? strlen(aString) : 0;
    [self replaceSel:aString length:length];
    return self;
}

- replaceSel:(const char *)aString length:(int)length
{
    return [self _replaceSel:aString length:length smartPaste:NO];
}

- replaceSel:(const char *)aString length:(int)length runs:(NXRunArray *)insertRuns
{
    NXTextInfo *info = _info;
    BOOL                canDraw = [self _canDraw];

    if (sp0.cp < 0)
	return self;
    NXAdjustTextCache(self, &info->layInfo.cache, sp0.cp);
    if (canDraw)
	[self lockFocus];
    [self _replaceSel:NULL :aString :length :insertRuns :0 :0 :0];
    if (canDraw) {
	if (FLUSHWINDOW(window))
	    PSflushgraphics();
	[self unlockFocus];
    }
    return self;
}

- replaceSelWithRichText:(NXStream *)stream
{
    [self readRTF:stream op:pasteboard_io];
    return self;
}

- scrollSelToVisible
{
    NXRect              _vis, _selRect;
    NXRect    *vis = &_vis;
    NXRect    *sel = &_selRect;
    NXCoord             minX, maxx, diffY;
    BOOL                visibleX = NO, visibleY = NO;

    [self getVisibleRect:vis];
    if (sp0.cp < 0) {
	NXSetRect(sel, 0., 0., NX_WIDTH(vis), NX_HEIGHT(vis));
	[self scrollRectToVisible:sel];
	return self;
    }
    minX = MIN(sp0.x, spN.x);
    maxx = MAX(sp0.x, spN.x);
    NXSetRect(sel, minX, sp0.y, maxx - minX, (spN.y - sp0.y) + spN.ht);
    [self getVisibleRect:vis];
    if (NX_Y(sel) >= NX_Y(vis) && NX_Y(sel) <= NX_MAXY(vis) &&
	NX_MAXY(sel) >= NX_Y(vis) && NX_MAXY(sel) <= NX_MAXY(vis))
	visibleY = YES;
    if (NX_X(sel) >= NX_X(vis) && NX_X(sel) <= NX_MAXX(vis) &&
	NX_MAXX(sel) >= NX_X(vis) && NX_MAXX(sel) <= NX_MAXX(vis))
	visibleX = YES;
    if (visibleY && visibleX)
	return self;
    if (NX_HEIGHT(sel) > NX_HEIGHT(vis))
	NX_HEIGHT(sel) = NX_HEIGHT(vis);
    if (NX_HEIGHT(vis) > NX_HEIGHT(sel)) {
	NXCoord             change1 = floor((NX_HEIGHT(vis) - NX_HEIGHT(sel)) / 2.0);
	NXCoord             change2 = floor(NX_HEIGHT(vis) / 4.0);
	NXCoord             change = MIN(change1, change2);

	if (NX_Y(sel) < change)
	    NX_Y(sel) = 0.0;
	else
	    NX_Y(sel) -= change;
    }
    NX_HEIGHT(sel) = NX_HEIGHT(vis);
    diffY = NX_MAXY(sel) - NX_MAXY(&bounds);
    if (diffY > 0.0) {
	if (NX_Y(sel) < diffY)
	    NX_Y(sel) = 0.0;
	else
	    NX_Y(sel) -= diffY;
    }
    if (visibleX) {
	NX_X(sel) = NX_X(vis);
	NX_WIDTH(sel) = NX_WIDTH(vis);
    }
    [self scrollRectToVisible:sel];
    return self;
}

- selectError
{
    typingRun.chars = 0;
    [self setSel:0 :MAXINT];
    return self;
}

- selectNull
{
    BOOL                canDraw = [self _canDraw];

    typingRun.chars = 0;
    if (sp0.cp >= 0 && canDraw) {
	[self lockFocus];
	[self hideCaret];
	if (!ISNULLSEL)
	    [self _hilite:NO :&sp0:&spN:NULL];
	if (FLUSHWINDOW(window))
	    PSflushgraphics();
	[self unlockFocus];
    } else
	[self hideCaret];	/* dont factor this out - saves focusing */
    sp0.cp = -1;
    return self;
}

- setSel:(int)start :(int)end
{
    [self _setSelPrologue : start == end ? YES : NO];
    [self _setSel:start :end];
    [self _setSelPostlogue];
    return self;
}

- setSelectable:(BOOL)flag
{
    tFlags._selectMode = (flag) ? SELECTABLE : NOTSELECTABLE;
    return self;
}

- showCaret
{

    BOOL canDraw = [self _canDraw];
    if (![window isKeyWindow] || ![self acceptsFirstResponder] ||
	tFlags._editMode == READONLY)
	return self;
    if ((tFlags._caretState == CARETDISABLED) &&
	(tFlags.haveDown == SINotTracking)) {
	if ((sp0.cp >= 0) && (sp0.cp == spN.cp) && (sp0.line == spN.line)) {
	    tFlags._caretState = CARETOFF;
	    if (canDraw)
		_NXBlinkCaret(0, 0.7, self, 1);
	    cursorTE = DPSAddTimedEntry(0.7, _NXBlinkCaretTE, self,
					NX_RUNMODALTHRESHOLD);
	/*
	 * !!! what to do about errors? 
	 */
	}
    }
    return self;
}

- _underline:(NXRun *)run with:(void *)data;
{
    UnderlineInfo *info = data;
    if (!run)
	return self;
    if (info->first) {
	info->first = NO;
	info->underline = run->rFlags.underline ? NO : YES;
    }
    run->rFlags.underline = info->underline ? YES : NO;
    return self;
}

- underline:sender
{
    UnderlineInfo info;
    info.first = YES;
    if (sp0.cp >= 0 && !tFlags.monoFont)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_underline:with:) with:&info];
    return self;
}

- _subscript:(NXRun *)run with:(void *)data;
{
    NXCoord sub;
    int new;
    if (!run)
	return self;
    sub = ([run->font pointSize] * 4.)/10.;
    new = run->subscript + (int)sub;
    run->superscript = 0;
    if (new < 256)
	run->subscript = new;
    return self;
}

- subscript:sender
{
    if (sp0.cp >= 0 && !tFlags.monoFont)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_subscript:with:) with:NULL];
    return self;
}

- _superscript:(NXRun *)run with:(void *)data;
{
    NXCoord super;
    int new;
    if (!run)
	return self;
    super = ([run->font pointSize] * 4.)/10.;
    new = run->superscript + (int)super;
    run->subscript = 0;
    if (new < 256)
	run->superscript = new;
    return self;
}

- superscript:sender
{
    if (sp0.cp >= 0 && !tFlags.monoFont)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_superscript:with:) with:NULL];
    return self;
}

- _unscript:(NXRun *)run with:(void *)data;
{
    if (!run)
	return self;
    run->superscript = 0;
    run->subscript = 0;
    return self;
}

- unscript:sender
{
    if (sp0.cp >= 0)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_unscript:with:) with:NULL];
    return self;
}

- _changeGray:(NXRun *)run with:(void *)data;
{
    float *gray = data;
    if (!run)
	return self;
    run->textGray = *gray;
    return self;
}

- _changeColor:(NXRun *)run with:(void *)data;
{
    int *RGBColor = data;
    if (!run)
	return self;
    run->textRGBColor = *RGBColor;
    return self;
}

- setSelGray:(NXCoord) gray
{
    if (sp0.cp >= 0)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeGray:with:) with:&gray];
    return self;
}

/*
 * Set all the selected runs to a particular color.
 * If the color is a gray value, remove the color from the runs & just set gray
 * value. Normally when setting color the gray value is still left inplace, and
 * used when a window is not on a color display. Might as well just use the gray
 * value if the color is gray. 
 */

- setSelColor:(NXColor) color
{
    float	red,green,blue,alpha;
    int		RGBColor;
    
    if (sp0.cp < 0)
	return self;
	
    NXConvertColorToRGBA (color, &red, &green, &blue, &alpha);
    if(red == blue && red == green) {
	RGBColor = -1;	
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeGray:with:) with:&red];
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeColor:with:) with:&RGBColor];
    }
    else {
	RGBColor = _NXToTextRGBColor(color);
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeColor:with:) with:&RGBColor];
    }
    return self;
}


- validRequestorForSendType:(NXAtom)sendType andReturnType:(NXAtom)returnType
{
    NXSelPt start, end;

    if (!asciitype) asciitype = NXUniqueString(NXAsciiPboardType);
    if (!rtftype) rtftype = NXUniqueString(NXRTFPboardType);

    if (!((sendType == NULL || sendType == asciitype || sendType == rtftype) && (returnType == NULL || returnType == asciitype || returnType == rtftype)))
	return [super validRequestorForSendType:sendType andReturnType:returnType];

    [self getSel:&start :&end];

    if (start.cp >= 0 && (!sendType || start.cp != end.cp) && (!returnType || [self isEditable])) {
	return self;
    } else {
	return [super validRequestorForSendType:sendType andReturnType:returnType];
    }
}

- readSelectionFromPasteboard:pboard
{
    return [self _pastefrompboard:pboard];
}

- (BOOL)writeSelectionToPasteboard:pboard types:(NXAtom *)types
{
    BOOL writeRTF = NO, writeASCII = NO;

    if (!asciitype) asciitype = NXUniqueString(NXAsciiPboardType);
    if (!rtftype) rtftype = NXUniqueString(NXRTFPboardType);
    while (types && *types) {
	if (*types == rtftype) writeRTF = YES;
	if (*types == asciitype) writeASCII = YES;
	types++;
    }
    if (!writeRTF && !writeASCII) return NO;

    return [self _copytopboard:pboard writeRTF:writeRTF] ? YES : NO;
}
    

@end		/* Selection */

@implementation Text (Font)
- changeFont:sender
{
    int oldsp0, oldspN, chars;
    BOOL canFlush = FLUSHWINDOW(window);
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    SenderAndStyle data;
    if (tFlags.disableFontPanel || sp0.cp < 0)
	return nil;
    globals->gFlags.changingFont = YES;
    data.paraStyle = 0;
    data.sender = sender;
    oldsp0 = sp0.cp;
    oldspN = spN.cp;
    chars = typingRun.chars;
    [window disableFlushWindow];
    tFlags.disableFontPanel = YES;
    [self setSel:sp0.cp:sp0.cp];
    typingRun.chars = chars;		/* gets trashed by setSel:: */
    [self replaceRunFrom:oldsp0 to:oldspN 
	selector:@selector(_convertFont:with:) with:&data];
    chars = typingRun.chars;	      /* just in case we picked a typing run */
    [self setSel:oldsp0:oldspN];
    tFlags.disableFontPanel = NO;
    [window reenableFlushWindow];
    if (canFlush)
	[window flushWindow];
    typingRun.chars = chars;		/* gets trashed by setSel:: */
    globals->gFlags.changingFont = NO;
    return self;
}

- font
{
    return theRuns->runs[0].font;
}

- (BOOL)isFontPanelEnabled
{
    return tFlags.disableFontPanel ? NO : YES;
}

- (BOOL)isMonoFont
{
    return (tFlags.monoFont);
}

- setFont:fontObj
{
    return [self _setFont:fontObj paraStyle:0];
}

- setFont:fontObj paraStyle:(void *)paraStyle
{
    return [self _setFont:fontObj paraStyle:paraStyle];
}

- setFontPanelEnabled: (BOOL)flag
{
    tFlags.disableFontPanel = flag ? NO : YES;
    return self;
}

- setMonoFont:(BOOL)flag
{
      /* If becoming mono make sure we do not have a ruler */
    if(!tFlags.monoFont && flag && HAVE_RULER(_info))
	[self toggleRuler:self];
    tFlags.monoFont = flag ? YES : NO;
    return self;
}

- _changeRun:(NXRun *)run with:(void *)data;
{
    FontAndStyle *fs = data;
    if (!run)
	return self;
    if (fs->font)
	run->font = fs->font;
    if (fs->paraStyle)
	run->paraStyle = fs->paraStyle;
    return self;
}

- setSelFont:fontId
{
    FontAndStyle data;
    data.font = fontId;
    data.paraStyle = 0;
    if (sp0.cp >= 0)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeRun:with:) with:&data];
    return self;
}

- setSelFont:fontId paraStyle:(void *)paraStyle
{
    FontAndStyle data;
    data.font = fontId;
    data.paraStyle = [self getUniqueStyle:paraStyle];
    if (sp0.cp >= 0)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeRun:with:) with:&data];
    return self;
}

- _changeFamily:(NXRun *)run with:(void *)data;
{
    char *family = data;
    const char *oldFamily;
    NXFontTraitMask traits;
    int weight;
    float size;
    id newFont, fontManager = [FontManager new];
    if (!run)
	return self;
    [fontManager getFamily:&oldFamily traits:&traits 
	weight:&weight size:&size ofFont:run->font];
    newFont = [fontManager findFont:family traits:traits
	weight:weight size:size];
    if (newFont)
	run->font = newFont;
    return self;
}

- setSelFontFamily:(const char *)fontName
{
    if (sp0.cp >= 0)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeFamily:with:) with:(void *)fontName];
    return self;
}

- _changeSize:(NXRun *)run with:(void *)data;
{
    float *newSize = data;
    const char *oldFamily;
    NXFontTraitMask traits;
    int weight;
    float size;
    id newFont, fontManager = [FontManager new];
    if (!run)
	return self;
    [fontManager getFamily:&oldFamily traits:&traits 
	weight:&weight size:&size ofFont:run->font];
    newFont = [fontManager findFont:oldFamily traits:traits
	weight:weight size:*newSize];
    if (newFont)
	run->font = newFont;
    return self;
}

- setSelFontSize:(float)size
{
    if (sp0.cp >= 0)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeSize:with:) with:&size];
    return self;
}

- _changeTrait:(NXRun *)run with:(void *)data;
{
    NXFontTraitMask *newTrait = data;
    const char *oldFamily;
    NXFontTraitMask traits;
    int weight;
    float size;
    id newFont, fontManager = [FontManager new];
    if (!run)
	return self;
    [fontManager getFamily:&oldFamily traits:&traits 
	weight:&weight size:&size ofFont:run->font];
    newFont = [fontManager findFont:oldFamily traits:*newTrait
	weight:weight size:size];
    if (newFont)
	run->font = newFont;
    return self;
}

- setSelFontStyle:(NXFontTraitMask)fontStyle
{
    if (sp0.cp >= 0)
	[self replaceRunFrom:sp0.cp to:spN.cp
	    selector:@selector(_changeTrait:with:) with:&fontStyle];
    return self;
}

- changeTabStopAt:(NXCoord)oldX to:(NXCoord)newX
{
    NXTextInfo *info = _info;
    NXTextCache *cache = &info->layInfo.cache;
    NXTextStyle temp;
    NXTabStop *tabs, *last;
    BOOL autodisplay = vFlags.disableAutodisplay;
    NXZone *zone = [self zone];
    
    if (sp0.cp < 0)
	return nil;
    vFlags.disableAutodisplay = YES;
    [self selectParagraph];
    vFlags.disableAutodisplay = autodisplay;
    NXAdjustTextCache(self, cache, sp0.cp);
    temp = *((NXTextStyle *) cache->curRun->paraStyle);
    NX_ZONEMALLOC(zone, tabs, NXTabStop, temp.numTabs);
    bcopy(temp.tabs, tabs, sizeof(NXTabStop) * temp.numTabs);
    temp.tabs = tabs;
    last = tabs + temp.numTabs;
    for (; tabs < last; tabs++) {
	if (tabs->x == oldX) {
	    tabs->x = newX;
	    break;
	}
    }
    [[self class] sortTabs:temp.tabs num:temp.numTabs];
    [self setSelFont:nil paraStyle:&temp];
    free(temp.tabs);
    return self;
}

- setSelProp:(NXParagraphProp)prop to:(NXCoord)val
{
    NXTextInfo *info = _info;
    NXCoord l, r, t, b;
    NXTextCache *cache = &info->layInfo.cache;
    NXTextStyle temp;
    NXTabStop *tabs, *last;
    BOOL autodisplay = vFlags.disableAutodisplay;
    NXRect visRect;
    NXZone *zone = [self zone];
    
    if (sp0.cp < 0)
	return nil;
    vFlags.disableAutodisplay = YES;
    [self selectParagraph];
    vFlags.disableAutodisplay = autodisplay;
    NXAdjustTextCache(self, cache, sp0.cp);
    temp = *((NXTextStyle *) cache->curRun->paraStyle);
    switch (prop) {
	case NX_LEFTALIGN:
	    temp.alignment = NX_LEFTALIGNED;
	    [self setSelFont:nil paraStyle:&temp];
	    break;
	case NX_RIGHTALIGN:
	    temp.alignment = NX_RIGHTALIGNED;
	    [self setSelFont:nil paraStyle:&temp];
	    break;
	case NX_CENTERALIGN:
	    temp.alignment = NX_CENTERED;
	    [self setSelFont:nil paraStyle:&temp];
	    break;
	case NX_FIRSTINDENT:
	    temp.indent1st = val;
	    [self setSelFont:nil paraStyle:&temp];
	    break;
	case NX_INDENT:
	    temp.indent2nd = val;
	    [self setSelFont:nil paraStyle:&temp];
	    break;
	case NX_ADDTAB:
	    NX_ZONEMALLOC(zone, tabs, NXTabStop, temp.numTabs+1);
	    bcopy(temp.tabs, tabs, sizeof(NXTabStop) * temp.numTabs);
	    temp.tabs = tabs;
	    tabs += temp.numTabs;
	    tabs->x = val;
	    tabs->kind = NX_LEFTTAB;
	    temp.numTabs++;
	    [[self class] sortTabs:temp.tabs num:temp.numTabs];
	    [self setSelFont:nil paraStyle:&temp];
	    free(temp.tabs);
	    break;
	case NX_REMOVETAB:
	    NX_ZONEMALLOC(zone, tabs, NXTabStop, temp.numTabs);
	    bcopy(temp.tabs, tabs, sizeof(NXTabStop) * temp.numTabs);
	    temp.tabs = tabs;
	    last = tabs + temp.numTabs;
	    for (; tabs < last; tabs++) {
		if (tabs->x == val) {
		    tabs->x = 1.0e38;
		    break;
		}
	    }
	    [[self class] sortTabs:temp.tabs num:temp.numTabs];
	    temp.numTabs--;
	    [self setSelFont:nil paraStyle:&temp];
	    free(temp.tabs);
	    break;
	case NX_LEFTMARGIN:
	case NX_RIGHTMARGIN:
	    [self getMarginLeft:&l right:&r top:&t bottom:&b];
	    [self getVisibleRect:&visRect];
	    if ([self _canDraw]) {
		[self lockFocus];
		PSsetgray(backgroundGray);
		NXRectFill(&visRect);
		[self unlockFocus];
	    }
	    if (prop == NX_LEFTMARGIN)
		[self setMarginLeft:val right:r top:t bottom:b];
	    else
		[self setMarginLeft:l right:val top:t bottom:b];
	    [self calcLine];
	    if (HAVE_RULER(_info))
		[self updateRuler];
	    break;
	case NX_JUSTALIGN:
	default:
	    break;
    }
    return self;
}

- _alignSel:(int)how
{
    if([self isMonoFont]) {
    	int	saveOpaque;
	saveOpaque = vFlags.opaque;
	vFlags.opaque = 1;
	[self setAlignment:how];
	[self calcLine];
	vFlags.opaque = saveOpaque;    
	}
    else {
	[self setSelProp:how to:0];
    }
    return self;
}

- alignSelLeft:sender
{
    return([self _alignSel:NX_LEFTALIGN]);
}

- alignSelRight:sender
{
    return([self _alignSel:NX_RIGHTALIGN]);
}

- alignSelCenter:sender
{
    return([self _alignSel:NX_CENTERALIGN]);
}



@end		/* Font */

/*

Modifications:

80
--
  4/10/90 pah	added support for Request Menu (validRequestor:,
		 readSelectionFromPasteboard:type:, and
		 writeSelectionToPasteboard:type:)
  5/09/90 chris added check for nil return from readType:data:length:

86
--
 6/12/90 pah	removed validRequestor: and added
		 validRequestForSendType:andReturnType: in support of
		 new Request Menu scheme

92
--
 8/20/90 gcockrft  RTF support for services menu stuff.
 			Don't allow ruler on mono font text objects.
 
94
--
 9/25/90 gcockrft	Fix for changing selection alignment in monofonts.	
 
 */

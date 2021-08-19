/*
	Cell.m
	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Control_Private.h"
#import "ClipView_Private.h"
#import "View_Private.h"
#import "textprivate.h"
#import "Cell.h"
#import "Application.h"
#import "Font.h"
#import "Text.h"
#import "Bitmap.h"
#import "NXImage.h"
#import "NXCursor.h"
#import "Window.h"
#import "nextstd.h"
#import "timer.h"
#import <dpsclient/wraps.h>
#import <objc/List.h>
#import <math.h>
#import <ctype.h>
#import <zone.h>

@interface Bitmap(InternalUseOnly)
+ findBitmapFor:(const char *)name;
+ _findExistingBitmap:(const char *)name;
@end

@interface NXImage(InternalUseOnly)
+ _imageNamed:(const char *)aString;
- (BOOL)_asIconHasAlpha;
@end

@interface Text(Private)
- _allowXYShowOptimization:(BOOL)flag;
@end

typedef struct {
    @defs (Text)
}                  *textId;
typedef textId      viewId;

#define DFLTFONTSIZE 	12.0
#define MAXCLIPVIEWS	8
#define CELLOFFSET 	(cFlags1.bordered || cFlags1.bezeled) ? \
			(cFlags1.bordered ? 2.0 : 3.0) : 0.0
#define TYPEISFP(entryType) (entryType == NX_FLOATTYPE || \
			     entryType == NX_DOUBLETYPE || \
			     entryType == NX_POSFLOATTYPE || \
			     entryType == NX_POSDOUBLETYPE)

static const char  *const defaultTitle = "Cell";
static const char  *const defaultIcon = "NXsquare16";
static const NXRect bigRect = {{0.0, 0.0}, {10000.0, 10000.0}};

static id           wrapText = nil;
static id           noWrapText = nil;
static id           clipViewList = nil;
static id           drawList = nil;
static int          mdFlags = 0;

@implementation Cell


- _initClassVars
{
    NXRect newR;
    NXZone *zone;

    newR.origin.x = newR.origin.y = newR.size.width = newR.size.height = 0.0;
    if (!wrapText) {
	zone = [NXApp zone];
	zone = zone ? zone : NXDefaultMallocZone();
	wrapText = [[Text allocFromZone:zone] _initFrame:&newR text:NULL alignment:NX_LEFTALIGNED services:NO];
	[wrapText _allowXYShowOptimization:YES];
	[wrapText setClipping:NO];
	[wrapText setAutodisplay:NO];
	[wrapText setCharFilter:NXFieldFilter];
	[wrapText removeFromSuperview];
	[wrapText _setWindow:nil];
	[wrapText setFontPanelEnabled:NO];
	noWrapText = [[Text allocFromZone:zone] _initFrame:&newR text:NULL alignment:NX_LEFTALIGNED services:NO];
	[noWrapText _allowXYShowOptimization:YES];
	[noWrapText setClipping:NO];
	[noWrapText setAutodisplay:NO];
	[noWrapText setCharFilter:NXFieldFilter];
	[noWrapText removeFromSuperview];
	[noWrapText _setWindow:nil];
	[noWrapText setNoWrap];
	[noWrapText setFontPanelEnabled:NO];
	drawList = [[List allocFromZone:zone] init];
	[drawList addObject:wrapText];
	clipViewList = [[List allocFromZone:zone] initCount:MAXCLIPVIEWS];
	[clipViewList addObject:[[[ClipView allocFromZone:zone] init] _markUsedByCell]];
    }

    return self;
}

+ initialize
{
    [self setVersion:1];
    return self;
}

+ new
{
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

+ (BOOL)prefersTrackingUntilMouseUp
{
    return NO;
}

- init
{
    [super init];
    if (!wrapText) [self _initClassVars];
    return self;
}


- _convertToText
{
    if (self->cFlags1.type != NX_TEXTCELL) {
	self->cFlags1.type = NX_TEXTCELL;
	[self setFont:[Font newFont:NXSystemFont size:DFLTFONTSIZE]];
	[self setStringValueNoCopy:(char *)defaultTitle shouldFree:NO];
    }
    return self;
}


+ newTextCell
{
    return [[self newTextCell:NULL] setStringValueNoCopy:defaultTitle];
}

+ newTextCell:(const char *)aString
{
    return [[self allocFromZone:NXDefaultMallocZone()] initTextCell:aString];
}

- initTextCell:(const char *)aString
{
    [super init];
    if (!wrapText) [self _initClassVars];
    return [self setStringValue:aString];
}


+ newIconCell
{
    return [self newIconCell:NULL];
}


+ newIconCell:(const char *)iconName
{
    return [[self allocFromZone:NXDefaultMallocZone()] initIconCell:iconName];
}

- initIconCell:(const char *)iconName
{
    [super init];
    if (!wrapText) [self _initClassVars];
    return[self setIcon:iconName];
}


- copy
{
    return [self copyFromZone:[self zone]];
}

- copyFromZone:(NXZone *)zone
{
    Cell *retval;
    retval = [super copyFromZone:zone];
    if (cFlags1.freeText && contents) retval->contents = 
	NXCopyStringBufferFromZone(contents,  zone);
    return retval;
}


- awake
{
    if (!wrapText)
	[self _initClassVars];
    return[super awake];
}


- free
{
    if (cFlags1.freeText) {
	free(contents);
	contents = NULL;
    }
    return[super free];
}


- _dontFreeText
{
    cFlags1.freeText = NO;
    return self;
}

- _useUserKeyEquivalent
{
    _cFlags3.useUserKeyEquivalent = YES;
    return self;
}

- (BOOL)_usesUserKeyEquivalent
{
    return _cFlags3.useUserKeyEquivalent;
}

- _setView:view
{
    return self;
}


- controlView
{
    return nil;
}

- (int)type
{
    return cFlags1.type;
}


- setType:(int)aType
{
    if (cFlags1.type != aType) {
	if (aType == NX_TEXTCELL) {
	    [self _convertToText];
	} else if (aType == NX_ICONCELL) {
	    [self setIcon:NULL];
	} else {
	    cFlags1.type = aType;
	}
    }
    return self;
}


- (int)state
{
    return cFlags1.state;
}


- setState:(int)value
{
    cFlags1.state = value ? 1 : 0;
    return self;
}


- incrementState
{
    cFlags1.state = cFlags1.state ? 0 : 1;
    return self;
}


- target
{
    return nil;
}


- setTarget:anObject
{
    return self;
}


- (SEL)action
{
    return (SEL)0;
}


- setAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "Cell -setAction:");
    return self;
}


- (int)tag
{
    return -1;
}


- setTag:(int)anInt
{
    return self;
}


- (BOOL)isOpaque
{
    return (BOOL)(cFlags1.bezeled);
}


- (BOOL)isEnabled
{
    return (BOOL)(!cFlags1.disabled);
}


- setEnabled:(BOOL)flag
{
    cFlags1.disabled = flag ? NO : YES;
    return self;
}


- (int)sendActionOn:(int)mask
{
    int                 retval;

    retval = (cFlags2.dontActOnMouseUp) ? 0 : NX_MOUSEUPMASK;
    retval |= (cFlags2.actOnMouseDown) ? NX_MOUSEDOWNMASK : 0;
    retval |= (cFlags2.actOnMouseDragged) ? NX_MOUSEDRAGGEDMASK : 0;
    retval |= (cFlags2.continuous) ? NX_PERIODICMASK : 0;

    cFlags2.dontActOnMouseUp = (mask & NX_MOUSEUPMASK) ? NO : YES;
    cFlags2.actOnMouseDown = (mask & NX_MOUSEDOWNMASK) ? YES : NO;
    cFlags2.actOnMouseDragged = (mask & NX_MOUSEDRAGGEDMASK) ? YES : NO;
    cFlags2.continuous = (mask & NX_PERIODICMASK) ? YES : NO;

    return retval;
}


- (BOOL)isContinuous
{
    return (BOOL)(cFlags2.continuous);
}


- setContinuous:(BOOL)flag
{
    cFlags2.continuous = flag ? YES : NO;
    return self;
}


- (BOOL)isEditable
{
    return (BOOL)(cFlags1.editable);
}


- setEditable:(BOOL)flag
{
    cFlags1.editable = flag ? YES : NO;
    cFlags1.selectable = cFlags1.editable;
    return self;
}


- (BOOL)isSelectable
{
    return (BOOL)(cFlags1.selectable || cFlags1.editable);
}


- setSelectable:(BOOL)flag
{
    cFlags1.editable = NO;
    cFlags1.selectable = flag ? YES : NO;
    return self;
}


- (BOOL)isBordered
{
    return (BOOL)(cFlags1.bordered);
}


- setBordered:(BOOL)flag
{
    cFlags1.bordered = (flag ? YES : NO);
    cFlags1.bezeled = NO;
    return self;
}

- (BOOL)isBezeled
{
    return (BOOL)(cFlags1.bezeled);
}


- setBezeled:(BOOL)flag
{
    cFlags1.bezeled = (flag ? YES : NO);
    cFlags1.bordered = NO;
    return self;
}


- (BOOL)isScrollable
{
    return (BOOL)(cFlags1.scrollable);
}


- setScrollable:(BOOL)flag
{
    cFlags1.scrollable = flag ? YES : NO;
    if (flag) [self setWrap:NO];
    return self;
}


- (BOOL)isHighlighted
{
    return (BOOL)(cFlags1.highlighted);
}


/*--------------------------*/
/* TextCell related methods */
/*--------------------------*/


- (int)alignment
{
    return cFlags1.alignment;
}


- _setCentered:(BOOL)flag
{
    _cFlags3.center = flag ? YES : NO;
    return self;
}

- setAlignment:(int)mode
{
    cFlags1.alignment = mode;
    return self;
}

- setWrap:(BOOL)flag
{
    cFlags2.noWrap = flag ? NO : YES;
    if (flag) [self setScrollable:NO];
    return self;
}

- font
{
    return (cFlags1.type == NX_TEXTCELL) ? support : nil;
}


- setFont:fontObj
{
    if (cFlags1.type == NX_TEXTCELL)
	support = fontObj;
    return self;
}


- (int)entryType
{
    return cFlags1.entryType;
}


- setEntryType:(int)aType
{
    if (cFlags1.type != NX_TEXTCELL)
	[self _convertToText];
    cFlags1.entryType = aType;
    return self;
}


#define skipwhitespace while (*p == ' ' || *p == '\t' || *p == '\n') p++

static
BOOL
validInt(BOOL signOk, const char *p)
{
    BOOL nonzero = NO;

    skipwhitespace;
    if (signOk && (*p == '+' || *p == '-'))
	p++;
    skipwhitespace;
    if (*p) {
	while (*p >= '0' && *p <= '9') {
	    if (*p != '0') nonzero = YES;
	    p++;
	}
    }
    skipwhitespace;

    return *p ? NO : (signOk || nonzero);
}

BOOL
_NXExtractDouble(const char *string, double *theDouble)
{
    char               *p = (char *)string;
    double              d = 0.0;
    int                 exponent, dp = 10;
    BOOL                negexp = NO, negmant = NO;

    skipwhitespace;
    if (*p == '-') {
	negmant = YES;
	p++;
	skipwhitespace;
    } else if (*p == '+') {
	p++;
    }
    if (isdigit(*p) || *p == '.') {
	if (*p != '.') {
	    d = (double)((*p++) - '0');
	    while (isdigit(*p))
		d = d * 10.0 + (double)((*p++) - '0');
	    if (*p != '.')
		goto scanforexponent;
	}
	p++;
	while (isdigit(*p)) {
	    d += (double)((*p++) - '0') / (double)dp;
	    dp *= 10;
	}
    } else {
	return NO;
    }

scanforexponent:

    if (*p == 'e') {
	p++;
	if (*p == '-') {
	    negexp = 1;
	    p++;
	} else if (*p == '+') {
	    negexp = 0;
	    p++;
	} else if (!isdigit(*p)) {
	    return NO;
	}
	if (isdigit(*p)) {
	    exponent = (*p) - '0';
	} else {
	    return NO;
	}
	p++;
	while (isdigit(*p))
	    exponent = exponent * 10 + (*p++) - '0';
	if (exponent) {
	    d *= pow(10.0, negexp ? (double)(0 - exponent) : (double)exponent);
	}
    }
    skipwhitespace;
    *theDouble = negmant ? 0.0 - d : d;

    return (*p ? NO : YES);
}

- (BOOL)isEntryAcceptable:(const char *)aString
{
    double              d;

    if (!aString || !aString[0]) {
	return (cFlags1.entryType != NX_POSINTTYPE &&
		cFlags1.entryType != NX_POSFLOATTYPE &&
		cFlags1.entryType != NX_POSDOUBLETYPE);
    }

    switch (cFlags1.entryType) {
    case NX_ANYTYPE:
	return YES;
    case NX_INTTYPE:
    case NX_POSINTTYPE:
	return validInt(cFlags1.entryType == NX_INTTYPE, aString);
    case NX_FLOATTYPE:
    case NX_DOUBLETYPE:
	return (_NXExtractDouble(aString, &d));
    case NX_POSFLOATTYPE:
    case NX_POSDOUBLETYPE:
	return (_NXExtractDouble(aString, &d) && d > 0.0);
    }

    return YES;
}


- setFloatingPointFormat:(BOOL)autoRange
    left:(unsigned)leftDigits
    right:(unsigned)rightDigits
{
    cFlags2.floatLeft = leftDigits > 10 ? 10 : leftDigits;
    cFlags2.floatRight = rightDigits > 14 ? 14 : rightDigits;
    cFlags2.autoRange = autoRange ? YES : NO;
    if (leftDigits && !TYPEISFP(cFlags1.entryType)) {
	cFlags1.entryType = NX_FLOATTYPE;
    }
    return self;
}


- (unsigned short)keyEquivalent
{
    return 0;
}


- (const char *)stringValue
{
    return contents;
}


- setStringValue:(const char *)aString
{
    double              d;

    if (cFlags1.type != NX_TEXTCELL) {
	cFlags1.type = NX_TEXTCELL;
	[self setFont:[Font newFont:NXSystemFont size:DFLTFONTSIZE]];
    }
    if (aString) {
	if (cFlags2.floatLeft && TYPEISFP(cFlags1.entryType) &&
	    _NXExtractDouble((char *)aString, &d)) {
	    if (contents && cFlags1.freeText) {
		free(contents);
	    }
	    contents = NULL;
	    if (cFlags1.entryType == NX_DOUBLETYPE ||
		cFlags1.entryType == NX_POSDOUBLETYPE) {
		[self setDoubleValue:d];
	    } else {
		[self setFloatValue:(float)d];
	    }
	} else {
	    if (contents != aString) {
		if (contents && cFlags1.freeText) free(contents);
		contents = NXCopyStringBufferFromZone(aString, [self zone]);
	    } else if (!cFlags1.freeText) {
		contents = NXCopyStringBufferFromZone(aString, [self zone]);
	    }
	    cFlags1.freeText = YES;
	}
    } else {
	if (contents && cFlags1.freeText) {
	    free(contents);
	}
	contents = NULL;
    }
    return self;
}


- setStringValueNoCopy:(const char *)aString
{
    return[self setStringValueNoCopy:(char *)aString shouldFree:NO];
}


- setStringValueNoCopy:(char *)aString shouldFree:(BOOL)flag
{
    if (cFlags1.type != NX_TEXTCELL) {
	cFlags1.type = NX_TEXTCELL;
	[self setFont:[Font newFont:NXSystemFont size:DFLTFONTSIZE]];
    }
    if (contents != aString) {
	if (cFlags1.freeText && contents) {
	    free(contents);
	}
	contents = (char *)aString;
	cFlags1.freeText = (flag ? YES : NO);
    } else if (!flag) {
	cFlags1.freeText = NO;
    }
    return self;
}


- (int)intValue
{
    return (cFlags1.type == NX_TEXTCELL && contents) ? atoi(contents) : 0;
}


- setIntValue:(int)anInt
{
    char                convertbuf[256];

    if (cFlags1.type == NX_TEXTCELL) {
	sprintf(convertbuf, "%d", anInt);
	return[self setStringValue:convertbuf];
    } else {
	return self;
    }
}

static void round(char s[256], char nextDigit)
{
    int pos;

    if (nextDigit >= '5' && nextDigit <= '9') {
	pos = strlen(s) - 1;
	while (pos >= 0 && s[pos] == '9') {
	    s[pos] = '0';
	    pos--;
	    if (s[pos] == '.') pos--;
	}
	if (pos < 0) {
	    return;
	} else if (s[pos] >= '0' && s[pos] <= '9') {
	    (s[pos])++;
	} else if (s[pos] == ' ') {
	    s[pos] = '1';
	}
    }
}

static
void
sdprintf(char textstr[256], double f, int left, int right, BOOL autoranging)
/* FORCES textstr TO BE A FORMATTED FLOAT f FOR PANEL DISPLAYS.
  
   CASES:   
   1) IF autoRange = NO, THE DISPLAY IS CONTROLLED BY 
   	left = number of left-hand decimal places
	right = number of right-hand decimal places
   2) IF autoRange = YES, THE DISPLAY HAS 
   	left+right = total number of digits
      WITH THE DECIMAL POINT RANGING
   3) REGARDLESS OF THE VALUE OF autoRange, SPECIAL EXCEPTIONS ARE
        (left = 0) => exponential notation forced
	(right = 0) => integer notation forced
   4) IN ANY CASE, IF THE DISPLAY OPTION IS INCOMPATIBLE WITH THE
      MAGNITUDE OF f, THE DISPLAY REVERTS TO EXPONENTIAL NOTION OR
      OTHERWISE MOVES THE DECIMAL POINT, ALWAYS ENSURING A CORRECT
      VALUE RESIDING IN THE STRING.
  
   COURTESY RICHARD CRANDALL.
*/
{
    char                str[256], sign, lastDigit = 0;
    int                 n, decplace, spaceplace, j = 1, textcount = 0;

    if (left == 0) {
	sprintf(textstr, "%g", f);
	return;
    }
    if (right == 0) {
	n = f;
	if (n >= 0)
	    sprintf(textstr, " %d", n);
	else
	    sprintf(textstr, "%d", n);
	return;
    }
    if (f < 0) {
	sign = '-';
	f = -f;
    } else
	sign = ' ';
    sprintf(str, "%c%10.14f", sign, f);
    n = 0;
    spaceplace = 0;
    decplace = 0;
    while (++n < strlen(str)) {
	if (str[n] == '.')
	    decplace = n;
	else if (str[n] == ' ')
	    ++spaceplace;
    }

    if ((decplace > left + right) || (decplace == 0)) {
	sprintf(textstr, "%g", f);
	return;
    }
    textstr[0] = sign;
    ++textcount;

    if (autoranging) {
	for (n = 0; (n < strlen(str) - spaceplace) &&
	     (n < left + right + 1); n++) {
	    textstr[n + 1] = str[1 + spaceplace + n];
	    lastDigit = str[2 + spaceplace + n];
	    ++textcount;
	}
	textstr[textcount] = 0;
	round(textstr, lastDigit);
	return;
    }
    n = decplace - left - 1;
    if ((n > 0) && (n <= spaceplace)) {
	for (n = 0; (n < decplace - left) && (n < left + right + 1); n++) {
	    textstr[n + 1] = str[1 + spaceplace + n];
	    lastDigit = str[2 + spaceplace + n];
	    ++textcount;
	}
	textstr[textcount] = 0;
	round(textstr, lastDigit);
	return;
    }
    if (n <= 0) {
	while ((++n) <= 0) {
	    textstr[j] = ' ';
	    --left;
	    ++j;
	    ++textcount;
	}
    }
    n = 0;
    while ((++n) < strlen(str)) {
	textstr[j] = str[n];
	++j;
	++textcount;
	lastDigit = str[n+1];
	if (n > left + right)
	    break;
    }
    textstr[textcount] = 0;
    round(textstr, lastDigit);
}


- (float)floatValue
{
    return (cFlags1.type == NX_TEXTCELL && contents) ? atof(contents) : 0.0;
}


- setFloatValue:(float)aFloat
{
    char               *s;
    char                convertbuf[256];

    bzero(convertbuf, 256);
    if (cFlags1.type == NX_TEXTCELL) {
	sdprintf(convertbuf, (double)aFloat, cFlags2.floatLeft, cFlags2.floatRight, cFlags2.autoRange);
	s = NXCopyStringBufferFromZone(convertbuf, [self zone]);
	return[self setStringValueNoCopy:s shouldFree:YES];
    } else {
	return self;
    }
}


- (double)doubleValue
{
    return (cFlags1.type == NX_TEXTCELL && contents) ? atof(contents) : 0.0;
}


- setDoubleValue:(double)aDouble
{
    char               *s;
    char                convertbuf[256];

    bzero(convertbuf, 256);
    if (cFlags1.type == NX_TEXTCELL) {
	sdprintf(convertbuf, aDouble,
		 cFlags2.floatLeft, cFlags2.floatRight, cFlags2.autoRange);
	s = NXCopyStringBufferFromZone(convertbuf, [self zone]);
	return[self setStringValueNoCopy:s shouldFree:YES];
    } else {
	return self;
    }
}


- takeIntValueFrom:sender
{
    return[self setIntValue:[sender intValue]];
}


- takeFloatValueFrom:sender
{
    return[self setFloatValue:[sender floatValue]];
}


- takeDoubleValueFrom:sender
{
    return[self setDoubleValue:[sender doubleValue]];
}


- takeStringValueFrom:sender
{
    return[self setStringValue:[sender stringValue]];
}


/*---------------------------*/
/* IconCells related methods */
/*---------------------------*/

/*
 * Return a Bitmap or an NXImage matching name.
 */
id _NXFindIconNamed(const char *name)
{
    id icon;
    if ((icon = [NXImage _imageNamed:name]) ||
	(icon = [Bitmap _findExistingBitmap:name]) ||
	(icon = [NXImage findImageNamed:name]) ||
	(icon = [Bitmap findBitmapFor:name])) {
	return icon; 
    }
    return nil;
}

BOOL _NXIconHasAlpha(id icon)
{
    if ([icon isKindOf:[NXImage class]]) {
	return [icon _asIconHasAlpha];
    } else {
	return ([icon type] != NX_NOALPHABITMAP) ? YES : NO;
    }
}

void _NXDrawIcon(id icon, NXRect *toRect, id view)
{
    NXPoint toPoint = toRect->origin;
    NXSize iconSize;

    [icon getSize:&iconSize];
    if (iconSize.width > NX_WIDTH(toRect) ||
	iconSize.height > NX_HEIGHT(toRect)) {
	NXRect srcRect = {{0.0, 0.0},
			  {MIN(NX_WIDTH(toRect), iconSize.width), 
			   MIN(NX_HEIGHT(toRect), iconSize.height)}};
	if ([view isFlipped] && [icon isKindOf:[NXImage class]]) {
	    NXSize tmpSize = {0.0, NX_HEIGHT(&srcRect)};
	    [view convertSize:&tmpSize fromView:nil];
	    toPoint.y += ABS(tmpSize.height);
	    NX_Y(&srcRect) = iconSize.height - NX_HEIGHT(&srcRect);
	}
	[icon composite:NX_SOVER fromRect:&srcRect toPoint:&toPoint];
    } else {
	if ([view isFlipped] && [icon isKindOf:[NXImage class]]) {
	    toPoint.y += NX_HEIGHT(toRect);
	}
	[icon composite:NX_SOVER toPoint:&toPoint];
    }
}

- (const char *)icon
{
    return (cFlags1.type == NX_ICONCELL) ? contents : NULL;
}

- setIcon:(const char *)iconName
{
    cFlags1.type = NX_ICONCELL;
    if (cFlags1.freeText && contents) {
	free(contents);
    }
    if (!iconName || !strlen(iconName)) {
	iconName = defaultIcon;
    }
    support = _NXFindIconNamed(iconName);
    contents = (char *)[support name];
    cFlags1.freeText = NO;

    return self;
}


/*-----------------*/
/* Generic methods */
/*-----------------*/


/* 
 * Internal flag manipulations are made by the following functions using
 * constants defined in Cell.h 
 */


int
_NXGetCellParam(Cell *self, int n)
{
    switch (n) {
    case NX_CELLDISABLED:
	return (self->cFlags1.disabled);
    case NX_CELLSTATE:
	return (self->cFlags1.state);
    case NX_CELLHIGHLIGHTED:
	return (self->cFlags1.highlighted);
    case NX_CELLEDITABLE:
	return (self->cFlags1.editable);
    }
    return 0;
}


void
_NXSetCellParam(Cell *self, int n, int val)
{
    val = val ? 1 : 0;
    switch (n) {
	case NX_CELLDISABLED:
	self->cFlags1.disabled = val;
	break;
    case NX_CELLSTATE:
	self->cFlags1.state = val;
	break;
    case NX_CELLHIGHLIGHTED:
	self->cFlags1.highlighted = val;
	break;
    case NX_CELLEDITABLE:
	self->cFlags1.editable = val;
	break;
    }
}


- (int)getParameter:(int)aParameter
{
    return _NXGetCellParam(self, aParameter);
}


- setParameter:(int)aParameter to:(int)value
{
    _NXSetCellParam(self, aParameter, value);
    return self;
}


static
void
getIconRect(NXSize *iconSize, NXRect *theRect, float offset)
{
    if (offset) {
	NXInsetRect(theRect, offset, offset);
    }
    if (iconSize->height < theRect->size.height) {
	theRect->origin.y +=
	  floor((theRect->size.height - iconSize->height) / 2.0);
	theRect->size.height = iconSize->height;
    }
    if (iconSize->width < theRect->size.width) {
	theRect->origin.x +=
	  floor((theRect->size.width - iconSize->width) / 2.0);
	theRect->size.width = iconSize->width;
    }
}


- getIconRect:(NXRect *)theRect
{
    NXSize              iconSize;

    if (cFlags1.type == NX_ICONCELL && support) {
	[support getSize:&iconSize];
	getIconRect(&iconSize, theRect, CELLOFFSET);
    }
    return self;
}


- getTitleRect:(NXRect *)theRect
{
    float               offset = CELLOFFSET;

    if (cFlags1.type == NX_TEXTCELL) {
	NXInsetRect(theRect, offset, offset);
    }
    return self;
}


- getDrawRect:(NXRect *)theRect
{
    float               offset = CELLOFFSET;

    NXInsetRect(theRect, offset, offset);

    return self;
}


- calcCellSize:(NXSize *)theSize
{
    return[self calcCellSize:theSize inRect:(NXRect *)(&bigRect)];
}


void
_NXCalcTextSizeInRect(const char *string, id font, NXSize *size,
		      const NXRect *rect, int alignment, BOOL noWrap)
{
    float               FIdes, FIasc;
    id                  drawText;
    textId              drawTextId;

    if (alignment == NX_LEFTALIGNED && noWrap) {
	drawText = noWrapText;
    } else {
	drawText = wrapText;
    }
    drawTextId = (textId) drawText;
    [drawText setAlignment:alignment];
    drawTextId->window = nil;
    string = string ? string : "";
    drawTextId->vFlags.disableAutodisplay = YES;
    [drawText renewFont:font text:string frame:rect tag:0];
    drawTextId->vFlags.disableAutodisplay = NO;
    [drawText getMinWidth:&size->width minHeight:&size->height
     maxWidth:rect->size.width maxHeight:10000.0];
    if (!*string) {
	NXTextFontInfo(font, &FIasc, &FIdes, &size->height);
    }
    size->width = ceil(size->width);
    size->height = ceil(size->height);
}


- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect
{
    NXRect              _inBounds;
    register NXRect    *inBounds = &_inBounds;
    NXCoord             offset = CELLOFFSET;

    switch (cFlags1.type) {
    case NX_ICONCELL:
	if (support) {
	    [support getSize:theSize];
	} else {
	    theSize->width = 0.0;
	    theSize->height = 0.0;
	}
	break;
    case NX_TEXTCELL:
	*inBounds = *aRect;
	NXInsetRect(inBounds, offset, offset);
	_NXCalcTextSizeInRect(contents, support, theSize, inBounds,
			      cFlags1.alignment, cFlags2.noWrap);
	break;
    case NX_NULLCELL:
	theSize->width = aRect->size.width;
	theSize->height = aRect->size.height;
	break;
    }

    if (offset) {
	theSize->width += 2.0 * offset;
	theSize->height += 2.0 * offset;
    }
    return self;
}


- calcDrawInfo:(const NXRect *)aRect
{
    return self;
}


- setTextAttributes:textObj
{
    [textObj setTextGray:[self isEnabled] ? NX_BLACK : NX_DKGRAY];
    [textObj setBackgroundGray:cFlags1.bezeled ? NX_WHITE : NX_LTGRAY];
    return textObj;
}


static void
fixSelection(textId text)
{
    if ((text->sp0.cp < 0) || (text->sp0.cp == text->spN.cp))
	return;
    [((id)text) selectNull];
}


void
_NXDrawTextCell(Cell *cell, id view, const NXRect *rect, BOOL clip)
{
    id                  drawText, returnText;
    textId              drawTextId;
    NXRect		tframe;
    BOOL		needsClip;

    /* be sure we have a cell, a view, we can draw, etc. */

    if (!cell || !view || ![view canDraw] ||
	!cell->support || cell->cFlags1.type != NX_TEXTCELL) {
	return;
    }

    /* set attributes of the Text object to draw with */

    if (cell->cFlags1.alignment == NX_LEFTALIGNED && cell->cFlags2.noWrap) {
	drawText = noWrapText;
    } else {
	drawText = wrapText;
    }
    returnText = [cell setTextAttributes:drawText];
    if (returnText != cell && returnText)
	drawText = returnText;
    drawTextId = (textId) drawText;
    [drawText setAlignment:cell->cFlags1.alignment];

    /* insert the Text object into the view heirarchy */

    if (((viewId) view)->subviews) {
	[((viewId) view)->subviews addObject:drawText];
    } else {
	[drawList replaceObjectAt:0 with:drawText];
	((viewId) view)->subviews = drawList;
    }
    drawTextId->superview = view;
    drawTextId->window = ((viewId) view)->window;
    [drawText _alreadyFlipped:((viewId) view)->vFlags.needsFlipped];
    fixSelection(drawTextId);

    /* set the text in the Text object (and don't draw yet) */

    drawTextId->vFlags.disableAutodisplay = YES;
    drawTextId->vFlags.noClip = YES;

    if (cell->cFlags1.scrollable) {
	tframe.size = bigRect.size;
	tframe.origin = rect->origin;
	[drawText renewFont:cell->support
	 text:(cell->contents ? cell->contents : "")
	 frame:&tframe tag:0];
	[drawText sizeToFit];
	[drawText getFrame:&tframe];
	needsClip = !NXContainsRect(rect, &tframe);
    } else {
	[drawText renewFont:cell->support
	 text:(cell->contents ? cell->contents : "")
	 frame:rect tag:0];
	needsClip = NO;
    }

    /* draw the Text object */

    if (needsClip || ![view isFlipped]) {
	[drawText lockFocus];
	if (needsClip) {
	    NXRectClip(rect);
	}
    }
    drawTextId->vFlags.noClip = clip ? YES : NO;
    [drawText drawSelf:rect :1];
    if (needsClip || ![view isFlipped]) {
	[drawText unlockFocus];
    }

    /* remove the Text object from the view heirarchy */

    if (((viewId) view)->subviews == drawList) {
	((viewId) view)->subviews = nil;
    } else {
	[((viewId) view)->subviews removeLastObject];
    }
    drawTextId->window = nil;
    drawTextId->superview = nil;
}


- drawInside:(const NXRect *)cellFrame inView:controlView
{
    NXRect              _inBounds = *cellFrame;
    register NXRect    *inBounds = &_inBounds;

    NXRect              _srcRect;
    register NXRect    *srcRect = &_srcRect;
    float               offset;

    switch (cFlags1.type) {
    case NX_ICONCELL:
	if (support) {
	    [support getSize:&srcRect->size];
	    getIconRect(&srcRect->size, inBounds, CELLOFFSET);
	    _NXDrawIcon(support, inBounds, controlView);
	}
	break;
    case NX_TEXTCELL:
	offset = CELLOFFSET;
	NXInsetRect(inBounds, offset, offset);
	if (cFlags1.bezeled) {
	    PSsetgray(NX_WHITE);
	    NXRectFill(inBounds);
	}
	_NXDrawTextCell(self, controlView, inBounds, YES);
	break;
    case NX_NULLCELL:
	break;
    }

    if (cFlags1.highlighted && NXDrawingStatus == NX_DRAWING) {
	NXHighlightRect(inBounds);
    }
    return self;
}


- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    if (cFlags1.bordered) {
	PSsetgray(0.0);
	NXFrameRect(cellFrame);
    } else if (cFlags1.bezeled) {
	_NXDrawWhiteBezel(cellFrame, (NXRect *)0, controlView);
    }
    return[self drawInside:cellFrame inView:controlView];
}


- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag
{
    if ((!cFlags1.disabled || !flag) &&
	((cFlags1.highlighted && !flag) || (!cFlags1.highlighted && flag))) {
	cFlags1.highlighted = flag ? YES : NO;
	if (NXDrawingStatus == NX_DRAWING) {
	    NXHighlightRect(cellFrame);
	}
    }
    return self;
}


- _setMouseDownFlags:(int)flags
{
    mdFlags = flags;
    return self;
}

- (int)mouseDownFlags
{
    return mdFlags;
}


- getPeriodicDelay:(float *)delay andInterval:(float *)interval
{
    *delay = 0.2;
    *interval = 0.025;
    return self;
}


- (BOOL)startTrackingAt:(const NXPoint *)startPoint inView:controlView
{
    return (BOOL)(cFlags2.continuous || cFlags2.actOnMouseDragged);
}


- (BOOL)continueTracking:(const NXPoint *)lastPoint
    at:(const NXPoint *)currentPoint
    inView:controlView
{
    return (BOOL)(cFlags2.continuous || cFlags2.actOnMouseDragged);
}


- stopTracking:(const NXPoint *)lastPoint
    at:(const NXPoint *)stopPoint
    inView:controlView
    mouseIsUp:(BOOL)flag
{
    return self;
}


#define ACTIVECELLMASK (NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK|NX_TIMERMASK)
#define pointsAreDifferent(p1, p2) ((p1.x != p2.x) || (p1.y != p2.y))

- (BOOL)trackMouse:(NXEvent *)theEvent
    inRect:(const NXRect *)cellFrame
    ofView:controlView
{
    NXPoint             lastPoint, curPoint;
    int                 oldMask, eventNum = 0, isStillDown;
    NXTrackingTimer    *timer = NULL;
    BOOL                okToTrack, retval = NO;
    float               delay, interval;

    lastPoint = theEvent->location;
    [controlView convertPoint:&lastPoint fromView:nil];

    if (!controlView ||
	(theEvent->type != NX_MOUSEUP && cellFrame &&
	 !NXMouseInRect(&lastPoint, cellFrame, [controlView isFlipped]))) {
	return NO;
    }
    mdFlags = (theEvent->type == NX_MOUSEDOWN) ? theEvent->flags : 0;

    oldMask = [((viewId) controlView)->window addToEventMask:ACTIVECELLMASK];

    okToTrack = (cFlags1.disabled) ? NO :
      [self startTrackingAt:&lastPoint inView:controlView];

    if (cFlags2.continuous && okToTrack) {
	[self getPeriodicDelay:&delay andInterval:&interval];
	timer = NXBeginTimer(NULL, delay, interval);
    }
    if (cFlags2.actOnMouseDown && !cFlags1.disabled &&
	theEvent->type == NX_MOUSEDOWN) {
	eventNum = theEvent->data.mouse.eventNum;
	[self incrementState];
	[controlView sendAction:[self action] to:[self target]];
	PSstilldown(eventNum, &isStillDown);
	if (isStillDown) {
	    theEvent = [NXApp currentEvent];
	} else {
	    retval = YES;
	}
    }
    if (!retval) for (;;) {
	if (theEvent->type == NX_TIMER) {
	    if (cFlags2.continuous) {
		[controlView sendAction:[self action] to:[self target]];
	    }
	} else {
	    curPoint = theEvent->location;
	    [controlView convertPoint:&curPoint fromView:nil];
	    if (theEvent->type == NX_MOUSEUP) {
		if (okToTrack) {
		    [self stopTracking:&lastPoint at:&curPoint
		     inView:controlView mouseIsUp:YES];
		}
		if (!cFlags2.dontActOnMouseUp && !cFlags1.disabled) {
		    if (!cFlags2.actOnMouseDown) {
			[self incrementState];
		    }
		    [controlView sendAction:[self action] to:[self target]];
		}
		retval = YES;
		break;
	    } else if (!cellFrame || 
	      NXMouseInRect(&curPoint, cellFrame, [controlView isFlipped])) {
		if (okToTrack && pointsAreDifferent(curPoint, lastPoint)) {
		    okToTrack = [self continueTracking:&lastPoint
				 at:&curPoint inView:controlView];
		    lastPoint = curPoint;
		}
		if (cFlags2.actOnMouseDragged &&
		    theEvent->type == NX_MOUSEDRAGGED) {
		    [controlView sendAction:[self action] to:[self target]];
		}
	    } else {
		if (okToTrack) {
		    [self stopTracking:&lastPoint at:&curPoint
		     inView:controlView mouseIsUp:NO];
		}
		retval = NO;
		break;
	    }
	}
	NXPing();
	theEvent = [NXApp getNextEvent:ACTIVECELLMASK];
    }

    if (timer)
	NXEndTimer(timer);
    [((viewId) controlView)->window setEventMask:oldMask];
    mdFlags = retval ? 0 : mdFlags;

    return retval;
}


void
_NXEditTextCell(Cell *cell, id view, NXRect *rect, id text, id delegate,
		NXEvent *theEvent, int start, int end)
/*
 * This method optimizes by using (textId) casts of text and view.
 * Therefore, it must be kept in sync with changes in the Text object!
 */
{
    NXPoint             localPoint;
    id                  clipView;

    if (!cell || !view || !text || !cell->support) {
	return;
    } else if (theEvent) {
	localPoint = theEvent->location;
	[view convertPoint:&localPoint fromView:nil];
	if (!NXMouseInRect(&localPoint, rect, [view isFlipped])) {
	    if (localPoint.x < rect->origin.x) {
		localPoint.x = rect->origin.x + 0.5;
	    } else if (localPoint.x > rect->origin.x + rect->size.width) {
		localPoint.x = rect->origin.x + rect->size.width - 0.5;
	    }
	    if (localPoint.y < rect->origin.y) {
		localPoint.y = rect->origin.y + 0.5;
	    } else if (localPoint.y > rect->origin.y + rect->size.height) {
		localPoint.y = rect->origin.y + rect->size.height - 0.5;
	    }
	    if (!NXMouseInRect(&localPoint, rect, [view isFlipped]))
		return;
	    [view convertPoint:&localPoint toView:nil];
	    theEvent->location = localPoint;
	}
    }

    [text setAlignment:cell->cFlags1.alignment];
    [cell setTextAttributes:text];
    [text setEditable:cell->cFlags1.editable];
    [text setSelectable:(cell->cFlags1.selectable || cell->cFlags1.editable)];
    [text setMinSize:&rect->size];
    [text setMaxSize:&bigRect.size];
    [text setHorizResizable:cell->cFlags1.scrollable ? YES : NO];
    [text setVertResizable:cell->cFlags1.scrollable ? YES : NO];

    fixSelection((textId) text);
    ((textId) text)->vFlags.disableAutodisplay = YES;
    [text renewFont:cell->support
     text:(cell->contents ? cell->contents : "")
     frame:(cell->cFlags1.scrollable ? &bigRect : rect) tag:0];
    ((textId) text)->vFlags.disableAutodisplay = NO;

    if (cell->cFlags1.scrollable) {
	[text sizeToFit];
	if (![clipViewList count]) {
	    clipView = [[[ClipView allocFromZone:[cell zone]] init] _markUsedByCell];
	} else {
	    clipView = [clipViewList removeObjectAt:0];
	}
	[clipView setDocView:text];
	[clipView setCopyOnScroll:NO];
	[clipView setDisplayOnScroll:YES];
	[view addSubview:clipView];
	[clipView setBackgroundGray:((textId) text)->backgroundGray];
	[((viewId) view)->window disableDisplay];
	[clipView setFrame:rect];
	[((viewId) view)->window reenableDisplay];
    } else {
	[view addSubview:text];
    }

    [((viewId) view)->window makeFirstResponder:text];
    [text setDelegate:delegate];
    [text scrollRectToVisible:rect];

    if (theEvent) {
	[text mouseDown:theEvent];
    } else {
	[text setSel:start :end];
    }
}


- _selectOrEdit:(const NXRect *)aRect
    inView:controlView
    target:anObject
    editor:textObj
    event:(NXEvent *)theEvent
    start:(int)selStart
    end:(int)selEnd
{
    NXRect              _inBounds;
    register NXRect    *inBounds = &_inBounds;
    register float      offset = CELLOFFSET;

    if (cFlags1.type == NX_TEXTCELL) {
	inBounds->size.width = aRect->size.width - offset * 2.0;
	inBounds->size.height = aRect->size.height - offset * 2.0;
	inBounds->origin.x = aRect->origin.x + offset;
	inBounds->origin.y = aRect->origin.y + offset;
	_NXEditTextCell(self, controlView, inBounds,
			textObj, anObject, theEvent, selStart, selEnd);
    }
    return self;
}


- edit:(const NXRect *)aRect
    inView:controlView
    editor:textObj
    delegate:anObject
    event:(NXEvent *)theEvent
{
    return[self _selectOrEdit:aRect inView:controlView
	   target:anObject editor:textObj
	   event:theEvent start:0 end:0];
}


- select:(const NXRect *)aRect
    inView:controlView
    editor:textObj
    delegate:anObject
    start:(int)selStart
    length:(int)selLength
{
    return[self _selectOrEdit:aRect inView:controlView
	   target:anObject editor:textObj
	   event:(NXEvent *)0
	   start:selStart end:(selStart + selLength)];
}


- endEditing:textObj
{
    id                  clipView = nil;

    if (textObj && ((textId) textObj)->superview) {
	clipView = [textObj superview];
	if ([clipView isKindOf:[ClipView class]] && [clipView _isUsedByCell]) {
	    [clipView removeFromSuperview];
	    [clipView setDocView:nil];
	    if ([clipViewList count] < MAXCLIPVIEWS) {
		[clipViewList addObject:clipView];
	    } else {
		[clipView free];
	    }
	}
    }
    [textObj removeFromSuperview];
    [textObj setDelegate:nil];
    [self setAlignment:[textObj alignment]];
    return self;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "*@ss", &contents, &support, &cFlags1, &cFlags2);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadTypes(stream, "*@ss", &contents, &support, &cFlags1, &cFlags2);
    cFlags1.freeText = YES;	/* we are the only holder of this string */
    return self;
}


- resetCursorRect:(const NXRect *)cellFrame inView:controlView
{
    float               offset;
    NXRect              inBounds = *cellFrame;

    if (cFlags1.disabled || ![self isSelectable]) {
	return self;
    }
    switch (cFlags1.type) {
    case NX_ICONCELL:
	break;
    case NX_TEXTCELL:
	offset = CELLOFFSET;
	NXInsetRect(&inBounds, offset, offset);
	[controlView addCursorRect:&inBounds cursor:NXIBeam];
	break;
    case NX_NULLCELL:
	break;
    }
    return self;
}


@end

/*
  
Modifications (starting at 0.8):
  
11/21/88 pah    fixed _NX[GS]etCellParam (CELLEDITABLE was broken)
		fixed setStringValue: (used to malfunction if the
		 string was the same as the current contents)
		made _initClassVars a C proc
		made _convertToText a C proc (eliminate _convertToIcon)
		got rid of cFlags2.dontFree
		got rid of setFlipped: isInFlippedView cFlags1.flipped
		got rid of cFlags2.shared and cFlags1._offset
		added support for character wrapping (cFlags2.noWrap)
		added support for SelectionCell (cFlags1._special)
11/22/88 pah	blasted setActionSentOnMouseDown: in favor of
		 more general sendActionOn: (including the two
		 cFlags2, dontActOnMouseUp and actOnMouseDragged) and
		 added support for it in trackMouse:inRect:ofView:...
		made setType: change the contents and support instance
		 variables in a way appropriate to the type
		fixed a whole mess of comments
11/23/88 pah	made validInt and _NXExtractDouble more robust
		made the workString for setFloatValue: et. al. static
		added static function getIconRect (shared by
		 getIconRect: and drawInside:inView:
		changed _NX{Draw,Edit}TextInViewRect to
		 _NX{Draw,Edit}TextCell and added setTextAttributes:
		eliminated _NXGetDrawText, _getInsideBounds:, and
		 _isInsideOpaque
12/2/88	 pah	support scrolling in editable text fields
		 (added isScrollable and setScrollable:)
12/11/88 pah	added readSelf:archiver: and writeSelf:archiver to support
		 old archive formats
		added mouseDownFlags to get the state of the flags when the
		 mouse goes down to start tracking
12/13/88 bgy	converted to the List object
  
0.82
----
  1/6/89 pah	added setWrap: to set word wrapping
  1/8/89 pah	add isSelectable/setSelectable:
		NXPing() in trackMouse:
		don't unlock focus around sendAction in trackMouse:
 1/11/89 pah	setTextAttributes: now returns the text object to draw with
 1/25/89 trey	made setString* methods take const params
 1/26/89 bgy	fixed the selection bogosity that occured when the renew
		 Font:... method was called.
 1/27/89 bs	added read: write:.
 2/01/89 trey	removed hack of moving field editor to (10K,10K) in endEditing:
 2/05/89 bgy	added following cursor tracking method:
		    - resetCursorRect:inView:
		 This method sets up an ibeam cursor if the cell is a text
		 cell.
 2/06/89 pah	don't let Cell class allocate an infinite number of ScrollViews
 2/09/89 pah	Matrix needs _setMouseDownFlags:
 2/14/89 pah	char * -> const char *
 2/15/89 pah	fix _NXExtractDouble to recognize leading plus signs
  
0.83
----
 3/19/89 pah	Make no wrap case actually do character wrapping

0.91
----
 5/19/89 trey	minimized static data
 5/19/89 pah	only lockFocus on the Text object if controlView isFlipped
 5/21/89 pah	fix scrolling field brain damage
 5/27/89 pah	move cursor rect stuff from Control into Cell

0.93
----
 6/15/89 pah	make stringValue and icon const char *
 6/19/89 pah	fix bezel trashing by setting minSize of the Text object
 6/19/89 wrp	changed ScrollViews to ClipViews

0.96
----
 7/20/89 pah	isSelectable returns YES if selectable OR editable
		fix round-off error in sdprintf
		check if the target's action message getNextEvent:'s a
		 mouse up by using PSstilldown instead of checking
		 currentEvent

12/20/89 trey	nuked unused character classes table

79
--
  4/3/90 pah	made default setTextAttributes: use isEnabled instead of
		 looking at the flag directly (makes Cell more subclassable)
		 this should probably be done in a lot more places!

83
--
 4/25/90 aozer	Added the functions _NXDrawIcon(), _NXFindIconNamed(), and
		_NXIconHasAlpha() to make life easier now that icons can be
		either instances of Bitmap or instances of NXImage.
		These functions are used by ButtonCell, Cell, Window, etc.
		When a setIcon: is done, we first look for named, existing
		instances of NXImage, then for named, existing instances
		of Bitmap, and then we do a findImageNamed: on NXImage and
		finally a findBitmapFor: on Bitmap, in that order.
 5/3/90 aozer	Renamed default icon from square16 to NXsquare16.

86
--
 6/12/90 pah	Added useUseKeyEquivalent flag to _cFlags3

92
--
 8/22/90 trey	freeText set whenever we unarchive
 8/20/90 aozer	setScrollable:YES set wrap:NO & vice versa (bug 4561)

94
--
 9/25/90 greg	Retrieve alignment state when done editing.
 9/25/90 aozer	Added scrollRectToVisible: in _NXEditTextCell() to fix 4440.
		I sure hope this bug fix won't break anything.

95
--
 10/3/90 aozer	Added prefersTrackingUntilMouseUp to allow Cell classes
		to let their views know whether they like to hold on to the 
		tracking until the user lets up on the mouse. NO by default.

100
---
 10/23/90 aozer	In _NXEditTextCell(), changed calls to setXXXResizable to
 		use cell->cFlags1.scrollable rather than YES before calling
		renewRuns:  
 
*/

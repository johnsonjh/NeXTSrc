/*
	ButtonCell.m
	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "ButtonCell_Private.h"
#import "Cell_Private.h"
#import "Application.h"
#import "Font.h"
#import "Bitmap.h"
#import "Control.h"
#import "NXImage.h"
#import "Text.h"
#import <objc/typedstream.h>
#import "nextstd.h"
#import <defaults.h>
#import <dpsclient/wraps.h>
#import <math.h>
#import <zone.h>

#define BXOFFSET	1.0
#define BYOFFSET	2.0
#define BWBORDERDELTA	3.0
#define BHBORDERDELTA	3.0

#define ICONINSET	(BF.bordered ? 2.0 : 0.0)
#define ICONINSETm2	(BF.bordered ? 4.0 : 0.0)
#define ICONINSETm3	(BF.bordered ? 6.0 : 0.0)

#define BF		bcFlags1
#define KEYEQ		bcFlags2.keyEquivalent

#define NORMAL_BKGD NX_LTGRAY
#define ALT_BKGD    NX_WHITE

#define HAS_ICON	(bcFlags1.iconAndText || bcFlags1.iconOverlaps)
#define HAS_TEXT	(bcFlags1.iconAndText || !bcFlags1.iconOverlaps)
#define ICON_ONLY	(bcFlags1.iconOverlaps && !bcFlags1.iconAndText && \
			 !bcFlags1.iconIsKeyEquivalent && icon.bmap.normal)
#define ICON_AND_TEXT	(bcFlags1.iconAndText && icon.bmap.normal)

#define CHANGEBKGD	(bcFlags1.changeBackground && \
			 (bcFlags1.hasAlpha || !bcFlags1.changeGray))
#define CHANGEGRAY	(bcFlags1.changeGray && \
			 (!bcFlags1.hasAlpha || !bcFlags1.changeBackground))
#define LIGHTBYBKGD	(bcFlags1.lightByBackground && \
			 (bcFlags1.hasAlpha || !bcFlags1.lightByGray))
#define LIGHTBYGRAY	(bcFlags1.lightByGray && \
			 (!bcFlags1.hasAlpha || !bcFlags1.lightByBackground))

static unsigned short initialButtonDelay = 0, initialButtonInterval = 0;

@implementation ButtonCell:ActionCell

+ initialize
{
    if (initialButtonDelay == 0) {
	const char *defStr = NULL;
	if (defStr = NXGetDefaultValue(NXSystemDomainName, "ButtonDelay")) {
	    float val = atof(defStr);
	    initialButtonDelay = (unsigned short)((MIN(MAX(val, 0.0), 60.0)) * 1000);
	} else {
	    initialButtonDelay = 400;
	}
	if (defStr = NXGetDefaultValue(NXSystemDomainName, "ButtonPeriod")) {
	    float val = atof(defStr);
	    initialButtonInterval = (unsigned short)((MIN(MAX(val, 0.0), 60.0)) * 1000);
	} else {
	    initialButtonInterval = 75;
	}
    }
    return self;
}

static float getKEDescent(font)
    id                  font;
{
    NXFontMetrics      *fontMetrics;
    float               pointSize;

    pointSize = [font pointSize];
    fontMetrics = [font readMetrics:NX_FONTMETRICS];
    return (-(fontMetrics->isFixedPitch) ?
	    ceil(fontMetrics->descender * pointSize) : 0.0);
}


+ new
{
    return [self newTextCell];
}

- init
{
    return [[self initTextCell:NULL] setTitleNoCopy:"Button"];
}

+ newTextCell
{
    return [[self newTextCell:NULL] setTitleNoCopy:"Button"];
}

+ newTextCell:(const char *)aString
{
    return [[self allocFromZone:NXDefaultMallocZone()] initTextCell:aString];
}

- initTextCell:(const char *)aString
{
    [super init];
    [self _convertToText:aString];
    BF.iconIsKeyEquivalent = YES;
    BF.bordered = YES;
    BF.pushIn = YES;
    BF.lightByGray = YES;
    BF.lightByBackground = YES;
    cFlags1.alignment = NX_CENTERED;
    icon.ke.font = support;
    icon.ke.descent = getKEDescent(icon.ke.font);
    periodicDelay = initialButtonDelay;
    periodicInterval = initialButtonInterval;
    return self;
}

+ newIconCell
{
    return [self newIconCell:"NXsquare16"];
}

+ newIconCell:(const char *)iconName
{
    return [[self allocFromZone:NXDefaultMallocZone()] initIconCell:iconName];
}

- initIconCell:(const char *)iconName
{
    [super initIconCell:iconName];
    icon.bmap.alternate = nil;
    BF.iconIsKeyEquivalent = NO;
    BF.bordered = YES;
    BF.pushIn = YES;
    BF.lightByGray = YES;
    BF.lightByBackground = YES;
    periodicDelay = initialButtonDelay;
    periodicInterval = initialButtonInterval;
    return self;
}


- _convertToText:(const char *)aString
{
    id                  fontObj;

    altContents = (char *)0;
    fontObj = [Font newFont:NXSystemFont size:12.0];
    if (cFlags1.type != NX_TEXTCELL) {
	cFlags1.type = NX_TEXTCELL;
	[self setFont:fontObj];
	cFlags1.alignment = NX_CENTERED;
	[self setTitle:aString];
    }
    return self;
}

- _convertToText
{
    return [self _convertToText:"Button"];
}

- _convertToIcon
{
    if (!icon.bmap.normal) {
	icon.bmap.alternate = nil;
	if (cFlags1.type != NX_ICONCELL) {
	    cFlags1.type = NX_ICONCELL;
	    [self setIcon:"NXsquare16"];
	}
    } else {
	cFlags1.type = NX_ICONCELL;
    }

    return self;
}


- copyFromZone:(NXZone *)zone
{
    ButtonCell	       *retval;

    retval = [super copyFromZone:zone];
    if (altContents) retval->altContents = 
	(char *)NXCopyStringBufferFromZone(altContents,  zone);
    if (!BF.iconIsKeyEquivalent) {
	retval->icon.bmap.normal = icon.bmap.normal;
	retval->icon.bmap.alternate = icon.bmap.alternate;
    } else {
	retval->icon.ke.font = icon.ke.font;
	retval->icon.ke.descent = icon.ke.descent;
    }

    return retval;
}


- free
{
    if (altContents)
	free(altContents);
    return[super free];
}


- (const char *)title
{
    if (HAS_TEXT) {
	return contents;
    } else {
	return (char *)0;
    }
}


- setTitleNoCopy:(const char *)aString
{
    if (BF.iconOverlaps) BF.iconAndText = YES;
    [super setStringValueNoCopy:aString];
    return self;
}

- setTitle:(const char *)aString
{
    if (BF.iconOverlaps) BF.iconAndText = YES;
    [super setStringValue:aString];
    return self;
}


- (const char *)altTitle
{
    if (HAS_TEXT) {
	return altContents;
    } else {
	return (char *)0;
    }
}


- setAltTitle:(const char *)aString
{
    if (altContents) {
	free(altContents);
	altContents = NULL;
    }
    if (cFlags1.type != NX_TEXTCELL) {
	[self _convertToText];
    }
    if (!aString || !strlen(aString)) {
	aString = ((char *)0);
    }
    if (aString) {
	altContents = NXCopyStringBufferFromZone(aString, [self zone]);
    }
    if (BF.iconOverlaps) {
	BF.iconAndText = YES;
    }
    return self;
}


static BOOL
getIconAlpha(normal, alternate)
    id                  normal, alternate;
{
    BOOL                hasAlpha = NO;

    if (normal != nil) {
	hasAlpha = _NXIconHasAlpha(normal);
    } else if (alternate != nil) {
	hasAlpha = _NXIconHasAlpha(alternate);
    }
    return hasAlpha;
}


- _getIconInfo:(NXSize *)iconSize
{
    NXSize              _altSize;
    register NXSize    *altSize = &_altSize;
    NXFontMetrics      *fontMetrics;
    float               pointSize;

    if (BF.iconIsKeyEquivalent) {
	pointSize = [icon.ke.font pointSize];
	fontMetrics = [icon.ke.font readMetrics:NX_FONTMETRICS];
	iconSize->width =
	  ceil((fontMetrics->fontBBox[2] -
		fontMetrics->fontBBox[0]) * pointSize);
	iconSize->height =
	  ceil((fontMetrics->fontBBox[3] -
		fontMetrics->fontBBox[1]) * pointSize);
	BF.iconSizeDiff = NO;
	BF.hasAlpha = NO;
	return self;
    }
    if (icon.bmap.normal) {
	[icon.bmap.normal getSize:iconSize];
	if (icon.bmap.alternate) {
	    [icon.bmap.alternate getSize:altSize];
	    if (altSize->width > iconSize->width) {
		iconSize->width = altSize->width;
		BF.iconSizeDiff = YES;
	    } else {
		BF.iconSizeDiff = (altSize->width < iconSize->width);
	    }
	    if (altSize->height > iconSize->height) {
		iconSize->height = altSize->height;
		BF.iconSizeDiff = YES;
	    } else {
		BF.iconSizeDiff = (altSize->height < iconSize->height);
	    }
	} else {
	    BF.iconSizeDiff = 0;
	}
    }
    BF.hasAlpha = getIconAlpha(icon.bmap.normal, icon.bmap.alternate);
    return self;
}

- setImage:image
{
    if (image) {
	icon.bmap.normal = image;
	if (BF.iconIsKeyEquivalent) {
	    icon.bmap.alternate = nil;
	    BF.iconIsKeyEquivalent = NO;
	}
	BF.hasAlpha = getIconAlpha(icon.bmap.normal, icon.bmap.alternate);
	if (!BF.iconAndText) {
	    BF.iconOverlaps = YES;
	}
    } else {
	if (ICON_ONLY) {
	    [self setTitle:""];
	}
	if (!BF.iconIsKeyEquivalent) {
	    BF.iconIsKeyEquivalent = YES;
	    if (support && [support isKindOf:[Font class]]) {
		icon.ke.font = support;
	    } else {
		icon.ke.font = [Font newFont:NXSystemFont size:12.0];
	    }
	    icon.ke.descent = getKEDescent(icon.ke.font);
	}
    }
    [[self controlView] updateCellInside:self];
    return self;
}

- setIcon:(const char *)iconName
{
    return [self setImage:((iconName && strlen(iconName)) ? 
					_NXFindIconNamed(iconName) : nil)];
}

- setAltImage:image
{
    if (image) {
	icon.bmap.alternate = image;
	if (BF.iconIsKeyEquivalent) {
	    icon.bmap.normal = icon.bmap.alternate;
	    BF.iconIsKeyEquivalent = NO;
	}
	BF.hasAlpha = getIconAlpha(icon.bmap.normal, icon.bmap.alternate);
	if (!BF.iconAndText) {
	    BF.iconOverlaps = YES;
	}
    } else if (!icon.bmap.normal) {
	if (ICON_ONLY) {
	    [self setTitle:""];
	}
	if (!BF.iconIsKeyEquivalent) {
	    BF.iconIsKeyEquivalent = YES;
	    if (support && [support isKindOf:[Font class]]) {
		icon.ke.font = support;
	    } else {
		icon.ke.font = [Font newFont:NXSystemFont size:12.0];
	    }
	    icon.ke.descent = getKEDescent(icon.ke.font);
	}
    }
    return self;
}

- setAltIcon:(const char *)iconName
{
    return [self setAltImage:((iconName && strlen(iconName)) ? 
					_NXFindIconNamed(iconName) : nil)];
}

- image
{
    return (HAS_ICON && !BF.iconIsKeyEquivalent) ? icon.bmap.normal : nil;
}

- (const char *)icon
{
    return [[self image] name];
}

- altImage
{
    return (HAS_ICON && !BF.iconIsKeyEquivalent) ? icon.bmap.alternate : nil;
}
    
- (const char *)altIcon
{
    return [[self altImage] name];
}

- (int)iconPosition
{
    if (BF.iconOverlaps) {
	return BF.iconAndText ? NX_ICONOVERLAPS : NX_ICONONLY;
    }
    if (BF.iconAndText) {
	if (BF.horizontal) {
	    return BF.bottomOrLeft ? NX_ICONLEFT : NX_ICONRIGHT;
	} else {
	    return BF.bottomOrLeft ? NX_ICONBELOW : NX_ICONABOVE;
	}
    } else {
	return NX_TITLEONLY;
    }
}


- setIconPosition:(int)aPosition
{
    NX_ASSERT(aPosition >= NX_TITLEONLY && aPosition <= NX_ICONOVERLAPS, \
	      "Invalid icon position in [ButtonCell -setIconPosition:]");
    BF.iconAndText = (aPosition > NX_ICONONLY) ? YES : NO;
    BF.iconOverlaps = (aPosition == NX_ICONONLY || aPosition == NX_ICONOVERLAPS) ? YES : NO;
    switch (aPosition) {
    case NX_ICONLEFT:
	BF.horizontal = YES;
	BF.bottomOrLeft = YES;
	[[self controlView] updateCellInside:self];
	break;
    case NX_ICONRIGHT:
	BF.horizontal = YES;
	BF.bottomOrLeft = NO;
	[[self controlView] updateCellInside:self];
	break;
    case NX_ICONBELOW:
	BF.horizontal = NO;
	BF.bottomOrLeft = YES;
	[self setAlignment:NX_CENTERED];
	break;
    case NX_ICONABOVE:
	BF.horizontal = NO;
	BF.bottomOrLeft = NO;
	[self setAlignment:NX_CENTERED];
	break;
    }
    return self;
}


- sound
{
    return sound;
}


- setSound:aSound
{
    sound = aSound;
    return self;
}


- (int)highlightsBy
{
    int                 retval = 0;

    retval |= (BF.pushIn ? NX_PUSHIN : 0);
    retval |= (BF.lightByContents ? NX_CONTENTS : 0);
    retval |= (BF.lightByGray ? NX_CHANGEGRAY : 0);
    retval |= (BF.lightByBackground ? NX_CHANGEBACKGROUND : 0);

    return retval;
}


- setHighlightsBy:(int)aType
{
    BF.lightByGray = (aType & NX_CHANGEGRAY) ? YES : NO;
    BF.lightByContents = (aType & NX_CONTENTS) ? YES : NO;
    BF.lightByBackground = (aType & NX_CHANGEBACKGROUND) ? YES : NO;
    BF.pushIn = (aType & NX_PUSHIN) ? YES : NO;
    [[self controlView] updateCell:self];
    return self;
}


- (int)showsStateBy
{
    int                 retval = 0;

    retval |= (BF.changeContents ? NX_CONTENTS : 0);
    retval |= (BF.changeGray ? NX_CHANGEGRAY : 0);
    retval |= (BF.changeBackground ? NX_CHANGEBACKGROUND : 0);

    return retval;
}


- setShowsStateBy:(int)aType
{
    BF.changeGray = (aType & NX_CHANGEGRAY) ? YES : NO;
    BF.changeContents = (aType & NX_CONTENTS) ? YES : NO;
    BF.changeBackground = (aType & NX_CHANGEBACKGROUND) ? YES : NO;
    [[self controlView] updateCell:self];
    return self;
}


- setType:(int)aType
{
    switch (aType) {
    case NX_MOMENTARYPUSH:
	BF.lightByContents = NO;
	BF.lightByGray = BF.lightByBackground = BF.pushIn = YES;
	BF.changeGray = BF.changeContents = BF.changeBackground = NO;
	break;
    case NX_MOMENTARYCHANGE:
	BF.lightByContents = YES;
	BF.lightByGray = BF.lightByBackground = BF.pushIn = NO;
	BF.changeGray = BF.changeContents = BF.changeBackground = NO;
	break;
    case NX_PUSHONPUSHOFF:
	BF.lightByGray = BF.pushIn = BF.lightByBackground = YES;
	BF.lightByContents = NO;
	BF.changeGray = BF.changeBackground = YES;
	BF.changeContents = NO;
	break;
    case NX_TOGGLE:
	BF.lightByGray = BF.lightByBackground = NO;
	BF.lightByContents = BF.pushIn = YES;
	BF.changeGray = BF.changeBackground = NO;
	BF.changeContents = YES;
	break;
    case NX_SWITCH:
	BF.lightByGray = BF.pushIn = BF.lightByBackground = NO;
	BF.lightByContents = YES;
	BF.changeGray = BF.changeBackground = NO;
	BF.changeContents = YES;
	[self setBordered:NO];
	if (!HAS_ICON || BF.iconIsKeyEquivalent) {
	    [self setIcon:"switch"];
	    [self setAltIcon:"switchH"];
	    BF.iconAndText = BF.horizontal = YES;
	    BF.iconOverlaps = BF.bottomOrLeft = NO;
	    cFlags1.alignment = NX_RIGHTALIGNED;
	}
	break;
    case NX_RADIOBUTTON:
	BF.lightByGray = BF.pushIn = BF.lightByBackground = NO;
	BF.lightByContents = YES;
	BF.changeGray = BF.changeBackground = NO;
	BF.changeContents = YES;
	[self setBordered:NO];
	if (!HAS_ICON || BF.iconIsKeyEquivalent) {
	    [self setIcon:"radio"];
	    [self setAltIcon:"radioH"];
	    BF.iconAndText = BF.horizontal = BF.bottomOrLeft = YES;
	    BF.iconOverlaps = NO;
	    cFlags1.alignment = NX_LEFTALIGNED;
	}
	break;
    }
    [[self controlView] updateCell:self];
    return self;
}


- (BOOL)isOpaque
{
    return (!bcFlags2.transparent && BF.bordered);
}


- (const char *)stringValue
{
    return cFlags1.state ? "" : (char *)0;
}


- setStringValue:(const char *)aString
{
    cFlags1.state = aString ? 1 : 0;
    [[self controlView] updateCellInside:self];
    return self;
}


- setStringValueNoCopy:(const char *)aString
{
    cFlags1.state = aString ? 1 : 0;
    [[self controlView] updateCellInside:self];
    return self;
}


- (int)intValue
{
    return cFlags1.state;
}


- setIntValue:(int)anInt
{
    cFlags1.state = (anInt ? 1 : 0);
    [[self controlView] updateCellInside:self];
    return self;
}


- (float)floatValue
{
    return (float)cFlags1.state;
}


- setFloatValue:(float)aFloat
{
    cFlags1.state = (aFloat ? 1 : 0);
    [[self controlView] updateCellInside:self];
    return self;
}


- (double)doubleValue
{
    return (double)cFlags1.state;
}


- setDoubleValue:(double)aDouble
{
    cFlags1.state = (aDouble ? 1 : 0);
    [[self controlView] updateCellInside:self];
    return self;
}


- setFont:fontObj
{
    if (BF.iconIsKeyEquivalent && icon.ke.font && fontObj) {
	if ([icon.ke.font pointSize] != [fontObj pointSize]) {
	    icon.ke.font = [Font newFont:[icon.ke.font name]
			    size:[fontObj pointSize]];
	    icon.ke.descent = getKEDescent(icon.ke.font);
	}
    }
    return [super setFont:fontObj];
}


- (BOOL)isBordered
{
    return (BF.bordered ? YES : NO);
}

- setBordered:(BOOL)flag
{
    if ((BF.bordered && !flag) || (!BF.bordered && flag)) {
	BF.bordered = flag ? YES : NO;
	[[self controlView] updateCell:self];
    }
    return self;
}


- (BOOL)isTransparent
{
    return bcFlags2.transparent;
}


- setTransparent:(BOOL)flag
{
    if (bcFlags2.transparent && !flag) {
	bcFlags2.transparent = flag ? YES : NO;
	[[self controlView] updateCell:self];
    } else {
	bcFlags2.transparent = flag ? YES : NO;
    }
    return self;
}


- setPeriodicDelay:(float)delay andInterval:(float)interval
{
    if (delay > 60.0)
	delay = 60.0;
    delay = delay * 1000.0;
    if (interval > 60.0)
	interval = 60.0;
    interval = interval * 1000.0;
    periodicDelay = (unsigned short)delay;
    periodicInterval = (unsigned short)interval;
    return self;
}

- getPeriodicDelay:(float *)delay andInterval:(float *)interval
{
    *delay = ((float)periodicDelay) / 1000.0;
    *interval = ((float)periodicInterval) / 1000.0;
    return self;
}


- (unsigned short)keyEquivalent
{
    return KEYEQ;
}


- setKeyEquivalent:(unsigned short)charCode
{
    KEYEQ = (char)charCode;
    if (BF.iconIsKeyEquivalent) {
	[[self controlView] updateCellInside:self];
    }
    return self;
}


- setKeyEquivalentFont:fontObj
{
    if (BF.iconIsKeyEquivalent) {
	icon.ke.font = fontObj;
	icon.ke.descent = getKEDescent(icon.ke.font);
	[[self controlView] updateCellInside:self];
    }
    return self;
}


- setKeyEquivalentFont:(const char *)fontName size:(float)fontSize
{
    return[self setKeyEquivalentFont:[Font newFont:fontName size:fontSize]];
}


/* 
 * Internal flag manipulations are made by the following functions using
 * constants defined in cell.h 
 */

int
_NXGetButtonParam(ButtonCell *self, int n)
{
    switch (n) {
    case NX_CELLDISABLED:
	return (self->cFlags1.disabled);
    case NX_CELLSTATE:
	return (self->cFlags1.state);
    case NX_CELLHIGHLIGHTED:
	return (self->cFlags1.highlighted);
    case NX_CELLEDITABLE:
	return (self->cFlags1.state);
    case NX_CHANGECONTENTS:
	return (self->BF.changeContents);
    case NX_CHANGEBACKGROUND:
	return (self->BF.changeBackground);
    case NX_CHANGEGRAY:
	return (self->BF.changeGray);
    case NX_LIGHTBYCONTENTS:
	return (self->BF.lightByContents);
    case NX_LIGHTBYBACKGROUND:
	return (self->BF.lightByBackground);
    case NX_LIGHTBYGRAY:
	return (self->BF.lightByGray);
    case NX_PUSHIN:
	return (self->BF.pushIn);
    case NX_OVERLAPPINGICON:
	return (self->BF.iconOverlaps);
    case NX_ICONHORIZONTAL:
	return (self->BF.horizontal);
    case NX_ICONONLEFTORBOTTOM:
	return (self->BF.bottomOrLeft);
    case NX_ICONISKEYEQUIVALENT:
	return (self->BF.iconIsKeyEquivalent);
    default:
	AK_ASSERT(NO, "ButtonCell - request for invalid parameter");
	return (0);
    }
}


void
_NXSetButtonParam(ButtonCell *self, int n, int val)
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
	self->cFlags1.state = val;
	break;
    case NX_CHANGECONTENTS:
	self->BF.changeContents = val;
	break;
    case NX_CHANGEBACKGROUND:
	self->BF.changeBackground = val;
	break;
    case NX_CHANGEGRAY:
	self->BF.changeGray = val;
	break;
    case NX_LIGHTBYCONTENTS:
	self->BF.lightByContents = val;
	break;
    case NX_LIGHTBYBACKGROUND:
	self->BF.lightByBackground = val;
	break;
    case NX_LIGHTBYGRAY:
	self->BF.lightByGray = val;
	break;
    case NX_PUSHIN:
	self->BF.pushIn = val;
	break;
    case NX_OVERLAPPINGICON:
	self->BF.iconOverlaps = val;
	break;
    case NX_ICONHORIZONTAL:
	self->BF.horizontal = val;
	break;
    case NX_ICONONLEFTORBOTTOM:
	self->BF.bottomOrLeft = val;
	break;
    case NX_ICONISKEYEQUIVALENT:
	self->BF.iconIsKeyEquivalent = val;
	break;
    default:
	AK_ASSERT(NO, "ButtonCell - invalid parameter set");
    }
}


- (int)getParameter:(int)aParameter
{
    return _NXGetButtonParam(self, aParameter);
}


- setParameter:(int)aParameter to:(int)value
{
    _NXSetButtonParam(self, aParameter, value);
    return self;
}


static void
insetBorder(inBounds, flipped)
    register NXRect    *inBounds;
    BOOL                flipped;
{
    inBounds->origin.x += BXOFFSET;
    if (flipped) {
	inBounds->origin.y += BHBORDERDELTA - BYOFFSET;
    } else {
	inBounds->origin.y += BYOFFSET;
    }
    inBounds->size.width -= BWBORDERDELTA;
    inBounds->size.height -= BHBORDERDELTA;
}


static void
uninsetBorder(inBounds, flipped)
    register NXRect    *inBounds;
    BOOL                flipped;
{
    inBounds->size.height += BHBORDERDELTA;
    inBounds->size.width += BWBORDERDELTA;
    if (flipped) {
	inBounds->origin.y -= BHBORDERDELTA - BYOFFSET;
    } else {
	inBounds->origin.y -= BYOFFSET;
    }
    inBounds->origin.x -= BXOFFSET;
}


- _getTitleSize:(NXSize *)sizePtr inRect:(NXRect *)inBounds
{
    char               *savedText;
    id                  savedSupport;
    NXSize              altSize;
    NXSize              _iSize = *sizePtr;

    register NXSize    *iSize = &_iSize;

    if (BF.bordered) {
	inBounds->size.width -= BWBORDERDELTA;
	inBounds->size.height -= BHBORDERDELTA +
	  (BF.iconAndText || !BF.iconOverlaps) ? 2.0 : 0.0;
    }
    if (ICON_AND_TEXT && !BF.iconOverlaps) {
	if (BF.horizontal) {
	    if (iSize->width) {
		inBounds->size.width -= iSize->width + ICONINSETm3;
	    }
	} else {
	    if (iSize->height) {
		inBounds->size.height -= iSize->height + ICONINSETm3;
	    }
	}
    }
    savedText = contents;
    savedSupport = support;

    if (ICON_ONLY) {
	if (cFlags1.type != NX_ICONCELL) {
	    [self _convertToIcon];
	}
	support = icon.bmap.normal;
    } else if (cFlags1.type == NX_ICONCELL) {
	cFlags1.type = NX_TEXTCELL;
    }
    if (!ICON_ONLY || support) {
	[super calcCellSize:sizePtr inRect:inBounds];
    } else {
	sizePtr->width = 0.0;
	sizePtr->height = 0.0;
    }

    if (BF.changeContents || BF.lightByContents) {
	if (ICON_ONLY) {
	    support = icon.bmap.alternate;
	} else {
	    contents = altContents;
	}
	if (contents || (ICON_ONLY && support)) {
	    [super calcCellSize:&altSize inRect:inBounds];
	    if (sizePtr->width < altSize.width) {
		sizePtr->width = altSize.width;
	    }
	    if (sizePtr->height < altSize.height) {
		sizePtr->height = altSize.height;
	    }
	}
    }
    contents = savedText;
    support = savedSupport;

    return self;
}


- _getIconRect:(NXRect *)theRect andTitleRect:(NXRect *)tRect
    flipped:(BOOL)flipped
{
    NXRect              _iconRect;
    register NXRect    *iconRect = &_iconRect;

    NXRect              _titleRect;
    register NXRect    *titleRect = &_titleRect;
    NXSize              titleSize;

    *iconRect = *theRect;
    [self _getIconRect:iconRect flipped:flipped];
    *titleRect = *theRect;
    titleSize = iconRect->size;
    [self _getTitleSize:&titleSize inRect:titleRect];
    *titleRect = *theRect;
    if (BF.bordered) {
	insetBorder(titleRect, flipped);
    }
    if (ICON_AND_TEXT && !BF.iconOverlaps) {
	if (BF.horizontal) {
	    titleRect->size.width -= iconRect->size.width + ICONINSETm3;
	    titleRect->origin.y +=
	      floor((titleRect->size.height - titleSize.height) / 2.0);
	    titleRect->size.height = titleSize.height;
	    if (BF.bottomOrLeft) {
		titleRect->origin.x = iconRect->origin.x +
		  iconRect->size.width + ICONINSET;
	    } else {
		titleRect->origin.x += ICONINSET;
	    }
	} else {
	    if (flipped ?
		!BF.bottomOrLeft : BF.bottomOrLeft) {
		titleRect->origin.y += iconRect->size.height + ICONINSET +
		  floor((titleRect->size.height - iconRect->size.height -
			 ICONINSET - titleSize.height) / 2.0);
	    } else {
		titleRect->origin.y += floor((iconRect->origin.y -
			     titleRect->origin.y - titleSize.height) / 2.0);
	    }
	    titleRect->size.height = titleSize.height;
	}
    } else {
	titleRect->origin.y +=
	  floor((titleRect->size.height - titleSize.height) / 2.0);
	titleRect->size.height = titleSize.height;
    }

    NXIntersectionRect(theRect, titleRect);
    *tRect = *titleRect;
    *theRect = *iconRect;

    return self;
}


- getDrawRect:(NXRect *)theRect
{
    if (BF.bordered) {
	insetBorder(theRect, YES);
    }
    return self;
}


- getTitleRect:(NXRect *)theRect
{
    NXRect              dummyRect = *theRect;

    [self _getIconRect:&dummyRect andTitleRect:theRect flipped:YES];
    return self;
}


- getIconRect:(NXRect *)theRect
{
    return[self _getIconRect:theRect flipped:YES];
}

- _getIconRect:(NXRect *)theRect flipped:(BOOL)flipped
{
    NXRect              _insetRect;
    register NXRect    *insetRect = &_insetRect;
    NXSize              iconSize;

    [self _getIconInfo:&iconSize];

    if (BF.bordered) {
	insetBorder(theRect, flipped);
    }
    *insetRect = *theRect;
    if (ICON_ONLY || (ICON_AND_TEXT && BF.iconOverlaps)) {
	insetRect->origin.x +=
	  floor((insetRect->size.width - iconSize.width) / 2.0);
	insetRect->origin.y +=
	  floor((insetRect->size.height - iconSize.height) / 2.0);
	insetRect->size = iconSize;
    } else if (ICON_AND_TEXT) {
	if (BF.horizontal) {
	    if (BF.bottomOrLeft) {
		insetRect->origin.x += ICONINSET;
	    } else {
		insetRect->origin.x += insetRect->size.width -
		  ICONINSET - iconSize.width;
	    }
	    insetRect->origin.y +=
	      floor((insetRect->size.height - iconSize.height) / 2.0);
	} else {
	    if (flipped ?
		!BF.bottomOrLeft : BF.bottomOrLeft) {
		insetRect->origin.y += ICONINSET;
	    } else {
		insetRect->origin.y += insetRect->size.height -
		  ICONINSET - iconSize.height;
	    }
	    insetRect->origin.x +=
	      floor((insetRect->size.width - iconSize.width) / 2.0);
	}
	insetRect->size = iconSize;
    } else {
	insetRect->size.width = 0.0;
	insetRect->size.height = 0.0;
	insetRect->origin.x = 0.0;
	insetRect->origin.y = 0.0;
    }

    if (BF.iconIsKeyEquivalent && BF.horizontal) {
	insetRect->origin.y = theRect->origin.y +
	  floor((theRect->size.height - iconSize.height) / 2.0);
	insetRect->origin.y +=
	  (flipped ? -icon.ke.descent : icon.ke.descent);
    } else if (ICON_AND_TEXT && !BF.iconOverlaps) {
	NXInsetRect(theRect, ICONINSET, ICONINSET);
    }
    NXIntersectionRect(theRect, insetRect);

    *theRect = *insetRect;

    return self;
}


- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect
{
    NXRect              _inBounds = *aRect;
    register NXRect    *inBounds = &_inBounds;
    register NXSize    *sizePtr = theSize;
    NXSize              iconSize;
    register NXSize    *iSize = &iconSize;

    [self _getIconInfo:iSize];
    *sizePtr = *iSize;
    [self _getTitleSize:sizePtr inRect:inBounds];

    if (BF.bordered) {
	sizePtr->height +=
	  (BF.iconAndText || !BF.iconOverlaps) ? 2.0 : 0.0;
    }
    if (BF.iconIsKeyEquivalent && ICON_AND_TEXT) {
	if (!BF.iconOverlaps) {
	    if (BF.horizontal) {
		if (iSize->width) {
		    sizePtr->width += iSize->width + ICONINSETm3;
		    if (iSize->height > sizePtr->height) {
			sizePtr->height = iSize->height;
		    }
		}
	    } else {
		if (iSize->height) {
		    sizePtr->height += iSize->height + ICONINSETm3;
		    if (iSize->width > sizePtr->width) {
			sizePtr->width = iSize->width;
		    }
		}
	    }
	}
    } else if (ICON_AND_TEXT) {
	if (BF.iconOverlaps) {
	    if (iSize->width > sizePtr->width) {
		sizePtr->width = iSize->width;
	    }
	    if (iSize->height > sizePtr->height) {
		sizePtr->height = iSize->height;
	    }
	} else {
	    if (BF.horizontal) {
		if (iSize->width) {
		    sizePtr->width += iSize->width + ICONINSETm3;
		    if (iSize->height + ICONINSETm2 > sizePtr->height) {
			sizePtr->height = iSize->height + ICONINSETm2;
		    }
		}
	    } else {
		if (iSize->height) {
		    sizePtr->height += iSize->height + ICONINSETm3;
		    if (iSize->width + ICONINSETm2 > sizePtr->width) {
			sizePtr->width = iSize->width + ICONINSETm2;
		    }
		}
	    }
	}
    }
    if (BF.bordered) {
	sizePtr->width += BWBORDERDELTA;
	sizePtr->height += BHBORDERDELTA;
    }
    return self;
}


#define sixArray(dest, s0, s1, s2, s3, s4, s5) \
    dest[0] = s0; dest[1] = s1; dest[2] = s2; \
    dest[3] = s3; dest[4] = s4; dest[5] = s5;

#define BACKINGA	1.0
#define BACKINGB	0.5

- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    NXRect              _rp = *cellFrame;
    register NXRect    *rp = &_rp;
    int                 miny, maxy;
    int                 psSides[6];
    float               g, grays[6];
    BOOL                flipped = [controlView isFlipped];

    if ([self controlView] != controlView)
	[self _setView:controlView];

    if (!bcFlags2.transparent) {
	if (BF.bordered) {
	    maxy = flipped ? NX_YMAX : NX_YMIN;
	    miny = flipped ? NX_YMIN : NX_YMAX;
	    if (cFlags1.highlighted && BF.pushIn) {
		sixArray(psSides, maxy, NX_XMAX, miny, NX_XMIN, NX_XMIN, miny);
		g = ((cFlags1.state &&
		      ((!BF.changeGray && !BF.changeBackground &&
			(BF.lightByGray || BF.lightByBackground)) ||
		       (!BF.lightByGray && !BF.lightByBackground &&
			(BF.changeGray || BF.changeBackground)))) ||
		     ((BF.lightByGray || BF.lightByBackground) &&
		      !cFlags1.state)) ? NX_WHITE : NX_LTGRAY;
		sixArray(grays, NX_WHITE, NX_WHITE, NX_BLACK, NX_BLACK, g, g);
	    } else {
		sixArray(psSides, maxy, NX_XMAX, miny, NX_XMIN, NX_XMAX, maxy);
		sixArray(grays, NX_BLACK, NX_BLACK, NX_WHITE,
			 NX_WHITE, NX_DKGRAY, NX_DKGRAY);
	    }
	    _NXDrawTiledRects(rp, (NXRect *)0, psSides, grays, 6, controlView);
	}
	[self drawInside:cellFrame inView:controlView];
    }
    return self;
}

- _compositeIcon:theIcon inRect:(NXRect *)iRect inView:controlView
{
    char               *savedContents;
    id                  savedSupport;
    int                 alignment;
    char                keStr[2];

    if (bcFlags1.iconAndText) {
	if (icon.bmap.normal != nil) {
	    if (BF.iconIsKeyEquivalent && !BF.iconOverlaps && KEYEQ) {
		keStr[0] = KEYEQ;
		keStr[1] = 0;
		savedContents = contents;
		savedSupport = support;
		contents = keStr;
		support = icon.ke.font;
		alignment = cFlags1.alignment;
		cFlags1.alignment = NX_CENTERED;
		NXOffsetRect(iRect, 1.0, 0.0);
		_NXDrawTextCell(self, controlView, iRect, NO);
		NXOffsetRect(iRect, - 1.0, 0.0);
		cFlags1.alignment = alignment;
		contents = savedContents;
		support = savedSupport;
	    } else if (!BF.iconIsKeyEquivalent) {
		_NXDrawIcon(theIcon, iRect, controlView);
	    }
	}
    }

    return self;
}

#define switchToAlternateIf(condition) \
    if (ICON_ONLY) { \
	support = icon.bmap.normal; \
    } \
    if (condition) { \
	if (ICON_ONLY) { \
	    if (icon.bmap.alternate) { \
		support = icon.bmap.alternate; \
	    } \
	} else if (altContents) { \
	    contents = altContents; \
	} \
	tRect = ICON_ONLY ? aiRect : tRect; \
	theIcon = icon.bmap.alternate ? \
	    icon.bmap.alternate : icon.bmap.normal; \
	iRect = icon.bmap.alternate ? aiRect : iRect; \
    }


- drawInside:(const NXRect *)aRect inView:controlView
{
    char               *savedText = contents;
    id                  savedSupport = support;
    id                  theIcon = icon.bmap.normal;
    BOOL                flipped = [controlView isFlipped], needsRectFill;
    float               gray;

    NXRect              _userRect = *aRect;
    register NXRect    *userRect = &_userRect;

    NXRect              _altIconRect;
    register NXRect    *aiRect = &_altIconRect;
    NXRect              titleRect;
    register NXRect    *tRect = &titleRect;
    NXRect              iconRect;
    register NXRect    *iRect = &iconRect;
    BOOL                drawState, swapIconRects = NO;

    iconRect = *userRect;
    [self _getIconRect:iRect andTitleRect:tRect flipped:flipped];

    if (cFlags1.highlighted && BF.pushIn && BF.bordered) {
	NXOffsetRect(tRect, 1.0, flipped ? 1.0 : -1.0);
	NXOffsetRect(iRect, 1.0, flipped ? 1.0 : -1.0);
	NXOffsetRect(userRect, 1.0, flipped ? 1.0 : 0.0);
    }
    if (BF.iconSizeDiff) {
	[icon.bmap.alternate getSize:&aiRect->size];
	if (aiRect->size.width == iRect->size.width &&
	    aiRect->size.height == iRect->size.height) {
	    [icon.bmap.normal getSize:&aiRect->size];
	    swapIconRects = YES;
	}
	aiRect->origin.x = iRect->origin.x +
	  floor((iRect->size.width - aiRect->size.width) / 2.0);
	aiRect->origin.y = iRect->origin.y +
	  floor((iRect->size.height - aiRect->size.height) / 2.0);
	if (swapIconRects) {
	    iRect = &_altIconRect;
	    aiRect = &iconRect;
	}
    } else {
	aiRect = iRect;
    }

    if (ICON_ONLY) {
	tRect = iRect;
	if (iRect == &_altIconRect &&
	    !BF.changeContents && !BF.lightByContents) {
	    iRect->origin.x = aiRect->origin.x;
	    iRect->origin.y = aiRect->origin.y;
	}
    }
    if (BF.bordered) {
	insetBorder(userRect, flipped);
    }
    needsRectFill = BF.bordered || LIGHTBYBKGD || LIGHTBYGRAY ||
	((!ICON_ONLY || cFlags1.bordered || BF.iconSizeDiff) &&
	 (BF.changeContents || BF.lightByContents) &&
	 contents && altContents);
    drawState = (cFlags1.state ? 2 : 0) + (cFlags1.highlighted ? 1 : 0);
    switch (drawState) {
    case 0:
	if (needsRectFill) {
	    PSsetgray(NORMAL_BKGD);
	    PSrectfill(userRect->origin.x, userRect->origin.y,
		       userRect->size.width, userRect->size.height);
	}
	[self _compositeIcon:icon.bmap.normal inRect:iRect inView:controlView];
	if (ICON_ONLY) {
	    support = icon.bmap.normal;
	}
	[super drawInside:tRect inView:controlView];
	break;
    case 1:
	if (needsRectFill) {
	    gray = LIGHTBYBKGD ? NX_WHITE : NORMAL_BKGD;
	    if (NXDrawingStatus != NX_DRAWING && LIGHTBYGRAY) {
		gray = NX_WHITE;
	    }
	    PSsetgray(gray);
	    PSrectfill(userRect->origin.x, userRect->origin.y,
		       userRect->size.width, userRect->size.height);
	}
	switchToAlternateIf(BF.lightByContents);
	[self _compositeIcon:theIcon inRect:iRect inView:controlView];
	cFlags1.highlighted = NO;
	[super drawInside:tRect inView:controlView];
	cFlags1.highlighted = YES;
	if (LIGHTBYGRAY && NXDrawingStatus == NX_DRAWING) {
	    NXHighlightRect(userRect);
	}
	break;
    case 2:
	if (needsRectFill) {
	    gray = CHANGEBKGD ? ALT_BKGD : NORMAL_BKGD;
	    if (NXDrawingStatus != NX_DRAWING && CHANGEGRAY) {
		gray = NX_WHITE;
	    }
	    PSsetgray(gray);
	    PSrectfill(userRect->origin.x, userRect->origin.y,
		       userRect->size.width, userRect->size.height);
	}
	switchToAlternateIf(BF.changeContents);
	[self _compositeIcon:theIcon inRect:iRect inView:controlView];
	[super drawInside:tRect inView:controlView];
	if (CHANGEGRAY && NXDrawingStatus == NX_DRAWING) {
	    NXHighlightRect(userRect);
	}
	break;
    case 3:
	if (needsRectFill) {
	    gray = ((LIGHTBYBKGD && !CHANGEBKGD)
		  || (!LIGHTBYBKGD && CHANGEBKGD)) ? NX_WHITE : NORMAL_BKGD;
	    if (NXDrawingStatus != NX_DRAWING &&
	     ((LIGHTBYGRAY && !CHANGEGRAY) || (CHANGEGRAY && !LIGHTBYGRAY))) {
		gray = NX_WHITE;
	    }
	    PSsetgray(gray);
	    PSrectfill(userRect->origin.x, userRect->origin.y,
		       userRect->size.width, userRect->size.height);
	}
	switchToAlternateIf((BF.changeContents && !BF.lightByContents) ||
			    (BF.lightByContents && !BF.changeContents));
	[self _compositeIcon:theIcon inRect:iRect inView:controlView];
	cFlags1.highlighted = NO;
	[super drawInside:tRect inView:controlView];
	cFlags1.highlighted = YES;
	if (NXDrawingStatus == NX_DRAWING &&
	    (LIGHTBYGRAY && !CHANGEGRAY) || (CHANGEGRAY && !LIGHTBYGRAY)) {
	    NXHighlightRect(userRect);
	}
	break;
    }

    contents = savedText;
    support = savedSupport;

    BF.lastState = cFlags1.state;

    return self;
}


- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag
{
    NXRect              _userRect = *cellFrame;
    NXRect             *userRect = &_userRect;
    BOOL                flipped = [controlView isFlipped];

    if (flag == cFlags1.highlighted || bcFlags2.transparent ||
	(cFlags1.disabled && flag)) {
	return self;
    }
    cFlags1.highlighted = flag ? YES : NO;
    if ((!BF.pushIn || !BF.bordered) && !BF.lightByContents && LIGHTBYGRAY &&
	!BF.changeContents && !BF.changeBackground && !BF.changeGray) {
	if (BF.bordered) {
	    insetBorder(userRect, flipped);
	    NXHighlightRect(userRect);
	    uninsetBorder(userRect, flipped);
	} else {
	    NXHighlightRect(userRect);
	}
    } else if (!BF.pushIn || !BF.bordered) {
	[self drawInside:userRect inView:controlView];
    } else {
	[self drawSelf:userRect inView:controlView];
    }

    return self;
}


- _startSound
{
    if (sound) {
	if (bcFlags2._momentarySound) {
	    [sound play];
	} else {
	    if (cFlags1.state &&
	      (BF.changeContents || BF.changeGray || BF.changeBackground)) {
		[sound stop];
	    } else {
		[sound play];
	    }
	}
    }
    return self;
}


- _stopSound
{
    if (bcFlags2._momentarySound) {
	[sound stop];
    }
    return self;
}


- (BOOL)trackMouse:(NXEvent *)theEvent inRect:(const NXRect *)cellFrame
    ofView:controlView
{
    BOOL                retval;

    [self _startSound];
    retval = [super trackMouse:theEvent inRect:cellFrame ofView:controlView];
    [self _stopSound];

    return retval;
}


- performClick:sender
{
    int state;
    volatile NXHandler	handler;
    id controlView = [self controlView];

    if (![self isEnabled])
	return self;

    if (controlView) {
	[[controlView window] disableDisplay];
	state = [self state];
	[controlView selectCell:self];
	[self setState:state];
	[[controlView window] reenableDisplay];
	if (!cFlags1.highlighted) {
	    cFlags1.highlighted = YES;
	    [controlView drawCell:self];
	}

	DPSFlush();
	[self _startSound];
	[self incrementState];
    
	NX_DURING {
	    handler.code = 0;
	    [controlView sendAction:[self action] to:[self target]];
	} NX_HANDLER {
	    handler = NXLocalHandler;
	} NX_ENDHANDLER
    
	[self _stopSound];
	if (cFlags1.highlighted) {
	    cFlags1.highlighted = NO;
	    [controlView drawCell:self];
	}
    
	if (handler.code) {
	    NX_RAISE(handler.code, handler.data1, handler.data2);
	}
    }

    return self;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "ss", &periodicDelay, &periodicInterval);
    NXWriteTypes(stream, "*@ss", &altContents, &sound, &bcFlags1, &bcFlags2);
    if (bcFlags1.iconIsKeyEquivalent)
	NXWriteType(stream, "@f", &icon.ke);
    else {
	if (!(bcFlags1.iconAndText || bcFlags1.iconOverlaps)) {
	    struct {
		id                  normal;
		id                  alternate;
	    }                   dummy;

	    dummy.normal = dummy.alternate = nil;
	    NXWriteType(stream, "@@", &dummy);
	} else {
	    NXWriteType(stream, "@@", &icon.bmap);
	}
    }
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    if (NXSystemVersion(stream) < NXSYSTEMVERSION083) {
	periodicDelay = 200;
	periodicInterval = 25;
    } else
	NXReadTypes(stream, "ss", &periodicDelay, &periodicInterval);
    NXReadTypes(stream, "*@ss", &altContents, &sound, &bcFlags1, &bcFlags2);
    if (bcFlags1.iconIsKeyEquivalent) {
	NXReadType(stream, "@f", &icon.ke);
    } else {
	NXReadType(stream, "@@", &icon.bmap);
    }
    return self;
}

@end

/*
  
Modifications (since 0.8):
  
11/21/88 pah	removed support for cFlags1.shared
		removed support for cFlags2.flipped (added 
		 method _getIconRect:flipped:, changed name of another
		 to _getIconRect:andTitleRect:flipped:
11/23/88 pah	got rid of _isInsideOpaque
		changed code to support _NX{Draw,Edit}TextCell instead
		 of _NX{Draw,Edit}TextInViewRect
11/28/88 pah	fixed isOpaque so that it returns YES if it has only
		 an icon and that icon has no alpha (this might cause
		 small nits if the icon is smaller than the rect
		 passed to drawSelf:inView:)
  
0.82
----
  1/9/89 pah	break sound starting and stopping out into _startSound and
		 _stopSound
 1/10/89 pah	add  button type NX_MOMENTARYCHANGE
 1/15/89 pah	ICON_ONLY buttons with alpha icons are still opaque (since
		 they may be smaller than the bounds)
 1/25/89 trey	setTitle: takes a const param
		made setString* methods take const params
		made keStr an automatic instead of static
 1/27/89 bs	added read: write:
02/18/89 avie	Don't setgray in button display unless necessary.		
  
0.83
----
 3/12/89 pah	Fix avie fix above so that switches are properly transparent
 3/16/89 pah	Fix alignment of key equivalents

0.92
----
  6/7/89 pah	Add performClick: (what a hack!)

0.93
----
 6/14/89 pah	support the fact that findBitmapFor: now returns nil if !found
 6/15/89 pah	make title, altTitle, icon, altIcon, stringValue all return
		 a const char * instead of char *
		add autodisplay to various methods

0.96
----
 7/20/89 pah	don't do performClick: if cell is disabled
 7/21/89 pah	bump key equivalent to the right by one pixel (to counter
		 the printer-width/screen-width font size averaging)

83
--
 4/25/90 aozer	Nuked compositeIcon(); instead use _NXDrawIcon() (defined in
		Cell.m), allowing for icons to be Bitmaps or NXImages.
		Renamed default icon from square16 to NXsquare16.

84
--
 5/6/90 aozer	Made setAltTitle: set altContents to NULL. (Fix to bug #5455: 
		setAltTitle:"" followed by another setAltTitle: would crash.)

89
--
 7/24/90 aozer	Added setImage:, image, setAltImage:, altImage. Made setIcon:
		and friends call setImage: and friends.

92
--
 8/20/90 aozer	Added defaults for periodic delay & interval (Bug 6117).

100
---
 10/24/90 aozer	Fixed a bug with the periodic delay & interval initialization
		& changed the interval from its 1.0 value of 0.025 to 0.075.
 
*/





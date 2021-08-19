/*
	TextFieldCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Cell_Private.h"
#import "TextFieldCell.h"
#import "View.h"
#import "Control.h"
#import "nextstd.h"
#import <dpsclient/wraps.h>

typedef struct {
    @defs (View)
}                  *viewId;

/*
 * The structure that we hang off the end of every instance of TextFieldCell.
 * Best way to refer to this is through the private macro below.
 *
 * Note that because there are many TextFieldCells, the private field itself
 * will be allocated lazily. Because each NXColor is 16 bytes, it makes sense
 * to put both in the private field rather than pointers (otherwise we would
 * allocate 3 16-byte chunks). If we extend the private field some day, we
 * might want to make the NXColor items pointers.
 */
typedef struct _TextFieldCellPrivate {
    NXColor textColor;
    NXColor backgroundColor;
} TextFieldCellPrivate;

#define private ((TextFieldCellPrivate *)_private)

#define ALLOCPRIVATE \
	if (!private) { \
	    NXZone *zone = [self zone]; \
	    NX_ZONEMALLOC(zone, private, TextFieldCellPrivate, 1); \
	    private->textColor = private->backgroundColor = _NXNoColor(); \
	}

@implementation TextFieldCell:ActionCell

+ initialize
{
    [self setVersion:1];
    return self;
}

+ new
{
    return [self newTextCell];
}

- init
{
    return [[self initTextCell:NULL] setStringValueNoCopy:"Field"];
}

+ newTextCell
{
    return [[self newTextCell:NULL] setStringValueNoCopy:"Field"];
}

+ newTextCell:(const char *)aString
{
    return [[self allocFromZone:NXDefaultMallocZone()] initTextCell:aString];
}

- initTextCell:(const char *)aString
{
    [super initTextCell:aString];
    backgroundGray = -1.0;
    textGray = NX_BLACK;
    return self;
}


- copy
{
    return [self copyFromZone:[self zone]];
}

- copyFromZone:(NXZone *)zone
{
    TextFieldCell      *retval;

    retval = [super copyFromZone:zone];
    retval->backgroundGray = backgroundGray;
    retval->textGray = textGray;
    if (private) {
	NX_ZONEMALLOC(zone, retval->_private, TextFieldCellPrivate, 1);
	*(TextFieldCellPrivate *)(retval->_private) = *private;
    } else {
	retval->_private = NULL;
    }
    return retval;
}

/*
 * The way we deal with colors & grays:
 * Color & gray are independent. However, if color isn't set, it follows
 * the gray. If color is explicitly set, it is on its own. Gray is always
 * set.
 *
 * Background gray is somewhat weird: The old convention is to keep
 * transparency information by storing -1 in the gray.  We now have a new
 * method to indicate that the background is transparent.  However it
 * is still the case that when the background
 * is made transparent, the color & gray information is lost; if you set the
 * gray or color after that point, the object is made opaque once again.
 *
 * We need to clean this up in a incompatible release!
 */

- (BOOL)isOpaque
{
    return (cFlags1.bezeled || backgroundGray >= 0.0);
}


- (float)backgroundGray
{
    return backgroundGray;
}


- setBezeled:(BOOL)flag
{
    if (backgroundGray < 0.0 && flag) {
	backgroundGray = NX_WHITE;
    }
    return[super setBezeled:flag];
}

- setBackgroundGray:(float)value
{
    id text;
    value = (value < 0.0 ? -1.0 : MAX(MIN(value,1.0),0.0));
    if ((value < 0.0) && private) {
	private->backgroundColor = _NXNoColor();
    }
    if (backgroundGray != value) {
	backgroundGray = value;
	if ((text = [[self controlView] currentEditor]) &&
	    ([[self controlView] selectedCell] == self ||
	     [[self controlView] cell] == self)) {
	    [text setBackgroundGray:value];
	}
	[[self controlView] updateCell:self];
    }

    return self;
}


- (float)textGray
{
    return textGray;
}


- setTextGray:(float)value
{
    id text;
    value = MAX(MIN(value,1.0),0.0);
    if (textGray != value) {
	textGray = value;
	if ((text = [[self controlView] currentEditor]) &&
	    ([[self controlView] selectedCell] == self ||
	     [[self controlView] cell] == self)) {
	    [text setTextGray:value];
	}
	[[self controlView] updateCellInside:self];
    }

    return self;
}

- setBackgroundColor:(NXColor)color
{
    id text;
    ALLOCPRIVATE;
    private->backgroundColor = color;
    if (backgroundGray < 0.0) {
	backgroundGray = NXGrayComponent(color); 
    }
    if ((text = [[self controlView] currentEditor]) &&
	([[self controlView] selectedCell] == self ||
	    [[self controlView] cell] == self)) {
	[text setBackgroundColor:color];
    }
    [[self controlView] updateCellInside:self];
    return self;
}

- setBackgroundTransparent:(BOOL)flag
{
    if (flag) {
	[self setBackgroundGray:-1.0];
    } else {
	[self setBackgroundGray:NX_WHITE];	/* ??? */
    }
    return self;
}

- (BOOL)isBackgroundTransparent
{
    return (backgroundGray < 0.0);
}

- (BOOL)backgroundTransparent	/* historical */
{
    return [self isBackgroundTransparent];
}

- setTextColor:(NXColor)color
{
    id text;
    ALLOCPRIVATE;
    private->textColor = color;
    if ((text = [[self controlView] currentEditor]) &&
	([[self controlView] selectedCell] == self ||
	    [[self controlView] cell] == self)) {
	[text setTextColor:color];
    }
    [[self controlView] updateCellInside:self];
    return self;
}

-(BOOL)_backColorSpecified
{
    return (private && _NXIsValidColor(private->backgroundColor));
}

-(BOOL)_textColorSpecified
{
    return (private && _NXIsValidColor(private->textColor));
}

- (NXColor)backgroundColor
{
    if ([self _backColorSpecified]) {
	return private->backgroundColor;
    } else if (backgroundGray < 0.0) {
	return NX_COLORCLEAR;	/* What else can we return? */
    } else {
	return NXConvertGrayToColor(backgroundGray);
    }
}

- (NXColor)textColor
{
    return [self _textColorSpecified] ?
		private->textColor : NXConvertGrayToColor(textGray);
}

- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    if ([self controlView] != controlView)
	[self _setView:controlView];

    if (cFlags1.bordered) {
	PSsetgray(0.0);
	NXFrameRectWithWidth(cellFrame, 1.0);
    } else if (cFlags1.bezeled) {
	if (backgroundGray > 0.3 && backgroundGray < 0.7) {
	    _NXDrawGrayBezel(cellFrame, NULL, controlView);
	} else {
	    _NXDrawWhiteBezel(cellFrame, NULL, controlView);
	}
    }
    return[self drawInside:cellFrame inView:controlView];
}


- drawInside:(const NXRect *)cellFrame inView:controlView
{
    NXSize cellSize;
    NXRect              _inBounds = *cellFrame;
    register NXRect    *inBounds = &_inBounds;

    if (backgroundGray >= 0.0) {
	if (cFlags1.bezeled) {
	    NXInsetRect(inBounds, 2.0, 2.0);
	} else if (cFlags1.bordered) {
	    NXInsetRect(inBounds, 1.0, 1.0);
	}
	if ([self _backColorSpecified] && [controlView shouldDrawColor]) {
	    NXSetColor (private->backgroundColor);
        } else {
	    PSsetgray (backgroundGray);
	}
	NXRectFill(inBounds);
	if (cFlags1.bezeled) {
	    NXInsetRect(inBounds, 1.0, 1.0);
	}
    } else {
	[self getDrawRect:inBounds];
    }

    if (_cFlags3.center) {
	[super calcCellSize:&cellSize inRect:inBounds];
	if (cellSize.width < inBounds->size.width) {
	    inBounds->origin.x += floor((inBounds->size.width - cellSize.width + 1.0) / 2.0);
	    inBounds->size.width = cellSize.width;
	}
	if (cellSize.height < inBounds->size.height) {
	    inBounds->origin.y += floor((inBounds->size.height - cellSize.height + 1.0) / 2.0);
	    inBounds->size.height = cellSize.height;
	}
    }

    _NXDrawTextCell(self, controlView, inBounds, YES);

    if (cFlags1.highlighted) NXHighlightRect(inBounds);

    return self;
}


- setTextAttributes:textObj
{
    float colorChange = 0.0;

#if 0
// ??? Don't know why we need this. But maybe we do. Nuke it in 88 if nothing
// breaks. -Ali
    float               savedBgGray = [textObj backgroundGray];

    [super setTextAttributes:textObj];
    [textObj setBackgroundGray:savedBgGray];
#endif

    if (![self backgroundTransparent]) {
	[textObj setBackgroundGray:backgroundGray];
	[textObj setBackgroundColor:[self backgroundColor]];
	if (cFlags1.disabled) {
	    colorChange = (backgroundGray > textGray) ? 0.333 : -0.333;
	}
    } else {
	if (cFlags1.disabled) {
	    colorChange = -0.333;
	}
    }
    [textObj setTextGray:textGray + colorChange];
    if (colorChange != 0.0) {
	NXColor color = [self textColor];
	[textObj setTextColor:NXChangeBrightnessComponent(color, 
				NXBrightnessComponent(color) + colorChange)];
    } else {
	[textObj setTextColor:[self textColor]];
    }

    return textObj;
}

- (BOOL)trackMouse:(NXEvent *)event inRect:(const NXRect *)aRect ofView:controlView
{
    BOOL                disabled, retval;

    disabled = cFlags1.disabled;
    cFlags1.disabled = YES;
    retval = [super trackMouse:event inRect:aRect ofView:controlView];
    cFlags1.disabled = disabled;
    return retval;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "ff", &backgroundGray, &textGray);
    NXWriteColor(stream, [self _textColorSpecified] ?
				[self textColor] : _NXNoColor());
    NXWriteColor(stream, [self _backColorSpecified] ?
				[self backgroundColor] : _NXNoColor());

    return self;
}


- read:(NXTypedStream *) stream
{
    int version;

    [super read:stream];
    version = NXTypedStreamClassVersion(stream, "TextFieldCell");
    NXReadTypes(stream, "ff", &backgroundGray, &textGray);
    if (version >= 1) {
	NXColor tmpColor;
	if (_NXIsValidColor(tmpColor = NXReadColor(stream))) {
	    [self setTextColor:tmpColor];
	}
	if (_NXIsValidColor(tmpColor = NXReadColor(stream))) {
	    [self setBackgroundColor:tmpColor];
	}
    }
    return self;
}


- free
{
    if (private) {
	NX_FREE(private);
    }

    return[super free];
}

@end

/*
  
Modifications (since 0.8):
  
11/21/88 pah	removed support for cFlags1.flipped
11/23/88 pah	removed _isInsideOpaque and _getInsideBounds:
		overrode setTextAttributes: from Cell and thereby
		 eliminated the need to override _selectOrEdit:...
		changed drawSelf:inView: to use _NXDrawTextCell instead
		 of _NXDrawTextInViewRect
  
0.82
----
 1/03/89 pah	fix setStringValue:, et. al., so they don't display if disabled
 1/12/89 pah	override trackMouse: so it does nothing
 1/25/89 trey	made setString* methods take const params
 1/27/89 bs	added read: write:

0.93
----
 6/15/89 pah	make things autodisplay

83
--
 4/27/90 aozer	Color support. By default, TextFieldCell has its text and
		background gray set; the colors aren't set.  On the color
		display, the grays are used.  Once color is set, then the color
		and the gray value are kept separately and the appropriate one
		is used depending on the output device.  When a TextFieldCell
		is archived, if the color & the gray are the same, then the
		color is not archived (this is an efficiency hack).
 4/27/90 aozer	Added setBackgroundTransparent:; use this method instead of
		the setBackgroundGray:-1.0 kludge. Setting background to
		transparent causes the object to forget the gray (and color)
		values. Setting gray or color (or setBackgroundTransparent:NO)
		makes the background opaque again...

85
--
 6/1/90 aozer	Fixed bug in copy (where it tried to copy *private even 
		when private was NULL).

87
--
 7/11/90 aozer	Cleaned up setTextAttributes & got it to set color if 
		specified.

*/

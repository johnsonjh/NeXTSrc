/*
	FormCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Cell_Private.h"
#import "FormCell.h"
#import "Cursor.h"
#import "View.h"
#import "Text.h"
#import "nextstd.h"
#import <dpsclient/wraps.h>
#import <math.h>
#import <zone.h>

@implementation FormCell:ActionCell


+ new
{
    return [self newTextCell];
}


- init
{
    [self initTextCell:NULL];
    [titleCell setStringValueNoCopy:"Field:"];
    return self;
}


+ newTextCell
{
    self = [self newTextCell:NULL];
    [titleCell setStringValueNoCopy:"Field:"];
    return self;
}


+ newTextCell:(const char *)aString
{
    return [[self allocFromZone:NXDefaultMallocZone()] initTextCell:aString];
}


- initTextCell:(const char *)aString
{
    self = [super initTextCell:NULL];
    [self setStringValueNoCopy:""];
    [self setAlignment:NX_LEFTALIGNED];
    titleCell = [[Cell allocFromZone:[self zone]] initTextCell:aString];
    [titleCell setAlignment:NX_RIGHTALIGNED];
    titleWidth = -1.0;
    titleEndPoint = -1.0;
    cFlags1.editable = YES;
    [self setBezeled:YES];
    return self;
}


- free
{
    [titleCell free];
    return[super free];
}


- copy
{
    FormCell *retval;
    NXZone *zone = [self zone];

    retval = [super copyFromZone:zone];
    retval->titleCell = [titleCell copyFromZone:zone];

    return retval;
}


- (NXCoord)titleWidth:(const NXSize *)aSize
{
    NXRect rect;
    NXSize titleSize;

    if (titleWidth < 0.0) {
	if (aSize) {
	    rect.origin.x = 0.0;
	    rect.origin.y = 0.0;
	    rect.size = *aSize;
	    [titleCell calcCellSize:&titleSize inRect:&rect];
	} else {
	    [titleCell calcCellSize:&titleSize];
	}
	return titleSize.width;
    } else {
	return titleWidth;
    }
}


- (NXCoord)titleWidth
{
    return[self titleWidth:(NXSize *)0];
}


- setTitleWidth:(NXCoord)width
{
    titleWidth = width;
    return self;
}


- (const char *)title
{
    return[titleCell stringValue];
}


- setTitle:(const char *)aString
{
    [titleCell setStringValue:aString];
    return self;
}


- titleFont
{
    return[titleCell font];
}


- setTitleFont:fontObj
{
    [titleCell setFont:fontObj];
    return self;
}


- (int)titleAlignment
{
    return[titleCell alignment];
}


- setTitleAlignment:(int)mode
{
    [titleCell setAlignment:mode];
    return self;
}


- setEnabled:(BOOL)flag
{
    [titleCell setEnabled:flag];
    return[super setEnabled:flag];
}


- (BOOL)isOpaque
{
    const char *title = [titleCell stringValue];
    return (!title || !strlen(title));
}


- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect
{
    NXRect inBounds;
    NXSize titleSize;

    inBounds = *aRect;

    theSize->width = 0.0;
    theSize->height = 0.0;

    if (titleWidth < 0.0) {
	[titleCell calcCellSize:&titleSize inRect:&inBounds];
	inBounds.size.width -= titleSize.width;
	if (inBounds.size.width > 0.0) {
	    [super calcCellSize:theSize inRect:&inBounds];
	}
    } else {
	inBounds.size.width = MIN(titleWidth, inBounds.size.width);
	[titleCell calcCellSize:&titleSize inRect:&inBounds];
	titleSize.width = MAX(titleSize.width, titleWidth);
	inBounds.size.width = aRect->size.width - titleWidth;
	if (inBounds.size.width > 0.0) {
	    [super calcCellSize:theSize inRect:&inBounds];
	}
    }

    theSize->width += titleSize.width;
    theSize->height = MAX(titleSize.height, theSize->height);

    return self;
}


- drawInside:(const NXRect *)cellFrame inView:controlView
{
    NXSize size;
    NXRect inBounds;

    inBounds = *cellFrame;
    [titleCell calcCellSize:&size];

    inBounds.size.width = MIN(inBounds.size.width, titleWidth < 0.0 ? size.width : titleEndPoint);
    inBounds.origin.x += inBounds.size.width;
    inBounds.size.width = cellFrame->size.width - inBounds.size.width;
    inBounds.size.height = cellFrame->size.height;
    inBounds.origin.y = cellFrame->origin.y;
    if (inBounds.size.width > 0.0) [super drawInside:&inBounds inView:controlView];

    return self;
}


- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    NXSize size;
    NXRect inBounds;

    if ([self controlView] != controlView) [self _setView:controlView];

    inBounds = *cellFrame;
    [titleCell calcCellSize:&size];

    inBounds.size.width = MIN(inBounds.size.width, titleWidth < 0.0 ? size.width : titleWidth);
    inBounds.origin.y += floor((inBounds.size.height - size.height) / 2.0);
    inBounds.size.height = size.height;
    [titleCell drawSelf:&inBounds inView:controlView];
    inBounds.origin.x += inBounds.size.width;
    inBounds.size.width = cellFrame->size.width - inBounds.size.width;
    inBounds.size.height = cellFrame->size.height;
    inBounds.origin.y = cellFrame->origin.y;
    if (inBounds.size.width > 0.0) {
	if (cFlags1.bordered) {
	    PSsetgray(0.0);
	    NXFrameRectWithWidth(&inBounds, 1.0);
	} else if (cFlags1.bezeled) {
	    _NXDrawWhiteBezel(&inBounds, NULL, controlView);
	}
	[super drawInside:&inBounds inView:controlView];
    }
    titleEndPoint = inBounds.origin.x;

    return self;
}


- _selectOrEdit:(const NXRect *)aRect
    inView:controlView
    target:anObject
    editor:textObj
    event:(NXEvent *)theEvent
    start:(int)selStart
    end:(int)selEnd
{
    NXRect inBounds = *aRect;
    float endPoint = (titleEndPoint >= 0.0 ? titleEndPoint : titleWidth);

    if (inBounds.origin.x < endPoint) {
	inBounds.size.width -= endPoint - inBounds.origin.x;
	inBounds.origin.x = endPoint;
    }
    return[super _selectOrEdit:&inBounds inView:controlView target:anObject
	   editor:textObj event:theEvent start:selStart end:selEnd];
}

- (BOOL)trackMouse:(NXEvent *)event inRect:(const NXRect *)aRect ofView:controlView
{
    BOOL disabled, retval;

    disabled = cFlags1.disabled;
    cFlags1.disabled = YES;
    retval = [super trackMouse:event inRect:aRect ofView:controlView];
    cFlags1.disabled = disabled;

    return retval;
}

- resetCursorRect:(const NXRect *)cellFrame inView:controlView
{
    NXSize size;
    NXRect inBounds;

    if (cFlags1.disabled || ![self isSelectable]) return self;

    inBounds = *cellFrame;
    [titleCell calcCellSize:&size];

    inBounds.size.width = MIN(inBounds.size.width, titleWidth < 0.0 ? size.width : titleWidth);
    inBounds.origin.y += floor((inBounds.size.height - size.height) / 2.0);
    inBounds.size.height = size.height;
    inBounds.origin.x += inBounds.size.width;
    inBounds.size.width = cellFrame->size.width - inBounds.size.width;
    inBounds.size.height = cellFrame->size.height;
    inBounds.origin.y = cellFrame->origin.y;
    if (inBounds.size.width > 0.0) [super resetCursorRect:&inBounds inView:controlView];
    titleEndPoint = inBounds.origin.x;

    return self;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "f@", &titleWidth, &titleCell);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadTypes(stream, "f@", &titleWidth, &titleCell);
    titleEndPoint = -1.0;
    return self;
}


@end

/*
  
Modifications (since 0.8):
  
11/21/88 pah	removed support for cFlags1.shared
11/23/88 pah	got rid of _getInsideBounds:
  
0.82
----
 1/13/89 pah	override trackMouse: so it does nothing
 1/25/89 trey	setTitle: takes a const param
 1/27/89 bs	added read: write:
 2/05/89 bgy	added the following cursor tracking method:
		    - resetCursorRect:inView:
		 This method sets up an ibeam cursor.

0.91
----
 5/27/89 pah	move cursor rect stuff from Control into the Cells

0.93
----
 6/15/89 pah	make title return const char *

*/





/*
	Box.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
	Author: Jean-Marie Hullot
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Box_Private.h"
#import "View_Private.h"
#import "Application.h"
#import "Cell.h"
#import "Font.h"
#import "Text.h"
#import <dpsclient/wraps.h>
#import <objc/List.h>
#import <math.h>
#import <zone.h>

#define BEZELOFFSET	3.0
#define RIDGEOFFSET	2.0
#define BOXOFFSET	1.0
#define NOBORDEROFFSET	0.0

#define DFLTOFFSET	5.0
#define MINEDGE		10.0

@implementation Box:View

+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}

- initFrame:(const NXRect *)frameRect
{
    id fontObj;
    NXZone *zone = [self zone];

    [super initFrame:frameRect];
    cell = [[Cell allocFromZone:zone] initTextCell:NULL];
    [cell setStringValueNoCopy:"Title"];
    [cell setAlignment:NX_CENTERED];
    bFlags.borderType = NX_RIDGE;
    bFlags.titlePosition = NX_ATTOP;
    fontObj = [Font newFont:NXSystemFont size:12.0];
    [cell setFont:fontObj];
    offsets.width = offsets.height = DFLTOFFSET;
    [self _tile];
    [self setContentView:[[View allocFromZone:zone] init]];
    [self setOpaque:YES];

    return self;
}

#define DIVIDER	10

static void
fromBoundsRect(Box *self,
	       NXRect *boundsRect, NXRect *titleRect,
	       NXRect *borderRect, NXRect *contentRect)
{
    NXSize              cellSize;
    NXCoord             titleOffset, offset = 0.0;
    NXSize              offsets;


    offsets = self->offsets;
    [self->cell calcCellSize:&cellSize];
    titleOffset = floor(cellSize.height / DIVIDER);
    switch (self->bFlags.borderType) {
    case NX_NOBORDER:
	offset = NOBORDEROFFSET;
	break;
    case NX_LINE:
	offset = BOXOFFSET;
	break;
    case NX_BEZEL:
	offset = BEZELOFFSET;
	break;
    case NX_RIDGE:
	offset = RIDGEOFFSET;
	break;
    }
    if (cellSize.width > NX_WIDTH(boundsRect))
	cellSize.width = NX_WIDTH(boundsRect);
    if (cellSize.height > NX_HEIGHT(boundsRect))
	cellSize.height = NX_HEIGHT(boundsRect);
    titleRect->size = cellSize;
    NX_X(titleRect) =
      NX_X(boundsRect)
      + floor((NX_WIDTH(boundsRect) - cellSize.width) / 2);
    NX_X(borderRect) = NX_X(boundsRect);
    NX_WIDTH(borderRect) = NX_WIDTH(boundsRect);
    NX_X(contentRect) = NX_X(boundsRect) + offset + offsets.width;
    NX_WIDTH(contentRect) = NX_WIDTH(boundsRect)
      - (2 * (offset + offsets.width));
    if (NX_WIDTH(contentRect) < 0.0)
	NX_WIDTH(contentRect) = 0.0;
    switch (self->bFlags.titlePosition) {
    case NX_NOTITLE:
	titleRect->origin.x = titleRect->origin.y = 0.0;
	titleRect->size.width = titleRect->size.height = 0.0;
	*borderRect = *boundsRect;
	NX_Y(contentRect) = NX_Y(boundsRect) + offset + offsets.height;
	NX_HEIGHT(contentRect) = NX_HEIGHT(boundsRect)
	  - (2 * (offset + offsets.height));
	break;
    case NX_ABOVETOP:
	NX_Y(titleRect) = NX_MAXY(boundsRect) - cellSize.height;
	NX_Y(borderRect) = NX_Y(boundsRect);
	NX_HEIGHT(borderRect)
	  = NX_HEIGHT(boundsRect)
	  - (cellSize.height + titleOffset);
	NX_Y(contentRect) = NX_Y(borderRect)
	  + (offset + offsets.height);
	NX_HEIGHT(contentRect) = NX_HEIGHT(borderRect)
	  - (2 * (offset + offsets.height));
	break;
    case NX_ATTOP:
	NX_Y(titleRect) = NX_MAXY(boundsRect) - cellSize.height;
	NX_Y(borderRect) = NX_Y(boundsRect);
	NX_HEIGHT(borderRect)
	  = NX_HEIGHT(boundsRect) - floor(cellSize.height / 2);
	NX_Y(contentRect) = NX_Y(borderRect)
	  + (offset + offsets.height);
	NX_HEIGHT(contentRect) =
	  NX_HEIGHT(boundsRect)
	  - (offset + offsets.height
	     + cellSize.height + titleOffset);
	break;
    case NX_BELOWTOP:
	NX_Y(titleRect) = NX_MAXY(boundsRect)
	  - (offset + titleOffset
	     + cellSize.height);
	*borderRect = *boundsRect;
	NX_Y(contentRect) = NX_Y(borderRect)
	  + (offset + offsets.height);
	NX_HEIGHT(contentRect) =
	  NX_HEIGHT(boundsRect)
	  - (offset + cellSize.height + (2 * titleOffset)
	     + offsets.height + offset);
	break;
    case NX_ABOVEBOTTOM:
	NX_Y(titleRect) = NX_Y(boundsRect)
	  + offset + titleOffset;
	*borderRect = *boundsRect;
	NX_Y(contentRect) = NX_MAXY(titleRect) + titleOffset;
	NX_HEIGHT(contentRect) =
	  NX_HEIGHT(boundsRect)
	  - (offset + offsets.height
	     + offset + cellSize.height + (2 * titleOffset));
	break;
    case NX_ATBOTTOM:
	NX_Y(titleRect) = NX_Y(boundsRect);
	NX_Y(borderRect) = NX_Y(boundsRect) + floor(cellSize.height / 2);
	NX_HEIGHT(borderRect) =
	  NX_HEIGHT(boundsRect) - floor(cellSize.height / 2);
	NX_Y(contentRect) = NX_Y(boundsRect)
	  + cellSize.height + titleOffset;
	NX_HEIGHT(contentRect) =
	  NX_HEIGHT(boundsRect)
	  - (offset + offsets.height
	     + cellSize.height + titleOffset);
	break;
    case NX_BELOWBOTTOM:
	NX_Y(titleRect) = NX_Y(boundsRect);
	NX_Y(borderRect) = NX_Y(boundsRect)
	  + cellSize.height + titleOffset;
	NX_HEIGHT(borderRect) =
	  NX_HEIGHT(boundsRect)
	  - (cellSize.height + titleOffset);
	NX_Y(contentRect) = NX_Y(borderRect)
	  + (offset + offsets.height);
	NX_HEIGHT(contentRect) = NX_HEIGHT(borderRect)
	  - (2 * (offset + offsets.height));
	break;
    }
    if (NX_HEIGHT(borderRect) < 0.0)
	NX_HEIGHT(borderRect) = 0.0;
    if (NX_HEIGHT(contentRect) < 0.0)
	NX_HEIGHT(contentRect) = 0.0;
}

- _tile
{
    NXRect              contentRect;

    fromBoundsRect(self, &self->bounds,
		   &self->titleRect,
		   &self->borderRect,
		   &contentRect);
    [self->contentView setFrame:&contentRect];

    return self;
}

static void
frameFromContentFrame(Box *self, NXRect *frameRect, const NXRect *contentFrame)
{
    NXCoord             offset = 0.0, titleHeight, titleOffset;
    NXSize              cellSize;


    [self->cell calcCellSize:&cellSize];
    titleHeight = cellSize.height;
    titleOffset = floor(titleHeight / DIVIDER);
    switch (self->bFlags.borderType) {
    case NX_NOBORDER:
	offset = NOBORDEROFFSET;
	break;
    case NX_LINE:
	offset = BOXOFFSET;
	break;
    case NX_BEZEL:
	offset = BEZELOFFSET;
	break;
    case NX_RIDGE:
	offset = RIDGEOFFSET;
	break;
    }
    NX_X(frameRect) =
      NX_X(contentFrame) - (offset + self->offsets.width);
    NX_WIDTH(frameRect) =
      NX_WIDTH(contentFrame) + (2 * (offset + self->offsets.width));
    switch (self->bFlags.titlePosition) {
    case NX_NOTITLE:
	NX_Y(frameRect) =
	  NX_Y(contentFrame) - (offset + self->offsets.height);
	NX_HEIGHT(frameRect) =
	  NX_HEIGHT(contentFrame)
	  + (2 * (offset + self->offsets.height));
	break;
    case NX_ABOVETOP:
	NX_Y(frameRect) =
	  NX_Y(contentFrame) - (offset + self->offsets.height);
	NX_HEIGHT(frameRect) =
	  NX_HEIGHT(contentFrame)
	  + (2 * (offset + self->offsets.height))
	  + titleHeight
	  + titleOffset;
	break;
    case NX_ATTOP:
	NX_Y(frameRect) =
	  NX_Y(contentFrame) - (offset + self->offsets.height);
	NX_HEIGHT(frameRect) =
	  NX_HEIGHT(contentFrame)
	  + (offset + self->offsets.height)
	  + titleHeight
	  + titleOffset;
	break;
    case NX_BELOWTOP:
	NX_Y(frameRect) =
	  NX_Y(contentFrame) - (offset + self->offsets.height);
	NX_HEIGHT(frameRect) =
	  NX_HEIGHT(contentFrame)
	  + (offset + self->offsets.height)
	  + titleHeight
	  + (2 * titleOffset)
	  + offset;
	break;
    case NX_ABOVEBOTTOM:
	NX_Y(frameRect) =
	  NX_Y(contentFrame)
	  - ((2 * titleOffset) + titleHeight + offset);
	NX_HEIGHT(frameRect) =
	  NX_HEIGHT(contentFrame)
	  + (offset + self->offsets.height)
	  + (2 * titleOffset)
	  + titleHeight
	  + offset;
	break;
    case NX_ATBOTTOM:
	NX_Y(frameRect) =
	  NX_Y(contentFrame)
	  - (titleOffset + titleHeight);
	NX_HEIGHT(frameRect) =
	  NX_HEIGHT(contentFrame)
	  + (offset + self->offsets.height)
	  + titleHeight
	  + titleOffset;
	break;
    case NX_BELOWBOTTOM:
	NX_Y(frameRect) =
	  NX_Y(contentFrame)
	  - ((offset + self->offsets.height)
	     + titleOffset + titleHeight);
	NX_HEIGHT(frameRect) =
	  NX_HEIGHT(contentFrame)
	  + (2 * (offset + self->offsets.height))
	  + titleOffset
	  + titleHeight;
	break;
    }
}


- awake
{
    [cell awake];
    [self _tile];
    return nil;
}

- free
{
    [cell free];
    return[super free];
}

- (int)borderType
{
    return bFlags.borderType;
}

- (int)titlePosition
{
    return bFlags.titlePosition;
}

- setBorderType:(int)aType
{
    bFlags.borderType = aType;
    [self _tile];
    return self;
}

- setTitlePosition:(int)aPosition
{
    bFlags.titlePosition = aPosition;
    [self _tile];
    return self;
}

- (const char *)title
{
    return[cell stringValue];
}

- setTitle:(const char *)aString
{
    [cell setStringValue:aString];
    [self _tile];
    return self;
}

- cell
{
    return cell;
}

- font
{
    return[cell font];
}

- setFont:fontObj
{
    [cell setFont:fontObj];
    [self _tile];
    return self;
}

- getOffsets:(NXSize *)aSize
{
    aSize->width = offsets.width;
    aSize->height = offsets.height;
    return self;
}

- setOffsets:(NXCoord)w :(NXCoord)h
{
    offsets.width = w;
    offsets.height = h;
    return self;
}

- sizeTo:(NXCoord)width :(NXCoord)height
{
    [super sizeTo:width :height];
    [self _tile];
    return self;
}

static void
calcBestContentBounds(Box *self, NXRect *theBounds)
{
    id                  theSubviews, *itemPtr;
    int                 i, size;
    NXRect              tmpRect;
    register NXRect    *itemFrame;

    itemFrame = &tmpRect;
    theSubviews = [self->contentView subviews];
    if (size = [theSubviews count]) {
	itemPtr = (id *)NX_ADDRESS(theSubviews);
	[*itemPtr getFrame:theBounds];
	itemPtr++;
	if (size > 1) {
	    for (i = 1; i < size; i++) {
		[*itemPtr getFrame:itemFrame];
		NXUnionRect(itemFrame, theBounds);
		itemPtr++;
	    }
	}
    } else
	NXSetRect(theBounds, 0.0, 0.0, 0.0, 0.0);
    [self->contentView convertRectToSuperview:theBounds];
}

- sizeToFit
{
    NXRect              oldContentBounds, contentBounds;
    NXSize              cellSize;
    int                 i, size;
    id                  contentSubviews;

    [contentView getFrame:&oldContentBounds];
    if ([contentView respondsTo:@selector(sizeToFit)]) {
	[contentView sizeToFit];
	[contentView getFrame:&contentBounds];
    } else {
	calcBestContentBounds(self, &contentBounds);
	contentSubviews = [contentView subviews];
	size = [contentSubviews count];
	for (i = 0; i < size; i++) {
	    [[contentSubviews objectAt:i]
	     moveBy:oldContentBounds.origin.x - contentBounds.origin.x
	     :oldContentBounds.origin.y - contentBounds.origin.y];
	}
    }
    [self convertRectToSuperview:&contentBounds];
    [self setFrameFromContentFrame:&contentBounds];
    [cell calcCellSize:&cellSize];
    if (cellSize.width + (2 * MINEDGE) > frame.size.width) {
	NXCoord             delta;

	delta = cellSize.width + (2 * MINEDGE) - frame.size.width;
	[self sizeBy:delta :0.0];
    }

    return self;
}

- setFrameFromContentFrame:(const NXRect *)contentFrame
{
    NXRect nframe;

    frameFromContentFrame(self, &nframe, contentFrame);
    [self setFrame:&nframe];

    return self;
}

- contentView
{
    return contentView;
}

- setContentView:aView
{
    id oldContent;

    oldContent = contentView;
    [contentView removeFromSuperview];
    contentView = aView;
    [super addSubview:contentView];
    [self _tile];

    return oldContent;
}

- addSubview:aView
{
    [contentView addSubview:aView];
    return self;
}


- drawSelf:(const NXRect *)rects :(int)rectCount
{
    int			count;
    NXRect              aRect;
    const NXRect       *rlist;

    if (bFlags.borderType == NX_RIDGE) {
	if (frame.size.height == RIDGEOFFSET) {
	    PSsetgray(NX_WHITE);
	    NXSetRect(&aRect, bounds.origin.x, bounds.origin.y,
		      bounds.size.width, 1.0);
	    NXFrameRectWithWidth(&aRect, 1.0);
	    PSsetgray(NX_DKGRAY);
	    aRect.origin.y++;
	    NXFrameRectWithWidth(&aRect, 1.0);
	    return self;
	} else if (frame.size.width == RIDGEOFFSET) {
	    PSsetgray(NX_DKGRAY);
	    NXSetRect(&aRect, bounds.origin.x, bounds.origin.y,
		      1.0, bounds.size.height);
	    NXFrameRectWithWidth(&aRect, 1.0);
	    PSsetgray(NX_WHITE);
	    aRect.origin.x++;
	    NXFrameRectWithWidth(&aRect, 1.0);
	    return self;
	}
    }

    if (!bFlags.transparent) {
	PSsetgray(NX_LTGRAY);
	NXRectFillList(rects, rectCount);
    }

    switch (bFlags.borderType) {
    case NX_NOBORDER:
	break;
    case NX_LINE:
	PSsetgray(NX_BLACK);
	NXFrameRectWithWidth(&borderRect, 1.0);
	break;
    case NX_BEZEL:
	rlist = rects;
	count = rectCount;
	while (count--) {
	    _NXDrawGrayBezel(&borderRect, rlist++, self);
	}
	break;
    case NX_RIDGE:
	rlist = rects;
	count = rectCount;
	while (count--) {
	    switch (bFlags.titlePosition) {
	    case NX_ATTOP:
		if ([self _canOptimizeDrawing]) {
		    NXRect r;
		    _NXDrawGrooveExceptTop(&borderRect, rlist++, self);
		    r.origin = borderRect.origin;
		    if (![self isFlipped]) r.origin.y += borderRect.size.height - 2.0;
		    r.size.height = 2.0;
		    r.size.width = 1.0;
		    if (![self _optimizedRectFill:&r gray:NX_DKGRAY]) {
			PSsetgray(NX_DKGRAY);
			NXRectFill(&r);
		    }
		    r.size.height = 1.0;
		    if (![self isFlipped]) r.origin.y += 1.0;
		    r.size.width = ((borderRect.size.width - titleRect.size.width) / 2.0);
		    if (![self _optimizedRectFill:&r gray:NX_DKGRAY]) {
			PSsetgray(NX_DKGRAY);
			NXRectFill(&r);
		    }
		    r.origin.y += [self isFlipped] ? 1.0 : -1.0;
		    r.origin.x += 1.0;
		    r.size.width -= 1.0;
		    if (![self _optimizedRectFill:&r gray:NX_WHITE]) {
			PSsetgray(NX_WHITE);
			NXRectFill(&r);
		    }
		    r.origin.x += r.size.width + titleRect.size.width - 1.0;
		    r.size.width = borderRect.origin.x + borderRect.size.width - r.origin.x;
		    if (![self _optimizedRectFill:&r gray:NX_WHITE]) {
			PSsetgray(NX_WHITE);
			NXRectFill(&r);
		    }
		    r.origin.y -= [self isFlipped] ? 1.0 : -1.0;
		    if (![self _optimizedRectFill:&r gray:NX_DKGRAY]) {
			PSsetgray(NX_DKGRAY);
			NXRectFill(&r);
		    }
		} else {
		    NXDrawGroove(&borderRect, rlist++);
		}
		break;
	    case NX_ATBOTTOM:
		NXDrawGroove(&borderRect, rlist++);
		break;
	    default:
		_NXDrawGroove(&borderRect, rlist++, self);
	    }
	}
	break;
    }

    switch (bFlags.titlePosition) {
    case NX_NOTITLE:
	return self;
    case NX_ATTOP:
    case NX_ATBOTTOM:
	if (rectCount != 1 || NXIntersectsRect(rects, &titleRect)) {
	    PSsetgray(NX_LTGRAY);
	    NXRectFill(&titleRect);
	}
	break;
    }

    [cell drawSelf:&titleRect inView:self];

    return self;
}

- _setBackgroundTransparent:(BOOL)flag
{
    bFlags.transparent = flag;
    return self;
}

- (BOOL)_backgroundTransparent;
{
    return bFlags.transparent;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteSize(stream, &offsets);
    NXWriteTypes(stream, "@@s", &cell, &contentView, &bFlags);
    return self;
}

- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadSize(stream, &offsets);
    NXReadTypes(stream, "@@s", &cell, &contentView, &bFlags);
    return self;
}

@end

/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the List object;
 1/18/88 wrp	moved the function NXDrawRidge from Box.m to graphicsOps.m
		  and changed declaration to void.
 1/25/89 trey	setTitle: takes a const param
 1/27/89 bs	added read: write:
 2/08/89 pah	made small ridged boxes work

0.91
----
 5/10/89 pah	make default title font be the unbold system font

0.93
----
 6/15/89 pah	make title return const char *
 6/17/89 pah	add extra clip rect to NXDrawGrayBezel call
 7/05/89 pah	add extra clip rect to NXDrawRidge (now NXDrawGroove)

0.96
----
 7/20/89 pah	fix drawSelf:: so that it does obscure subviews when
		 rects is different than the bounds

80
--
 4/9/90 aozer	Added _setBackgroundTransparent: for Keith.

*/


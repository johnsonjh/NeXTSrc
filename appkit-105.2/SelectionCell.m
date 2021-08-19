/*
	SelectionCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "SelectionCell.h"
#import "Font.h"
#import "NXImage.h"
#import "Application.h"
#import "Text.h"
#import "nextstd.h"
#import <dpsclient/dpsNeXT.h>
#import <dpsclient/wraps.h>
#import <math.h>
#import <zone.h>

#define MARGIN 2.0

static id menuArrow = nil;
static id menuArrowH = nil;

@implementation SelectionCell:Cell

static void initClassVars()
{
    if (!menuArrow) {
	menuArrow = [NXImage findImageNamed:"NXmenuArrow"];
	menuArrowH = [NXImage findImageNamed:"NXmenuArrowH"];
    }
}


+ new
{
    return [self newTextCell];
}

- init
{
    return [[self initTextCell:NULL] setStringValueNoCopy:"ListItem"];
}

+ newTextCell
{
    return [[self newTextCell:NULL] setStringValueNoCopy:"ListItem"];
}

+ newTextCell:(const char *)aString
{
    return [[self allocFromZone:NXDefaultMallocZone()] initTextCell:aString];
}

- initTextCell:(const char *)aString
{
    [super initTextCell:aString];
    cFlags1.alignment = NX_LEFTALIGNED;
    cFlags2._isLeaf = YES;
    cFlags2.noWrap = YES;
    if (!menuArrow) initClassVars();
    return self;
}


- awake
{
    if (!menuArrow) initClassVars();
    return[super awake];
}


- (BOOL)isOpaque
{
    return YES;
}


- setLeaf:(BOOL)flag
{
    cFlags2._isLeaf = flag ? YES : NO;
    return self;
}


- (BOOL)isLeaf
{
    return cFlags2._isLeaf;
}


- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect
{
    NXSize iconSize;

    [super calcCellSize:theSize inRect:aRect];
    [menuArrow getSize:&iconSize];
    theSize->width += MARGIN + iconSize.width;
    theSize->height = MAX(iconSize.height + (cFlags1.bordered ? 2.0 : 0.0), theSize->height);

    return self;
}

- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    return[self drawInside:cellFrame inView:controlView];
}


- _drawInside:(const NXRect *)cellFrame inView:controlView
{
    NXRect aRect;
    NXSize iconSize;

    aRect = *cellFrame;
    [menuArrow getSize:&iconSize];
    aRect.size.width -= MARGIN + iconSize.width;
    _NXDrawTextCell(self, controlView, &aRect, YES);
    if (![self isLeaf]) {
	aRect.origin.x += aRect.size.width;
	aRect.origin.y += floor((aRect.size.height + ([controlView isFlipped] ? iconSize.height : -iconSize.height)) / 2);
	[((cFlags1.state || cFlags1.highlighted) ? menuArrowH : menuArrow)
	 composite :NX_COPY toPoint:&aRect.origin];
    }
    if (cFlags1.bordered && (cFlags1.state || cFlags1.highlighted)) {
	aRect = *cellFrame;
	PSsetgray(NX_DKGRAY);
	aRect.size.height = 1.0;
	NXRectFill(&aRect);
	aRect.origin.y += cellFrame->size.height - 1.0;
	NXRectFill(&aRect);
    }

    return self;
}


- drawInside:(const NXRect *)cellFrame inView:controlView
{
    PSsetgray((cFlags1.state || cFlags1.highlighted) ? NX_WHITE : NX_LTGRAY);
    NXRectFill(cellFrame);
    return [self _drawInside:cellFrame inView:controlView];
}

- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag
{
    if ((cFlags1.highlighted && !flag) || (!cFlags1.highlighted && flag)) {
	cFlags1.highlighted = flag ? YES : NO;
	[self drawInside:cellFrame inView:controlView];
    }
    return self;
}

@end

/*
  
Modifications (since 0.8):
  
 1/03/89 pah	renamed to SelectionCell for 0.82

0.91
----
 5/19/89 trey	minimized static data

84
--
 5/7/90 aozer	Converted from Bitmap to NXImage.

*/


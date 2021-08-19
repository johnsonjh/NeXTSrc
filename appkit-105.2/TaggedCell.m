/*
	TaggedCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
  
	DEFINED IN: The Application Kit
	HEADER FILES: TaggedCell.h
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "TaggedCell.h"
#import "appkitPrivate.h"
#import "Text.h"
#import <dpsclient/wraps.h>
#import <zone.h>

@implementation TaggedCell

+ new
{
    return [self newTextCell];
}

- init
{
    return [[self initTextCell:NULL] setStringValueNoCopy:"TaggedItem" shouldFree:NO];
}

+ newTextCell
{
    return [[self newTextCell:NULL] setStringValueNoCopy:"TaggedItem" shouldFree:NO];
}

+ newTextCell:(const char *)aString
{
    return [[self allocFromZone:NXDefaultMallocZone()] initTextCell:aString];
}

- initTextCell:(const char *)aString
{
    [super initTextCell:aString];
    cFlags1.alignment = NX_LEFTALIGNED;
    cFlags2.noWrap = YES;
    return self;
}

- (BOOL)isOpaque
{
    return YES;
}

- setTag:(int)anInt
{
    tag = anInt;
    return self;
}

- (int)tag
{
    return tag;
}

- drawInside:(const NXRect *)cellFrame inView:controlView
{
    NXRect aRect;

    if (cFlags1.state || cFlags1.highlighted) {
	PSsetgray(NX_WHITE);
	NXRectFill(cellFrame);
	cFlags1.entryType = 1;
    } else if (cFlags1.entryType) {
	PSsetgray(NX_LTGRAY);
	NXRectFill(cellFrame);
	cFlags1.entryType = 0;
    }
    _NXDrawTextCell(self, controlView, cellFrame, YES);
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
  
 2/14/89 pah	new for 0.82

0.91
----
 5/10/89 pah	optimize drawing
  
*/





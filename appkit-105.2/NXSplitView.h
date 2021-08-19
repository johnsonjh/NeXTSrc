/*
	NXSplitView.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "View.h"

@interface NXSplitView : View
{
    id                  delegate;
}

- initFrame:(const NXRect *)frameRect;

- delegate;
- setDelegate:anObject;
- adjustSubviews;
- (NXCoord)dividerHeight;
- drawDivider:(const NXRect *)aRect;

- mouseDown:(NXEvent *)theEvent;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- resizeSubviews:(const NXSize *)oldSize;
- setAutoresizeSubviews:(BOOL)flag;
- (BOOL)acceptsFirstMouse;

@end

@interface Object(NXSplitViewDelegate)
- splitView:sender resizeSubviews:(const NXSize *)oldSize;
- splitView:sender getMinY:(NXCoord *)minY maxY:(NXCoord *)maxY ofSubviewAt:(int)offset;
- splitViewDidResizeSubviews:sender;
@end

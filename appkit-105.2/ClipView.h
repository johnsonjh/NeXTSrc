/*
	ClipView.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "View.h"
#import "color.h"

@interface ClipView : View
{
    float               backgroundGray;
    id                  docView;
    NXRect              _docRect;
    id                  cursor;
    void               *_private;
    struct __clFlags {
	unsigned int        isGraySet:1;
	unsigned int        _RESERVED:11;
	unsigned int        _onlyUncovered:1;
	unsigned int        _reflectScroll:1;
	unsigned int        _usedByCell:1;
	unsigned int        _scrollClipTo:1;
    }                   _clFlags;
}

+ initialize;

- initFrame:(const NXRect *)frameRect;
- setBackgroundGray:(float)value;
- (float)backgroundGray;
- setBackgroundColor:(NXColor)color;
- (NXColor)backgroundColor;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- setDocView:aView;
- docView;
- getDocRect:(NXRect *)aRect;
- setDocCursor:anObj;
- resetCursorRects;
- getDocVisibleRect:(NXRect *)aRect;
- descendantFrameChanged:sender;
- descendantFlipped:sender;
- setCopyOnScroll:(BOOL)flag;
- setDisplayOnScroll:(BOOL)flag;
- autoscroll:(NXEvent *)theEvent;
- constrainScroll:(NXPoint *)newOrigin;
- rawScroll:(const NXPoint *)newOrigin;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;
- free;

- rotate:(NXCoord)angle;
- rotateTo:(NXCoord)angle;
- setDrawRotation:(NXCoord)angle;

- awake;
- moveTo:(NXCoord)x :(NXCoord)y;
- sizeTo:(NXCoord)width :(NXCoord)height;
- setDrawOrigin:(NXCoord)x :(NXCoord)y;
- setDrawSize:(NXCoord)width :(NXCoord)height;
- translate:(NXCoord)x :(NXCoord)y;
- scale:(NXCoord)x :(NXCoord)y;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end

@interface View(ClipViewSuperview)
- reflectScroll:aClipView;
- scrollClip:aClipView to:(const NXPoint *)aPoint;
@end

/*
	ScrollView.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Box.h"
#import "color.h"

@interface ScrollView : View
{
    id                  vScroller;
    id                  hScroller;
    id                  contentView;
    float               pageContext;
    float               lineAmount;
    struct __sFlags {
	unsigned int        vScrollerRequired:1;
	unsigned int        hScrollerRequired:1;
	unsigned int        vScrollerStatus:1;
	unsigned int        hScrollerStatus:1;
	unsigned int        noDynamicScrolling:1;
	unsigned int        borderType:2;
	unsigned int        rulerInstalled:1;
	unsigned int        _RESERVED:8;
    }                   _sFlags;
    id                  _ruler;
}

+ getFrameSize:(NXSize *)fSize forContentSize:(const NXSize *)cSize horizScroller:(BOOL)hFlag vertScroller:(BOOL)vFlag borderType:(int)aType;
+ getContentSize:(NXSize *)cSize forFrameSize:(const NXSize *)fSize horizScroller:(BOOL)hFlag vertScroller:(BOOL)vFlag borderType:(int)aType;

- initFrame:(const NXRect *)frameRect;
- getDocVisibleRect:(NXRect *)aRect;
- getContentSize:(NXSize *)contentViewSize;
- resizeSubviews:(const NXSize *)oldSize;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- setDocView:aView;
- docView;
- setDocCursor:anObj;
- (int)borderType;
- setBorderType:(int)aType;
- setBackgroundGray:(float)value;
- (float)backgroundGray;
- setBackgroundColor:(NXColor)color;
- (NXColor)backgroundColor;
- setVertScrollerRequired:(BOOL)flag;
- setHorizScrollerRequired:(BOOL)flag;
- vertScroller;
- horizScroller;
- setVertScroller:anObject;
- setHorizScroller:anObject;
- setLineScroll:(float)value;
- setPageScroll:(float)value;
- setCopyOnScroll:(BOOL)flag;
- setDisplayOnScroll:(BOOL)flag;
- setDynamicScrolling:(BOOL)flag;
- tile;
- reflectScroll:cView;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;
+ new;

@end

@interface ScrollView(Ruler)
- toggleRuler:sender;
- ruler;
@end


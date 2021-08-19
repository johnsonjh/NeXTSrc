/*

	FrameView.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#import "View.h"

@interface FrameView :View
{
    id		    titleCell;
    id              closeButton;	
    id              iconifyButton;	
    struct _frFlags{
	unsigned int    style:4;	


	unsigned int    buttonMask:3;	

	unsigned int    RESERVED:9;
    }               frFlags;	
}

+ newFrame:(const NXRect *) theFrame style:(int)aStyle buttonMask:(int)theMask owner:theWindow;
+ (NXCoord)minFrameWidth:(const char *)theTitle forStyle:(int)aStyle buttonMask:(int)theMask;
+ getFrameRect:(NXRect *)fRect forContentRect:(const NXRect *)cRect style:(int)aStyle;
+ getContentRect:(NXRect *)cRect forFrameRect:(const NXRect *)fRect style:(int)aStyle;

- initFrame:(const NXRect *)theFrame style:(int)aStyle buttonMask:(int)theMask owner:theWindow;
- (BOOL)_setMask:(int)theMask;
- _commonFrameViewInit;
- getCloseButton;
- getIconifyButton;
- doIconify:theSender;
- free;
- setTitle:(const char *)aString;
- doClose:button;
- _resize:(NXEvent *) theEvent;
- (const char *)title;
- setCloseTarget:aTarget;
- setCloseAction:(SEL) anAction;
- becomeKeyWindow;
- resignKeyWindow;
- _focusDown:(BOOL)clipNeeded;
- displayBorder;
- _getTitleRect:(NXRect *)tRect;
- _displayTitle;
- drawSelf:(const NXRect *) clips :(int)clipCount;
- _drawFrame:(const NXRect *) clips :(int)clipCount;
- _drawMiniWorld:(const NXRect *) clips :(int)clipCount;
- _drawMenuFrame:(const NXRect *) clips :(int)clipCount;
- _drawTitledFrame:(const NXRect *) clips :(int)clipCount;
- (int)_inResize:(const NXPoint *)thePoint;
- mouseDown:(NXEvent *) theEvent;
- mouseUp:(NXEvent *) theEvent;
- rightMouseDown:(NXEvent *) theEvent;
- rightMouseUp:(NXEvent *) theEvent;
- sizeTo:(NXCoord) width :(NXCoord) height;
- awake;
- _setDragMargins:(BOOL)left :(BOOL)right :(BOOL)top;
- tile;
- moveTo:(NXCoord) x :(NXCoord) y;
- (BOOL) acceptsFirstMouse;
- read:(NXTypedStream *) s;
- write:(NXTypedStream *) s;
- _setCloseEnabled:(BOOL)aFlag andDisplay:(BOOL)display;
@end



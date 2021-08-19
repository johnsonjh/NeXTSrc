/*
	View.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Window.h"
#import "screens.h"

/* Autosizing parameters */

#define	NX_NOTSIZABLE		(0)
#define	NX_MINXMARGINSIZABLE	(1)
#define	NX_WIDTHSIZABLE		(2)
#define	NX_MAXXMARGINSIZABLE	(4)
#define	NX_MINYMARGINSIZABLE	(8)
#define	NX_HEIGHTSIZABLE	(16)
#define	NX_MAXYMARGINSIZABLE	(32)

/* Are we drawing, printing, or copying PostScript to the scrap? */

extern short NXDrawingStatus;

/* NXDrawingStatus values */

#define NX_DRAWING	1	/* we're drawing */
#define NX_PRINTING	2	/* we're printing */
#define NX_COPYING	3	/* we're copying to the scrap */

extern BOOL NXScreenDump;	/* Do we draw selection while printing? */

@interface View : Responder
{
    NXRect              frame;
    NXRect              bounds;
    id                  superview;
    id                  subviews;
    id                  window;
    struct __vFlags {
	unsigned int        noClip:1;
	unsigned int        translatedDraw:1;
	unsigned int        drawInSuperview:1;
	unsigned int        alreadyFlipped:1;
	unsigned int        needsFlipped:1;
	unsigned int        rotatedFromBase:1;
	unsigned int        rotatedOrScaledFromBase:1;
	unsigned int        opaque:1;
	unsigned int        disableAutodisplay:1;
	unsigned int        needsDisplay:1;
	unsigned int        validGState:1;
	unsigned int        newGState:1;
	unsigned int        _RESERVED:2;
	unsigned int        _noVerticalAutosizing:1;
	unsigned int        _hasDirtySubview:1;
    }                   vFlags;
    struct ___vFlags {
	unsigned int        autosizing:6;
	unsigned int        autoresizeSubviews:1;
	unsigned int        notifyWhenFlipped:1;
	unsigned int        ancestorNotifyWasEnabled:1;
	unsigned int        needsAncestorNotify:1;
	unsigned int        notifyToInitGState:1;
	unsigned int        wantsGState:1;
	unsigned int        noCopyOnScroll:1;
	unsigned int        noDisplayOnScroll:1;
	unsigned int        specialClip:1;
	unsigned int        mark:1;
    }                   _vFlags;
    int                 _gState;
    id                  _frameMatrix;
    id                  _drawMatrix;
    unsigned int        _reservedView1;
    unsigned int        _reservedView2;
}

- init;
- initFrame:(const NXRect *)frameRect;
- awake;
- free;
- window;
- superview;
- subviews;
- (BOOL)isDescendantOf:aView;
- findAncestorSharedWith:aView;
- opaqueAncestor;
- addSubview:aView;
- addSubview:aView :(int)place relativeTo:otherView;
- windowChanged:newWindow;
- removeFromSuperview;
- replaceSubview:oldView with:newView;
- notifyAncestorWhenFrameChanged:(BOOL)flag;
- suspendNotifyAncestorWhenFrameChanged:(BOOL)flag;
- notifyWhenFlipped:(BOOL)flag;
- descendantFrameChanged:sender;
- descendantFlipped:sender;
- resizeSubviews:(const NXSize *)oldSize;
- superviewSizeChanged:(const NXSize *)oldSize;
- setAutoresizeSubviews:(BOOL)flag;
- setAutosizing:(unsigned int)mask;
- moveTo:(NXCoord)x :(NXCoord)y;
- sizeTo:(NXCoord)width :(NXCoord)height;
- setFrame:(const NXRect *)frameRect;
- rotateTo:(NXCoord)angle;
- moveBy:(NXCoord)deltaX :(NXCoord)deltaY;
- sizeBy:(NXCoord)deltaWidth :(NXCoord)deltaHeight;
- rotateBy:(NXCoord)deltaAngle;
- getFrame:(NXRect *)theRect;
- (float)frameAngle;
- setDrawOrigin:(NXCoord)x :(NXCoord)y;
- setDrawSize:(NXCoord)width :(NXCoord)height;
- setDrawRotation:(NXCoord)angle;
- translate:(NXCoord)x :(NXCoord)y;
- scale:(NXCoord)x :(NXCoord)y;
- rotate:(NXCoord)angle;
- getBounds:(NXRect *)theRect;
- (float)boundsAngle;
- setFlipped:(BOOL)flag;
- (BOOL)isFlipped;
- (BOOL)isRotatedFromBase;
- (BOOL)isRotatedOrScaledFromBase;
- setOpaque:(BOOL)flag;
- (BOOL)isOpaque;
- convertPointFromSuperview:(NXPoint *)aPoint;
- convertPointToSuperview:(NXPoint *)aPoint;
- convertRectFromSuperview:(NXRect *)aRect;
- convertRectToSuperview:(NXRect *)aRect;
- convertPoint:(NXPoint *)aPoint fromView:aView;
- convertPoint:(NXPoint *)aPoint toView:aView;
- convertSize:(NXSize *)aSize fromView:aView;
- convertSize:(NXSize *)aSize toView:aView;
- convertRect:(NXRect *)aRect fromView:aView;
- convertRect:(NXRect *)aRect toView:aView;
- centerScanRect:(NXRect *)aRect;
- (BOOL)canDraw;
- setAutodisplay:(BOOL)flag;
- (BOOL)isAutodisplay;
- setNeedsDisplay:(BOOL)flag;
- (BOOL)needsDisplay;
- update;
- drawInSuperview;
- (int)gState;
- allocateGState;
- freeGState;
- notifyToInitGState:(BOOL)flag;
- initGState;
- renewGState;
- clipToFrame:(const NXRect *)frameRect;
- (BOOL)lockFocus;
- unlockFocus;
- (BOOL)isFocusView;
- setClipping:(BOOL)flag;
- (BOOL)doesClip;
- (BOOL)getVisibleRect:(NXRect *)theRect;
- displayIfNeeded;
- display:(const NXRect *)rects :(int)rectCount :(BOOL)clipFlag;
- displayFromOpaqueAncestor:(const NXRect *)rects :(int)rectCount :(BOOL)clipFlag;
- display:(const NXRect *)rects :(int)rectCount;
- display;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- (float)backgroundGray;
- scrollPoint:(const NXPoint *)aPoint;
- scrollRectToVisible:(const NXRect *)aRect;
- autoscroll:(NXEvent *)theEvent;
- adjustScroll:(NXRect *)newVisible;
- (BOOL)calcUpdateRects:(NXRect *)rects :(int *)rectCount :(NXRect *)enclRect :(NXRect *)goodRect;
- invalidate:(const NXRect *)rects :(int)rectCount;
- scrollRect:(const NXRect *)aRect by:(const NXPoint *)delta;
- hitTest:(NXPoint *)aPoint;
- (BOOL)mouse:(NXPoint *)aPoint inRect:(NXRect *)aRect;
- findViewWithTag:(int)aTag;
- (int)tag;
- (BOOL)performKeyEquivalent:(NXEvent *)theEvent;
- (BOOL)acceptsFirstMouse;
- copyPSCodeInside:(const NXRect *)rect to:(NXStream *)stream;
- printPSCode:sender;
- faxPSCode:sender;
- (BOOL)knowsPagesFirst:(int *)firstPageNum last:(int *)lastPageNum;
- openSpoolFile:(char *)filename;
- beginPSOutput;
- beginPrologueBBox:(const NXRect *)boundingBox creationDate:(const char *)dateCreated createdBy:(const char *)anApplication fonts:(const char *)fontNames forWhom:(const char *)user pages:(int)numPages title:(const char *)aTitle;
- endHeaderComments;
- endPrologue;
- beginSetup;
- endSetup;
- beginPage:(int)ordinalNum label:(const char *)aString bBox:(const NXRect *)pageRect fonts:(const char *)fontNames;
- beginPageSetupRect:(const NXRect *)aRect placement:(const NXPoint *)location;
- addToPageSetup;
- endPageSetup;
- endPage;
- beginTrailer;
- endTrailer;
- endPSOutput;
- spoolFile:(const char *)filename;
- (float)heightAdjustLimit;
- (float)widthAdjustLimit;
- adjustPageWidthNew:(float *)newRight left:(float)oldLeft right:(float)oldRight limit:(float)rightLimit;
- adjustPageHeightNew:(float *)newBottom top:(float)oldTop bottom:(float)oldBottom limit:(float)bottomLimit;
- (BOOL)getRect:(NXRect *)theRect forPage:(int)page;
- placePrintRect:(const NXRect *)aRect offset:(NXPoint *)location;
- drawSheetBorder:(float)width :(float)height;
- drawPageBorder:(float)width :(float)height;
- addCursorRect:(const NXRect *)aRect cursor:anObj;
- removeCursorRect:(const NXRect *)aRect cursor:anObj;
- discardCursorRects;
- resetCursorRects;
- (BOOL)shouldDrawColor;
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

@interface Object(SenderOfPrintPSCode)
- (BOOL)shouldRunPrintPanel:aView;
@end


@interface View (IconDragging)
- dragFile:(const char *)filename fromRect:(NXRect *)rect slideBack:(BOOL) aFlag event:(NXEvent *)event;
@end

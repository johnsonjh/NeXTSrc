/*
	Window.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Responder.h"
#import "screens.h"
#import "graphics.h"
#import "color.h"
#import <objc/hashtable.h>

/* Window Types */

#define NX_PLAINSTYLE		0
#define NX_TITLEDSTYLE		1
#define NX_MENUSTYLE		2
#define NX_MINIWINDOWSTYLE	3
#define NX_MINIWORLDSTYLE	4
#define NX_TOKENSTYLE		5
#define NX_RESIZEBARSTYLE	6
#define NX_SIZEBARSTYLE		NX_RESIZEBARSTYLE	/* historical */

#define NX_FIRSTWINSTYLE	NX_PLAINSTYLE
#define NX_LASTWINSTYLE		NX_RESIZEBARSTYLE
#define NX_NUMWINSTYLES		(NX_LASTWINSTYLE - NX_FIRSTWINSTYLE + 1)

#define NX_CLOSEBUTTONMASK		1
#define NX_RESIZEBUTTONMASK		2
#define NX_MINIATURIZEBUTTONMASK	4
#define NX_ALLBUTTONS \
	(NX_CLOSEBUTTONMASK|NX_RESIZEBUTTONMASK|NX_MINIATURIZEBUTTONMASK)

#define NX_ICONWIDTH		48.0
#define NX_ICONHEIGHT		48.0
#define NX_TOKENWIDTH		64.0
#define NX_TOKENHEIGHT		64.0

/* Window Level Hierarchy */

#define NX_NORMALLEVEL		0
#define NX_FLOATINGLEVEL	3
#define NX_DOCKLEVEL		5
#define NX_SUBMENULEVEL		10
#define NX_MAINMENULEVEL	20

@interface Window : Responder
{
    NXRect              frame;
    id                  contentView;
    id                  delegate;
    id                  firstResponder;
    id                  lastLeftHit;
    id                  lastRightHit;
    id                  counterpart;
    id                  fieldEditor;
    int                 winEventMask;
    int                 windowNum;
    float               backgroundGray;
    struct _wFlags {
	unsigned int        style:4;
	unsigned int        backing:2;
	unsigned int        buttonMask:3;
	unsigned int        visible:1;
	unsigned int        isMainWindow:1;
	unsigned int        isKeyWindow:1;
	unsigned int        isPanel:1;
	unsigned int        hideOnDeactivate:1;
	unsigned int        dontFreeWhenClosed:1;
	unsigned int        oneShot:1;
    }                   wFlags;
    struct _wFlags2 {
	unsigned int        deferred:1;
	unsigned int        _cursorRectsDisabled:1;
	unsigned int        _haveFreeCursorRects:1;
	unsigned int        _validCursorRects:1;
	unsigned int        docEdited:1;
	unsigned int        dynamicDepthLimit:1;
	unsigned int        _worksWhenModal:1;
	unsigned int        _limitedBecomeKey:1;
	unsigned int        _needsFlush:1;
	unsigned int        _newMiniIcon:1;
	unsigned int        _ignoredFirstMouse:1;
	unsigned int        _RESERVED:1;
	unsigned int        _windowDying:1;
	unsigned int        _tempHidden:1;
	unsigned int        _hiddenOnDeactivate:1;
	unsigned int        _floatingPanel:1;
    }                   wFlags2;
    id                  _borderView;
    short               _displayDisabled;
    short               _flushDisabled;
    void               *_cursorRects;
    NXHashTable        *_trectTable;
    id                  _invalidCursorView;
    const char         *_miniIcon;
    void	       *_private;
}

+ getFrameRect:(NXRect *)fRect forContentRect:(const NXRect *)cRect style:(int)aStyle;
+ getContentRect:(NXRect *)cRect forFrameRect:(const NXRect *)fRect style:(int)aStyle;
+ (NXCoord)minFrameWidth:(const char *)aTitle forStyle:(int)aStyle buttonMask:(int)aMask;
+ (NXWindowDepth)defaultDepthLimit;

- init;
- initContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;
- initContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag screen:(const NXScreen *)screen;
- free;
- awake;
- setTitle:(const char *)aString;
- setTitleAsFilename:(const char *)aString;
- setExcludedFromWindowsMenu:(BOOL)flag;
- (BOOL)isExcludedFromWindowsMenu;
- setContentView:aView;
- contentView;
- setDelegate:anObject;
- delegate;
- (const char *)title;
- (int)buttonMask;
- (int)windowNum;
- getFieldEditor:(BOOL)createFlag for:anObject;
- endEditingFor:anObject;
- placeWindowAndDisplay:(const NXRect *)frameRect;
- placeWindow:(const NXRect *)frameRect;
- placeWindow:(const NXRect *)frameRect screen:(const NXScreen *)screen;
- (BOOL)constrainFrameRect:(NXRect *)frameRect toScreen:(const NXScreen *)screen;
- sizeWindow:(NXCoord)width :(NXCoord)height;
- moveTo:(NXCoord)x :(NXCoord)y;
- moveTopLeftTo:(NXCoord)x :(NXCoord)y;
- moveTo:(NXCoord)x :(NXCoord)y screen:(const NXScreen *)screen;
- moveTopLeftTo:(NXCoord)x :(NXCoord)y screen:(const NXScreen *)screen;
- getFrame:(NXRect *)theRect;
- getFrame:(NXRect *)rect andScreen:(const NXScreen **)screen;
- getMouseLocation:(NXPoint *)thePoint;
- (int)style;
- useOptimizedDrawing:(BOOL)flag;
- disableFlushWindow;
- reenableFlushWindow;
- flushWindow;
- flushWindowIfNeeded;
- disableDisplay;
- reenableDisplay;
- (BOOL)isDisplayEnabled;
- displayIfNeeded;
- display;
- update;
- (int)setEventMask:(int)newMask;
- (int)addToEventMask:(int)newEvents;
- (int)removeFromEventMask:(int)oldEvents;
- (int)eventMask;
- setTrackingRect:(const NXRect *)aRect inside:(BOOL)insideFlag owner:anObject tag:(int)trackNum left:(BOOL)leftDown right:(BOOL)rightDown;
- discardTrackingRect:(int)trackNum;
- makeFirstResponder:aResponder;
- firstResponder;
- sendEvent:(NXEvent *)theEvent;
- windowExposed:(NXEvent *)theEvent;
- windowMoved:(NXEvent *)theEvent;
- windowResized:(NXEvent *)theEvent;
- screenChanged:(NXEvent *)theEvent;
- makeKeyWindow;
- becomeKeyWindow;
- resignKeyWindow;
- becomeMainWindow;
- resignMainWindow;
- displayBorder;
- rightMouseDown:(NXEvent *)theEvent;
- (BOOL)commandKey:(NXEvent *)theEvent;
- close;
- setFreeWhenClosed:(BOOL)flag;
- miniaturize:sender;
- deminiaturize:sender;
- (BOOL)tryToPerform:(SEL)anAction with:anObject;
- validRequestorForSendType:(NXAtom)sendType andReturnType:(NXAtom)returnType;
- setBackgroundGray:(float)value;
- (float)backgroundGray;
- setBackgroundColor:(NXColor)color;
- (NXColor)backgroundColor;
- dragFrom:(float)x :(float)y eventNum:(int)num;
- setHideOnDeactivate:(BOOL)flag;
- (BOOL)doesHideOnDeactivate;
- center;
- makeKeyAndOrderFront:sender;
- orderFront:sender;
- orderBack:sender;
- orderOut:sender;
- orderWindow:(int)place relativeTo:(int)otherWin;
- setMiniwindowIcon:(const char *)anIcon;
- (const char *)miniwindowIcon;
- setDocEdited:(BOOL)flag;
- (BOOL)isDocEdited;
- (BOOL)isVisible;
- (BOOL)isKeyWindow;
- (BOOL)isMainWindow;
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
- (BOOL)worksWhenModal;
- convertBaseToScreen:(NXPoint *)aPoint;
- convertScreenToBase:(NXPoint *)aPoint;
- performClose:sender;
- performMiniaturize:sender;
- (int)gState;
- setOneShot:(BOOL)flag;
- (BOOL)isOneShot;
- faxPSCode:sender;
- printPSCode:sender;
- copyPSCodeInside:(const NXRect *)rect to:(NXStream *)stream;
- smartFaxPSCode:sender;
- smartPrintPSCode:sender;
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
- endPageSetup;
- endPage;
- beginTrailer;
- endTrailer;
- endPSOutput;
- spoolFile:(const char *)filename;
- (float)heightAdjustLimit;
- (float)widthAdjustLimit;
- (BOOL)getRect:(NXRect *)theRect forPage:(int)page;
- placePrintRect:(const NXRect *)aRect offset:(NXPoint *)location;
- addCursorRect:(const NXRect *)aRect cursor:anObj forView:aView;
- removeCursorRect:(const NXRect *)aRect cursor:anObj forView:aView;
- disableCursorRects;
- enableCursorRects;
- discardCursorRects;
- invalidateCursorRectsForView:aView;
- resetCursorRects;
- setDepthLimit:(NXWindowDepth)limit;
- (NXWindowDepth)depthLimit;
- setDynamicDepthLimit:(BOOL)flag;
- (BOOL)hasDynamicDepthLimit;
- (const NXScreen *)screen;
- (const NXScreen *)bestScreen;
- (BOOL)canStoreColor;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag screen:(const NXScreen *)screen;
+ new;

@end

@interface Object(WindowDelegate)
- windowWillClose:sender;
- windowWillReturnFieldEditor:sender toObject:client;
- windowWillResize:sender toSize:(NXSize *)frameSize;
- windowDidResize:sender;
- windowDidExpose:sender;
- windowDidMove:sender;
- windowDidBecomeKey:sender;
- windowDidResignKey:sender;
- windowDidBecomeMain:sender;
- windowDidResignMain:sender;
- windowWillMiniaturize:sender toMiniwindow:miniwindow;
- windowDidMiniaturize:sender;
- windowDidDeminiaturize:sender;
- windowDidUpdate:sender;
- windowDidChangeScreen:sender;
@end

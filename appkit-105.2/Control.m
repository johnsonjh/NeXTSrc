/*
	Control.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Control_Private.h"
#import "Application_Private.h"
#import "Cell_Private.h"
#import "View_Private.h"
#import "Window.h"
#import "Matrix.h"
#import "Cell.h"
#import "Text.h"
#import "nextstd.h"
#import "publicWraps.h"
#import <zone.h>
#import <dpsclient/wraps.h>

typedef struct {
    @defs (Window)
}                  *windowId;

#define CELL (cell ? cell : [self selectedCell])

#define CANDRAW (window && ((windowId)window)->windowNum > 0 && \
		 !(((windowId)window)->_displayDisabled))

@implementation Control:View


+ setCellClass:factoryId
{
    return self;
}

+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}

- initFrame:(const NXRect *)frameRect
{
    [super initFrame:frameRect];
    vFlags.opaque = YES;
    conFlags.enabled = YES;
    conFlags.calcSize = YES;
    conFlags.editingValid = YES;
    return self;
}


- free
{
    [self abortEditing];
    [cell free];
    return[super free];
}


- sizeToFit
{
    NXSize              cellSize;

    if (cell) {
	[cell calcCellSize:&cellSize];
	[self sizeTo:cellSize.width:cellSize.height];
    }
    return self;
}


- sizeTo:(NXCoord)width :(NXCoord)height
{
    if (frame.size.width != width || frame.size.height != height) {
	[super sizeTo:width :height];
    }
    return self;
}


- calcSize
{
    if (conFlags.calcSize) {
	[cell calcDrawInfo:&bounds];
	conFlags.calcSize = NO;
    }
    return self;
}


/*
 * Control Cell Manipulation Routines
 */

- cell
{
    return cell;
}


- setCell:aCell
{
    id oldCell = cell;
    cell = aCell;
    return oldCell;
}

- selectedCell
{
    return cell;
}


- target
{
    return[cell target];
}


- setTarget:anObject
{
    [cell setTarget:anObject];
    return self;
}


- (SEL)action
{
    return[cell action];
}


- setAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "[Control -setAction:]");
    [cell setAction:aSelector];
    return self;
}


- (int)tag
{
    return tag;
}


- setTag:(int)anInt
{

    tag = anInt;
    return self;
}


- (int)selectedTag
{
    id aCell;
    aCell = [self selectedCell];
    return (aCell ?[aCell tag] : -1);
}


- ignoreMultiClick:(BOOL)flag
{
    conFlags.ignoreMultiClick = flag ? YES : NO;
    return self;
}

- (int)mouseDownFlags
{
    return[cell mouseDownFlags];
}


- (int)sendActionOn:(int)mask
{
    return[cell sendActionOn:mask];
}


- (BOOL)isContinuous
{
    return[cell isContinuous];
}


- setContinuous:(BOOL)flag
{
    [cell setContinuous:flag];
    return self;
}


- (BOOL)isEnabled
{
    return (cell ? [cell isEnabled] : conFlags.enabled);
}


- setEnabled:(BOOL)flag
{
    if (cell) {
	if ([cell isEnabled] != flag) {
	    [cell setEnabled:flag];
	    if ([cell controlView] != self) {
		if (!flag) {
		    [self abortEditing];
		}
		[self update];
	    }
	}
    } else {
	if ((conFlags.enabled && !flag) || (!conFlags.enabled && flag)) {
	    conFlags.enabled = flag ? YES : NO;
	    if (!flag) {
		[self abortEditing];
	    }
	    [self update];
	}
    }
    return self;
}


- setFloatingPointFormat:(BOOL)autoRange
    left:(unsigned)leftDigits
    right:(unsigned)rightDigits
{
    if (cell) {
	[cell setFloatingPointFormat:autoRange
	    left:leftDigits right:rightDigits];
	if ([cell controlView] != self) {
	    [self abortEditing];
	    [self updateCellInside:cell];
	}
    }
    return self;
}


- (int)alignment
{
    return[cell alignment];
}


- setAlignment:(int)mode
{
    if (cell && mode != [cell alignment]) {
	[cell setAlignment:mode];
	if ([cell controlView] != self) {
	    [self abortEditing];
	    [self updateCellInside:cell];
	}
    }
    return self;
}


- font
{
    return[cell font];
}


- setFont:fontObj
{
    id textObj;

    if (cell && fontObj != [cell font]) {
	[cell setFont:fontObj];
	if ([cell controlView] != self) {
	    [self updateCellInside:cell];
	    if (textObj = [self currentEditor]) {
		[textObj setFont:fontObj];
	    }
	}
    }

    return self;
}


- setStringValueNoCopy:(char *)aString shouldFree:(BOOL)flag
{
    id                  c = CELL;
    BOOL                autodisplaydisabled;

    if (c) {
	[self abortEditing];
	autodisplaydisabled = vFlags.disableAutodisplay;
	if (!autodisplaydisabled) {
	    vFlags.disableAutodisplay = YES;
	}
	[c setStringValueNoCopy:aString shouldFree:flag];
	if (!autodisplaydisabled) {
	    vFlags.disableAutodisplay = NO;
	    [self drawCellInside:c];
	} else {
	    vFlags.needsDisplay = YES;
	}
    }
    return self;
}


#define SET_VALUE(setValue, value) \
    id              c = CELL; \
    BOOL	    autodisplaydisabled; \
    if (c && [c controlView] != self) { \
	[self abortEditing]; \
	autodisplaydisabled = vFlags.disableAutodisplay; \
	if (!autodisplaydisabled) vFlags.disableAutodisplay = YES; \
	[c setValue:value]; \
	if (!autodisplaydisabled) { \
	    vFlags.disableAutodisplay = NO; \
	    [self drawCellInside:c]; \
	} else vFlags.needsDisplay = YES; \
    } else if (c) { \
	[c setValue:value]; \
    }

- setStringValue:(const char *)aString
{
    SET_VALUE(setStringValue, aString);
    return self;
}


- setStringValueNoCopy:(const char *)aString
{
    SET_VALUE(setStringValueNoCopy, aString);
    return self;
}


- setIntValue:(int)anInt
{
    SET_VALUE(setIntValue, anInt);
    return self;
}


- setFloatValue:(float)aFloat
{
    SET_VALUE(setFloatValue, aFloat);
    return self;
}


- setDoubleValue:(double)aDouble
{
    SET_VALUE(setDoubleValue, aDouble);
    return self;
}


#define RETURN_VALUE(value, default) \
    id              c = CELL; \
    if (c) { \
	if ([cell controlView] != self) { \
	    [self validateEditing]; \
	} \
	return [c value]; \
    } else return default;


- (const char *)stringValue
{
    RETURN_VALUE(stringValue, (char *)0);
}


- (int)intValue
{
    RETURN_VALUE(intValue, 0);
}


- (float)floatValue
{
    RETURN_VALUE(floatValue, 0.0);
}


- (double)doubleValue
{
    RETURN_VALUE(doubleValue, 0.0);
}


- update
{
    if (vFlags.disableAutodisplay || !CANDRAW) {
	conFlags.calcSize = YES;
	vFlags.needsDisplay = YES;
    } else {
	[self calcSize];
	[self display];
    }
    return self;
}


- updateCell:aCell
{
    if (vFlags.disableAutodisplay || !CANDRAW) {
	vFlags.needsDisplay = YES;
	conFlags.calcSize = YES;
    } else {
	[self drawCell:aCell];
    }
    return self;
}


- updateCellInside:aCell
{
    if (vFlags.disableAutodisplay || !CANDRAW) {
	vFlags.needsDisplay = YES;
    } else {
	[self drawCellInside:aCell];
    }
    return self;
}


- drawCellInside:aCell
{
    if (aCell && aCell == cell) {
	if (CANDRAW) {
	    [self lockFocus];
	    [cell drawInside:&bounds inView:self];
	    [window flushWindow];
	    [self unlockFocus];
	} else {
	    vFlags.needsDisplay = YES;
	}
    }
    return self;
}


- drawCell:aCell
{
    return (aCell && aCell == cell) ?[self display] : self;
}

- selectCell:aCell
{
    if (aCell && aCell == cell && ![cell state]) {
	[cell incrementState];
	[self drawCell:cell];
    }
    return self;
}

- sendAction:(SEL)theAction to:theTarget
{
    if ([NXApp sendAction:theAction to:theTarget from:self])
	return self;
    else if (theAction && [NXApp _isRunningModal])
	NXBeep();
    return nil;
}


- (BOOL)_isBlockedByModalResponder:theCell
{
    SEL                 action;
    id                  target, responder;

    return NO;	/* worksWhenModal */

    if ([NXApp _isRunningModal] && window != [NXApp keyWindow]) {
	target = [theCell target];
	if (!target) {
	    action = [theCell action];
/*	    responder = findResponder([NXApp keyWindow], action, YES); */
	    return (responder ? NO : YES);
	} else {
	    return YES;
	}
    } else {
	return NO;
    }
}

/* Control Interface
 * -----------------
 *
 * The 'Control Interface' of an object is the set of methods that 
 * can be used as an action by a Control which target is set to this object.
 * Since a Control object can be used as target by another Control we define
 * a generic Control Interface for Controls that consists in the set of
 * methods that set some specific value of the receiver to the 
 * corresponding one of the sender.
 *
 * Ex: if you want to display the float specific value of a Slider in
 * a TextField while dragging the Slider, set the target of the Slider
 * to the TextField and its action to 'takeFloatValueFrom:' 
 */


- takeIntValueFrom:sender
{
    return[self setIntValue:[sender intValue]];
}


- takeFloatValueFrom:sender
{
    return[self setFloatValue:[sender floatValue]];
}


- takeDoubleValueFrom:sender
{
    return[self setDoubleValue:[sender doubleValue]];
}


- takeStringValueFrom:sender
{
    return[self setStringValue:[sender stringValue]];
}


- drawSelf:(const NXRect *)rects :(int)rectCount
{
    const char	       *savedText = NULL;

    if (!cell)
	return self;

    if ([self currentEditor]) {
	savedText = [cell stringValue];
	if (savedText) savedText = (const char *)NXCopyStringBuffer(savedText);
	[cell _setView:NULL];
	[cell setStringValueNoCopy:NULL];
    }
    [cell drawSelf:&bounds inView:self];
    if (savedText) {
	[cell _setView:NULL];
	[cell setStringValueNoCopy:(char *)savedText shouldFree:YES];
	[cell _setView:self];
    }

    return self;
}

static BOOL mouseHit(id view, NXRect *r, NXPoint *p)
{
    NXPoint localPoint = *p;
    [view convertPoint:&localPoint fromView:nil];
    return (NXMouseInRect(&localPoint, r, [view isFlipped]));
}

#define ACTIVECONTROLMASK	(NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK)

- mouseDown:(NXEvent *)theEvent
{
    volatile NXHandler	handler;
    volatile BOOL       mouseUp = YES;
    int                 oldMask;

    if (!cell || (conFlags.ignoreMultiClick && theEvent->data.mouse.click > 1) || ![cell isEnabled]) {
	return [super mouseDown:theEvent];
    }

    handler.code = 0;

    oldMask = [window addToEventMask:ACTIVECONTROLMASK];
    [self lockFocus];

    for (;;) {
	if (mouseHit(self, &bounds, &theEvent->location)) {
	    [cell highlight:&bounds inView:self lit:YES];
	    [window flushWindow];
	    NX_DURING {
		mouseUp = [cell trackMouse:theEvent
		    inRect:&bounds ofView:self];
	    } NX_HANDLER {
		handler = NXLocalHandler;
	    } NX_ENDHANDLER
	    if ([self window])	/* ??? temporary hack for Keith */
		[cell highlight:&bounds inView:self lit:NO];
	    else
		if (cell) _NXSetCellParam(cell, NX_CELLHIGHLIGHTED, 0);
	    [window flushWindow];
	    if (mouseUp) {
		DPSFlush();
		break;
	    }
	}
	if (handler.code) {
	    int isStillDown;
	    PSstilldown(theEvent->data.mouse.eventNum, &isStillDown);
	    if (isStillDown) {
		theEvent = [NXApp getNextEvent:NX_MOUSEUPMASK];
	    }
	    break;
	}
	theEvent = [NXApp getNextEvent:ACTIVECONTROLMASK];
	if (theEvent->type == NX_MOUSEUP) {
	    break;
	}
    }

    [self unlockFocus];
    [window setEventMask:oldMask];

    if (handler.code) {
	NX_RAISE(handler.code, handler.data1, handler.data2);
    }

    return self;
}

/*
 * Control Text-Editing Routines
 */

- currentEditor
{
    id sview, textEdit;

    textEdit = [window getFieldEditor:NO for:self];
    sview = [textEdit superview];
    if (sview != self) {
	sview = [sview superview];
    }
    return (sview == self) ? textEdit : nil;
}


- abortEditing
{
    id textObj;

    textObj = [self currentEditor];
    if (textObj) {
	[textObj setDelegate:nil];
	if (![window makeFirstResponder:window]) {
	    NX_ASSERT(NO, "Couldn't gracefully abortEditing");
	}
	[CELL endEditing:textObj];
    }
    conFlags.editingValid = YES;

    return textObj ? self : nil;
}


#define BUF_MAX		1024

- _validateEditing:textObject
{
    char               *string;
    const char	       *cellString;
    int                 length,blength;
    id			old_view;
    char		buffer[BUF_MAX];

    length = [textObject textLength] + 1;
    blength = [textObject byteLength] + 1;
    if (blength > BUF_MAX) {
	NX_ZONEMALLOC([self zone], string, char, blength);
    } else {
	string = buffer;
    }
    [textObject getSubstring:string start:0 length:length];
    old_view = [CELL _setView:nil];
    cellString = [CELL stringValue];
    if (string && (!cellString || strcmp(string, cellString))) [CELL setStringValue:string];
    [CELL _setView:old_view];
    conFlags.editingValid = YES;
    if (blength > BUF_MAX) NX_FREE(string);
    return self;
}


- validateEditing
{
    id textEdit = [self currentEditor];
    return textEdit ? [self _validateEditing:textEdit]:self;
}


- _setWindow:theWindow
 /*
  * We don't want to move the global Text object of the window while a
  * Control's window is changed. 
  */
{
    [self abortEditing];
    return[super _setWindow:theWindow];
}

- resetCursorRects
{
    NXRect visible;

    if ([self getVisibleRect:&visible]) {
	[cell resetCursorRect:&visible inView:self];
    }

    return self;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "i@s", &tag, &cell, &conFlags);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadTypes(stream, "i@s", &tag, &cell, &conFlags);
    return self;
}


@end

/*
	
Modifications (since 0.8):
  
0.81
----
11/22/88 pah	added sendActionOn: (defers to Cell's sendActionOn:)
		 and removed setActionSentOnMouseDown:
12/2/88  pah	supported scrolling editable cells by having
		 abortEditing check for subviews which are ScrollViews
12/5/88  pah	added isTracking
		added setCellClass: factory method (also added it
		 to all subclasses of Control)
12/11/88 pah	removed isTracking
		added mouseDownFlags cover for Cell's method of same name
12/13/88 pah	added extra code to set<Type>Value: methods to make sure that
		 the cell doesn't get drawn twice
		added setStringValueNoCopy:shouldFree: for completeness
0.82
----
12/31/88 pah	added selectedTag and selectedCell for Matrix compatibility
 1/03/89 pah	general clean-up
 1/04/89 pah	eliminate conFlags.multiCell
		abortEditing and validateEditing don't look at editingValid
		 (although they still set it to YES)
 1/03/89 bgy	enhance sendAction: to use delegates
 1/05/89 pah	added autodisplay code for sizeTo::, setEnabled: and setFont:
 1/09/89 pah	added setCell: to allow swapping of cell on the fly (ouch!)
		sendAction:to: returns nil if no action was sent
 1/12/89 pah	overrode update to do calcSize
	 	added updateCell: and updateCellInside: to mirror update method
 1/25/89 trey	made setString* methods take const params
 1/27/89 bs	added read: write:
 2/01/89 trey	in abortEditing, [window endEditingFor:] is sent before
		 [cell endEditing:].  The delegate is set to nil before
		 this to prevent infinite loops, and to prevent didEnd/willEnd
		 being called.
 2/06/89 pah	always set vFlags.needsDisplay if we want to draw and can't

0.91
----
 5/10/89 pah	add allowMultiClick: to prevent button-bouncing
 5/18/89 pah	setFont: will change the Text object's font as well if editing
 5/28/89 pah	move resetCursorRect: responsibility to Cells

0.92
----
  6/5/89 pah	change allowMultiClick: to ignoreMultiClick:

0.93
----
 6/05/89 pah	change allowMultiClick: to ignoreMultiClick:
 6/14/89 pah	get rid of calls to _NX[GS]etCellParam()
 6/15/89 trey	- _validateEditing: uses a static buffer if possible
 6/15/89 pah	eliminate redundant autodisplay (since ActionCells now do that)
		add setAlignment:/alignment methods
		return const char * instead char *
		make _amEditing public (currentEditor)
 6/16/89 pah	add some fixes since some other kit methods now return const
 6/16/89 wrp	put flushWindow inside focus lock to reduce ps

0.94
----
 7/13/89 pah	removed abortEditing from sizeTo:: (subclasses are now
		 responsible for dealing with sizeTo:: while editing)
79
--
  4/3/90 pah	fixed sendAction:to: so that it implements the 2.0 semantic
		 when running modal (i.e. when modal, the responder chain is
		 the key window, then the frontmost modal window unless they
		 are the same in which case it is the key window and the next
		 frontmost modal window).
		also fixed sendAction:to: to enforce delivery restrictions on
		 action messages sent while running modal.  sendAction:to: now
		 beeps if a target/action message cannot be delivered while
		 running modal.
		moved tryToPerform: behaviour of application delegate
		 into Application's tryToPerform: where it belongs

80
--
  4/7/90 pah	moved _NXCalcTargetForAction() and sendAction:to:from:
		 into Application (since objects other than Controls can
		 send target/action messages down the responder chain).

93
--
 8/28/90 aozer	Fixed drawSelf:: to copy the string value instead of just
		referencing; this fixes bug 7379.

94
--
 9/25/90 gcockrft	use byteLength instead of textLength
 
105
---
 11/6/90 glc	use blength not length
 
 
 */


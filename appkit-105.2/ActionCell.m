/*
	ActionCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "ActionCell.h"
#import "View.h"
#import "Control.h"
#import "PopUpList.h"
#import "nextstd.h"

@implementation ActionCell:Cell

- target
{
    return target;
}


- setTarget:anObject
{
    target = anObject;
    return self;
}


- (SEL)action
{
    return action;
}


- setAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "ActionCell -setAction:");
    action = aSelector;
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

/*
 * _view is a private hook to make ActionCells autodisplay when (and only
 * when) setTypeValue: is called.  This is accomplished by overriding
 * setStringValue: and setStringValueNoCopy:shouldFree: (which eventually
 * get called by all the setTypeValue: methods in Cell) to send the message
 * updateCellInside: to _view.  _view should, therefore, never be an object
 * which is not an inheritor of the Control class.  _view is set either by
 * _setView: (be careful with this one!) or, more normally, whenever
 * drawSelf:inView: is called with a Control as the view to draw in.
 *
 * setStringValue: has to be tricky because it must disable autodisplay
 * before calling super's implementation because super's implementation may
 * call setStringValueNoCopy:shouldFree: which would call updateCellInside:
 * (causing that method to draw the cell's inside twice).
 *
 * It is highly questionable as to whether this should have been put in, and
 * its whole existence arises from the questionable autodisplay mechanism.
 * However, Interface Builder requires this hook to allow connecting cells
 * to other cells without intervening code to do the autodisplaying.
 *
 * WARNING: Be very careful to keep this up to date with changes in
 * Cell: if setFloatValue:, setDoubleValue:, or setIntValue: is changed
 * so that it does NOT call setStringValue: or setStringValueNoCopy:shouldFree:
 * then be sure to update this class accordingly!
 *
 * This is a classic example of a wart we signed up to live with for all
 * eternity.  Hopefully, it won't cause too many problems in the future.
 */


- _setView:view
{
    id oldView = _view;

    if ([view isKindOf:[Control class]]) {
	_view = view;
    } else {
	_view = nil;
    }

    return oldView;
}


- controlView
{
    return _view;
}

- setBordered:(BOOL)flag
{
    if ((cFlags1.bordered && !flag) || (!cFlags1.bordered && flag) || cFlags1.bezeled) {
	cFlags1.bordered = (flag ? YES : NO);
	cFlags1.bezeled = NO;
	[[self controlView] updateCell:self];
    }
    return self;
}

- setBezeled:(BOOL)flag
{
    if ((cFlags1.bezeled && !flag) || (!cFlags1.bezeled && flag) || cFlags1.bordered) {
	cFlags1.bezeled = (flag ? YES : NO);
	cFlags1.bordered = NO;
	[[self controlView] updateCell:self];
    }
    return self;
}

- setIcon:(const char *)iconName
{
    [super setIcon:iconName];
    [[self controlView] updateCellInside:self];
    return self;
}

- setFont:fontObj
{
    id textObj;
    Control *controlView = (Control *)[self controlView];

    if (cFlags1.type == NX_TEXTCELL && support != fontObj) {
	[super setFont:fontObj];
	[controlView updateCellInside:self];
	if (([[self controlView] selectedCell] == self ||
	     [[self controlView] cell] == self) &&
	    (textObj = [controlView currentEditor])) {
	    [textObj setFont:fontObj];
	}
    }

    return self;
}

- setAlignment:(int)mode
{
    Control *controlView = (Control *)[self controlView];

    if (cFlags1.alignment != mode) {
	cFlags1.alignment = mode;
	if ([[self controlView] selectedCell] == self ||
	    [[self controlView] cell] == self) {
	    [controlView abortEditing];
	}
	[controlView updateCellInside:self];
    }

    return self;
}


- setEnabled:(BOOL)flag
{
    Control *controlView = (Control *)[self controlView];

    if ((cFlags1.disabled && flag) || (!cFlags1.disabled && !flag)) {
	cFlags1.disabled = !flag;
	if ([[self controlView] selectedCell] == self ||
	    [[self controlView] cell] == self) {
	    [controlView abortEditing];
	}
	[controlView updateCell:self];
    }

    return self;
}

- setFloatingPointFormat:(BOOL)autoRange
    left:(unsigned)leftDigits
    right:(unsigned)rightDigits
{
    Control *controlView = (Control *)[self controlView];

    [super setFloatingPointFormat:autoRange
	left:leftDigits right:rightDigits];
    if ([[self controlView] selectedCell] == self ||
	[[self controlView] cell] == self) {
	[controlView abortEditing];
    }
    [controlView updateCellInside:self];

    return self;
}

- (const char *)stringValue
{
    if ([[self controlView] selectedCell] == self ||
	[[self controlView] cell] == self) {
	[[self controlView] validateEditing];
    }
    return [super stringValue];
}

- (int)intValue
{
    if ([[self controlView] selectedCell] == self ||
	[[self controlView] cell] == self) {
	[[self controlView] validateEditing];
    }
    return [super intValue];
}

- (float)floatValue
{
    if ([[self controlView] selectedCell] == self ||
	[[self controlView] cell] == self) {
	[[self controlView] validateEditing];
    }
    return [super floatValue];
}

- (double)doubleValue
{
    if ([[self controlView] selectedCell] == self ||
	[[self controlView] cell] == self) {
	[[self controlView] validateEditing];
    }
    return [super doubleValue];
}

#define BUFLEN 1024

/* the autodisplay disabling is because NoCopy can call this back */

- setStringValue:(const char *)aString
{
    char		oldContentsBuf[BUFLEN];
    char		*oldContents = NULL;
    BOOL                autodisplaydisabled;
    BOOL		forceRedraw;
    Control	       *controlView = (Control *)[self controlView];

    forceRedraw = NO;
    if (([controlView selectedCell] == self || [controlView cell] == self) &&
	[controlView abortEditing]) {
	forceRedraw = YES;
    } else if (contents) {
	if (strlen(contents) < BUFLEN) {
	    oldContents = oldContentsBuf;
	    strcpy(oldContents, contents);
	} else {
	    oldContents = NULL;
	    forceRedraw = YES;	/* string is so long we give up and redraw */
	}
    } else {
	oldContents = NULL;
    }
    if (controlView) {
	autodisplaydisabled = ![controlView isAutodisplay];
	[controlView setAutodisplay:NO];
	[super setStringValue:aString];
	if (!autodisplaydisabled) {
	    [controlView setAutodisplay:YES];
	}
	if (forceRedraw ||
	    ((oldContents || contents) &&
	     (!oldContents || !contents || strcmp(contents, oldContents)))) {
	    [controlView updateCellInside:self];
	}
    } else {
	[super setStringValue:aString];
    }

    return self;
}


- setStringValueNoCopy:(char *)aString shouldFree:(BOOL)flag
{
    BOOL                needsRedraw;
    Control	       *controlView = (Control *)[self controlView];

    needsRedraw =
	((([controlView selectedCell] == self || [controlView cell] == self) &&
	  [controlView abortEditing]) ||
	((aString != contents) && (!aString || !contents)) ||
        (aString && contents && strcmp(aString, contents)));
    [super setStringValueNoCopy:aString shouldFree:flag];
    if (needsRedraw && controlView) {
	[controlView updateCellInside:self];
    }

    return self;
}


- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    if ([self controlView] != controlView) {
	[self _setView:controlView];
    }
    return[super drawSelf:cellFrame inView:controlView];
}


/*
 * End of _view hook stuff.
 */


- write:(NXTypedStream *) stream
{
    [super write:stream];
    if ([target isKindOf:[PopUpList class]])
        NXWriteObject(stream, target);
    else
        NXWriteObjectReference(stream, target);
    NXWriteTypes(stream, "i:", &tag, &action);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    target = NXReadObject(stream);
    NXReadTypes(stream, "i:", &tag, &action);
    return self;
}


@end

/*
  
Modificatons (since 0.8):
  
12/13/88 pah	added setStringValueNoCopy:shouldFree: for completeness
		removed setFloatValue:, setDoubleValue:, setIntValue:, and
		 setStringValueNoCopy: since they are redundant (they all call
		 either setStringValueNoCopy:shouldFree: or setStringValue:
  
0.82
----
 1/04/88 pah	_setView: only sets the _view if it is a Control view
 1/10/88 pah	setStringValue: only displays so if the string changes
 1/25/89 trey	made setString* methods take const params
 1/27/89 bs	added read: write:

0.91
----
 4/26/89 pah	add comment explaining the _view stuff
		 use a static buffer in setStringValue:
 5/10/89 pah	validate editing on stringValue, intValue, etc.
  

0.92
----
 6/09/89 trey	changed test in setStringValue so that redundant setting
		 to NULL doesnt needlessly erase cell
 6/13/89 pah	abort editing on setStringValue:, etc.

0.93
----
 6/15/89 pah	add autodisplay mechanism to everything
		 add method controlView to return the last Control view the
		  cell was drawn in
		 add methods: setFont:, setAlignment:, setEnabled:,
		  setBezeled:, setBordered:, setFloatingPointFormat:...,
		  setIcon:
		 validateEditing in stringValue, et. al.
 6/16/89 pah	const char * instead of char * for stringValue

*/





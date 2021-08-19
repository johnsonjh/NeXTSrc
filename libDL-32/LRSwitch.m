/*
	LRSwitch.m
  	Copyright 1988, NeXT, Inc.
	M. Hawley
*/
#import <appkit/appkit.h>
#import <string.h>
#import "LRSwitch.h"
#import "bitmap.h"
#define DefaultGap 4.;

@implementation  LRSwitch
/*
 * An LRSwitch has a left title, a right title, and a diamond-shaped arrow
 * in the middle pointing to the currently active side.
 */

#if 0
+ _newFrame:(NXRect *)frameRect {
  return [super newFrame:frameRect];
}
#endif


+ newFrame:(NXRect const *)frameRect
 /*
  * TYPE: Creating
  *
  * Creates a new LRSwitch with default parameters in the given frame.
  */
{
  self = [super newFrame:frameRect];
  [self setClipping:NO];
  leftText = [Cell newTextCell:""];
  rightText = [Cell newTextCell:""];
  button = [ButtonCell newTextCell:""];
  [button setBordered:NO];
  [button setType:NX_TOGGLE];
  bitmap("diamondl"); [button setIcon:"diamondl"];
  bitmap("diamondr"); [button setAltIcon:"diamondr"];
  bFlags.calcSize = 1;
  gap = DefaultGap;
  [self setState:0];
  return self;
}

+ newFrame:(NXRect *)r :(char *)left :(char *)right
/*
 * TYPE: Creating
 *
 * Creates a new LRSwitch in the given frame.
 * The left and right titles are set to 'left' and 'right'.
 */
{
  self = [LRSwitch newFrame:r];
  [self setLeftTitle:left];
  [self setRightTitle:right];
  bFlags.calcSize = 1;
  [self redraw];
  *r = bounds;
  return self;
}

- free
 /*
  * TYPE: Disposing
  *
  * You sometimes call this method.  You call this method to deallocate
  * the storage for your LRSwitch. 
  */
{
    [leftText free];
    [rightText free];
    [button free];
    return ([super free]);
}


- target
 /* 
 /* 
  * TYPE: Querying
  *
  * Returns the target of the cell.
  */
{
  return [button target];
}


- setTarget:anObject
 /*
  * TYPE: Modifying
  *
  * Sets the target of the cell.
  */
{
  [button setTarget:anObject];
  return self;
}


- (SEL) action
 /* 
  * TYPE: Querying
  *
  * Returns the action of the cell 
  */
{
  return [button action];
}


- setAction:(SEL) aSelector
 /*
  * TYPE: Modifying
  *
  * Sets the action of the cell.
  */
{
  [button setAction:aSelector];
  return self;
}


- setLeftTitle:(char *)aString
 /*
  * TYPE: Modifying
  *
  * Sets the title of the button, that is the text of the cell to aString.
  */
{  
  if (aString && !strlen(aString)) aString = ((char *)0);
  [leftText setStringValue:aString];
  bFlags.calcSize = 1;
  return self;
}


- setRightTitle:(char *)aString
 /*
  * TYPE: Modifying
  *
  * Sets the title of the button, that is the text of the cell to aString.
  */
{  
  if (aString && !strlen(aString)) aString = ((char *)0);
  [rightText setStringValue:aString];
  bFlags.calcSize = 1;
  return self;
}


- (const char *)icon
 /*
  * TYPE: Querying
  *
  * You sometimes call this method.  Call this method to get the current
  * value of the LRSwitch's icon = cell's icon. 
  */
{
    return [button icon];
}


- setIcon:(char const *)name
 /* 
  * TYPE: Modifying
  *
  * You sometimes call this method.  Call this method to get the current
  * value of the LRSwitch's icon = cell's icon. 
  */
{
  [button setIcon:name];  
  bFlags.calcSize = 1;
  return self;
}


- (const char *)altIcon
 /*
  * TYPE: Querying
  *
  * You sometimes call this method.  Call this method to get the current
  * value of the LRSwitch's alternate icon = cell's alternate icon. 
  */
{
    return [button altIcon];
}


- setAltIcon:(char const *)name
 /* 
  * TYPE: Modifying
  *
  * You sometimes call this method.  Call this method to get the current
  * value of the LRSwitch's alternate icon = cell's alternate icon. 
  */
{
  [button setAltIcon:name];
  bFlags.calcSize = 1;
  return self;
}


- setFont:fontObj
 /*
  * TYPE: Modifying
  *
  * You sometimes call this method.  You call this method to set the font
  * for your LRSwitch.  
  * You only need to call this method if you do not want to use the
  * default font. 
  */
{  
  [leftText setFont:fontObj];
  [rightText setFont:fontObj];
  bFlags.calcSize = 1;
  return self;
}


- (unsigned short)keyEquivalent
 /* 
  * TYPE: Querying
  * 
  * Gets the key equivalent char of the button.
  */
{
    return [button keyEquivalent];
}


- setKeyEquivalent:(unsigned short)charCode
 /* 
  * TYPE: Modifying
  * 
  * Sets the key equivalent char of the button cell.
  */
{
  [button setKeyEquivalent:charCode];
  return self;
}


- setEnabled:(BOOL)value
 /*
  * TYPE: Modifying
  *
  * Sets whether the button is enabled or not.
  */
{
	if (!value) {
		[leftText setEnabled:value];
		[rightText setEnabled:value];
	}
	else {
		[self setState:[button state]];
	}
	[button setEnabled:value];
	[self display];
	return self;
}

- setState:(int)value
 /*
  * TYPE: Modifying
  *
  * Sets the LRSwitch state (0 or 1) and redraws
  */
{
  [leftText setEnabled:(BOOL)!value];
  [rightText setEnabled:(BOOL)value];
  [button setState:value];
  /*	[button setParameter:NX_CELLSTATE to:value];	*/
  [self display];
  return self;
}


- (int)state
 /* 
  * TYPE: Querying
  *
  * Returns the button state (0 or 1).
  */
{
  return [button getParameter:NX_CELLSTATE];
}


- (int)intValue
 /* 
  * TYPE: Querying
  *
  * Gets button specific integer value = button state .
  */
{
  return [button intValue];
}


- setIntValue:(int)anInt
 /* 
  * TYPE: Modifying
  *
  * Sets control specific integer value = button state. Redraws the button
  */
{
    return [self setState:(anInt ? 1 : 0)];
}


- sizeTo:(NXCoord)width :(NXCoord)height
 /* 
  *  TYPE: Resizing
  *
  * Changes the width and the height of the button frame.
  */
{  
  bFlags.calcSize = 1;
  return [super sizeTo:width :height];
}


- sizeToFit
 /* 
  *  TYPE: Resizing
  *
  * Changes the width and the height of the button frame so that they get
  * the minimum size to contain the cell.
  */
{
  NXSize	theSize;
  
  [button calcCellSize:&theSize];
  [self sizeTo:theSize.width :theSize.height];
  /* fix later */
  return self;
}


/** Graphics **/


- redraw
 /*
  * TYPE: Managing
  *
  * Recomputes the bounds of the title or icon inside the button if
  * necessary. Informs the button that these bounds are valid.
  */
{
#define max(a,b) (((a)>(b))? (a) : (b))
	bFlags.calcSize = 0;
	[leftText calcCellSize:&leftR.size];
	[rightText calcCellSize:&rightR.size];
	[button calcCellSize:&buttonR.size];
	bounds.size.height = max(buttonR.size.height, leftR.size.height);
	bounds.size.width = max(buttonR.size.width + leftR.size.width + rightR.size.width + 2*gap, bounds.size.width);
	[button calcDrawInfo:&bounds];

	buttonR.origin.x = (int)(bounds.size.width - buttonR.size.width)/2 - frame.origin.x;
	leftR.origin.x = buttonR.origin.x - (gap + leftR.size.width);
	rightR.origin.x = buttonR.origin.x + (gap + buttonR.size.width);

	buttonR.origin.y = leftR.origin.y = rightR.origin.y = 0;
	if (buttonR.size.height < bounds.size.height)
		buttonR.origin.y = (int)(bounds.size.height - buttonR.size.height)/2;
	if (leftR.size.height < bounds.size.height)
		leftR.origin.y = (int)(bounds.size.height - leftR.size.height)/2;
	if (rightR.size.height < bounds.size.height)
		rightR.origin.y = (int)(bounds.size.height - rightR.size.height)/2;
	[super sizeTo: bounds.size.width :bounds.size.height];
	return self;
}


- drawSelf:(NXRect *) rects :(int)rectCount
 /*
  * TYPE: Displaying
  *
  * You never call this method, but often override it to implement LRSwitch
  * objects which draw themselves differently. 
  */
{

	if (bFlags.calcSize) [self redraw];
	[leftText drawSelf:&leftR inView:self];
	[rightText drawSelf:&rightR inView:self];
	[button drawSelf:&buttonR inView:self];
	return self;
}

/** EventHandling **/

- (BOOL)_mouseHit :(NXPoint *) basePoint
 /* */
{
    NXPoint         localPoint;

    localPoint = *basePoint;
    [self convertPoint:&localPoint fromView:nil];

    return (NXPointInRect(&localPoint, &bounds));
}


#define ACTIVEBUTTONMASK	(NX_LMOUSEUPMASK|NX_MOUSEDRAGGEDMASK)

- mouseDown:(NXEvent *) theEvent
 /*
  * TYPE: EventHandling
  *
  * You never call this method, but may override it to implement
  * subclassses of the LRSwitch class.  This method is called for you by
  * your LRSwitch's Window object when the mouse goes down in your LRSwitch.
  * This method takes over event processing until a mouseUp is received.
  * If the mouseUp is inside of the LRSwitch, the target will be sent the
  * action message with self and tag as parameter. 
  */
{
    BOOL            hit, lastHit;
    int             state, pingVal;
    int		    oldMask, pingResult;

    lastHit = [self _mouseHit:&theEvent->location];

    if (![button isEnabled] || !lastHit) {
	[NXApp getNextEvent:NX_LMOUSEUPMASK
		waitFor:NX_FOREVER threshold:NX_MODALRESPTHRESHOLD];
	return self;
    }
    oldMask = [window eventMask];
    [window setEventMask:(oldMask | ACTIVEBUTTONMASK)];
    state =  [button state];
    [self lockFocus];

    while (1) {
	if (lastHit) {
	    [self unlockFocus];
	    [self sendAction:[button action] to:[button target]];
	    [self lockFocus];
	    NXPing();
        }
	
        if (DPSPeekEvent(DPSGetCurrentContext(),theEvent,ACTIVEBUTTONMASK,0.,NX_MODALRESPTHRESHOLD)) {
	    [NXApp getNextEvent:ACTIVEBUTTONMASK
			waitFor:NX_FOREVER threshold:NX_MODALRESPTHRESHOLD];
	    hit = [self _mouseHit:&theEvent->location];
	    if (hit != lastHit) {
	        lastHit = hit;
            }
	    if (theEvent->type == NX_LMOUSEUP) {
	        if (lastHit) {
		    [self unlockFocus]; 
		    [self setState:(state ? 0 : 1)];
		    [self lockFocus];
	        }
	        break;
	    }
	}
    }
    [self unlockFocus];
nope:
    [window setEventMask:oldMask];
    return self;
}


- (BOOL)performKeyEquivalent :(NXEvent *) theEvent
 /*
  * TYPE: EventHandling
  *
  * You sometimes call this method. If the char in the event record is
  * equal to your LRSwitch's command key equivalent, your LRSwitch will
  * simulate a successful mouse click and return YES, otherwise, it will
  * return NO. 
  */
{
    
    if (theEvent->data.key.charCode != [button keyEquivalent])
	return NO;
    [self performClick:self];
    return YES;
}


- performClick:sender
 /* 
  * TYPE: Managing
  *
  * Use this method when you want to highlight the button then
  * send the action then  unhighlight the button that is when you
  * want to act exactly as if you had pressed the button.
  */
{
    [self lockFocus];
    DPSFlush();		/* be sure button is lit while we do the action */
    [self setState:([button state] ? 0 : 1)];
    [self sendAction:[button action] to:[button target]];
    [self unlockFocus];
    return self;
}

/* this stuff is for archiving, which we need for nib */
- write:(NXTypedStream *) stream {
    [super write: stream];
    NXWriteTypes (stream, "@@@fs", &leftText, &rightText, &button, &gap, 
    			&bFlags);
    NXWriteRect (stream, &leftR);
    NXWriteRect (stream, &rightR);
    NXWriteRect (stream, &buttonR);
    };

- read:(NXTypedStream *) stream {
    [super read: stream];
    NXReadTypes (stream, "@@@fs", &leftText, &rightText, &button, &gap, 
    			&bFlags);
    NXReadRect (stream, &leftR);
    NXReadRect (stream, &rightR);
    NXReadRect (stream, &buttonR);
    };


#if 0
- (BOOL) _wantsFirstMouse
{
    return YES;
}
#endif

grabLRSwitch() {}

@end

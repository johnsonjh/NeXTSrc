/*
	Button.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Control_Private.h"
#import "ButtonCell_Private.h"
#import "Cell_Private.h"
#import "Button.h"
#import "MenuTemplate.h"
#import "Window.h"
#import "Application.h"
#import <math.h>
#import <string.h>
#import <zone.h>

typedef struct {
    @defs (Window)
}                  *windowId;

typedef struct {
    @defs (MenuTemplate)
}		    *menuTemplateId;

#define CANDRAW (window && ((windowId)window)->windowNum > 0 && \
		 !(((windowId)window)->_displayDisabled))

#define AUTODISPLAY(condition) if (condition) [self updateCellInside:cell]

static const NXRect dfltFrame = {{0.0, 0.0}, {50.0, 50.0}};
static id buttonCellClass = nil;

@implementation Button:Control


+ setCellClass:factoryId
{
    buttonCellClass = factoryId;
    return self;
}


+ new
{
    self = [self newFrame:(NXRect *)(&dfltFrame) title:NULL tag:-1 target:nil action:(SEL)0 key:0 enabled:1];
    return self;
}

- init
{
    return [self initFrame:(NXRect *)(&dfltFrame) title:NULL tag:-1 target:nil action:(SEL)0 key:0 enabled:1];
}


+ newFrame:(const NXRect *)frameRect
{
    self = [self newFrame:frameRect title:NULL tag:-1 target:nil action:(SEL)0 key:0 enabled:1];
    return self;
}


- initFrame:(const NXRect *)frameRect
{
    return [self initFrame:frameRect title:NULL tag:-1 target:nil action:(SEL)0 key:0 enabled:1];
}


+ newFrame:(const NXRect *)frameRect
    title:(const char *)aString
    tag:(int)anInt
    target:anObject
    action:(SEL)aSelector
    key:(unsigned short)charCode
    enabled:(BOOL)flag
{
    return [[self allocFromZone:NXDefaultMallocZone()]
	initFrame:frameRect title:aString tag:anInt target:anObject
	action:aSelector key:charCode enabled:flag];
}

- initFrame:(const NXRect *)frameRect
    title:(const char *)aString
    tag:(int)anInt
    target:anObject
    action:(SEL)aSelector
    key:(unsigned short)charCode
    enabled:(BOOL)flag
{
    NXZone *zone = [self zone];
    [super initFrame:frameRect];
    [self setClipping:NO];
    if (!buttonCellClass) buttonCellClass = [ButtonCell class];
    if (_NXCanAllocInZone(buttonCellClass, @selector(newTextCell:), @selector(initTextCell:))) {
	cell = [[buttonCellClass allocFromZone:zone] initTextCell:aString];
    } else {
	cell = [buttonCellClass newTextCell:aString];
    }
    [cell setAction:aSelector];
    [cell setEnabled:flag];
    [cell setKeyEquivalent:charCode];
    tag = anInt;
    [cell setTarget:anObject];
    vFlags.opaque = [cell isOpaque];
    [self setFlipped:YES];
    return self;
}


+ newFrame:(const NXRect *)frameRect
    icon:(const char *)aString
    tag:(int)anInt
    target:anObject
    action:(SEL)aSelector
    key:(unsigned short)charCode
    enabled:(BOOL)flag
{
    return [[self allocFromZone:NXDefaultMallocZone()]
	initFrame:frameRect icon:aString tag:anInt target:anObject
	action:aSelector key:charCode enabled:flag];
}


- initFrame:(const NXRect *)frameRect
    icon:(const char *)aString
    tag:(int)anInt
    target:anObject
    action:(SEL)aSelector
    key:(unsigned short)charCode
    enabled:(BOOL)flag
{
    NXZone *zone = [self zone];
    [super initFrame:frameRect];
    [self setClipping:NO];
    if (!buttonCellClass) buttonCellClass = [ButtonCell class];
    if (_NXCanAllocInZone(buttonCellClass, @selector(newIconCell:), @selector(initIconCell:))) {
	cell = [[buttonCellClass allocFromZone:zone] initIconCell:aString];
    } else {
	cell = [buttonCellClass newIconCell:aString];
    }
    [cell setAction:aSelector];
    [cell setEnabled:flag];
    [cell setKeyEquivalent:charCode];
    tag = anInt;
    [cell setTarget:anObject];
    vFlags.opaque = [cell isOpaque];
    [self setFlipped:YES];
    return self;
}


- (const char *)title
{
    return[cell title];
}


- setTitle:(const char *)aString
{
    [cell setTitle:aString];
    return self;
}

- setTitleNoCopy:(const char *)aString {
    [cell setTitleNoCopy:aString];
    return self;
}


- (const char *)altTitle
{
    return[cell altTitle];
}


- setAltTitle:(const char *)aString
{
    [cell setAltTitle:aString];
    return self;
}


- (const char *)icon
{
    return[cell icon];
}


- setIcon:(const char *)iconName
{
    [cell setIcon:iconName];
    return self;
}

- image
{
    return [cell image];
}

- setImage:image
{
    [cell setImage:image];
    return self;
}

- (const char *)altIcon
{
    return[cell altIcon];
}

- altImage
{
    return [cell altImage];
}

- setAltIcon:(const char *)iconName
{
    [cell setAltIcon:iconName];
    return self;
}

- setAltImage:image
{
    [cell setAltImage:image];
    return self;
}

- (int)iconPosition
{
    return[cell iconPosition];
}


- setIconPosition:(int)aPosition
{
    [cell setIconPosition:aPosition];
    return self;
}


- setIcon:(const char *)iconName position:(int)aPosition
{
    [cell setIcon:iconName];
    [cell setIconPosition:aPosition];
    return self;
}


- setType:(int)aType
{
    [cell setType:aType];
    return self;
}


- (int)state
{
    return [cell state];
}


- setState:(int)value
{
    if ([cell state] != value) {
	[cell setState:value];
	AUTODISPLAY(YES);
    }
    return self;
}


- (BOOL)isBordered
{
    return[cell isBordered];
}


- setBordered:(BOOL)flag
{
    [cell setBordered:flag];
    vFlags.opaque = [cell isOpaque];
    return self;
}


- (BOOL)isTransparent
{
    return[cell isTransparent];
}


- setTransparent:(BOOL)flag
{
    BOOL oldTransparent = [cell isTransparent];

    [cell setTransparent:flag];
    vFlags.opaque = [cell isOpaque];
    if (oldTransparent != flag) {
	if (flag) {
	    if (vFlags.disableAutodisplay || !CANDRAW) {
		vFlags.needsDisplay = YES;
	    } else {
		[self displayFromOpaqueAncestor:0 :0 :NO];
	    }
	}
    }

    return self;
}

- setPeriodicDelay:(float)delay andInterval:(float)interval
{
    [cell setPeriodicDelay:delay andInterval:interval];
    return self;
}

- getPeriodicDelay:(float *)delay andInterval:(float *)interval
{
    [cell getPeriodicDelay:delay andInterval:interval];
    return self;
}


- (unsigned short)keyEquivalent
{
    return[cell keyEquivalent];
}


- setKeyEquivalent:(unsigned short)charCode
{
    [cell setKeyEquivalent:charCode];
    return self;
}

- sound
{
    return [cell sound];
}

- setSound:aSound
{
    [cell setSound:aSound];
    return self;
}

/** Graphics **/

- display
{
    if ([cell isOpaque] || vFlags.opaque) {
	return[super display];
    } else {
	return[self displayFromOpaqueAncestor:(NXRect *)0 :0 :NO];
    }
}


- highlight:(BOOL)flag
{
    if ([cell isHighlighted] != flag) {
	if (CANDRAW) {
	    [self lockFocus];
	    [cell highlight:&bounds inView:self lit:flag];
	    [window flushWindow];
	    [self unlockFocus];
	} else if (cell) {
	    _NXSetButtonParam(cell, NX_CELLHIGHLIGHTED, flag);
	    vFlags.needsDisplay = YES;
	}
    }
    return self;
}


/* Event Handling */

- (BOOL)performKeyEquivalent:(NXEvent *)theEvent
{
    if (theEvent->data.key.charCode != [cell keyEquivalent]) {
	return NO;
    }
    if (![self _isBlockedByModalResponder:cell]) {
	[cell _setMouseDownFlags:(theEvent ? theEvent->flags : 0)];
	[self performClick:self];
	[cell _setMouseDownFlags:0];
    } else {
	return NO;
    }
    return YES;
}


- performClick:sender
{
    volatile NXHandler	handler;

    if (![self isEnabled]) return self;

    [self highlight:YES];
    DPSFlush();
    [cell _startSound];
    [cell incrementState];

    NX_DURING {
	handler.code = 0;
	[self sendAction:[cell action] to:[cell target]];
    } NX_HANDLER {
	handler = NXLocalHandler;
    } NX_ENDHANDLER

    [cell _stopSound];
    [self highlight:NO];	/* unhighlight even if there's an error */

    if (handler.code) {
	NX_RAISE(handler.code, handler.data1, handler.data2);
    }

    return self;
}


- (BOOL)acceptsFirstMouse
{
    return YES;
}

#ifndef SPECULATE

/* Warning!  This is NOT public, do NOT document. */

- nibInstantiate
{
    char *className;
    
    if ([[cell target] isKindOf:[MenuTemplate class]]
        && (className = ((menuTemplateId)[cell target])->menuClassName)
	&& !strcmp(className, "PopUpList")
    	&& ([cell action] == @selector(popUp:))) {
        [cell setTarget:[[cell target] nibInstantiate]];
    }

    return self;
}

#endif

@end

/*
  
Modifications (since 0.8):
  
12/5/88  pah	added factory method setCellClass:
  
0.82
----
 1/5/89  pah	added autodisplay to setIcon:, setIconPosition:, setType:
		 setState:, setBordered:, setTransparent:, and overrode
		 setEnabled: to autodisplay the inside only
 1/8/89  pah	highlight: only does so if CANDRAW
 1/9/89  pah	performClick: plays the sound (if any)
 1/10/89  pah	add button type NX_MOMENTARYCHANGE
 1/25/89 trey	setTitle: takes a const param
 2/05/89 pah	more char * -> const char *
 2/10/89 pah	always set vFlags.needsDisplay when we want to draw but can't
 2/15/89 pah	don't pass key equivalents on if running modal and action is
		 not destined for the first responder in the keyWindow and
		 that responder respondsTo: the action
  
0.83
----
 3/08/89 pah	Fix display to call displayFromOpaqueAncestor::: if the
		 button is not opaque
 3/16/89 pah	Fix message send only to FR in keyWindow if running modal

0.91
----
  5/2/89 pah	add allowMultiClick:
 5/15/89 pah	fix setIcon:NULL (partially) by changing an == to an =
 5/24/89 pah	make drawInside: fill the correct rectangle if ICON_ONLY

0.92
----
  6/6/89 pah	change allowMultiClick: to ignoreMultiClick:
 6/13/89 pah	moved drawSelf:: and mouseDown: to Control

0.93
----
 6/14/89 pah	add sound and setSound: methods to mirror ButtonCell's
 6/15/89 pah	make title, altTitle, icon, altIcon return a const char *
		 eliminate redundant autodisplaying now that ActionCell's
		  autodisplay
 6/16/89 pah	only displayFromOpaqueAncestor::: both [cell isOpaque] and
		 vFlags.opaque are false
 6/16/89 wrp	put flushWindow inside focus lock to reduce ps

0.96
----
 7/18/89 pah	make performKeyEquivalent: return NO (cmd-key not processed)
		 if the button is blocked by a modal panel.
 7/20/89 pah	don't do performClick: if the button is disabled

77
--
  2/6/90 pah	Added PRIVATE method (which doesn't have an underbar) to
		 support InterfaceBuilder and PopUpLists.

89
--
 7/24/90 aozer	Added setImage:, image, setAltImage:, altImage (all cover
		methods for the actual methods in ButtonCell).
91
--
 8/14/90 chris	Added setTitleNoCopy:
 
*/


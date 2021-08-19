/*
	Responder.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
	
	Changes:
	08/05/88 wrp 	Removed keyRepeated: method.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "publicWraps.h"
#import "appkitPrivate.h"
#import "Responder.h"
#import "nextstd.h"
#import <stdio.h>
#import <string.h>

static id forwardMethod(Responder *self, SEL action, const char *actionName, NXEvent *theEvent);

@implementation Responder


- nextResponder
{
    return nextResponder;
}


- setNextResponder:aResponder
{
    nextResponder = aResponder;
    return self;
}


- (BOOL)tryToPerform:(SEL)anAction with:anObject
{
    if ([self respondsTo:anAction])
	if ([self perform:anAction with:anObject])
	    return YES;
 /* else recurse to find someone who wants this message */
    return[nextResponder tryToPerform:anAction with:anObject];
}

- validRequestorForSendType:(NXAtom)sendType andReturnType:(NXAtom)returnType
{
    return [nextResponder validRequestorForSendType:sendType andReturnType:returnType];
}

- (BOOL)performKeyEquivalent:(NXEvent *)theEvent
{
    return NO;
}


- mouseDown:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "mouseDown:", theEvent);
}


- rightMouseDown:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "rightMouseDown:", theEvent);
}


- mouseUp:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "mouseUp:", theEvent);
}


- rightMouseUp:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "rightMouseUp:", theEvent);
}


- mouseMoved:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "mouseMoved:", theEvent);
}


- mouseDragged:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "mouseDragged:", theEvent);
}


- rightMouseDragged:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "rightMouseDragged:", theEvent);
}


- mouseEntered:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "mouseEntered:", theEvent);
}


- mouseExited:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "mouseExited:", theEvent);
}


- keyDown:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "keyDown:", theEvent);
}


- keyUp:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "keyUp:", theEvent);
}


- flagsChanged:(NXEvent *)theEvent
{
    return forwardMethod(self, _cmd, "flagsChanged:", theEvent);
}


- noResponderFor:(const char *)eventType
{
    if (!strcmp("keyDown:", eventType))
	NXBeep();
    return self;
}


- (BOOL)acceptsFirstResponder
{
    return NO;
}


- becomeFirstResponder
{
    return self;
}


- resignFirstResponder
{
    return self;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteObjectReference(stream, nextResponder);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    nextResponder = NXReadObject(stream);
    return self;
}

@end

/*
 * The check for command-. probably doesn't belong here, but it is
 * inappropriate for the system to beep when a command-. is not responded
 * to (since it does, in fact, take an action (which is to do the
 * NXUserAborted() stuff) by way of sending ^C to the application).
 * noResponderFor: doesn't get the event so it can't do that suppression.
 */

static id forwardMethod(Responder *self, SEL action, const char *actionName, NXEvent *theEvent)
{
    if (self->nextResponder) {
	[self->nextResponder perform:action with:(id)theEvent];
    } else {
	if (!(theEvent->type == NX_KEYDOWN &&
	      (theEvent->flags & NX_COMMANDMASK) &&
	      theEvent->data.key.charCode == '.')) {
	    [self noResponderFor:actionName];
	}
    }
    return self;
}


/*
  
Modifications (starting at 0.8):
  
0.93
----
 6/22/89 trey	beeps when keyDown is not dispatched to any responder
 8/07/89 trey	NXPrintWarnings nuked

80
--
  4/6/90 pah	added _tryToPerform:with: which is just like tryToPerform:with:
		 except that it returns the object which responded instead of
		 a BOOL indicating whether an object responded (perhaps this
		 should be the semantic of the public tryToPerform:with:?)

85
--
  5/22/90 trey	nuked unnecessary uses of sel_getName()

86
--
 6/12/90 pah	nuked _tryToPerform:with:
		added validRequestorForSendType:andReturnType: to make request
		 menu updating much quicker (no more respondsTo:'s)
*/

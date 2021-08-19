/*
	Responder.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import <dpsclient/dpsclient.h>
#import <objc/hashtable.h>

@interface Responder : Object
{
    id                  nextResponder;
    id                  _reserved;
}

- nextResponder;
- setNextResponder:aResponder;
- (BOOL)tryToPerform:(SEL)anAction with:anObject;
- (BOOL)performKeyEquivalent:(NXEvent *)theEvent;
- validRequestorForSendType:(NXAtom)sendType andReturnType:(NXAtom)returnType;
- mouseDown:(NXEvent *)theEvent;
- rightMouseDown:(NXEvent *)theEvent;
- mouseUp:(NXEvent *)theEvent;
- rightMouseUp:(NXEvent *)theEvent;
- mouseMoved:(NXEvent *)theEvent;
- mouseDragged:(NXEvent *)theEvent;
- rightMouseDragged:(NXEvent *)theEvent;
- mouseEntered:(NXEvent *)theEvent;
- mouseExited:(NXEvent *)theEvent;
- keyDown:(NXEvent *)theEvent;
- keyUp:(NXEvent *)theEvent;
- flagsChanged:(NXEvent *)theEvent;
- noResponderFor:(const char *)eventType;
- (BOOL)acceptsFirstResponder;
- becomeFirstResponder;
- resignFirstResponder;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

@end

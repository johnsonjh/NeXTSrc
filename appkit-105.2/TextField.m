/*
	TextField.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
  
	DEFINED IN: The Application Kit
	HEADER FILES: TextField.h 
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Control_Private.h"
#import "TextField.h"
#import "Matrix.h"
#import "TextFieldCell.h"
#import "Text.h"
#import "Window.h"
#import "publicWraps.h"
#import "nextstd.h"
#import <math.h>
#import <zone.h>

static id textFieldCellClass = nil;

@implementation TextField:Control

+ setCellClass:factoryId
{
    textFieldCellClass = factoryId;
    return self;
}


+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}


- initFrame:(const NXRect *)frameRect
{
    NXZone *zone = [self zone];

    [super initFrame:frameRect];
    [self setFlipped:YES];
    [self setClipping:NO];
    if (!textFieldCellClass) textFieldCellClass = [TextFieldCell class];
    if (_NXCanAllocInZone(textFieldCellClass, @selector(new), @selector(init))) {
	cell = [[textFieldCellClass allocFromZone:zone] init];
    } else {
	cell = [textFieldCellClass new];
    }
    [cell setAlignment:NX_LEFTALIGNED];
    [cell setStringValueNoCopy:""];
    [cell setBezeled:YES];
    [self setEditable:YES];
    vFlags.opaque = [cell isOpaque];

    return self;
}


- sizeTo:(float)width :(float)height
{
    NXCoord dx, dy;
    id sview, textObj;

    dx = width - bounds.size.width;
    dy = height - bounds.size.height;
    [super sizeTo:width :height];
    if (textObj = [self currentEditor]) {
	sview = [textObj superview];
	if (sview == self) {
	    [textObj sizeBy:dx :dy];
	} else if ([sview superview] == self) {
	    [sview sizeBy:dx :dy];
	} else {
	    [self abortEditing];
	    [self selectText:self];
	}
    }

    return self;
}

- setBackgroundColor:(NXColor)color
{
    [cell setBackgroundColor:color];
    return self;
}

- (NXColor)backgroundColor
{
    return [cell backgroundColor];
}

- setBackgroundTransparent:(BOOL)flag
{
    [cell setBackgroundTransparent:flag];
    vFlags.opaque = !flag;
    return self;
}

- (BOOL)isBackgroundTransparent
{
    return vFlags.opaque ? NO : YES;
}

- (BOOL)backgroundTransparent	/* historical */
{
    return [self isBackgroundTransparent];
}

- setTextColor:(NXColor)color
{
    [cell setTextColor:color];
    return self;
}

- (NXColor)textColor
{
    return [cell textColor];
}

- setBackgroundGray:(float)value
{
    [cell setBackgroundGray:value];
    if (value < 0.0) vFlags.opaque = NO;
    return self;
}


- (float)backgroundGray
{
    return[cell backgroundGray];
}


- setTextGray:(float)value
{
    [cell setTextGray:value];
    return self;
}


- (float)textGray
{
    return[cell textGray];
}


- setEnabled:(BOOL)flag
{
    [cell setEnabled:flag];
    return self;
}

- (BOOL)isBordered
{
    return[cell isBordered];
}


- setBordered:(BOOL)flag
{
    [cell setBordered:flag];
    return self;
}


- (BOOL)isBezeled
{
    return[cell isBezeled];
}


- setBezeled:(BOOL)flag
{
    [cell setBezeled:flag];
    return self;
}


- (BOOL)isEditable
{
    return[cell isEditable];
}


- setEditable:(BOOL)flag
{
    [cell setEditable:flag];
    return self;
}


- (BOOL)isSelectable
{
    return[cell isSelectable];
}


- setSelectable:(BOOL)flag
{
    [cell setSelectable:flag];
    return self;
}


- setPreviousText:anObject
{
    previousText = anObject;
    return self;
}


- setNextText:anObject
{
    nextText = anObject;
    if ([anObject respondsTo:@selector(setPreviousText:)] &&
	[anObject respondsTo:@selector(selectText:)]) {
	[anObject setPreviousText:self];
    }
    return self;
}


- mouseDown:(NXEvent *)theEvent
{
    id textEdit;

    if ([cell isEnabled] && [cell isSelectable]) {
	if ([window makeFirstResponder:self]) {
	    [window endEditingFor:self];
	    textEdit = [window getFieldEditor:YES for:self];
	    if (_NXGetShlibVersion() <= MINOR_VERS_1_0) [textEdit setCharFilter:NXFieldFilter];
	    conFlags.editingValid = NO;
	    [cell edit:&bounds inView:self editor:textEdit delegate:self event:theEvent];
	}
    } else {
	if (nextResponder) {
	    [nextResponder mouseDown:theEvent];
	} else {
	    [self noResponderFor:"mouseDown:"];
	}
    }

    return self;
}


- (SEL)errorAction
{
    return errorAction;
}


- setErrorAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "[Control -setAction:]");
    errorAction = aSelector;
    return self;
}



- selectText:sender
{
    id                  textEdit;

    if ([cell isEnabled] && [cell isSelectable]) {
	[window endEditingFor:self];
	textEdit = [window getFieldEditor:YES for:self];
	if (_NXGetShlibVersion() <= MINOR_VERS_1_0) [textEdit setCharFilter:NXFieldFilter];
	conFlags.editingValid = NO;
	[cell select:&bounds inView:self editor:textEdit delegate:self start:0 length:32000];
    } else if (sender && sender == nextText && sender != self) {
	[previousText selectText:self];
    } else if (nextText != self) {
	[nextText selectText:self];
    }
    return self;
}


- textDelegate
{
    return textDelegate;
}


- setTextDelegate:anObject
{
    textDelegate = anObject;
    return self;
}

#define BUF_SIZE 256

- (BOOL)textWillEnd:textObject
{
    char *string;
    int length,blength;
    char buffer[BUF_SIZE];
    BOOL retvalue;

    retvalue = NO;
    length = [textObject textLength];
    blength = [textObject byteLength];

    if (blength >= BUF_SIZE) {
	string = (char *)NXZoneMalloc([self zone], blength + 1);
    } else {
	string = buffer;
    }
    [textObject getSubstring:string start:0 length:length + 1];
    string[blength] = '\0';
    retvalue = ![cell isEntryAcceptable:string];
    if (blength >= BUF_SIZE) free(string);
    if (retvalue && errorAction) {
	[self sendAction:errorAction to:[cell target]];
    }
    if ([textDelegate respondsTo:@selector(textWillEnd:)]) {
	retvalue = [textDelegate textWillEnd:textObject] || retvalue;
    }

    if (retvalue) NXBeep();

    return retvalue;
}


- textDidEnd:textObject endChar:(unsigned short)whyEnd
{
    id retval = self;

    [self _validateEditing:textObject];
    if ([textDelegate respondsTo:@selector(textDidEnd:endChar:)]) {
	[textDelegate textDidEnd:textObject endChar:whyEnd];
    }
    [cell endEditing:textObject];

    [self display];

    switch (whyEnd) {
    case NX_RETURN:
	if ([self sendAction:[cell action] to:[cell target]]) {
	    break;
	}
    case NX_TAB:
	if (nextText) {
	    retval = [nextText selectText:self];
	} else {
	    [self selectText:self];
	}
	break;
    case NX_BACKTAB:
	if (previousText) {
	    retval = [previousText selectText:self];
	} else {
	    [self selectText:self];
	}
	break;
    }

    [window flushWindow];

    return retval;
}

- (BOOL)textWillChange:textObject
{
    if ([cell isEditable]) {
	if ([textDelegate respondsTo:@selector(textWillChange:)]) {
	    return[textDelegate textWillChange:textObject];
	} else {
	    return NO;
	}
    } else {
	return YES;
    }
}


- textDidChange:textObject
{
    if ([textDelegate respondsTo:@selector(textDidChange:)]) {
	return[textDelegate textDidChange:textObject];
    } else {
	return self;
    }
}


- text:textObject isEmpty:(BOOL)flag
{
    if ([textDelegate respondsTo:@selector(textDidGetKeys:isEmpty:)]) {
	return[textDelegate textDidGetKeys:textObject isEmpty:flag];
    } else if ([textDelegate respondsTo:@selector(text:isEmpty:)]) {
	return[textDelegate text:textObject isEmpty:flag];
    } else {
	return self;
    }
}

- textDidGetKeys:textObject isEmpty:(BOOL)flag
{
    return [self text:textObject isEmpty:flag];
}


- (BOOL)acceptsFirstResponder
{
    return [cell isSelectable];
}

- (BOOL)acceptsFirstMouse
{
    return YES;
}

- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteObjectReference(stream, nextText);
    NXWriteObjectReference(stream, previousText);
    NXWriteObjectReference(stream, textDelegate);
    NXWriteTypes(stream, ":", &errorAction);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    nextText = NXReadObject(stream);
    previousText = NXReadObject(stream);
    textDelegate = NXReadObject(stream);
    NXReadTypes(stream, ":", &errorAction);
    return self;
}


@end

/*
  
Modifications (since 0.8):
  
12/4/88  pah	fixed BACKTAB when going backwards to some other
		 multiple cell object (real fix should involve making
		 selectText a target/action method)
12/5/88  pah	added factory method setCellClass:
12/11/88 pah	added textDelegate and errorAction instance variables
		 along with textDelegate, setTextDelegate:,
		 errorAction and setErrorAction: methods
		changed textWillChange: and textDidEnd:endChar: to
		 support textDelegate and errorAction
		added textWillEnd:, textDidChange:, and text:isEmpty:
		 to support textDelegate
		added selectText: target/action version of selectText
		 retained selectText for backward compatibility
0.82
----
  1/5/89 pah	added isSelectable/setSelectable support
  1/6/89 pah	added autodisplay code for setBackgroundGray:, setTextGray:,
		 setAlignment:, setEneabled:, setBordered: and setBezeled:
         bgy	use selectAll: instead of selectAll in the text object
 1/10/89 pah	setStringValue:, et. al. do not autodisplay if field is
		 disabled
 1/12/89 pah	clicking in non-editable fields passes the mouseDown: down
		 the responder chain
 1/14/89 pah	removed selectText (non-target/action version of selectText:)
 1/15/89 pah	RETURN acts like TAB in TextField with no action
 1/25/89 trey	made setString* methods take const params
 1/27/89 bs	added read: write:
 2/01/89 trey	textDidEnd:endChar: sends endEditing to the cell AFTER
		 the textDelegate gets his textDidEnd:endChar: message
0.83
----
 3/15/89 pah	take out special case to not display if disabled (the user
		 can setAutodisplay:NO if that's what he wants).
 3/18/89 pah	drawSelf:: does not abortEditing now
		check that receiver understands selectText: (as well as
		 setPreviousText:) before sending setPreviousText:

0.93
----
 6/15/89 pah	move autodisplay code to TextFieldCell
 6/16/89 pah	properly check if field is valid via makeFirstResponder:

0.94
----
 7/13/89 pah	fix sizeTo:: to preserve editing state

0.96
----
 7/20/89 pah	replace all isEditable checks with isSelectable except
		 textWillChange:
		display the cell in textDidEnd:endChar:

80
--
 4/9/90 aozer	Added methods to set/get color values and
		setBackgroundTransparent:

87
--
 7/11/90 aozer	Added the inadvertently forgetten backgroundTransparent method

105
---
 11/6/90 glc	use bytelength not length
 

*/


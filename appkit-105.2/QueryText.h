/*

	QueryText.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#import "Text.h"

@interface QueryText : Text
{
    BOOL completionEnabled;	
    BOOL returnCompletes;	
}

+ newFrame:(NXRect *)frameRect text:(char *)theText alignment:(int)mode;
- initFrame:(NXRect *)frameRect text:(char *)theText alignment:(int)mode;

- setCharFilter:(NXCharFilterFunc)aFunc;
- setCompletionEnabled:(BOOL)flag ;
- (BOOL)returnCompletes ;
- setReturnCompletes:(BOOL)flag ;
- (BOOL)completionEnabled ;
- _completeFileName;
- keyDown: (NXEvent *) theEvent ;
- paste:sender;
@end

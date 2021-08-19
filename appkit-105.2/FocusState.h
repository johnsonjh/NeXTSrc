/*

	FocusState.h
	Copyright 1989, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#import "PSMatrix.h"

@interface FocusState : PSMatrix
{
    NXRect           theClip;
    BOOL	     clipSet;
    BOOL	     clipEmpty;
}

- clip:(const NXRect *)aRect;
- flush;
- reset;

@end



/*

	Alert.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#import <objc/Object.h>
#import "graphics.h"

@interface Alert : Object
{
    id	msg;
    id	first;
    id	second;
    id	third;
    id	panel;
    id	title;
    NXCoord buttonHeight, buttonSpacing;
    NXSize defaultPanelSize;
}

+ new;
- init;

- buttonPressed:sender;

@end


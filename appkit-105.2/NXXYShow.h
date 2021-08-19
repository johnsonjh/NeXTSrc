/*
	NXXYShow.h
	Responsibility: Paul Hegarty
	Copyright (c) 1988, 1989, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import <appkit/graphics.h>

@interface NXXYShow:Object
{
    @public
    int bufferSize;
    char *text, *curText;
    float *pos, *curPos;
    float curX, curY, gray;
    float *screenWidths;
    id font, screenFont;
    BOOL first, freeWidths;
    float startX, startY;
}

+ newFont:newFont gray:(float)gray;

- initFont:newFont gray:(float)gray;

- free;

- finalize;

- currentPoint:(NXCoord *)x :(NXCoord *)y;
- moveTo:(NXCoord)x :(NXCoord)y;
- show:(const char *)string;
- show:(const char *)string count:(int)count;

@end





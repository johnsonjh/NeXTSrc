/*

	NXFaxCoverView.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Chris Franklin
	  
	DEFINED IN: The Application Kit
	HEADER FILES: NXFaxCoverView.h
*/

#import "View.h"

@interface NXFaxCoverView:View
{
    NXRect textRect;
    NXCoord lineHeight;
    NXRect graphicsRect;
    id cache;
    float bboxPS[4];
    char *dataPS;
    int lengthPS;
}

- init;
- setCoverText : aTextObject at: (NXCoord) x : (NXCoord) y;
- setBackgroundData : (char *) data  length : (int) length bbox : (float *) bbox;
- setGraphicsRect : (const NXRect *) aRect;
- setTextRect : (const NXRect *) aRect lineHeight : (NXCoord) height;
- graphicsRect : (NXRect *) aRect;
- textRect : (NXRect *) aRect;
- drawGraphics;
@end


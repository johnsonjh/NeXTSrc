#import "View.h"
#import <sys/param.h>

#define NUMCOLORS 200

@interface NXColorSwatch : View
{
	id 		colorPicker;
	int 		currentswatch,mousedColor;
	NXColor 	colors[NUMCOLORS];
	char           *filename;
}

+ newFrame:(NXRect const *)theFrame;

- drawSelf:(NXRect *)rects :(int)rectCount;
- drawColor:(int)num;
- setColorPicker:anObject;
- (NXColor)color ;
- setFilename:(char *)filen;
- writeColors;
- readColors;

@end

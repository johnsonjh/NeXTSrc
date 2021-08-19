#import "View.h"


@interface NXColorWheel : View
{
	id wheelbitmap,maskbitmap;
	float brightness;
	NXPoint currentPt;
	id colorBrightSlider,colorPicker;
	unsigned char *image; 
	//put instance variables concerning state of selection
}

+ newFrame:(NXRect const *)theFrame;
- initFrame:(NXRect const *)theFrame;

- drawSelf:(NXRect *)rects :(int)rectCount;
- render;
- drawPosition;
- resize;
- pointForColor:(NXColor *)color:(NXPoint *)pt;
- renderWheel:(float)bright;
- (BOOL)pointInCircle:(NXPoint *)pt;
- showColor:(NXColor *)color;
- currentColor:(NXColor *)color;
- currentHue:(NXColor *)color;
- setBrightness:sender;
- setColorBrightSlider:anObject;
- mouseDown:(NXEvent *)theEvent;

@end

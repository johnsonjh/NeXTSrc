/*
	NXColorWheel.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "color.h"
#import "colorPickerPrivate.h"
#import "NXColorWheel.h"
#import "NXColorPicker.h"
#import "NXColorSlider.h"
#import "Application.h"
#import "NXImage.h"
#import "Button.h"
#import "Form.h"
#import "Window.h"
#import "tiff.h"
#import "nextstd.h"
#import <dpsclient/wraps.h>
#import <libc.h>
#import <sys/dir.h>
#import <fcntl.h>
#import <math.h>
#import <mach.h>

#define POINTER_SIZE 4.0
#define POINTER_SIZEd2 4.0/2.0

#define INTRND(x) ((int)((((x)-floor(x))<=.5)?floor(x):ceil(x)))

#define NXNotify(msg) _NXKitAlert("ColorPanel","Alert",msg, "OK", NULL, NULL)

#define IMAGESIZE(w,h,b,s) (((int)(s)) * (((int)(h))*((((int)(w))*((int)b)+7)/8)))

#define PI (double)(3.1415926535897932384626433)	

@implementation  NXColorWheel:View

static float arctangent(float y, float x)
{
    float a;

    if (y == 0) {
	if (x >= 0) return 0.0;
	return (float)PI;
    }
    if (x == 0) {
	if (y >= 0) return (float)(PI/2);
	return (float)(3*PI/2);
    }
    a = atan(y/x);
    if (x > 0) {
	if (y > 0) return a;
	return (float)(2*PI + a);
    }

    return (float)(PI + a);
}

+ newFrame:(const NXRect *)theFrame
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:theFrame];
}

- initFrame:(const NXRect *)theFrame
{
    NXSize newSize;
    brightness = 1.0;
    currentPt.x = floor(bounds.size.width/2.0);
    currentPt.y = floor(bounds.size.height/2.0);
    [self setFrame:theFrame];
    image = (unsigned char *)NXZoneMalloc([self zone], IMAGESIZE(frame.size.width, frame.size.height, 8, 4));
    newSize.width = theFrame->size.width;
    newSize.height = theFrame->size.height;    

    wheelbitmap = [NXImage findImageNamed:"NXcolor_wheel"];
    maskbitmap = [[NXImage allocFromZone:[self zone]] initSize:&newSize];
    [maskbitmap setCacheDepthBounded:NO];
    [maskbitmap setFlipped:NO];

    [self renderWheel:brightness];
    [self render];

    [window addToEventMask:NX_MOUSEDOWNMASK|NX_MOUSEDRAGGEDMASK];

    return self;
}

- (BOOL)acceptsFirstMouse
{
    return YES;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    PSsetgray(NX_LTGRAY);
    NXRectFill(rects);
    [maskbitmap composite:NX_SOVER fromRect:rects toPoint:&rects->origin];
    [self drawPosition];
    return self;
}
- compositeWheel
{
    NXPoint origin = {0.0,0.0};
    [self lockFocus];
    [maskbitmap composite:NX_SOVER toPoint:&origin];
    [self drawPosition];
    [self unlockFocus];
    return self;
}
- render
{
    NXPoint origin = {0.0, 0.0};
    
    [maskbitmap lockFocus];
    [wheelbitmap composite:NX_COPY toPoint:&origin];
    PSsetalpha(1.0 - brightness);
    PSsetgray(NX_BLACK);
    PScompositerect(0.0, 0.0, bounds.size.width, bounds.size.height, NX_SATOP);
    [maskbitmap unlockFocus];

    return self;
}

- drawPosition
{
    PSsetgray(NX_BLACK);
    PSrectfill(currentPt.x-POINTER_SIZEd2, currentPt.y-POINTER_SIZEd2, POINTER_SIZE, POINTER_SIZE);
    PSsetgray(NX_WHITE);
    PSrectfill(currentPt.x-1.0, currentPt.y-1.0, POINTER_SIZEd2, POINTER_SIZEd2);
    return self;
}

- resize
{
    NXSize sz,newSize;

    [wheelbitmap getSize:&sz];
    if (sz.width == frame.size.width && sz.height == frame.size.height) return self;
    newSize.width = frame.size.width;
    newSize.height = frame.size.height;    
    [wheelbitmap setSize:&newSize];
    [maskbitmap setSize:&newSize];
    if (image) {
	free(image);
	image = NULL;
    }
    image = (unsigned char *)NXZoneMalloc([self zone], IMAGESIZE(frame.size.width,frame.size.height,8,4));
    [self renderWheel:1.0];

    return self;
}

- pointForColor:(NXColor *)color :(NXPoint *)pt
{
    int width, height;
    float center, theta, h, s, b, ah, as;
    
    h = NXHueComponent(*color);
    s = NXSaturationComponent(*color);
    b = NXBrightnessComponent(*color);
    ah = 2.0;
    as = 2.0;
    
    width = height = MIN(frame.size.width, frame.size.height);
    center = width / 2.0;
    
    theta = 2 * PI * h;
    pt->x = floor((center * cos(theta) * s) + center);
    pt->y = floor((center * sin(theta) * s) + center);
    
    return self;
    
}

- renderWheel:(float)bright
{
    NXRect r;
    int width, height, x, y, size;
    float center, warp;
    unsigned char *local = image;
    float rr, rry, theta;
    float red, green, blue, hue, sat, bri;
    
    width = frame.size.width;
    height = frame.size.height;
    size = IMAGESIZE(width, height, 8, 1);
    
    width = height = MIN(width, height);
    [wheelbitmap lockFocus];
    center = width/2.0;
    warp = center*center;
    for (y = 0;y < height;y++) {
	rry = (y-center)*(y-center);
	for(x = 0; x < width; x++) {
	    rr = rry + (x-center)*(x-center);
	    if (rr > warp) {
		*(local++) = 0;
		*(local++) = 0;
		*(local++) = 0;
		*(local++) = 0;
	    } else {
		theta = arctangent(center-y, x-center);		  
		hue = theta/(2*PI);
		sat = sqrt(rr/warp);
		bri = bright;
		_NXHSBToRGB(hue, sat, bri, &red, &green, &blue);
		*(local++) = (int)(255*red);
		*(local++) = (int)(255*green);
		*(local++) = (int)(255*blue);
		*(local++) = (int)(255);
	    }
	}
    }
    r.size.width = r.size.height = width;
    _NXImageBitmapWithFlip(&r, width, height, 8, 4, NX_MESHED, NX_COLORMASK|NX_ALPHAMASK,image, NULL, NULL, NULL, NULL,NO);
    [wheelbitmap unlockFocus];

    return self;
}

- (BOOL)pointInCircle:(NXPoint *)pt
{
    int width,height;
    float center, warp, rr, rrx;
    
    width = height = MIN(frame.size.width, frame.size.height);
    center = width/2.0;
    warp = center*center;
    rrx = (pt->x-center)*(pt->x-center);
    rr = rrx + (pt->y-center)*(pt->y-center);

    return (rr <= warp);    
}

- showColor:(NXColor *)color
{
    NXColor ncolor;
    NXRect rects;
    
    NXSetRect(&rects,currentPt.x - POINTER_SIZEd2, currentPt.y - POINTER_SIZEd2, POINTER_SIZE, POINTER_SIZE);
    [self pointForColor:color:&currentPt];
    brightness = NXBrightnessComponent(*color);
    [self render];
    [self display];
    [self currentHue:&ncolor];
    [colorBrightSlider setSoverSlider:1:&ncolor];
    [colorBrightSlider setFloatColor:NXBrightnessComponent(*color)];
    [window flushWindow];
    
    return self;
}

- currentColor:(NXColor *)color
{
    unsigned char *pos;
    float r,g,b,br;

    pos = image+((int)currentPt.x+(int)(frame.size.width*(frame.size.height-currentPt.y)))*4;
    br = brightness/255.0;
    r = (*(pos))*br;
    g = (*(pos+1))*br;
    b = (*(pos+2))*br;
    *color = NXConvertRGBAToColor(r,g,b,1.0);

    return self;
}

- currentHue:(NXColor *)color
{
    unsigned char *pos;
    float r,g,b;

    pos = image+((int)currentPt.x+(int)(frame.size.width*(frame.size.height-currentPt.y)))*4;
    r = ((*(pos))/255.0);
    g = ((*(pos+1))/255.0);
    b = ((*(pos+2))/255.0);
    *color = NXConvertRGBAToColor(r,g,b,1.0);

    return self;
}

- setBrightness:sender
{
    NXColor color;

    brightness = [sender getFloatColor];
    [self render];
    [self compositeWheel];
    [window flushWindow];
    [self currentColor:&color];
    [colorPicker setAndSendButDontDisplay:color:0];

    return self;
}

- setColorBrightSlider:anObject
{
    int st[4] = {0,0,0,255}, en[4] = {255,255,255,255};

    colorBrightSlider = anObject;
    [colorBrightSlider setTitle:" "];
    [colorBrightSlider setTextWidth:0];
    [colorBrightSlider setTitleColor:1.0];
    [colorBrightSlider setGradation:st:en];
    [colorBrightSlider setAction:@selector(setBrightness:)];
    [colorBrightSlider setTarget:self];
    [colorBrightSlider setUnits:1];

    return self;
}

- setColorPicker:anObject
{
    colorPicker = anObject;
    return self;
}

- mouseDown:(NXEvent *)theEvent
{
    NXPoint mouseLocation, oPt,cPt;
    NXColor color;
    BOOL loop = YES;
    NXRect pointer;
    float t;
    NXEvent *nextEvent;

    mouseLocation = theEvent->location; 
    [window addToEventMask:NX_MOUSEDRAGGEDMASK];
    [self convertPoint:&mouseLocation fromView:nil];
    oPt = mouseLocation;
    cPt.x = floor(frame.size.width/2.0);
    cPt.y = floor(frame.size.height/2.0);
    
    NXSetRect(&pointer, currentPt.x - POINTER_SIZEd2, currentPt.y - POINTER_SIZEd2, POINTER_SIZE, POINTER_SIZE);
    if (![self pointInCircle:&mouseLocation]) return self;
    currentPt = mouseLocation;
    [self display:&pointer :1];
    [self currentHue:&color];
    [colorBrightSlider setSoverSlider:1:&color];
    [self currentColor:&color];
    [window flushWindow];

    while (loop){
	nextEvent = [NXApp getNextEvent:(NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK)];
	switch (nextEvent->type) {
	case NX_LMOUSEUP:
	    loop = NO;
	    [self currentColor:&color];
	    [colorPicker setAndSendButDontDisplay:color:0];
	    break;
	default:
	    mouseLocation = nextEvent->location; 
	    [self convertPoint:&mouseLocation fromView:nil];
	    NXSetRect(&pointer,currentPt.x - POINTER_SIZEd2, currentPt.y - POINTER_SIZEd2, POINTER_SIZE, POINTER_SIZE);
	    if ([self pointInCircle:&mouseLocation]) {
		if (theEvent->flags & NX_SHIFTMASK) {
		    if ((cPt.x - oPt.x == 0.0) && (cPt.y - oPt.y == 0.0)) {
			currentPt = cPt;
		    } else {
			t = ((cPt.x - oPt.x) * (cPt.x - mouseLocation.x) + (cPt.y - oPt.y) * (cPt.y - mouseLocation.y)) /
			    ((cPt.x - oPt.x) * (cPt.x - oPt.x) + (cPt.y - oPt.y) * (cPt.y - oPt.y));
			currentPt.x = floor(cPt.x + t * (oPt.x - cPt.x));
			currentPt.y = floor(cPt.y + t * (oPt.y - cPt.y));
		    }
		} else {
		    currentPt = mouseLocation;
		}
		[self display:&pointer:1];
		[self currentHue:&color];
		[colorBrightSlider setSoverSlider:1:&color];
		[self currentColor:&color];
		[window flushWindow];
		NXPing();
		[colorPicker setAndSendButDontDisplay:color:0];
	    }
	}
    }

    return self;
}	
	

@end


/*
    
4/1/90 kro	new class for beginner mode to draw the color wheel
4/16/90 keith   fixed updating of wheel and dragging mouse loop
		added (SHIFT-constrain) of saturation
appkit-85
---------
 6/2/90 keith	fixed bug that did not update the wheel after a 
 		color was chosen.	
 
appkit-90
----------
8/4/90		Greg added zone stuff. Code was finessed ala paul and keith design review

appkit-95
----------
10/2/90		 fixed double display and send message bug
*/
/*
	NXColorSlider.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "NXColorSlider.h"
#import "Application.h"
#import "NXImage.h"
#import "Font.h"
#import "Text.h"
#import "TextField.h"
#import "colorPickerPrivate.h"
#import "color.h"
#import "tiff.h"
#import "nextstd.h"
#import <objc/hashtable.h>
#import <dpsclient/wraps.h>
#import <math.h>
#import <libc.h>
#import <zone.h>

#define TEXTSIZE 12.0
#define NUMWIDTH 6.0
#define NUMHEIGHT 8.0
#define LETWIDTH 5.0
#define LETHEIGHT 7.0
#define TEXTWIDTH 30.0
#define TEXTHEIGHT 14.0
#define KNOBSIZE 10.0
#define SLIDERBORDER KNOBSIZE/2.0
#define HORIZONTAL (frame.size.width > frame.size.height)
#define DEFAULT_FONT KitString(ColorPanel,"Helvetica","default font")

static const int default_startcolor[4] = {0,0,0,255};
static const int default_endcolor[4] = {255,255,255,255};
#define DEFAULT_TITLEGRAY	0.0

#define IMAGESIZE(w,h,b,s) (((int)(s)) * (((int)(h))*((((int)(w))*((int)b)+7)/8)))

@implementation  NXColorSlider:Control

+ newFrame:(NXRect const *)theFrame
{
    return [[self alloc] initFrame:theFrame];
}

- initFrame:(NXRect const *)theFrame
{
    id fontObj;
    NXRect aRect,textFrame;
    NXSize newSize;

    aRect = *theFrame;
    [self setFrame:theFrame];
    enabled = YES;
    textwidth = TEXTWIDTH;
    titlegray = DEFAULT_TITLEGRAY;
    bcopy(default_startcolor, start_color, sizeof(int) * 4);
    bcopy(default_endcolor, end_color, sizeof(int) * 4);
    
    if (theFrame->size.width > theFrame->size.height){
        newSize.width = theFrame->size.width + KNOBSIZE;
        newSize.height = theFrame->size.height;    
	bitmap = [[NXImage allocFromZone:[self zone]] initSize:&newSize];
	[bitmap setCacheDepthBounded:NO];
    } else {
        newSize.width = theFrame->size.width;
        newSize.height = theFrame->size.height + KNOBSIZE;    
	bitmap = [[NXImage allocFromZone:[self zone]] initSize:&newSize];
	[bitmap setCacheDepthBounded:NO];
    }
    [bitmap setFlipped: NO];
    NXSetRect(&textFrame, theFrame->size.width - textwidth,
			  floor((theFrame->size.height - TEXTHEIGHT) / 2.0),
			  textwidth, TEXTHEIGHT);
    fontObj = [Font newFont:DEFAULT_FONT size:10.0];
    valueField = [[TextField allocFromZone:[self zone]] initFrame:&textFrame];
    [[[[[[[valueField setFont: fontObj] setEditable:NO] setBezeled:NO] setBordered:NO] setBackgroundGray:NX_LTGRAY] setTextGray:NX_BLACK] setAlignment:NX_CENTERED];
    [self addSubview:valueField];

    return self;
}

- setEnabled:(BOOL)flag
{
   if ((enabled && !flag) || (!enabled && flag)) {
	enabled = flag ? YES : NO;
	[self update];
    }
    return self;
}

- setTitle:(const char *)title
{
    if (title) {
	free(colortitle);
	colortitle = NXCopyStringBufferFromZone(title, [self zone]);
    }
    return self;
}

- setTextWidth:(int)tw
{
    textwidth = tw;
    if (!tw) [valueField removeFromSuperview];
    [self resize: &frame];
    return self;
}

- setTitleColor:(float)col
{
    titlegray = col;
    return self;
}

- setHSBSlider:(int)ishue
{
    ishueslider = ishue;
    return self;
}  

- setSoverSlider:(int)isit :(NXColor *)thecolor
{
    soverslider = isit;
    sovercolor = *thecolor;
    [self update];
    return self;
}

- resize:(const NXRect *)newRect
{
    NXSize newSize;
    [self setFrame:newRect];
    if (newRect->size.width>newRect->size.height){
        newSize.width = newRect->size.width + KNOBSIZE;
        newSize.height = newRect->size.height;    
	[bitmap setSize:&newSize];
    } else {
        newSize.width = newRect->size.width;
        newSize.height = newRect->size.height+KNOBSIZE;    
	[bitmap setSize:&newSize];
    }
    if (textwidth) {
	[valueField sizeTo:textwidth: TEXTHEIGHT];
	[valueField moveTo:newRect->size.width - TEXTWIDTH :floor((newRect->size.height - TEXTHEIGHT) / 2.0)];
    }
    [self renderBitmap:newRect];
    return self;
}

- renderGradation:(int *)start:(int *)end:(NXRect *)fr:(float)endbuf:(int)type
{
    int x;
    NXRect im;
    NXColor ncolor;
    float r,g,b,width;
    unsigned char *grad,*ptr;
    
    width = fr->size.width - endbuf * 2.0;
    if(width < 0)
    width = 1;
    grad = NXZoneMalloc(NXDefaultMallocZone(), IMAGESIZE(width, 1, 8, 3));
    r = (end[0] - start[0]) / width;
    g = (end[1] - start[1]) / width;
    b = (end[2] - start[2]) / width;

    if (type) {
	PSsethsbcolor(ITOF(start[0]), ITOF(start[1]), ITOF(start[2]));
    } else {
	PSsetrgbcolor(ITOF(start[0]), ITOF(start[1]), ITOF(start[2]));
    }
    PSrectfill(0.0, 0.0, endbuf, fr->size.height);	      
    ptr = grad;
    for (x = 0; x < width; x++) {
	if (type) {
	    ncolor = NXConvertHSBAToColor(ITOF(start[0] + r*x), ITOF(start[1] + g*x), ITOF(start[2] + b*x), 1.0);
	    *(ptr++) = FTOI(NXRedComponent(ncolor));
	    *(ptr++) = FTOI(NXGreenComponent(ncolor));
	    *(ptr++) = FTOI(NXBlueComponent(ncolor));
	} else {
	    *(ptr++) = start[0] + r*x;
	    *(ptr++) = start[1] + g*x;
	    *(ptr++) = start[2] + b*x;
	}
    }	
    NXSetRect(&im, endbuf, 0.0, width, fr->size.height);
    _NXImageBitmapWithFlip(&im, (int)width, 1, 8, 3, NX_MESHED, NX_COLORMASK, grad, NULL, NULL, NULL, NULL,NO);
    free(grad);
    if (type) {
	PSsethsbcolor(ITOF(end[0]), ITOF(end[1]), ITOF(end[2]));
    } else {
	PSsetrgbcolor(ITOF(end[0]), ITOF(end[1]), ITOF(end[2]));
    }
    PSrectfill(endbuf + width, 0., endbuf, fr->size.height);	
          
    return self;
}

static void drawBezel(float x,float y,float w,float h)
{
    PSgsave();
    PStranslate(x, y);
    PSsetgray(NX_DKGRAY);
    PSrectfill(0., 0., w, h);
    PSsetgray(NX_WHITE);
    PSrectfill(1.0, 0.0, w-1.0, h-1.0);
    PSsetgray(NX_BLACK);
    PSrectfill(1.0, 1.0, w-2.0, h-2.0);
    PSsetgray(NX_LTGRAY);
    PSrectfill(2.0, 1.0, w-3.0, h-3.0);
    PSgrestore(); 
    return;
}

static void drawKnob(float x, float y, float w, float h)
{
    PSgsave();
    PStranslate(x, y);
    PSsetgray(NX_BLACK);
    PSrectfill(0.0, 0.0, w,h);
    PSsetgray(NX_WHITE);
    PSrectfill(0.0, 1.0, w-1.0, h-1.0);
    PSsetgray(NX_DKGRAY);
    PSrectfill(1.0, 1.0, w-2.0, h-2.0);
    PSsetgray(NX_LTGRAY);
    PSrectfill(1.0, 2.0, w-3.0, h-3.0);
    PSgrestore();
    return;
}

- renderBitmap:(const NXRect *)tf
{
    NXRect im;
     
    /* RENDER THE GRADATION IN THE BUFFER BITMAP */

    [bitmap lockFocus];

    PSsetgray(NX_LTGRAY);
    PSrectfill(0.0, 0.0, tf->size.width, tf->size.height);
    if (HORIZONTAL) {
        drawBezel(0.0, 0.0, tf->size.width - (textwidth + 2.), tf->size.height);
    } else {
        drawBezel(0.0, 0.0, tf->size.width, tf->size.height - (textwidth + 2.0));
    }

    PSgsave();
    PStranslate(2.0, 2.0);
    if (HORIZONTAL) {
	NXSetRect(&track, 2.0, 2.0, tf->size.width - (textwidth + 2.0) - 4.0, tf->size.height - 4.0);
	dist = tf->size.width - (textwidth + 2.0) - 4.0 - 10.0;	
	NXSetRect(&im, 0.0, 0.0, dist + 10.0, tf->size.height - 4.0);
	[self renderGradation: start_color: end_color: &im: 5.0: ishueslider];
    } else {
	NXSetRect(&track, 2.0, 2.0, tf->size.width - 4.0, tf->size.height - (textwidth + 2.0) - 4.0);
	dist = tf->size.height - (textwidth + 2.0) - 4.0 - 10.0;	
	PStranslate(tf->size.width - 4.0, 0.0);
	PSrotate(90.0);
	NXSetRect(&im, 0.0, 0.0, dist + 10.0, tf->size.width - 4.0);
	[self renderGradation: start_color: end_color: &im: 5.0: ishueslider];
    }
    PSgrestore();
       
    /* RENDER THE KNOBBY AT THE END OF THE BITMAP */

    if (HORIZONTAL) {
	drawKnob(frame.size.width, 0.0, 10.0, frame.size.height-2.0);
    } else {
	drawKnob(0.0, frame.size.height, frame.size.width - 2.0, 10.0);
    }
    
    /* RENDER THE TEXT IN THE SLIDER */

    PSgsave();
    PSsetgray(titlegray);
    DPSPrintf(DPSGetCurrentContext(), "/%s findfont %f scalefont setfont ",DEFAULT_FONT, TEXTSIZE);
    if (HORIZONTAL){
        PSmoveto(5.0, floor((tf->size.height - ( TEXTSIZE * .75 )) / 2.0));
    } else {
	PStranslate(tf->size.width, 0.0);
	PSrotate(90.0);
	PStranslate(floor((tf->size.width - (TEXTSIZE * .75)) / 2.0), 5.0);
	PSmoveto(0.0, 0.0);
    }
    DPSPrintf(DPSGetCurrentContext()," (%s) show ", colortitle ? colortitle : KitString(ColorPanel,"Shade","default slider title"));
    PSgrestore();

    [bitmap unlockFocus];

    return self;

}

- setGradation:(int *)start:(int *)end
{
    int x;
    for (x = 0; x < 4; x++) {
       start_color[x] = start[x];
       end_color[x] = end[x];
    }
    return self;
}

- newGradation:(int *)start:(int *)end
{
    int x;
    NXRect im;
    float height;
    
    height = (frame.size.height > frame.size.width) ? frame.size.width : frame.size.height;
    for (x = 0; x < 4; x++) {
       start_color[x] = start[x];
       end_color[x] = end[x];
    }

    [bitmap lockFocus];

    if (frame.size.height > frame.size.width) {
        PStranslate(frame.size.width, 0.0);
	PSrotate(90.0);
    }

    PSgsave();
    PStranslate(2.0, 2.0);
    NXSetRect(&im, 0.0, 0.0, dist + 10.0, height - 4.0);
    [self renderGradation: start: end: &im: 5.0: ishueslider];
    PSgrestore();
    
    PSgsave();
    PSsetgray(titlegray);
    DPSPrintf(DPSGetCurrentContext(), "/%s findfont %f scalefont setfont ",DEFAULT_FONT, TEXTSIZE);
    PSmoveto(5.0, floor((height - (TEXTSIZE * .75)) / 2.0));
    DPSPrintf(DPSGetCurrentContext(), " (%s) show ", colortitle ? colortitle : KitString(ColorPanel,"Shade","default slider title"));
    PSgrestore();

    [bitmap unlockFocus];
    
    return self;
}

- (BOOL) acceptsFirstMouse
{
    return YES;
}

- colorbitmap 
{
    return bitmap;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    NXPoint pt = {0.0,0.0};
    
    if (cur.width != frame.size.width) [self resize: &frame];
    [bitmap composite:NX_COPY toPoint:&pt];
    if (enabled) {
	if (soverslider) {
	    NXSetColor(sovercolor);
	    PScompositerect(2.0, 2.0, track.size.width, track.size.height, NX_PLUS);
	}
	[self blitSelf];  
    } else {
        PSsetgray(NX_LTGRAY);
	PScompositerect(track.origin.x, track.origin.y, track.size.width, track.size.height,NX_PLUSL);
    }
    cur.width = frame.size.width;
    
    return self;
}

- setUnits:(int)un
{
    units = un;
    return self;
}

- blitSelf
{
    NXRect r;
    NXPoint pt = {0.0,0.0};
    
    if (![self canDraw]) return self;

    [self lockFocus];
    if (HORIZONTAL) {
	pt.x = ceil((track.origin.x + 5.0) + (track.size.width - 10.0) *oldvalue) - 5.0;
	NXSetRect(&r, pt.x, 0.0, 10.0, frame.size.height);
    } else {
        pt.y = ceil((track.origin.y + 5.0) + (track.size.height - 10.0) * oldvalue) - 5.0;
        NXSetRect(&r, 0.0, pt.y, frame.size.width, 10.0);
    }
    [bitmap composite:1 fromRect: &r toPoint: &pt];
    if (soverslider) {
        if (HORIZONTAL){
	    pt.y += 2.0;
	    r.size.height -= 4.0;
	} else {
	    pt.x += 2.0;
	    r.size.width -= 4.0;
	}
	NXSetColor(sovercolor);
	PScompositerect(pt.x, pt.y, r.size.width, r.size.height, NX_PLUS);
    }
    if (HORIZONTAL) {
	NXSetRect(&r, frame.size.width, 0.0, 10.0, frame.size.height - 2.0);
	pt.x = ceil((track.origin.x + 5.0) + (track.size.width - 10.0) * value) - 5.0;
	pt.y = 1.0;
    } else {
	NXSetRect(&r, 0.0, frame.size.height, frame.size.width - 2.0, 10.0);
	pt.y = ceil((track.origin.y + 5.0) + (track.size.height - 10.0) * value) - 5.0;
	pt.x = 1.0;
    }
    [bitmap composite:1 fromRect:&r toPoint:&pt];
    if (textwidth) {
	if (units) [valueField setIntValue:(int)(value*100)];
	else [valueField setIntValue:FTOI(value)];
    }
    [self unlockFocus];
    
    return self;
}

- (float)valueForMouseLocation:(const NXPoint *)pt
{
    if (HORIZONTAL) {
        return(pt->x - (track.origin.x + KNOBSIZE / 2.0)) / (track.size.width - KNOBSIZE);
    } else {
        return(pt->y - (track.origin.y + KNOBSIZE / 2.0)) / (track.size.height - KNOBSIZE);
    }
}

- knobRect:(NXRect *)kr forValue:(float)val
{
    if (HORIZONTAL) {
	kr->origin.x = ceil(track.origin.x + (track.size.width - 10.0) * val);
	kr->origin.y = 1.0;
	kr->size.width = 10.0;
	kr->size.height = frame.size.height - 2.0;
    } else {
	kr->origin.x = 1.0;
	kr->origin.y = ceil(track.origin.y + (track.size.height - 10.0) * val);
	kr->size.width = frame.size.width - 2.0;
	kr->size.height = 10.0;
    }
    return self;    
}

- mouseDown:(NXEvent *)theEvent
{
    int startval;
    BOOL loop = YES;
    NXEvent *nextEvent;
    NXPoint mouseLocation;
    float x, y, oval, sval, nval;
    
    if (!enabled) return self;
    
    mouseLocation = theEvent->location; 
    [window addToEventMask:NX_LMOUSEDRAGGEDMASK];
    [self convertPoint: &mouseLocation fromView: nil];
    oldvalue = value;
    startval = FTOI(value);
    x = mouseLocation.x; y = mouseLocation.y;
    [self knobRect:&knob forValue: value];
    
    if ((theEvent->flags & NX_ALTERNATEMASK) && NXPointInRect(&mouseLocation, &knob)) {
	while (loop) {
	    nextEvent = [NXApp getNextEvent:(NX_MOUSEUPMASK | NX_MOUSEDRAGGEDMASK)];
	    switch(nextEvent->type) {
	    case NX_LMOUSEUP:
		loop = NO;
		[self sendAction: _action to: _target];
		break;
	    default:
		oldvalue = value;
		mouseLocation = nextEvent->location; 
		[self convertPoint: &mouseLocation fromView: nil];
		value = HORIZONTAL ? ITOF(startval + mouseLocation.x - x) : ITOF(startval + mouseLocation.y - y);
		if (value > 1.0) {
		    value = 1.0;
		} else if (value < 0.0) {
		    value = 0.0;
		}
		[self blitSelf];  
		NXPing();
		[self sendAction: _action to: _target];
	    }
	}
    } else if (NXPointInRect(&mouseLocation, &knob)) {
	oval = value;
	sval = [self valueForMouseLocation: &mouseLocation];
	while (loop) {
	    nextEvent = [NXApp getNextEvent:(NX_LMOUSEUPMASK | NX_LMOUSEDRAGGEDMASK)];
	    switch(nextEvent->type) {
	    case NX_LMOUSEUP:
		loop = NO;
		[self sendAction: _action to: _target];
		break;
	    default:
		mouseLocation = nextEvent->location; 
		[self convertPoint: &mouseLocation fromView: nil];
		nval = [self valueForMouseLocation: &mouseLocation];
		oldvalue = value;
		value = oval + (nval - sval);
		if (value > 1.0) {
		    value = 1.0;
		} else if (value < 0.0) {
		    value = 0.0;
		}
		[self blitSelf];  
		NXPing();
		[self sendAction: _action to: _target];
	    }
	}
    } else if (NXPointInRect(&mouseLocation, &track)) {
	value = [self valueForMouseLocation:&mouseLocation];
	if (value > 1.0) {
	    value = 1.0;
	} else if (value < 0.0) {
	    value = 0.0;
	}
	[self blitSelf];  
	NXPing();
	[self sendAction:_action to:_target];
	[self mouseDown:theEvent];
    }

    return self;
}	

- (int)getIntColor
{
    return FTOI(value);
}

- (float)getFloatColor
{
    return value;
}

- setIntColor:(int)col
{
    if (value != ITOF(col)) {
	value = ITOF(col);
	[self update];
    }
    return self;
}

- setFloatColor:(float)col
{
    if (value != col) {
	value = col;
	[self update];
    }
    return self;
}

- setTarget:anObject
{
    _target = anObject;
    return self;
}

- setAction:(SEL) aSelector
{
    _action = aSelector;
    return self;
}

@end


/*
    
4/1/90 kro	made vertical slider work
		Fixed knob dragging

appkit-80
---------
 4/8/90 keith	cleaned up the tracking loop.	
 
appkit-90
----------
8/4/90	keith	Greg added zone stuff. Code was finessed after paul and keith design review

98
--
 10/12/90 aozer	Zonified uses of NXImage

*/


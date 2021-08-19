/*
	NXColorPanel.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Window_Private.h"
#import "Panel_Private.h"
#import "View_Private.h"
#import "NXColorPanel.h"
#import "NXColorPicker.h"
#import "NXColorWell.h"
#import "Application.h"
#import "Box.h"
#import "colorPickerPrivate.h"
#import "publicWraps.h"
#import "privateWraps.h"
#import <dpsclient/wraps.h>
#import <objc/List.h>
#import <libc.h>
#import <math.h>
#import <mach.h>
#import <zone.h>

#define DEFAULTX 607.0
#define DEFAULTY 613.0
#define DEFAULTWIDTH 317.0 
#define DEFAULTHEIGHT 168.0 
#define DEFAULTFRAMEWIDTH 319.0 
#define DEFAULTFRAMEHEIGHT 200.0 
#define COLORWIDTH 12.0
#define COLORHEIGHT 12.0
#define MAXWIDTH 369.0
#define MAXADDHEIGHT 92.0

@interface NXColorPanelPrivate
- _initContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag colorMask:(int)colormask;
@end

@implementation NXColorPanel

static id colorWindow = nil;
static id sharedColorPanel = nil;

+ (BOOL)_canAlloc { return NO; }

+ allocFromZone:(NXZone *)zone { return [self doesNotRecognize:_cmd]; }

+ alloc { return [self doesNotRecognize:_cmd]; }


+ new
{
    NXRect theFrame = {{DEFAULTX,DEFAULTY}, {DEFAULTWIDTH,DEFAULTHEIGHT}};

    return [self newContent:&theFrame
	    style:NX_RESIZEBARSTYLE
	    backing:NX_BUFFERED
	    buttonMask:NX_CLOSEBUTTONMASK
	    defer:NO];
}

+ newColorMask:(int)colormask
{
    NXRect theFrame = {{DEFAULTX,DEFAULTY}, {DEFAULTWIDTH,DEFAULTHEIGHT}};

    return [self newContent:&theFrame
	    style:NX_RESIZEBARSTYLE
	    backing:NX_BUFFERED
	    buttonMask:NX_CLOSEBUTTONMASK
	    defer:NO
	    colorMask:colormask];
}

+ newContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
{
    return [self newContent:contentRect
		style:aStyle
		backing:bufferingType
		buttonMask:mask
		defer:flag
		colorMask:NX_ALLMODESMASK];
}

+ newContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
    colorMask:(int)colormask
{
    NXZone *zone;

    if (sharedColorPanel)
	return sharedColorPanel;

    zone = NXCreateZone(vm_page_size, vm_page_size, YES);
    NXNameZone(zone, "NXColorPanel");
    return [[super allocFromZone:zone] _initContent:contentRect
		    style:aStyle
		    backing:bufferingType
		    buttonMask:mask
		    defer:flag
		    colorMask:colormask];
}

- _initContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
    colorMask:(int)colormask
{
    if (!sharedColorPanel) {
	NXRect theFrame = {{DEFAULTX,DEFAULTY}, {DEFAULTWIDTH,DEFAULTHEIGHT}};
	
	[super initContent:&theFrame
	    style:NX_RESIZEBARSTYLE
	    backing:NX_BUFFERED
	    buttonMask:NX_CLOSEBUTTONMASK
	    defer:NO];
	sharedColorPanel = self;
	_accessory = nil;
	if (self) {	
	    if (_bottomView==nil){
	        NXRect bFrame = {{0.0, 0.0}, {DEFAULTWIDTH, 1.0}};
	        _bottomView = [[View allocFromZone:[self zone]] initFrame:&bFrame];
	        [contentView addSubview:_bottomView];
		[_bottomView setAutosizing:NX_WIDTHSIZABLE];
	    }
	    if (!LOADCOLORPICKERNIB("ColorPicker",self)) {
		[self free];
		sharedColorPanel = nil;
	    } else {
	        [contentView addSubview:_colorPicker];
	        [contentView setAutoresizeSubviews:YES];
		if(![_colorPicker setInitialColorMask:colormask]) {
		    [self free];
		    sharedColorPanel = nil;
		    return sharedColorPanel;
		}
		[_colorPicker setAutoresizeSubviews:YES];
		[_colorPicker setAutosizing:NX_WIDTHSIZABLE|NX_HEIGHTSIZABLE];
		_min.width = frame.size.width;
		_min.height = frame.size.height;
		_max.width = MAXWIDTH;
		_max.height = frame.size.height+ MAXADDHEIGHT;
		[self setDelegate:self];
		[self setTitle:KitString(ColorPanel,"Colors","title of color Panel")];
		[self setBecomeKeyOnlyIfNeeded:YES];
		[self setFloatingPanel:YES];
		[self setOneShot:NX_KITPANELSONESHOT];
		[self useOptimizedDrawing:NX_KITPANELSOPTIMIZED];
		[self display];
		{
		    char colordir[MAXPATHLEN+1];
		    strcpy(colordir, NXHomeDirectory());
		    strcat(colordir,"/.NeXT/Colors");
		    mkdir(colordir, 0755);
		}
	    }
	}
    }

    return sharedColorPanel;
}

+ sharedInstance:(BOOL)create
{
    if (!create) {
	return sharedColorPanel;
    } else {
	return [NXColorPanel new];
    }
}

- (int)colorMask
{
    return [_colorPicker colorMask];
}

- setColorMask:(int)colormask
{
    [_colorPicker setColorMask:colormask];
    return self;
}

- center:anObject
{
    NXRect myFrame,superFrame;
    [anObject getFrame:&myFrame];
    [[anObject superview] getFrame:&superFrame];
    [anObject moveTo:floor((superFrame.size.width - myFrame.size.width)/2.0)
    		    :myFrame.origin.y];
    return self;
}

- accessoryView
{
    return _accessory;
}

- setAccessoryView:aView
{
    id                  oldView;
    NXRect 		newFrame;
    
    
    if (!_accessory || _accessory != aView) {
	oldView = [self _doSetAccessoryView:aView topView:_colorPicker bottomView:nil oldView:&_accessory];
	NXSetRect(&newFrame,0.0,0.0,0.0,0.0);
	[aView getFrame:&newFrame];
	_min.width = (DEFAULTFRAMEWIDTH < newFrame.size.width) ? newFrame.size.width : DEFAULTFRAMEWIDTH;
	_min.height = DEFAULTFRAMEHEIGHT + newFrame.size.height;
	 
	if (MAXWIDTH > _min.width) {
	    _max.width = MAXWIDTH;
	} else {
	    _max.width = _min.width;
	}
	_max.height = DEFAULTFRAMEHEIGHT + MAXADDHEIGHT + newFrame.size.height;
	
	return oldView;
    } else {
	return aView;
    }
}

#ifdef 0
- setAccessoryView:anObject
{
   NXRect windowFrame,colorFrame,accessoryFrame;
   NXRect dividerFrame,contentFrame,origC,origF;
   
   NXSetRect(&origC,DEFAULTX,DEFAULTY,DEFAULTWIDTH,DEFAULTHEIGHT);
   [Window getFrameRect:&origF forContentRect:&origC style:NX_RESIZEBARSTYLE];
   _min = origF.size; 
   _max.width = MAXWIDTH;
   _max.height = origF.size.height + MAXADDHEIGHT;
   
   [_colorPicker setAutoresizeSubviews:NO];
   
   [_colorPicker getFrame:&colorFrame];
   windowFrame = frame;
   
   if(anObject!=nil){
	if(_accessory != nil)[_accessory removeFromSuperview];
	_accessory = anObject;
	[anObject getFrame:&accessoryFrame];
	accessoryFrame.origin.x = accessoryFrame.origin.y = 0.0;
   } else {
	if(_accessory != nil)[_accessory removeFromSuperview];
	_accessory = nil;
	NXSetRect(&accessoryFrame,0.0,0.0,0.0,0.0);
   }
   if(_accessory !=nil){
        [contentView addSubview:_accessory];
	if(accessoryFrame.size.width > colorFrame.size.width){
	    if(accessoryFrame.size.width > _max.width){
		_max.width = accessoryFrame.size.width;
		colorFrame.size.width = accessoryFrame.size.width;
	    } else {
		colorFrame.size.width = accessoryFrame.size.width;
	    }
	    accessoryFrame.origin.x = accessoryFrame.origin.y = 0.0;
	} else {
	    accessoryFrame.origin.y = 0.0;
	    accessoryFrame.origin.x = floor((colorFrame.size.width - 
			    accessoryFrame.size.width)/2.0);
	}
	colorFrame.origin.x = 0.0;
	if(accessoryFrame.size.width > _min.width){
	    _min.width = accessoryFrame.size.width;
	}
	_min.height = _min.height+accessoryFrame.size.height+2.0;
	_max.height = _max.height+accessoryFrame.size.height+2.0;
	NXSetRect(&dividerFrame,0.0,accessoryFrame.size.height,
		    (colorFrame.size.width>accessoryFrame.size.width)?
		    colorFrame.size.width:accessoryFrame.size.width,2.0);
	if (_divider == nil){
	    _divider = [[[[[Box allocFromZone:[self zone]] initFrame:&dividerFrame] 
			    setOffsets:0.0:0.0]
			    setTitlePosition:NX_NOTITLE]
			    setAutosizing:NX_WIDTHSIZABLE];
	} 
	colorFrame.origin.y = dividerFrame.origin.y+2.0;

	[contentView addSubview:_divider];
	contentFrame.size.width = dividerFrame.size.width;
	contentFrame.size.height = colorFrame.size.height +
				dividerFrame.size.height +
				accessoryFrame.size.height;
   } else {
	if (_divider != nil) [_divider removeFromSuperview];
	colorFrame.origin.x = colorFrame.origin.y = 0.0;
	contentFrame.size = colorFrame.size;
   }
   [Window getFrameRect: &origF forContentRect: &contentFrame 
   	style: NX_RESIZEBARSTYLE];
   origF.origin.y = frame.origin.y - ( origF.size.height - frame.size.height );
   origF.origin.x = frame.origin.x;
   [self disableFlushWindow];
   [contentView setAutoresizeSubviews:NO];
   [self placeWindow: &origF];
   [_divider sizeTo: dividerFrame.size.width : dividerFrame.size.height];
   [_divider moveTo: dividerFrame.origin.x : dividerFrame.origin.y];
   [_accessory moveTo: accessoryFrame.origin.x: accessoryFrame.origin.y];
   [_colorPicker moveTo: colorFrame.origin.x: colorFrame.origin.y];
   [_colorPicker setAutoresizeSubviews: YES];
   [_colorPicker sizeTo: colorFrame.size.width: colorFrame.size.height];
   [self center: _accessory];
  /* [_accessory setAutosizing: NX_MINXMARGINSIZABLE|NX_MAXXMARGINSIZABLE];*/
   [contentView setAutoresizeSubviews:YES];
   [self reenableFlushWindow];
   [self display];
    
    return self;
}
#endif

- setColorPicker:anObject
{
    _colorPicker = anObject;
    [_colorPicker setAction:@selector(_notifyTarget:)];
    [_colorPicker setTarget:self];
    return self;
}

- setContinuous:(BOOL)flag
{
    [_colorPicker setContinuous:flag];
    return self;
}

- setShowAlpha:(BOOL)flag
{
   [_colorPicker setShowAlpha:flag];
   return self;
}

- setMode:(int)mode
{
   [_colorPicker setMode:mode];
   return self;
}

- setColor:(NXColor)color
{
    [_colorPicker setColor:color];
    return self;    
}

- (NXColor)getColor
{
    return [_colorPicker getColor];
}

- (NXColor)color
{
    return [_colorPicker getColor];
} 

- _getColor:(NXColor *)color
{
    [_colorPicker _getColor:color];
    return self;
} 

- setAction:(SEL) aSelector
{
    _action = aSelector;
    return self;
} 

- setTarget:anObject
{
    _target = anObject;
    return self;
}

- updateCustomColorList
{
    [_colorPicker updateCustomColorList];
    return self;
}

- (BOOL)_notifyTarget:sender
{
    return [NXApp sendAction:_action to:_target from:self];
}

#define RND(A) ((A - .5 > floor(A)) ? ceil(A) : floor(A))

- windowWillResize:sender toSize:(NXSize *)frameSize
{
   frameSize->height = 
   	frame.size.height - (((int)(frame.size.height - 
	frameSize->height))/13)*13;
 
   if (frameSize->width > _max.width) frameSize->width = _max.width;
   else if (frameSize->width < _min.width) frameSize->width = _min.width;
	
   if (frameSize->height > _max.height) frameSize->height = _max.height;
   else if (frameSize->height < _min.height) frameSize->height = _min.height;
   
   return self;
}

+ getColorWindow:(NXColor)col
{
    NXRect wFrame;
    id colorWindowView = nil;

    [colorWindow getFrame:&wFrame];
    if (!colorWindow) {
	wFrame.size.width = COLORWIDTH;
	wFrame.size.height = COLORHEIGHT;
	wFrame.origin.x = 0.0; wFrame.origin.y = 0.0;
	colorWindow = [[Window allocFromZone:[NXApp zone]] initContent:&wFrame
				      style:NX_PLAINSTYLE
				    backing:NX_BUFFERED
				 buttonMask:0
				      defer:NO];
	[colorWindow _setWindowLevel:NX_MAINMENULEVEL];
	colorWindowView = [colorWindow contentView];
	[colorWindowView setClipping:NO];
    }
    colorWindowView = [colorWindow contentView];
    [colorWindowView lockFocus];
    PSsetgray(NX_BLACK);
    NXRectFill(&wFrame);
    NXSetColor(col);
    PScompositerect(1.0, 1.0, COLORWIDTH - 2.0, COLORHEIGHT - 2.0, NX_COPY);
    [colorWindowView unlockFocus];

    return colorWindowView;
}

static id findSubViewContainingPoint(id view,NXPoint *vPt)
{
    id sublist,v;
    int i;
    NXPoint tpt;
    NXRect vFrame;
    
    sublist = [view subviews];
    for(i = 0; i < [sublist count]; i++){
	v = [sublist objectAt:i];
	if ([v respondsTo:@selector(acceptColor:atPoint:)]){
	    tpt = *vPt;
	    [v getBounds:&vFrame];
	    [v convertPoint:&vFrame.origin toView:nil];
	    if(NXPointInRect(vPt, &vFrame)) return v;
	} 
    }
    for(i = 0; i < [sublist count]; i++){
	v = findSubViewContainingPoint([sublist objectAt:i], vPt);
	if(v != nil) return v;
    }
    
    return nil;
}

static id getColorAcceptor(int window, NXPoint *p)
{
    int windowNum;
    unsigned int localWindowNum;
    NXPoint vPt;
    int i, count, context;
    int found;
    id w,v;
    int *windows;

    _NXPSgetwindowbelow(window, &windowNum, &p->x, &p->y, &found);
    if (found) {
	PScurrentowner(window, &context);
	PScountscreenlist(context, &count);
	windows = (int *)NXZoneMalloc(NXDefaultMallocZone(), sizeof(int)*count);
	_NXPSscreenlist(context, count, windows);
	for (i = 0; i < count; i++) {
	    if (windows[i] == windowNum) {
		free(windows);
		NXConvertGlobalToWinNum(windowNum, &localWindowNum);
		windowNum = localWindowNum;
		w = [NXApp findWindow:windowNum];
		vPt = *p;
		v = findSubViewContainingPoint([w contentView],p);
		return v;
	    }
        }
	free(windows);
    }
    
    return nil;
}


+ (BOOL)dragColor:(NXColor)col withEvent:(NXEvent *)theEvent fromView:controlView
{
    NXPoint _p;
    register NXPoint* p = &_p;
    id colorAcceptor, colorWindowView = nil;

    colorWindowView = [self getColorWindow:col];
    *p = theEvent->location;
    [[controlView window] convertBaseToScreen:p];
    [colorWindow moveTo:p->x - 6.0 :p->y - 6.0];
    [[colorWindow flushWindow] orderFront:self];
    [colorWindow dragFrom:6.0 :6.0 eventNum:theEvent->data.mouse.eventNum];
    if (colorAcceptor = getColorAcceptor([colorWindow windowNum], p)) {
	[colorAcceptor acceptColor:col atPoint:p];
    }
    [colorWindow orderOut:self];
    
    return YES;
}

+ (BOOL)dragColor:(NXColor)col withEvent:(NXEvent *)theEvent inView:controlView
{
    return [self dragColor:col withEvent:theEvent fromView:controlView];
}

- (BOOL)worksWhenModal
{
    return YES;
}

@end



/*
    
4/1/90 kro	color panel that contains the color picker control

80
--
 4/8/90 keith	added masking stuff for all the color modes.

85
--
 6/2/90 keith	added NXColorWell color window dragging stuff.

86
--
 6/12/90 pah	made NXColorPanel worksWhenModal

89
--
 7/23/90 aozer	Removed displayOnScreenChange

91
--
 8/12/90 keith	cleaned up a little

94
--
 9/12/90 trey	named the color panel zone

95
--
10/2/90 keith	fixed resize constraining

99
--
 10/19/90 aozer	Moved /.NeXT/Colors creation from NXColorCustom to here.
		Still need to get rid of the system() call.


*/

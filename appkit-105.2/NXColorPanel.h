/*
	NXColorPanel.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Panel.h"
#import "View.h"

#define NX_GRAYMODE 0
#define NX_RGBMODE 1
#define NX_CMYKMODE 2
#define NX_HSBMODE 3
#define NX_CUSTOMPALETTEMODE 4
#define NX_CUSTOMCOLORMODE 5
#define NX_BEGINMODE 6

#define NX_GRAYMODEMASK 1
#define NX_RGBMODEMASK 2
#define NX_CMYKMODEMASK 4
#define NX_HSBMODEMASK 8
#define NX_CUSTOMPALETTEMODEMASK 16
#define NX_CUSTOMCOLORMODEMASK 32
#define NX_BEGINMODEMASK 64

#define NX_ALLMODESMASK \
    (NX_GRAYMODEMASK|\
    NX_RGBMODEMASK| \
    NX_CMYKMODEMASK| \
    NX_HSBMODEMASK| \
    NX_CUSTOMPALETTEMODEMASK| \
    NX_CUSTOMCOLORMODEMASK| \
    NX_BEGINMODEMASK)

@interface NXColorPanel : Panel
{
    id                  _colorPicker;
    id			_bottomView;
    NXSize              _min;
    NXSize		_max;
    SEL                 _action;
    id                  _target;
    id                  _accessory, _divider;
    unsigned int        _reservedCPint[8];
}

+ (BOOL)dragColor:(NXColor)color withEvent:(NXEvent *)theEvent fromView:sourceView;

+ new;
+ newColorMask:(int)colormask;
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag colorMask:(int)colormask;
+ sharedInstance:(BOOL)create;

+ allocFromZone:(NXZone *)zone;
+ alloc;

- (int)colorMask;
- setColorMask:(int)colormask;
- setAccessoryView:aView;
- setContinuous:(BOOL)flag;
- setShowAlpha:(BOOL)flag;
- setMode:(int)mode;
- setColor:(NXColor)color;
- (NXColor)color;
- setAction:(SEL)aSelector;
- setTarget:anObject;

- updateCustomColorList;

@end

@interface View(ColorAcceptor)
- acceptColor:(NXColor)color atPoint:(NXPoint *)aPoint;
@end

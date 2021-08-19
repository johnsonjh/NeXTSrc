/*
	NXColorPicker.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "privateWraps.h"
#import "Box_Private.h"
#import "Application.h"
#import "Button.h"
#import "ButtonCell.h"
#import "Matrix.h"
#import "NXBrowser.h"
#import "NXColorPicker.h"
#import "NXColorSlider.h"
#import "NXColorPalette.h"
#import "NXColorCustom.h"
#import "NXColorPanel.h"
#import "NXColorSwatch.h"
#import "NXColorWheel.h"
#import "NXColorWell.h"
#import "NXImage.h"
#import "colorPickerPrivate.h"
#import "nextstd.h"
#import "tiff.h"
#import <libc.h>
#import <math.h>
#import <dpsclient/wraps.h>

#define EXTRASWATCHHEIGHT 26.0
#define PALETTEWIDTH 297.0

#define NX_GRAYMODE 0
#define NX_RGBMODE 1
#define NX_CMYKMODE 2
#define NX_HSBMODE 3
#define NX_CUSTOMPALETTEMODE 4
#define NX_CUSTOMCOLORMODE 5
#define NX_BEGINMODE 6

#define DEFAULTMODE NX_BEGINMODE

#define REDSLIDER 0
#define GREENSLIDER REDSLIDER + 1
#define BLUESLIDER GREENSLIDER + 1
#define CYANSLIDER BLUESLIDER + 1
#define MAGENTASLIDER CYANSLIDER + 1
#define YELLOWSLIDER MAGENTASLIDER + 1
#define BLACKSLIDER YELLOWSLIDER + 1
#define HUESLIDER BLACKSLIDER + 1
#define SATURATIONSLIDER HUESLIDER + 1
#define BRIGHTNESSSLIDER SATURATIONSLIDER + 1
#define ALPHASLIDER BRIGHTNESSSLIDER + 1
#define GRAYSLIDER ALPHASLIDER + 1

static int modemasks[NUMMODES] = {
	NX_GRAYMODEMASK,
	NX_RGBMODEMASK,
	NX_CMYKMODEMASK,
	NX_HSBMODEMASK,
	NX_CUSTOMPALETTEMODEMASK,
	NX_CUSTOMCOLORMODEMASK,
	NX_BEGINMODEMASK
};
    
#define WHEELSLIDERWIDTH 24.0
#define COLORSLIDERHEIGHT 19.0
#define COLORSLIDERGAP 5.0
#define COLORCOMMANDSWIDTH 110.0
#define COLORBUTTONHEIGHT 21.0

#define CUSTOMMODE 7
#define DELTAWHEELWIDTH 55.0
#define DELTAWHEELHEIGHT 34.0
#define DELTAWHEELX -4.0
#define DELTAWHEELY -4.0

#define LASTCOLORSIZE 30.0
#define RND(x) ((((x)-floor(x))<=.5)?floor(x):ceil(x))

@implementation  NXColorPicker:Control

+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}

- initFrame:(NXRect const *)frameRect
{
    [super initFrame:frameRect];
    max.width = frameRect->size.width;
    max.height = frameRect->size.height;
    showalpha = 1;
    colormode = NX_BEGINMODE;
    lastmode = 1;
    color = NX_COLORBLACK;
    [self setAutoresizeSubviews:YES];
    return self;
}

- setShowAlpha:(BOOL)flag
{
    showalpha = flag ? YES : NO;
    if (!showalpha) NXChangeAlphaComponent(color, NX_NOALPHA);
    if (sliders[ALPHASLIDER]) [sliders[ALPHASLIDER] setEnabled:showalpha];
    return self;
}

- (int)colorMode
{
    return colormode;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    PSsetgray(NX_LTGRAY);
    NXRectFillList(rects, rectCount);
    return self;
}

- (int)colorMask 
{ 
    return colormask;
}
 
- setColorMask:(int) mask
{
    colormask = mask;
    return self;
}

- setPositionView:anObject
{
   [anObject getFrame:&viewRect];
   [anObject removeFromSuperview];
   return self;
}

- setInitialColorMask:(int)mask;
{
    NXRect sw;
    int x, r, c, nummodes = 0;
    
    for (x = 0; x < NUMMODES; x++) {
	if (!(mask & modemasks[x])) {
	    [colorModeButtons getRow:&r andCol:&c ofCell:[colorModeButtons findCellWithTag:x]];
	    [colorModeButtons removeColAt:c andFree:NO];
	} else {
	    nummodes++;
	}
    }
    [colorModeButtons sizeToCells];

    [self addSubview:colorBoxView];
    [self addSubview:colorModeButtons];
    [self addSubview:colorSwatchView];
    [self addSubview:magnifyButton];
    [colorBoxView getFrame:&swatch];
    
    colormask = mask;
    [self setColorMode:self];

    if (nummodes == 1) {
    
	swatch.size.height -= EXTRASWATCHHEIGHT;
	[colorBoxView sizeTo:swatch.size.width:swatch.size.height];
	
	[magnifyButton getFrame:&sw];
	sw.origin.y -= EXTRASWATCHHEIGHT;
	[magnifyButton moveTo:sw.origin.x:sw.origin.y];
	
	[window getFrame:&sw];
	sw.origin.y += EXTRASWATCHHEIGHT;
	sw.size.height -= EXTRASWATCHHEIGHT;
	
	[window placeWindow:&sw];
	sw = frame;
	if (mask & NX_GRAYMODEMASK) [window setTitle:KitString(ColorPanel,"Shades","Color panel title if only gray mode is available")];
	[colorModeButtons removeFromSuperview];
    }

    [colorBoxView setColor:color]; 
     
    return self;
}

- createSlider:(NXRect *)aRect type:(int)type :(int)r1 :(int)g1 :(int)b1 :(int)r2 :(int)g2 :(int)b2 :(const char *)title :(float)titlecolor :(int)units
{
    int start[4], end[4];
    id slider;
    
    start[0] = r1;
    start[1] = g1;
    start[2] = b1;
    start[3] = 255;
    end[0] = r2;
    end[1] = g2;
    end[2] = b2;
    end[3] = 255;

    slider = [[NXColorSlider allocFromZone:[self zone]] initFrame:aRect];
    [slider setTitle:title];
    [slider setTitleColor:titlecolor];
    [slider setGradation:start:end];
    [slider setAction:@selector(setColorFromSlider:)];
    [slider setTarget:self];
    [slider setUnits:units];
    
    return slider;
}

- setColorSwatchView:anObject
{
    char filename[MAXPATHLEN+1];

    colorSwatchView = anObject;
    strcpy(filename, NXHomeDirectory());
    strcat(filename, "/.NeXT/Colors/colorcache");
    [colorSwatchView setColorPicker:self];
    [colorSwatchView setFilename:filename];
    [colorSwatchView readColors];
	    
    return self;
}

- createColorWheelView
{
    NXRect aRect;
    id anObject, colorWheelView;
    
    colorWheelView = [[Box allocFromZone:[self zone]] initFrame:&viewRect];
    [colorWheelView setAutosizing:NX_MINYMARGINSIZABLE|NX_WIDTHSIZABLE];
    [colorWheelView setBorderType:NX_NOBORDER];
    [colorWheelView setTitlePosition:NX_NOTITLE];
    NXSetRect(&aRect,floor(((viewRect.size.width - (viewRect.size.height - 9.0)) - WHEELSLIDERWIDTH) / 3.0),
    		    0.0, (viewRect.size.height - 9.0), (viewRect.size.height - 9.0));
    wheelView = [[NXColorWheel allocFromZone:[self zone]] initFrame:&aRect];
    [wheelView setAutosizing:NX_MINXMARGINSIZABLE|NX_MAXXMARGINSIZABLE];
    NXSetRect(&aRect, floor((((viewRect.size.width - (viewRect.size.height - 9.0))
    			 - WHEELSLIDERWIDTH) / 3.0) * 2.0 + (viewRect.size.height - 9.0)),
    		    0.0, WHEELSLIDERWIDTH, (viewRect.size.height - 9.0));
    anObject = [[NXColorSlider allocFromZone:[self zone]] initFrame:&aRect];
    [colorWheelView addSubview:wheelView];
    [colorWheelView addSubview:anObject];
    [colorWheelView setAutoresizeSubviews:YES];
    [wheelView setColorBrightSlider:anObject];
    [wheelView setColorPicker:self];

    return colorWheelView;
}

- createCustomPaletteView
{
    NXRect aRect;
    id anObject, customPaletteView;
    
    customPaletteView = [[View allocFromZone:[self zone]] initFrame:&viewRect];
    [customPaletteView setAutosizing:NX_MINYMARGINSIZABLE|NX_WIDTHSIZABLE];
    [customPaletteView setAutoresizeSubviews:YES];
    
    NXSetRect(&aRect,0.0, COLORBUTTONHEIGHT + 10.0, viewRect.size.width, viewRect.size.height - (COLORBUTTONHEIGHT + 10.0) - 4.0);
    anObject = [[Box allocFromZone:[self zone]] initFrame:&aRect];
    [anObject setAutosizing:NX_WIDTHSIZABLE];
    [anObject setAutoresizeSubviews:YES];
    [anObject setBorderType:NX_BEZEL];
    [anObject setOffsets:0.0:0.0];
    [anObject setTitlePosition:NX_NOTITLE];
    [customPaletteView addSubview:anObject];
    aRect.origin.x = aRect.origin.y = 0.0;
   /* aRect.size.width -= 2.0;*/
    aRect.size.width = PALETTEWIDTH;
    aRect.size.height -= 2.0;
    paletteView = [[NXColorPalette allocFromZone:[self zone]] initFrame:&aRect];
    [paletteView setAutosizing:NX_WIDTHSIZABLE];
    [paletteView setAutoresizeSubviews:YES];
    [anObject addSubview:paletteView];

    NXSetRect(&aRect, 0.0, 6.0, viewRect.size.width - COLORCOMMANDSWIDTH, COLORBUTTONHEIGHT);
    anObject = [[Button allocFromZone:[self zone]] initFrame:&aRect
    		       title:NULL
		       tag:0
		       target:paletteView
		       action:@selector(setCurrentPalette:)
		       key:0
		       enabled:YES];
    [customPaletteView addSubview:anObject];
    [paletteView setPaletteButton:anObject];
    [anObject setAutosizing:NX_WIDTHSIZABLE];
    
    NXSetRect(&aRect, viewRect.size.width - COLORCOMMANDSWIDTH, 6.0, COLORCOMMANDSWIDTH, COLORBUTTONHEIGHT);
    anObject = [[Button allocFromZone:[self zone]] initFrame:&aRect
    		       title:KitString(ColorPanel,"Options","title of button in palette view")
		       tag:0
		       target:paletteView
		       action:@selector(doPaletteOption:)
		       key:0
		       enabled:YES];
    [customPaletteView addSubview:anObject];
    [paletteView setOptionButton:anObject];
    [anObject setAutosizing:NX_MINXMARGINSIZABLE];
        
    return customPaletteView;
}

- createCustomColorView
{
    NXRect aRect;
    id anObject, customColorView;
    
    customColorView = [[View allocFromZone:[self zone]] initFrame:&viewRect];
    [customColorView setAutosizing:NX_MINYMARGINSIZABLE|NX_WIDTHSIZABLE];
    
    NXSetRect(&aRect, 0.0, 6.0, viewRect.size.width - COLORCOMMANDSWIDTH, COLORBUTTONHEIGHT);
    anObject = [[Button allocFromZone:[self zone]] initFrame:&aRect
    		       title:NULL
		       tag:0
		       target:customColors
		       action:@selector(setCurrentCustom:)
		       key:0
		       enabled:YES];
    [customColorView addSubview:anObject];
    [customColors setCustomButton:anObject];
    [anObject setAutosizing:NX_WIDTHSIZABLE];
    
    NXSetRect(&aRect, viewRect.size.width - COLORCOMMANDSWIDTH, 6.0, COLORCOMMANDSWIDTH, COLORBUTTONHEIGHT);
    anObject = [[Button allocFromZone:[self zone]] initFrame:&aRect
    		       title:KitString(ColorPanel,"Options","title of pup up in custom color view")
		       tag:0
		       target:customColors
		       action:@selector(doCustomOption:)
		       key:0
		       enabled:YES];
    [customColorView addSubview:anObject];
    [customColors setOptionButton:anObject];
    [anObject setAutosizing:NX_MINXMARGINSIZABLE];
    
    NXSetRect(&aRect, 0.0, COLORBUTTONHEIGHT + 10.0, viewRect.size.width, viewRect.size.height - (COLORBUTTONHEIGHT + 10.0) - 4.0);
    anObject = [[NXBrowser allocFromZone:[self zone]] initFrame:&aRect];
    [customColorView addSubview:anObject];
    [customColors setColorBrowser:anObject];
    [anObject setAutosizing:NX_WIDTHSIZABLE];
    
    [customColorView setAutoresizeSubviews:YES];
    
    return customColorView;
}

- createRGBView
{
    NXRect aRect;
    id rgbView;
    
    rgbView = [[View allocFromZone:[self zone]] initFrame:&viewRect];
    [rgbView setAutosizing:NX_MINYMARGINSIZABLE|NX_WIDTHSIZABLE];
    NXSetRect(&aRect, 0.0, floor((viewRect.size.height - 
    		    (4 * COLORSLIDERHEIGHT + 3 * COLORSLIDERGAP)) / 2.0),
    		    viewRect.size.width, COLORSLIDERHEIGHT);
    if (!sliders[ALPHASLIDER]) {
        sliders[ALPHASLIDER] = [self createSlider:&aRect type:ALPHASLIDER :0 :0 :0 :255 :255 :255 :KitString(ColorPanel,"Opacity","title of alpha slider") :NX_WHITE :1];
        [sliders[ALPHASLIDER] setAutosizing:NX_WIDTHSIZABLE];
        [sliders[ALPHASLIDER] setEnabled:showalpha];
    }
    
    aRect.origin.y += COLORSLIDERHEIGHT + COLORSLIDERGAP;
    sliders[BLUESLIDER] = [self createSlider:&aRect type:BLUESLIDER :0:0:0:0:0:255:KitString(ColorPanel,"Blue","title of blue slider"):NX_WHITE:0];
    [sliders[BLUESLIDER] setAutosizing:NX_WIDTHSIZABLE];
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[GREENSLIDER] = [self createSlider:&aRect type:GREENSLIDER :0:0:0:0:255:0:KitString(ColorPanel,"Green","title of green slider"):NX_WHITE:0];
    [sliders[GREENSLIDER] setAutosizing:NX_WIDTHSIZABLE];
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[REDSLIDER] = [self createSlider:&aRect type:REDSLIDER :0:0:0:255:0:0:KitString(ColorPanel,"Red","title of red slider"):NX_WHITE:0];
    [sliders[REDSLIDER] setAutosizing:NX_WIDTHSIZABLE];
    
    [rgbView addSubview:sliders[REDSLIDER]];
    [rgbView addSubview:sliders[GREENSLIDER]];
    [rgbView addSubview:sliders[BLUESLIDER]];
    [rgbView addSubview:sliders[ALPHASLIDER]];

    [rgbView setAutoresizeSubviews:YES];
    
    return rgbView;
}

- createHSBView
{
    NXRect aRect;
    id hsbView;

    hsbView = [[View allocFromZone:[self zone]] initFrame:&viewRect];
    [hsbView setAutosizing:NX_MINYMARGINSIZABLE|NX_WIDTHSIZABLE];
    NXSetRect(&aRect, 0.0, floor((viewRect.size.height - 
    		    (4 * COLORSLIDERHEIGHT + 3 * COLORSLIDERGAP)) / 2.0),
    		    viewRect.size.width, COLORSLIDERHEIGHT);
    if (!sliders[ALPHASLIDER]) {
        sliders[ALPHASLIDER] = [self createSlider:&aRect type:ALPHASLIDER :0 :0 :0 :255 :255 :255 :KitString(ColorPanel,"Opacity","title of alpha slider") :NX_WHITE :1];
        [sliders[ALPHASLIDER] setAutosizing:NX_WIDTHSIZABLE];
        [sliders[ALPHASLIDER] setEnabled:showalpha];
    }
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[BRIGHTNESSSLIDER] = [self createSlider:&aRect type:BRIGHTNESSSLIDER :0 :0 :0 :0 :0 :255 :KitString(ColorPanel,"Brightness","title of brightness slider") :NX_WHITE :0];
    [sliders[BRIGHTNESSSLIDER] setAutosizing:NX_WIDTHSIZABLE];
    [sliders[BRIGHTNESSSLIDER] setHSBSlider:1];
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[SATURATIONSLIDER] = [self createSlider:&aRect type:SATURATIONSLIDER :0 :0 :0 :0 :255 :0 :KitString(ColorPanel,"Saturation","title of saturation slider") :NX_BLACK :0];
    [sliders[SATURATIONSLIDER] setAutosizing:NX_WIDTHSIZABLE];
    [sliders[SATURATIONSLIDER] setHSBSlider:1];
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[HUESLIDER] = [self createSlider:&aRect type:HUESLIDER :0 :255 :255 :255 :255 :255 :KitString(ColorPanel,"Hue","title of hue slider") :NX_BLACK :0];
    [sliders[HUESLIDER] setAction:@selector(setHueColor:)];
    [sliders[HUESLIDER] setAutosizing:NX_WIDTHSIZABLE];
    [sliders[HUESLIDER] setHSBSlider:1];
    
    [hsbView addSubview:sliders[HUESLIDER]];
    [hsbView addSubview:sliders[SATURATIONSLIDER]];
    [hsbView addSubview:sliders[BRIGHTNESSSLIDER]];
    [hsbView addSubview:sliders[ALPHASLIDER]];

    [hsbView setAutoresizeSubviews:YES];
    
    return hsbView;
}

- createCMYKView
{
    NXRect aRect;
    id cmykView;
     
    cmykView = [[View allocFromZone:[self zone]] initFrame:&viewRect];
    [cmykView setAutosizing:NX_MINYMARGINSIZABLE|NX_WIDTHSIZABLE];
    
    NXSetRect(&aRect, 0.0, floor((viewRect.size.height - 
    		    (4 * COLORSLIDERHEIGHT + 3 * COLORSLIDERGAP)) / 2.0),
    		    viewRect.size.width, COLORSLIDERHEIGHT);
    sliders[BLACKSLIDER] = [self createSlider:&aRect type:BLACKSLIDER :255 :255 :255 :0 :0 :0 :KitString(ColorPanel,"Black","title of black slider") :NX_BLACK :1];
    [sliders[BLACKSLIDER] setAutosizing:NX_WIDTHSIZABLE];
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[YELLOWSLIDER] = [self createSlider:&aRect type:YELLOWSLIDER :255 :255 :255 :255 :255 :0 :KitString(ColorPanel,"Yellow","title of yellow slider") :NX_BLACK :1];
    [sliders[YELLOWSLIDER] setAutosizing:NX_WIDTHSIZABLE];
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[MAGENTASLIDER] = [self createSlider:&aRect type:MAGENTASLIDER :255 :255 :255 :255 :0 :255 :KitString(ColorPanel,"Magenta","title of magenta slider") :NX_BLACK :1];
    [sliders[MAGENTASLIDER] setAutosizing:NX_WIDTHSIZABLE];
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[CYANSLIDER] = [self createSlider:&aRect type:CYANSLIDER :255 :255 :255 :0 :255 :255 :KitString(ColorPanel,"Cyan","title of cyan slider") :NX_BLACK :1];
    [sliders[CYANSLIDER] setAutosizing:NX_WIDTHSIZABLE];
    
    [cmykView addSubview:sliders[BLACKSLIDER]];
    [cmykView addSubview:sliders[YELLOWSLIDER]];
    [cmykView addSubview:sliders[MAGENTASLIDER]];
    [cmykView addSubview:sliders[CYANSLIDER]];

    [cmykView setAutoresizeSubviews:YES];
    
    return cmykView;
}

- createGrayView
{
    int a;
    NXRect aRect;
    id grayView;
    NXSize aSize = {0.0,0.0};
    
    grayView = [[View allocFromZone:[self zone]] initFrame:&viewRect];
    [grayView setAutosizing:NX_MINYMARGINSIZABLE|NX_WIDTHSIZABLE];
    NXSetRect(&aRect, 0.0, floor((viewRect.size.height - 
    		    (4 * COLORSLIDERHEIGHT + 3 * COLORSLIDERGAP)) / 2.0),
    		    viewRect.size.width, (COLORSLIDERHEIGHT));
    if(sliders[ALPHASLIDER] == nil){
        sliders[ALPHASLIDER] = [self createSlider:&aRect type:ALPHASLIDER :0 :0 :0 :255 :255 :255 :KitString(ColorPanel,"Opacity","title of alpha slider") :NX_WHITE :1];
        [sliders[ALPHASLIDER] setAutosizing:NX_WIDTHSIZABLE];
        [sliders[ALPHASLIDER] setEnabled:showalpha];
    }
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    [alphaButtons removeFromSuperview];
    [alphaButtons setAction:@selector(setAlpha:)];
    [alphaButtons setIntercell:&aSize];
    [grayView addSubview:alphaButtons];
    [alphaButtons sizeTo:viewRect.size.width-30.0:COLORSLIDERHEIGHT+1.0];
    [alphaButtons moveTo:aRect.origin.x:aRect.origin.y];
    
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    [grayButtons removeFromSuperview];
    [grayButtons setAction:@selector(setGray:)];
    [grayButtons setIntercell:&aSize];
    [grayView addSubview:grayButtons];
    [grayButtons sizeTo:viewRect.size.width-30.0:COLORSLIDERHEIGHT+1.0];
    [grayButtons moveTo:aRect.origin.x:aRect.origin.y];
    
    for (a = 0; a < [grayButtons cellCount]; a++){
        /*[[grayButtons findCellWithTag:a] setBordered:NO];
        [[alphaButtons findCellWithTag:a] setBordered:NO]*/;
    }
    aRect.origin.y += COLORSLIDERHEIGHT+COLORSLIDERGAP;
    sliders[GRAYSLIDER] = [self createSlider:&aRect type:GRAYSLIDER :0 :0 :0 :255 :255 :255 :KitString(ColorPanel,"Gray","title of gray slider") :NX_WHITE :1];
    [sliders[GRAYSLIDER] setAutosizing:NX_WIDTHSIZABLE];
    
    [grayView addSubview:sliders[GRAYSLIDER]];
    [grayView setAutoresizeSubviews:YES];
    
    return grayView;
}


- wheelView
{
    return wheelView;
}

- setColorModeButtons:anObject
{
    int a;
    NXSize aSize = {0.0,0.0};
    
    colorModeButtons = anObject;
    for (a = 0; a < [colorModeButtons cellCount]; a++) {
	[[colorModeButtons findCellWithTag:a] setHighlightsBy:NX_CHANGEBACKGROUND];
    }
    [colorModeButtons setIntercell:&aSize];
    [colorModeButtons selectCellWithTag:NX_BEGINMODE];
    [colorModeButtons setAutosizeCells:YES];
    [colorModeButtons setAutosizing:NX_MINYMARGINSIZABLE|NX_WIDTHSIZABLE];

    return self;
}

- setColorBoxView:anObject
{
    colorBoxView = anObject;
    [colorBoxView setBordered:NO];
    [colorBoxView setAction:@selector(newColor:)];
    [colorBoxView setTarget:self];
    return self;
}

- doMagnify:sender
{
    NXSize newSize;
    NXColor newcolor;
    int colorwindow, numcolors, bps, r, g, b, a;
    static id magnifyBitmap = nil;
    
    if (magnifyBitmap == nil) {
        newSize.width = newSize.height = 48.0;
        magnifyBitmap = [[NXImage allocFromZone:[self zone]] initSize:&newSize];
	[magnifyBitmap setFlipped:NO];
	[magnifyBitmap setCacheDepthBounded:NO];
    }
    PSgsave();
    [magnifyBitmap lockFocus];
    _NXMagnifyColors(&colorwindow,&numcolors,&bps);
    _NXReadPixels(&r, &g, &b, &a, 19.0, 33.0);
    newcolor = NXConvertRGBAToColor(ITOF(r), ITOF(g), ITOF(b), ITOF(a));
    [magnifyBitmap unlockFocus];
    PSgrestore();
    [self setAndSendColorValues:newcolor:0];

    return self;
}

- setGrayButtons:anObject
{
    int x;
    
    grayButtons = anObject;
    for (x = 0; x < [grayButtons cellCount]; x++) {
	[[grayButtons findCellWithTag:x] setHighlightsBy:NX_CHANGEGRAY];
    }
    [grayButtons setAutosizeCells:YES];
    
    return self;
}

- setAlphaButtons:anObject
{
    int x;
    
    alphaButtons = anObject;
    for (x = 0; x < [alphaButtons cellCount]; x++) {
	[[alphaButtons findCellWithTag:x] setHighlightsBy:NX_CHANGEGRAY];
    }
    [alphaButtons setAutosizeCells:YES];
    
    return self;
}

static void posterizeValues(id *sliders, int index, int tag)
{
   
    if (tag==0) [sliders[index] setIntColor:0];
    else if (tag==1) [sliders[index] setIntColor:43];
    else if (tag==2) [sliders[index] setIntColor:85];
    else if (tag==3) [sliders[index] setIntColor:128];
    else if (tag==4) [sliders[index] setIntColor:170];
    else if (tag==5) [sliders[index] setIntColor:213];
    else if (tag==6) [sliders[index] setIntColor:255];
    [sliders[index] display];
}

- setGray:sender
{
    int index = [sender selectedTag];
    posterizeValues(sliders, GRAYSLIDER, index);
    [self setColorFromSlider:sliders[GRAYSLIDER]];
    return self;
}

- setAlpha:sender
{
    int index = [sender selectedTag];
    if (showalpha){
	posterizeValues(sliders, ALPHASLIDER, index);
	[self setColorFromSlider:sliders[ALPHASLIDER]];
    }
    return self;
}

- setColorFromSlider:sender
{
    if (sender == sliders[REDSLIDER])
    	color = NXChangeRedComponent(color, [sender getFloatColor]);
    else if (sender == sliders[GREENSLIDER])
    	color = NXChangeGreenComponent(color, [sender getFloatColor]);
    else if (sender == sliders[BLUESLIDER])
    	color = NXChangeBlueComponent(color, [sender getFloatColor]);
    else if (sender == sliders[ALPHASLIDER])
    	color = NXChangeAlphaComponent(color, [sender getFloatColor]);
    else if (sender == sliders[SATURATIONSLIDER] || sender == sliders[BRIGHTNESSSLIDER]){
        color = NXConvertHSBAToColor([sliders[HUESLIDER] getFloatColor],
				     [sliders[SATURATIONSLIDER] getFloatColor],
				     [sliders[BRIGHTNESSSLIDER] getFloatColor],
				     [sliders[ALPHASLIDER] getFloatColor]);
    }
    else if (sender == sliders[CYANSLIDER])
    	color = NXChangeCyanComponent(color, [sender getFloatColor]);
    else if (sender == sliders[MAGENTASLIDER])
    	color = NXChangeMagentaComponent(color, [sender getFloatColor]);
    else if (sender == sliders[YELLOWSLIDER])
    	color = NXChangeYellowComponent(color, [sender getFloatColor]);
    else if (sender == sliders[BLACKSLIDER])
    	color = NXChangeBlackComponent(color, [sender getFloatColor]);
    else if (sender == sliders[GRAYSLIDER])
    	color = NXChangeGrayComponent(color, [sender getFloatColor]);
    [colorBoxView setColor:color];
    [window flushWindow];
    [self updateColor:0];
    
    return self;
}

- setHueColor:sender
{
    NXEvent *currentEvent;
    
    color = NXChangeHueComponent(color, [sender getFloatColor]);
    [colorBoxView setColor:color];
    currentEvent = [NXApp currentEvent];
    if(currentEvent->type == NX_MOUSEUP){
       [self updateHueSlider:[sender getIntColor]];
       [sliders[BRIGHTNESSSLIDER] display];
       [sliders[SATURATIONSLIDER] display];
    }
    [window flushWindow];
    [self updateColor:0];
    
    return self;
}

- updateHueSlider:(int)h
{
    int start_color[4], end_color[4], hue;
    
    hue = h;
    start_color[0] = hue;
    start_color[1] = 0;
    start_color[2] = 255;
    start_color[3] = 255;
    end_color[0] = hue;
    end_color[1] = 255;
    end_color[2] = 255;
    end_color[3] = 255;
    [sliders[SATURATIONSLIDER] newGradation:start_color:end_color];
    start_color[0] = hue;
    start_color[1] = 255;
    start_color[2] = 0;
    start_color[3] = 255;
    end_color[0] = hue;
    end_color[1] = 255;
    end_color[2] = 255;
    end_color[3] = 255;
    [sliders[BRIGHTNESSSLIDER] newGradation:start_color:end_color];
    
    return self;
}

- doSizeView:anObject:(NXRect *)fr
{
    NXRect newfr;
    
    [anObject getFrame:&newfr];
    if ((newfr.origin.x != fr->origin.x) || (newfr.origin.y != fr->origin.y))
        [anObject moveTo:fr->origin.x:fr->origin.y];
    if (newfr.size.width != fr->size.width) {
    	[anObject sizeTo:fr->size.width:fr->size.height];
    }
    
    return self;
}

- checkColorModeButtons:anObject
{
    NXSize newsz;
    NXRect a,b;
    
    [colorModeButtons getFrame:&a];
    [anObject getFrame:&b];
    b.size.width -= 16.0;
    if (a.size.width != b.size.width) {
	[colorModeButtons getCellSize:&newsz];
	newsz.width = RND((b.size.width) / 7.0);
	[colorModeButtons setCellSize:&newsz];
	[colorModeButtons sizeToCells];
    }
    
    return self;
}

- checkAlphaSlider:anObject
{
    NXRect a,b;
    
    [sliders[ALPHASLIDER] getFrame:&a];
    [anObject getFrame:&b];
    if (a.size.width!=b.size.width) [sliders[ALPHASLIDER] sizeTo:b.size.width:b.size.height];
	
    return self;
}

- setMode:(int)mode
{
    if (colormode != mode) {
	[colorModeButtons selectCellWithTag:mode];
	if([colorModeButtons findCellWithTag:mode] != nil)
	    [self setColorMode:colorModeButtons];
    }
    return self;
}

- center:anObject
{
    NXRect myFrame,superFrame;
    
    [anObject getFrame:&myFrame];
    [[anObject superview] getFrame:&superFrame];
    [anObject moveTo:floor((superFrame.size.width - myFrame.size.width) / 2.0):myFrame.origin.y];
		    
    return self;
}

- setColorMode:sender
{
    int oldmode, shouldsize=1;
    NXRect oldFrame;
    
    if (sender != self) {
	oldmode = colormode;
	colormode = [sender selectedTag];
        if (oldmode == colormode) return self;
	[modeView[oldmode] getFrame:&oldFrame];
	[modeView[oldmode] removeFromSuperview];
	if (oldmode == NX_GRAYMODE || oldmode == NX_RGBMODE || oldmode == NX_HSBMODE)
	      [sliders[ALPHASLIDER] removeFromSuperview];
	if ([window isKeyWindow]) [[NXApp mainWindow] makeKeyWindow];
    } else {
	if ((NXNumberOfColorComponents(NXColorSpaceFromDepth([NXApp colorScreen]->depth)) == 1) && (colormask & NX_GRAYMODEMASK)) {
	    [colorModeButtons selectCellWithTag:NX_GRAYMODE];
	    colormode = NX_GRAYMODE;
	} else {
	    colormode = DEFAULTMODE;
	}
	shouldsize = 0;
    }
    [window disableFlushWindow];
    switch (colormode){
       case (NX_BEGINMODE):
           if (modeView[NX_BEGINMODE] == nil) {
	       modeView[NX_BEGINMODE] = [self createColorWheelView];
	       [modeView[NX_BEGINMODE] _setBackgroundTransparent:YES];
	   }
	   [self addSubview:modeView[NX_BEGINMODE] :NX_BELOW relativeTo:colorBoxView];
	   if(shouldsize)[self doSizeView:modeView[NX_BEGINMODE]:&oldFrame];
	   color = NXChangeAlphaComponent(color,1.0);
	   [wheelView showColor:&color];
           break;
       case (NX_GRAYMODE):
           if (modeView[NX_GRAYMODE] == nil)
	       modeView[NX_GRAYMODE] = [self createGrayView];
           [self addSubview:modeView[NX_GRAYMODE]:NX_BELOW relativeTo:colorBoxView];
	   if(shouldsize)[self doSizeView:modeView[NX_GRAYMODE]:&oldFrame];
	   [self checkAlphaSlider:sliders[GRAYSLIDER]];
           [modeView[NX_GRAYMODE] addSubview:sliders[ALPHASLIDER]];
	   [sliders[GRAYSLIDER] setFloatColor:NXGrayComponent(color)];
	   if(NXAlphaComponent(color) == NX_NOALPHA)
	       [sliders[ALPHASLIDER] setFloatColor:1.0];
	   else
	       [sliders[ALPHASLIDER] setFloatColor:NXAlphaComponent(color)];
           break;
       case (NX_RGBMODE):
	   if (modeView[NX_RGBMODE] == nil)
	       modeView[NX_RGBMODE] = [self createRGBView];
	   [self addSubview:modeView[NX_RGBMODE] :NX_BELOW relativeTo:colorBoxView];
	   if(shouldsize)[self doSizeView:modeView[NX_RGBMODE] :&oldFrame];
	   [self checkAlphaSlider:sliders[REDSLIDER]];
           [modeView[NX_RGBMODE] addSubview:sliders[ALPHASLIDER]];
	   [sliders[REDSLIDER] setFloatColor:NXRedComponent(color)];
	   [sliders[GREENSLIDER] setFloatColor:NXGreenComponent(color)];
	   [sliders[BLUESLIDER] setFloatColor:NXBlueComponent(color)];
	   if(NXAlphaComponent(color) == NX_NOALPHA)
	       [sliders[ALPHASLIDER] setFloatColor:1.0];
	   else
	       [sliders[ALPHASLIDER] setFloatColor:NXAlphaComponent(color)];
           break;
       case (NX_CMYKMODE):
           if (modeView[NX_CMYKMODE] == nil)
	       modeView[NX_CMYKMODE] = [self createCMYKView];
           [self addSubview:modeView[NX_CMYKMODE]:NX_BELOW relativeTo:colorBoxView];
	   if(shouldsize)[self doSizeView:modeView[NX_CMYKMODE]:&oldFrame];
	   [sliders[CYANSLIDER] setFloatColor:NXCyanComponent(color)];
	   [sliders[MAGENTASLIDER] setFloatColor:NXMagentaComponent(color)];
	   [sliders[YELLOWSLIDER] setFloatColor:NXYellowComponent(color)];
	   [sliders[BLACKSLIDER] setFloatColor:NXBlackComponent(color)];
           break;
       case (NX_HSBMODE):
           if (modeView[NX_HSBMODE] == nil)
	       modeView[NX_HSBMODE] = [self createHSBView];
           [self addSubview:modeView[NX_HSBMODE] :NX_BELOW relativeTo:colorBoxView];
	   if(shouldsize)[self doSizeView:modeView[NX_HSBMODE] :&oldFrame];
	   [self checkAlphaSlider:sliders[HUESLIDER]];
           [modeView[NX_HSBMODE] addSubview:sliders[ALPHASLIDER]];
	   [sliders[HUESLIDER] setFloatColor:NXHueComponent(color)];
	   [sliders[SATURATIONSLIDER] setFloatColor:NXSaturationComponent(color)];
	   [sliders[BRIGHTNESSSLIDER] setFloatColor:NXBrightnessComponent(color)];
	   if(NXAlphaComponent(color) == NX_NOALPHA)
	       [sliders[ALPHASLIDER] setFloatColor:1.0];
	   else
	       [sliders[ALPHASLIDER] setFloatColor:NXAlphaComponent(color)];
	   [self updateHueSlider:FTOI(NXHueComponent(color))];
           break;
       case (NX_CUSTOMPALETTEMODE):
           if (modeView[NX_CUSTOMPALETTEMODE] == nil) {
		modeView[NX_CUSTOMPALETTEMODE] = [self createCustomPaletteView];
                [paletteView findCustomPalettes:self];
	   }
           [self addSubview:modeView[NX_CUSTOMPALETTEMODE] :NX_BELOW relativeTo:colorBoxView];
	   if(shouldsize)[self doSizeView:modeView[NX_CUSTOMPALETTEMODE] :&oldFrame];
           break;
       case (NX_CUSTOMCOLORMODE):
           if (modeView[NX_CUSTOMCOLORMODE] == nil) {
	        customColors = [[NXColorCustom allocFromZone:[self zone]] init];
		modeView[NX_CUSTOMCOLORMODE] = [self createCustomColorView];
                [customColors findCustomColors:self];
	   }
	   [self addSubview:modeView[NX_CUSTOMCOLORMODE] :NX_BELOW relativeTo:colorBoxView];
	   if(shouldsize)[self doSizeView:modeView[NX_CUSTOMCOLORMODE] :&oldFrame];
	   [window makeFirstResponder:customColorBrowser];
           break;
    }
    [colorBoxView setColor:color];
    [self display];
    [window reenableFlushWindow];
    [window flushWindow];

    return self;
}

- keyDown:(NXEvent *)theEvent
{
    [customColors keyDown:theEvent];
    return self;
}

- setAndSendButDontDisplay:(NXColor)newcolor:(int)doit
{
   if (!showalpha) newcolor = NXChangeAlphaComponent(newcolor, 1.0);
   color = newcolor;
   [colorBoxView setColor:color];
   [window flushWindow];
   [self updateColor:doit];
   return self;
}

- setAndSendColorValues:(NXColor)newcolor:(int)doit
{
   if (!showalpha) newcolor = NXChangeAlphaComponent(newcolor, 1.0);
   [self setColorValues:newcolor];
   [self updateColor:doit];
   return self;
}

- setColorValues:(NXColor)newcolor
{
    color = newcolor;
    switch (colormode){
       case (NX_GRAYMODE):
	   [sliders[GRAYSLIDER] setFloatColor:NXGrayComponent(color)];
	   if(NXAlphaComponent(color) == NX_NOALPHA)
	       [sliders[ALPHASLIDER] setFloatColor:1.0];
	   else
	       [sliders[ALPHASLIDER] setFloatColor:NXAlphaComponent(color)];
           break;
       case (NX_RGBMODE):
	   [sliders[REDSLIDER] setFloatColor:NXRedComponent(color)];
	   [sliders[GREENSLIDER] setFloatColor:NXGreenComponent(color)];
	   [sliders[BLUESLIDER] setFloatColor:NXBlueComponent(color)];
	   if(NXAlphaComponent(color) == NX_NOALPHA)
	       [sliders[ALPHASLIDER] setFloatColor:1.0];
	   else
	       [sliders[ALPHASLIDER] setFloatColor:NXAlphaComponent(color)];
           break;
       case (NX_CMYKMODE):
	   [sliders[CYANSLIDER] setFloatColor:NXCyanComponent(color)];
	   [sliders[MAGENTASLIDER] setFloatColor:NXMagentaComponent(color)];
	   [sliders[YELLOWSLIDER] setFloatColor:NXYellowComponent(color)];
	   [sliders[BLACKSLIDER] setFloatColor:NXBlackComponent(color)];
           break;
       case (NX_HSBMODE):
	   [sliders[HUESLIDER] setFloatColor:NXHueComponent(color)];
	   [sliders[SATURATIONSLIDER] setFloatColor:NXSaturationComponent(color)];
	   [sliders[BRIGHTNESSSLIDER] setFloatColor:NXBrightnessComponent(color)];
	   if(NXAlphaComponent(color) == NX_NOALPHA)
	       [sliders[ALPHASLIDER] setFloatColor:1.0];
	   else
	       [sliders[ALPHASLIDER] setFloatColor:NXAlphaComponent(color)];
	   [self updateHueSlider:FTOI(NXHueComponent(color))];
           break;
       case (NX_BEGINMODE):
	   [wheelView showColor:&color];
           break;
    }
    [colorBoxView setColor:color];
    [window flushWindow];
    return self;
}

- setContinuous:(BOOL)flag
{
    continuous = flag;
    return self;
}


- updateColor:(int)doit
{
    NXEvent *currentEvent;
    currentEvent = [NXApp currentEvent];
    
    if (currentEvent->type == NX_MOUSEUP || doit) {
	[self sendAction:_action to:_target];
	[NXColorWell activeWellsTakeColorFrom:self];
    } else if (continuous) {
	[self sendAction:_action to:_target];
	[NXColorWell activeWellsTakeColorFrom:self continuous:YES];
    }
    
    return self;
}

- (BOOL)acceptsFirstMouse
{
    return YES;
}


- newColor:sender
{
    [self setColor:[sender color]];
    return self;
}

- setColor:(NXColor)newcolor
{
    if(!showalpha)newcolor = NXChangeAlphaComponent(newcolor,NX_NOALPHA);
    [self setAndSendColorValues:newcolor:1];
    return self;    
}

- _setColor:(NXColor *)newcolor
{
    if (!showalpha)*newcolor = NXChangeAlphaComponent(*newcolor,NX_NOALPHA);
    [self setAndSendColorValues:*newcolor:1];
    return self;    
}

- (NXColor)getColor
{
    if(!showalpha)NXChangeAlphaComponent(color,NX_NOALPHA);
    return color;
} 

- (NXColor)color
{
    if(!showalpha)NXChangeAlphaComponent(color,NX_NOALPHA);
    return color;
} 

- _getColor:(NXColor *)newcolor
{
    if(!showalpha)NXChangeAlphaComponent(color,NX_NOALPHA);
    *newcolor = color;
    return self;	
} 

- setCustomColorBrowser:anObject
{
    customColorBrowser = anObject;
    return self;
}

- setTarget:anObject
{
  _target=anObject;
  return self;
}

- setAction:(SEL) aSelector
{
   NX_ASSERT(!aSelector || ISSELECTOR(aSelector), \
	      "Bad selector passed to Control's setAction:");
  _action=aSelector;
  return self;
}

- updateCustomColorList
{
    [customColors updateCustomColorList];
    return self;
}

@end


/*
    
4/1/90 kro	added beginmode to colorpicker

appkit-80
---------
 4/8/90 keith	added masking stuff for all the color modes.	
 
appkit-82
---------
 4/16/90 keith	fixed screen change display bug.
 
appkit-90
---------
8/4/90	keith	Greg added zone stuff. Code was finessed after paul and keith 
		design review

94
--
 9/19/90 aozer	Added code in setColorMode: to set the initial mode
		to gray if graymode is available and on monochrome system.

95
--
 10/2/90 keith	Fixed it so that if only one mode is used the buttons are 
		removed and the magnify button is lowered.
			
98
--
 10/13/90 keith	Changed "Alpha" to "Opacity" in slider title.	
		
*/

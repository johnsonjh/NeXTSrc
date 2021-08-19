/*
	color.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <stddef.h>
#import <objc/typedstream.h>

/*
 * NXColor structure for passing colors around in the Application Kit.
 * Do not access structure elements directly; the values stored in the 
 * various fields are likely to change between releases.
 */ 
typedef struct _NXColor {
    unsigned short colorField[8];
} NXColor;

/*
 * Basic functions to create new NXColor values.  All values are in the
 * range 0..1 (inclusive), following the PostScript model. (The only 
 * exception is an alpha value of NX_NOALPHA, described below.)
 *
 *   RGB = red, green, blue
 *   HSB = hue, saturation, brightness
 *   CMYK = cyan, magenta, yellow, black
 *   A = alpha
 */
extern NXColor NXConvertRGBAToColor (float, float, float, float);
extern NXColor NXConvertHSBAToColor (float, float, float, float);
extern NXColor NXConvertCMYKAToColor (float, float, float, float, float);
extern NXColor NXConvertGrayAlphaToColor (float, float);

/* 
 * Basic functions to extract color components from NXColors.  If you don't
 * want the alpha value, pass NULL and the value won't be returned. 
 */ 
extern void NXConvertColorToRGBA (NXColor, float *, float *, float *, float *);
extern void NXConvertColorToHSBA (NXColor, float *, float *, float *, float *);
extern void NXConvertColorToCMYKA (NXColor, float *, float *, float *, float *, float *);
extern void NXConvertColorToGrayAlpha (NXColor, float *, float *);

/*
 * Macros to allow dealing with colors without involving alpha.
 * Use these rather than the functions above unless you explicitly
 * wish to deal with alpha values.
 */
#define NXConvertRGBToColor(r, g, b) \
		NXConvertRGBAToColor(r, g, b, NX_NOALPHA)
#define NXConvertHSBToColor(h, s, b) \
		NXConvertHSBAToColor(h, s, b, NX_NOALPHA)
#define NXConvertCMYKToColor(c, m, y, k) \
		NXConvertCMYKAToColor(c, m, y, k, NX_NOALPHA)
#define NXConvertGrayToColor(g) \
		NXConvertGrayAlphaToColor(g, NX_NOALPHA)

#define NXConvertColorToRGB(color, r, g, b) \
		NXConvertColorToRGBA(color, r, g, b, NULL)
#define NXConvertColorToHSB(color, h, s, b) \
		NXConvertColorToHSBA(color, h, s, b, NULL)
#define NXConvertColorToCMYK(color, c, m, y, k) \
		NXConvertColorToCMYKA(color, c, m, y, k, NULL)
#define NXConvertColorToGray(color, g) \
		NXConvertColorToGrayAlpha(color, g, NULL)

/*
 * Functions to get individual components one at a time. For two or more
 * components, it's more efficient to call NXConvertColorToXXX.
 */
extern float NXRedComponent (NXColor);
extern float NXBlueComponent (NXColor);
extern float NXGreenComponent (NXColor);
extern float NXGrayComponent (NXColor);
extern float NXHueComponent (NXColor);
extern float NXSaturationComponent (NXColor);
extern float NXBrightnessComponent (NXColor);
extern float NXCyanComponent (NXColor);
extern float NXYellowComponent (NXColor);
extern float NXMagentaComponent (NXColor);
extern float NXBlackComponent (NXColor);
extern float NXAlphaComponent (NXColor);

/*
 * Functions to set individual components one at a time.  Note that the
 * color argument is not modified; instead, a new color is returned.
 * To change the red component of a color you'd do something like:
 *    myColor = NXChangeRedComponent(myColor, 0.5);
 */
extern NXColor NXChangeRedComponent (NXColor, float);
extern NXColor NXChangeBlueComponent (NXColor, float);
extern NXColor NXChangeGreenComponent (NXColor, float);
extern NXColor NXChangeGrayComponent (NXColor, float);
extern NXColor NXChangeHueComponent (NXColor, float);
extern NXColor NXChangeSaturationComponent (NXColor, float);
extern NXColor NXChangeBrightnessComponent (NXColor, float);
extern NXColor NXChangeCyanComponent (NXColor, float);
extern NXColor NXChangeYellowComponent (NXColor, float);
extern NXColor NXChangeMagentaComponent (NXColor, float);
extern NXColor NXChangeBlackComponent (NXColor, float);
extern NXColor NXChangeAlphaComponent (NXColor, float);

/*
 * Misc functions.
 */
extern void NXWriteColor (NXTypedStream *, NXColor);
extern NXColor NXReadColor (NXTypedStream *);
extern BOOL NXEqualColor (NXColor, NXColor);

/*
 * Set the current color to the specified color. NXSetColor() will generate 
 * alpha only if the alpha is specified in the color and the output is going 
 * to a device capable of dealing with alpha.
 */
extern void NXSetColor (NXColor);

/*
 * Some "standard" colors.
 */
#define NX_COLORBLACK	NXConvertGrayToColor(0.0)
#define NX_COLORWHITE	NXConvertGrayToColor(1.0)
#define NX_COLORGRAY	NXConvertGrayToColor(0.5)
#define NX_COLORLTGRAY	NXConvertGrayToColor(2./3.)
#define NX_COLORDKGRAY	NXConvertGrayToColor(1./3.)
#define NX_COLORRED	NXConvertRGBToColor(1.0, 0.0, 0.0)
#define NX_COLORGREEN	NXConvertRGBToColor(0.0, 1.0, 0.0)
#define NX_COLORBLUE	NXConvertRGBToColor(0.0, 0.0, 1.0)
#define NX_COLORCYAN	NXConvertCMYKToColor(1.0, 0.0, 0.0, 0.0)
#define NX_COLORYELLOW	NXConvertCMYKToColor(0.0, 0.0, 1.0, 0.0)
#define NX_COLORMAGENTA	NXConvertCMYKToColor(0.0, 1.0, 0.0, 0.0)
#define NX_COLORORANGE	NXConvertRGBToColor(1.0, 0.5, 0.0)
#define NX_COLORPURPLE	NXConvertRGBToColor(0.5, 0.0, 0.5)
#define NX_COLORBROWN	NXConvertRGBToColor(0.6, 0.4, 0.2)

#define NX_COLORCLEAR	NXConvertGrayAlphaToColor(0.0, 0.0)

/*
 * The following value will be returned by functions returning alpha 
 * if the alpha is not specified in the color.  You can also use NX_NOALPHA
 * as an argument to any function accepting alpha.
 */

#define NX_NOALPHA (-1.0)


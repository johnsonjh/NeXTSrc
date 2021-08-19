/*
	color.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

/*
 * Need to optimize the various functions to get individual components.
 *
 * Need to optimize NXReadColor() and NXWriteColor().
 *
 * In this release, all colors are stored in RGB mode. If there is black,
 * it is in the BLACK component. A better way to store this stuff would be to
 * change mode depending on the last NXConvertColorToXXX or 
 * NXChangeXXXComponent call.
 */
#import "color.h"
#import "appkitPrivate.h"
#import "errors.h"
#import "View.h"	/* For NXDrawingStatus */
#import <libc.h>
#import <limits.h>
#import <dpsclient/wraps.h>

/*
 * What follows is the private declarations for NXColor.
 */
#define RED(c)    ((c).colorField[1])
#define GREEN(c)  ((c).colorField[2])
#define BLUE(c)   ((c).colorField[3])
#define BLACK(c)  ((c).colorField[0])
#define ALPHA(c)  ((c).colorField[4])
#define MODE(c)   ((c).colorField[7])

#define NOMODE 0	/* Currently we use only these two */	
#define RGBMODE 1
#define HSBMODE 2
#define CMYKMODE 3

/*
 * Limit the value to the range 0..1. 
 */
#define LIMIT(x) (((x) < 0.0) ? 0.0 : (((x) > 1.0) ? 1.0 : (x)))

/*
 * Encoded values range between 0 and MAXVAL, inclusive. We try to make
 * sure that 0.5 maps back and forth without changing to something like .498.
 */
#define MAXVAL (USHRT_MAX-1)

/*
 * Take a value 0.0..1.0 and make it 0 .. max unsigned short - 1.
 * The DECODE & ENCODE macros can't deal with alpha values (because of 
 * NX_NOALPHA); use DECODEALPHA and ENCODEALPHA.
 */
#define DECODE(i)  (((float)(i)) / ((float)MAXVAL))
#define ENCODE(x)  (((x) < 0.0) ? 0 : (((x) > 1.0) ? MAXVAL : (unsigned short)(((x) * MAXVAL) + .5))) 

/*
 * If a color has no alpha, set the encoded alpha field to NOALPHA.
 * The value visible to the user is NX_NOALPHA, defined in color.h.
 * Macros ENCODEALPHA and DECODEALPHA should be used to get/set alpha values
 * in all of the below functions.
 */
#define NOALPHA (USHRT_MAX)
#define DECODEALPHA(c) ((ALPHA(c) == NOALPHA) ? NX_NOALPHA : DECODE(ALPHA(c)))
#define ENCODEALPHA(a) ((a == NX_NOALPHA) ? NOALPHA : ENCODE(a)) 

NXColor NXConvertRGBAToColor (float r, float g, float b, float a)
{
    NXColor color = {{0}};
    RED(color) = ENCODE(r);
    GREEN(color) = ENCODE(g);
    BLUE(color) = ENCODE(b);
    MODE(color) = RGBMODE;
    ALPHA(color) = ENCODEALPHA(a);
    return color;
}

NXColor NXConvertHSBAToColor (float h, float s, float b, float a)
{
    float red, green, blue;    
    _NXHSBToRGB(LIMIT(h), LIMIT(s), LIMIT(b), &red, &green, &blue);
    return NXConvertRGBAToColor(red, green, blue, a);
}

NXColor NXConvertCMYKAToColor (float c, float m, float y, float k, float a)
{
    NXColor color = NXConvertRGBAToColor(1.0-c, 1.0-m, 1.0-y, a);
    BLACK(color) = ENCODE(k);
    return color;
}

NXColor NXConvertGrayAlphaToColor (float g, float a)
{
    return NXConvertRGBAToColor(g, g, g, a);
}

void NXConvertColorToCMYKA (NXColor color, float *c, float *m, float *y, float *k, float *a)
{
    *c = 1.0 - DECODE(RED(color));
    *m = 1.0 - DECODE(GREEN(color));
    *y = 1.0 - DECODE(BLUE(color));
    *k = DECODE(BLACK(color));
    if (a) *a = NXAlphaComponent(color);
}

void NXConvertColorToRGBA (NXColor color, float *r, float *g, float *b, float *a)
{
    float black = DECODE(BLACK(color));

    if ((*r = DECODE(RED(color)) - black) < 0.0) *r = 0.0;
    if ((*g = DECODE(GREEN(color))  - black) < 0.0) *g = 0.0;
    if ((*b = DECODE(BLUE(color))  - black) < 0.0) *b = 0.0;
    if (a) *a = NXAlphaComponent(color);
}

void NXConvertColorToHSBA (NXColor color, float *h, float *s, float *b, float *a)
{
    float red, green, blue;

    NXConvertColorToRGB (color, &red, &green, &blue);
    _NXRGBToHSB (red, green, blue, h, s, b);
    if (a) *a = NXAlphaComponent(color);
}

void NXConvertColorToGrayAlpha (NXColor color, float *g, float *a)
{
    *g = NXGrayComponent (color);
    if (a) *a = NXAlphaComponent(color);    
}

float NXRedComponent (NXColor color)
{
    float red = DECODE(RED(color)) - DECODE(BLACK(color));
    return (red < 0.0 ? 0.0 : red);
}

float NXGreenComponent  (NXColor color)
{
    float green = DECODE(GREEN(color)) - DECODE(BLACK(color));
    return (green < 0.0 ? 0.0 : green);
}


float NXBlueComponent  (NXColor color)
{
    float blue = DECODE(BLUE(color)) - DECODE(BLACK(color));
    return (blue < 0.0 ? 0.0 : blue);
}

float NXCyanComponent (NXColor color)
{
    return 1.0 - DECODE(RED(color));
}


float NXMagentaComponent  (NXColor color)
{
    return 1.0 - DECODE(GREEN(color));
}

float NXYellowComponent  (NXColor color)
{
    return 1.0 - DECODE(BLUE(color));
}

float NXBlackComponent  (NXColor color)
{
    return DECODE(BLACK(color));
}

float NXHueComponent (NXColor color)
{
    float hue, saturation, brightness, alpha;
    NXConvertColorToHSBA (color, &hue, &saturation, &brightness, &alpha);
    return hue;
}

float NXSaturationComponent (NXColor color)
{
    float hue, saturation, brightness, alpha;
    NXConvertColorToHSBA (color, &hue, &saturation, &brightness, &alpha);
    return saturation;
}

float NXBrightnessComponent (NXColor color)
{
    float hue, saturation, brightness, alpha;
    NXConvertColorToHSBA (color, &hue, &saturation, &brightness, &alpha);
    return brightness;
}

float NXGrayComponent (NXColor color)
{
    float red, green, blue;
    NXConvertColorToRGB (color, &red, &green, &blue);
    return red * .3 + green * .59 + blue * .11;
}

float NXAlphaComponent (NXColor color)
{
    return DECODEALPHA(color);
}

NXColor NXChangeRedComponent (NXColor color, float val)
{
    float black = DECODE(BLACK(color));

    if (black != 0.0) {
	GREEN(color) = ENCODE(DECODE(GREEN(color))  - black);
	BLUE(color) = ENCODE(DECODE(BLUE(color))  - black);
	BLACK(color) = ENCODE(0.0);
    }
    RED(color) = ENCODE(val);
    return color;
}

NXColor NXChangeGreenComponent (NXColor color, float val)
{
    float black = DECODE(BLACK(color));

    if (black != 0.0) {
	RED(color) = ENCODE(DECODE(RED(color))  - black);
	BLUE(color) = ENCODE(DECODE(BLUE(color))  - black);
	BLACK(color) = ENCODE(0.0);
    }
    GREEN(color) = ENCODE(val);
    return color;
}

NXColor NXChangeBlueComponent (NXColor color, float val)
{
    float black = DECODE(BLACK(color));

    if (black != 0.0) {
	GREEN(color) = ENCODE(DECODE(GREEN(color))  - black);
	RED(color) = ENCODE(DECODE(RED(color))  - black);
	BLACK(color) = ENCODE(0.0);
    }
    BLUE(color) = ENCODE(val);
    return color;
}

NXColor NXChangeHueComponent (NXColor color, float val)
{
    float h, s, b, a;
    NXConvertColorToHSBA (color, &h, &s, &b, &a);
    return NXConvertHSBAToColor (val, s, b, a);
}


NXColor NXChangeSaturationComponent (NXColor color, float val)
{
    float h, s, b, a;
    NXConvertColorToHSBA (color, &h, &s, &b, &a);
    return NXConvertHSBAToColor (h, val, b, a);
}


NXColor NXChangeBrightnessComponent (NXColor color, float val)
{
    float h, s, b, a;
    NXConvertColorToHSBA (color, &h, &s, &b, &a);
    return NXConvertHSBAToColor (h, s, val, a);
}


NXColor NXChangeCyanComponent (NXColor color, float val)
{
    RED(color) = ENCODE(1.0 - val);
    return color;
}

NXColor NXChangeMagentaComponent (NXColor color, float val)
{
    GREEN(color) = ENCODE(1.0 - val);
    return color;
}

NXColor NXChangeYellowComponent (NXColor color, float val)
{
    BLUE(color) = ENCODE(1.0 - val);
    return color;
}

NXColor NXChangeBlackComponent (NXColor color, float val)
{
    BLACK(color) = ENCODE(val);
    return color;
}

NXColor NXChangeGrayComponent (NXColor color, float val)
{
    GREEN(color) = RED(color) = BLUE(color) = ENCODE(val);
    BLACK(color) = ENCODE(0.0);
    return color;
}

NXColor NXChangeAlphaComponent (NXColor color, float val)
{
    ALPHA(color) = ENCODEALPHA(val);
    return color;
}

/*
 * The following defines are used only when reading/writing colors.
 */

#define COLORMODEMASK 31
    #define COLORRGB 0
    #define COLORGRAY 1
    #define COLORNONE 2
    #define COLORLASTMODE COLORNONE
#define COLORZEROBLACK 64
#define COLORNOALPHA 128

void NXWriteColor (NXTypedStream *stream, NXColor color)
{
    unsigned char format = ((BLACK(color) == 0) ? COLORZEROBLACK : 0) |
			   ((ALPHA(color) == NOALPHA) ? COLORNOALPHA : 0);

    if (!_NXIsValidColor(color)) {
	format = COLORNONE;
    } else if ((RED(color) == GREEN(color)) && (GREEN(color) == BLUE(color))) {
	format |= COLORGRAY;
    } else {
	format |= COLORRGB;
    }

    NXWriteType (stream, "c", &format);

    if (!_NXIsValidColor(color)) {
	return;
    }

    if ((format & COLORMODEMASK) == COLORGRAY) {
	NXWriteType (stream, "s", &RED(color));
    } else {
	NXWriteTypes (stream, "sss", &RED(color), &GREEN(color), &BLUE(color));
    }
    if ((format & COLORZEROBLACK) == 0) {
	NXWriteType (stream, "s", &BLACK(color));
    }
    if ((format & COLORNOALPHA) == 0) {
	NXWriteType (stream, "s", &ALPHA(color));
    }
}

NXColor NXReadColor (NXTypedStream *stream)
{
    unsigned char format;
    NXColor color;

    NXReadType (stream, "c", &format);

    if ((format & COLORMODEMASK) == COLORNONE) {
	return _NXNoColor();
    }

    if ((format & COLORMODEMASK) > COLORLASTMODE) {
	NX_RAISE(NX_newerTypedStream, 0, 0);
    }
    
    if ((format & COLORMODEMASK) == COLORGRAY) {
	NXReadType (stream, "s", &RED(color));
	GREEN(color) = BLUE(color) = RED(color);
    } else {
	NXReadTypes (stream, "sss", &RED(color), &GREEN(color), &BLUE(color));
    }

    if ((format & COLORZEROBLACK) == 0) {
	NXReadType (stream, "s", &BLACK(color));
    } else {
	BLACK(color) = 0;
    }
    if ((format & COLORNOALPHA) == 0) {
	NXReadType (stream, "s", &ALPHA(color));
    } else {
	ALPHA(color) = NOALPHA;
    }
    MODE(color) = RGBMODE;

    return color;
}

NXColor _NXNoColor ()
{
    NXColor noColor = {0, 0, 0, 0, 0, 0, 0, 0};
    return noColor;
}

BOOL _NXIsValidColor (NXColor color)
{
    return (MODE(color) == RGBMODE) ? YES : NO;
}

BOOL _NXIsPureGray (NXColor color, float gray)
{
    unsigned short dGray = ENCODE(gray);
    return ((RED(color) == dGray) && (BLUE(color) == dGray) &&
	    (GREEN(color) == dGray));
}

void NXSetColor (NXColor color)
{
    float r = DECODE(RED(color));
    float g = DECODE(GREEN(color));
    float b = DECODE(BLUE(color));

    if (DECODE(BLACK(color)) == 0.0) {
	if ((r == g) && (g == b)) {
	    PSsetgray (r);
	} else {
	    PSsetrgbcolor (r, g, b);
	}
    } else {
	PSsetcmykcolor (1.0-r, 1.0-g, 1.0-b, DECODE(BLACK(color)));
    }
    if (NXDrawingStatus != NX_PRINTING) {
	float alpha = DECODEALPHA(color);
	if (alpha != NX_NOALPHA) {
	    PSsetalpha (DECODE(ALPHA(color)));
	}
    }
}

BOOL NXEqualColor (NXColor colorOne, NXColor colorTwo)
{
    float r1, g1, b1, a1, r2, g2, b2, a2;

    NXConvertColorToRGBA(colorOne, &r1, &g1, &b1, &a1);
    NXConvertColorToRGBA(colorTwo, &r2, &g2, &b2, &a2);

    return (((g1 == g2) && (r1 == r2) && (b1 == b2) && (a1 == a2)) ? YES : NO);
}


/*
 * Convert RGB to HSB. 
 * Come in with red, green, and blue all in the range 0..1
 * Return with hue, saturation, and brightness in the range 0..1
 * If saturation and value are  both 0.0, hue is undefined (returned as 0)
 */
void _NXRGBToHSB (float red, float green, float blue, float *hue, float *saturation, float *brightness)
{
    float h = 0.0, v, s, m;

    /* Set v to MAX(red, green, blue), and m to MIN(red, green, blue). */

    if (red >= green && red >= blue) {
	v = red;
	m = (green < blue) ? green : blue; 
    } else if (green >= blue && green >= red) {
	v = green;
	m = (red < blue) ? red : blue;
    } else {
	v = blue;
	m = (green < red) ? green : red;
    }
	
    if (v > 0.0) s = (v - m) / v;
    else s = 0.0;

    if (s != 0.0) {
	float rl = (v - red) / (v - m);   // Distance from the color to red
	float gl = (v - green) / (v - m); // Distance from the color to green
	float bl = (v - blue) / (v - m);  // Distance from the color to blue
	if (v == red) {
	    if (m == green) {
		h = 5 + bl;
	    } else {
		h = 1 - gl;
	    }
	} else if (v == green) {
	    if (m == blue) {
		h = 1 + rl;
	    } else {
		h = 3 - bl;
	    }
	} else if (m == red) {
	    h = 3 + gl;
	} else {
	    h = 5 - rl;
	}
    }
    *brightness = v;
    *hue = h / 6.0;
    *saturation = s;
}

/* 
 * Convert HSB to RGB. 
 * Come in with hue, saturation, and brightness all in the range 0..1.
 */
void _NXHSBToRGB (float hue, float saturation, float brightness, float *red, float *green, float *blue)
{
    float hueTimesSix, frac, p1, p2, p3;

    if (hue == 1.0) {
	hue = 0.0;
    }
    hueTimesSix = hue * 6.0;
    frac = hueTimesSix - (int)hueTimesSix;
    p1 = brightness * (1.0 - saturation);
    p2 = brightness * (1.0 - (saturation * frac));
    p3 = brightness * (1.0 - (saturation * (1.0 - frac)));

    switch ((int)hueTimesSix) {
	case 0:
	    *red = brightness;
	    *green = p3;
	    *blue = p1;
	    break;
	case 1:
	    *red = p2;
	    *green = brightness;
	    *blue = p1;
	    break;
	case 2:
	    *red = p1;
	    *green = brightness;
	    *blue = p3;
	    break;
	case 3:
	    *red = p1;
	    *green = p2;
	    *blue = brightness;
	    break;
	case 4:
	    *red = p3;
	    *green = p1;
	    *blue = brightness;
	    break;
	case 5:
	    *red = brightness;
	    *green = p1;
	    *blue = p2;
	    break;
    }
}



/*

Modifications (since 78):

78
--
 3/5/90 aozer	Created.
 3/7/90 aozer	Implemented most of the functionality in a highly unoptimized
		way; need to go back and redo most for speed.

79
--
 3/28/90 aozer	Added NXGrayComponent, NXConvertGrayToColor, 
		NXConvertGrayAlphaToColor, and the various convert functions
		without alpha components. Also lots of macros to allow for
		setting/getting colors without specifying an alpha.
 3/30/90 aozer	Made NXWriteColor & NXReadColor follow NXColor paradigm and
		not typedstream paradigm.

83
--
 4/26/90 aozer	Added _NXNoColor(). This can be written out as one
		byte and will indicate the absence of any color in some
		cases.

87
--
 7/11/90 aozer	Made NXSetColor generate PSsetgray if r==g==b.
 7/12/90 aozer	Limit checking on NXConvertHSBToColor

*/






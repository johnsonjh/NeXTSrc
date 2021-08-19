/*
 * Private color structure used in the color picker files.
 * Note that if the color is in HSB form, the r, g, b fields
 * hold h, s, b, values; if the color is in CMYK form, the 
 * four fields hold c, m, y, k values (with no place for alpha).
 */
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} CPColor;

/*
 * Currently there seems to be a bug with passing structs
 * around, so for now use the original routines.
 */



#define convertRGBtoHSB(rgb, hsb) \
{float h, s, b, a; \
 NXConvertColorToHSBA( \
  NXConvertRGBAToColor(ITOF((rgb)->r),ITOF((rgb)->g),ITOF((rgb)->b),0.0), \
  &h,&s,&b,&a); \
 (hsb)->r = FTOI(h); (hsb)->g = FTOI(s); (hsb)->b = FTOI(b); \
}

#define convertHSBtoRGB(rgb, hsb) \
{float r, g, b, a; \
 NXConvertColorToRGBA( \
  NXConvertHSBAToColor(ITOF((hsb)->r),ITOF((hsb)->g),ITOF((hsb)->b),0.0), \
  &r,&g,&b,&a); \
 (rgb)->r = FTOI(r); (rgb)->g = FTOI(g); (rgb)->b = FTOI(b); \
}

#define convertRGBtoCMYK(rgb, cmyk) \
{float c, m, y, k, a; \
 NXConvertColorToCMYKA( \
  NXConvertRGBAToColor(ITOF((rgb)->r),ITOF((rgb)->g),ITOF((rgb)->b),0.0), \
  &c,&m,&y,&k,&a); \
 (cmyk)->r=FTOI(c); (cmyk)->g=FTOI(m); (cmyk)->b=FTOI(y); (cmyk)->a=FTOI(k); \
}

#define convertCMYKtoRGB(rgb, cmyk) \
{float r, g, b, a; \
 NXConvertColorToRGBA( \
  NXConvertCMYKAToColor(ITOF((cmyk)->r), ITOF((cmyk)->g), ITOF((cmyk)->b), ITOF((cmyk)->a), 0.0), \
  &r,&g,&b,&a); \
 (rgb)->r = FTOI(r); (rgb)->g = FTOI(g); (rgb)->b = FTOI(b); \
}



#define ITOF(a) ((float)((1.0/255.0)*(a)))
#define FTOI(a) ((int)(a/(1.0/255.0)))
#define NUMSPACE 32.0

/*
 * Segment in the shared library from which we get the
 * various color picker nib files.
 */
#define COLORPICKERSEGMENT "__APPKIT_PANELS"

/*
 * Define STANDALONECOLORPICKER" if you wish to run this color
 * picker without the Kit.
 */

#ifdef STANDALONECOLORPICKER
#define LOADCOLORPICKERNIB(section, master) \
	[NXApp loadNibFile:section ".nib" owner:master withNames:NO fromZone:[master zone]]
#else
#define LOADCOLORPICKERNIB(section, owner) \
	_NXLoadNib(COLORPICKERSEGMENT, section, owner, [owner zone])
#endif

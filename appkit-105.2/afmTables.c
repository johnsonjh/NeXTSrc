
/*
	afmTables.c
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	This file has a bunch of static const tables used for AFM parsing.
	It is included by the paring module.
*/

#import "afmprivate.h"

/* types of tokens ("Keys") that we know about in this version */
typedef enum {
  /* top level tokens -  StartFontMetrics_t to ScreenFontSize_t must be contiguous */
    StartFontMetrics_t = 0,
    FontName_t,
    FullName_t,
    FamilyName_t,
    Weight_t,
    ItalicAngle_t,
    IsFixedPitch_t,
    FontBBox_t,
    UnderlinePosition_t,
    UnderlineThickness_t,
    Version_t,
    Notice_t,
    EncodingScheme_t,
    CapHeight_t,
    XHeight_t,
    Ascender_t,
    Descender_t,
    IsScreenFont_t,
    ScreenFontSize_t,
    EndFontMetrics_t,
    StartCharMetrics_t,
    EndCharMetrics_t,
    StartKernData_t,
    EndKernData_t,
    StartComposites_t,
    EndComposites_t,
  /* Character Metrics Tokens - must be contiguous */
    C_t,
    WX_t,
    W_t,
    N_t,
    B_t,
    L_t,
  /* Kerning Tokens - must be contiguous */
    StartTrackKern_t,
    TrackKern_t,
    EndTrackKern_t,
    StartKernPairs_t,
    EndKernPairs_t,
    KP_t,
    KPX_t,
  /* Composite Char tokens - must be contiguous */
    CC_t,
    PCC_t,
  /* newline, and any other name we dont recognize as a token */
    newline_t,
    unknown_t
} Token;


/* The names of the unencoded chars are hashed along with their position in the NextStep encoding.  Each element hashed is a sequence of bytes.  The first byte is the position in the encoding.  The following bytes are the name of the character, ter
minated by a semicolon.  The strings we put in the table are always immediately terminated by a semicolon.  The strings that we hash from the file can also be terminated by white space.
 */
static const char ISOLatinTableData[] =
        "\x81" "Agrave;"
        "\x82" "Aacute;"
        "\x83" "Acircumflex;"
        "\x84" "Atilde;"
        "\x85" "Adieresis;"
        "\x86" "Aring;"
        "\x87" "Ccedilla;"
        "\x88" "Egrave;"
        "\x89" "Eacute;"
        "\x8a" "Ecircumflex;"
        "\x8b" "Edieresis;"
        "\x8c" "Igrave;"
        "\x8d" "Iacute;"
        "\x8e" "Icircumflex;"
        "\x8f" "Idieresis;"
        "\x90" "Eth;"
        "\x91" "Ntilde;"
        "\x92" "Ograve;"
        "\x93" "Oacute;"
        "\x94" "Ocircumflex;"
        "\x95" "Otilde;"
        "\x96" "Odieresis;"
        "\x97" "Ugrave;"
        "\x98" "Uacute;"
        "\x99" "Ucircumflex;"
        "\x9a" "Udieresis;"
        "\x9b" "Yacute;"
        "\x9c" "Thorn;"
        "\x9d" "mu;"
        "\x9e" "multiply;"
        "\x9f" "divide;"
        "\xa0" "copyright;"
        "\xb0" "registered;"
        "\xb5" "brokenbar;"
        "\xbe" "logicalnot;"
        "\xc0" "onesuperior;"
        "\xc9" "twosuperior;"
        "\xcc" "threesuperior;"
        "\xd1" "plusminus;"
        "\xd2" "onequarter;"
        "\xd3" "onehalf;"
        "\xd4" "threequarters;"
        "\xd5" "agrave;"
        "\xd6" "aacute;"
        "\xd7" "acircumflex;"
        "\xd8" "atilde;"
        "\xd9" "adieresis;"
        "\xda" "aring;"
        "\xdb" "ccedilla;"
        "\xdc" "egrave;"
        "\xdd" "eacute;"
        "\xde" "ecircumflex;"
        "\xdf" "edieresis;"
        "\xe0" "igrave;"
        "\xe2" "iacute;"
        "\xe4" "icircumflex;"
        "\xe5" "idieresis;"
        "\xe6" "eth;"
        "\xe7" "ntilde;"
        "\xec" "ograve;"
        "\xed" "oacute;"
        "\xee" "ocircumflex;"
        "\xef" "otilde;"
        "\xf0" "odieresis;"
        "\xf2" "ugrave;"
        "\xf3" "uacute;"
        "\xf4" "ucircumflex;"
        "\xf6" "udieresis;"
        "\xf7" "yacute;"
        "\xfc" "thorn;"
        "\xfd" "ydieresis;"
        "\xfd" "ydieresis;"
    ;


/* Table of token strings, tags, and offsets in the AFMStruct for their info */

typedef struct {
    char *name;
    Token tag;
    short offset;
} TokenInfo;

const static NXGlobalFontInfo BogusInfo;

#define OFFSET(field)	((char *)&(BogusInfo.field) - (char *)&BogusInfo)

static const TokenInfo TokenTableData[] = {
	{"StartFontMetrics", StartFontMetrics_t, OFFSET(formatVersion)},
	{"EndFontMetrics", EndFontMetrics_t, 0},
	{"FontName", FontName_t, OFFSET(name)},
	{"FullName", FullName_t, OFFSET(fullName)},
	{"FamilyName", FamilyName_t, OFFSET(familyName)},
	{"Weight", Weight_t, OFFSET(weight)},
	{"ItalicAngle", ItalicAngle_t, OFFSET(italicAngle)},
	{"IsFixedPitch", IsFixedPitch_t, OFFSET(isFixedPitch)},
	{"FontBBox", FontBBox_t, 0},
	{"UnderlinePosition", UnderlinePosition_t, OFFSET(underlinePosition)},
	{"UnderlineThickness", UnderlineThickness_t, OFFSET(underlineThickness)},
	{"Version", Version_t, OFFSET(version)},
	{"Notice", Notice_t, OFFSET(notice)},
	{"EncodingScheme", EncodingScheme_t, 0},
	{"CapHeight", CapHeight_t, OFFSET(capHeight)},
	{"XHeight", XHeight_t, OFFSET(xHeight)},
	{"Ascender", Ascender_t, OFFSET(ascender)},
	{"Descender", Descender_t, OFFSET(descender)},
	{"IsScreenFont", IsScreenFont_t, OFFSET(isScreenFont)},
	{"ScreenFontSize", ScreenFontSize_t, OFFSET(screenFontSize)},
	{"StartCharMetrics", StartCharMetrics_t, 0},
	{"EndCharMetrics", EndCharMetrics_t, 0},
	{"StartKernData", StartKernData_t, 0},
	{"EndKernData", EndKernData_t, 0},
	{"StartComposites", StartComposites_t, 0},
	{"EndComposites", EndComposites_t, 0},
	{"C", C_t, 0},
	{"WX", WX_t, 0},
	{"W", W_t, 0},
	{"N", N_t, 0},
	{"B", B_t, 0},
	{"L", L_t, 0},
	{"StartTrackKern", StartTrackKern_t, 0},
	{"TrackKern", TrackKern_t, 0},
	{"EndTrackKern", EndTrackKern_t, 0},
	{"StartKernPairs", StartKernPairs_t, 0},
	{"EndKernPairs", EndKernPairs_t, 0},
	{"KP", KP_t, 0},
	{"KPX", KPX_t, 0},
	{"CC", CC_t, 0},
	{"PCC", PCC_t, 0},
	{0, 0, 0}
};


/* macros for testing character classes */

#define WH 0x1		/* masks for white space */
#define NL 0x2		/* masks for newline */
#define SC 0x4		/* masks for semicolon */

#define IS_WHITE(c)	(charBits[c] == WH)
    /* is the char whitespace (but not newline)? */

#define IS_WHITE_NL_OR_SEMI(c)	(charBits[c] != 0)
    /* is the char whitespace (including newline) or semicolon? */


static const unsigned char charBits[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, WH,NL,0, 0, WH,0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	WH,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, SC,0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


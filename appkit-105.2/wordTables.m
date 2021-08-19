
/*
	wordTables.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Text.h"
#import "nextstd.h"
#import "appkitPrivate.h"
#import "errors.h"
#import <ctype.h>
#import <stdlib.h>
#import <streams/streams.h>
#import <objc/typedstream.h>
#import <sys/file.h>

/* values for the token field of a NXFSM */
#define W	1		/* word is white space */
#define	B	0		/* word is black stuff */
#define N	(-1)		/* word is a new line */


/**** English Language Word Tables ****/

unsigned const char _NXEnglishSmartLeftChars[] = {
    ' ', NX_FIGSPACE, '\n', '\t', '(', '[', '\"', '\'', '#', '$', '/', '-',
    '`', '{',
    '\xa1',	/* exclamdown */
    '\xa3',	/* sterling */
    '\xa5',	/* yen */
    '\xa6',	/* florin */
    '\xa7',	/* section */
    '\xa8',	/* currency */
    '\xa9',	/* quotesingle */
    '\xaa',	/* quotedblleft */
    '\xab',	/* guillemotleft */
    '\xac',	/* guilsinglleft */
    '\xb1',	/* endash */
    '\xb8',	/* quotesinglbase */
    '\xb9',	/* quotedblbase */
    '\xbf',	/* questiondown */
    '\xd0',	/* emdash */
    '\xd1',	/* plusminus */
    '\0'};

unsigned const char _NXEnglishSmartRightChars[] = {
    ' ', NX_FIGSPACE, '\n', '\t', ')', ']', '.', ',', ';', ':', '?', '\'',
    '!', '\"', '%', '*', '-', '/', '}',
    '\xa2',	/* cent */
    '\xad',	/* guilsinglright */
    '\xb0',	/* registered */
    '\xa2',	/* dagger */
    '\xa3',	/* daggerdbl */
    '\xb1',	/* endash */
    '\xba',	/* quotedblright */
    '\xbb',	/* guillemotright */
    '\xbd',	/* perthousand */
    '\xc0',	/* onesuperior */
    '\xc9',	/* twosuperior */
    '\xcc',	/* threesuperior */
    '\xd0',	/* emdash */
    '\xe3',	/* ordfeminine */
    '\xeb',	/* ordmasculine */
    '\0'};

/*  The character classes for english words.  NX_eccQuote includes normal
    single quote and "smart" single quote. NX_special are percents, currency
    symbols, hyphen (not dashes) and figspace. NX_eccControl are the first 32
    control characters. NX_eccOther are punctuation and other singular chars */
enum _NXEnglishCharCats {
	NX_eccLetter = 0 * sizeof(NXFSM),  NX_eccDigit = 1 * sizeof(NXFSM),
	NX_eccPeriod = 2 * sizeof(NXFSM),  NX_eccQuote = 3 * sizeof(NXFSM),   NX_eccComma = 4 * sizeof(NXFSM),
	NX_eccSpecial = 5 * sizeof(NXFSM), NX_eccControl = 6 * sizeof(NXFSM), NX_eccWhite = 7 * sizeof(NXFSM),
	NX_eccNL = 8 * sizeof(NXFSM),	   NX_eccOther = 9 * sizeof(NXFSM)
};

#define NUM_CHAR_CLASSES 10

/* table mapping chars to their classes */
const unsigned char _NXEnglishCharCatTable[256] = {
	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,
	NX_eccControl,	NX_eccWhite,	NX_eccNL,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,
	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,
	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,	NX_eccControl,

	NX_eccWhite,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccSpecial,	NX_eccSpecial,	NX_eccOther,	NX_eccQuote,
	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccComma,	NX_eccSpecial,	NX_eccPeriod,	NX_eccOther,
	NX_eccDigit,	NX_eccDigit,	NX_eccDigit,	NX_eccDigit,	NX_eccDigit,	NX_eccDigit,	NX_eccDigit,	NX_eccDigit,
	NX_eccDigit,	NX_eccDigit,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,

	NX_eccOther,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccLetter,

	NX_eccOther,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccControl,

	NX_eccSpecial,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccOther,	NX_eccOther,

	NX_eccOther,	NX_eccOther,	NX_eccSpecial,	NX_eccSpecial,	NX_eccOther,	NX_eccSpecial,	NX_eccSpecial,	NX_eccOther,
	NX_eccSpecial,	NX_eccQuote,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccLetter,	NX_eccLetter,
	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,
	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccSpecial,	NX_eccOther,	NX_eccOther,

	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,
	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,	NX_eccOther,
	NX_eccOther,	NX_eccOther,	NX_eccSpecial,	NX_eccSpecial,	NX_eccSpecial,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,

	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,
	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccLetter,	NX_eccControl,	NX_eccControl
};

/* states for word wrapping table */
#define EINIT1	&_NXEnglishBreakTable[0*NUM_CHAR_CLASSES],0,0
#define BGBL	&_NXEnglishBreakTable[1*NUM_CHAR_CLASSES],0,0		/* gobble up a word */
#define WGBL	&_NXEnglishBreakTable[2*NUM_CHAR_CLASSES],0,0		/* gobble up white space */

const NXFSM _NXEnglishBreakTable[] = {
  /*	   Letter      Digit       Period      Quote       Comma       Special     Control     White       NL          Other	*/

 /*EINIT1*/{BGBL},     {BGBL},     {BGBL},     {BGBL},     {BGBL},     {BGBL},     {NULL,0,B}, {WGBL},     {NULL,0,N}, {BGBL},
 /*BGBL*/  {BGBL},     {BGBL},     {BGBL},     {BGBL},     {BGBL},     {BGBL},     {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {BGBL},
 /*WGBL*/  {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {WGBL},     {NULL,0,N}, {NULL,1,W}
};

/* states for non-word wrapping table */
#define EINIT2	&_NXEnglishNoBreakTable[0*NUM_CHAR_CLASSES],0,0
#define GOBL	&_NXEnglishNoBreakTable[1*NUM_CHAR_CLASSES],0,0	/* gobble up a whole line */

const NXFSM _NXEnglishNoBreakTable[] = {
  /*	   Letter      Digit       Period      Quote       Comma       Special     Control     White       NL          Other	*/

 /*EINIT2*/{GOBL},     {GOBL},     {GOBL},     {GOBL},     {GOBL},     {GOBL},     {NULL,0,B}, {GOBL},     {NULL,0,N}, {GOBL},
 /*GOBL*/  {GOBL},     {GOBL},     {GOBL},     {GOBL},     {GOBL},     {GOBL},     {NULL,1,B}, {GOBL},     {NULL,1,B}, {GOBL}
};

/* states for double click table */
#define EINIT3	&_NXEnglishClickTable[0*NUM_CHAR_CLASSES],0,0
#define PIECE	&_NXEnglishClickTable[1*NUM_CHAR_CLASSES],0,0	/* base state, achieved after a piece of a word */
#define AFLET	&_NXEnglishClickTable[2*NUM_CHAR_CLASSES],0,0	/* after a letter */
#define AFDIG	&_NXEnglishClickTable[3*NUM_CHAR_CLASSES],0,0	/* after a digit */
#define WHITE	&_NXEnglishClickTable[4*NUM_CHAR_CLASSES],0,0	/* gobbles white space */
#define FRSTP	&_NXEnglishClickTable[5*NUM_CHAR_CLASSES],0,0	/* after a period leading period */
#define AFPC	&_NXEnglishClickTable[6*NUM_CHAR_CLASSES],0,0	/* after a period or (comma (after a digit)) */
#define AFPOP	&_NXEnglishClickTable[7*NUM_CHAR_CLASSES],0,0	/* after a (single quote or right quote) (after a digit or letter) */

const NXFSM _NXEnglishClickTable[] = {
  /*	   Letter      Digit       Period      Quote       Comma       Special     Control     White       NL          Other	*/

 /*EINIT3*/{AFLET},    {AFDIG},    {FRSTP},    {NULL,0,B}, {NULL,0,B}, {PIECE},    {NULL,0,B}, {WHITE},    {NULL,0,N}, {NULL,0,B},
 /*PIECE*/ {AFLET},    {AFDIG},    {AFPC},     {NULL,1,B}, {NULL,1,B}, {PIECE},    {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {NULL,1,B},
 /*AFLET*/ {AFLET},    {AFDIG},    {AFPC},     {AFPOP},    {NULL,1,B}, {PIECE},    {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, 
 /*AFDIG*/ {AFLET},    {AFDIG},    {AFPC},     {AFPOP},    {AFPC},     {PIECE},    {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {NULL,1,B},
 /*WHITE*/ {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {NULL,1,W}, {WHITE},    {NULL,0,N}, {NULL,1,W},
 /*FRSTP*/ {NULL,1,B}, {AFDIG},    {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, {NULL,1,B}, 
 /*AFPC*/  {NULL,2,B}, {AFDIG},    {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, 
 /*AFPOP*/ {AFLET},    {AFDIG},    {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}, {NULL,2,B}
};


/**** C Language Word Tables ****/

unsigned const char _NXCSmartLeftChars[] = {
    ' ', NX_FIGSPACE, '\n', '\t', '!', '\"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', '-', '.', '/', ':', '<', '=', '>', '?', '@',
    '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~',
    '\xa1',	/* exclamdown */
    '\xa7',	/* section */
    '\xa8',	/* currency */
    '\xa9',	/* quotesingle */
    '\xaa',	/* quotedblleft */
    '\xab',	/* guillemotleft */
    '\xac',	/* guilsinglleft */
    '\xd1',	/* plusminus */
    '\0'};

unsigned const char _NXCSmartRightChars[] = {
    ' ', NX_FIGSPACE, '\n', '\t', '!', '\"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@',
    '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~',
    '\xa0',	/* copyright */
    '\xa2',	/* cent */
    '\xad',	/* guilsinglright */
    '\xb0',	/* registered */
    '\xa2',	/* dagger */
    '\xa3',	/* daggerdbl */
    '\xba',	/* quotedblright */
    '\xbb',	/* guillemotright */
    '\xbd',	/* perthousand */
    '\xc0',	/* onesuperior */
    '\xc9',	/* twosuperior */
    '\xcc',	/* threesuperior */
    '\xe3',	/* ordfeminine */
    '\xeb',	/* ordmasculine */
    '\0'};


/*  The character classes for C.  NX_cccDigit is 1-9. NX_cccQuote includes
    normal single quote and "smart" single quote.  NX_cccControl are the
    control characters. NX_cccOther are punctuation and other singular chars */
enum _NXCCharCats {	
	NX_cccLetter = 0 * sizeof(NXFSM),	NX_cccDigit = 1 * sizeof(NXFSM),	NX_cccZero = 2 * sizeof(NXFSM),
	NX_cccPeriod = 3 * sizeof(NXFSM),	NX_cccQuote = 4 * sizeof(NXFSM),	NX_cccControl = 5 * sizeof(NXFSM),
	NX_cccWhite = 6 * sizeof(NXFSM),	NX_cccNL = 7 * sizeof(NXFSM),		NX_cccOther = 8 * sizeof(NXFSM),
	NX_cccPlus = 9 * sizeof(NXFSM),		NX_cccMinus = 10 * sizeof(NXFSM),	NX_cccGThan = 11 * sizeof(NXFSM),
	NX_cccLThan = 12 * sizeof(NXFSM),	NX_cccExclam = 13 * sizeof(NXFSM),	NX_cccAmper = 14 * sizeof(NXFSM),
	NX_cccBar = 15 * sizeof(NXFSM),		NX_cccStar = 16 * sizeof(NXFSM),	NX_cccEqual = 17 * sizeof(NXFSM),
	NX_cccBSlash = 18 * sizeof(NXFSM),	NX_cccSlash = 19 * sizeof(NXFSM),	NX_cccHat = 20 * sizeof(NXFSM),
	NX_cccPerc = 21 * sizeof(NXFSM)
};

#undef NUM_CHAR_CLASSES
#define NUM_CHAR_CLASSES 22

/* table mapping chars to their classes.  _, @, #, and $ act like letters for C and ObjC. */
const unsigned char _NXCCharCatTable[256] = {
	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,
	NX_cccControl,	NX_cccWhite,	NX_cccNL,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,
	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,
	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,	NX_cccControl,

	NX_cccWhite,	NX_cccExclam,	NX_cccOther,	NX_cccLetter,	NX_cccLetter,	NX_cccPerc,	NX_cccAmper,	NX_cccQuote,
	NX_cccOther,	NX_cccOther,	NX_cccStar,	NX_cccPlus,	NX_cccOther,	NX_cccMinus,	NX_cccPeriod,	NX_cccSlash,
	NX_cccZero,	NX_cccDigit,	NX_cccDigit,	NX_cccDigit,	NX_cccDigit,	NX_cccDigit,	NX_cccDigit,	NX_cccDigit,
	NX_cccDigit,	NX_cccDigit,	NX_cccOther,	NX_cccOther,	NX_cccLThan,	NX_cccEqual,	NX_cccGThan,	NX_cccOther,

	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccOther,	NX_cccBSlash,	NX_cccOther,	NX_cccHat,	NX_cccLetter,

	NX_cccOther,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccOther,	NX_cccBar,	NX_cccOther,	NX_cccOther,	NX_cccControl,

	NX_cccOther,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccOther,	NX_cccOther,

	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,
	NX_cccOther,	NX_cccQuote,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccLetter,	NX_cccLetter,
	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,
	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,

	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,
	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,
	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccOther,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,

	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,
	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccLetter,	NX_cccControl,	NX_cccControl
};

/* states for word wrapping table */
#undef BGBL
#undef WGBL
#define X	NULL						/* a termination state */
#define CINIT1	&_NXCBreakTable[0*NUM_CHAR_CLASSES],0,0
#define BGBL	&_NXCBreakTable[1*NUM_CHAR_CLASSES],0,0		/* gobble up a word */
#define WGBL	&_NXCBreakTable[2*NUM_CHAR_CLASSES],0,0		/* gobble up white space */

const NXFSM _NXCBreakTable[] = {
  /*	   Letter  Digit   Zero    Period  Quote   Control White   NL      Other   +       -       >       <       !       &       |       *       =       \       /       ^       %    */

 /*CINIT1*/{BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {X,0,B},{WGBL}, {X,0,N},{BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL},
 /*BGBL*/  {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {X,1,B},{X,1,B},{X,1,B},{BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL}, {BGBL},
 /*WGBL*/  {X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{WGBL}, {X,0,N},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W}
};

/* states for double click table */
#undef WHITE
#undef AFPOP
#define CINIT2	&_NXCClickTable[0*NUM_CHAR_CLASSES],0,0
#define ID	&_NXCClickTable[1*NUM_CHAR_CLASSES],0,0	/* base ID state, achieved after a piece of an ID */
#define AFLD	&_NXCClickTable[2*NUM_CHAR_CLASSES],0,0	/* after a letter or digit while scanning an ID */
#define NUM	&_NXCClickTable[3*NUM_CHAR_CLASSES],0,0	/* after a piece of a number */
#define WHITE	&_NXCClickTable[4*NUM_CHAR_CLASSES],0,0	/* gobbles white space */
#define AFPER	&_NXCClickTable[5*NUM_CHAR_CLASSES],0,0	/* after a period after a digit */
#define AFPOP	&_NXCClickTable[6*NUM_CHAR_CLASSES],0,0	/* after a (single quote or right quote) (after a digit or letter) */
#define AFBS1	&_NXCClickTable[7*NUM_CHAR_CLASSES],0,0	/* after a backslash (\0 special cased as a word) */
#define AFBS2	&_NXCClickTable[8*NUM_CHAR_CLASSES],0,0	/* after 1st char after a backslash */
#define AFBS3	&_NXCClickTable[9*NUM_CHAR_CLASSES],0,0	/* after 2nd char after a backslash */
#define PLUS	&_NXCClickTable[10*NUM_CHAR_CLASSES],0,0/* after a plus */
#define MINUS	&_NXCClickTable[11*NUM_CHAR_CLASSES],0,0/* after a minus */
#define GT	&_NXCClickTable[12*NUM_CHAR_CLASSES],0,0/* after a greater than */
#define LT	&_NXCClickTable[13*NUM_CHAR_CLASSES],0,0/* after a less than */
#define BANG	&_NXCClickTable[14*NUM_CHAR_CLASSES],0,0/* after an exclamation point */
#define AMPER	&_NXCClickTable[15*NUM_CHAR_CLASSES],0,0/* after an ampersand */
#define BAR	&_NXCClickTable[16*NUM_CHAR_CLASSES],0,0/* after a vertical bar */
#define STAR	&_NXCClickTable[17*NUM_CHAR_CLASSES],0,0/* after a star */
#define EQ	&_NXCClickTable[18*NUM_CHAR_CLASSES],0,0/* after an equal sign */
#define SLASH	&_NXCClickTable[19*NUM_CHAR_CLASSES],0,0/* after a slash */
#define HAT	&_NXCClickTable[20*NUM_CHAR_CLASSES],0,0/* after a hat */
#define PERC	&_NXCClickTable[21*NUM_CHAR_CLASSES],0,0/* after a percent sign */

const NXFSM _NXCClickTable[] = {
  /*	   Letter  Digit   Zero    Period  Quote   Control White   NL      Other   +       -       >       <       !       &       |       *       =       \       /       ^       %    */

 /*CINIT2*/{ID},   {NUM},  {NUM},  {AFPER},{X,0,B},{X,0,B},{WHITE},{X,0,N},{X,0,B},{PLUS}, {MINUS},{GT},   {LT},   {BANG}, {AMPER},{BAR},  {STAR}, {EQ},   {AFBS1},{SLASH},{HAT},  {PERC},
 /*ID*/    {AFLD}, {AFLD}, {AFLD}, {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*AFLD*/  {AFLD}, {AFLD}, {AFLD}, {X,1,B},{AFPOP},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*NUM*/   {NUM},  {NUM},  {NUM},  {AFPER},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*WHITE*/ {X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{WHITE},{X,0,N},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},{X,1,W},
 /*AFPER*/ {X,1,B},{NUM},  {NUM},  {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*AFPOP*/ {AFLD}, {AFLD}, {AFLD}, {X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},{X,2,B},
 /*AFBS1*/ {X,0,B},{AFBS2},{X,0,B},{X,0,B},{X,0,B},{X,1,B},{X,0,B},{X,0,N},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},{X,0,B},
 /*AFBS2*/ {X,1,B},{AFBS3},{AFBS3},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*AFBS3*/ {X,1,B},{X,0,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*PLUS*/  {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{PLUS}, {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*MINUS*/ {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{MINUS},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*GT*/    {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{GT},   {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*LT*/    {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{LT},   {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*BANG*/  {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{BANG}, {X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*AMPER*/ {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{AMPER},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*BAR*/   {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{BAR},  {X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*STAR*/  {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},
 /*EQ*/    {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{EQ},   {X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*SLASH*/ {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*HAT*/   {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
 /*BAR*/   {X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},{X,0,B},{X,1,B},{X,1,B},{X,1,B},{X,1,B},
};



static void writeFSM(NXTypedStream *ts, NXFSM *table, int count);
static void readFSM(NXZone *zone, NXTypedStream *ts, NXFSM **table, int *count);

/* keys to ID which types of info we put in a file */
#define SMART_LEFT	1
#define SMART_RIGHT	2
#define WORD_BREAKS	4

void NXWriteWordTable(NXStream *st, unsigned char *smartLeft,
		unsigned char *smartRight, unsigned char *charClasses,
		NXFSM *wrapBreaks, int wrapBreaksCount,
		NXFSM *clickBreaks, int clickBreaksCount, BOOL charWrap)
{
    NXTypedStream *ts;
    int piecesMask = 0;		/* mask of whats going in the stream */
    
    if (!(ts = NXOpenTypedStream(st, NX_WRITEONLY)))
	NX_RAISE(NX_wordTablesWrite, st, 0);

    if (smartLeft)
	piecesMask |= SMART_LEFT;
    if (smartRight)
	piecesMask |= SMART_RIGHT;
    if (charClasses || wrapBreaks || clickBreaks) {
	if (!charClasses || !wrapBreaks || !clickBreaks)
	    NX_RAISE(NX_wordTablesWrite, st, 0);
	piecesMask |= WORD_BREAKS;
    }
    NXWriteType(ts, "i", &piecesMask);

    if (piecesMask & SMART_LEFT)
	NXWriteType(ts, "*", &smartLeft);
    if (piecesMask & SMART_RIGHT)
	NXWriteType(ts, "*", &smartRight);
    if (piecesMask & WORD_BREAKS) {
	NXWriteType(ts, "c", &charWrap);
	NXWriteArray(ts, "c", 256, charClasses);
	writeFSM(ts, wrapBreaks, wrapBreaksCount);
	writeFSM(ts, clickBreaks, clickBreaksCount);
    }

    NXCloseTypedStream(ts);
}


void NXReadWordTable(NXZone *zone, NXStream *st, unsigned char **smartLeft,
		unsigned char **smartRight, unsigned char **charClasses,
		NXFSM **wrapBreaks, int *wrapBreaksCount,
		NXFSM **clickBreaks, int *clickBreaksCount, BOOL *charWrap)
{
    NXTypedStream *ts;
    int piecesMask = 0;		/* mask of whats in the stream */

    if (!(ts = NXOpenTypedStream(st, NX_READONLY)))
	NX_RAISE(NX_wordTablesRead, st, 0);

    NXReadType(ts, "i", &piecesMask);

    if (piecesMask & SMART_LEFT)
	NXReadType(ts, "*", smartLeft);
    else
	*smartLeft = NULL;
    if (piecesMask & SMART_RIGHT)
	NXReadType(ts, "*", smartRight);
    else
	*smartRight = NULL;
    if (piecesMask & WORD_BREAKS) {
	NXReadType(ts, "c", charWrap);
	*charClasses = NXZoneMalloc(zone, 256 * sizeof(char));
	NXReadArray(ts, "c", 256, *charClasses);
	readFSM(zone, ts, wrapBreaks, wrapBreaksCount);
	readFSM(zone, ts, clickBreaks, clickBreaksCount);
    } else {
	*charClasses = NULL;
	*wrapBreaks = NULL;
	*clickBreaks = NULL;
    }

    NXCloseTypedStream(ts);
}


static void writeFSM(NXTypedStream *ts, NXFSM *table, int count)
{
    int i;
    NXFSM *trans;
    int scratch;

    NXWriteType(ts, "i", &count);
    for (i = count, trans = table; i--; trans++) {
	if (trans->next)
	    scratch = trans->next - table;
	else
	    scratch = -1;
	NXWriteTypes(ts, "iss", &scratch, &trans->delta, &trans->token);
    }
}


static void readFSM(NXZone *zone, NXTypedStream *ts, NXFSM **table, int *count)
{
    int i;
    int transOffset;
    NXFSM *trans;

    NXReadType(ts, "i", count);
    *table = NXZoneMalloc(zone, *count * sizeof(NXFSM));
    for (i = *count, trans = *table; i--; trans++) {
	NXReadTypes(ts, "iss", &transOffset, &trans->delta, &trans->token);
	if (transOffset >= 0)
	    trans->next = *table + transOffset;
	else
	    trans->next = NULL;
    }
}


void _NXCheckWordTableSizes()
{
    AK_ASSERT(sizeof(_NXEnglishBreakTable)/sizeof(NXFSM) == NXEnglishBreakTableSize, "Word table sizes are wrong");
    AK_ASSERT(sizeof(_NXEnglishClickTable)/sizeof(NXFSM) == NXEnglishClickTableSize, "Word table sizes are wrong");
    AK_ASSERT(sizeof(_NXCBreakTable)/sizeof(NXFSM) == NXCBreakTableSize, "Word table sizes are wrong");
    AK_ASSERT(sizeof(_NXCClickTable)/sizeof(NXFSM) == NXCClickTableSize, "Word table sizes are wrong");
}

/* NXStringOrderTable support */

NXStringOrderTable *NXDefaultStringOrderTable(void)
{
    int fd = -1;
    const char *const *languages;
    char file[MAXPATHLEN+1];
    static BOOL haveDefaultStringOrderTable = YES;
    static NXStringOrderTable *defaultStringOrderTable = NULL;

    if (!defaultStringOrderTable && haveDefaultStringOrderTable) {
	languages = [NXApp systemLanguages];
	while (fd < 0 && languages && *languages) {
	    if (!strcmp(*languages, KIT_LANGUAGE)) break;
	    sprintf(file, "%s/Languages/%s/CharOrdering", _NXAppKitFilesPath, *languages);
	    fd = open(file, O_RDONLY, 0);
	    languages++;
	}
	if (fd < 0) {
	    sprintf(file, "%s/CharOrdering", _NXAppKitFilesPath);
	    fd = open(file, O_RDONLY, 0);
	}
	if (fd >= 0) {
	    defaultStringOrderTable = (NXStringOrderTable *)NXZoneCalloc(NXDefaultMallocZone(), 1, sizeof(NXStringOrderTable));
	    if (read(fd, defaultStringOrderTable, sizeof(NXStringOrderTable)) != sizeof(NXStringOrderTable)) {
		free(defaultStringOrderTable);
		defaultStringOrderTable = NULL;
		haveDefaultStringOrderTable = NO;
		NX_ASSERT(YES, "Couldn't load default character ordering file.");
	    }
	    (void)close(fd);
	} else {
	    haveDefaultStringOrderTable = NO;
	    NX_ASSERT(YES, "Couldn't find default character ordering file.");
	}
    }

    return defaultStringOrderTable;
}

int _NXOrderStringsWithLength(const unsigned char *s1, const unsigned char *s2, BOOL caseSensitive, int length)
{
    return NXOrderStrings(s1, s2, caseSensitive, length, NULL);
}

int _NXOrderStrings(const unsigned char *s1, const unsigned char *s2, BOOL caseSensitive)
{
    return NXOrderStrings(s1, s2, caseSensitive, -1, NULL);
}

int NXOrderStrings(const unsigned char *s1, const unsigned char *s2, BOOL caseSensitive, int length, NXStringOrderTable *table)
{
    unsigned char ord1, ord2;
    unsigned char secOrd1 = 0, secOrd2 = 0;
    unsigned const char *primaryTable, *secondaryTable;
    unsigned const char *word1 = s1, *word2 = s2;

    if (!length) return 0;

    if (!table) table = NXDefaultStringOrderTable();

    if (!table) {			/* this only works for ASCII */
	if (caseSensitive) {
	    return (length > 0) ? strncmp((char *)s1, (char *)s2, length) : strcmp((char *)s1, (char *)s2);
	} else {
	    while (*s1 && *s2 && tolower(*s1) == tolower(*s2) && length) {
		s1++; s2++; length--;
	    }
	    if (!length) return 0;
	    if (!*s1 && *s2) return -1;
	    if (!*s2 && *s1) return 1;
	    if (!*s2) return 0;
	    if (tolower(*s1) < tolower(*s2)) {
		return -1;
	    } else if (tolower(*s1) > tolower(*s2)) {
		return 1;
	    } else {
		while (*word1 == *word2) {
		    word1++;
		    word2++;
		}
		if (islower(*word1) && isupper(*word2)) {
		    return -1;
		}
		if (islower(*word2) && isupper(*word1)) {
		    return 1;
		}
	    }
	    return 0;
	}
    }

    if (caseSensitive) {
	primaryTable = table->primary;
	secondaryTable = table->secondary;
    } else {
	primaryTable = table->primaryCI;
	secondaryTable = table->secondaryCI;
    }

    ord1 = primaryTable[*s1];
    ord2 = primaryTable[*s2];

    while (*s1) {
	if (ord1 == ord2) {
	    if (length > 0 && (((s1 - word1) == (length-1)) || ((s2 - word2) == (length-1)))) return 0;
	  /* get next ordinal value for the first string */
	    if (!secOrd1) {
		secOrd1 = secondaryTable[*s1];
		if (!secOrd1) {
		    ord1 = primaryTable[*++s1];
		} else {
		    ord1 = secOrd1;
		}
	    } else {
		ord1 = primaryTable[*++s1];
	    }
	  /* get next ordinal value for the second string */
	    if (!secOrd2) {
		secOrd2 = secondaryTable[*s2];
		if (!secOrd2) {
		    ord2 = primaryTable[*++s2];
		} else {
		    ord2 = secOrd2;
		}
	    } else {
		ord2 = primaryTable[*++s2];
	    }
	} else if (ord1 > ord2) {
	    return 1;
	} else {
	    return -1;
	}
    }

    return *s2 ? -1 : 0;
}

/*

Modifications (starting at 0.8):

80
--
 4/08/90 trey	added NXOrderStrings, NXOrderStringsWithLength

86
--
 6/12/90 pah	added temporary implementations of NXOrderStrings*

*/

/*
	rtfdefs.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

/*
 *  These defines are used to specify the current group type.  This
 * will enable us to always know whether we are within the bounds a
 * a certain group.
 */
#define FONTTABLE_GROUP    1
#define STYLESHEET_GROUP   2
#define COLORTABLE_GROUP   3
#define PICTURE_GROUP      4
#define FOOTNOTE_GROUP     5
#define HEADER_GROUP       6
#define FOOTER_GROUP       7
#define INFO_GROUP         8
#define DOCUMENT_GROUP     9
#define STYLE_GROUP        10
#define ATTACHMENT_GROUP   11
#define SPECIAL_GROUP      12


/*
 *  Error values.
 */
#define BAD_FONTTABLE      1
#define BAD_DIRECTIVE      2
#define BAD_STYLESHEET     3
#define BAD_PROPERTY       4

/*
 *  I took these two defines from textprivate.h.
 */
#define TXISPARA(LD)  (LD & (int)0x4000)

#define LBRACE '{'
#define RBRACE '}'

/* to be tuned later */
#define BSIZE 1024
#define FIRSTCHUNK 10

#define BOLD 1
#define ITALIC 2
#define STRIKETHROUGH 4
#define OUTLINE 8
#define SHADOW 16
#define UNDERLINE 32

#define SPECIAL 1		/* val = char value, 0 means no value */
#define DEST 2			/* val is destination, 0 mean ignore dest */
#define PROP 3		/* val is number of parameters */

#define FONTPROP 1
#define BOLDPROP 2
#define ITALICPROP 3
#define SIZEPROP 4
#define FIRSTINDENTPROP 5
#define LEFTINDENTPROP 6
#define LEFTJUSTPROP 7
#define RIGHTJUSTPROP  8
#define JUSTIFYPROP 9
#define CENTERPROP 10
#define INITPROP 11
#define SUPERSCRIPTPROP 12
#define SUBSCRIPTPROP 13
#define PLAINPROP 14
#define GRAYPROP 15
#define UNDERLINEPROP 16
#define STOPUNDERLINEPROP 17
#define COLORPROP 18
#define REDPROP 19
#define GREENPROP 20
#define BLUEPROP 21

#define PAPERW_SPECIAL (1 + 0xFF)
#define PAPERH_SPECIAL (2 + 0xFF)
#define NEXTSTYLE_SPECIAL  (3 + 0xFF)
#define BASEDONSTYLE_SPECIAL (4 + 0xFF)
#define SETTAB_SPECIAL   (5 + 0xFF)
#define STYLE_SPECIAL (6 + 0xFF)
#define HEX_SPECIAL (7+0xFF)
#define MARGL_SPECIAL (8+0xFF)
#define MARGR_SPECIAL (9+0xFF)
#define SMARTCOPY_SPECIAL (10+0xFF)

#define DOCDEST 1
#define FONTDEST 2

#define CONVERSION (20.0)
#define CONVERT(x) floor((((float)x)/20.0))
#define VERY_BIG_NUMBER 1.0e38


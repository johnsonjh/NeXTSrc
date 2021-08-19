/*
	rtfstructs.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifndef RTFSTRUCTS_H
#define RTFSTRUCTS_H

#import "Text.h"
#import "rtfdefs.h"
#import "FontManager.h"
#import <objc/hashtable.h>

typedef struct {
    NXAtom family;
    NXAtom rtf;
} RichTextFamily;


typedef struct {
    char *adobeName, *family, *font;
    int props;
} NXFontStyle;

typedef struct {
    id font;
    int fontNum;
    NXAtom family;
    NXAtom rtfFamily;
    NXFontTraitMask face;
}  NXFontEntry;

typedef struct {
    int num, seen;
    NXAtom fontName;
}  NXFontItem;

@interface NXTabStopList:Object
{
@public
    int numTabs;
    int maxTabs;
    NXTabStop *tabs;
    id copies;
    BOOL needSorting;
}

- init;
- addTabStop:(NXCoord) pos kind:(short)kind;
- (NXTabStop *)tabs;
- empty;
@end

typedef struct {
    int font, size, group, textGray, textRGBColor;
    NXFontTraitMask face;
    unsigned char superscript;
    unsigned char subscript;
    NXTextStyle textStyle;
    int numTabs;
    id info;
    NXTabStop *tabs;
    NXTabStopList *buildTabs;
    BOOL tabsCopied;
    BOOL underline;
} NXProps;

typedef struct _NXStyleSheetEntry {
    int num;
    char *name;
    int nextStyle;
    int basedOnStyle;
    NXProps prop;
} NXStyleSheetEntry;


typedef struct _NXSymbolTable {
    char *name;
    unsigned char type;
    int value;
}  NXSymbolTable;

@interface Text(GraphicsSupport)

+ classForDirective:(const char *)directive;
+ (const NXSymbolTable *) symbolForDirective:(const char *)directive;
+ (const char *) directiveForClass:class;
- readCellRichText:(NXStream *)stream sym:(NXSymbolTable *)st;
@end

#endif RTFSTRUCTS_H

/*
 
 87
 --
7/12/90	glc	Changes to support color.

99
--
10/17/90 glc    Changes to support reentrant RTF


*/
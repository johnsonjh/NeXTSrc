/*
	FontManager.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>

typedef unsigned int NXFontTraitMask;

/*
 * Font Traits
 *
 * This list should be kept small since the more traits that are assigned
 * to a given font, the harder it will be to map it to some other family.
 * Some traits are mutually exclusive such as NX_EXPANDED and NX_CONDENSED.
 */

#define NX_ITALIC		0x00000001
#define NX_BOLD			0x00000002
#define NX_UNBOLD		0x00000004
#define NX_NONSTANDARDCHARSET	0x00000008
#define NX_NARROW		0x00000010
#define NX_EXPANDED		0x00000020
#define NX_CONDENSED		0x00000040
#define NX_SMALLCAPS		0x00000080
#define NX_POSTER		0x00000100
#define NX_COMPRESSED		0x00000200
#define NX_UNITALIC 		0x01000000	/* historical */

/* whatToDo values */

#define NX_NOFONTCHANGE		0
#define NX_VIAPANEL		1
#define NX_ADDTRAIT		2
#define NX_SIZEUP		3
#define NX_SIZEDOWN		4
#define NX_HEAVIER		5
#define NX_LIGHTER		6
#define NX_REMOVETRAIT		7

#define NX_CHANGETRAIT		2	/* historical */

@interface FontManager : Object
{
    id                  panel;
    id                  menu;
    SEL                 action;
    int                 whatToDo;
    NXFontTraitMask     traitToChange;
    id                  selFont;
    struct _fmFlags {
	unsigned int        multipleFont:1;
	unsigned int        disabled:1;
	unsigned int        _RESERVED:14;
    }                   fmFlags;
    unsigned short      _lastPos;
    unsigned int        _reservedFMint1;
    unsigned int        _reservedFMint2;
    unsigned int        _reservedFMint3;
    unsigned int        _reservedFMint4;
}

+ setFontPanelFactory:factoryId;
+ new;
+ allocFromZone:(NXZone *)zone;
+ alloc;

- (BOOL)isMultiple;
- selFont;
- setSelFont:fontObj isMultiple:(BOOL)flag;
- convertFont:fontObj;
- getFontMenu:(BOOL)create;
- getFontPanel:(BOOL)create;
- findFont:(const char *)family traits:(NXFontTraitMask)traits weight:(int)weight size:(float)size;
- getFamily:(const char **)family traits:(NXFontTraitMask *)traits weight:(int *)weight size:(float *)size ofFont:fontObj;
- (char **)availableFonts;
- convert:fontObj toFamily:(const char *)family;
- convert:fontObj toHaveTrait:(NXFontTraitMask)trait;
- convert:fontObj toNotHaveTrait:(NXFontTraitMask)trait;
- convertWeight:(BOOL)upFlag of:fontObj;
- addFontTrait:sender;
- removeFontTrait:sender;
- modifyFont:sender;
- orderFrontFontPanel:sender;
- modifyFontViaPanel:sender;
- (BOOL)isEnabled;
- setEnabled:(BOOL)flag;
- (SEL)action;
- setAction:(SEL)aSelector;
- finishUnarchiving;
- sendAction;

@end

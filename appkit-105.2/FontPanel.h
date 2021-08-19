/*
	FontPanel.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved.
*/

#import "Panel.h"
#import "Font.h"

/* Tags of views in the FontPanel */

#define NX_FPPREVIEWBUTTON	131
#define NX_FPREVERTBUTTON	130
#define NX_FPSETBUTTON		132
#define NX_FPPREVIEWFIELD	128
#define NX_FPSIZEFIELD		129
#define NX_FPSIZETITLE		133
#define NX_FPCURRENTFIELD	134

@interface FontPanel : Panel
{
    id                  faces;
    id                  families;
    id                  preview;
    id                  current;
    id                  size;
    id                  sizes;
    id                  manager;
    id                  selFont;
    NXFontMetrics      *selMetrics;
    int                 curTag;
    id                  accessoryView;
    id                  _reserved1;
    id                  setButton;
    id                  separator;
    id                  sizeTitle;
    char               *lastPreview;
    struct _fpFlags {
	unsigned int        multipleFont:1;
	unsigned int        dirty:1;
	unsigned int        _RESERVED:13;
	unsigned int        _dontPreview:1;
    }                   fpFlags;
    unsigned short      _reservedFPshort1;
    id                  _reserved2;
    id		        _chooser;
    id		        _titles;
    id		        _previewBox;
}

+ new;
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;

+ allocFromZone:(NXZone *)zone;
+ alloc;

- accessoryView;
- setAccessoryView:aView;
- textDidEnd:textObject endChar:(unsigned short)endChar;
- textDidGetKeys:textObject isEmpty:(BOOL)flag;
- orderWindow:(int)place relativeTo:(int)otherWin;
- windowWillResize:sender toSize:(NXSize *)frameSize;
- setPanelFont:fontObj isMultiple:(BOOL)flag;
- panelConvertFont:fontObj;
- (BOOL)worksWhenModal;
- (BOOL)isEnabled;
- setEnabled:(BOOL)flag;

@end

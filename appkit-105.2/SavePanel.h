/*
	SavePanel.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Panel.h"

/* Tags of views in the SavePanel */

#define NX_SPICONBUTTON		150
#define NX_SPTITLEFIELD		151
#define NX_SPBROWSER		152
#define NX_SPCANCELBUTTON	NX_CANCELTAG
#define NX_SPOKBUTTON		NX_OKTAG
#define NX_SPFORM		155

typedef struct __NXDirInfo *_NXDirInfo;

extern int NXCompleteFilename(char *path, int maxPathSize);

@interface SavePanel : Panel
{
    id                  form;
    id                  browser;
    id                  okButton;
    id                  accessoryView;
    id                  separator;
    char               *filename;
    char               *directory;
    const char        **filenames;
    char               *requiredType;
    _NXDirInfo         *_columns;
    NXHashTable        *_typeTable;
    struct _spFlags {
	unsigned int        opening:1;
	unsigned int        exitOk:1;
	unsigned int        allowMultiple:1;
	unsigned int        dirty:1;
	unsigned int        invalidateMatrices:1;
	unsigned int        filtered:1;
	unsigned int        _RESERVED:2;
	unsigned int        _largeFS:1;
	unsigned int	    _delegateValidatesNew:1;
	unsigned int	    _delegateValidatesOld:1;
	unsigned int        _checkCase:1;
	unsigned int        _cancd:1;
	unsigned int        _UnixExpert:1;
	unsigned int        _backwards:1;
	unsigned int        _forwards:1;
    }                   spFlags;
    unsigned short      directorySize;
    int                 _cdcolumn;
    IMP                 _filterMethod;
    id                  _homeButton;
    id                  _spreserved6;
    id                  _spreserved7;
    id             	_spreserved8;
}

+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;

+ allocFromZone:(NXZone *)zone;
+ alloc;

- free;
- ok:sender;
- cancel:sender;
- (int)runModalForDirectory:(const char *)path file:(const char *)name;
- (int)runModal;
- (const char *)filename;
- (const char *)directory;
- setDirectory:(const char *)path;
- setPrompt:(const char *)prompt;
- setTitle:(const char *)title;
- (const char *)requiredFileType;
- setRequiredFileType:(const char *)type;
- accessoryView;
- setAccessoryView:aView;
- selectText:sender;
- textDidEnd:textObject endChar:(unsigned short)endChar;
- textDidGetKeys:textObject isEmpty:(BOOL)flag;
- (BOOL)commandKey:(NXEvent *)theEvent;
- setDelegate:anObject;

@end

@interface Object(SavePanelDelegate)
- (BOOL)panelValidateFilenames:sender;
- (BOOL)panel:sender filterFile:(const char *)filename inDirectory:(const char *)directory;
@end

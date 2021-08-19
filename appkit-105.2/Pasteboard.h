/*
	Pasteboard.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import <objc/hashtable.h>

/* standard Pasteboard types */

extern NXAtom NXAsciiPboardType;
extern NXAtom NXPostScriptPboardType;
extern NXAtom NXTIFFPboardType;
extern NXAtom NXRTFPboardType;
extern NXAtom NXFilenamePboardType;
extern NXAtom NXTabularTextPboardType;
extern NXAtom NXFontPboardType;
extern NXAtom NXRulerPboardType;

/* historical types */

#define NXAsciiPboard		NXAsciiPboardType
#define NXPostScriptPboard	NXPostScriptPboardType
#define NXTIFFPboard		NXTIFFPboardType
#define NXRTFPboard		NXRTFPboardType
#define NXFilenamePboard	NXFilenamePboardType
#define NXTabularTextPboard	NXTabularTextPboardType
#define NXFindInfoPboard	NXFindPboardType

/* standard Pasteboard names */

extern NXAtom NXSelectionPboard;
extern NXAtom NXFontPboard;
extern NXAtom NXRulerPboard;
extern NXAtom NXFindPboard;

/* historical names */

#define NXSelectionPBName	NXSelectionPboard
#define NXRulerPBName		NXRulerPboard
#define NXFontPBName		NXFontPboard
#define NXFindInfoPBName	NXFindPboard

@interface Pasteboard : Object
{
    id                  owner;
    int                 _realChangeCount;
    int                 _ourChangeCount;
    port_t              _server;
    void               *_reservedPasteboard5;
    unsigned int        _reservedPasteboard6;
    NXAtom             *_typesArray;
    port_t              _client;
    BOOL               *_typesProvided;
    NXAtom _name;
    NXAtom _host;
    unsigned int        _reservedPasteboard3;
    unsigned int        _reservedPasteboard4;
}

+ new;
+ newName:(const char *)name;
+ allocFromZone:(NXZone *)zone;
+ alloc;

- (const char *)name;
- free;
- freeGlobally;
- declareTypes:(const char *const *)newTypes num:(int)numTypes owner:newOwner;
- (int)changeCount;
- writeType:(const char *)dataType data:(const char *)theData length:(int)numBytes;
- (const NXAtom *)types;
- readType:(const char *)dataType data:(char **)theData length:(int *)numBytes;

@end

@interface Object(PasteboardOwner)
- pasteboard:sender provideData:(NXAtom)type;
@end

/*

	FontFile.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#import "FontManager.h"

@interface FontFile : Object
{
    char *data;
    int count;
    int byteSex;
}

+ new:(const char *)fontsDirectory;
- init:(const char *)fontsDirectory;

- (unsigned int)count;
- (int)offset:(const char *)name;
- (int)matchFamily:(const char *)family    traits:(NXFontTraitMask)flags    weight:(int)weight;
- (int)matchTrait:(NXFontTraitMask)trait to:fontFile at:(int)offset;
- (int)excludeTrait:(NXFontTraitMask)trait from:fontFile at:(int)offset;
- (int)matchFamily:(const char *)family to:fontFile at:(int)offset;
- (int)modifyWeight:(BOOL)upFlag of:fontFile at:(int)offset;
- (char *)family:(int)offset;
- (char *)face:(int)offset;
- (char *)name:(int)offset;
- (char *)getFullName:(char *)fullName of:(int)offset withSize:(float)size;
- (BOOL)isItalic:(int)offset;
- (BOOL)isBold:(int)offset;
- (BOOL)isUnbold:(int)offset;
- (int)weight:(int)offset;
- (NXFontTraitMask)traits:(int)offset;
@end


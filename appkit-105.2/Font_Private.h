#import "Font.h"

@interface Font(Private)

+ (void)_freePBSData;
+ _findFont:(const char *)fontName size:(float)fontSize style:(int)fontStyle matrix:(const float *)fontMatrix;
+ _clearDocFontsUsed;
+ _clearSheetFontsUsed;
+ _clearPageFontsUsed;
+ _writeDocFontsUsed;
+ _writePageFontsUsed;
- _commonFontInit;
- _read:(int)mask for:(const char *) fontName size:(float)thisSize;
- _mapFont:(const char *)fontName size:(float)fontSize style:(int)fontStyle newName:(char *)mappedName;

@end

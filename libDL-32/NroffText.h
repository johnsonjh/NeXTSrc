#import <appkit/Text.h>
#import <appkit/Font.h>

@interface NroffText :Text
{
    BOOL noWrap;	/* don't wrap around when looking for stuff */
    BOOL doAlert;
    id target;
    char *currFile;
    int copyrightNeeded;
}

- setWrapAround:(BOOL)flag;
- setDoAlert:(BOOL)flag;
- setMargins;
- resetMargins;
- setTarget:(id) theTarget;
- setResizeableText;
- sizeTo:(NXCoord) x :(NXCoord) y;
- superviewSizeChanged:(NXSize *) oldSize;
- setFonts:(id) plain :(id) bold;
- rectInView:(NXRect *) theRect;
- bringSelToTop;
- bringSelToBottom;
- (int)findText:(const char *)s forward:(BOOL) findFwd;
- paste:sender;
- resignFirstResponder;
- (NXRunArray *)getTheRuns;
- setTheRuns;
- setText:(char const *)s;
- setSelEnd;
- (char *)getSelString:(char *)s maxLen:(int)len;
- save:sender;
- mouseDown:(NXEvent *) e;
- setParaStyle;
- setDfltParaStyle:(int)fontNum;
- setCurrFile:(char *)s;
- setCopyrightNeeded:(int)afterPrintedpages;
- (int)copyrightNeeded;
- setFontChangeable:(BOOL)yes;
- setCustomLineFuncs: (BOOL) yes;
@end

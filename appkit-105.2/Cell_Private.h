#import "Cell.h"

@interface Cell(Private)

- _dontFreeText;
- _useUserKeyEquivalent;
- (BOOL)_usesUserKeyEquivalent;
- _setView:view;
- _setMouseDownFlags:(int)flags;
- _selectOrEdit:(const NXRect *) aRect inView:controlView target:anObject editor:textObj event:(NXEvent *)theEvent start:(int)selStart end:(int)selEnd;
- _setCentered:(BOOL)flag;

@end

#import "Matrix.h"

@interface Matrix(Private)

- _clearSelectedCell;
- _setSelectedCell:aCell;
- _drawCellAt:(int)row :(int)col insideOnly:(BOOL)insideOnly;
- _highlightCellAt:(int)row :(int)col lit:(BOOL)flag andDraw:(BOOL)draw;
- _scrollRowToCenter:(int)row;
- (BOOL)_mouseHit :(NXPoint *) basePoint row:(int *)row col:(int *)col;
- (BOOL)_loopHit :(NXPoint *) basePoint row:(int *)row col:(int *)col;
- (BOOL)_radioHit :(NXPoint *) basePoint row:(int *)row col:(int *)col;
- _findFirstOne:(int *)row :(int *)col;
- _turnOffAllExcept:(int)row :(int)col andDraw:(BOOL)flag;
- _makeEditable:theCell :(int)theRow :(int)theCol:(NXEvent *)theEvent;
- _mouseDown_listMode:(NXEvent *) theEvent;
- _mouseDown_normalMode:(NXEvent *) theEvent;
- _selectRange:(BOOL)lit :(int)x :(int)y :(int)ref :(BOOL)includeX;
- _shiftDown:(NXEvent *) theEvent:theCell:(int)theRow:(int)theCol;
- _alternateDown:(NXEvent *) theEvent:theCell:(int)theRow:(int)theCol;
- _normalDown:(NXEvent *) theEvent:theCell:(int)theRow:(int)theCol;
- _selectTextOfCell:aCell;
- _selectTextOfNextCell;
- _selectTextOfPreviousCell;
- _getDrawingRow:(int *)row andCol:(int *)col;
- _sendDoubleActionToCellAt:(NXPoint *)point;
+ (BOOL)_isKitCellClass:cellClass;

@end

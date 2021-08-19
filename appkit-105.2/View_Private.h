#import "View.h"

@interface View(Private)

- _setNoVerticalAutosizing:(BOOL)flag;
- (BOOL)_canOptimizeDrawing;
- (BOOL)_optimizedRectFill:(const NXRect *)rect gray:(float)gray;
- (BOOL)_setOptimizedXYShowFont:font gray:(float)gray;
- (BOOL)_optimizedXYShow:(const char *)text numChars:(int)numChars at:(NXCoord)x :(NXCoord)y;
- freeAndUnlink;
- _commonAwake;
- _setWindow:theWindow;
- _setSuperview:sView;
- _addSubview:aView;
- _removeSubview:aView;
- _setRotatedFromBase:(BOOL)f;
- _setRotatedOrScaledFromBase:(BOOL)f;
- _alreadyFlipped:(BOOL)state;
- _drawMatrix;
- _computeBounds;
- (BOOL)_convertPointFromSuperview :(NXPoint *)aPoint test:(BOOL)testFlag;
- (BOOL)_convertRectFromSuperview :(NXRect *)aRect test:(BOOL)testFlag;
- _convertPoint :(NXPoint *)aPoint fromView:aView;
- _convertPoint :(NXPoint *)aPoint toView:aView;
- _centerScanPoint:(NXPoint *)aPoint;
- _crackRect:(NXRect *)aRect;
- _crackPoint:(NXPoint *)aPoint;
- _setBorderViewGState:(int)gs;
- _clipToFrame:(const NXRect *)frameRect;
- _focusDown:(BOOL)clipNeeded;
- _mark:(BOOL)flag;
- _invalidateGStates;
- _invalidateFocus;
- _invalidateSubviewsFocus;
- _fixInvalidatedFocus;
- _removeFreedViewFromFocusStack;
- (int)_clip :(const NXRect *)superClips :(int)superCount :(NXRect *)selfClips;
- _display:(const NXRect *)rects :(int)rectCount;
- _scrollPoint:(const NXPoint *)aPoint fromView:aView;
- _scrollRectToVisible:(const NXRect *)aRect fromView:aView;
- _realCopyPSCodeInside:(const NXRect *)rect to:(NXStream *)stream helpedBy:target;
- _realPrintPSCode:sender helpedBy:target doPanel:(BOOL)panelFlag forFax:(BOOL) faxFlag;
- _printPages:(int)firstLabel :(int)lastLabel printInfo:pInfo helpedBy:target;
- _printAndPaginate:(int)firstLabel printInfo:pInfo helpedBy:target;
- (void)_calcMarginSize:(NXSize *)size printInfo:pInfo;
- (void)_calcWidths:(float **)defaultSpace num:(int *)numWidths margin:(const NXSize *)marginSize printInfo:pInfo helpedBy:target;
- (void)_calcHeights:(float **)defaultSpace num:(int *)numHeights margin:(const NXSize *)marginSize printInfo:pInfo helpedBy:target;
- (void)_doPageArea:(const NXRect *)localRect finishPage:(BOOL)finishFlag helpedBy:target pageLabel:(int)label;
- _generateFaxCoverSheet;
- _resetCursorRects:(BOOL)flag;
- (BOOL)_requiresDisplayOnScreenChange;

@end

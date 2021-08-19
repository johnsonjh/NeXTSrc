#import "Window.h"

@interface Window(Private)

- _initContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag contentView:aView;
- _initContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag screen:(const NXScreen *)screen contentView:aView;
- (BOOL)_canOptimizeDrawing;
- _commonWindowInit:(BOOL) displayBorder;
- _viewDetaching:aView;
- _viewFreeing:aView;
- _commonAwake;
- _resetDisableCounts;
- _resizeWindow:(const NXRect *)frameRect userAction:(BOOL)userFlag;
- _borderView;
- _lastLeftHit;
- _lastRightHit;
- _displayTitle;
- _dosetTitle:(const char *)aString;
- _makeMiniView;
- _setCounterpart:theCounterpart;
- _getCounterpart;
- _justOrderOut;
- _doOrderWindow:(int)place relativeTo:(int)otherWin findKey:(BOOL)doKeyCalc level:(int)level;
- _doOrderWindow:(int)place relativeTo:(int)otherWin findKey:(BOOL)doKeyCalc level:(int)level forcounter:(BOOL)aCounter;
- _setWindowLevel:(int)newLevel;
- _setWindowLevelToLevelOf:(Window *)otherWindow;
- _tempHide:(BOOL)hideIt relWin:(int)otherWin;
- (BOOL)_visibleAndCanBecomeKey;
- _wsmIconInitFor : (int) globalWindow;
- (BOOL)_wsmOwnsWindow;
- _remotePrintWindow:(NXEvent *)theEvent;
- _remoteCopyWindow:(NXEvent *)theEvent;
- _realSmartPrintPSCode:sender doPanel:(BOOL)panelFlag forFax:(BOOL)faxFlag;
- _discardCursorRectsForView:aView;
- (BOOL)_wantsToDestroyRealWindow;
- _destroyRealWindow:(BOOL)orderingOut;
- _adjustDynamicDepthLimit;
- (BOOL)_isDocWindow;
- (BOOL)_colorSpecified;
- (BOOL)_sendColor;
- (BOOL)_hasCursorRects;

@end

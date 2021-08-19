#import "ClipView.h"

// Define to explicitly allocate gstates for ClipView & Scrollers.
// #define ALLOCGSTATESINSCROLLVIEWS

@interface ClipView(Private)

- _markUsedByCell;
- (BOOL)_isUsedByCell;
- _selfBoundsChanged;
- _pinDocRect;
- _setDocViewFromRead:aView;
- _alignCoords;
- _scrollPoint:(const NXPoint *)aPoint fromView:aView;
- _scrollRectToVisible:(const NXRect *)aRect fromView:aView;
- _scrollTo:(const NXPoint *) newOrigin;

@end

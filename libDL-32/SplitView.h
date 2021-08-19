#import <appkit/View.h>

@interface SplitView : View {
	id views;
	int nviews;
	BOOL valid;
}

+ newFrame:(NXRect const *)r ;
- addFixedView:view divider:(BOOL)divider ;
- addView:view divider:(BOOL)divider ;
- addView:view
  height:(float)height
  divider:(BOOL)divider ;

- addView:newView
    height:(NXCoord)height
    scroll:(BOOL)scroll
    divider:(BOOL)divider ;

- addView:view
    height:(NXCoord)height
    scroll:(BOOL)scroll
    bezel:(BOOL)bezel
    divider:(BOOL)divider
    at:(int)offset ;

- (int)divider:view ;
- setDivider:view:(BOOL)flag ;
- (float)minHeight:view ;
- setMinHeight:view:(float)y ;
- (int)stretchable:view ;
- setStretchable:view:(BOOL)flag ;
- scrollview:view ;
- sizeTo:(NXCoord)x :(NXCoord)y ;
- superviewSizeChanged:(NXSize *)old ;
- drawSelf:(NXRect *)rects :(int)rectCount ;
- mouseDown:(NXEvent *)e ;
- refreshText:(id)textView;
extern void fixScrolls(id view, id splitView);
@end

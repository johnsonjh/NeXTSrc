#import <appkit/appkit.h>
#import "BezelView.h"

@implementation  BezelView 

+ new:(float)theGap {
	self = [super new];
	gap = theGap - 4;
	return self;
}

- selfDisplay: (NXRect *) r : (int) n {
}

- drawSelf:(NXRect *)clips :(int)clipCount {
    NXRect r;
    r = bounds;
    PSsetgray(NX_LTGRAY);
    NXRectFill(&bounds);
    r.origin.x += gap;
    r.origin.y += gap;
    r.size.width -= 2*gap;
    r.size.height -= 2*gap;
    NXDrawWhiteBezel(&r, &r);
}



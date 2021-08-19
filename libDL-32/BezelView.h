#import <appkit/View.h>

@interface  BezelView :View {
    float gap;
}

+ new:(float)theGap;
- selfDisplay: (NXRect *) r : (int) n ;
- drawSelf:(NXRect *)clips :(int)clipCount ;
@end

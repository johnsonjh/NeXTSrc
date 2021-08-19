#import "color.h"
#import "NXBrowserCell.h"

@interface NXColorCell:NXBrowserCell
{
    NXColor swatchcolor;
    int colortype;
}

- setSwatchColor:(NXColor)col;
- (BOOL)isLeaf;
- (NXColor)swatchColor;
- setColorType:(int)atype;
- (int)colorType;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- drawInside:(const NXRect *)cellFrame inView:controlView;

@end


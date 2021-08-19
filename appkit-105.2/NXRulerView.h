#import "Text.h"

@interface NXRulerView:View
{
@public
    NXCoord firstIndent, leftIndent, rightIndent, leftMargin, rightMargin;
    NXCoord startX, endX;
    id font, text;
    id tokenlist;
    NXCoord descender;
    NXTextStyle *lastStyle;
}
- initFrame:(NXRect *)aRect;
- setFont:newFont;
- removeToken:token;
- setRulerStyle:(NXTextStyle *)style for:text;
- (NXCoord) height;
- setTextObject:text;
- (NXCoord) textX:(NXCoord) x;
- (NXCoord) convertX:(NXCoord) x;
@end










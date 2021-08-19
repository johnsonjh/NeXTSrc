#import "ButtonCell.h"

@interface ButtonCell(Private)

- _convertToText:(const char *)aString;
- _convertToText;
- _convertToIcon;
- _getIconInfo:(NXSize *)iconSize;
- _getTitleSize:(NXSize *) sizePtr inRect:(NXRect *) inBounds;
- _getIconRect:(NXRect *) theRect andTitleRect:(NXRect *) tRect flipped:(BOOL)flipped;
- _getIconRect:(NXRect *) theRect flipped:(BOOL)flipped;
- _compositeIcon:theIcon inRect:(const NXRect *)iRect inView:controlView;
- _startSound;
- _stopSound;

@end

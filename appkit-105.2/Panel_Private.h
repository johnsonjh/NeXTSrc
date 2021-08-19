#import "Panel.h"

@interface Panel(Private)

- _setTempHidden:(BOOL)aFlag;
- _doSetAccessoryView:new topView:top bottomView:bottom oldView:(id *)old;
- _replaceAccessoryView:curView with:newView topView:top bottomView:bottom;
+ _writeFrame:(const NXRect *)theFrame toDefaults:(const char *)panelName domain:(const char *)domain;
+ (BOOL)_readFrame:(NXRect *)theFrame fromDefaults:(const char *)panelName domain:(const char *)domain;

@end

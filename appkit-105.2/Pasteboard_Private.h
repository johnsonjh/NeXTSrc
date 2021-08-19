#import "Pasteboard.h"

@interface Pasteboard(Private)

+ _newName:(const char *)name host:(const char *)host;
+ (void)_provideAllPromisedData;
- (void)_providePromisedData;

@end

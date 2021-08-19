#import "NXImageRep.h"
#import "NXBitmapImageRep.h"
#import "NXEPSImageRep.h"
#import "NXCachedImageRep.h"
#import "NXCustomImageRep.h"
#import "tiff.h"
#import "tiffPrivate.h"

/* Private methods of subclasses of NXImageRep. */

@interface NXImageRep(Private)
- (BOOL)_canDrawOutsideOfItsBounds;
@end

@interface NXBitmapImageRep(Private)
- (BOOL)_loadData;
- (BOOL)_loadFromTIFF:(TIFF *)tiff imageNumber:(int)num;
- _initFrom:(const char *)fileName from:(int)dataSource imageNumber:(int)num; 
- _initFrom:(const char *)fileName from:(int)dataSource imageNumber:(int)num tiff:(TIFF *)tiff;
- _initFromTIFF:(TIFF *)tiff imageNumber:(int)num;
+ _newListFromStream:(NXStream *)stream fileName:(const char *)fileName from:(int)dataSource zone:(NXZone *)zone;
+ (List *)_newListFromIcon:(const char *)icon inApp:(const char *)app zone:(NXZone *)zone;
- _forgetData;
- _getFromTIFF:(TIFF *)tiff;
- (const char *)_appName;
- (const char *)_fileName;
@end

@interface NXEPSImageRep(Private)
+ (List *)_newListFromIcon:(const char *)icon inApp:(const char *)app zone:(NXZone *)zone;
- (BOOL)_loadFromStream:(NXStream *)stream;
- (BOOL)_loadData;
- _forgetData;
- (const char *)_appName;
- (const char *)_fileName;
@end

@interface NXCachedImageRep(Private)
- _getCacheGState:(int *)gs andRect:(NXRect *)rect;
- _getCacheWindow:(Window **)win andRect:(NXRect *)rect;
@end


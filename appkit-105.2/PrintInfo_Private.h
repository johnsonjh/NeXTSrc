#import "PrintInfo.h"

@interface PrintInfo(Private)

- _setCurrentPage:(int)anInt;
- _setPrivateData:(NXPrivatePrintInfo)data;
-(NXPrivatePrintInfo) _privateData;
- _updatePrinterInfo;
- _updateFaxInfo;
@end

BOOL _NXIsFaxType(const char *printerType);

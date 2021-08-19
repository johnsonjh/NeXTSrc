#import "PrintPanel.h"
#import "ChoosePrinter.h"

@interface PrintPanel(Private)

- _setControlsEnabled:(BOOL)flag;
- _updatePrintStat:(const char *)statusString label:(const char *)label;
- _copyFromFaxPanelPageMode:pageModeFax firstPage : firstPageFax lastPage: lastPageFax;

@end

/* indices into colCell data */

#define NAME		0
#define TYPE		1
#define HOST		2
#define PORT		3
#define NOTE		4
#define NUM_COLS	5

@interface ChoosePrinter(Private)
+ _buildPrinterMatrix:(const NXRect *)aFrame tabs:(const float *)tabs zone:(NXZone *)zone;
+ (void)_loadPrinterList:matrix isPrinter:(BOOL)printerFlag orderIt:(BOOL)orderIt;
+ _writePrintInfo:(BOOL)printerFlag lastValues:(const char **)lastValues cell:cell;
@end

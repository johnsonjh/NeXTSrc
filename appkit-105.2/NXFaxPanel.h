/*

	NXFaxPanel.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Chris Franklin
	  
	DEFINED IN: The Application Kit
	HEADER FILES: NXFaxPanel.h
*/

#import "Panel.h"
#import <sys/stat.h>

#define _NX_FAXCANCEL -1
#define _NX_FAXNOCOVER 0
#define _NX_FAXWITHCOVER 1
#define _NX_FAXPREVIEWNOCOVER 2
#define _NX_FAXPREVIEWWITHCOVER 3

/*  default values used for managing background eps, phone list and rtf form */

#define _NX_LIBRARYPATH "~/Library:/LocalLibrary:/NextLibrary"


/* default names for ditto */

#define NX_LIBRARYPATH "LibraryPath"
#define NX_FAXORIGINS "FaxOrigins"
#define NX_FAXWANTSCOVER "FaxWantsCover"
#define NX_FAXWANTSNOTIFY "FaxWantsNotify"
#define NX_FAXWANTSHIRES "FaxWantsHires"

@interface NXFaxPanel:Panel
{
    id	faxIcon;
    id  faxForm;
    id	faxNumber;
    id  faxName;
    id  addButton;
    id  deleteButton;
    id  replaceButton;
    id  faxOkButton;
    id  coverCheckBox;
    id  notifyCheckBox;
    id  hiresCheckBox;
    id	phoneList;
    id	coverScroller;
    id	coverIcon;
    id	coverWindow;
    id  paperView;
    id  coverText;
    id  paperBox;
    id  numbersMatrix;
    id  firstPage;
    id  lastPage;
    id  name;
    id  status;
    id  pageMode;
    id  modemButton;
    id  preview;
    char faxExit;
    char nameIsEmpty;
    char numberIsEmpty;
    NXRect paperViewMax;
    
    NXStream *streamPS;
    time_t mtimePS;
    dev_t devPS;
    ino_t inoPS;
    char *dataPS;
    NXRect boundsPS;
    float  bboxPS[4];
    int    lengthPS;
    
    int numbersChanged;
    time_t numbersMtime;
    dev_t numbersDev;
    ino_t numbersIno;
    
    id focusWindow;
    int _unusedInt1;
}


/* methods used externally */
+ new;
+ allocFromZone:(NXZone *)zone;
+ alloc;
- (int) runModal;
- writeCoverSheet;
-  writeFaxNumberList: (char *) format;
- writeFaxToList: (char *) format;
- (BOOL) notifyIsChecked;
- (BOOL) hiresIsChecked;
- coverBBox : (NXRect *) boundingBox;
- setFaxIcon:anObject;
- setFaxForm:anObject;
- setFaxNumber:anObject;
- setFaxName:anObject;
- setAddButton:anObject;
- setDeleteButton:anObject;
- setReplaceButton:anObject;
- setFaxOkButton:anObject;
- setCoverCheckBox:anObject;
- setNotifyCheckBox:anObject;
- setHiresCheckBox:anObject;
- setPhoneList:anObject;
- setCoverScroller:anObject;
- setCoverIcon:anObject;
- setCoverWindow:anObject;
- setFirstPage:anObject;
- setLastPage:anObject;
- setName:anObject;
- setPageMode:anObject;
- setStatus:anObject;
- setModemButton:anObject;
- setPreview:anObject;
- coverSheet:sender;
- cancelFax:sender;
- okCover:sender;
- deleteNumber:sender;
- addNumber:sender;
- numberClicked:sender;
- cancelCover:sender;
- okFax:sender;
- modemHit:sender;
- previewHit:sender;
- pageModeHit:sender;
- enableButtonsIsMultipleSelection: (BOOL) flag;
- (BOOL)readPSFromStream:(NXStream *)stream;
- textDidGetKeys:textObject isEmpty:(BOOL)flag;
- textDidResize:textObject oldBounds:(const NXRect *)oldBounds    invalid:(NXRect *)invalidRect;
- _setControlsEnabled:(BOOL)flag;
@end



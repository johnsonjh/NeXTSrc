#import <objc/Object.h>

#define MAXCOLORLISTS 20

#define RGBATYPE 0
#define CMYKTYPE 1
#define HSBATYPE 2

#define NUMDEFAULTS 10

typedef struct {
     char *name;
     int type;
     NXColor color;
} NXCustomColor;	
     
@interface NXColorCustom : Object
{
    int currentlist, numlists;
    id saveAlert, alertField;
    id customPopUp, customButton;
    id optionButton, optionPopUp;
    id colorPicker, colorBrowser;
    NXCustomColor defaultColors[NUMDEFAULTS];
    char *cfname[MAXCOLORLISTS];
}

+ new;
- init;
- (int)browser:sender fillMatrix:matrix inColumn:(int)column;
- newColorFile;
- doSave:sender;
- openColorFile;
- (BOOL)panelValidateFilename:sender;
- (int)addColorList:matrix named:(char *)fname;
- writeList:(char *)fname fromMatrix:matrix;
- seekWord:(char *)word;
- findCustomColors:sender;
- removeColorList;
- addColor;
- doNew:sender;
- removeColor;
- setCustomColor:sender;
- setCustomButton:anObject;
- setCurrentCustom:sender;
- setColorBrowser:anObject;
- doCustomOption:sender;
- updateCustomColorList;
- setOptionButton:anObject;

@end

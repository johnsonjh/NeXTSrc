#import "View.h"

#define MAXPALETTES 20

@interface NXColorPalette : View
{
	id palette[MAXPALETTES],bitmap,position;
	int currentpalette,numpalettes,numuntitled,placeimage;
	id paletteButton,palettePopUp;
	id optionButton,optionPopUp;
	id alertField,saveAlert,colorPicker;
	//put instance variables concerning state of selection
}

extern void _NXReadPixels(int *r,int *g,int *b,int *a,float srcX,float srcY);

+ newFrame:(NXRect const *)theFrame;
- initFrame:(NXRect const *)theFrame;
- newPalette;
- openPalette;
- findCustomPalettes:sender;
- pasteTIFF;
- copyTIFF;
- removePalette;
- savePalette;
- doSave:sender;
- addTiff:(const char *)filen:(int)home:(int)place;
- setCurrentPalette:sender;
- doPaletteOption:sender;
- (BOOL) acceptsFirstMouse;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- mouseDown:(NXEvent *)theEvent;
- setOptionButton:anObject;
- setPaletteButton:anObject;

@end

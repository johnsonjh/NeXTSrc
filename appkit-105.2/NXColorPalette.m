/*
	NXColorPalette.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "colorPickerPrivate.h"
#import "NXColorPalette.h"
#import "NXColorPicker.h"
#import "Application.h"
#import "Bitmap.h"
#import "NXImage.h"
#import "Button.h"
#import "Form.h"
#import "OpenPanel.h"
#import "SavePanel.h"
#import "PopUpList.h"
#import "Pasteboard.h"
#import "Window.h"
#import "tiff.h"
#import <dpsclient/wraps.h>
#import <objc/hashtable.h>
#import <libc.h>
#import <sys/dir.h>
#import <fcntl.h>
#import <math.h>
#import <mach.h>

@interface Bitmap(InternalUseOnly)
+ findBitmapFor:(const char *)name;
- setFlip:(BOOL)flag;
@end

/*
 * Palette files and their names; these are searched in 
 * the current appkit files directory.
 */

#define DEFAULTPALETTEFILE1 KitString(ColorPanel,"Spectrum.tiff","default palette 1 file name")
#define DEFAULTPALETTEFILE2 KitString(ColorPanel,"Default.tiff","default palette 2 file name")
#define DEFAULTPALETTE1 KitString(ColorPanel,"Spectrum","default palette 1 name")
#define DEFAULTPALETTE2 KitString(ColorPanel,"Default","default palette 2 name")

#define ALERT_ICON_TAG 123

#define NEW_TAG 1
#define OPEN_TAG 2
#define COPY_TAG 3
#define PASTE_TAG 4
#define REMOVE_TAG 5
#define SAVE_TAG 6

#define MAXPALETTEWIDTH 297.0
#define MAXPALETTEHEIGHT 75.0
#define INTRND(x) ((int)((((x)-floor(x))<=.5)?floor(x):ceil(x)))
#define COPY_STRIP_DIR_AND_EXT(A,B) (*(strrchr(strcpy((A),strrchr((B), '/')+1), '.')) = '\0')
#define COPY_STRIP_DIR(A,B) (strcpy((A),strrchr((B), '/')+1))
#define STRIP_EXT(A) (*(strrchr((A), '.')) = '\0')

#define S_WIDTH 11
#define S_HEIGHT 8

#define IMAGESIZE(w,h,b,s) (((int)(s)) * (((int)(h))*((((int)(w))*((int)b)+7)/8)))
	
extern int alphasort(struct direct **d1, struct direct **d2);

@implementation  NXColorPalette:View

+ newFrame:(NXRect const *)theFrame
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:theFrame];
}

- initFrame:(NXRect const *)theFrame
{
    [super initFrame:theFrame];
    currentpalette = -1;
    numuntitled = 1;
    return self;
}

static int isTIFF(struct direct *dentry)
{
    char *period = strrchr(dentry->d_name,'.');
    return (period && !strcmp(period, ".tiff"));
}

- newPalette
{
    id cell;
    NXSize sz;
    char newpal[256];
    
    sprintf(newpal, "Untitled%d", numuntitled);
    numuntitled++;
    [self addTiff:NULL:1:0];  
    [palette[currentpalette] lockFocus];
    [palette[currentpalette] getSize:&sz];
    PSsetgray(1.0);
    PSrectfill(0.0,0.0,sz.width,sz.height);
    [palette[currentpalette] unlockFocus];
    cell = [palettePopUp addItem:newpal];
    [paletteButton setTitle:newpal];
    [[palettePopUp itemList] selectCell:cell];
    [self setCurrentPalette:[palettePopUp itemList]];
    
    return self;
}

- openPalette
{
    id op;
    char *opdir;
    const char *fileName;
    const char *fileTypes[] = {"tiff",NULL};
    char newpal[1024];
    
    op = [OpenPanel new];
    opdir = NXCopyStringBufferFromZone([op directory], NXDefaultMallocZone());
    if ([op runModalForTypes:fileTypes] && (fileName = [op filename])) {
	[self addTiff:fileName:0:1];
	COPY_STRIP_DIR_AND_EXT(newpal, fileName);
	[palettePopUp addItem:newpal];
	[paletteButton setTitle:newpal];
    }
    [op setDirectory:opdir];
    free(opdir);
    
    return self;
}

- findCustomPalettes:sender
{
    int numentries, a;
    struct direct **dirp;
    char dirname[MAXPATHLEN+1];
    
    strcpy(dirname, NXHomeDirectory());
    strcat(dirname, "/.NeXT/Colors");
    colorPicker = sender;
    optionPopUp = [[PopUpList allocFromZone:[self zone]] init];
    [[optionPopUp addItem:KitString(ColorPanel,"New","command for a new palette")] setTag:NEW_TAG];
    [[optionPopUp addItem:KitString(ColorPanel,"Open...","command to open a palette")] setTag:OPEN_TAG];
    [[optionPopUp addItem:KitString(ColorPanel,"Copy","command to copy a palette")] setTag:COPY_TAG];
    [[[optionPopUp addItem:KitString(ColorPanel,"Paste","command to paste a palette")] setEnabled:NO] setTag:PASTE_TAG];
    [[[optionPopUp addItem:KitString(ColorPanel,"Remove","command to remove a palette")] setEnabled:NO] setTag:REMOVE_TAG];
    [[[optionPopUp addItem:KitString(ColorPanel,"Save...","command to save a palette")] setEnabled:NO] setTag:SAVE_TAG];
    NXAttachPopUpList(optionButton, optionPopUp);
    [optionPopUp changeButtonTitle:NO];
    [optionButton setIcon:"pulldown"];
    [optionButton setTitle:KitString(ColorPanel,"Options","title for palette options pop up")];
    
    numentries = scandir(dirname, &dirp, &isTIFF, alphasort);
    palettePopUp = [[PopUpList allocFromZone:[self zone]] init];

    /* FIRST ADD THE TWO DEFAULT PALETTES IN /usr/lib/NextStep */

    [paletteButton setTitle:DEFAULTPALETTE1];
    NXAttachPopUpList(paletteButton,palettePopUp);
    [palettePopUp addItem:DEFAULTPALETTE1];
    strcpy(dirname, _NXAppKitFilesPath);
    strcat(dirname, DEFAULTPALETTEFILE1);
    [self addTiff:dirname:0:0];
#ifdef USEDEFAULTPALETTE2
    [palettePopUp addItem:DEFAULTPALETTE2];
    strcpy(dirname, _NXAppKitFilesPath);
    strcat(dirname, DEFAULTPALETTEFILE2);
    [self addTiff:dirname:0:0];
#endif

    if (numentries != -1){
	for(a = 0; a < numentries; a++) {
	    if (a <= MAXPALETTES) {
		[self addTiff:dirp[a]->d_name:1:0];
		STRIP_EXT(dirp[a]->d_name);
		[palettePopUp addItem:dirp[a]->d_name];
	    }
	    free(dirp[a]);
	}
	free(dirp);
    }

    currentpalette = 0;

    [[palettePopUp itemList] sizeToFit];	/* necessary? */
    [palettePopUp display];
    
    return self;
}

static BOOL tiffPasteType(const char* const *types)
{
    char *const *s = types;
    if (s) while (*s) {
	if (!strcmp(*s, NXTIFFPboardType)) return YES;
	s++;
    }
    return NO;
}

- pasteTIFF
{
    NXSize psz;
    int length = 0;
    char *data = NULL;
    NXStream *stream = NULL;
    NXPoint ppt1, pt = {0.0,0.0};
    id pb = [Pasteboard new];

    if (currentpalette < 1) {
	_NXKitAlert("ColorPanel","System Palette", "You can not paste in the System Palettes.", NULL, NULL, NULL);
        return self;
    }

    if (!tiffPasteType([pb types])) return self; 
   
    if ([pb readType:NXTIFFPboardType data:&data length:&length] && data && length && (stream = NXOpenMemory(data, length, NX_READONLY))) {
	bitmap = [NXImage newFromStream:stream];
	[bitmap setFlipped:NO];
	NXCloseMemory(stream, NX_FREEBUFFER);
	placeimage = 1;
	[self lockFocus];
	[position getSize:&psz];
	ppt1.x = floor(frame.size.width / 2.0) - floor(psz.width / 2.0);
	ppt1.y = floor(frame.size.height / 2.0) - floor(psz.height / 2.0);
	[bitmap composite:NX_COPY toPoint:&pt];
	[position composite:NX_SOVER toPoint:&ppt1];
	[window flushWindow];
	[self unlockFocus];
    }
    if (data && length) {
	vm_deallocate (task_self(), (vm_address_t)data, (vm_size_t)length); 	
    }

    return self;
}

static void NewViewToArray(
    unsigned char **redptr,float srcX,float srcY,
    float srcW,float srcH,int *wi,int *he,int *bp,int *sp,int *pc)
{ 
    int pi,size,mask;
    unsigned char *rptr=0,*gptr=0,*bptr=0,*aptr=0;
    NXRect imagebounds;

    NXSetRect(&imagebounds, 0.0, 0.0, srcW, srcH);
    PSgsave();
    PStranslate(srcX, srcY);
    NXSizeBitmap(&imagebounds, &size, wi, he, bp, sp, pc, &pi);
    rptr  = (unsigned char *)NXZoneMalloc(NXDefaultMallocZone(), size);
   
    switch (*sp) {
       case 1:
	   mask = NX_MONOTONICMASK;
	   break;
       case 2:
	   gptr =  rptr + (size / (*sp));
	   mask = NX_ALPHAMASK | NX_MONOTONICMASK;
	   break;
       case 3:
           if(*pc == NX_PLANAR) {
		gptr =  rptr + (size / (*sp)) * 1;
		bptr =  rptr + (size / (*sp)) * 2;
	   }
	   mask =  NX_COLORMASK;
	   break;
       case 4:
           if(*pc == NX_PLANAR) {
	       gptr =  rptr + (size / (*sp)) * 1;
	       bptr =  rptr + (size / (*sp)) * 2;
	       aptr =  rptr + (size / (*sp)) * 3;
	   }
	   mask = NX_ALPHAMASK | NX_COLORMASK;
	   break;
    }
    NXReadBitmap(&imagebounds,*wi,*he,*bp,*sp,*pc,pi,
    			rptr,gptr,bptr,aptr,NULL);
    PSgrestore();
    *redptr=rptr;
}

- copyTIFF
{
    char *data;
    unsigned char *idata;
    NXSize sz;
    NXImageInfo aimage;
    NXStream* stream;
    const char* types[1];
    int length, maxlen,wi,he,bp,sp,pc;
    
    types[0] = NXTIFFPboardType;
    [palette[currentpalette] getSize:&sz];
    aimage.width=sz.width;
    aimage.height=sz.height;
    [palette[currentpalette] lockFocus];
    NewViewToArray(&idata, 0.0, 0.0, sz.width, sz.height, &wi, &he, &bp, &sp, &pc);
    [palette[currentpalette] unlockFocus];
    [[Pasteboard new] declareTypes:types num:1 owner:self];
    aimage.bitsPerSample = bp;
    aimage.samplesPerPixel = sp;
    aimage.photoInterp = ((sp > 3) ? NX_COLORMASK : NX_MONOTONICMASK) | ((sp == 2 || sp == 4) ? NX_ALPHAMASK : 0);
    aimage.planarConfig = pc;
    stream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
    NXWriteTIFF(stream, &aimage, idata);
    NXGetMemoryBuffer(stream, &data, &length, &maxlen);
    NXFlush(stream);
    [[Pasteboard new] writeType:NXTIFFPboardType data:data length:length];
    NXCloseMemory(stream, NX_FREEBUFFER);
     
    return self;
}

- removePalette
{
    int x;
    NXPoint pt = {0.0,0.0};
    char removename[MAXPATHLEN+1],removefile[MAXPATHLEN+1];
    
    if(currentpalette < 1) {
	_NXKitAlert("ColorPanel","System Palette", "You can not remove the System Palettes.", NULL, NULL, NULL);
        return self;
    }
    if(numpalettes <= 1) return self;
    strcpy(removename, [paletteButton title]);
    if(!_NXKitAlert("ColorPanel","Remove Palette", "Are you sure you want to remove %s from your custom palettes?", "Remove", "Cancel", NULL, removename))
    	return self;
    strcpy(removefile, NXHomeDirectory());
    strcat(removefile, "/.NeXT/Colors/");
    strcat(removefile, removename);
    strcat(removefile, ".tiff");
    if(!access(removefile, W_OK|F_OK)){
	unlink(removefile);
    }
    for(x = currentpalette; x < MAXPALETTES; x++) {
	if(palette[x + 1] == nil)break;
	[palette[x] lockFocus];
	[palette[x+1] composite:1 toPoint:&pt];
	[palette[x] unlockFocus];
    }
    currentpalette--;
    numpalettes--;
    [palette[numpalettes] free];
    palette[numpalettes] = nil;
    if(currentpalette < 0) currentpalette = 0;
    [palettePopUp removeItem:removename];
    [[palettePopUp itemList] sizeToFit];
    [[palettePopUp itemList] selectCellAt:currentpalette:0];
    [palettePopUp display];
    [paletteButton setTitle:[palettePopUp selectedItem]]; 
    currentpalette = [palettePopUp indexOfItem:[palettePopUp selectedItem]];
    [self display];
    
    return self;
}

- savePalette
{
    BOOL ok;
    char *data;
    unsigned char *idata;
    NXSize sz;
    NXImageInfo aimage;
    NXStream* stream;
    int length, maxlen,wi,he,bp,sp,pc;
    char newfile[MAXPATHLEN+1],testname[MAXPATHLEN+1],curname[MAXPATHLEN+1];
    char oldname[MAXPATHLEN+1];
    
    if(currentpalette < 1) {
	_NXKitAlert("ColorPanel","System Palette", "You can not change the System Palettes. Create a new palette if you wish to customize one", NULL, NULL, NULL);
        return self;
    }
    if(saveAlert == nil) {
	LOADCOLORPICKERNIB("ColorPickerAlert", self);
	[[[saveAlert contentView] findViewWithTag:ALERT_ICON_TAG] setIcon:"app"];
    }
    strcpy(newfile, NXHomeDirectory());
    strcat(newfile, "/.NeXT/Colors/");
    strcpy(testname, newfile);
    strcpy(curname, [paletteButton title]);
    strcpy(oldname, [paletteButton title]);
    strcat(testname, curname);
    strcat(testname, ".tiff");
    if(access(testname, F_OK)) {
	[alertField setStringValue:curname at:0];
	[alertField selectTextAt:0];
	ok = ![NXApp runModalFor:saveAlert];
	[saveAlert orderOut:self];
	if (!ok) return self;
	strcat(newfile, [alertField stringValueAt:0]);
        strcpy(curname, [alertField stringValueAt:0]);
	strcat(newfile, ".tiff");
    } else {
	if(!_NXKitAlert("ColorPanel","Palette Exists", "Replace existing Palette?", "OK", "Cancel", NULL)) return self;
	strcpy(newfile, testname);
    }
    [paletteButton setTitle:curname];
    if (strcmp(curname, oldname)) {
	[palettePopUp insertItem:curname at:[palettePopUp indexOfItem:oldname]];
	[palettePopUp removeItem:oldname];
    }
    [[palettePopUp itemList] sizeToFit];
    [palettePopUp display];
    [palette[currentpalette] getSize:&sz];
    aimage.width = sz.width;
    aimage.height = sz.height;
    [palette[currentpalette] lockFocus];
    NewViewToArray(&idata, 0.0, 0.0, sz.width, sz.height, &wi, &he, &bp, &sp, &pc);
    [palette[currentpalette] unlockFocus];
    aimage.bitsPerSample = bp;
    aimage.samplesPerPixel = sp;
    aimage.photoInterp = ((sp > 3)? NX_COLORMASK : NX_MONOTONICMASK) | ((sp == 2 || sp == 4) ? NX_ALPHAMASK : 0);
    aimage.planarConfig = pc;
    stream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
    NXWriteTIFF(stream, &aimage, idata);
    NXGetMemoryBuffer(stream, &data, &length, &maxlen);
    NXFlush(stream);
    NXSaveToFile(stream, newfile);
    NXCloseMemory(stream, NX_FREEBUFFER);
    
    return self;
}

- doSave:sender
{
    char newfile[MAXPATHLEN+1];
    
    if([sender selectedTag] == 0) { 
       [NXApp stopModal:YES];
       return self;
    }
    strcpy(newfile, NXHomeDirectory());
    strcat(newfile, "/.NeXT/Colors/");
    strcat(newfile, [alertField stringValueAt:0]);
    strcat(newfile, ".tiff");
    if(!access(newfile, F_OK|W_OK)) {
	_NXKitAlert("ColorPanel","Palette Exists", "This Palette exists.  Try another name.", NULL, NULL, NULL);
	return self;
    }
    [NXApp stopModal:NO];
    
    return self;
}


- addTiff:(const char *)filen:(int)home:(int)place
{
    NXSize sz,psz,newSize;
    char dirname[MAXPATHLEN+1];
    NXPoint pt = {0.0,0.0},ppt1;
    
    if(home) {
	strcpy(dirname, NXHomeDirectory());
	strcat(dirname, "/.NeXT/Colors/");
    }
    currentpalette = numpalettes++;
    newSize.width = MAXPALETTEWIDTH;
    newSize.height = MAXPALETTEHEIGHT;
    
    palette[currentpalette] = [NXImage newSize:&newSize];
    [palette[currentpalette] setCacheDepthBounded:NO];
    [palette[currentpalette] setFlipped:NO];
    if(filen == NULL || (!strncmp(filen, "Untitled", 8))) {
        [palette[currentpalette] lockFocus];
        [palette[currentpalette] getSize:&sz];
        PSsetgray(1.0);
        PSrectfill(0.0, 0.0, sz.width, sz.height);
        [palette[currentpalette] unlockFocus];
        return self;
    }
    if(home) strcat(dirname, filen);
    else strcpy(dirname, filen);
    bitmap = [NXImage newFromFile:dirname];
    [bitmap setCacheDepthBounded:NO];
    [bitmap setFlipped:NO];
    position = [NXImage findImageNamed:"NXcolor_position"];
    [position setFlipped:NO];
    [palette[currentpalette] lockFocus];
    [palette[currentpalette] getSize:&sz];
    PSsetgray(1.0);
    PSrectfill(0.0, 0.0, sz.width, sz.height);
    if(!place) {
	[bitmap composite:NX_COPY toPoint:&pt];
	[palette[currentpalette] unlockFocus];
	[bitmap free];
	bitmap = nil;
	return self;
    }
    [palette[currentpalette] unlockFocus];
    [self lockFocus];
    PSsetgray(1.0);
    PSrectfill(0.0, 0.0, frame.size.width, frame.size.height);
    [position getSize:&psz];
    ppt1.x = floor(frame.size.width / 2.0) - floor(psz.width / 2.0);
    ppt1.y = floor(frame.size.height / 2.0) - floor(psz.height / 2.0);
    [bitmap composite:NX_COPY toPoint:&pt];
    [position composite:NX_SOVER toPoint:&ppt1];
    [window flushWindow];
    [self unlockFocus];
    placeimage = 1;
    
    return self;
}

- setCurrentPalette:sender
{
    NXPoint origin = {0.0, 0.0};
    
    if(bitmap != nil){
	[palette[currentpalette] lockFocus];
	[bitmap composite:NX_COPY toPoint:&origin];
	[palette[currentpalette] unlockFocus];
	[bitmap free];
	bitmap = nil;
	placeimage = 0;
    }
    currentpalette = [sender selectedRow];
    if(currentpalette < 1) {
       [[[optionPopUp itemList] findCellWithTag:PASTE_TAG] setEnabled:NO];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_TAG] setEnabled:NO];
       [[[optionPopUp itemList] findCellWithTag:SAVE_TAG] setEnabled:NO];
    } else {
       [[[optionPopUp itemList] findCellWithTag:PASTE_TAG] setEnabled:YES];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_TAG] setEnabled:YES];
       [[[optionPopUp itemList] findCellWithTag:SAVE_TAG] setEnabled:YES];
    }
    [self display];
    
    return self;
}

- doPaletteOption:sender
{
    NXPoint origin = {0.0, 0.0};
    
    if (bitmap != nil) {
	[palette[currentpalette] lockFocus];
	[bitmap composite:NX_COPY toPoint:&origin];
	[palette[currentpalette] unlockFocus];
	[bitmap free];
	bitmap = nil;
	placeimage = 0;
    }
    
    switch ([sender selectedRow]) {
	case 0:
	    break;
	case NEW_TAG:
	    [self newPalette];
	    break;
	case OPEN_TAG:
	    [self openPalette];
	    break;
	case COPY_TAG:
	    [self copyTIFF];
	    break;
	case PASTE_TAG:
	    [self pasteTIFF];
	    break;
	case REMOVE_TAG:
	    [self removePalette];
	    break;
	case SAVE_TAG:
	    [self savePalette];
	    break;
    }
    
    if(currentpalette < 1) {
       [[[optionPopUp itemList] findCellWithTag:PASTE_TAG] setEnabled:NO];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_TAG] setEnabled:NO];
       [[[optionPopUp itemList] findCellWithTag:SAVE_TAG] setEnabled:NO];
    } else {
       [[[optionPopUp itemList] findCellWithTag:PASTE_TAG] setEnabled:YES];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_TAG] setEnabled:YES];
       [[[optionPopUp itemList] findCellWithTag:SAVE_TAG] setEnabled:YES];
    }
    [[optionPopUp itemList] display];

    return self;
}

- (BOOL) acceptsFirstMouse
{
    return YES;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    NXPoint pt = {0.0,0.0};
    [palette[currentpalette] composite:NX_COPY toPoint:&pt];
    return self;
}

#define RND(x) (((x)>floor(x)+.5)?ceil(x):floor(x))

extern void _NXReadPixels(int *r,int *g,int *b,int *a,float srcX,float srcY)
{ 
    int pc,pi,size,wi,he,bp,sp,shift,amask=0;
    unsigned char *rptr,*gptr,*bptr,*aptr;
    NXRect imagebounds;
    
    NXSetRect(&imagebounds, 0.0, 0.0, 1.0, 1.0);
    PSgsave();
    PStranslate(srcX, srcY);
    NXSizeBitmap(&imagebounds, &size, &wi, &he, &bp, &sp, &pc, &pi);
    rptr  = (unsigned char *)NXZoneMalloc(NXDefaultMallocZone(), size);
    size /= sp;
    
    if(sp == 4){
        gptr = rptr + size;
	bptr = gptr + size;
	aptr = bptr + size;
	NXReadBitmap(&imagebounds, wi, he, bp, sp, pc, pi, rptr, gptr, bptr, aptr, NULL);
	shift = 8 - bp;
	amask = (1 << bp) - 1;
	*r = (rptr[0] >> shift) & amask;
	*g = (gptr[0] >> shift) & amask;
	*b = (bptr[0] >> shift) & amask;
	*a = (aptr[0] >> shift) & amask;
    } else if(sp == 3) {
        gptr = rptr + size;
	bptr = gptr + size;
	NXReadBitmap(&imagebounds, wi, he, bp, sp, pc, pi, rptr, gptr, bptr, NULL, NULL);
	shift = 8 - bp; 
	amask = (1 << bp) - 1;
	*r = (rptr[0] >> shift) & amask;
	*g = (gptr[0] >> shift) & amask;
	*b = (bptr[0] >> shift) & amask;
	*a = (1 << bp) - 1;
    } else if(sp == 2) {
        aptr = rptr + size;
	NXReadBitmap(&imagebounds, wi, he, bp, sp, pc, pi, rptr, aptr, NULL, NULL, NULL);
	shift = 8 - bp;
	amask = (1 << bp) - 1;
	*r = RND(((rptr[0] >> shift) & amask) * (256.0 / amask));
	*g = *r;
	*b = *r;
	*a = RND(((aptr[0] >> shift) & amask) * (256.0 / amask));
    } else if(sp == 1) {
	NXReadBitmap(&imagebounds, wi, he, bp, sp, pc, pi, rptr, NULL, NULL, NULL, NULL);
	shift = 8 - bp;
	amask = (1 << bp) - 1;
	*r = (rptr[0] >> shift) & amask;
	*g = *r;
	*b = *r;
	*a = (1 << bp) - 1;
    }
    if((*a) != 0) {
	*r = INTRND(amask * ( (float)(*r) / (*a) ));
	*g = INTRND(amask * ( (float)(*g) / (*a) ));
	*b = INTRND(amask * ( (float)(*b) / (*a) ));
    }
    if (bp == 2) {
	*r = *r * 85;
	*g = *g * 85;
	*b = *b * 85;
	*a = *a * 85;
    } else if (bp == 4){
	*r = *r * 17;
	*g = *g * 17;
	*b = *b * 17;
	*a = *a * 17;
    }
    if(rptr) free((char *)rptr);
    PSgrestore();
    
    return;		
}

- mouseDown:(NXEvent *)theEvent
{
    int r,g,b,a,x,y;
    NXRect fr;
    NXPoint mouseLocation;
    NXSize psz;
    NXColor newcolor, aColor;
    int oldMask = 0, loop=YES;
    BOOL validEvent;
    NXPoint pt = {0.0,0.0},pt0 = {0.0,0.0},ppt1,ppt2,mpt1,mpt2;

    if (((theEvent->flags & NX_ALTERNATEMASK) || placeimage) && (currentpalette < 1)) return self;

    mouseLocation = theEvent->location; 
    [self convertPoint:&mouseLocation fromView:nil];
    x = (int)mouseLocation.x;
    y = (int)mouseLocation.y;
    if (!NXPointInRect(&mouseLocation, &bounds)) return self;

    if (placeimage) {
	[position getSize:&psz];
	ppt1.x = floor(frame.size.width / 2.0) - floor(psz.width / 2.0);
	ppt1.y = floor(frame.size.height / 2.0) - floor(psz.height / 2.0);
	mpt1.x = mouseLocation.x;
	mpt1.y = mouseLocation.y;
    }

    oldMask = [window eventMask];
    [window addToEventMask:NX_MOUSEDRAGGEDMASK|NX_MOUSEUPMASK];

    while (loop) {
	switch (theEvent->type) {
	    case NX_MOUSEUP:
		loop = NO;
		if (placeimage) {
		    [palette[currentpalette] lockFocus];
		    [bitmap composite:NX_COPY toPoint:&pt];
		    [palette[currentpalette] unlockFocus];
		    [window setEventMask:oldMask];
		    [self display];
		}
		break;
	    case NX_MOUSEDOWN:
		if (placeimage) break;
	    case NX_MOUSEDRAGGED:
		if (placeimage) {
		    mpt2 = theEvent->location; 
		    [self convertPoint:&mpt2 fromView:nil];
		    ppt2.x = ppt1.x + (mpt2.x - mpt1.x);
		    ppt2.y = ppt1.y + (mpt2.y - mpt1.y);
		    pt.x = mpt2.x - mpt1.x;
		    pt.y = mpt2.y - mpt1.y;
		    [self lockFocus];
		    [palette[currentpalette] composite:NX_COPY toPoint:&pt0];	    
		    [bitmap composite:NX_COPY toPoint:&pt];
		    [position composite:NX_SOVER toPoint:&ppt2];
		    [window flushWindow];
		    [self unlockFocus];
		    NXPing();
		} else if (theEvent->flags & NX_ALTERNATEMASK) {
		    [palette[currentpalette] lockFocus];
		    x = (x / S_WIDTH) * S_WIDTH;
		    y = (y / S_HEIGHT) * S_HEIGHT;
		    [colorPicker _getColor:&aColor];
		    NXSetColor(aColor);
		    NXSetRect(&fr, (float)x, (float)y, S_WIDTH-1.0, S_HEIGHT-1.0);
		    PScompositerect(fr.origin.x, fr.origin.y,
			fr.size.width, fr.size.height, NX_COPY);
		    [palette[currentpalette] unlockFocus];
		    [self lockFocus];
		    [palette[currentpalette] composite:1 fromRect:&fr toPoint:&fr.origin];    
		    [self unlockFocus];
		    [window flushWindow];
		} else {
		    [palette[currentpalette] lockFocus];
		    _NXReadPixels(&r, &g, &b, &a, mouseLocation.x, mouseLocation.y);
		    newcolor = NXConvertRGBAToColor(ITOF(r), ITOF(g), ITOF(b), ITOF(a));
		    [palette[currentpalette] unlockFocus];
		    [colorPicker setAndSendColorValues:newcolor:0];
		}
	}
	if (loop) {
	    theEvent = [NXApp getNextEvent:(NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK)];
	    validEvent = NO;
	    while (!placeimage && !validEvent) {
		mouseLocation = theEvent->location; 
		[self convertPoint:&mouseLocation fromView:nil];
		x = (int)mouseLocation.x;
		y = (int)mouseLocation.y;
		if (!NXPointInRect(&mouseLocation, &bounds)) {
		    theEvent = [NXApp getNextEvent:(NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK)];
		} else {
		    validEvent = YES;
		}
	    }
	}
    }

    if (placeimage) {
	[bitmap free];
	bitmap = nil;
	placeimage = 0;
    } else {
	[colorPicker updateColor:0];
    }
    
    return self;
}	
	
- setOptionButton:anObject
{
    optionButton = anObject;
    return self;
}

- setPaletteButton:anObject
{
    paletteButton = anObject;
    return self;
}


@end


/*
    
4/1/90 kro	added opening, saving, copying and pasting TIFF

91
--
 8/12/90 keith	cleaned up a little, changed use of Bitmap to NXImage

96
--
 10/2/90 aozer	Changed bitmap name from position to NXposition.
 10/4/90 aozer	Made NXposition (and all bitmaps) into NXImages.

98
--
 10/18/90 aozer	Disabled the second palette (default); it was just all black.
 10/18/90 aozer	Checked for return value of readType:... & open stream in
		pasteTIFF (bug 5537).

*/

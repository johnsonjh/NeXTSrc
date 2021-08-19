/*
	NXColorCustom.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "colorPickerPrivate.h"
#import "NXColorCell.h"
#import "NXColorCustom.h"
#import "NXColorPicker.h"
#import "NXBrowser.h"
#import "Application.h"
#import "Button.h"
#import "Form.h"
#import "OpenPanel.h"
#import "SavePanel.h"
#import "PopUpList.h"
#import "Window.h"
#import "color.h"
#import "tiff.h"
#import <dpsclient/wraps.h>
#import <libc.h>
#import <mach.h>
#import <sys/dir.h>
#import <NXCType.h>
#import <time.h>

#define ALERT_ICON_TAG 123
#define ALERT_BUTTON_TAG 12321
#define ALERT_TEXT1_TAG 12345
#define ALERT_TEXT2_TAG 1234

#define ITOF(a) ((float)((1.0/255.0)*(a)))
#define FTOI(a) ((int)(a/(1.0/255.0)))
#define INTRND(x) ((int)((((x)-floor(x))<=.5)?floor(x):ceil(x)))
#define COPY_STRIP_DIR_AND_EXT(A,B) (*(strrchr(strcpy(A,strrchr(B, '/')+1), '.')) = '\0')
#define COPY_STRIP_DIR(A,B) (strcpy(A,strrchr(B, '/')+1))
#define STRIP_EXT(A) (*(strrchr(A, '.')) = '\0')

#define CHARSIZE 250
#define DEFAULT_COLOR_LIST_NAME KitString(ColorPanel,"NeXT","the default color lists Title")
#define FREE(x) if(x)free(x),x=0
#define COLOR_EXTENSION ".color"

#define NEW_COLOR_TAG 1
#define REMOVE_COLOR_TAG 2
#define NEW_LIST_TAG 3
#define OPEN_LIST_TAG 4
#define REMOVE_LIST_TAG 5

extern int alphasort(struct direct **d1, struct direct **d2);

@implementation  NXColorCustom

+ new 
{
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

- init
{
    defaultColors[0].color = NX_COLORBLACK;
    defaultColors[0].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Black","the color black"), [self zone]);
    defaultColors[0].type = 0;
    defaultColors[1].color = NX_COLORBLUE;
    defaultColors[1].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Blue","the color blue"), [self zone]);
    defaultColors[1].type = 0;
    defaultColors[2].color = NX_COLORBROWN;
    defaultColors[2].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Brown","the color brown"), [self zone]);
    defaultColors[2].type = 0;
    defaultColors[3].color = NX_COLORCYAN;
    defaultColors[3].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Cyan","the color cyan"), [self zone]);
    defaultColors[3].type = 1;
    defaultColors[4].color = NX_COLORGREEN;
    defaultColors[4].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Green","the color green"), [self zone]);
    defaultColors[4].type = 0;
    defaultColors[5].color = NX_COLORMAGENTA;
    defaultColors[5].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Magenta","the color magenta"), [self zone]);
    defaultColors[5].type = 1;
    defaultColors[6].color = NX_COLORORANGE;
    defaultColors[6].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Orange","the color orange"), [self zone]);
    defaultColors[6].type = 0;
    defaultColors[7].color = NX_COLORPURPLE;
    defaultColors[7].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Purple","the color purple"), [self zone]);
    defaultColors[7].type = 0;
    defaultColors[8].color = NX_COLORRED;
    defaultColors[8].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Red","the color red"), [self zone]);
    defaultColors[8].type = 0;
    defaultColors[9].color = NX_COLORYELLOW;
    defaultColors[9].name = NXCopyStringBufferFromZone(KitString(ColorPanel,"Yellow","the color yellow"), [self zone]);
    defaultColors[9].type = 1;
    
    numlists = 1;
    
    return self;
}

- setColorBrowser:anObject
{
    colorBrowser = anObject;
    [colorBrowser setCellClass:[NXColorCell class]];
    [colorBrowser setDelegate:self];
    [colorBrowser setTarget:self];
    [colorBrowser setAction:@selector(setCustomColor:)];
    [colorBrowser hideLeftAndRightScrollButtons:YES];
    [colorBrowser setTitled:NO];
    [colorBrowser setMaxVisibleColumns:1];
    [colorBrowser useScrollButtons:YES];
    [colorBrowser allowMultiSel:NO];
    [colorBrowser allowBranchSel:NO];
    [colorBrowser loadColumnZero];
    return self;
}

- (int)browser:sender fillMatrix:matrix inColumn:(int)column
{
    int a;
    id cell;
    
    [matrix allowEmptySel:NO];
    if (!currentlist) {
        [matrix renewRows:NUMDEFAULTS cols:1];
	for (a = 0; a < NUMDEFAULTS; a++) {
	    cell = [matrix cellAt:a:0];
	    [cell setColorType:defaultColors[a].type];
	    [cell setSwatchColor:defaultColors[a].color];
	    [cell setStringValue:defaultColors[a].name];
	}
	return NUMDEFAULTS;
    } else {
        return [self addColorList:matrix named:cfname[currentlist]];
    }
}

- setCustomColor:sender
{
    [colorPicker setColor:[[[colorBrowser matrixInColumn:0] selectedCell] swatchColor]];
    return self;
}

static int isColor(struct direct *dentry)
{
    char *period = strrchr(dentry->d_name, '.');
    return (period && !strcmp(period, COLOR_EXTENSION));
}

- newColorFile
{
    BOOL ok;
    id buttons;
    char newfile[MAXPATHLEN+1];
    
    if (!saveAlert) {
	LOADCOLORPICKERNIB("ColorCustomAlert", self);
        [[[saveAlert contentView] findViewWithTag:ALERT_ICON_TAG] setIcon:"app"];
    }

    buttons = [[saveAlert contentView] findViewWithTag:ALERT_BUTTON_TAG];
    [buttons setAction:@selector(doSave:)];
    [[[saveAlert contentView] findViewWithTag:ALERT_TEXT1_TAG] setStringValue:
	KitString(ColorPanel,"Custom Color Lists will always appear in the Color Panel", "Text in the alert for creating a new color list.")];
    [[[saveAlert contentView] findViewWithTag:ALERT_TEXT2_TAG] setStringValue:
	KitString(ColorPanel,"as a new custom list.","End of sentence Add color ...")];
    [alertField setStringValue:KitString(ColorPanel,"Untitled","default color in field") at:0];
    [alertField selectTextAt:0];
    ok = ![NXApp runModalFor:saveAlert];
    [saveAlert orderOut:self];
    if (!ok) return self;

    currentlist = numlists++;
    strcpy(newfile, NXHomeDirectory());
    strcat(newfile, "/.NeXT/Colors/");
    strcat(newfile, [alertField stringValueAt:0]);
    strcat(newfile, COLOR_EXTENSION);
    FREE(cfname[currentlist]);
    cfname[currentlist] = NXCopyStringBufferFromZone(newfile, [self zone]);
    
    [customPopUp addItem:[alertField stringValueAt:0]];
    [customButton setTitle:[alertField stringValueAt:0]];
    [self writeList:cfname[currentlist] fromMatrix:nil];
    [colorBrowser loadColumnZero];
    
    return self;
}

- doSave:sender
{
    char newfile[MAXPATHLEN+1];
    
    if (![sender selectedTag]) return [NXApp stopModal:YES];

    strcpy(newfile, NXHomeDirectory());
    strcat(newfile, "/.NeXT/Colors/");
    strcat(newfile, [alertField stringValueAt:0]);
    strcat(newfile, COLOR_EXTENSION);

    if (!access(newfile, F_OK|W_OK)) {
	_NXKitAlert("ColorPanel", "Palette Exists", "This Palette exists.  Try another name.", NULL, NULL, NULL);
	return self;
    }

    return [NXApp stopModal:NO];
}

- openColorFile
{
    id op, opdelegate;
    char *opdir;
    const char *fileName;
    char newColorListFile[MAXPATHLEN+1], colorDirectory[MAXPATHLEN+1];
    static const char *fileTypes[] = {"color", NULL};
    
    strcpy(colorDirectory, NXHomeDirectory());
    strcat(colorDirectory, "/.NeXT/Colors/");

    op = [OpenPanel new];
    opdelegate = [op delegate];
    opdir = NXCopyStringBufferFromZone([op directory], NXDefaultMallocZone());
    [op setDelegate:self];

    if ([op runModalForTypes:fileTypes] && (fileName = [op filename])) {
	strcpy(newColorListFile, fileName);
	currentlist = numlists++;
	FREE(cfname[currentlist]);
	cfname[currentlist] = NXCopyStringBufferFromZone(fileName, [self zone]);
	[colorBrowser loadColumnZero];
	COPY_STRIP_DIR(newColorListFile, cfname[currentlist]);
	strcat(colorDirectory, newColorListFile);
	FREE(cfname[currentlist]);
	cfname[currentlist] = NXCopyStringBufferFromZone(colorDirectory, [self zone]);
	[self writeList:cfname[currentlist] fromMatrix:[colorBrowser matrixInColumn:0]];
	COPY_STRIP_DIR_AND_EXT(newColorListFile, cfname[currentlist]);
	[customPopUp addItem:newColorListFile];
	[customButton setTitle:newColorListFile];
    }

    [op setDelegate:opdelegate];
    [op setDirectory:opdir];
    free(opdir);

    return self;
}

- (BOOL)panelValidateFilename:sender
{
    id matrix;
    int a, cnt;
    const char *title;
    char fileName[MAXPATHLEN+1];

    COPY_STRIP_DIR_AND_EXT(fileName, [sender filename]);
    matrix = [customPopUp itemList];
    cnt = [matrix cellCount];
    for (a = 0; a < cnt; a++){
	title = [[matrix cellAt:a :0] title];
	if (!strcmp(fileName, title)) {
	    _NXKitAlert("ColorPanel", "Color List", "A %s Color List is already open", NULL, NULL, NULL, fileName);
	    return NO;
        }
    }

    return YES;
}

static int numlength(const char *num)
{
    int cnt = 0;
    while ((*num > '0' && *num < '9') || (*num == '.')) {
	cnt++;
	num++;
    }
    return cnt;
}

static int numcompare(const char *s1, const char *s2, int *length)
{
    float num1, num2;
    
    sscanf(s1, "%f", &num1);
    sscanf(s2, "%f", &num2);
    if (num1 < num2) {
	return -1;
    } else if (num1 > num2) {
	return 1;
    } else {
       *length = numlength(s1);
       return 0;
    }
}

static int mystrcmp(const char *s1, const char *s2)
{
    int num, ret;
   
    while (*s1 && *s2) {
	if (NXIsDigit(*s1) && NXIsDigit(*s2)){
	    if (ret = numcompare(s1, s2, &num)) {
		return ret;
	    } else {
		s1 += num;
		s2 += num;
	    }
	}
	if (*s1 < *s2) {
	    return -1;
	} else if (*s1 > *s2) {
	    return 1;
	}
	s1++;
	s2++;
    }
    if (s1) {
	return 1;
    } else if (s2) {
	return -1;
    } else {
	return 0;
    }
}

static int comparecolors(const void *p, const void *q)
{
    return mystrcmp(((NXCustomColor *)p)->name, ((NXCustomColor *)q)->name);
}

- (int)addColorList:matrix named:(char *)file
{
    id cell;
    FILE *inputFile;
    float c, m, y, k;
    NXCustomColor *newcolors;
    int ncsize, a, matches, type, colorcount, dummy;
    char buffer[MAXPATHLEN+1], linebuf[MAXPATHLEN+1];

    /* OPEN THE FILE OF NEW COLORS */
    ncsize = 23;
    newcolors = NXZoneMalloc([self zone], ncsize * sizeof(NXCustomColor));

    colorcount = 0;
    if ((inputFile = fopen(file, "r")) != NULL){
	fgets(linebuf, 1024, inputFile);
	sscanf(linebuf, "%d", &dummy);
	while (fgets(linebuf, 1024, inputFile) != NULL) {
	    matches = sscanf(linebuf, "%d %f %f %f %f %s", &type, &c, &m, &y, &k, buffer);
	    if (matches == 6) {
		if (colorcount >= ncsize) {
		    ncsize *= 2;
		    newcolors = NXZoneRealloc([self zone], newcolors, ncsize * sizeof(NXCustomColor));
		}
		if (buffer[strlen(buffer)-1] == '\n') buffer[strlen(buffer)-1] = '\0';
		newcolors[colorcount].name = NXCopyStringBuffer(buffer);
		newcolors[colorcount].type = type;
		if (type == RGBATYPE) {
		    newcolors[colorcount].color = NXConvertRGBAToColor(c, m, y, k);
		} else { 
		    newcolors[colorcount].color = NXConvertCMYKAToColor(c, m, y, k, 1.0);
		}
		colorcount++;
	    }
	}
	fclose(inputFile);
    } else {
	_NXKitAlert("ColorPanel","File Error", "Cant find file.", NULL, NULL, NULL);
        return 0;
    }

    /* SORT THE NEW COLORS */
    qsort(newcolors, colorcount, sizeof(NXCustomColor), comparecolors);

    /* LOAD THE NEW MATRIX */
    [matrix renewRows:colorcount cols:1];
    for (a = 0; a < colorcount; a++) {
	cell = [matrix cellAt:a:0];
	[cell setColorType:newcolors[a].type];
	[cell setSwatchColor:newcolors[a].color];
	[cell setStringValueNoCopy:newcolors[a].name shouldFree:YES];
    }

    free(newcolors);

    return colorcount;
}

- writeList:(char *)fname fromMatrix:matrix
{
    NXColor color;
    int a, cnt, type;
    FILE *outputFile;
    
    /* SAVE THE FILE OF COLORS */
    if ((outputFile = fopen(fname, "w")) != NULL){
       if(matrix == nil){
	   fprintf(outputFile, "0\n");
	   fclose(outputFile);
	   return self;
       }
       cnt = [matrix cellCount];
       fprintf(outputFile, "%d\n", cnt);
       for (a = 0; a < cnt; a++){
	    color = [[matrix cellAt:a :0] swatchColor];
	    type = [[matrix cellAt:a :0] colorType];
	    if (type) {
		fprintf(outputFile, "%d %f %f %f %f ", type,
		    NXCyanComponent(color),
		    NXMagentaComponent(color),
		    NXYellowComponent(color),
		    NXBlackComponent(color));
	    } else {
		fprintf(outputFile, "%d %f %f %f %f ", type,
		    NXRedComponent(color),
		    NXGreenComponent(color),
		    NXBlueComponent(color),
		    NXAlphaComponent(color));
	    }
	    fprintf(outputFile, "%s\n", [[matrix cellAt:a:0] stringValue]);
       }
       fclose(outputFile);
    } else {
	_NXKitAlert("ColorPanel", "File Error", "Cant save file.", NULL, NULL, NULL);
    }

    return self;
}

- seekWord:(const char *)word
{
    int a, cnt;
    const char *title;
    id colorMatrix = [colorBrowser matrixInColumn:0];
    
    cnt = [colorMatrix cellCount];
    for (a = 0; a < cnt; a++){
	title = [[colorMatrix cellAt:a:0] stringValue];
	if (title && mystrcmp(word, title) == -1) {
	    [colorMatrix selectCellAt:(a ? a-1 : 0) :0];
	    [colorMatrix scrollCellToVisible:(a ? a-1 : 0) :0];
	    return self;
	}
    }
    [colorMatrix selectCellAt:a-1 :0];
    [colorMatrix scrollCellToVisible:a-1 :0];
    
    return self;
}

- findCustomColors:sender
{
    int a, numentries;
    struct direct **dirp;
    char dirname[MAXPATHLEN+1], filename[MAXPATHLEN+1];
    
    strcpy(dirname, NXHomeDirectory());
    strcat(dirname, "/.NeXT/Colors");
    colorPicker = sender;
    [colorPicker setCustomColorBrowser:colorBrowser];
    optionPopUp = [[PopUpList allocFromZone:[self zone]] init];
    [[[optionPopUp addItem:KitString(ColorPanel,"New Color...","command to add a new color")] setEnabled:NO] setTag:NEW_COLOR_TAG];
    [[[optionPopUp addItem:KitString(ColorPanel,"Remove Color","command to remove a color")] setEnabled:NO] setTag:REMOVE_COLOR_TAG];
    [[optionPopUp addItem:KitString(ColorPanel,"New List...","command to add a new list")] setTag:NEW_LIST_TAG];
    [[optionPopUp addItem:KitString(ColorPanel,"Open List...","command to open a new list")] setTag:OPEN_LIST_TAG];
    [[[optionPopUp addItem:KitString(ColorPanel,"Remove List","command to remove a list")] setEnabled:NO] setTag:REMOVE_LIST_TAG];
    NXAttachPopUpList(optionButton,optionPopUp);
    [optionPopUp changeButtonTitle:NO];
    [optionButton setIcon:"pulldown"];
    [optionButton setTitle:KitString(ColorPanel,"Options","title of pop up for custom color commands")];
    
    numentries = scandir(dirname, &dirp, &isColor, alphasort);
    customPopUp = [[PopUpList allocFromZone:[self zone]] init];
    
    /* FIRST ADD THE DEFAULT COLOR LIST */

    [customButton setTitle:DEFAULT_COLOR_LIST_NAME];
    NXAttachPopUpList(customButton,customPopUp);
    [customPopUp addItem:DEFAULT_COLOR_LIST_NAME];
    cfname[0] = NXCopyStringBufferFromZone(DEFAULT_COLOR_LIST_NAME, [self zone]);
    
    if (numentries != -1) {
	for (a = 0; a < numentries; a++) {
	    if (a <= MAXCOLORLISTS) {
		numlists++;
		strcpy(filename, dirname);
		strcat(filename, "/");
		strcat(filename, dirp[numlists - 2]->d_name);
		FREE(cfname[numlists-1]);	/* why? */
		cfname[numlists-1] = NXCopyStringBufferFromZone(filename, [self zone]);
		STRIP_EXT(dirp[a]->d_name);
		[customPopUp addItem:dirp[a]->d_name];
	    }
	    free(dirp[a]);
	}
	free(dirp);
    }

    currentlist = 0;

    [[customPopUp itemList] sizeToFit];	/* necessary? */
    [customPopUp display];
    
    return self;
}


- removeColorList
{
    int x;
    char removename[MAXPATHLEN+1], removefile[MAXPATHLEN+1];
    
    if (currentlist < 1) {
	_NXKitAlert("ColorPanel","System Color", "You can not remove the System Color List.", NULL, NULL, NULL);
        return self;
    }
    
    if (numlists <= 1) return self;

    strcpy(removename, [customButton title]);
    if(!_NXKitAlert("ColorPanel","Remove Color List", "Are you sure you want to remove %s from your custom color lists?", "Remove", "Cancel", NULL, removename))
    	return self;

    strcpy(removefile, NXHomeDirectory());
    strcat(removefile, "/.NeXT/Colors/");
    strcat(removefile, removename);
    strcat(removefile, COLOR_EXTENSION);
    if (!access(removefile, W_OK|F_OK)) unlink(removefile);

    FREE(cfname[currentlist]);
    for (x = currentlist; x < MAXCOLORLISTS-1; x++) cfname[x] = cfname[x+1];

    currentlist--;
    numlists--;
    if (currentlist < 0) currentlist = 0;

    [customPopUp removeItem:removename];
    [[customPopUp itemList] sizeToFit];
    [[customPopUp itemList] selectCellAt:currentlist :0];
    [customPopUp display];
    [customButton setTitle:[customPopUp selectedItem]]; 
    currentlist = [customPopUp indexOfItem:[customPopUp selectedItem]];

    [colorBrowser loadColumnZero];

    return self;
}

- addColor
{
    if (!saveAlert) {
	LOADCOLORPICKERNIB("ColorCustomAlert", self);
        [[[saveAlert contentView] findViewWithTag:ALERT_ICON_TAG] setIcon:"app"];
    }
    [[[saveAlert contentView] findViewWithTag:ALERT_BUTTON_TAG] setAction:@selector(doNew:)];
    [[[saveAlert contentView] findViewWithTag:ALERT_TEXT1_TAG] setStringValue:
	KitString(ColorPanel,"Custom Colors will always appear in the Color Panel","informative text when adding a new custom color") ];
    [[[saveAlert contentView] findViewWithTag:ALERT_TEXT2_TAG] setStringValue:
	KitString(ColorPanel,"as a new custom color.","more informative text when adding a new custom color")];
    
    [alertField setStringValue:KitString(ColorPanel,"Untitled","default color name") at:0];
    [alertField selectTextAt:0];
    if (![NXApp runModalFor:saveAlert]) {
	[self writeList:cfname[currentlist] fromMatrix:[colorBrowser matrixInColumn:0]];
    }
    [saveAlert orderOut:self];
    return self;
}

- doNew:sender
{
    id cell, matrix;
    const char *title;
    int a, retval, cnt;
    NXSize cellSize, matrixCellSize;
    char newcolor[MAXPATHLEN+1];
    
    if (![sender selectedTag]) return [NXApp stopModal:YES];

    strcpy(newcolor, [alertField stringValueAt:0]);

    matrix = [colorBrowser matrixInColumn:0];
    for (a = 0, cnt = [matrix cellCount]; a < cnt; a++) {
        title = [[matrix cellAt:a:0] stringValue];
        retval = title ? 1 : strcmp(newcolor, title);
	if (!retval) {  
            _NXKitAlert("ColorPanel","Color Exists", "This Color exists.  Try another name.", NULL, NULL, NULL);
            return self;
        } else if (retval < 0) {
	    break;
	}
    }

    [NXApp stopModal:NO];

    [matrix insertRowAt:a];
    cell = [matrix cellAt:a:0];
    [cell setStringValue:newcolor];
    [cell setColorType:RGBATYPE];
    [cell setSwatchColor:[colorPicker color]];
    [cell setLoaded:YES];

    if (!a) {
	[cell calcCellSize:&cellSize];
	[matrix getCellSize:&matrixCellSize];
	cellSize.width = matrixCellSize.width;
	[matrix setCellSize:&cellSize];
    }

    [matrix sizeToCells];
    [matrix selectCellAt:a:0];
    [matrix scrollCellToVisible:a:0];
    [matrix display];

    return self;
}

- removeColor
{
    int x;
    id matrix;
    const char *listname, *colorname;

    if (currentlist < 1) {
	_NXKitAlert("ColorPanel", "System Color", "You can not remove a color from the System Color List.", NULL, NULL, NULL);
        return self;
    }

    colorname = [[[colorBrowser matrixInColumn:0] selectedCell] stringValue];
    listname = [customButton title];

    if (!colorname || !listname) return self;

    if(!_NXKitAlert("ColorPanel","Remove Color", "Are you sure you want to remove %s from your %s custom color list?", "Remove", "Cancel", NULL,colorname,listname))
    	return self;

    matrix = [colorBrowser matrixInColumn:0];
    x = [matrix selectedRow];
    [matrix removeRowAt:x andFree:NO];
    [matrix sizeToCells];
    [matrix selectCellAt:x:0];
    [matrix display];

    [self writeList:cfname[currentlist] fromMatrix:matrix];

    return self;
}

- setCustomButton:anObject
{
    customButton = anObject;
    return self;
}

- setCurrentCustom:sender
{
    int newlist;
    
    newlist = [sender selectedRow];
    if (newlist == currentlist) {
        return self;
    } else {
        currentlist = newlist;
    }
    [colorBrowser loadColumnZero];
    if (currentlist < 1 || access(cfname[currentlist], W_OK|F_OK)) {
       [[[optionPopUp itemList] findCellWithTag:NEW_COLOR_TAG] setEnabled:NO];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_COLOR_TAG] setEnabled:NO];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_LIST_TAG] setEnabled:NO];
    } else {
       [[[optionPopUp itemList] findCellWithTag:NEW_COLOR_TAG] setEnabled:YES];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_COLOR_TAG] setEnabled:YES];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_LIST_TAG] setEnabled:YES];
    }

    return self;
}

- doCustomOption:sender
{
   switch ([[sender selectedCell] tag]){
      case 0:
         break;
      case NEW_COLOR_TAG:
	 [self addColor];
         break;
      case REMOVE_COLOR_TAG:
         [self removeColor];
         break;
      case NEW_LIST_TAG:
         [self newColorFile];
         break;
      case OPEN_LIST_TAG:
         [self openColorFile];
         break;
      case REMOVE_LIST_TAG:
         [self removeColorList];
         break;
    }
    if (currentlist < 1 || access(cfname[currentlist], W_OK|F_OK)) {
       [[[optionPopUp itemList] findCellWithTag:NEW_COLOR_TAG] setEnabled:NO];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_COLOR_TAG] setEnabled:NO];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_LIST_TAG] setEnabled:NO];
    } else {
       [[[optionPopUp itemList] findCellWithTag:NEW_COLOR_TAG] setEnabled:YES];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_COLOR_TAG] setEnabled:YES];
       [[[optionPopUp itemList] findCellWithTag:REMOVE_LIST_TAG] setEnabled:YES];
    }
    [[optionPopUp itemList] display];
    return self;
}

- keyDown:(NXEvent *)theEvent
{
   struct timeval curtime; 
   struct timezone tzp;
   static int numkey = 0;
   static int oldtime = 0;
   static char findword[100];
   
   gettimeofday(&curtime, &tzp);
   if (numkey < 100 && curtime.tv_sec - oldtime < 2) {
       findword[numkey++] = theEvent->data.key.charCode;
       findword[numkey] = '\0';
   } else {
       numkey = 0;
       findword[numkey++] = theEvent->data.key.charCode;
       findword[numkey] = '\0';
   }
   oldtime = curtime.tv_sec;  
   [self seekWord:findword];
   [self setCustomColor:self];
   
   return self;
}

- updateCustomColorList
{
    [colorBrowser loadColumnZero];
    return self;
}

- setOptionButton:anObject
{
    optionButton = anObject;
    return self;
}

@end

/*
    
4/1/90 kro	added custom color ability to color picker

91
--
 8/12/90 keith	cleaned up a little

92
--
 8/21/90 pah & keith	cleaned up a little more

95
--
 10/2/90 keith	fixed orderOut of modal alerts


*/

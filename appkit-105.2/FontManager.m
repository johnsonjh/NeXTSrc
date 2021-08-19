/*
	FontManager.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif

#import "appkitPrivate.h"
#import "Font_Private.h"
#import "FontManager.h"
#import "FontPanel.h"
#import "FontFile.h"
#import "Application.h"
#import "Control.h"
#import "Menu.h"
#import "MenuCell.h"
#import "Matrix.h"
#import "nextstd.h"
#import <streams/streams.h>
#import <stdio.h>
#import <string.h>
#import <sys/param.h>
#import <sys/file.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <objc/List.h>
#import <zone.h>

static const char   fontpath[] = _NX_FONTPATH;

@implementation FontManager

/* Factory methods */

static FontManager *sharedFontManager = nil;
static id fontpanelfactory = nil;
static id fontFiles = nil;
static const char *bold = NULL, *unbold, *italic, *unitalic, *heavier, *lighter, *smaller, *larger;

static void initStrings()
{
    bold = KitString(FontManager, "Bold", "Menu item which makes a font bold.");
    unbold = KitString(FontManager, "Unbold", "Menu item which makes a font not bold.");
    italic = KitString(FontManager, "Italic", "Menu item which makes a font italic.");
    unitalic = KitString(FontManager, "Unitalic", "Menu item which makes a font not italic.");
    heavier = KitString(FontManager, "Heavier", "Menu item which makes a font heavier (thicker).");
    lighter = KitString(FontManager, "Lighter", "Menu item which makes a font lighter (thinner).");
    smaller = KitString(FontManager, "Smaller", "Menu item which makes a font go down one point in size.");
    larger = KitString(FontManager, "Larger", "Menu item which makes a font go up one point in size.");
}

+ (BOOL)_canAlloc { return NO; }

+ allocFromZone:(NXZone *)zone { return [self doesNotRecognize:_cmd]; }

+ alloc { return [self doesNotRecognize:_cmd]; }


+ setFontPanelFactory:factoryId
{
    fontpanelfactory = factoryId;
    return self;
}

+ new
{
    NXZone *zone;

    if (!sharedFontManager) {
	zone = [NXApp zone];
	sharedFontManager = [[super allocFromZone:(zone ? zone : NXDefaultMallocZone())] init];
	sharedFontManager->action = @selector(changeFont:);
	if (!fontpanelfactory) fontpanelfactory = [FontPanel class];
	if (NXSystemFont)
	    [sharedFontManager setSelFont:[Font newFont:NXSystemFont size:12.0] isMultiple:NO];
    }

    return sharedFontManager;
}

/*
 * The following functions and methods are used to manage the FontFile
 * objects which the FontManager uses to quickly convert one font to another
 * or get information about a given font.  These methods and functions are
 * not public or visible to subclassers, but their functionality is
 * available via the \fBconvert:\fR... methods as well as \fBfindFont:\fR...
 * and \fBgetFamily:\fR... methods.
 */

/*
 * Maps in the font summary information in all the \fB.fontdirectory\fR files.
 * This MUST be done before ANY of the above functionality will work.
 * FontFile new: knows that ':' is a delimiter in the name of the file.
 */

- _loadFontFiles
{
    id                  ff;
    const char         *path;

    fontFiles = [[List allocFromZone:[self zone]] init];

    path = fontpath;
    while (path) {
	if (ff = [[FontFile allocFromZone:[self zone]] init:path]) {
	    [fontFiles addObject:ff];
	}
	if (path = strchr(path, ':')) {
	    path++;
	}
    }

    return self;
}

/*
 * A tag encodes a FontFile and an offset in that file. tag = 0 is invalid.
 * Note: these functions and macros are duplicated in FontPanel.m.
 * The shouldn't ever really change, but if they do, keep them in sync.
 */

static int
tagOf(id file, int offset)
{
    int                 tag = [fontFiles indexOf:file] + 1;

    if (tag > 0) {
	tag <<= 24;
	tag += offset;
    } else {
	tag = 0;
    }
    return tag;
}

static id
fileOf(int tag)
{
    if (!fontFiles)
	[sharedFontManager _loadFontFiles];
    tag = ((tag & 037700000000) >> 24) - 1;
    if (tag >= 0 && tag < [fontFiles count]) {
	return[fontFiles objectAt:tag];
    } else {
	return nil;
    }
}

#define offsetOf(tag) (tag & 077777777)

/* Macros which get information about a font given a tag */

#define weightOf(tag) [fileOf(tag) weight:offsetOf(tag)]

#define traitsOf(tag) [fileOf(tag) traits:offsetOf(tag)]

#define nameOf(tag) [fileOf(tag) name:offsetOf(tag)]

#define isItalic(tag) [fileOf(tag) isItalic:offsetOf(tag)]

/* Returns the tag of a font given a PostScript name from the passed metrics */

int
_NXFindFont(NXFontMetrics *fm)
{
    id                  file;
    int                 i, offset, count;

    if (!fontFiles) {
	[sharedFontManager _loadFontFiles];
    }
    count = [fontFiles count];
    if (fm && count) {
	for (i = 0; i < count; i++) {
	    file = [fontFiles objectAt:i];
	    offset = [file offset:fm->name];
	    if (offset >= 0) {
		return tagOf(file, offset);
	    }
	}
    }
    return 0;
}

/*
 * Returns the tag of a font which matches the font represented by the
 * specified tag, but converted to the specified family.
 */

int
_NXMatchFamily(int tag, const char *family)
{
    id                  matchFile, file;
    int                 i, offset, count, matchOffset;

    matchFile = fileOf(tag);
    matchOffset = offsetOf(tag);

    count = [fontFiles count];
    for (i = 0; i < count; i++) {
	file = [fontFiles objectAt:i];
	offset = [file matchFamily:family to:matchFile at:matchOffset];
	if (offset >= 0) {
	    return tagOf(file, offset);
	}
    }

    return 0;
}

/*
 * Returns the tag of a font which matches the specified family, traits
 * and weight.  Returns 0 if one cannot be found.
 */

int
_NXMatchFont(const char *family, NXFontTraitMask traits, int weight)
{
    id                  file;
    int                 i, offset, count;

    if (!fontFiles) {
	[sharedFontManager _loadFontFiles];
    }
    count = [fontFiles count];
    for (i = 0; i < count; i++) {
	file = [fontFiles objectAt:i];
	offset = [file matchFamily:family traits:traits weight:weight];
	if (offset >= 0) {
	    return tagOf(file, offset);
	}
    }

    return 0;
}

/*
 * Returns the fontFiles List.  This is used by the NeXT FontPanel to
 * efficiently fill the browsers, etc.  Unfortunately, since we don't
 * publish the FontFile API, it would probably be difficult for someone
 * writing their own FontPanel to duplicate this functionality as
 * efficiently as we do. 
 */

- _fontFiles
{
    if (!fontFiles) {
	[self _loadFontFiles];
    }
    return fontFiles;
}

/* Creating the menu. */

#define addmitem(menu, title, theAction, ke, tag) \
   [[menu addItem:title action:theAction keyEquivalent:ke] setTag:tag];

static id createMenu()
{
    id                  menu;

    menu = [Menu newTitle:KitString(FontManager, "Font", "The title of the Font menu.")];

    [menu addItem: KitString(FontManager, "Font Panel...", NULL)
	action:@selector(orderFrontFontPanel:)
	keyEquivalent:*KitString(FontManager, "t", "Key Equivalent for Font Panel...")];

    if (!bold) initStrings();

    addmitem(menu, bold, @selector(changeFontTrait:), *KitString(FontManager, "b", "Key Equivalent for Bold."), NX_BOLD);
    addmitem(menu, italic,  @selector(changeFontTrait:), *KitString(FontManager, "i", "Key Equivalent for Italic."), NX_ITALIC);
    addmitem(menu,KitString(FontManager, "Larger", "Menu item which makes the font larger."), @selector(changeFont:), 0, NX_SIZEUP);
    addmitem(menu, KitString(FontManager, "Smaller", "Menu item which makes the font smaller."), @selector(changeFont:), 0, NX_SIZEDOWN);
    addmitem(menu, heavier, @selector(changeFont:), 0, NX_HEAVIER);
    addmitem(menu, lighter, @selector(changeFont:), 0, NX_LIGHTER);

    [menu sizeToFit];

    return menu;
}

#ifndef SPECULATE

/*
 * InterfaceBuilder calls this to set the menu instance variable.
 */

static BOOL removeMatrixItem(id matrix, const char *name)
{
    const char *s;
    id cell;
    int row, rows, cols;

    if (name && matrix) {
	[matrix getNumRows:&rows numCols:&cols];
	for (row = 0; row < rows; row++) {
	    cell = [matrix cellAt:row :0];
	    s = [cell stringValue];
	    if (s && !strcmp(s, name)) {
		[matrix removeRowAt:row andFree:YES];
		return YES;
	    }
	}
    }

    return NO;
}

- setMenu:anObject
/* This should become public post-1.0 (Mathematica uses it, for example). */
{
    id matrix;

    menu = anObject;
    [menu setAutoupdate:YES];
    matrix = [menu itemList];
    if (!bold) initStrings();
    if (removeMatrixItem(matrix, unitalic) || removeMatrixItem(matrix, bold)) [menu sizeToFit];

    return self;
}

#endif !SPECULATE

/* Public methods */

- (BOOL)isMultiple
{
    return fmFlags.multipleFont;
}


- selFont
{
    return selFont;
}

static BOOL itemNeedsUpdate(BOOL enable, id item)
{
    if (enable != [item isEnabled]) {
	[item setEnabled:![item isEnabled]];
	return YES;
    } else {
	return NO;
    }
}

static BOOL traitOk(int trait, id file, int offset)
{
    id f;
    int count, i;

    count = [fontFiles count];
    for (i = 0; i < count; i++) {
	f = [fontFiles objectAt:i];
	if ([f matchTrait:trait to:file at:offset] >= 0) {
	    return YES;
	}
    }

    return NO;
}

- _updateMenuItems
{
    int rows, cols;
    id f, file, cell, matrix;
    const char *title;
    NXFontMetrics	*fm;
    int i, row, tag, offset, weight, count;
    BOOL done, boldOk = YES, italicOk = YES, titleUpdate = NO;

    if (!bold) initStrings();
    fm = [selFont readMetrics:NX_FONTHEADER];
    if (fm) {
	tag = _NXFindFont(fm);
	if (tag) {
	    file = fileOf(tag);
	    offset = offsetOf(tag);
	    matrix = [menu itemList];
	    [matrix getNumRows:&rows numCols:&cols];
	    for (row = 0; row < rows; row++) {
		cell = [matrix cellAt:row :0];
		title = [cell title];
		if (!title) {
		    /* do nothing */
		} else if (!strcmp(title, bold) || !strcmp(title, unbold)) {
		    if ([file isBold:offset]) {
			[cell setTag:NX_UNBOLD];
			if (strcmp(title, unbold)) {
			    [cell setTitle:unbold];
			    titleUpdate = YES;
			}
			boldOk = fmFlags.multipleFont ||
					traitOk(NX_UNBOLD, file, offset);
		    } else {
			[cell setTag:NX_BOLD];
			if (strcmp(title, bold)) {
			    [cell setTitle:bold];
			    titleUpdate = YES;
			}
			boldOk = fmFlags.multipleFont ||
					traitOk(NX_BOLD, file, offset);
		    }
		    if (itemNeedsUpdate(boldOk && !fmFlags.disabled, cell) || titleUpdate) {
			[matrix drawCellInside:cell];
		    }
		} else if(!strcmp(title,italic)||!strcmp(title,unitalic)) {
		    if ([file isItalic:offset]) {
			[cell setTag:NX_UNITALIC];
			if (strcmp(title, unitalic)) {
			    [cell setTitle:unitalic];
			    titleUpdate = YES;
			}
			italicOk = fmFlags.multipleFont ||
					traitOk(NX_UNITALIC, file, offset);
		    } else {
			[cell setTag:NX_ITALIC];
			if (strcmp(title, italic)) {
			    [cell setTitle:italic];
			    titleUpdate = YES;
			}
			italicOk = fmFlags.multipleFont ||
					traitOk(NX_ITALIC, file, offset);
		    }
		    if (itemNeedsUpdate(italicOk && !fmFlags.disabled, cell) || titleUpdate) {
			[matrix drawCellInside:cell];
		    }
		} else if (!strcmp(title, lighter)) {
		    if (!fmFlags.multipleFont) {
			weight = [file weight:offset];
			count = [fontFiles count];
			done = NO;
			i = 0;
			while (!done && i < count) {
			    f = [fontFiles objectAt:i];
			    if ([f modifyWeight:NO of:file at:offset] >= 0) {
				done = YES;
				if (itemNeedsUpdate(!fmFlags.disabled, cell)) {
				    [matrix drawCellInside:cell];
				}
			    }
			    i++;
			}
			if (!done && itemNeedsUpdate(NO, cell)) {
			    [matrix drawCellInside:cell];
			}
		    } else {
			if (itemNeedsUpdate(!fmFlags.disabled, cell)) {
			    [matrix drawCellInside:cell];
			}
		    }
		} else if (!strcmp(title, heavier)) {
		    if (!fmFlags.multipleFont) {
			weight = [file weight:offset];
			count = [fontFiles count];
			done = NO;
			i = 0;
			while (!done && i < count) {
			    f = [fontFiles objectAt:i];
			    if ([f modifyWeight:YES of:file at:offset] >= 0) {
				done = YES;
				if (itemNeedsUpdate(!fmFlags.disabled, cell)) {
				    [matrix drawCellInside:cell];
				}
			    }
			    i++;
			}
			if (!done && itemNeedsUpdate(NO, cell)) {
			    [matrix drawCellInside:cell];
			}
		    } else {
			if (itemNeedsUpdate(!fmFlags.disabled, cell)) {
			    [matrix drawCellInside:cell];
			}
		    }
		} else if (!strcmp(title, smaller)) {
		    if (itemNeedsUpdate(([selFont pointSize] > 1.0) && !fmFlags.disabled, cell)) {
			[matrix drawCellInside:cell];
		    }
		} else if (!strcmp(title, larger)) {
		    if (itemNeedsUpdate(!fmFlags.disabled, cell)) {
			[matrix drawCellInside:cell];
		    }
		}
	    }
	}
    }

    return self;
}


- setSelFont:fontObj isMultiple:(BOOL)flag
{
    if (fontObj) {
	fmFlags.multipleFont = flag ? YES : NO;
	selFont = fontObj;
	[panel setPanelFont:fontObj isMultiple:flag];
	whatToDo = NX_NOFONTCHANGE;
    }

    return self;
}

#ifdef FONT_MANAGER_DEBUG

static void printFont(id self, id font)
{
    const char *family;
    NXFontTraitMask traits;
    int weight;
    float s;

    if (!font) {
	printf("<null font>\n");
    } else {
	[self getFamily:&family traits:&traits weight:&weight size:&s ofFont:font];
	printf("family: %s traits: 0x%x weight: %d\n", family, traits, weight);
    }
}

#endif

- convertFont:fontObj
{
    id retval = nil;

    if (!fontObj) return selFont;

#ifdef FONT_MANAGER_DEBUG
    printf("Request to convert: ");
    printFont(self, fontObj);
#endif

    switch (whatToDo) {
    case NX_VIAPANEL:
	retval = [panel panelConvertFont:fontObj];
	break;
    case NX_ADDTRAIT:
	retval = [self convert:fontObj toHaveTrait:traitToChange];
	break;
    case NX_REMOVETRAIT:
	retval = [self convert:fontObj toNotHaveTrait:traitToChange];
	break;
    case NX_SIZEUP:
	retval = [Font newFont:[fontObj name] size:[fontObj pointSize]+1.0 style:[fontObj style] matrix:[fontObj matrix]];
	break;
    case NX_SIZEDOWN:
	retval = [Font newFont:[fontObj name] size:[fontObj pointSize]-1.0 style:[fontObj style] matrix:[fontObj matrix]];
	break;
    case NX_HEAVIER:
	retval = [self convertWeight:YES of:fontObj];
	break;
    case NX_LIGHTER:
	retval = [self convertWeight:NO of:fontObj];
	break;
    }

#ifdef FONT_MANAGER_DEBUG
    printf("Result: ");
    printFont(self, retval ? retval : fontObj);
#endif

    return retval ? retval : fontObj;
}


- getFontMenu:(BOOL)create
{
    if (menu) {
	return menu;
    } else if (create) {
	[self setMenu:createMenu()];
	[[menu itemList] setTarget:self];
	return menu;
    } else {
	return nil;
    }
}

/* ??? remove this method */
#ifndef SPECULATE

- menu
{
    return[self getFontMenu:YES];
}

#endif !SPECULATE

- getFontPanel:(BOOL)create
{
    if (panel) {
	return panel;
    } else if (create) {
	if (!fontFiles) {
	    [self _loadFontFiles];
	}
	panel = [fontpanelfactory new];
	if (fmFlags.disabled && [panel respondsTo:@selector(setEnabled:)]) [panel setEnabled:NO];
	return panel;
    } else {
	return nil;
    }
}

/* Methods to get info about fonts and to convert from one to another. */

- findFont:(const char *)family
    traits:(NXFontTraitMask)traits
    weight:(int)weight
    size:(float)size
{
    int                 tag;

    if (size > 0.0) {
	if (!(traits&NX_ITALIC)) {
	    traits |= NX_UNITALIC;
	} else {
	    traits &= (~(NX_UNITALIC));
	}
	if (!(traits&(NX_BOLD|NX_UNBOLD)) && weight <= 0) {
	    traits |= NX_UNBOLD;
	}
	tag = _NXMatchFont(family, traits, weight);
	if (!tag && !(traits & NX_NONSTANDARDCHARSET)) {
	    tag = _NXMatchFont(family,traits|NX_NONSTANDARDCHARSET,weight);
	}
	if (tag) {
	    return[Font newFont:nameOf(tag) size:size];
	}
    }
    return nil;
}


- getFamily:(const char **)family
    traits:(NXFontTraitMask *)traits
    weight:(int *)weight
    size:(float *)size
    ofFont:fontObj
{
    id                  file;
    int                 tag, offset;
    NXFontMetrics      *fm;

    fm = [fontObj readMetrics:NX_FONTHEADER];
    if (fm) {
	tag = _NXFindFont(fm);
	if (tag) {
	    file = fileOf(tag);
	    offset = offsetOf(tag);
	    if (family) *family = [file family:offset];
	    if (traits) *traits = [file traits:offset];
	    if (weight) *weight = [file weight:offset];
	    if (family) {
		NX_ASSERT(!strcmp(*family, fm->familyName), "Font family metric does not match .fontdirectory in [FontManager -getFamily:traits:...");
	    }
	} else {
	    if (family) *family = fm->familyName;
	    if (traits) *traits = 0;
	    if (weight) *weight = 0.0;
	}
	if (size) *size = [fontObj pointSize];
    } else {
	if (family) *family = NULL;
	if (traits) *traits = 0;
	if (weight) *weight = 0;
	if (size) *size = 0.0;
    }

    return self;
}

- (char **)availableFonts
{
    id file;
    char *name;
    char **list = NULL;
    BOOL alreadyEntered;
    int i, j, ffcount, namecount, count = 0;

    if (!fontFiles) {
	[self _loadFontFiles];
    }
    ffcount = [fontFiles count];
    for (i = 0; i < ffcount; i++) {
	count += [[fontFiles objectAt:i] count];
    }
    if (count) {
	list = (char **)NXZoneMalloc([self zone], sizeof(char *) * (count + 1));
	count = 0;
	for (i = 0; i < ffcount; i++) {
	    file = [fontFiles objectAt:i];
	    namecount = [file count];
	    while (namecount--) {
		name = [file name:namecount];
		if (name) {
		    alreadyEntered = NO;
		    for (j = 0; j < count; j++) {
			if (!strcmp(name, list[j])) {
			    alreadyEntered = YES;
			    break;
			}
		    }
		    if (!alreadyEntered) {
			list[count++] = strcpy((char *)NXZoneMalloc([self zone], strlen(name)+1), name);
		    }
		}
	    }
	}
	list[count] = NULL;
    }

    return list;
}
		    
	
	
- convert:fontObj toFamily:(const char *)family
{
    int tag;
    id retval = nil;

    tag = _NXFindFont([fontObj readMetrics:NX_FONTHEADER]);
    if (tag) {
	tag = _NXMatchFamily(tag, family);
	if (tag) retval = [Font newFont:nameOf(tag) size:[fontObj pointSize] style:[fontObj style] matrix:[fontObj matrix]];
    }

    return retval ? retval : fontObj;
}

- convert:fontObj toHaveTrait:(NXFontTraitMask)trait
{
    id retval = nil, file, f;
    int i, tag, match, offset, count;

    tag = _NXFindFont([fontObj readMetrics:NX_FONTHEADER]);
    if (tag) {
	file = fileOf(tag);
	offset = offsetOf(tag);
	if (trait & [file traits:offset]) return fontObj;
	count = [fontFiles count];
	for (i = 0; i < count; i++) {
	    f = [fontFiles objectAt:i];
	    match = [f matchTrait:trait to:file at:offset];
	    if (match >= 0) {
		retval = [Font newFont:[f name:match] size:[fontObj pointSize] style:[fontObj style] matrix:[fontObj matrix]];
	    }
	}
    }

    return retval ? retval : fontObj;
}

- convert:fontObj toNotHaveTrait:(NXFontTraitMask)trait
{
    id retval = nil, file, f;
    int i, tag, match, offset, count;

    tag = _NXFindFont([fontObj readMetrics:NX_FONTHEADER]);
    if (tag) {
	file = fileOf(tag);
	offset = offsetOf(tag);
	if (!(trait & [file traits:offset])) return fontObj;
	count = [fontFiles count];
	for (i = 0; i < count; i++) {
	    f = [fontFiles objectAt:i];
	    match = [f excludeTrait:trait from:file at:offset];
	    if (match >= 0) {
		retval = [Font newFont:[f name:match] size:[fontObj pointSize] style:[fontObj style] matrix:[fontObj matrix]];
	    }
	}
    }

    return retval ? retval : fontObj;
}

- convertWeight:(BOOL)upFlag of:fontObj
{
    id retval = nil, file, f, bestFile = nil;
    int i, tag, weight, match, offset, count, bestMatch = -1, bestWeight = 0;

    tag = _NXFindFont([fontObj readMetrics:NX_FONTHEADER]);
    if (tag) {
	file = fileOf(tag);
	offset = offsetOf(tag);
	weight = [file weight:offset];
	count = [fontFiles count];
	for (i = 0; i < count; i++) {
	    f = [fontFiles objectAt:i];
	    match = [f modifyWeight:upFlag of:file at:offset];
	    if (match >= 0 && (bestMatch < 0 ||
	       ABS([f weight:match] - weight) < ABS(bestWeight - weight))) {
		bestMatch = match;
		bestWeight = [f weight:match];
		bestFile = f;
	    }
	}
    }
    if (bestMatch >= 0) {
	retval = [Font newFont:[bestFile name:bestMatch] size:[fontObj pointSize] style:[fontObj style] matrix:[fontObj matrix]];
    }

    return retval ? retval : fontObj;
}

/* Target/Action methods sent by the font menu. */

/* These are historical and are NOT in the API. */

- changeFontTrait:sender
{
    return [self addFontTrait:sender];
}

- changeFont:sender
{
    return [self modifyFont:sender];
}

- changeFontViaPanel:sender;
{
    return [self modifyFontViaPanel:sender];
}

/* These are the correct versions. */

- addFontTrait:sender
{
    if (traitToChange = [sender selectedTag]) {
	whatToDo = NX_ADDTRAIT;
	[self sendAction];
    }
    return self;
}

- removeFontTrait:sender
{
    if (traitToChange = [sender selectedTag]) {
	whatToDo = NX_REMOVETRAIT;
	[self sendAction];
    }
    return self;
}

- modifyFontTrait:sender
{
    return [self addFontTrait:sender];
}

- modifyFont:sender
{
    if (whatToDo = [sender selectedTag]) {
	[self sendAction];
    }
    return self;
}

- orderFrontFontPanel:sender
{
    return[[self getFontPanel:YES] orderFront:sender];
}

/* Target/Action method invoked by a FontPanel object. */

- modifyFontViaPanel:sender
{
    if (!panel) {
	NX_ASSERT(NO, "Message from non-existent FontPanel in [FontManager -modifyFontViaPanel:]");
	return self;
    }
    whatToDo = NX_VIAPANEL;
    [self sendAction];

    return self;
}

/* Enabling/disabling */

- (BOOL)isEnabled
{
    return !fmFlags.disabled;
}

- setEnabled:(BOOL)flag
{
    fmFlags.disabled = flag ? NO : YES;
    if ([panel respondsTo:@selector(setEnabled:)]) [panel setEnabled:flag];
    return self;
}

/* Managing the target/action of the FontManager. */

- (SEL)action
{
    return action;
}


- setAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "FontManager -setAction:");
    action = aSelector;
    return self;
}


- sendAction
{
    BOOL retval = [NXApp sendAction:action to:nil from:self];
    [self setSelFont:[self convertFont:selFont] isMultiple:fmFlags.multipleFont];
    return retval ? self : nil;
}

- (BOOL)worksWhenModal
{
    return YES;
}

- finishUnarchiving
{
    return[FontManager new];
}

@end

/*
  
Modifications (since 0.8):
  
 3/12/89 pah	Menu should be titled Font not Fonts
 3/14/89 pah	Lazily evaluate reading of the FontFiles (speeds startup time)
 3/15/89 pah	Change menu to getFontMenu: and add getFontPanel:
 3/19/89 pah	Take out custom font support
 3/20/89 pah	Publicized the target/action methods as well as all the
 		 convert: methods.
0.91
----
 5/19/89 trey	minimized static data
 5/22/89 pah	add availableFonts method
 5/27/89 pah	implement font menu scheme (bold/italic toggle)

0.93
----
 6/16/89 pah	fix to accomodate kit functions that now return const char *

0.96
----
 7/20/89 pah	_updateMenuItem: incorrectly disabled heavier/lighter if
		 there is a multiple font selection -- fixed
		only reset whatToDo if setSelFont:isMultiple: sets a NEW
		 font (changing the existing) -- this will help anti-socials
		 who call setSelFont:isMultiple: from their convertFont:'s
		add FONT_MANAGER_DEBUG, if def'ed, it will print out all
		 conversion activity on stdout
		change changeFont:, etc. to modifyFont: (argh!)

79
--
  4/3/90 pah	fixed sendAction to use NXApp's sendAction:to:from: method
		 so that the responder chain is managed in one method only
		made the FontManger so it worksWhenModal

80
--
  4/7/90 pah	sendAction:to:from: moved to Application

83
--
  5/3/90 pah	remove unneccessary sizeToFit in setMenu:

86
--
 6/12/90 pah	Fixed convert: methods to preserve style and matrix of Font

*/

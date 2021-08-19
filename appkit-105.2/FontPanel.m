/*
	FontPanel.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "FontManager_Private.h"
#import "Panel_Private.h"
#import "Cell_Private.h"
#import "View_Private.h"
#import "FontPanel.h"
#import "FontFile.h"
#import "TaggedCell.h"
#import "TextField.h"
#import "Button.h"
#import "ScrollView.h"
#import "Form.h"
#import "Matrix.h"
#import "Menu.h"
#import "NXCursor.h"
#import "NXSplitView.h"
#import "Text.h"
#import "ActionCell.h"
#import "Application.h"
#import "nextstd.h"
#import <stdio.h>
#import <dpsclient/wraps.h>
#import <streams/streams.h>
#import <string.h>
#import <sys/param.h>
#import <sys/file.h>
#import <objc/List.h>

static const char *const standardSizes[13] = { "8", "9", "10", "11", "12", "14", "16", "18", "24", "36", "48", "64", NULL };

@interface FontPanel(Private)
- _loadFamilies;
@end

@implementation FontPanel:Panel

extern int _NXMatchFont(const char *family, NXFontTraitMask traits, int weight);
extern int _NXMatchFamily(int tag, const char *family);
extern int _NXFindFont(NXFontMetrics *fm);

/* Factory methods */

static id fontFiles = nil;
static id sharedFontPanel = nil;
static id fontPanelFactory = nil;
static char *multiFontMessage = NULL;
static char *badFontMessage = NULL;

#define MIN_CHOOSER_SIZE 96.0

+ (BOOL)_canAlloc { return fontPanelFactory != nil; }

/* If someone just calls this out of the blue, its an error.  They must use a new method.  Else we're being called as part of nib loading, and we do a special hack to make sure an object of the right type is created in spite of the class stored in 
the nib file. */
+ allocFromZone:(NXZone *)zone
{
    if (fontPanelFactory)
	return _NXCallPanelSuperFromZone(fontPanelFactory, _cmd, zone);
    else
	return [self doesNotRecognize:_cmd];
}

/* If someone just calls this out of the blue, its an error.  They must use a new method.  We depend on nibInstantiate using allocFromZone:. */
+ alloc
{
    return [self doesNotRecognize:_cmd];
}


+ new
{
    return [self newContent:NULL style:0 backing:0 buttonMask:0 defer:NO];
}


+ newContent:(const NXRect *)contentRect
       style:(int)aStyle
     backing:(int)bufferingType
  buttonMask:(int)mask
       defer:(BOOL)flag
{
    NXCoord delta;
    BOOL gotpbframe = NO;
    NXRect frameRect, pframeRect, pbFrame, chFrame;
    id fontManager = [FontManager new];

    if (fontManager && !sharedFontPanel) {
	if (fontPanelFactory) {
	    return _NXCallPanelSuper(fontPanelFactory,
		      @selector(newContent:style:backing:buttonMask:defer:),
			    contentRect, aStyle, bufferingType, mask, flag);
	} else {
	    gotpbframe = [self _readFrame:&pframeRect fromDefaults:"NXFontPanelPreview" domain:[NXApp appName]];
	    fontPanelFactory = self;
	    self = sharedFontPanel = _NXLoadNibPanel("FontPanel");
	    fontPanelFactory = nil;
	    if (self) {
		manager = fontManager;
		fontFiles = [manager _fontFiles];
		[self _loadFamilies];
		[self setPanelFont:[manager selFont] isMultiple:[manager isMultiple]];
		[[sharedFontPanel contentView] setAutoresizeSubviews:YES];
		[self setOneShot:NX_KITPANELSONESHOT];
		[self setBecomeKeyOnlyIfNeeded:YES];
		[self useOptimizedDrawing:NX_KITPANELSOPTIMIZED];
		if ([[self class] _readFrame:&frameRect fromDefaults:"NXFontPanel" domain:[NXApp appName]]) {
		    [self placeWindow:&frameRect];
		    [self windowDidResize:self];
		}
		if (gotpbframe) {
		    [_previewBox getFrame:&pbFrame];
		    delta = pframeRect.size.height - pbFrame.size.height;
		    [_chooser getFrame:&chFrame];
		    if (chFrame.size.height - delta < MIN_CHOOSER_SIZE) {
			delta = chFrame.size.height - MIN_CHOOSER_SIZE;
		    }
		    if (chFrame.size.height - delta >= MIN_CHOOSER_SIZE && pbFrame.size.height + delta >= 0.0) {
			chFrame.size.height -= delta;
			chFrame.origin.y += delta;
			pbFrame.size.height += delta;
			[_previewBox setFrame:&pbFrame];
			[_chooser setFrame:&chFrame];
		    }
		}
	    }
	}
    }

    return sharedFontPanel;
}

/* A tag encodes a FontFile and an offset in that file. tag = 0 is invalid. */

static int tagOf(id file, int offset)
{
    int tag = [fontFiles indexOf:file] + 1;

    if (tag > 0) {
	tag <<= 24;
	tag += offset;
    } else {
	tag = 0;
    }
    return tag;
}

static id fileOf(int tag)
{
    tag = ((tag & 037700000000) >> 24) - 1;
    if (tag >= 0 && tag < [fontFiles count]) {
	return[fontFiles objectAt:tag];
    } else {
	return nil;
    }
}

#define offsetOf(tag) (tag & 077777777)

/* Macros which get traits of a font given a tag */

#define weightOf(tag) [fileOf(tag) weight:offsetOf(tag)]

#define traitsOf(tag) [fileOf(tag) traits:offsetOf(tag)]

#define nameOf(tag) [fileOf(tag) name:offsetOf(tag)]

#define isItalic(tag) [fileOf(tag) isItalic:offsetOf(tag)]

/*
 * Selects a cell in one of the browsers that matches the passed parameter.
 * Returns 1 if the cell was selected, -1 if it is already selected
 */

static int fastSelectCell(id matrix, int row)
{
    int selRow;
    id selected;
    NXRect cFrame, selFrame;

    selRow = [matrix selectedRow];
    if (row != selRow) {
	[matrix scrollCellToVisible:row :0];
	if ([matrix canDraw]) {
	    [matrix lockFocus];
	    if (row >= 0) {
		[matrix getCellFrame:&cFrame at:row :0];
		NXHighlightRect(&cFrame);
		[[matrix cellAt:row :0] setEntryType:1];// hack, see TaggedCell
	    }
	    selected = [matrix selectedCell];
	    if (selected) {
		[matrix getCellFrame:&selFrame at:selRow :0];
		NXHighlightRect(&selFrame);
		[selected setEntryType:0];		// hack, see TaggedCell
	    }
	    [matrix unlockFocus];
	}
	[[matrix window] disableDisplay];
	[matrix selectCellAt:row :0];
	[[matrix window] reenableDisplay];
	return 1;
    } else {
	return -1;
    }
}

static BOOL selectSizeCell(id matrix, float size)
{
    int rows, cols;

    [matrix getNumRows:&rows numCols:&cols];
    while (rows--) {
	if (((float)[[matrix cellAt:rows :0] tag]) / 1000.0 == size) {
	    fastSelectCell(matrix, rows);
	    return YES;
	}
    }

    [matrix selectCellAt:-1:-1];

    return NO;
}

static BOOL selectFaceCell(id matrix, char *name)
{
    char *s;
    int rows, cols, tag;

    [matrix getNumRows:&rows numCols:&cols];
    while (rows--) {
	tag = [[matrix cellAt:rows :0] tag];
	s = nameOf(tag);
	if (s && !strcmp(name, s)) {
	    fastSelectCell(matrix, rows);
	    return YES;
	}
    }

    [matrix selectCellAt:-1:-1];

    return NO;
}

/*
 * Returns 0 if no such family, -1 if that is already the selected family
 * and 1 if a different family has been selected.
 */

static int selectFamilyCell(id matrix, const char *name)
{
    const char *s;
    int rows, cols;

    [matrix getNumRows:&rows numCols:&cols];
    while (rows--) {
	s = [[matrix cellAt:rows :0] stringValue];
	if (s && !strcmp(name, s)) {
	    return fastSelectCell(matrix, rows);
	}
    }

    [matrix selectCellAt:-1:-1];

    return 0;
}

/*
 * Used by the loadF* methods to sort the entries (by alpha or weight).
 * This function also eliminates duplicates (very important).
 * Assumes the matrix is sorted EXCEPT for the last item.
 */

static BOOL sortMatrix(id matrix, int count, BOOL sortByWeight)
{
    id  c, cell, cellList;
    const char *s = NULL, *t;
    int i, w, weight = NO, tag;

    cellList = [matrix cellList];
    cell = [cellList removeObjectAt:--count];
    t = [cell stringValue];

    if (t) {
	if (t[0] == 'I' && t[1] == 'T' && t[2] == 'C') {
	    t += 3 + ((t[3] == ' ') ? 1 : 0);
	}
	if (sortByWeight) {
	    for (i = 0; i < count; i++) {
		s = [[cellList objectAt:i] stringValue];
		if (s && !strcmp(s, t)) {
		    [cell free];
		    return NO;
		}
	    }
	    tag = [cell tag];
	    weight = weightOf(tag);
	}
	for (i = 0; i < count; i++) {
	    c = [cellList objectAt:i];
	    s = [c stringValue];
	    if (s) {
		if (s[0] == 'I' && s[1] == 'T' && s[2] == 'C') {
		    s += 3 + ((s[3] == ' ') ? 1 : 0);
		}
		if (!sortByWeight && strcmp(s, t) > 0) {
		    break;
		} else if (!strcmp(s, t)) {
		    [cell free];
		    return NO;
		} else if (sortByWeight) {
		    tag = [c tag];
		    w = weightOf(tag);
		    if (w > weight || (w == weight && isItalic(tag))) {
			break;
		    }
		}
	    }
	}
	[cellList insertObject:cell at:i];
    } else {
	[cell free];
	return NO;
    }

    return YES;
}

/* Loads the family and face matrices with info from one FontFile. */

static void loadFamilies(id matrix, id file)
{
    id cell;
    char *familyName;
    int i, rows, cols, count, tag;

    count = [file count];
    [matrix getNumRows:&rows numCols:&cols];
    for (i = 0; i < count; i++) {
	tag = tagOf(file, i);
	cell = [matrix findCellWithTag:tag];
	if (!cell) {
	    familyName = [file family:i];
	    if (familyName && strcmp(familyName, "Lexi")) {
		[matrix addRow];
		cell = [matrix cellAt:rows++:0];
		[cell setStringValueNoCopy:[file family:i]];
		[cell setTag:tag];
		if (!sortMatrix(matrix, rows, NO)) {
		    [matrix renewRows:--rows cols:1];
		}
	    }
	}
    }

    [matrix sizeToCells];
    [matrix display];
}

static void loadFaces(id matrix, const char *family, id file)
{
    id cell;
    int count, rows, cols;
    char *f, *targetFamily = NULL;

    if (family) {
	count = [file count];
	[matrix getNumRows:&rows numCols:&cols];
	while (count--) {
	    f = [file family:count];
	    if (f == targetFamily || f == family || !strcmp(f, family)) {
		targetFamily = f;
		[matrix addRow];
		cell = [matrix cellAt:rows++:0];
		[cell setStringValueNoCopy:[file face:count]];
		[cell setTag:tagOf(file, count)];
		[cell setEntryType:0];			// hack, see TaggedCell
		if (cell) {
		    _NXSetCellParam(cell, NX_CELLSTATE, 0);
		    _NXSetCellParam(cell, NX_CELLHIGHLIGHTED, 0);
		}
		if (!sortMatrix(matrix, rows, YES)) {
		    [matrix renewRows:--rows cols:1];
		}
	    }
	}
    }
}

/* Matrix loading methods */

- _loadSizes
{
    float aFloat;
    id cell;
    int i = 0, rows = 0;

    [self disableDisplay];
    while (standardSizes[i]) {
	[sizes addRow];
	cell = [sizes cellAt:rows++:0];
	[cell setStringValueNoCopy:standardSizes[i]];
	aFloat = atof(standardSizes[i]);
	[cell setTag:(int)(aFloat * 1000.0)];
	i++;
    }
    [self reenableDisplay];
    [sizes sizeToCells];
    [sizes clearSelectedCell];
    [sizes display];

    return self;
}

- _loadFamilies
{
    int i, count;

    [self disableDisplay];
    [[families renewRows:0 cols:1] clearSelectedCell];
    count = [fontFiles count];
    for (i = 0; i < count; i++) {
	loadFamilies(families, [fontFiles objectAt:i]);
    }
    [self reenableDisplay];
    [[[families superview] superview] display];

    return self;
}


- _loadFaces
{
    const char *family;
    int i, count, rows, cols, numRows, numCols;

    [self disableDisplay];
    family = [families stringValue];
    [faces getNumRows:&numRows numCols:&numCols];
    [faces renewRows:0 cols:1];
    if (family) {
	count = [fontFiles count];
	for (i = 0; i < count; i++) {
	    loadFaces(faces, family, [fontFiles objectAt:i]);
	}
    }
    [faces getNumRows:&rows numCols:&cols];
    [faces clearSelectedCell];
    [self reenableDisplay];
    [faces sizeToCells];
    [faces display];

    return self;
}

- _dontPreview
{
    fpFlags._dontPreview = YES;
    return self;
}

/* NIB outlet setting methods */

- _pushSet:sender
{
    [setButton performClick:sender];
    return self;
}

static id createScrollingMatrix(id scrollView, SEL action)
{
    id retval;
    NXSize cellSize;
    NXRect mFrame, sFrame;

    [scrollView getFrame:&sFrame];
    [scrollView setVertScrollerRequired:YES];
    [scrollView setBorderType:NX_BEZEL];
    mFrame.origin.x = mFrame.origin.y = 0.0;
    [ScrollView getContentSize:&mFrame.size
		  forFrameSize:&sFrame.size
		 horizScroller:NO
		  vertScroller:YES
		    borderType:NX_BEZEL];
    retval = [[Matrix allocFromZone:[scrollView zone]] initFrame:&mFrame
			mode:NX_RADIOMODE
		   cellClass:[TaggedCell class]
		     numRows:1
		     numCols:1];
    [retval allowEmptySel:YES];
    [retval setAction:action];
    [retval setDoubleAction:@selector(_pushSet:)];
    [retval setBackgroundGray:NX_LTGRAY];
    cellSize.width = cellSize.height = 0.0;
    [retval setIntercell:&cellSize];
    [retval sizeToFit];
    [retval getCellSize:&cellSize];
    cellSize.width = mFrame.size.width;
    [retval setCellSize:&cellSize];
    [retval renewRows:0 cols:1];
    [retval setAutoscroll:YES];
    [scrollView setDocView:retval];

    return retval;
}

- setFaces:anObject
{
    faces = createScrollingMatrix(anObject, @selector(_chooseFace:));
    [faces setTarget:self];
    return self;
}

- setFamilies:anObject
{
    families = createScrollingMatrix(anObject, @selector(_chooseFamily:));
    [families setTarget:self];
    return self;
}

- setSizes:anObject
{
    sizes = createScrollingMatrix(anObject, @selector(_chooseSize:));
    [sizes setTarget:self];
    [sizes setPrototype:[[[sizes cellList] removeObjectAt:0] setAlignment:NX_CENTERED]];
    [self _loadSizes];
    return self;
}

- _setupSplitView
{
    id splitView;
    NXRect vframe, svframe;

    [_previewBox getFrame:&vframe];
    [_chooser getFrame:&svframe];
    NXUnionRect(&vframe, &svframe);
    splitView = [[NXSplitView allocFromZone:[self zone]] initFrame:&svframe];
    [splitView setAutosizing:NX_WIDTHSIZABLE|NX_HEIGHTSIZABLE];
    [splitView setDelegate:self];
    [_previewBox getBounds:&vframe];
    [_previewBox convertPoint:&vframe.origin toView:splitView];
    [_previewBox moveTo:vframe.origin.x :vframe.origin.y];
    [splitView addSubview:_previewBox];
    [_chooser getBounds:&vframe];
    [_chooser convertPoint:&vframe.origin toView:splitView];
    [_chooser moveTo:vframe.origin.x :vframe.origin.y];
    [splitView addSubview:_chooser];
    [[self contentView] addSubview:splitView];

    return self;
}

- setChooser:anObject
{
    _chooser = anObject;
    if (_previewBox) [self _setupSplitView];
    return self;
}

- setPreviewBox:anObject
{
    _previewBox = anObject;
    if (_chooser) [self _setupSplitView];
    return self;
}
    
- setPreview:anObject
{
    preview = anObject;
    [[preview cell] setWrap:NO];
    return self;
}

- setCurrent:anObject
{
    current = anObject;
    [[current cell] _setCentered:YES];
    [current removeFromSuperview];
    return self;
}

- setFamilyTitle:anObject
{
    if (!_titles) _titles = [[List allocFromZone:[self zone]] initCount:2];
    [_titles insertObject:anObject at:0];
    return self;
}

- setFacesTitle:anObject
{
    if (!_titles) _titles = [[List allocFromZone:[self zone]] initCount:2];
    [_titles insertObject:anObject at:1];
    return self;
}

/* Determines the size of the font from the size field and fontObj's size */

- (float)_getSize:fontObj
{
    const char *s;
    float sizeValue;

    s = [size stringValue];
    sizeValue = [size floatValue];
    if (s && (*s == '+' || *s == '-')) {
	selectSizeCell(sizes, sizeValue);
	return[fontObj pointSize] + sizeValue;
    } else if (sizeValue) {
	selectSizeCell(sizes, sizeValue);
	return sizeValue;
    } else {
	return[fontObj pointSize];
    }
}

/* Updates the browsers to reflect fontObj */

- _reflectFont:fontObj metrics:(NXFontMetrics *)fm
{
    int result;
    char buf[12];
    BOOL canDraw;
    const char *s;
    NXSelPt start, end;
    float fontSize = [fontObj pointSize];

    if (fontObj && fm) {
	result = selectFamilyCell(families, fm->familyName);
	if (result) {
	    canDraw = [faces canDraw];
	    if (canDraw && result > 0) {
		[faces lockFocus];
	    }
	    if (result > 0) {
		[self _loadFaces];
	    }
	    selectFaceCell(faces, fm->name);
	    if (canDraw && result > 0) {
		[faces unlockFocus];
	    }
	} else {
	    [[faces renewRows:0 cols:1] sizeToCells];
	}
	selectSizeCell(sizes, fontSize);
	s = [size stringValue];
	if ([size floatValue] != fontSize) {
	    [size setFloatValue:fontSize];
	}
	if (firstResponder == fieldEditor) {
	    [fieldEditor getSel:&start :&end];
	    if (end.cp - start.cp < 12) {
		bzero(buf, 12);
		[fieldEditor getSubstring:buf
		    start:start.cp
		    length:end.cp - start.cp + 1];
		if (atof(buf) != fontSize) {
		    if ([self isKeyWindow]) [size selectText:self];
		}
	    } else {
		if ([self isKeyWindow]) [size selectText:self];
	    }
	} else {
	    if ([self isKeyWindow]) [size selectText:self];
	}
    } else {
	fastSelectCell(families, -1);
	[[[faces renewRows:0 cols:1] sizeToCells] clearSelectedCell];
	fastSelectCell(sizes, -1);
	s = [size stringValue];
	[size setStringValueNoCopy:NULL];
	if ([self isKeyWindow]) [size selectText:self];
    }

    return self;
}

/*
 * Looks in the passed in matrix to find the closest match to the font
 * specified by curTag in the specified family.  Returns the result via
 * theTag, and theFace.  theTag will be 0 if no match could be found.
 * This is called only if an exact match cannot be found.
 */

static void findClosestMatch(id matrix, int curTag, const char *family, int *theTag, id *theFace)
{
    int rows, cols;
    id face0, face1;
    NXFontTraitMask curTraits, traits0, traits1;

    curTraits = traitsOf(curTag);
    [matrix getNumRows:&rows numCols:&cols];
    if (rows == 1) {
	*theFace = [matrix cellAt:0 :0];
    } else if (curTraits & NX_NONSTANDARDCHARSET) {
	*theTag = _NXMatchFont(family, curTraits & (~NX_NONSTANDARDCHARSET),
			       weightOf(curTag));
	if (*theTag) {
	    *theFace = [matrix findCellWithTag:*theTag];
	}
    } else if (rows == 2) {
	face0 = [matrix cellAt:0 :0];
	face1 = [matrix cellAt:1 :0];
	traits0 = traitsOf([face0 tag]);
	traits1 = traitsOf([face1 tag]);
	if ((traits0 & NX_ITALIC) && !(traits1 & NX_ITALIC)) {
	    *theFace = (curTraits & NX_ITALIC) ? face0 : face1;
	} else if (!(traits0 & NX_ITALIC) && (traits1 & NX_ITALIC)) {
	    *theFace = (curTraits & NX_ITALIC) ? face1 : face0;
	} else if (curTraits & NX_UNBOLD) {
	    if ((traits0 & NX_UNBOLD) && !(traits1 & NX_UNBOLD)) {
		*theFace = face0;
	    } else if (!(traits0 & NX_UNBOLD) && (traits1 & NX_UNBOLD)) {
		*theFace = face1;
	    }
	} else if (curTraits & NX_BOLD) {
	    if ((traits0 & NX_BOLD) && !(traits1 & NX_BOLD)) {
		*theFace = face0;
	    } else if (!(traits0 & NX_BOLD) && (traits1 & NX_BOLD)) {
		*theFace = face1;
	    }
	}
    }
}

/* Updates the name of the currently selected font and (optionally) previews */

- _updatePreview:(BOOL)flag
{
    NXRect vbounds;
    float fontSize;
    id f = nil, face;
    NXFontMetrics *fm;
    int tag, textLength;
    char *s = NULL, *fontName;
    const char *text, *family;
    BOOL revertEnabled = YES, badFont = NO;
    char name[MAXPATHLEN+1];

    fontSize = [self _getSize:selFont];

    face = [faces selectedCell];
    family = [families stringValue];
    if (!face && !fpFlags.multipleFont && family) {
	tag = _NXMatchFamily(curTag, family);
	if (tag) {
	    face = [faces findCellWithTag:tag];
	} else {
	    findClosestMatch(faces, curTag, family, &tag, &face);
	}
	if (face) {
	    [faces selectCell:face];
	    if (tag) {
		curTag = tag;
	    } else {
		tag = [face tag];
	    }
	}
    } else if (!face && ((fpFlags.multipleFont && flag) || (!fpFlags.multipleFont && !family))) {
	if (family) {
	    f = [manager convert:selFont toFamily:family];
	    f = [Font newFont:[f name] size:fontSize];
	    fm = [f readMetrics:NX_FONTHEADER];
	} else {
	    fm = selMetrics;
	}
	tag = _NXFindFont(fm);
    } else {
	curTag = tag = [face tag];
    }

    if (tag) {
	fontName = nameOf(tag);
	f = [Font newFont:fontName size:fontSize];
	if (!f) {
	    tag = 0;
	    badFont = YES;
	} else {
	    s = [fileOf(tag) getFullName:name of:offsetOf(tag) withSize:fontSize];
	    revertEnabled = (f != selFont);
	}
    }

    if (!lastPreview) lastPreview = NX_ZONEMALLOC([self zone], lastPreview, char, 256);

    if (!fpFlags._dontPreview && tag && !fpFlags.multipleFont && (flag || (s && !strcmp(lastPreview, s) && [preview superview]))) {
	text = [preview stringValue];
	textLength = text ? strlen(text) : 0;
	if (textLength < 256) {
	    if (!textLength || !*text || !strcmp(text, lastPreview)) {
		if (f != [preview font]) {
		    [self disableDisplay];
		    [preview setStringValue:s];
		    [self reenableDisplay];
		} else {
		    [preview setStringValue:s];
		}
	    }
	    if (s && strlen(s) < 256) {
		strcpy(lastPreview, s);
	    } else if (s) {
		strncpy(lastPreview, s, 256);
		lastPreview[255] = '\0';
	    } else {
		lastPreview[0] = '\0';
	    }
	}
	[preview setFont:f];
	if (![preview superview]) {
	    [current getFrame:&vbounds];
	    [preview setFrame:&vbounds];
	    [[current superview] addSubview:preview];
	    [current removeFromSuperview];
	    [preview display];
	}
    } else {
	if (fpFlags.multipleFont) {
	    if (!multiFontMessage) {
		s = (char *)KitString(FontManager, "Multiple Font Selection", "Message given to user when a selection with multiple fonts is made.");
		multiFontMessage = (char *)NXZoneMalloc([self zone], strlen(s)+7);
		strcpy(multiFontMessage, "<< ");
		strcat(multiFontMessage, s);
		strcat(multiFontMessage, " >>");
	    }
	    [current setStringValueNoCopy:multiFontMessage];
	} else if (badFont) {
	    if (!badFontMessage) {
		s = (char *)KitString(FontManager, "Unusable Font", "Message given to user when the selected font is bad (not usuable).");
		badFontMessage = (char *)NXZoneMalloc([self zone], strlen(s)+7);
		strcpy(badFontMessage, "<< ");
		strcat(badFontMessage, s);
		strcat(badFontMessage, " >>");
	    }
	    [current setStringValueNoCopy:badFontMessage];
	} else {
	    [current setStringValue:s];
	}
	if (![current superview]) {
	    [preview getFrame:&vbounds];
	    [current setFrame:&vbounds];
	    [[preview superview] addSubview:current];
	    if ([[fieldEditor superview] superview] == preview) [size selectText:self];
	    [preview removeFromSuperview];
	    [current display];
	}
    }

    [[contentView findViewWithTag:NX_FPREVERTBUTTON] setEnabled:revertEnabled];

    return self;
}

/* Target/Action methods sent as a result of user selections */

- _chooseFamily:sender
{
    [self disableFlushWindow];
    [self _loadFaces];
    [self _updatePreview:NO];
    [self reenableFlushWindow];
    [self flushWindow];
    return self;
}

- _chooseFace:sender
{
    [self _updatePreview:NO];
    return self;
}

- _chooseSize:sender
{
    id cell;
    float tag;

    cell = [sender selectedCell];
    if (cell) {
	[self disableFlushWindow];
	tag = (float)[cell tag];
	[size setFloatValue:tag / 1000.0];
	if ([self isKeyWindow]) [size selectText:self];
	[self _updatePreview:NO];
	[self reenableFlushWindow];
	[self flushWindow];
    }
    return self;
}

- _preview:sender
{
    [self _updatePreview:YES];
    return self;
}

- _set:sender
{
    [self disableFlushWindow];
    [self _updatePreview:YES];
    [manager modifyFontViaPanel:sender];
    [self reenableFlushWindow];
    [self flushWindow];
    if ([self isKeyWindow]) {
	if ([NXApp _isRunningModal]) {
	    [(_NXLastModalSession ? _NXLastModalSession->window : nil) makeKeyWindow];
	} else {
	    [[NXApp mainWindow] makeKeyWindow];
	}
    }

    return self;
}

- _revert:sender
{
    [self disableFlushWindow];
    [self _reflectFont:(fpFlags.multipleFont ? nil : selFont) metrics :selMetrics];
    [self _updatePreview:YES];
    [self reenableFlushWindow];
    [self flushWindow];
    return self;
}

/* NXSplitView delegate method. */

- splitView:sender getMinY:(NXCoord *)minY maxY:(NXCoord *)maxY ofSubviewAt:(int)offset
{
    NXRect svbounds;

    [sender getBounds:&svbounds];
    *minY = 0.0;
    *maxY = svbounds.size.height - MIN_CHOOSER_SIZE;

    return self;
}

- splitViewDidResizeSubviews:sender
{
    NXRect pbframe;
    [_previewBox getFrame:&pbframe];
    [[self class] _writeFrame:&pbframe toDefaults:"NXFontPanelPreview" domain:[NXApp appName]];
    return self;
}

- splitView:sender resizeSubviews:(const NXSize *)oldSize
{
    NXRect cframe, pbframe;

    [sender adjustSubviews];
    [_chooser getFrame:&cframe];
    [_previewBox getFrame:&pbframe];
    if (cframe.size.height < MIN_CHOOSER_SIZE) {
	pbframe.size.height -= MIN_CHOOSER_SIZE - cframe.size.height;
	[_previewBox sizeTo:pbframe.size.width :pbframe.size.height];
	cframe.origin.y -= MIN_CHOOSER_SIZE - cframe.size.height;
	cframe.size.height = MIN_CHOOSER_SIZE;
	[_chooser setFrame:&cframe];
    }

    return self;
}

/* Public methods */

- accessoryView
{
    return accessoryView;
}

- setAccessoryView:aView
{
    id                  oldView;

    if (!accessoryView || accessoryView != aView) {
	oldView = [self _doSetAccessoryView:aView topView:[_chooser superview] bottomView:separator oldView:&accessoryView];
	[self windowDidResize:self];
	return oldView;
    } else {
	return aView;
    }
}

- textDidEnd:textObject endChar:(unsigned short)endChar
{
    selectSizeCell(sizes, [size floatValue]);
    [self _updatePreview:NO];
    return self;
}

- text:textObject isEmpty:(BOOL)flag
{
    if (flag && [sizes selectedCell]) {
	selectSizeCell(sizes, 0.0);
	[self _updatePreview:NO];
    }
    return self;
}

- textDidGetKeys:textObject isEmpty:(BOOL)flag
{
    return [self text:textObject isEmpty:flag];
}

- windowWillResize:sender toSize:(NXSize *)theSize
{
    NXRect avFrame;
    NXCoord minWidth, minHeight;

    minWidth = 250.0;
    minHeight = 200.0;
    if (accessoryView) {
	[accessoryView getFrame:&avFrame];
	minWidth = MAX(minWidth, avFrame.size.width);
	minHeight += avFrame.size.height;
    }
    if (theSize->width < minWidth) theSize->width = minWidth;
    if (theSize->height < minHeight) theSize->height = minHeight;

    return self;
}

static void adjustViewPairs(id left, id right, NXCoord rightEdge, BOOL updateCells)
{
    id matrix;
    NXSize csize, cellSize;
    NXRect leftFrame, rightFrame;

    [left getFrame:&leftFrame];
    [right getFrame:&rightFrame];
    leftFrame.size.width = rightEdge - leftFrame.origin.x;
    rightFrame.size.width = floor((leftFrame.size.width-4.0)/2.0);
    rightFrame.origin.x = leftFrame.origin.x + leftFrame.size.width - rightFrame.size.width;
    leftFrame.size.width = leftFrame.size.width - rightFrame.size.width - 4.0;
    [left setFrame:&leftFrame];
    [right setFrame:&rightFrame];
    if (updateCells) {
	[ScrollView getContentSize:&csize
			forFrameSize:&leftFrame.size
			horizScroller:NO
			vertScroller:YES
			borderType:NX_BEZEL];
	matrix = [left docView];
	[matrix getCellSize:&cellSize];
	cellSize.width = csize.width;
	[matrix setCellSize:&cellSize];
	[matrix sizeToCells];
	matrix = [right docView];
	cellSize.width += rightFrame.size.width - leftFrame.size.width;
	[matrix setCellSize:&cellSize];
	[matrix sizeToCells];
    }
}

- windowDidMove:sender
{
    NXRect aFrame, wFrame;

    if (accessoryView) {
	[accessoryView getFrame:&aFrame];
    } else {
	aFrame.size.height = 0.0;
    }
    [self getFrame:&wFrame];
    wFrame.size.height -= aFrame.size.height;
    [[self class] _writeFrame:&wFrame toDefaults:"NXFontPanel" domain:[NXApp appName]];

    return self;
}

- windowDidResize:sender
{
    NXRect aFrame;

    [sizeTitle getFrame:&aFrame];
    adjustViewPairs([[families superview] superview], [[faces superview] superview], aFrame.origin.x - 4.0, YES);
    adjustViewPairs([_titles objectAt:0], [_titles objectAt:1], aFrame.origin.x - 4.0, NO);

    return [self windowDidMove:sender];
}

- resignKeyWindow
{
    [self endEditingFor:self];
    return [super resignKeyWindow];
}

- orderWindow:(int)place relativeTo:(int)otherWin
{
    [super orderWindow:place relativeTo:otherWin];
    if (place == NX_ABOVE) {
	if (fpFlags.dirty) {
	    [self _revert:self];
	    fpFlags.dirty = NO;
	}
    }
    return self;
}

- setPanelFont:fontObj isMultiple:(BOOL)flag
{
    if (fontObj && (fpFlags.multipleFont != flag || selFont != fontObj)) {
	fpFlags.multipleFont = flag ? YES : NO;
	selFont = fontObj;
	selMetrics = [fontObj readMetrics:NX_FONTHEADER];
	if ([self isVisible]) {
	    [self _revert:self];
	} else {
	    fpFlags.dirty = YES;
	}
    }
    return self;
}

- panelConvertFont:fontObj
{
    int tag;
    char *face;
    float fontSize;
    id retval = nil;
    const char *family;

    fontSize = [self _getSize:fontObj];
    tag = [faces selectedTag];
    face = nameOf(tag);
    if (face) {
	retval = [Font newFont:face size:fontSize style:[fontObj style] matrix:[fontObj matrix]];
    } else {
	family = [families stringValue];
	if (family) {
	    retval = [manager convert:fontObj toFamily:family];
	    retval = [Font newFont:[retval name] size:fontSize style:[fontObj style] matrix:[fontObj matrix]];
	} else {
	    retval = [Font newFont:[fontObj name] size:fontSize style:[fontObj style] matrix:[fontObj matrix]];
	}
    }

    return retval ? retval : fontObj;
}


- (BOOL)worksWhenModal
{
    return YES;
}

- (BOOL)isEnabled
{
    return [setButton isEnabled];
}

- setEnabled:(BOOL)flag
{
    [setButton setEnabled:flag];
    return self;
}

@end

/*
  
Modifications (since 0.8):
  
 2/14/89 pah	implemented for the first time for 0.82
  
0.83
----
 3/08/89 pah	Take out debugging messages about font setting
 3/15/89 pah	Error handling code around runModalFor:
 3/17/89 pah	Add setAccessoryView: to allow customization of the panel

0.91
----
 5/12/89 pah	Optimize drawing performance
 5/22/89 pah	Add constrained resizing

0.93
----
 6/16/89 pah	add support for kit functions that now return const char *

0.94
----
 7/13/89 pah	no longer set tags programatically (set in IB)
 7/13/89 pah	screen Lexi out of the FontPanel

79
--
  4/3/90 pah	added support so that the FontPanel will run when the
		 application has a modal window up.  involves overriding
		 worksWhenModal and changing the key window deference
		 code so that it defers key to either the main window or
		 the frontmost modal window depending on whether we are
		 running modal.
		made the FontPanel BecomeKeyOnlyWhenHitAcceptsFirstResponder
85
--
 4/19/90 trey	nuked use of NXWait

86
--
 6/12/90 pah	Fixed panelConvertFont: to preserve Font's style and matrix

89
--
 7/19/90 pah	Changed scrolling lists from Browsers to ScrollView's of Matrix's

*/

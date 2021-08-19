/*
	textTabs.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Cell.h"
#import "Application.h"
#import "Text.h"
#import "textprivate.h"
#import <stdlib.h>
#import <string.h>
#import <objc/List.h>
#import "rtfstructs.h"
#import "rtfdefs.h"

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategoryTabs=0\n");
    asm(".globl .NXTextCategoryTabs\n");
#endif

typedef struct {
    @defs (Text)
} TextId;

typedef struct {
    int numTabs;
    NXTabStop *tabs;
} TabInfo;
extern NXHashTablePrototype tabProto, cellProto, dirProto, classProto;
static NXHashTable *viewTab = 0, *dirTab = 0, *classTab = 0;

@interface NXViewFrameCell:Cell
- setView:theView;
+ gViewFrameCell:view;
- readRichText:(NXStream *) stream forView:view;
- writeRichText:(NXStream *) stream forView:view;
@end

typedef struct {
    @defs(NXViewFrameCell)
} ViewFrameCell;

static NXViewFrameCell *gViewFrameCell = nil;

static int compareTabStops(const void *arg1, const void *arg2)
{
    const NXTabStop *p1 = arg1;
    const NXTabStop *p2 = arg2;
    if (p1->x < p2->x)
	return -1;
    else if (p1->x > p2->x)
	return 1;
    return 0; 
}

typedef struct {
    NXSymbolTable sym;
    id class;
} DirInfo;

@implementation Text (Tabs)
+ sortTabs:(NXTabStop *)tabs num:(int)numTabs
{
    qsort(tabs, numTabs, sizeof(NXTabStop), compareTabStops);
    return self;
}

@end

@implementation NXTabStopList

- init
{
    NXZone *zone = [self zone];
    maxTabs = 20;
    tabs = (NXTabStop *) NXZoneMalloc(zone, maxTabs * sizeof(NXTabStop));
    return self;
}

- addTabStop:(NXCoord) pos kind:(short)kind
{
    NXTabStop *tab;
    if (numTabs >= maxTabs) {
        NXZone *zone = [self zone];
	maxTabs *= 2;
	tabs = (NXTabStop *) NXZoneRealloc(
	    zone, tabs, maxTabs * sizeof(NXTabStop));
    }
    if (numTabs) {
	tab = tabs + numTabs - 1;
	if (tab->x < pos)
	    needSorting = YES;
    }
    tab = tabs + numTabs;
    tab->kind = kind;
    tab->x = pos;
    numTabs++;
    return self;
}

- (NXTabStop *)tabs
{
    if (needSorting) {
	needSorting = NO;
	qsort(tabs, numTabs, sizeof(NXTabStop), compareTabStops);
    }
    return tabs;
}

- empty
{
    numTabs = 0;
    needSorting = NO;
    return self;
}

- free
{
    free(tabs);
    return [super free];
}

@end

@implementation NXUniqueTabs

- (NXTabStop *)getTabs:(NXTabStop *)tabs num:(int)numTabs
{
    TabInfo temp, *result;
    int size;
    NXZone *zone = [self zone];
    
    if (!tabs || numTabs <= 0 )
	return NULL;
    if (!tabHash) {
	NXZone *zone = [self zone];
	tabHash = NXCreateHashTableFromZone(tabProto, 0, 0, zone);
    }
    temp.numTabs = numTabs;
    temp.tabs = tabs;
    result = NXHashGet(tabHash, &temp);
    if (result)
	return result->tabs;
    result = (TabInfo *)NXZoneMalloc(zone, sizeof(TabInfo));
    result->numTabs = numTabs;
    size = (sizeof(NXTabStop) * numTabs);
    result->tabs = (NXTabStop *) NXZoneMalloc (zone, size);
    bcopy(tabs, result->tabs, size);
    NXHashInsert(tabHash, result);
    return result->tabs;
}

- free
{
    if (tabHash)
	NXFreeHashTable(tabHash);
    return [super free];
}

@end


static unsigned tabHash(const void *info, const void *data)
{
    const TabInfo *tab = data;
    const unsigned char *first = (unsigned char *)tab->tabs;
    const unsigned char *last = first + (sizeof(NXTabStop) * tab->numTabs);
    unsigned hash = 0;
    while (first < last)
	hash += *first++;
    return hash;
}

static int tabEqual(const void *info, const void *data1, const void *data2)
{
    const TabInfo *tab1 = data1;
    const TabInfo *tab2 = data2;
    const unsigned char *first = (unsigned char *)tab1->tabs;
    const unsigned char *second = (unsigned char *)tab2->tabs;
    const unsigned char *last = first + (sizeof(NXTabStop) * tab1->numTabs);
    if (tab1->numTabs != tab2->numTabs)
	return 0;
    while (first < last) {
	if (*first++ != *second++)
	    return 0;
    }
    return 1;
}

static void tabFree(const void *info, void *data)
{
    TabInfo *tab = data;
    free(tab->tabs);
    free(tab);
}

static NXHashTablePrototype tabProto = {
    tabHash, tabEqual, tabFree, 0
};

@implementation NXViewFrameCell

- setView:theView
{
    support = theView;
    return self;
}

+ gViewFrameCell:view
{
    if (!gViewFrameCell)
	gViewFrameCell = [[self allocFromZone:[NXApp zone]] init];
    gViewFrameCell->support = view;
    return gViewFrameCell;
}

- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    NXRect rect;
    TextId *text = (TextId *)controlView;
    NXTextGlobals *globals = TEXTGLOBALS(text->_info);
    
    if (globals->gFlags.inDrawSelf)
	return self;
    [support getFrame:&rect];
    if (NX_X(&rect) != NX_X(cellFrame) || NX_Y(&rect) != NX_Y(cellFrame))
	[support moveTo:NX_X(cellFrame):NX_Y(cellFrame)];
    rect = *cellFrame;
    [support convertRect:&rect fromView:controlView];
    [support lockFocus];
    [support drawSelf:&rect:1];
    [support unlockFocus];
    return self;
}

- calcCellSize:(NXSize *) theSize
{
    NXRect rect;
    [support getFrame:&rect];
    *theSize = rect.size;
    return self;
}

- free
{
    NXHashRemove(viewTab, self);
    support = [support free];
    return [super free];
}

- readRichText:(NXStream *) stream forView:view
{
    if ([support respondsTo:@selector(readRichText:forView:)])
    	return([support  readRichText:stream forView:view]);
    return self;
}

- writeRichText:(NXStream *) stream forView:view
{
    if ([support respondsTo:@selector(writeRichText:forView:)])
    	return([support  writeRichText:stream forView:view]);
    return self;
}


@end

@implementation Text (Graphics)

- setLocation:(NXPoint *)origin ofCell:cell
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    if (globals->cellInfo) {
	NXTextCellInfo temp, *ptr;
	temp.cell = cell;
	ptr = NXHashGet(globals->cellInfo, &temp);
	if (ptr) {
	    ptr->origin = *origin;
	}
    }
    return self;
}
- replaceSelWithCell:cell
{
    NXLayInfo *info = (NXLayInfo *)_info;
    NXTextCache *cache = &info->cache;
    NXRun run;
    char *buf = "\254";
    NXRunArray insert;
    
    if (tFlags.monoFont)
	return self;
    NXAdjustTextCache(self, cache, sp0.cp);
    run = *cache->curRun;
    run.info = cell;
    run.rFlags.graphic = YES;
    run.chars = 1;
    insert.chunk.allocated = insert.chunk.used = sizeof(NXRun);
    insert.runs[0] = run;
    [self replaceSel:buf length:1 runs:&insert];
    return self;
}

- replaceSelWithView:view
{
    id cell = [[NXViewFrameCell allocFromZone:[self zone]] init];
    [cell setView:view];
    [view moveTo:1.0e30:1.0e30];
    [self addSubview:view];
    [self replaceSelWithCell:cell];
    if (!viewTab)
	viewTab = NXCreateHashTable(cellProto, 0, 0);
    NXHashInsert(viewTab, self);
    return self;
}

- getLocation:(NXPoint *)origin ofCell:cell
{
    NXTextGlobals *globals = TEXTGLOBALS(_info);
    NXTextCellInfo temp, *ptr;
    if (!globals->cellInfo)
	return nil;
    temp.cell = cell;
    ptr = NXHashGet(globals->cellInfo, &temp);
    if (!ptr)
	return nil;
    *origin = ptr->origin;
    return self;
}

- getLocation:(NXPoint *)origin ofView:view
{
    id cell;
    if (!viewTab)
	return nil;
    cell = NXHashGet(viewTab, [NXViewFrameCell gViewFrameCell:view]);
    if (!cell)
	return nil;
    return [self getLocation:origin ofCell:cell];
}

+ registerDirective:(const char *)directive forClass:class
{
    DirInfo temp, *ptr;
    if (!dirTab) 
	dirTab = NXCreateHashTable(dirProto, 0, 0);
    if (!classTab)
	classTab = NXCreateHashTable(classProto, 0, 0);
    temp.sym.name = (char *)directive;
    if (NXHashGet(dirTab, &temp))
	return nil;
    temp.class = class;
    if (NXHashGet(classTab, &temp))
	return nil;
    ptr = (DirInfo *)malloc(sizeof(DirInfo));
    ptr->sym.name = NXCopyStringBuffer(directive);
    ptr->sym.type = DEST;
    ptr->sym.value = SPECIAL_GROUP;
    ptr->class = class;
    NXHashInsert(dirTab, ptr);
    NXHashInsert(classTab, ptr);
    return self;
}

@end

@implementation Text(GraphicsSupport)
+ classForDirective:(const char *)directive
{
    DirInfo temp, *ptr;
    if (!dirTab)
	return nil;
    temp.sym.name = (char *) directive;
    ptr = NXHashGet(dirTab, &temp);
    if (!ptr)
	return nil;
    return ptr->class;
}

+ (const NXSymbolTable *) symbolForDirective:(const char *)directive
{
    DirInfo temp, *ptr;
    if (!dirTab)
	return NULL;
    temp.sym.name = (char *)directive;
    ptr = NXHashGet(dirTab, &temp);
    if (!ptr)
	return NULL;
    return &ptr->sym;
}

+ (const char *) directiveForClass:class;
{
    DirInfo temp, *ptr;
    if (!classTab)
	return NULL;
    temp.class = class;
    ptr = NXHashGet(classTab, &temp);
    if (!ptr)
	return NULL;
    return ptr->sym.name;
}

- readCellRichText:(NXStream *)stream sym:(NXSymbolTable *)st
{
    id class = [[self class] classForDirective:st->name];
    id cell;
    if (!class)
	return nil;
#ifdef BRYAN_SHOULD_CHECK_THIS
    cell = [[class allocFromZone:[self zone]] init];
#else
    cell = [class new];
#endif
    [cell readRichText:stream forView:self];
    return cell;
}

@end


static unsigned cellHash(const void *info, const void *data)
{
    const ViewFrameCell *cell = data;
    return NXPtrHash(info, cell->support);
}

static int cellEqual(const void *info, const void *data1, const void *data2)
{
    const ViewFrameCell *cell1 = data1;
    const ViewFrameCell *cell2 = data2;
    return NXPtrIsEqual(info, cell1->support, cell2->support);
}

static void cellFree(const void *info, void *data)
{
    
}

static NXHashTablePrototype cellProto = {
    cellHash, cellEqual, cellFree, 0
};

static unsigned dirHash(const void *info, const void *data)
{
    const DirInfo *dir = data;
    return NXStrHash(info, dir->sym.name);
}

static int dirEqual(const void *info, const void *data1, const void *data2)
{
    const DirInfo *dir1 = data1;
    const DirInfo *dir2 = data2;
    return NXStrIsEqual(info, dir1->sym.name, dir2->sym.name);
}

static void dirFree(const void *info, void *data)
{
    
}

static NXHashTablePrototype dirProto = {
    dirHash, dirEqual, dirFree, 0
};

static unsigned classHash(const void *info, const void *data)
{
    const DirInfo *class = data;
    return NXPtrHash(info, class->class);
}

static int classEqual(const void *info, const void *data1, const void *data2)
{
    const DirInfo *class1 = data1;
    const DirInfo *class2 = data2;
    return NXPtrIsEqual(info, class1->class, class2->class);
}

static void classFree(const void *info, void *data)
{
    
}

static NXHashTablePrototype classProto = {
    classHash, classEqual, classFree, 0
};


/***

91
--
 8/11/90 glc	Don't allow pasting of graphics in monofont text objects

92
--
 8/20/90 gcockrft  Added archiving support for NXViewFrameCell
 
93
--
 9/4/90 trey	fixed static topStopList to be alloc'ed in NXApp's zone

94
--
 9/25/90 gcockrft	- createCellForView:view  yanked.
 			if(!viewTab) fix in getLocation:ofView:

99
--
10/17/90 glc    Changes to support reentrant RTF. No more global list of 
		outstanding tabs.

****/
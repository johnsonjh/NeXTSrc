/*
	OpenPanel.m
  	Copyright 1988, 1989, 1990, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif

#import "appkitPrivate.h"
#import "SavePanel_Private.h"
#import "OpenPanel.h"
#import "NXBrowser.h"
#import <libc.h>

@implementation OpenPanel:SavePanel

/* NXDirEntry and NXFileFilterFunc are obsolete.  Do not use. */

typedef struct _NXDirEntry {
    struct _NXDirEntry *next;
    ino_t               ino;
    long                dirno;
    char                visible;
    char                document;
    unsigned short      mode;
    short               x;
    short               y;
    short               z;
    dev_t               dev;
    unsigned short      flags;
    char	        name[MAXPATHLEN+1];
} NXDirEntry;

typedef (*NXFileFilterFunc)(id self, NXDirEntry *dirEntry);

static id sharedOpenPanel = nil;

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
    if (!sharedOpenPanel) {
	[super _dontCache];
	self = [super newContent:contentRect
			    style:aStyle
			  backing:bufferingType
		       buttonMask:mask
			    defer:flag];
	if (self) {
	    spFlags.opening = YES;
	    [[[self contentView] findViewWithTag:NX_OPTITLEFIELD] setStringValueNoCopy: KitString(SavePanel, "Open", "The title of the OpenPanel.")];
	    sharedOpenPanel = self;
	}
    } else {
	self = sharedOpenPanel;
    }

    return self;
}

static void freeFilterTypes(char **filterTypes)
{
    char **start;

    if (filterTypes) {
 	start = filterTypes;
	while (*filterTypes) {
	    free(*filterTypes);
	    filterTypes++;
	}
	free(start);
    }
}

- _free
{
    sharedOpenPanel = nil;
    return self;
}

- free
{
    freeFilterTypes(filterTypes);
    return [super free];
}

- (char **)_filterTypes
{
    return filterTypes;
}

- allowMultipleFiles:(BOOL)flag
{
    spFlags.allowMultiple = flag ? YES : NO;
    [browser allowMultiSel:spFlags.allowMultiple];
    return self;
}

- (const char *const *)filenames
{
    return (const char *const *)filenames;
}

- setFileFilterFunc:(NXFileFilterFunc)aFunc
{
    _reservedPtr0 = (void *)aFunc;
    return self;
}

- (NXFileFilterFunc)fileFilterFunc
{
    return (NXFileFilterFunc)_reservedPtr0;
}

- (int)runModalForDirectory:(const char *)path
    file:(const char *)name
    types:(const char *const *)fileTypes
{
    int i, count = 0;
    const char *const *ft;

    if (fileTypes && *fileTypes) {
	ft = fileTypes;
	while (*ft) {
	    count++;
	    ft++;
	}
	if (!filterTypes) {
	    spFlags.invalidateMatrices = YES;
	} else {
	    i = 0;
	    while (!spFlags.invalidateMatrices && i < count) {
		if (!filterTypes[i] || strcmp(filterTypes[i], fileTypes[i])) {
		    spFlags.invalidateMatrices = YES;
		}
		i++;
	    }
	    if (!spFlags.invalidateMatrices && filterTypes[i]) {
		spFlags.invalidateMatrices = YES;
	    }
	}
	if (spFlags.invalidateMatrices) {
	    freeFilterTypes(filterTypes);
	    filterTypes = (char **)NXZoneMalloc([self zone], (count+1) * sizeof(char *));
	    for (i = 0; i < count; i++) {
		filterTypes[i] = NXCopyStringBufferFromZone(fileTypes[i], [self zone]);
	    }
	    filterTypes[i] = NULL;
	}
    } else {
	spFlags.invalidateMatrices = (filterTypes != NULL);
	freeFilterTypes(filterTypes);
	filterTypes = NULL;
    }

    spFlags.filtered = (filterTypes != NULL);

    return [super runModalForDirectory:path file:name];
}

- (int)runModalForTypes:(const char *const *)fileTypes
{
    return [self runModalForDirectory:directory file:NULL types:fileTypes];
}

- (int)runModalForDirectory:(const char *)path file:(const char *)name
{
    spFlags.invalidateMatrices = filterTypes ? YES : NO;
    freeFilterTypes(filterTypes);
    filterTypes = NULL;
    return [super runModalForDirectory:path file:name];
}

- (BOOL)_checkType:(const char *)file
{
    BOOL allowNoSuffix;
    const char *suffix;
    const char *const *types;

    if (!filterTypes || !*filterTypes) return YES;

    types = (char **)filterTypes;
    allowNoSuffix = !**types;
    if (allowNoSuffix) types++;

    suffix = _NXGetFileSuffix(file);
    if (!suffix) {
	return allowNoSuffix;
    } else {
	while (*types) {
	    if (!strcmp(*types, suffix)) {
		return YES;
	    } else {
		types++;
	    }
	}
    }

    return NO;
}

- (BOOL)_checkFile:(const char *)file exclude:(NXHashTable *)hashTable column:(int)column directory:(const char *)path leaf:(int)leafStatus
{
    NXDirEntry de;
    struct stat st;
    const char *suffix;
    const char *const *types;
    BOOL allowNoSuffix, isAutomount, isOtherwiseOk;

    isOtherwiseOk = [super _checkFile:file exclude:hashTable column:column directory:path leaf:leafStatus];
    if (!isOtherwiseOk || !filterTypes || !*filterTypes) return isOtherwiseOk;

    types = (char **)filterTypes;
    allowNoSuffix = !**types;
    if (allowNoSuffix) types++;

    suffix = _NXGetFileSuffix(file);
    if (!suffix) {
	if (!allowNoSuffix) {
	    if (leafStatus >= 0) {
		return !leafStatus;
	    } else if ([self _stat:&st file:file inColumn:column isAutomount:&isAutomount]) {
		return isAutomount || ((st.st_mode & S_IFMT) == S_IFDIR);
	    } else {
		return NO;
	    }
	}
    } else {
	while (*types) {
	    if (!strcmp(*types, suffix)) {
		if (_reservedPtr0) {
		    [self _stat:&st file:file inColumn:column isAutomount:&isAutomount];
		    de.next = NULL;
		    de.ino = st.st_ino;
		    de.dirno = 0;
		    de.visible = YES;
		    de.document = NXHashMember(_typeTable, suffix);
		    de.mode = st.st_mode;
		    de.x = de.y = de.z = 0;
		    de.dev = st.st_dev;
		    de.flags = 0;
		    strcpy(de.name, file);
		    return (*((NXFileFilterFunc)_reservedPtr0))(self, &de); 
		}
		return YES;
	    } else {
		types++;
	    }
	}
	if (_typeTable && NXHashMember(_typeTable, suffix)) return NO;
	if (!allowNoSuffix) {
	    if (leafStatus >= 0) {
		return !leafStatus;
	    } else if ([self _stat:&st file:file inColumn:column isAutomount:&isAutomount]) {
		return isAutomount || ((st.st_mode & S_IFMT) == S_IFDIR);
	    } else {
		return NO;
	    }
	}
    }

    return YES;
}

@end

/*
  
Modifications (since 0.8):
  
 2/14/89 pah	completely rewritten for 0.82
		allow multiple files to be selected at once
  
0.83
----
 3/09/89 pah	Notify user when invalid file is specified
 3/10/89 pah	Don't show file packages unless we are running for that type
 3/11/89 pah	Get rid of filterTypes and setFilterTypes:
 3/15/89 pah	Start in current directory (not ~)
 3/17/89 pah	Error handling around runModalFor:
		Add setAccessoryView: to allow customization of the panel

0.91
----
 5/23/89 pah	fix up relation between SavePanel and OpenPanel (inherit more)

84
--
  5/6/90 pah	changes to reflect rewritten SavePanel

*/

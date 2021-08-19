/*
	SavePanel.m
	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif

#import "appkitPrivate.h"
#import "SavePanel_Private.h"
#import "Application_Private.h"
#import "Panel_Private.h"
#import "Matrix_Private.h"
#import "Window_Private.h"
#import "View_Private.h"
#import "OpenPanel.h"
#import "Button.h"
#import "NXBrowser.h"
#import "NXBrowserCell.h"
#import "NXImage.h"
#import "Form.h"
#import "Speaker.h"
#import "Listener.h"
#import "QueryText.h"
#import <defaults.h>
#import "errors.h"
#import "nextstd.h"
#import "perfTimer.h"
#import "publicWraps.h"
#import "pbs.h"
#import <dpsclient/dpsNeXT.h>
#import <streams/streams.h>
#import <mach.h>
#import <pwd.h>
#import <libc.h>
#import <NXCType.h>
#import <sys/time.h>
#import <sys/stat.h>
#import <sys/dir.h>
#import <sys/dir.h>
#import <sys/errno.h>
#import <sys/time_stamp.h>

@interface NXBrowser(Timing)
- _doTiming;
- _unhookSubviews;
- _reattachSubviews:svs;
@end

@implementation SavePanel

#define MAX_COLUMNS 127
#define HIDDENFILE ".hidden"

/* Class variables */

static id           sharedSavePanel = nil;
static id           savePanelFactory = nil;
static BOOL	    savePanelTimings = NO;
static BOOL	    savePanelDebug = NO;
static BOOL	    dontCache = NO;

#ifdef AKDEBUG

static int _stat(const char *path, struct stat *st, BOOL l)
{
    if (!savePanelTimings && savePanelDebug) fprintf(stderr, "%s(%s)\n", l ? "lstat" : "stat", path);
    return l ? lstat(path, st) : stat(path, st);
}

static int _chdir(const char *path, BOOL getit)
{
    if (!savePanelTimings && savePanelDebug) fprintf(stderr, "%s(%s)\n", getit ? "getwd" : "chdir", path);
    return getit ? (int)getwd((char *)path) : chdir(path);
}

#define STAT(path, st) _stat(path, st, NO)
#define LSTAT(path, st) _stat(path, st, YES)
#define CHDIR(path) _chdir(path, NO)
#define GETWD(buffer) _chdir(buffer, YES)

#else

#define STAT(path, st) stat(path, st)
#define LSTAT(path, st) lstat(path, st)
#define CHDIR(path) chdir(path)
#define GETWD(buffer) getwd(buffer)

#endif

/* Factory methods */

+ _dontCache
{
    dontCache = YES;
    return self;
}

+ (BOOL)_canAlloc { return savePanelFactory != nil; }

/* If someone just calls this out of the blue, its an error.  They must use a new method.  Else we're being called as part of nib loading, and we do a special hack to make sure an object of the right type is created in spite of the class stored in 
the nib file. */
+ allocFromZone:(NXZone *)zone
{
    if (savePanelFactory)
	return _NXCallPanelSuperFromZone(savePanelFactory, _cmd, zone);
    else
	return [self doesNotRecognize:_cmd];
}

/* If someone just calls this out of the blue, its an error.  They must use a new method.  We depend on nibInstantiate using allocFromZone:. */
+ alloc
{
    return [self doesNotRecognize:_cmd];
}

+ newContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
{
    NXRect frameRect;
    char currentDirectory[MAXPATHLEN+1];

    if (!sharedSavePanel || dontCache) {
	if (NXGetDefaultValue(_NXAppKitDomainName, "SavePanelTiming")) savePanelTimings = YES;
	if (NXGetDefaultValue(_NXAppKitDomainName, "SavePanelDebug")) savePanelDebug = YES;
	if (savePanelFactory) {
	    return _NXCallPanelSuper(savePanelFactory,
		      @selector(newContent:style:backing:buttonMask:defer:),
			    contentRect, aStyle, bufferingType, mask, flag);
	} else {
	    savePanelFactory = self;
	    self = _NXLoadNibPanel("SavePanel");
	    savePanelFactory = nil;
	    if (self) {
		GETWD(currentDirectory);
		directorySize = strlen(currentDirectory);
		directory = strcpy((char *)NXZoneMalloc([self zone], directorySize+1), currentDirectory);
		_cdcolumn = -1;
		[[self contentView] setAutoresizeSubviews:YES];
		[self setOneShot:YES];
		[self useOptimizedDrawing:NX_KITPANELSOPTIMIZED];
		if ([[self class] _readFrame:&frameRect fromDefaults:([self isKindOf:[OpenPanel class]] ? "OpenPanel" : "SavePanel") domain:NXSystemDomainName]) {
		    [Window getContentRect:&frameRect forFrameRect:&frameRect style:[self style]];
		    [self sizeWindow:frameRect.size.width :frameRect.size.height];
		}
		[[[self contentView] findViewWithTag:NX_SPTITLEFIELD] setStringValueNoCopy:KitString(SavePanel, "Save", "Title of the SavePanel.")];
	    } else {
		dontCache = NO;
		return nil;
	    }
	}
	if (!dontCache) {
	    sharedSavePanel = self;
	}
    } else {
	self = sharedSavePanel;
    }

    dontCache = NO;

    return self;
}

/* NIB functions. */

- setBrowser:anObject
{
    browser = anObject;
    if (savePanelTimings) [browser _doTiming];
    [browser setTag:NX_SPBROWSER];
    [browser setDoubleAction:@selector(_doubleHit:)];
    [browser setAction:@selector(_itemHit:)];
    [browser useScrollButtons:YES];
    return self;
}

- setForm:anObject
{
    form = anObject;
    [form setPreviousText:self];
    fieldEditor = [[QueryText allocFromZone:[self zone]] init];
    [fieldEditor setCompletionEnabled:YES];
    [fieldEditor setFontPanelEnabled:NO];
    return self;
}

- setOkButton:anObject;
{
    okButton = anObject;
    [okButton setEnabled:NO];
    return self;
}

- setHomeButton:anObject
{
    _homeButton = anObject;
    return self;
}

- setIconButton:anObject
{
    [anObject setIcon:"app"];
    return self;
}

/* Private C functions. */

static BOOL checkCase()
/*
 * Returns whether browser should sort case sensitively.
 */
{
    const char *s;
    const char *name;

    name = [NXApp appName];
    if (!(s = NXUpdateDefault(name, "NXCaseSensitiveBrowser"))) {
	s = NXGetDefaultValue(name, "NXCaseSensitiveBrowser");
    }
    if (!s) {
	return NO;
    } else if (s[0] == '0') {
	return s[1] ? YES : NO;
    } else if (s[0] == 'n' || s[0] == 'N') {
	if (s[1] == 'o' || s[1] == 'O') {
	    return s[2] ? YES : NO;
	}
    } else {
	return YES;
    }

    return NO;
}

const char *_NXGetFileSuffix(const char *pname)
{
    const char *suffix = NULL;
    if (pname) while (*pname) if (*pname++ == '.') suffix = pname;
    return suffix ? (*suffix ? suffix : NULL) : NULL;
}

static unsigned NXCRStrHash(const void *info, const void *data)
{
    char *s, *nl, *sdata;
    char string[MAXPATHLEN+1];

    sdata = (char *)data;
    nl = strchr(sdata, '\n');
    if (nl) {
	s = strncpy(string, sdata, nl-sdata+1);
	*(strchr(s, '\n')) = '\0';
    } else {
	s = sdata;
    }

    return NXStrHash(info, s);
}

static int NXCRStringIsEqual(const void *info, const void *data1, const void *data2)
{
    const char *s1 = data1;
    const char *s2 = data2;

    if (!s1 && !s2) return YES;
    if (!s1 || !s2) return NO;
    while (*s1 && *s1 != '\n' && *s2 && *s2 != '\n' && *s1 == *s2) {
	s1++; s2++;
    }

    return (*s1 == *s2 || (*s1 == '\n' && !*s2) || (*s2 == '\n' && !*s1));
}

static NXHashTablePrototype NXCRStringPrototype;

static int dostrcmp(const char *const *arg1, const char *const *arg2, BOOL cs)
{
    const char *s1;
    const char *s2;

    if (!arg1 && !arg2) return 0;
    s1 = *arg1; s2 = *arg2;
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    if (*s1 == '.' && *s1 != *s2) {
	return 1;
    } else if (*s2 == '.' && *s1 != *s2) {
	return -1;
    } else {
	return NXOrderStrings((unsigned char *)s1, (unsigned char *)s2, cs, -1, NULL);
    }
}

static int csstrcmp(const void *arg1, const void *arg2)
{
    return dostrcmp(arg1, arg2, YES);
}

static int cistrcmp(const void *arg1, const void *arg2)
{
    return dostrcmp(arg1, arg2, NO);
}

#define CHUNK 127

static char **addFile(NXZone *zone, const char *file, int length, char **files, int count)
{
    if (!files) files = (char **)NXZoneMalloc(zone, CHUNK*sizeof(char *));
    files[count] = (char *)NXZoneMalloc(zone, (length+1)*sizeof(char));
    strcpy(files[count], file);
    count++;
    if (!((count+4) % CHUNK)) {
	files = (char **)NXZoneRealloc(zone, files, ((((count+4)/CHUNK)+1)*CHUNK)*sizeof(char *));
    }
    files[count] = NULL;
    return files;
}

static void freeFiles(_NXDirInfo dirInfo)
{
    char **strings, **files;

    files = dirInfo ? dirInfo->files : NULL;
    if (files) {
	strings = files;
	while (*strings) free(*strings++);
	free(files);
	dirInfo->files = NULL;
    }
}

/* Browser delegate methods. */

- (BOOL)browser:sender columnIsValid:(int)column
{
    struct stat st;
    BOOL isAutomount;

    if (_columns[column] && [self _stat:&st file:"." inColumn:column isAutomount:&isAutomount]) {
	return ((isAutomount || ((st.st_mode & S_IFMT) == S_IFDIR)) && (st.st_mtime == _columns[column]->mtime));
    }

    return NO;
}

- (const char *)browser:sender titleOfColumn:(int)column
{
    return (column ? "" : "/");
}

- (BOOL)_checkType:(const char *)file
{
    return YES;
}

- (BOOL)_checkFile:(const char *)file exclude:(NXHashTable *)hashTable column:(int)column directory:(const char *)path leaf:(int)leafStatus
/*
 * Returns YES if the file is okay to include.
 */
{
    return ((file && *file &&
	    !((file[0] == '.' && (!file[1] || (file[1] == '.' && !file[2]))) ||
	      (!spFlags._UnixExpert && file[0] == '.') ||
	      (hashTable && NXHashGet(hashTable, file)))) &&
	      (!path || !_filterMethod || _filterMethod(delegate, @selector(panel:filterFile:inDirectory:), self, file, path)));
}

#ifdef AKDEBUG

static unsigned statTicks = 0;
static unsigned gdeTicks = 0;

- _resetTimers
{
    statTicks = 0;
    gdeTicks = 0;
    return self;
}

- _stopTimers:(const char *)func
{
    if (statTicks || gdeTicks) {
	printf("%s: elapsed time in stat() = %d ms, in getdirentries() = %d ms.\n", func, statTicks/1000, gdeTicks/1000);
    }
    return self;
}

- display
{
    if (savePanelTimings) [self _resetTimers];
    [super display];
    if (savePanelTimings) [self _stopTimers:"display"];
    return self;
}

#endif

- (int)browser:sender getNumRowsInColumn:(int)column;
{
    long basep;
    struct stat st;
    char *buf, *path;
    struct direct *dp;
    _NXDirInfo dirInfo;
    char **files = NULL;
    int leafStatus, cc, fd, fileCount = 0;
    NXStream *stream;
    int length = 0, maxlen = 0;
    NXHashTable *hashTable = NULL;
    char *file, *hiddenFiles = NULL;
    BOOL isAutomount;
    char pathbuf[MAXPATHLEN+1];
    char dirbuf[vm_page_size];
#ifdef AKDEBUG
    struct tsval start, end;
#endif

    if (column > MAX_COLUMNS) return 0;
    if (!_columns) _columns = (_NXDirInfo *)NXZoneCalloc([self zone], MAX_COLUMNS, sizeof(_NXDirInfo));
    if (!_columns) return 0;

    if (_columns[column]) {
	dirInfo = _columns[column];
	freeFiles(dirInfo);
	if (dirInfo) {
	    free(dirInfo->path);
	    dirInfo->path = NULL;
	}
    } else {
	dirInfo = _columns[column] = (_NXDirInfo)NXZoneMalloc([self zone], sizeof(struct __NXDirInfo));
    }

    if (!dirInfo) return 0;

    if (column > 0) {
	path = [sender getPath:pathbuf toColumn:column];
	NX_ASSERT((path != NULL), "SavePanel error in getting path from the NXBrowser");
	if (!path) return 0;
    } else {
	path = strcpy(pathbuf, "/");
    }

    dirInfo->path = NXCopyStringBufferFromZone(path, [self zone]);
    if (_cdcolumn == column) _cdcolumn = -1;

    fd = open(path, O_RDONLY, 0644);
    if (fd > 0 &&
	[self _stat:&st file:"." inColumn:column isAutomount:&isAutomount] &&
	(isAutomount || (st.st_mode & S_IFMT) == S_IFDIR)) {
#ifdef AKDEBUG
	if (savePanelTimings) kern_timestamp(&start);
#endif
	cc = getdirentries(fd, (buf = dirbuf), 8192, &basep);
#ifdef AKDEBUG
	if (savePanelTimings) {
	    kern_timestamp(&end);
	    gdeTicks += end.low_val - start.low_val;
	}
#endif
	if (cc) {
	    strcat(path, "/");
	    strcat(path, HIDDENFILE);
	    if (!spFlags._UnixExpert && (stream = NXMapFile(path, NX_READONLY))) {
		NXGetMemoryBuffer(stream, &hiddenFiles, &length, &maxlen);
		NXCloseMemory(stream, NX_SAVEBUFFER);
		NXCRStringPrototype.hash = NXCRStrHash;
		NXCRStringPrototype.isEqual = NXCRStringIsEqual;
		NXCRStringPrototype.free = NXNoEffectFree;
		NXCRStringPrototype.style = 0;
		if (hashTable = NXCreateHashTableFromZone(NXCRStringPrototype, (length >> 4), NULL, [self zone])) {
		    file = hiddenFiles;
		    while (file) {
			NXHashInsert(hashTable, file);
			file = strchr(file, '\n');
			if (file) file++;
		    }
		    NXHashInsert(hashTable, HIDDENFILE);
		}
	    }
	    *(strrchr(path, '/')) = '\0';
	    leafStatus = spFlags._largeFS ? NO : -1;
	    while (cc) {
		dp = (struct direct *)buf;
		if (dp->d_fileno && [self _checkFile:dp->d_name exclude:hashTable column:column directory:path leaf:leafStatus]) {
		    files = addFile([self zone], dp->d_name, dp->d_namlen, files, fileCount++);
		}
		buf += dp->d_reclen;
		if (buf >= dirbuf + cc) {
		    cc = getdirentries(fd, (buf = dirbuf), 8192, &basep);
		}
	    }
	    if (!spFlags._UnixExpert) {
		if (hashTable) {
		    NXEmptyHashTable(hashTable);
		    NXFreeHashTable(hashTable);
		}
		if (hiddenFiles && maxlen) vm_deallocate(task_self(), (vm_address_t)hiddenFiles, maxlen);
	    }
	}
	close(fd);
    }

    dirInfo->device = st.st_dev;
    dirInfo->inode = st.st_ino;
    dirInfo->mtime = st.st_mtime;
    dirInfo->files = files;
    dirInfo->count = fileCount;
    if (dirInfo->files && fileCount) {
	if (spFlags._checkCase) {
	    qsort(dirInfo->files, fileCount, sizeof(char *), csstrcmp);
	} else {
	    qsort(dirInfo->files, fileCount, sizeof(char *), cistrcmp);
	}
    }

    return fileCount;
}

- (char *)_getPathTo:(int)column into:(char *)path
{
    if (_columns && _columns[column] && _columns[column]->path && *(_columns[column]->path)) {
	return strcpy(path, _columns[column]->path);
    } else {
	return [browser getPath:path toColumn:column];
    }
}

- (BOOL)_stat:(struct stat *)statbuf file:(const char *)file inColumn:(int)column isAutomount:(BOOL *)isAutomount
/*
 * Returns NO if file doesn't exist, YES otherwise.
 */
{
    struct stat stbuf;
    struct stat *st = &stbuf;
    char path[MAXPATHLEN+1];
    BOOL automount, statOk = YES;
#ifdef AKDEBUG
    struct tsval start, end;
#endif

    if (!file) return NO;
    if (statbuf) st = statbuf;
    if (!isAutomount && !statbuf) isAutomount = &automount;

    *path = '\0';

    if (!spFlags._cancd || _cdcolumn != column) {
	[self _getPathTo:column into:path];
	strcat(path, "/");
	if (spFlags._cancd && !CHDIR(path)) {
	    *path = '\0';
	    _cdcolumn = column;
	}
    }

    strcat(path, file);

#ifdef AKDEBUG
    if (savePanelTimings) kern_timestamp(&start);
#endif

    if (isAutomount) {
	*isAutomount = NO;
	if (LSTAT(path, st) < 0) {
	    statOk = NO;
	} else if ((st->st_mode & S_IFMT) == S_IFLNK) {
	    if (st->st_mode & S_ISVTX) {
		*isAutomount = YES;
	    } else if (STAT(path, st) < 0) {
		statOk = NO;
	    }
	}
    } else if (STAT(path, st) < 0) {
	statOk = NO;
    }

#ifdef AKDEBUG
    if (savePanelTimings) {
	kern_timestamp(&end);
	statTicks += end.low_val - start.low_val;
    }
#endif

    return statOk;

}

- browser:sender loadCell:cell atRow:(int)row inColumn:(int)column
{
    char **files;
    struct stat st;
    BOOL isAutomount;
    const char *suffix;

    if (_columns && _columns[column] && (files = _columns[column]->files)) {
	[cell setStringValueNoCopy:files[row]];
	suffix = _NXGetFileSuffix([cell stringValue]);
	if (_typeTable && NXHashMember(_typeTable, suffix)) {
	    [cell setLeaf:YES];
	} else if (![self _stat:&st file:files[row] inColumn:column isAutomount:&isAutomount]) {
	    [cell setLeaf:YES];
	} else if (isAutomount) {
	    [cell setLeaf:NO];
	} else if ((st.st_mode & S_IFMT) != S_IFDIR) {
	    [cell setLeaf:YES];
	} else {
	    [cell setLeaf:NO];
	}
	if (spFlags._largeFS && [cell isLeaf]) {
	    [cell setEnabled:[self _checkFile:files[row] exclude:NULL column:column directory:NULL leaf:YES]];
	}
    }

    return self;
}


- (BOOL)browser:sender selectCell:(const char *)title inColumn:(int)column
{
    char **files;
    id theCell, matrix;
    _NXDirInfo dirInfo;
    struct stat st;
    BOOL isAutomount = NO;
    int rows, cols, count, cmp, row = 0, insertLocation = -1;

    if (title && column < MAX_COLUMNS) {
	dirInfo = (_columns ? _columns[column] : NULL);
	files = dirInfo ? dirInfo->files : NULL;
	if (files) {
	    while (files[row]) {
		cmp = dostrcmp(files+row, &title, YES);
		if (!cmp) {
		    [sender getLoadedCellAtRow:row inColumn:column];
		    matrix = [sender matrixInColumn:column];
		    [matrix _scrollRowToCenter:row];
		    if ([[matrix cellAt:row :0] isEnabled]) {
			[matrix selectCellAt:row :0];
			return YES;
		    } else {
			[matrix selectCellAt:-1 :-1];
			return NO;
		    }
		} else if (cmp > 0 && insertLocation < 0) {
		    insertLocation = row;
		}
		row++;
	    }
	    if (insertLocation < 0) insertLocation = row;
	} else {
	    insertLocation = 0;
	}
	if ([self _stat:&st file:title inColumn:column isAutomount:&isAutomount]) {
	    files = dirInfo->files = addFile([self zone], title, strlen(title), dirInfo->files, dirInfo->count);
	    if (files) {
		char *file = files[dirInfo->count];
		count = (dirInfo->count - insertLocation) * sizeof(char *);
		if (count) bcopy(files+insertLocation, files+insertLocation+1, count);
		files[insertLocation] = file;
		dirInfo->count++;
		matrix = [sender matrixInColumn:column];
		if (matrix) {
		    [self disableDisplay];
		    [matrix getNumRows:&rows numCols:&cols];
		    [matrix insertRowAt:insertLocation];
		    [matrix sizeToCells];
		    theCell = [matrix cellAt:insertLocation :0];
		    [theCell setStringValueNoCopy:files[insertLocation]];
		    [theCell setLeaf:!(isAutomount || ((st.st_mode & S_IFMT) == S_IFDIR))];
		    [matrix selectCellAt:insertLocation :0];
		    [matrix _scrollRowToCenter:insertLocation];
		    [self reenableDisplay];
		    [browser displayColumn:column];
		}
		return YES;
	    }
	}
    }

    [[sender matrixInColumn:column] selectCellAt:-1 :-1];

    return NO;
}

/* Public methods. */

- _free
{
    sharedSavePanel = nil;
    savePanelFactory = nil;
    return self;
}

- free
{
    int i;

    free(directory);
    free(filenames);
    free(filename);
    if (_columns) {
	for (i = 0; i < MAX_COLUMNS; i++) {
	    freeFiles(_columns[i]);
	    free(_columns[i]);
	}
	free(_columns);
    }
    if (_typeTable) NXFreeHashTable(_typeTable);
    [self _free];

    return [super free];
}

- _setDirectory:(const char *)path
{
    int pathLength;

    if (!path) return self;
    pathLength = strlen(path);
    if (pathLength <= directorySize && directory) {
	strcpy(directory, path);
    } else {
	free(directory);
	directory = NXCopyStringBufferFromZone(path, [self zone]);
	directorySize = pathLength;
    }

    return self;
}

- cancel:sender
{
    char path[MAXPATHLEN+1];

    spFlags.exitOk = NO;
    [self _getPathTo:[browser lastColumn] into:path];
    [self _setDirectory:path];
    [NXApp stopModal:NX_CANCELTAG];

    return self;
}

- (int)runModal
{
    return [self runModalForDirectory:directory file:NULL];
}

- (const char *)filename
{
    return filename;
}

- (const char *)directory
{
    return directory;
}

#ifndef SPECULATE

- (char *)dirPath
{
    return (char *)[self directory];
}

#endif !SPECULATE

- setDirectory:(const char *)path
{
    char pathbuf[MAXPATHLEN+1];

    if (path) {
	strcpy(pathbuf, path);
	path = ([self _cleanPath:pathbuf] ? pathbuf : path);
    }
    [self _setDirectory:path];
    if ([self isVisible]) [browser setPath:directory];

    return self;
}


- setPrompt:(const char *)prompt
{
    [form setTitle:prompt at:0];
    return self;
}

- setTitle:(const char *)title
{
    [[[self contentView] findViewWithTag:NX_SPTITLEFIELD] setStringValue:title];
    return self;
}

- (const char *)requiredFileType
{
    return (const char *)requiredType;
}

- setRequiredFileType:(const char *)type
{
    free(requiredType);
    requiredType = type ? NXCopyStringBufferFromZone(type, [self zone]) : NULL;
    if ([self isVisible]) {
	[self _indicatePrefix:[form stringValue]];
    }
    return self;
}

- accessoryView
{
    return accessoryView;
}

- setAccessoryView:aView
/*
 * The autosizing works a bit differently if you are
 * adding an accessory view than when you are resizing
 * via user action.
 */
{
    id oldView;

    if (!accessoryView || accessoryView != aView) {
	oldView = [self _doSetAccessoryView:aView topView:form bottomView:_homeButton oldView:&accessoryView];
	return oldView;
    } else {
	return aView;
    }
}

- _setForm:(const char *)string copy:(BOOL)copyString select:(BOOL)selectText ok:(BOOL)clickOk
{
    if (copyString && string && *string) {
	[form setStringValue:string];
    } else {
	[form setStringValueNoCopy:((string && *string) ? string : NULL)];
    }
    if (selectText && !clickOk) [form selectText:self];
    [okButton setEnabled:(string && *string)];
    if (clickOk) [okButton performClick:self];
    return self;
}

- selectText:sender
 /*
  * Called when TAB or SHIFT-TAB is hit in the form.  Based on the information
  * collected in textDidEnd:endChar:, we either go to the next entry or the
  * previous entry in the current directory (and wrap around if necessary).
  */
{
    BOOL cycled = NO;
    id matrix, cell = nil;
    int row, rows, cols;
    int increment = spFlags._backwards ? -1 : 1;

    matrix = [browser matrixInColumn:[browser lastColumn]];
    [matrix getNumRows:&rows numCols:&cols];
    row = [matrix selectedRow];

    if (row < 0) {
	row = spFlags._backwards ? rows-1 : 0;
    } else {
	row += increment;
	if (row < 0) row = rows-1;
	if (row >= rows) row = 0;
    }

    cell = [browser getLoadedCellAtRow:row inColumn:[browser lastColumn]];

    while (row >= 0 && row < rows && (!cell || ![cell isEnabled] || (!spFlags.opening && [cell isLeaf]))) {
	row += increment;
	if (!cycled) {
	    if (row >= rows) {
		cycled = YES;
		row = 0;
	    } else if (row < 0) {
		cycled = YES;
		row = rows - 1;
	    }
	}
	cell = (row >= 0 && row < rows) ? [browser getLoadedCellAtRow:row inColumn:[browser lastColumn]] : nil;
    }

    if (cell && (spFlags.opening || ![cell isLeaf])) {
	[matrix selectCellAt:row :0];
	[matrix scrollCellToVisible:row :0];
	[self _setForm:[cell stringValue] copy:NO select:NO ok:NO];
    }

    [form selectText:self];

    return self;
}

- _selectForm:sender
{
    if (![fieldEditor window]) [form selectText:self];
    return self;
}

- textDidEnd:textObj endChar:(unsigned short)endChar
 /*
  * Called whenever the user types TAB, SHIFT-TAB, or RETURN in the form.
  * All we do is record which was pressed so that we know whether to tab
  * in the browser (and which direction) or not.
  */
{
    if (endChar == NX_BACKTAB) {
	spFlags._forwards = NO;
	spFlags._backwards = YES;
    } else if (endChar == NX_TAB) {
	spFlags._forwards = YES;
	spFlags._backwards = NO;
    } else {
	spFlags._forwards = NO;
	spFlags._backwards = NO;
	if (![textObj textLength] && ![okButton isEnabled]) [self perform:@selector(_selectForm:) with:self afterDelay:1 cancelPrevious:YES];
    }
    return self;
}


- text:textObj isEmpty:(BOOL)isEmpty
 /*
  * Called every time a character is typed in the form.  We take this
  * opportunity to indicate if what the user has typed prefix matches
  * any of the files in the current directory as well as to update the
  * OK button so that it is disabled when there is no text in the form.
  */
{
    int textLength,btextLength;
    char textString[MAXPATHLEN+1];

    textLength = [textObj textLength];
    btextLength = [textObj byteLength];
    if (textLength && btextLength < MAXPATHLEN) {
	[okButton setEnabled:YES];
	[textObj getSubstring:textString start:0 length:textLength];
	textString[btextLength--] = '\0';
	[self _indicatePrefix:textString];
    } else {
	[okButton setEnabled:NO];
	if (textObj) [self _indicatePrefix:""];
    }

    return self;
}

- textDidGetKeys:textObj isEmpty:(BOOL)flag
{
    return [self text:textObj isEmpty:flag];
}

static BOOL clearSelectedCells(id matrix)
/*
 * Clears all highlighted cells in the matrix and clears the selectedCell.
 */
{
    int rows = 0, cols;
    id cell, window;
    BOOL gotOne = NO;

    window = [matrix window];
    [matrix getNumRows:&rows numCols:&cols];
    while (rows--) {
	cell = [matrix cellAt:rows :0];
	if ([cell isLoaded] && ([cell isHighlighted] || [cell state])) {
	    if (!gotOne) [window disableFlushWindow];
	    gotOne = YES;
	    _NXSetCellParam(cell, NX_CELLSTATE, 0);
	    _NXSetCellParam(cell, NX_CELLHIGHLIGHTED, 0);
	    [matrix drawCellAt:rows :0];
	}
    }
    if (gotOne) {
	[window reenableFlushWindow];
	[window flushWindow];
    }
    [matrix clearSelectedCell];

    return gotOne;
}

- findHome:sender
{
    const char *user;
    struct passwd *pw;

    user = (sender == self) ? NULL : [form stringValue];
    if (user && *user == '~') user++;
    if (!user || !*user || strchr(user, '/')) {
	clearSelectedCells([browser matrixInColumn:[browser lastColumn]]);
	[self _setForm:NXHomeDirectory() copy:NO select:NO ok:NO];
    } else {
	pw = getpwnam(user);
	clearSelectedCells([browser matrixInColumn:[browser lastColumn]]);
	if (pw) {
	    [self _setForm:pw->pw_dir copy:YES select:NO ok:NO];
	} else {
	    [self _setForm:NXHomeDirectory() copy:NO select:NO ok:NO];
	}
    }

    return [self ok:okButton];
}

static BOOL hasMultipleSelection(id matrix)
{
    id cell;
    int i, count, rows, cols;

    if (!matrix) return NO;
    [matrix getNumRows:&rows numCols:&cols];
    for (count = 0, i = 0; i < rows; i++) {
	cell = [matrix cellAt:i :0];
	if ([cell isHighlighted] || [cell state]) {
	    count++;
	    if (count > 1) return YES;
	}
    }

    return NO;
}

- (BOOL)commandKey:(NXEvent *)theEvent
 /*
  * Command-SPACE does filename completion.
  * Command-H goes back to the home directory.
  */
{
    if ([self isVisible]) {
	if (theEvent->data.key.charCode == 0x20 && [fieldEditor superview]) {
	    [fieldEditor _completeFileName];
	    return YES;
	} else if (theEvent->data.key.charCode == 'H') {
	    [_homeButton performClick:self];
	    return YES;
	}
    }
    return [super commandKey:theEvent];
}

- windowDidResize:sender
{
    NXRect aFrame, bFrame;

    if (accessoryView) {
	[accessoryView getFrame:&aFrame];
    } else {
	aFrame.size.height = 0.0;
    }
    [self getFrame:&bFrame];
    bFrame.size.height -= aFrame.size.height;

    return [[self class] _writeFrame:&bFrame toDefaults:([self isKindOf:[OpenPanel class]] ? "OpenPanel" : "SavePanel") domain:NXSystemDomainName];
}

- windowWillResize:sender toSize:(NXSize *)theSize
{
    NXRect avFrame;
    NXCoord minWidth, minHeight;

    minWidth = 250.0;
    minHeight = 250.0;
    if (accessoryView) {
	[accessoryView getFrame:&avFrame];
	minWidth = MAX(minWidth, avFrame.size.width);
	minHeight += avFrame.size.height;
    }
    if (theSize->width < minWidth) theSize->width = minWidth;
    if (theSize->height < minHeight) theSize->height = minHeight;

    return self;
}

#define UPARROW		173
#define DOWNARROW	175

- sendEvent:(NXEvent *)event
{
    if (event->type == NX_KEYDOWN && event->data.key.charSet == NX_SYMBOLSET) {
	switch (event->data.key.charCode) {
	    case UPARROW:
		spFlags._forwards = NO;
		spFlags._backwards = YES;
		[self selectText:self];
		return self;
	    case DOWNARROW:
		spFlags._forwards = YES;
		spFlags._backwards = NO;
		[self selectText:self];
		return self;
	}
    }
    return [super sendEvent:event];
}

- setDelegate:anObject
{
    if ([anObject respondsTo:@selector(panel:filterFile:inDirectory:)]) {
	_filterMethod = [anObject methodFor:@selector(panel:filterFile:inDirectory:)];
    } else {
	_filterMethod = NULL;
    }
    spFlags._delegateValidatesNew = [anObject respondsTo:@selector(panelValidateFilenames:)];
    spFlags._delegateValidatesOld = [anObject respondsTo:@selector(validateFilename:)];
    return [super setDelegate:anObject];
}

static NXHashTable *_NXBuildTypeTable(NXZone *zone)
{
    port_t server;
    char *end, *data;
    unsigned int length;
    NXHashTable *hashTable;

    server = _NXLookupAppkitServer(NULL, NULL, NULL);
    NX_DURING
	if (_NXLoadFileExtensions(server, &data, &length)) NX_VALRETURN(NULL);
    NX_HANDLER
	if (NXLocalHandler.code == NX_pasteboardComm && (int)NXLocalHandler.data1 == RCV_TIMED_OUT)
	    return NULL;
	else
	    NX_RERAISE();
    NX_ENDHANDLER
    if (!data || !length) return NULL;
    hashTable = NXCreateHashTableFromZone(NXStrPrototype, length >> 2, NULL, zone);
    if (!hashTable) return NULL;
    end = data+length;
    while (data < end && *data) {
	NXHashInsert(hashTable, data);
	data += strlen(data)+1;
    }

    return hashTable;
}

static BOOL checkDefault(const char *domain, const char *defName)
{
    const char *ue;
    if (!(ue = NXUpdateDefault(domain, defName))) ue = NXGetDefaultValue(domain, defName);
    return (ue && strlen(ue) == 3 &&
	(ue[0] == 'y' || ue[0] == 'Y') &&
	(ue[1] == 'e' || ue[1] == 'E') &&
	(ue[2] == 's' || ue[2] == 'S')) ? YES : NO;
}

- (BOOL)_initializePanel:(BOOL)display path:(const char *)path name:(const char *)name savedCWD:(char *)savedCWD
{
    int i;
    id svs = nil;
    BOOL savedCWDOk = NO;
    const char *defValue;
    NXRect frameRect, accFrame;
    char file[MAXPATHLEN+1];

    if (display) svs = [browser _unhookSubviews];

    if (!(defValue = NXUpdateDefault(NXSystemDomainName, "BrowserColumnWidth")))
	defValue = NXGetDefaultValue(NXSystemDomainName, "BrowserColumnWidth");
    i = defValue ? atoi(defValue) : 0;
    if (i > 50 && i != [browser minColumnWidth]) [browser setMinColumnWidth:i];
    if ([[self class] _readFrame:&frameRect fromDefaults:([self isKindOf:[OpenPanel class]] ? "OpenPanel" : "SavePanel") domain:NXSystemDomainName]) {
	[Window getContentRect:&frameRect forFrameRect:&frameRect style:[self style]];
	if (accessoryView) {
	    [accessoryView getFrame:&accFrame];
	} else {
	    accFrame.size.height = 0.0;
	}
	[self sizeWindow:frameRect.size.width :frameRect.size.height+accFrame.size.height];
    }

    if (display) [NXApp _orderFrontModalWindow:self];

    if (!_typeTable) _typeTable = _NXBuildTypeTable([self zone]);

    if (display) {
	[browser _reattachSubviews:svs];
	DPSFlush();
    }

    if (savedCWD) savedCWDOk = GETWD(savedCWD) ? YES : NO;
    spFlags._cancd = savedCWDOk;
    spFlags._checkCase = checkCase();

    if (checkDefault(NXSystemDomainName, "UnixExpert")) {
	if (!spFlags._UnixExpert) spFlags.invalidateMatrices = YES;
	spFlags._UnixExpert = YES;
    } else {
	if (spFlags._UnixExpert) spFlags.invalidateMatrices = YES;
	spFlags._UnixExpert = NO;
    }

    spFlags._largeFS = checkDefault(NXSystemDomainName, "LargeFileSystem") ? YES : NO;

    if ([browser delegate] != self) {
	[self disableDisplay];
	[browser setDelegate:self];
	[browser loadColumnZero];
	[self reenableDisplay];
    }

    if (path && strlen(path)) {
	strcpy(file, path);
    } else {
	strcpy(file, directory);
    }
    if (name) {
	strcat(file, "/");
        strcat(file, name);
    }

    if (spFlags.invalidateMatrices) {
	if (_columns) {
	    i = [browser lastColumn];
	    i = MIN(i, MAX_COLUMNS);
	    while (i >= 0) {
		if (_columns[i]) _columns[i]->mtime = 0;
		i--;
	    }
	}
	spFlags.invalidateMatrices = NO;
    }

    MARKTIME(savePanelTimings, "Cleaning path name.", 1);
    if (![self _cleanPath:file]) *file = '\0';
    MARKTIME(savePanelTimings, "Setting path in NXBrowser.", 1);
    [self disableDisplay];
    [browser setPath:(*file ? file : directory)];
    [self reenableDisplay];
    [browser displayAllColumns];
    MARKTIME(savePanelTimings, "Validating visible columns.", 1);
    [browser validateVisibleColumns];
    DUMPTIMES(savePanelTimings);
    CLEARTIMES(savePanelTimings);

    return savedCWDOk;
}

- (int)runModalForDirectory:(const char *)path file:(const char *)name
 /*
  * Entry point for all runModals (both for OpenPanel and SavePanel).
  */
{
    volatile NXHandler handler;
    volatile int retval = 0;
    volatile BOOL savedCWDOk;
    char savedCWD[MAXPATHLEN+1];

    [form setStringValue:NULL];
    savedCWDOk = [self _initializePanel:YES path:path name:name savedCWD:savedCWD];
    [self _setForm:name copy:YES select:YES ok:NO];

    NXPing();
    NX_DURING {
	handler.code = 0;
	retval = [NXApp runModalFor:self];
    } NX_HANDLER {
	handler = NXLocalHandler;
	if (handler.code == dps_err_ps) {
	    NXReportError((NXHandler *)(&handler));
	}
    } NX_ENDHANDLER

    [self orderOut:self];

    if (savedCWDOk) {
	CHDIR((char *)savedCWD);
	_cdcolumn = -1;
	spFlags._cancd = NO;
    }

    if (handler.code && handler.code != dps_err_ps) {
	NX_RAISE(handler.code, handler.data1, handler.data2);
    }

    return retval;
}

/* End of Public Methods */

static BOOL alllc(const char *s)
{
    while (s && *s) {
	if (!NXIsLower(*s)) return NO;
	s++;
    }
    return YES;
}

static char *prefixMatch(const char *prefix, const char *word, BOOL checkCase, BOOL ignoreCase)
 /*
  * If prefix is a prefix of word, then the part of word NOT matched by
  * the prefix (i.e. the end of the word) is returned, otherwise NULL.
  */
{
    if (!prefix || !word) return NO;

    if (ignoreCase) {
 	while (NXToLower(*prefix) == NXToLower(*word) && *prefix) {
	    prefix++;
	    word++;
	}
    } else if (!checkCase && alllc(prefix)) {
	while (*prefix == NXToLower(*word) && *prefix) {
	    prefix++;
	    word++;
	}
    } else {
	while (*prefix == *word && *prefix) {
	    prefix++;
	    word++;
	}
    }

    return *prefix ? NULL : word;
}

static void truncCommonString(char *string, const char *sub)
{
    if (string && sub) {
	while (*string == *sub) {
	    string++;
	    sub++;
	}
	*string = '\0';
    }
}

- (BOOL)_completeName:(char *)name maxLen:(int)length
{
    BOOL done = NO;
    struct stat stbuf;
    struct stat *st = NULL;
    _NXDirInfo dirInfo = NULL;
    char *dirPath, *s, *prefix;
    int i, column, diff, matches;
    char path[MAXPATHLEN+1];

    if (*name == '/' || *name == '~') {
	strcpy(path, name);
    } else {
	[self _getPathTo:[browser lastColumn] into:path];
	strcat(path, "/");
	strcat(path, name);
    }
    if (![self _cleanPath:path]) return -1;
    prefix = strrchr(path, '/');
    if (!prefix) return -1;
    if (prefix == path) {
	dirPath = "/";
    } else {
	dirPath = path;
    }
    *prefix++ = '\0';

    for (column = [browser lastColumn]; column >= 0; column--) {
	dirInfo = _columns[column];
	if (dirInfo) {
	    if (!strcmp(dirPath, dirInfo->path)) break;
	    if (!st && (st = &stbuf) && STAT(dirPath, st) < 0) return -1;
	    if (st && st->st_ino == dirInfo->inode && st->st_dev == dirInfo->device) break;
	}
    }
    if (column < 0) return -1;

    diff = strlen(name) - strlen(prefix);
    name += diff;
    length -= diff;

    for (i = 0, matches = 0; i < dirInfo->count && !done; i++) {
	s = dirInfo->files[i];
	if (prefixMatch(prefix, s, YES, NO)) {
	    if (!matches) {
		strncpy(name, s, length);
	    } else {
		truncCommonString(name, s);
	    }
	    matches++;
	} else if (spFlags._checkCase || !prefixMatch(prefix, s, NO, YES)) {
	    done = (dostrcmp(&s, &prefix, spFlags._checkCase) > 0);
	}
    }

    if (column == [browser lastColumn]) [self _indicatePrefix:name];

    return matches;
}

- _indicatePrefix:(const char *)prefix
 /*
  * Updates the last column of the browser so that a line matching
  * prefix is selected.  Even if a match cannot be made, the column is
  * scrolled to the vacinity.
  *
  * NOTE: Assumes the prefix is in the current directory!!!
  *
  * Selects the first cell in the current matrix which prefix matches
  * the given prefix.  Whether or not the prefix matches any of the files,
  * the matrix is scrolled to the general vacinity.
  */
{
    id matrix;
    char **files;
    const char *s;
    int row, rows, cols, matchRow;
    BOOL done = NO, exactOnly = !spFlags.opening;
    char *ext;
    char prefixBuf[MAXPATHLEN+1], extPrefix[MAXPATHLEN];

    matrix = [browser matrixInColumn:[browser lastColumn]];
    if (!matrix) return self;

    if (!prefix || !*prefix) {
	clearSelectedCells(matrix);
	return self;
    }
    if (s = strchr(prefix, '/')) {
	while (s && *s == '/') s++;
	if (*s) {
	    clearSelectedCells(matrix);
	    return self;
	} else {
	    strcpy(prefixBuf, prefix);
	    *(strchr(prefixBuf, '/')) = '\0';
	    prefix = prefixBuf;
	    if (!*prefix) return self;
	}
    }
    if (prefix[0] == '~' || (prefix[0] == '.' && (!prefix[1] || (prefix[1] == '.' && !prefix[2])))) {
	clearSelectedCells(matrix);
	return self;
    }

    if (spFlags.opening) {
	s = [[matrix selectedCell] stringValue];
	if (prefixMatch(prefix, s, spFlags._checkCase, NO)) exactOnly = YES;
    } else if (requiredType && *requiredType) {
	ext = strrchr(prefix, '.');
	if (!ext || strcmp(ext+1, requiredType)) {
	    strcpy(extPrefix, prefix);
	    strcat(extPrefix, ".");
	    strcat(extPrefix, requiredType);
	    prefix = extPrefix;
	}
    } else if (requiredType) {
	/* should only indicate prefix if there is NO valid extension */
    }

    files = _columns ? _columns[[browser lastColumn]]->files : NULL;
    if (!files) return self;

    [matrix getNumRows:&rows numCols:&cols];
    for (row = 0, matchRow = -1; row < rows && !done;) {
	if (prefixMatch(prefix, files[row], (spFlags._checkCase || !spFlags.opening), NO)) {
	    if (!exactOnly || (files[row] && !strcmp(prefix, files[row]))) {
		id cell = [browser getLoadedCellAtRow:row inColumn:[browser lastColumn]];
		[matrix _scrollRowToCenter:row];
		if ([cell isEnabled]) {
		    [matrix selectCellAt:row :0];
		    return self;
		} else if (exactOnly) {
		    return self;
		}
	    } else if (matchRow < 0) {
		matchRow = row;
	    }
	}
	if (spFlags._checkCase || !prefixMatch(prefix, files[row], NO, YES)) {
	    done = (dostrcmp(files+row, &prefix, spFlags._checkCase) > 0);
	}
	if (!done) row++;
    }

    if (!spFlags.opening || !exactOnly) {
	clearSelectedCells(matrix);
	[matrix _scrollRowToCenter:(matchRow < 0 ? (row < rows ? row : rows - 1) : matchRow)];
    }

    return self;
}

- _updateWorkspace:sender
{
    id speaker = [NXApp appSpeaker];
    port_t sendPort = [speaker sendPort];
    port_t workspacePort = NXPortFromName(NX_WORKSPACEREQUEST, NULL);

    if (workspacePort != PORT_NULL) {
	[speaker setSendPort:workspacePort];
	[speaker performRemoteMethod:"update"];
	[speaker setSendPort:sendPort];
    }

    return self;
}

- (BOOL)_validateNames:(const char *)name checkBrowser:(BOOL)checkBrowser
 /*
  * Ensures that the specified name (and all the names selected in the
  * last column of the browser if allowMultipleFiles is set) do, indeed,
  * exist, or, in the case of the SavePanel, that it does NOT exist (or
  * the user knows that she wants to replace it).  It also gives the
  * delegate an opportunity to validate the names.
  */
{
    struct stat st;
    char *extension;
    BOOL isAutomount;
    id cell, matrix = nil;
    int i, rows, cols, count = 1;
    char extendedName[MAXPATHLEN+1];

    if (checkBrowser) {
	if (spFlags.opening) {
	    count = 0;
	    matrix = [browser matrixInColumn:[browser lastColumn]];
	    [matrix getNumRows:&rows numCols:&cols];
	    for (i = 0; i < rows; i++) {
		cell = [matrix cellAt:i :0];
		if ([cell isLoaded] && [cell isLeaf] && ([cell isHighlighted] || [cell state])) {
		    count++;
		    name = [cell stringValue];
		    if (!spFlags.allowMultiple) break;
		}
	    }
	    if (!count && name && *name) {
		count = [self _stat:&st file:name inColumn:[browser lastColumn] isAutomount:&isAutomount] ? 1 : 0;
		if (isAutomount || ((st.st_mode & S_IFMT) == S_IFDIR)) count = 0;
	    }
	    if (!count) return NO;
	} else {
	    if (!name || !*name) return NO;
	    if (requiredType && *requiredType) {
		extension = strrchr(name, '.');
		if (!extension || strcmp(extension+1, requiredType)) {
		    strcpy(extendedName, name);
		    strcat(extendedName, ".");
		    strcat(extendedName, requiredType);
		    name = extendedName;
		}
	    } else if (requiredType) {
		/* should disallow if it has ANY valid extension */
	    }
	    if ([self _stat:NULL file:name inColumn:[browser lastColumn] isAutomount:NULL]) {
		BOOL inOrder = YES; 
		const char *msg = KitString2(SavePanel, ">The file %s in %s exists. Replace it?", &inOrder, "If the second %s is the file, put instead of >.");
		if (!NXRunAlertPanel(
		    _NXKitString("SavePanel", "Save"), msg,
		    KitString(SavePanel, "Replace", "Button user chooses to overwrite an existing file."),
		    _NXKitString("SavePanel", "Cancel"),
		    NULL, inOrder ? name : directory, inOrder ? directory : name)) {
		    return NO;
		}
	    }
	}
    }

    free(filenames);
    filenames = (const char **)NXZoneMalloc([self zone], (count+1)*sizeof(char *));
    filenames[count] = NULL;
    if (count == 1) {
	free(filename);
	filename = (char *)NXZoneMalloc([self zone], strlen(directory)+strlen(name)+2);
	strcpy(filename, directory);
	if (!*directory || directory[strlen(directory)-1] != '/') strcat(filename, "/");
	strcat(filename, name);
	filenames[0] = strrchr(filename, '/')+1;
    } else {
	free(filename);
	filename = NULL;
	for (i = rows-1; i >= 0; i--) {
	    cell = [matrix cellAt:i :0];
	    if ([cell isLoaded] && [cell isLeaf] && ([cell isHighlighted] || [cell state])) {
		filenames[--count] = [cell stringValue];
	    }
	}
    }

    if (spFlags._delegateValidatesNew) {
	if (![delegate panelValidateFilenames:self]) return NO;
    } else if (spFlags._delegateValidatesOld) {
	if (![delegate validateFilename:self]) return NO;
    }

    [self perform:@selector(_updateWorkspace:) with:nil afterDelay:100 cancelPrevious:YES];

    return YES;
}

static int slashCount(const char *file)
{
    int count = 0;

    file = strchr(file, '/');
    while (file) {
	count++;
	while (*file == '/') file++;
	file = strchr(file, '/');
    }

    return count;
}

- (BOOL)_cleanPath:(char *)path
{
    char *np, *p;
    struct passwd *pw;
    BOOL replaceSlash;
    char wd[MAXPATHLEN+1];
    char npath[MAXPATHLEN+1];

    p = path;
    np = npath;
    while (*p) {
	if (*p != '/') {
	    if (*p == '~') {
		p++;
		if (!*p || *p == '/') {
		    if (*p) {
			strcpy(npath, NXHomeDirectory());
			strcat(npath, p);
			strcpy(path, npath);
		    } else {
			strcpy(path, NXHomeDirectory());
		    }
		    return [self _cleanPath:path];
		} else {
		    np = p;
		    p = strchr(p, '/');
		    if (p) *p = '\0';
		    pw = getpwnam(np);
		    if (pw) {
			if (p) {
			    strcpy(npath, pw->pw_dir);
			    *p = '/';
			    strcat(npath, p);
			    strcpy(path, npath);
			} else {
			    strcpy(path, pw->pw_dir);
			}
			return [self _cleanPath:path];
		    } else {
			_NXKitAlert("SavePanel", "Bad user", "No such user: %s", NULL, NULL, NULL, np);
		    }
		}
	    }
	    return NO;
	}
	*np++ = '/';
	while (*p == '/') p++;
	if (*p == '.' && (!p[1] || p[1] == '/')) {
	    p++; np--;
	} else if (*p == '.' && p[1] == '.' && (!p[2] || p[2] == '/')) {
	    if (replaceSlash = (p[2] == '/')) p[2] = '\0';
	    if (spFlags._cancd) {
		_cdcolumn = -1;
		if (CHDIR(path)) return NO;
	    } else {
		if (!GETWD(wd)) return NO;
		if (CHDIR(path)) return NO;
	    }
	    if (!GETWD(npath)) {
		if (!spFlags._cancd) CHDIR(wd);
		return NO;
	    }
	    if (!spFlags._cancd) CHDIR(wd);
	    if (replaceSlash) {
		p[2] = '/';
		strcat(npath, p+2);
	    }
	    strcpy(path, npath);
	    return [self _cleanPath:path];
	} else {
	    while (*p && *p != '/') *np++ = *p++;
	}
    }

    *np = '\0';
    strcpy(path, npath);
    if (!*path) strcpy(path, "/");

    return YES;
}

static char *createPath(char *path)
{
    char *slash, *firstNewComponent = NULL;

    slash = strchr(path, '/');
    if (slash != path) return NULL;
    while (slash) {
	while (*slash == '/') slash++;
	slash = strchr(slash, '/');
	if (slash) *slash = '\0';
	if (mkdir(path, 0777)) {
	    if (errno != EEXIST) return NULL;
	} else {
	    if (!firstNewComponent) firstNewComponent = strrchr(path, '/')+1;
	}
	if (slash) *slash = '/';
    }

    return firstNewComponent;
}

- _handleInvalidPath:(char *)path
 /*
  * If this is called in the context of the SavePanel, then the user is asked
  * if she wishes to create the path, otherwise, an error alert is presented
  * to the user.  If the user decides to create the path, then ok: is called
  * (recursively, since ok: called this). 
  */
{
    char *newComponent, *slash;

    if (!spFlags.opening) {
	slash = strrchr(path, '/');
	if (slash) *slash = '\0';
	if (_NXKitAlert("SavePanel", "Nonexistent path", "The path %s does not exist, create it?", "Create", "Cancel", NULL, path) > 0) {
	    if (newComponent = createPath(path)) {
		if (slash) *slash = '/';
		[form setStringValue:newComponent];
		return [self ok:self];
	    }
	    _NXKitAlert("SavePanel", NULL, "Couldn't create path.", NULL, NULL, NULL);
	}
    } else {
	_NXKitAlert("SavePanel", NULL, "Invalid path: %s.", NULL, NULL, NULL, path);
    }

    return nil;
}

static void checkFile(const char *path, BOOL *exists, BOOL *isLeaf)
{
    struct stat st;

    if (isLeaf) *isLeaf = YES;
    if (exists) *exists = YES;

    if (!LSTAT(path, &st)) {
	if ((st.st_mode & S_IFMT) == S_IFLNK) {
	    if (st.st_mode & S_ISVTX) {
		if (isLeaf) *isLeaf = NO;
	    } else if (!STAT(path, &st)) {
		if (isLeaf) *isLeaf = ((st.st_mode & S_IFMT) != S_IFDIR);
	    } else {
		if (exists) *exists = NO;
	    }
	} else {
	    if (isLeaf) *isLeaf = ((st.st_mode & S_IFMT) != S_IFDIR);
	}
    } else {
	if (exists) *exists = NO;
    }
}

- (BOOL)_optimizeOk:(char *)path
/*
 * This little gem allows us to skip updating the
 * path in the NXBrowser if the user specifices
 * EXACTLY the file she wants to open/save in.
 * Not to worry, the next time the panel comes
 * up, the browser will get updated to the
 * proper directory.
 */
{
    char *leaf, *extension;
    BOOL exists, isLeaf, addedExtension = NO;

    if (leaf = strrchr(path, '/')) {
	if (requiredType && *requiredType) {
	    extension = strrchr(leaf, '.');
	    if (!extension || strcmp(extension+1, requiredType)) return NO;
	}
	if (![self _checkType:leaf+1]) return NO;
	checkFile(path, &exists, &isLeaf);
	*leaf++ = '\0';
	if (!spFlags.opening) {
	    if (!exists) {
		checkFile((*path ? path : "/"), &exists, &isLeaf);
	    } else {
		isLeaf = YES;
	    }
	}
	if (exists && ((spFlags.opening && isLeaf) || (!spFlags.opening && !isLeaf))) {
	    [self _setDirectory:(*path ? path : "/")];
	    if ([self _validateNames:leaf checkBrowser:NO]) {
		spFlags.exitOk = YES;
		[NXApp stopModal:NX_OKTAG];
		return YES;
	    }
	}
    /* couldn't optimize, so clean up path */
	*--leaf = '/';
	if (addedExtension) {
	    leaf = strrchr(path, '.');
	    if (leaf) *leaf = '\0';
	}
    }

    return NO;
}

- ok:sender
{
    int i;
    struct stat st;
    const char *file;
    id selectedCell, retval = self;
    BOOL isADir, isAutomount, displayDisabled = NO;
    char path[MAXPATHLEN+1];
    char npath[MAXPATHLEN+1];

#ifdef AKDEBUG
    if (savePanelTimings) [self _resetTimers];
#endif

    if (!_columns) [self _initializePanel:NO path:NULL name:NULL savedCWD:NULL];

    selectedCell = [[browser matrixInColumn:[browser lastColumn]] selectedCell];
    if (selectedCell && ![selectedCell isLeaf]) {
	MARKTIME(savePanelTimings, "Simple advance into selected branch.", 1);
	[browser addColumn];
	[self _setForm:"" copy:NO select:YES ok:NO];
	goto done;
    }

    MARKTIME(savePanelTimings, "Processing ok:", 1);

    file = [form stringValue];
    while (*file == ' ' || *file == '	') file++;
    if (file && *file) {
	if (*file == '/' || *file == '~') {
	    strcpy(npath, file);
	} else {
	    MARKTIME(savePanelTimings, "Getting path to clicked-on column.", 2);
	    [self _getPathTo:[browser lastColumn] into:npath];
	    strcat(npath, "/");
	    strcat(npath, file);
	}
	MARKTIME(savePanelTimings, "Cleaning path.", 2);
	strcpy(path, npath);
	if (![self _cleanPath:path]) {
	    MARKTIME(savePanelTimings, "Bad path.", 2);
	    NXBeep();
	    [form selectText:self];
	    goto done;
	}

	if ([self _optimizeOk:path]) goto done;

	[self disableFlushWindow];

	if (*file != '/' && *file != '~') {
	    for (i = 0; path[i] && npath[i] && path[i] == npath[i]; i++);
	} else {
	    i = 0;
	}
	if (slashCount(path+i) > [browser numVisibleColumns]) {
	    displayDisabled = YES;
	    [self disableDisplay];
	}
	[browser setPath:path];
	if (displayDisabled) {
	    [self reenableDisplay];
	    if ([browser needsDisplay]) {
		MARKTIME(savePanelTimings, "Displaying all columns.", 2);
		[browser displayAllColumns];
	    }
	}
	MARKTIME(savePanelTimings, "Getting path from NXBrowser.", 2);
	[self _getPathTo:[browser lastColumn] into:npath];
	[self _setDirectory:npath];
	for (i = 0; path[i] && npath[i] && path[i] == npath[i]; i++);
	if (path[i] == '/') i++;
	file = path+i;
	if (strchr(file, '/')) {
	    MARKTIME(savePanelTimings, "Handling case where slash remains in leaf of path name.", 2);
	    strcpy(npath, file);
	    *(strchr(npath, '/')) = '\0';
	    isADir = YES;
	    if ([self _stat:&st file:npath inColumn:[browser lastColumn] isAutomount:&isAutomount]) {
		isADir = isAutomount || ((st.st_mode & S_IFMT) == S_IFDIR);
	    }
	    if (!isADir) {
		/* This bcopy may do overlapping copies! */
		bcopy(npath, npath+strlen(directory)+1, strlen(npath)+1);
		strcpy(npath, directory);
		npath[strlen(directory)] = '/';
		MARKTIME(savePanelTimings, "Putting up an alert.", 3);
		_NXKitAlert("SavePanel", "Bad path", "%s is not a directory!", NULL, NULL, NULL, npath);
	    } else {
		strcpy(npath, directory);
		strcat(npath, "/");
		strcat(npath, file);
	    }
	    MARKTIME(savePanelTimings && isADir, "Putting up an alert.", 3);
	    if (isADir && [self _handleInvalidPath:npath]) {
		[self reenableFlushWindow];
		[self flushWindow];
		goto done;
	    } else {
		[self _setForm:file copy:YES select:YES ok:NO];
		[self _indicatePrefix:file];
		[self reenableFlushWindow];
		[self flushWindow];
		retval = nil;
		goto done;
	    }
	}
	[self _setForm:file copy:YES select:NO ok:NO];
	MARKTIME(savePanelTimings, "Indicating prefix.", 2);
	if (selectedCell && spFlags.opening) [[browser matrixInColumn:[browser lastColumn]] selectCell:selectedCell];
	[self _indicatePrefix:file];
    } else if (selectedCell || hasMultipleSelection([browser matrixInColumn:[browser lastColumn]])) {
	[self _getPathTo:[browser lastColumn] into:npath];
	[self _setDirectory:npath];
    }

    [form selectText:self];

    [self reenableFlushWindow];
    [self flushWindow];

    if ([self _validateNames:[form stringValue] checkBrowser:YES]) {
	spFlags.exitOk = YES;
	[NXApp stopModal:NX_OKTAG];
    }

done:

    DUMPTIMES(savePanelTimings);
    CLEARTIMES(savePanelTimings);
#ifdef AKDEBUG
    if (savePanelTimings) [self _stopTimers:"display"];
#endif

    return retval;
}


- _formHit:sender
{
    if (spFlags._forwards || spFlags._backwards) {
	spFlags._forwards = NO;
	spFlags._backwards = NO;
	[sender selectText:self];
    } else {
	[okButton performClick:sender];
    }

    return self;
}


- _doubleHit:sender
{
    if ([[[browser matrixInColumn:[browser lastColumn]] selectedCell] isLeaf]) {
	[okButton setEnabled:YES];
	[okButton performClick:sender];
    }
    return self;
}

- _itemHit:sender
{
    id cell, matrix;

    matrix = [browser matrixInColumn:[browser lastColumn]];
    cell = [matrix selectedCell];
    if ([cell isLeaf]) {
	if (spFlags.allowMultiple && hasMultipleSelection(matrix)) {
	    [self _setForm:"" copy:NO select:YES ok:NO];
	    [okButton setEnabled:YES];
	} else {
	    [self _setForm:[cell stringValue] copy:NO select:YES ok:NO];
	}
    } else {
	[form selectText:self];
	[self _indicatePrefix:[form stringValue]];
    }

    return self;
}

@end

/*
  
Modifications (since 0.8):
  
 2/18/89 pah	completely rewritten for 0.82
		move .places code to places.m
		add setPrompt:
		make panel use the browser
		rename dirPath to be directory
		make command-space do escape completion
		support doc-wrappers and .hidden files
  
0.83
----
 3/08/89 pah	Fix bug whereby SavePanel confused directories with files
 3/09/89 pah	Notify user when invalid path is specified
 3/10/89 pah	Add method setRequiredExtension:
 3/17/89 pah	Add setAccessoryView: to allow customization of the panel
		Error handling around runModalFor:
 3/18/89 pah	Allow user to create a non-existent path

0.91
----
 4/25/89 pah	Make UnixExpert, backwards, forwards, and lastMatched be
		 instance variables (flags) as well as make lastMatch be
		 per-instance
 5/10/89 pah	optimize drawing and statting
 5/22/89 pah	add constrained resizing
 5/23/89 pah	add hook for OpenPanel to inherit more of factory (_dontCache)
 5/19/89 trey	added -requiredFileType

0.92
----
  6/5/89 pah	added timing stuff
  6/8/89 pah	optimized some more stuff
 6/10/89 pah	added wait cursor

0.93
----
 6/16/89 pah	add support for kit methods that now return const char *
		add support for stopModal: scheme
 6/17/89 pah	add case insensitivity support
		add support for NXRunAlertPanel()

0.94
----
 7/13/89 pah	ripped out orderWindow:relativeTo: -- don't need it anymore
		no longer set tags of controls programatically (set in IB)
		fix bug whereby typing test when required type is foo will
		 complain if test exists (even if test.foo doesn't!)

0.96
----
 7/20/89 pah	always check .places against the directory since the
		 faked directory entries made by automount can screw up
		 .places (and we can't recover because it doesn't permanently
		 update the write date of the directory).  This slows the
		 panel down, but ...
		when the user clicks on an item, stat it to be sure that
		 it is still there, hasn't changed from a dir to a file
		 or vice versa, etc.  Beep when a problem is found with it.
84
--
 5/14/90 pah	Rewritten to support NXBrowser.

85
--
  6/4/90 pah	Fixed bug: adding a non-existent item to a column by
  		 explicitly typing its name can could cause a boundary
		 condition to occur in browser:selectCell:atRow:inColumn:
		Added _handleInvalidPath: which does just that.
		If the user selects multiple files in the OpenPanel, the
		 form must be blanked out.
86
--
 6/12/90 pah	Fixed bug #4992: spaces and other funky characters in
		 a path that is being created by the SavePanel didn't work
		Fixed bug #5036: setRequiredType:"" now works
		Fixed bug #4993: typing /as/hegarty/foo/bar will now complain
		 correctly if /as/hegarty/foo is not a directory
		Fixed bug #4584: double-clicking in the SavePanel is now
		 the same as the OpenPanel
		Fixed bug #5999: the SavePanel now properly creates extended 
		 paths
		Fixed bug #6240: proper complaints appear if user types 
		 ~nosuchuser
		Fixed bug #6191: added device to dirInfo (what a bozo!)
		Fixed bug: now call validateVisibleColumns before ordering 
		 front

94
--
 9/25/90 gcockrft	use byteLength instead of textLength

99
--
 10/20/90 aozer	Fixed #11204: panel:filterFile:inDirectory: doesn't work
		by changing delegate to anObject in setDelegate:
 

*/


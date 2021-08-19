#import "SavePanel.h"
#import <objc/hashtable.h>
#import <sys/stat.h>

@interface SavePanel(Private)

struct __NXDirInfo {
    dev_t		 device;
    ino_t		 inode;
    time_t		 mtime;
    int			 count;
    char		*path;
    char	       **files;
};

+ _dontCache;
- _free;
- _setDirectory:(const char *)path;
- (BOOL)_checkFile:(const char *)file exclude:(NXHashTable *)hashTable column:(int)column directory:(const char *)path leaf:(int)leafStatus;
- (BOOL)_completeName:(char *)path maxLen:(int)maxlen;
- _indicatePrefix:(const char *)prefix;
- (char *)_getPathTo:(int)column into:(char *)path;
- (BOOL)_stat:(struct stat *)statbuf file:(const char *)file inColumn:(int)column isAutomount:(BOOL *)isAutomount;
- (BOOL)_validateNames:(const char *)name;
- (BOOL)_cleanPath:(char *)path;
- (char **)_filterTypes;
- _goHome:sender;
- _formHit:sender;
- _doubleHit:sender;
- _itemHit:sender;

@end

@interface SavePanel(Compatibility)
- validateFilename:sender;
@end

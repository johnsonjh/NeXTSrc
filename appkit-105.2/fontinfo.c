/*
	fontinfo.m
	Copyright 1989, NeXT, Inc.
	Responsibility: Paul Hegarty
  	
	Gets info about fonts for appkit clients.
	Part of the appkit server.
*/

#import "pbtypes.h"
#import "nextstd.h"
#import <objc/hashtable.h>
#import <streams/streams.h>
#import <string.h>
#import <sys/file.h>
#import <sys/time.h>
#import <sys/stat.h>
#import <sys/param.h>
#import <sys/types.h>
#import <sys/dir.h>
#import <mach.h>

extern void fixNFSData(const char *path, char **start, int *len, int *maxLen);
extern BOOL _NXSetUser(const char *user);
extern BOOL _NXGetHomeDir(char *user);
extern void _NXUnsetUser(void);

#ifdef DEBUG
    #define DEBUG_MSG(msg) fprintf(stderr, "%s\n", msg)
#else
    #define DEBUG_MSG(msg) {}
#endif

typedef struct {
    int inode;
    char *data;
    int length;
    int maxlen;
    time_t mtime;
} FontDirectoryInfo;

static FontDirectoryInfo *loadData(const char *file)
{
    NXStream *s;
    FontDirectoryInfo *info;

    NX_MALLOC(info, FontDirectoryInfo, 1);

    s = NXMapFile(file, NX_READONLY);
    if (s) {
	NXGetMemoryBuffer(s, &info->data, &info->length, &info->maxlen);
	fixNFSData(file, &info->data, &info->length, &info->maxlen);
	NXCloseMemory(s, NX_SAVEBUFFER);
    } else {
	info->data = NULL;
	info->length = 0;
    }

    if (info->data && info->length) {
	return info;
    } else {
	NX_FREE(info);
	return NULL;
    }
}

static void freeFontDirectoryInfo(FontDirectoryInfo *info)
{
    vm_deallocate(task_self(), (vm_address_t)info->data, info->maxlen);
    NX_FREE(info);
}

static int getPath(const char *path, const char *home, const char *user, char *fullpath)
{
    char *slash;
    int strlength, userfonts = 0;

    if (!path) {
	DEBUG_MSG("Font directory request with null path");
	return FONTDIR_BAD;
    } else if (path[0] == '~') {
	userfonts = 1;
	if (path[1] == '/' || !path[1]) {
	    if (access(home, F_OK)) {
		strcpy(fullpath, user);
		if (!_NXGetHomeDir(fullpath)) {
		    DEBUG_MSG("Font directory request for unknown user");
    		    return FONTDIR_BAD;
		}
	    } else {
		strcpy(fullpath, home);
	    }
	    path++;
	} else {
	    strcpy(fullpath, &path[1]);
	    slash = strchr(fullpath, '/');
	    if (slash) *slash = '\0';
	    if (_NXGetHomeDir(fullpath)) {
		slash = strchr(path, '/');
		if (slash) {
		    path = slash;
		} else {
		    path += strlen(path);
		}
	    } else {
		DEBUG_MSG("Font directory request in unknown user directory");
		return FONTDIR_BAD;
	    }
	}
    } else if (path[0] == '/') {
	fullpath[0] = '\0';
    } else {
	DEBUG_MSG("Font directory request in relative directory");
	return FONTDIR_BAD;
    }

    strcat(fullpath, path);
    strlength = strlen(fullpath) - 1;
    if (fullpath[strlength] != '/') {
	fullpath[++strlength] = '/';
	fullpath[++strlength] = '\0';
    }

    return userfonts;
}

static BOOL sameFonts(char *fontlist, char *fontdirectory)
{
    DIR *dirp;
    NXStream *s;
    struct direct *dp;
    int i, length = 0;
    char *files = NULL, *ext;

    s = NXMapFile(fontlist, NX_READONLY);
    if (s) NXGetMemoryBuffer(s, &files, &length, &i);
    dirp = opendir(fontdirectory);
    if (dirp) for (i = 0, dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	if (i >= length && dp->d_name && (ext = strrchr(dp->d_name, '.')) && !strcmp(ext, ".font")) {
	    if (s) NXCloseMemory(s, NX_FREEBUFFER);
	    DEBUG_MSG("New fonts in Fonts directory");
	    return NO;
	}
	if (dp->d_name && (files && !strcmp(files+i, dp->d_name))) i += strlen(files+i)+1;
    }
    if (s) NXCloseMemory(s, NX_FREEBUFFER);
    if (i < length) {
	DEBUG_MSG("Some fonts have been deleted from Fonts directory");
	return NO;
    }

    DEBUG_MSG("Fonts directory has not changed");

    return YES;
}

static void touch(char *filename)
{
    struct timeval tvp[2];
    struct timezone tz;
    gettimeofday(tvp, &tz);
    tvp[1] = tvp[0];
    utimes(filename, tvp);
    DEBUG_MSG("Updating .fontdirectory write time");
}

static int getFontDirectory(char *path, char *user, char *home, char **data, unsigned int *dataLength, int build)
{
    static NXHashTable *hashTable = NULL;
    static int cachedInode = 0;

    BOOL rebuild = NO, setUserOk;
    int pid, pathOk, userfonts = 0;
    struct stat afmst, fontdirst;
    FontDirectoryInfo tempInfo, *info = NULL, *oldInfo;
    char fullpath[MAXPATHLEN+1];
    char afmpath[MAXPATHLEN+1];
    char fontdirpath[MAXPATHLEN+1];

    setUserOk = _NXSetUser(user);

    *data = NULL;
    *dataLength = 0;
    pathOk = getPath(path, home, user, fullpath);

    if (pathOk < 0) {
	if (setUserOk) _NXUnsetUser();
	return pathOk;
    } else if (pathOk > 0) {
	userfonts = 1;
    }

#ifdef DEBUG
    fprintf(stderr, "Font directory request: %s\n", fullpath);
#endif

    if (stat(fullpath, &afmst)) {
	DEBUG_MSG("Couldn't stat Fonts directory");
	if (setUserOk) _NXUnsetUser();
	return FONTDIR_BAD;
    } else {
	strcpy(fontdirpath, fullpath);
	strcat(fontdirpath, ".fontdirectory");
	if (stat(fontdirpath, &fontdirst)) {
	    DEBUG_MSG("No font directory found");
	    if (userfonts && build) {
		if (!access(fullpath, X_OK|R_OK|W_OK)) {
		    DEBUG_MSG("No directory found, rebuilding ...");
		    pid = fork();
		    if (!pid) {
			execl("/usr/bin/buildafmdir", "buildafmdir", fullpath, 0);
			NXLogError("pbs error: could not exec buildafmdir");
			exit(-1);
		    } else {
			if (fork() == 0) {
			    execl("/usr/bin/cacheAFMData", "cacheAFMData", fullpath, 0);
			    NXLogError("pbs error: could not exec cacheAFMData");
			    exit(-1);
			}
			if (setUserOk) _NXUnsetUser();
			return (pid < 0) ? FONTDIR_BAD : FONTDIR_STALE;
		    }
		} else {
		    if (setUserOk) _NXUnsetUser();
		    NXLogError("pbs error: can't update font information in %s", fullpath);
		    return FONTDIR_ROTTEN;
		}
	    } else {
		if (setUserOk) _NXUnsetUser();
		return FONTDIR_BAD;
	    }
	}
	if (fontdirst.st_mtime < afmst.st_mtime) {
	    strcpy(afmpath, fullpath);
	    strcat(afmpath, "afm");
	    if (!stat(afmpath, &afmst) && fontdirst.st_mtime < afmst.st_mtime) {
		rebuild = YES;
	    } else {
		strcpy(afmpath, fullpath);
		strcat(afmpath, ".fontlist");
		rebuild = !sameFonts(afmpath, fullpath);
		if (!rebuild) touch(fontdirpath);
	    }
	}
    }

    tempInfo.inode = fontdirst.st_ino;
    if (hashTable && (info = NXHashGet(hashTable, &tempInfo)) && info->mtime < fontdirst.st_mtime) {
	DEBUG_MSG("Directory is newer than cached version");
	NXHashRemove(hashTable, info);
	freeFontDirectoryInfo(info);
	info = NULL;
    }
    
    if (!info) {
	DEBUG_MSG("Loading data from the directory");
	info = loadData(fontdirpath);
	if (!hashTable) hashTable = NXCreateHashTable(NXPtrStructKeyPrototype, 3, NULL);
	if (info) {
	    DEBUG_MSG("Adding newly loaded directory to the cache");
	    info->inode = fontdirst.st_ino;
	    NXHashInsert(hashTable, info);
	}
    }

    if (info) {
	if (userfonts && cachedInode != fontdirst.st_ino) {
	    tempInfo.inode = cachedInode;
	    oldInfo = hashTable ? NXHashRemove(hashTable, &tempInfo) : NULL;
	    if (oldInfo) {
		DEBUG_MSG("Discarding previously cached user's directory");
		freeFontDirectoryInfo(oldInfo);
		oldInfo = NULL;
	    }
	    cachedInode = fontdirst.st_ino;
	}
	info->mtime = fontdirst.st_mtime;
	*data = info->data;
	*dataLength = info->length;
    }

    if (!info) {
	if (setUserOk) _NXUnsetUser();
	return FONTDIR_BAD;
    } else if (rebuild) {
	if (build && userfonts) {
	    if (!access(fullpath, X_OK|R_OK|W_OK)) {
		DEBUG_MSG("Data in the directory may be stale, rebuilding ...");
		pid = fork();
		if (!pid) {
		    execl("/usr/bin/buildafmdir", "buildafmdir", fullpath, 0);
		    NXLogError("pbs error: could not exec buildafmdir");
		    exit(-1);
		} else {
		    if (fork() == 0) {
			execl("/usr/bin/cacheAFMData", "cacheAFMData", fullpath, 0);
			NXLogError("pbs error: could not exec cacheAFMData");
			exit(-1);
		    }
		    if (setUserOk) _NXUnsetUser();
		    return (pid < 0) ? FONTDIR_ROTTEN : FONTDIR_STALE;
		}
	    } else {
		if (setUserOk) _NXUnsetUser();
		NXLogError("pbs error: can't update font information in %s", fullpath);
		return FONTDIR_ROTTEN;
	    }
	} else {
	    DEBUG_MSG("Data in the directory may be stale.");
	    if (setUserOk) _NXUnsetUser();
	    return FONTDIR_ROTTEN;
	}
    }

    if (setUserOk) _NXUnsetUser();

    return FONTDIR_OK;
}

int _NXGetFontDirectory(port_t server,
			data_t path, unsigned int pathLen,
			data_t user, unsigned int userLen,
			data_t home, unsigned int homeLen,
			data_t *data, unsigned int *dataLength)
{
    int ret;

    ret = getFontDirectory(path, user, home, data, dataLength, 0);
    vm_deallocate(task_self(), (vm_address_t)path, pathLen);
    vm_deallocate(task_self(), (vm_address_t)user, userLen);
    vm_deallocate(task_self(), (vm_address_t)home, homeLen);
    return ret;
}

int _NXBuildFontDirectory(port_t server,
			  data_t path, unsigned int pathLen,
			  data_t user, unsigned int userLen,
			  data_t home, unsigned int homeLen,
			  data_t *data, unsigned int *dataLength)
{
    int ret;

    ret = getFontDirectory(path, user, home, data, dataLength, 1);
    vm_deallocate(task_self(), (vm_address_t)path, pathLen);
    vm_deallocate(task_self(), (vm_address_t)user, userLen);
    vm_deallocate(task_self(), (vm_address_t)home, homeLen);
    return ret;
}

/*
	
Modifications

94
--
 9/24/90 trey	cacheAFMData launched along with buildafmdir
*/

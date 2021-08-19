/*
	request.m
	Copyright 1989, NeXT, Inc.
	Responsibility: Paul Hegarty
  	
	Gets info about Services Menu services.
	Part of the appkit server (pbs).
*/

#import "afmprivate.h"
#import "app.h"
#import "nextstd.h"
#import <streams/streams.h>
#import <string.h>
#import <servers/netname.h>
#import <mach.h>
#import <cthreads.h>
#import <libc.h>
#import <ldsyms.h>
#import <sys/errno.h>
#import <sys/loader.h>
#import <pwd.h>
#import <sys/types.h>
#import <sys/dir.h>

#define LITTLE_ENDIAN_NATIVE	0
#define BIG_ENDIAN_NATIVE	1

static NXHashTable *servicesEnabledTable = NULL;
static BOOL haveServicesEnabledTable = YES;
static char *servicesEnabledTableOwner = NULL;
static time_t servicesEnabledTableModifyTime = 0;

static char *rmData = NULL;
static char *rmUser = NULL;
static int rmDataLength = 0;
static unsigned int rmCacheTime = 0;
static char *nonnativermData = NULL;

static char *fileext_wsmdata = NULL;
static int fileext_wsmlength = 0;

/* Caching seteuid and getpwnam functions. */

BOOL _NXSetUid(int uid)
{
    struct passwd *pwd;
    static lastUid = 0;

    if (uid <= 0) return NO;
    if (uid != lastUid) {
	if (pwd = getpwuid(uid)) {
	    initgroups(pwd->pw_name, pwd->pw_gid);
	    setegid(pwd->pw_gid);
	}
	lastUid = uid;
    }
    if (seteuid(uid)) return NO;

    return YES;
}

int _NXGetUid(const char *user)
{
    int uid = -1;
    struct passwd *pwd;
    static char *cachedUser = NULL;
    static int cachedUid = -1;

    if (user && *user) {
	if (cachedUser && !strcmp(cachedUser, user)) {
	    uid = cachedUid;
	} else {
	    pwd = getpwnam(user);
	    if (pwd) {
		if (cachedUser) free(cachedUser);
		cachedUser = NXCopyStringBuffer(user);
		uid = cachedUid = pwd->pw_uid;
	    }
	}
    }

    return uid;
}

BOOL _NXGetHomeDir(char *user)
{
    struct passwd *pwd;
    static char *cachedUser = NULL;
    static char *cachedHomeDirectory = NULL;

    if (user && *user) {
	if (!cachedUser || strcmp(user, cachedUser)) {
	    pwd = getpwnam(user);
	    if (pwd && pwd->pw_dir) {
		strcpy(user, pwd->pw_dir);
		free(cachedUser);
		cachedUser = NXCopyStringBuffer(user);
		free(cachedHomeDirectory);
		cachedHomeDirectory = NXCopyStringBuffer(pwd->pw_dir);
	    } else {
		return NO;
	    }
	} else {
	    strcpy(user, cachedHomeDirectory);
	}
    }

    return YES;
}


BOOL _NXSetUser(const char *user)
{
    return _NXSetUid(_NXGetUid(user));
}

void _NXUnsetUid()
{
    seteuid(getuid());
}

void _NXUnsetUser()
{
    _NXUnsetUid();
}

/* Functions to get info about the enabled state of services. */
 
void _NXUpdateDynamicServices(port_t server, const char *user, int ulength)
{
    int pid;

    _NXSetUser(user);
    pid = fork();
    if (!pid) {
	execl("/usr/bin/make_services", "make_services", 0);
	NXLogError("pbs error: child could not exec make_services");
	exit(-1);
    } else if (pid < 0) {
	NXLogError("pbs error: could not fork off make_services");
    }
    _NXUnsetUser();

    vm_deallocate(task_self(), (vm_address_t)user, ulength);
}

static const char *_NXGetServicesMenuItem(const char *name, NXRMEntry **retval)
{
    int i;
    const char *s;
    BOOL gotit = NO;
    NXRMEntry *entry;
    int *indr, *defaultIndex;
    NXRMHeader *rmHeader = (NXRMHeader *)rmData;

    if (!name || !*name || !rmHeader) {
	if (retval) *retval = NULL;
	return NULL;
    }

    for (i = 0; i < rmHeader->numEntries; i++) {
	entry = (NXRMEntry *)(rmData + rmHeader->entries + i * rmHeader->entrySize);
	if (entry && entry->menuEntries) {
	    defaultIndex = NULL;
	    indr = (int *)(rmData + rmHeader->indirectStringTable) + entry->menuEntries;
	    while (*indr) {
		s = (rmData + rmHeader->stringTable + *indr);
		if (!defaultIndex && s && !strcmp(s, SERVICES_MENU_DEFAULT_LANGUAGE)) {
		    if (gotit) {
			if (retval) *retval = entry;
			return (rmData + rmHeader->stringTable + *(indr+1));
		    }
		    defaultIndex = indr+1;
		}
		indr++;
		s = (rmData + rmHeader->stringTable + *indr);
		if (s && !strcmp(s, name)) {
		    if (defaultIndex) {
			if (retval) *retval = entry;
			return (rmData + rmHeader->stringTable + *defaultIndex);
		    }
		    gotit = YES;
		}
		indr++;
	    }
	}
    }

    if (retval) *retval = NULL;

    return NULL;
}

static void freeTable(NXHashTable *anStrTable)
{
    char *data;
    NXHashState state;
    if (anStrTable) {
	state = NXInitHashState(anStrTable);
	while (NXNextHashState(anStrTable, &state, (void **)&data)) free(data);
	NXFreeHashTable(anStrTable);
    }
}

static int _NXDoIsServicesMenuItemEnabled(const char *name, const char *user, BOOL isRealName)
{
    FILE *file;
    char *data;
    struct stat st;
    BOOL fileExists;
    const char *realName;
    int fd, hashCount, length, count = 0;
    char filename[MAXPATHLEN+1], buffer[MAXPATHLEN];

    if (!user || !*user) return EPERM;

    if (servicesEnabledTableOwner && strcmp(servicesEnabledTableOwner, user)) {
	free(servicesEnabledTableOwner);
	servicesEnabledTableOwner = NXCopyStringBuffer(user);
	freeTable(servicesEnabledTable);
	servicesEnabledTable = NULL;
	haveServicesEnabledTable = YES;
    }

    strcpy(filename, user);
    if (!_NXGetHomeDir(filename)) return cthread_errno();
    strcat(filename, "/.NeXT/services/.disabled");

    _NXSetUser(user);

    if (!haveServicesEnabledTable) {
	if (!stat(filename, &st)) haveServicesEnabledTable = YES;
    } else {
	fileExists = stat(filename, &st) ? NO : YES;
	if (!fileExists) haveServicesEnabledTable = NO;
	if (servicesEnabledTable && (!fileExists || st.st_mtime != servicesEnabledTableModifyTime)) {
	    freeTable(servicesEnabledTable);
	    servicesEnabledTable = NULL;
	}
    }

    if (!haveServicesEnabledTable) {
	_NXUnsetUser();
	return -1;
    }

    if (!servicesEnabledTable) {
	file = fopen(filename, "r");
	if (file) {
	    while (fgets(buffer, MAXPATHLEN, file)) {
		length = strlen(buffer);
		if (buffer[length-1] == '\n') buffer[length-1] = '\0';
		if (*buffer) {
		    if (!servicesEnabledTable) servicesEnabledTable = NXCreateHashTable(NXStrPrototype, 5, NULL);
		    if (NXHashMember(servicesEnabledTable, buffer)) {
			data = NXHashRemove(servicesEnabledTable, buffer);
			free(data);
		    } else {
			NXHashInsert(servicesEnabledTable, NXCopyStringBuffer(buffer));
		    }
		    count++;
		}
	    }
	    hashCount = servicesEnabledTable ? NXCountHashTable(servicesEnabledTable) : 0;
	    if (!hashCount) {
		unlink(filename);
	    } else if (count > hashCount * 2) {
		strcpy(buffer, filename);
		strcat(buffer, ".new");
		fd = open(buffer, O_CREAT|O_EXCL|O_WRONLY, 0644);
		if (fd >= 0) {
		    NXHashState	state = NXInitHashState(servicesEnabledTable);
		    while (NXNextHashState(servicesEnabledTable, &state, (void **)&data)) {
			write(fd, data, strlen(data));
			write(fd, "\n", 1);
		    }
		    if (!close(fd)) {
			unlink(filename);
			link(buffer, filename);
			unlink(buffer);
		    } else {
			unlink(buffer);
		    }
		}
	    }
	}
    }

    _NXUnsetUser();

    if (!servicesEnabledTable) {
	haveServicesEnabledTable = NO;
	return -1;
    }

    realName = isRealName ? name : _NXGetServicesMenuItem(name, NULL);
    if (!realName) realName = name;

    return realName ? (NXHashMember(servicesEnabledTable, realName) ? 0 : -1) : ENOENT;
}

/* Functions to get the Services Menu info for a user. */

static void setDisabledBits()
{
    int *indr;
    const char *s;
    NXRMEntry *entry;
    int i = 0, defIndr = -1;
    NXRMHeader *rmHeader = (NXRMHeader *)rmData;

    _NXDoIsServicesMenuItemEnabled("foo", rmUser, YES);	/* inits servicesEnabledTable */

    if (!servicesEnabledTable) return;			/* none are disabled */

    indr = (int *)(rmData + rmHeader->indirectStringTable);
    for (i = 0; i < rmHeader->indirectStringTableLength; i++) {	/* find default language string */
	s = rmData + rmHeader->stringTable + *(indr+i);
	if (s && !strcmp(s, SERVICES_MENU_DEFAULT_LANGUAGE)) {
	    defIndr = *(indr+i);
	    break;
	}
    }

    if (defIndr < 0) return;				/* should never happen, but ... */

    for (i = 0; i < rmHeader->numEntries; i++) {
	entry = (NXRMEntry *)(rmData + rmHeader->entries + (i * rmHeader->entrySize));
	indr = (int *)(rmData + rmHeader->indirectStringTable) + entry->menuEntries;
	while (*indr) {
	    if (*indr == defIndr) {
		s = rmData + rmHeader->stringTable + *(indr+1);
		if (NXHashMember(servicesEnabledTable, s)) {
		    entry->flags |= SERVICE_DISABLED;
		} else {
		    entry->flags &= ~SERVICE_DISABLED;
		}
	    }
	    indr += 2;
	}    
    }
}

static void freeRMData()
{
    if (rmData) vm_deallocate(task_self(), (vm_address_t)rmData, rmDataLength);
    rmData = NULL;
    rmDataLength = 0;
    if (nonnativermData) free(nonnativermData);
    nonnativermData = NULL;
    if (rmUser) free(rmUser);
    rmUser = NULL;
}

static void getRMDataForUser(const char *user)
{
    int maxlen;
    BOOL userOk;
    NXStream *stream;
    struct stat cachest;
    struct timeval time;
    struct timezone tz;
    char buffer[MAXPATHLEN+1];

    userOk = _NXSetUser(user);

    if (!rmUser || strcmp(user, rmUser)) freeRMData();

    strcpy(buffer, user);
    if (_NXGetHomeDir(buffer)) {
	strcat(buffer, "/.NeXT/services/.cache");
    } else {
	if (userOk) _NXUnsetUser();
	return;
    }

    if (rmData && (stat(buffer, &cachest) || cachest.st_mtime > rmCacheTime)) freeRMData();

    if (!rmData) {
	stream = NXMapFile(buffer, NX_READONLY);
	if (stream) {
	    NXGetMemoryBuffer(stream, &rmData, &rmDataLength, &maxlen);
	    fixNFSData(buffer, &rmData, &rmDataLength, &maxlen);
	    rmUser = NXCopyStringBuffer(user);
	    setDisabledBits();
	    if (!stat(buffer, &cachest)) {
		rmCacheTime = cachest.st_mtime;
	    } else {
		NXLogError("pbs error: couldn't stat services cache");
		gettimeofday(&time, &tz);
		rmCacheTime = time.tv_sec;
	    }
	    NXCloseMemory(stream, NX_SAVEBUFFER);
	}
    }

    if (userOk) _NXUnsetUser();
}

/* Function to enable/disable services. */

int _NXIsServicesMenuItemEnabled(port_t server, const char *name, int nlength, const char *user, int ulength)
{
    int retval = _NXDoIsServicesMenuItemEnabled(name, user, NO);
    vm_deallocate(task_self(), (vm_address_t)name, nlength);
    vm_deallocate(task_self(), (vm_address_t)user, ulength);
    return retval;
}

int _NXDoSetServicesMenuItemEnabled(port_t server, const char *name, int nlength, const char *user, int ulength, int enabled)
{
    char *data;
    struct stat st;
    NXRMEntry *entry;
    const char *realName;
    const char *nameArg = name;
    int fd, alreadyEnabled, retval = 0;
    char filename[MAXPATHLEN+1];

    getRMDataForUser(user);

    realName = _NXGetServicesMenuItem(name, &entry);
    alreadyEnabled = _NXDoIsServicesMenuItemEnabled((realName && *realName) ? realName : name, user, (realName && *realName));
    if (alreadyEnabled > 0) {
	retval = alreadyEnabled;	/* error */
	goto done;
    }
    if ((enabled && !alreadyEnabled) || (!enabled && alreadyEnabled)) {
	strcpy(filename, user);
	if (!_NXGetHomeDir(filename)) {
	    retval = cthread_errno();
	    goto done;
	}
	strcat(filename, "/.NeXT/services");
	mkdir(filename, 0755);
	strcat(filename, "/.disabled");
	_NXSetUser(user);
	fd = open(filename, O_APPEND|O_WRONLY, 0644);
	if (fd < 0 && cthread_errno() == ENOENT) fd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0644);
	if (fd >= 0) {
	    name = (realName && *realName) ? realName : name;
	    if (write(fd, name, strlen(name)) != strlen(name)) {
		_NXUnsetUser();
		retval = cthread_errno();
		goto done;
	    }
	    if (write(fd, "\n", 1) != 1) {
		_NXUnsetUser();
		retval = cthread_errno();
		goto done;
	    }
	    if (close(fd)) {
		_NXUnsetUser();
		retval = cthread_errno();
		goto done;
	    } else {
		_NXUnsetUser();
		if (entry) {
		    if (enabled) {
			entry->flags &= ~SERVICE_DISABLED;
		    } else {
			entry->flags |= SERVICE_DISABLED;
		    }
		    free(nonnativermData);
		    nonnativermData = NULL;
		}
		if (servicesEnabledTable && realName && *realName) {
		    if (!enabled) {
			NXHashInsert(servicesEnabledTable, NXCopyStringBuffer(realName));
		    } else {
			data = NXHashRemove(servicesEnabledTable, realName);
			free(data);
		    }
		    if (!stat(filename, &st)) servicesEnabledTableModifyTime = st.st_mtime;
		}
		goto done;
	    }
	}
	_NXUnsetUser();
	retval = cthread_errno();
    }

done:

    vm_deallocate(task_self(), (vm_address_t)nameArg, nlength);
    vm_deallocate(task_self(), (vm_address_t)user, ulength);

    return retval;
}

/* Functions to update the Services Menu information. */

BOOL servicesInfoIsNew(const char *applist, char *data, int length)
{
    NXStream *stream;
    BOOL isNew = YES;
    char *filedata = NULL;
    int filelength = 0, filemaxlen;

    stream = NXMapFile(applist, NX_READONLY);
    if (stream) {
	NXGetMemoryBuffer(stream, &filedata, &filelength, &filemaxlen);
	if (data && filedata && length == filelength) {
	    isNew = NO;
	    while (length > 0) {
		length--;
		if (filedata[length] != data[length]) {
		    isNew = YES;
		    break;
		}
	    }
	}
	NXCloseMemory(stream, NX_FREEBUFFER);
    }

    return isNew;
}

BOOL noCacheFor(const char *applist)
{
    char *slash;
    char cache[MAXPATHLEN+1];

    strcpy(cache, applist);
    slash = strrchr(cache, '/');
    if (slash) {
	strcpy(slash, "/.cache");
	if (access(cache, R_OK)) return YES;
    }

    return NO;
}

void _NXUpdateRequestServers(port_t server, char *user, int ulen, char *data, int length)
{
    int pid;
    char *slash;
    NXStream *stream;
    BOOL rebuild = NO;
    char applist[MAXPATHLEN+1];
    char newapplist[MAXPATHLEN+1];

    strcpy(applist, user);
    if (_NXGetHomeDir(applist)) {
	strcat(applist, "/.NeXT/services/.applist");
	_NXSetUser(user);
	if (servicesInfoIsNew(applist, data, length)) {
	    stream = NXOpenMemory(data, length, NX_READONLY);
	    if (stream) {
		slash = strrchr(applist, '/');
		if (slash) {
		    *slash = '\0';
		    mkdir(applist, 0755);
		    *slash = '/';
		}
		strcpy(newapplist, applist);
		strcat(newapplist, ".new");
		if (!NXSaveToFile(stream, newapplist)) {
		    rebuild = YES;
		    unlink(applist);
		    link(newapplist, applist);
		    unlink(newapplist);
		}
		NXClose(stream);
	    } else {
		rebuild = noCacheFor(applist);
	    }
	} else {
	    rebuild = noCacheFor(applist);
	}
	if (rebuild) {
#ifdef DEBUG
	    NXLogError("pbs notification: forking make_services");
#endif
	    pid = fork();
	    if (!pid) {
		execl("/usr/bin/make_services", "make_services", 0);
		NXLogError("pbs error: child could not exec make_services");
		exit(-1);
	    } else if (pid < 0) {
		NXLogError("pbs error: could not fork off make_services");
	    }
	}
	_NXUnsetUser();
    }

    vm_deallocate(task_self(), (vm_address_t)data, length);
    vm_deallocate(task_self(), (vm_address_t)user, ulen);
}

/* Request made by Application */

int _NXGetServicesMenuData(port_t server, int littleEndian, data_t user, unsigned int ulen, data_t *data, unsigned int *length)
{
    int i;
    char *s;

    getRMDataForUser(user);

    if ((littleEndian && LITTLE_ENDIAN_NATIVE) || (!littleEndian && BIG_ENDIAN_NATIVE)) {
	if (data) *data = rmData;
    } else {
	if (!nonnativermData && rmData && rmDataLength) {
	    nonnativermData = (char *)malloc(rmDataLength);
	    if (nonnativermData) {
		bcopy(rmData, nonnativermData, rmDataLength);
		swapLongs(nonnativermData, 10);
		s = nonnativermData + 40;
		for (i = 0; i < ((NXRMHeader *)rmData)->numEntries; i++) {
		    swapLongs(s, 5); s += 20;
		    swapShorts(s, 4); s += 8;
		    swapLongs(s, 1); s += 4;
		    swapShorts(s, 4); s += 8;
		}
	    }
	}
	if (data) *data = nonnativermData;
    }

    if (length) *length = rmData ? rmDataLength : 0;
    
    vm_deallocate(task_self(), (vm_address_t)user, ulen);

    return KERN_SUCCESS;
}

/* From WSM */

void _NXUpdateFileExtensions(port_t server, char *data, int length)
{
    if (fileext_wsmdata && fileext_wsmlength) {
	vm_deallocate(task_self(), (vm_address_t)fileext_wsmdata, fileext_wsmlength);
    }
    fileext_wsmdata = data;
    fileext_wsmlength = length;
}

/* Request made by Application */

int _NXLoadFileExtensions(port_t server, data_t *data, unsigned int *length)
{
    *data = fileext_wsmdata;
    *length = fileext_wsmlength;
    return KERN_SUCCESS;
}

/* mig error handler */

void MsgError(kern_return_t errorCode)
{
    return;
}

/*

Modifications (starting at 2.0):

  4/6/90 pah	New for 2.0.

86
--
 6/10/90 trey	coallesced byte swapping code with rest of pbs

95
--
 10/1/90 trey	_NXGetUid maintains a one element cache

*/

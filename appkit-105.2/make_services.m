#import "afmprivate.h"
#import <appkit/nextstd.h>
#import <streams/streams.h>
#import <string.h>
#import <servers/netname.h>
#import <mach.h>
#import <libc.h>
#import <ldsyms.h>
#import <sys/errno.h>
#import <sys/loader.h>
#import <pwd.h>
#import <NXCType.h>
#import <sys/types.h>
#import <sys/dir.h>
#import <objc/NXStringTable.h>
#import "logErrorInc.c"

extern const struct section *getsectbynamefromheader();

#define BUFSIZE 256
#define MAXTYPES 16
#define MAXLANGUAGES 32

#define LITTLE_ENDIAN_NATIVE	0
#define BIG_ENDIAN_NATIVE	1

typedef struct {
    char *data;
    int length;
} RequestData;

static NXRMEntry *list = NULL;
static NXStrTable sTable = NULL;
static int rmCount = 0;
static int bufferCount;

static int runningHeadCount = 0;

static int *indirectStrings = NULL;
static int islLength, islSize;

static int rtcount = 0, stcount = 0;
static int returnTypes[MAXTYPES+1], sendTypes[MAXTYPES+1];
static int micount = 0, kecount = 0;
static int menuItems[MAXLANGUAGES+1], keyEquivalents[MAXLANGUAGES+1];

static void printRMData(char *data);

static BOOL checkHeaderMagic(char *buf, int size)
{
    struct mach_header *mhp;
    if (size < sizeof(struct mach_header)) return NO;
    mhp = (struct mach_header *)buf;
    return (mhp->magic == MH_MAGIC) && (mhp->filetype == MH_EXECUTE) && (mhp->sizeofcmds <= size);
}

static char *openMacho(const char *fileName, const char *segName, const char *sectName, int *length, char **fileBuffer, int *fileBufLen, BOOL *isADir)
{
    int fd;
    char *buf;
    struct stat info;
    char buffer[MAXPATHLEN+1];

    if (isADir) *isADir = NO;
    fd = open(fileName, O_RDONLY, 0666);
    if (fd >= 0) {
	if (fstat(fd, &info) == 0) {
	    if ((info.st_mode & S_IFMT) == S_IFDIR) {
		buf = strrchr(fileName, '.');
		if (!strcmp(buf, ".app")) {
		    close(fd);
		    strcpy(buffer, fileName);
		    strcat(buffer, "/services");
		    fd = open(buffer, O_RDONLY, 0666);
		    if (fd < 0 || fstat(fd, &info) != 0) info.st_size = 0;
		    if (isADir) *isADir = YES;
		} else {
		    info.st_size = 0;
		}
	    }
	    if (info.st_size > 0) {
		if (map_fd(fd, 0, (vm_offset_t *)&buf, TRUE, info.st_size) == KERN_SUCCESS) {
		    if (buf && checkHeaderMagic(buf, info.st_size)) {
			const struct mach_header *mhp = (struct mach_header *)buf;
			const struct section *sect = getsectbynamefromheader(mhp, (char *)segName, (char *)sectName);
			if (sect) {
			    *fileBuffer = buf;
			    *fileBufLen = info.st_size;
			    *length = sect->size;
			    close(fd);
			    return (char *)mhp + sect->offset;
			} else {
			    close(fd);
			    return NULL;
			}
		    }
		    *fileBuffer = buf;
		    *fileBufLen = info.st_size;
		    *length = info.st_size;
		    close(fd);
		    return buf;
		}
	    }
	}
	close(fd);
    }

    return NULL;
}

static void addNXRMEntry(NXRMEntry *newEntry)
{
    int i;
    NXRMEntry *entry;

    if (newEntry) {
	if (!list) {
	    bufferCount = 8;
	    list = (NXRMEntry *)malloc(bufferCount * sizeof(NXRMEntry));
	} else if (rmCount >= bufferCount) {
	    bufferCount *= 2;
	    list = (NXRMEntry *)realloc(list, bufferCount * sizeof(NXRMEntry));
	}
	newEntry->next = 0;
	newEntry->head = rmCount+1;
	newEntry->isHead = YES;
	for (i = 0; i < rmCount; i++) {
	    entry = &list[i];
	    if (entry->sendTypes == newEntry->sendTypes && entry->returnTypes == newEntry->returnTypes) {
		newEntry->head = entry->head;
		while (entry->next) entry = &list[entry->next-1];
		entry->next = rmCount+1;
		newEntry->isHead = NO;
		break;
	    }
	}
	if (i == rmCount) newEntry->head = runningHeadCount++;
	list[rmCount++] = *newEntry;
    }
}

static const char *convertType(const char *s)
{
   if (!s) return s;
   s = NXUniqueString(s);
   if (s == NXUniqueString("NXAsciiPboard")) return "NeXT plain ascii pasteboard type";
   if (s == NXUniqueString("NXPostScriptPboard")) return "NeXT Encapsulated PostScript v1.2 pasteboard type";
   if (s == NXUniqueString("NXTIFFPboard")) return "NeXT TIFF v4.0 pasteboard type";
   if (s == NXUniqueString("NXRTFPboard")) return "NeXT Rich Text Format v1.0 pasteboard type";
   if (s == NXUniqueString("NXFilenamePboard")) return "NeXT filename pasteboard type";
   if (s == NXUniqueString("NXTabularTextPboard")) return "NeXT tabular text pasteboard type";
   if (s == NXUniqueString("NXFontPboard")) return "NeXT font pasteboard type";
   if (s == NXUniqueString("NXRulerPboard")) return "NeXT ruler pasteboard type";
   if (s == NXUniqueString("NXFindPboard")) return "NeXT ruler pasteboard type";
   if (s == NXUniqueString("NXAsciiPboardType")) return "NeXT plain ascii pasteboard type";
   if (s == NXUniqueString("NXPostScriptPboardType")) return "NeXT Encapsulated PostScript v1.2 pasteboard type";
   if (s == NXUniqueString("NXTIFFPboardType")) return "NeXT TIFF v4.0 pasteboard type";
   if (s == NXUniqueString("NXRTFPboardType")) return "NeXT Rich Text Format v1.0 pasteboard type";
   if (s == NXUniqueString("NXFilenamePboardType")) return "NeXT filename pasteboard type";
   if (s == NXUniqueString("NXTabularTextPboardType")) return "NeXT tabular text pasteboard type";
   if (s == NXUniqueString("NXFontPboardType")) return "NeXT font pasteboard type";
   if (s == NXUniqueString("NXRulerPboardType")) return "NeXT ruler pasteboard type";
   if (s == NXUniqueString("NXFindPboardType")) return "NeXT ruler pasteboard type";
   return s;
}

static void sortList(int *p)
{
    int smallest, *q;

    while (*p) {
	q = p;
	smallest = *q++;
	while (*q) {
	    if (*q < smallest) {
		*p = *q;
		*q = smallest;
		smallest = *p;
	    }
	    q++;
	}
	p++;
    }
}

static int addToIndirectStringList(int *list, int count, BOOL sort)
{
    int *p, *q, *match;

    if (!count || !list) return 0;

    list[count++] = 0;
    if (sort) sortList(list);

    if (!indirectStrings) {
	islLength = 1;
	islSize = 8;
	indirectStrings = (int *)malloc(islSize * sizeof(int));
	bzero(indirectStrings, islSize * sizeof(int));
    }

    p = indirectStrings+1;
    q = list;
    while (p <= indirectStrings+islLength-count) {
	while (*p != *q && p <= indirectStrings+islLength-count) p++;
	if (p <= indirectStrings+islLength-count) {
	    match = p;
	    while (*p == *q && q < list+count) {
		p++; q++;
	    }
	    if (q == list+count) return (match-indirectStrings);
	    q = list;
	    p = match+1;
	}
    }

    if (islLength + count > islSize) {
	while (islLength + count > islSize) islSize *= 2;
	indirectStrings = (int *)realloc(indirectStrings, islSize * sizeof(int));
    }

    match = p = indirectStrings+islLength;
    islLength += count;
    while (count--) *p++ = *list++;
    
    return (match-indirectStrings);
}

static void parseLProj(const char *key, const char *directory, int *items, int *count)
{
    id table;
    DIR *dirp;
    struct direct *dp;
    const char *value;
    int language, item;
    char *s, *extension;
    char buffer[MAXPATHLEN+1];

    if (chdir(directory)) return;

    dirp = opendir(".");
    if (dirp) {
	for (dp = readdir(dirp); dp != NULL && *count < MAXLANGUAGES; dp = readdir(dirp)) {
	    if (dp->d_name && dp->d_name[0] && (dp->d_name[0] != '.')) {
		extension = strrchr(dp->d_name, '.');
		if (extension && !strcmp(extension, ".lproj")) {
		    strcpy(buffer, dp->d_name);
		    strcat(buffer, "/ServicesMenu.strings");
		    table = [NXStringTable newFromFile:buffer];
		    if (table) {
			value = [table valueForStringKey:key];
			if (value) {
			    strcpy(buffer, dp->d_name);
			    *(strrchr(buffer, '.')) = '\0';
			    s = buffer;
			    while (*s) {
				*s = NXToLower(*s);
				s++;
			    }
			    NXStringTableInsert(sTable, value, &item);
			    NXStringTableInsert(sTable, buffer, &language);
			    items[(*count)++] = language;
			    items[(*count)++] = item;
			}
			[table free];
		    }
		}
	    }
	}
	closedir(dirp);
    }
}

static char *getAttribute(char *string, NXRMEntry *entry, int *done, int findFirst, const char *directory)
{
    static int menuItemLength, keyEquivalentLength, keywordsInitialized = NO;
    static NXAtom Message, Port, Executable, Host, UserData, Timeout;
    static NXAtom SendType, ReturnType, MenuItem, KeyEquivalent, Activate, Deactivate;

    NXAtom keyword;
    int i, returnType, sendType;
    int menuItem, keyEquivalent, language, kwlength;
    char *cr, *colon, *s, *t, *key;
    char buffer[BUFSIZE];
    char path[BUFSIZE];

    if (!keywordsInitialized) {
	Message = NXUniqueString("message");
	Port = NXUniqueString("port");
	Executable = NXUniqueString("executable");
	Host = NXUniqueString("host");
	UserData = NXUniqueString("userdata");
	Timeout = NXUniqueString("timeout");
	SendType = NXUniqueString("sendtype");
	ReturnType = NXUniqueString("returntype");
	MenuItem = NXUniqueString("menuitem");
	KeyEquivalent = NXUniqueString("keyequivalent");
	Activate = NXUniqueString("activateprovider");
	Deactivate = NXUniqueString("deactivaterequestor");
	keywordsInitialized = YES;
	menuItemLength = strlen(MenuItem);
	keyEquivalentLength = strlen(KeyEquivalent);
    }

    if (done) *done = NO;
    if (!string || !*string) return string;

    cr = strchr(string, '\n');
    if (!cr) cr = string+strlen(string);
    if (cr-string > BUFSIZE) return *cr ? cr+1 : cr;
    strncpy(buffer, string, cr-string);
    buffer[cr-string] = '\0';

    key = t = s = buffer;
    colon = index(s, ':');
    if (!colon) return *cr ? cr+1 : cr;
    while (*key == ' ' || *key == '	') key++;
    while (s < colon) {
	if (NXIsLower(*s)) {
	    *t++ = *s++;
	} else if (NXIsUpper(*s)) {
	    char c = *s++;
	    *t++ = NXToLower(c);
	} else if ((*s == ' ' || *s == '	')) {
	    s++;
	} else return *cr ? cr+1 : cr;
    }
    *t = '\0';
    s++; while (*s == ' ' || *s == '	') s++;
    t = s + strlen(s) - 1;
    while (*t == ' ' || *t == '	') *t-- = '\0';

    keyword = NXUniqueString(key);
    if (keyword == Message) {
	*done = YES;
	if (findFirst) {
	    NXStringTableInsert(sTable, s, &entry->msgName);
	} else {
	    return string;
	}
    } else if (keyword == Port) {
	NXStringTableInsert(sTable, s, &entry->portName);
    } else if (keyword == Executable) {
	if (directory && *s && *s != '/') {
	    strcpy(path, directory);
	    strcat(path, "/");
	    strcat(path, s);
	}
	NXStringTableInsert(sTable, path, &entry->applicationPath);
    } else if (keyword == Host) {
	NXStringTableInsert(sTable, s, &entry->hostName);
    } else if (keyword == Activate) {
	if ((s[0] == 'N' || s[0] == 'n') && (s[1] == 'O' || s[1] == 'o') && !s[2])  entry->flags |= SERVICE_DONTACTIVATE;
    } else if (keyword == Deactivate) {
	if ((s[0] == 'N' || s[0] == 'n') && (s[1] == 'O' || s[1] == 'o') && !s[2])  entry->flags |= SERVICE_DONTDEACTIVATE;
    } else if (keyword == UserData) {
	NXStringTableInsert(sTable, s, &entry->userData);
    } else if (keyword == Timeout) {
	int timeout = 0;
	while (*s >= '0' && *s <= '9') {
	    timeout = timeout * 10 + (int)*s - (int)'0';
	    s++;
	}
	entry->timeout = timeout;
    } else if (keyword == SendType && stcount < MAXTYPES) {
	NXStringTableInsert(sTable, convertType(s), &sendType);
	if (sendType) {
	    for (i = 0; i < stcount; i++) if (sendTypes[i] == sendType) break;
	    if (i == stcount) sendTypes[stcount++] = sendType;
	}
    } else if (keyword == ReturnType && rtcount < MAXTYPES){
	NXStringTableInsert(sTable, convertType(s), &returnType);
	if (returnType) {
	    for (i = 0; i < rtcount; i++) if (returnTypes[i] == returnType) break;
	    if (i == rtcount) returnTypes[rtcount++] = returnType;
	}
    } else {
	kwlength = strlen(keyword);
	if (kwlength < BUFSIZE) {
	    if (kwlength >= menuItemLength && !strcmp(keyword+kwlength-menuItemLength, MenuItem)) {
		if (micount < MAXLANGUAGES) {
		    NXStringTableInsert(sTable, s, &menuItem);
		    if (menuItem) {
			strncpy(buffer, keyword, kwlength-menuItemLength);
			buffer[kwlength-menuItemLength] = '\0';
			NXStringTableInsert(sTable, *buffer ? buffer : SERVICES_MENU_DEFAULT_LANGUAGE, &language);
			menuItems[micount++] = language;
			menuItems[micount++] = menuItem;
			if (!*buffer && directory) parseLProj(s, directory, menuItems, &micount);
		    }
		}
	    } else if (kwlength >= keyEquivalentLength && !strcmp(keyword+kwlength-keyEquivalentLength, KeyEquivalent)) {
		if (kecount < MAXLANGUAGES) {
		    keyEquivalent = *(unsigned char *)s;
		    if (keyEquivalent) {
			strncpy(buffer, keyword, kwlength-keyEquivalentLength);
			buffer[kwlength-keyEquivalentLength] = '\0';
			NXStringTableInsert(sTable, *buffer ? buffer : SERVICES_MENU_DEFAULT_LANGUAGE, &language);
			keyEquivalents[kecount++] = language;
			keyEquivalents[kecount++] = keyEquivalent;
			if (!*buffer && directory) parseLProj(s, directory, keyEquivalents, &kecount);
		    }
		}
	    }
	}
    }

    return *cr ? cr+1 : cr;
}

static int isOk(NXRMEntry *entry)
{
    return (entry->msgName && entry->menuEntries && entry->portName && (entry->sendTypes || entry->returnTypes));
}

BOOL _NXGetHomeDir(char *home)
{
    struct passwd *pwd;
    static char *cachedHomeDirectory = NULL;

    if (home) {
	if (!cachedHomeDirectory) {
	    pwd = getpwuid(geteuid());
	    if (pwd && pwd->pw_dir) {
		cachedHomeDirectory = NXCopyStringBuffer(pwd->pw_dir);
	    } else {
		return NO;
	    }
	}
	strcpy(home, cachedHomeDirectory);
    } else {
	return NO;
    }

    return YES;
}

static void processFile(const char *file)
{
    BOOL isADir;
    NXRMEntry entry;
    int i, length, done, fileBufferLen;
    char *data, *s, *fileBuffer, *directory = NULL;
    char directoryBuffer[MAXPATHLEN+1];

    if ((data = openMacho(file, "__ICON", "__request", &length, &fileBuffer, &fileBufferLen, &isADir)) ||
	(data = openMacho(file, "__ICON", "__services", &length, &fileBuffer, &fileBufferLen, &isADir))) {
	strcpy(directoryBuffer, file);
	if (!isADir) {
	    s = strrchr(directoryBuffer, '/');
	    if (s) {
		*s = '\0';
		directory = directoryBuffer;
	    }
	} else {
	    directory = directoryBuffer;
	}
	i = 0;
	s = data;
	while (s && *s && s < data + length) {
	    micount = kecount = stcount = rtcount = 0;
	    bzero((char *)&entry, sizeof(NXRMEntry));
	    do {
		s = getAttribute(s, &entry, &done, YES, directory);
	    } while (s && *s && s < data + length && !done);
	    if (done) do {
		s = getAttribute(s, &entry, &done, NO, directory);
	    } while (s && *s && s < data + length && !done);
	    entry.sendTypes = addToIndirectStringList(sendTypes, stcount, YES);
	    entry.returnTypes = addToIndirectStringList(returnTypes, rtcount, YES);
	    entry.menuEntries = addToIndirectStringList(menuItems, micount, NO);
	    entry.keyEquivalents = addToIndirectStringList(keyEquivalents, kecount, NO);
	    if (isOk(&entry)) addNXRMEntry(&entry);
	}
	vm_deallocate(task_self(), (vm_address_t)fileBuffer, fileBufferLen);
    }

}

static int getHeads()
{
    int i, j, *heads;
    NXRMEntry *entries = list;

    if (!rmCount || !list) return 0;
    heads = (int *)malloc(rmCount * sizeof(int));
    for (j = 1, i = 0; j <= rmCount; j++, entries++) if (entries->isHead) heads[i++] = j;
    addToIndirectStringList(heads, i, NO);
    free(heads);

    return i;
}
	
static void processDirectory(char *buffer)
{   
    char *file;
    DIR *dirp;
    struct direct *dp;

    dirp = opendir(buffer);
    if (dirp) {
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	    if (dp->d_name && dp->d_name[0] && (dp->d_name[0] != '.')) {
		strcat(buffer, dp->d_name);
		processFile(buffer);
		file = strrchr(buffer, '/');
		if (file) *file = '\0';
	    }
	}
	closedir(dirp);
    }
}


static char *processRequestData(char *wsmdata, unsigned int wsmdatalength, NXStream *stream, BOOL debug)
{
    void *userData;
    NXStackType type;
    NXRMHeader header;
    char *file, *strings, *rmData = NULL, *retval = NULL;
    int headCount, dummy, stringsLength, rmDataLength = 0;
    char buffer[MAXPATHLEN+1];

    if (wsmdata && wsmdatalength && (sTable = NXCreateStringTable(NX_stackGrowContig, 256, NULL))) {
	NXStringTableInsert(sTable, "", &dummy);
	rmCount = 0;
	file = wsmdata;
	while (file < wsmdata + wsmdatalength) {
	    processFile(file);
	    file += strlen(file)+1;
	}
	if (_NXGetHomeDir(buffer)) {
	    strcat(buffer, "/.NeXT/services/");
	    processDirectory(buffer);
	}
	strcpy(buffer, "/LocalLibrary/Services/");
	processDirectory(buffer);
	if (list) {
	    NXGetStringTableInfo(sTable, &strings, &stringsLength, &dummy, &userData, &type);
	    if (strings) {
		headCount = getHeads();
		if (runningHeadCount != headCount) retval = "headcount wrong";
		rmDataLength = stringsLength + rmCount * sizeof(NXRMEntry) + sizeof(NXRMHeader) + islLength * sizeof(int);
		rmData = (char *)malloc(rmDataLength);
		if (rmData) {
		    header.version = SERVICES_MENU_VERSION;
		    header.numEntries = rmCount;
		    header.entrySize = sizeof(NXRMEntry);
		    header.entries = sizeof(NXRMHeader);
		    header.stringTableLength = stringsLength;
		    header.stringTable = header.entries + rmCount * sizeof(NXRMEntry);
		    header.indirectStringTableLength = islLength;
		    header.indirectStringTable = header.stringTable + stringsLength;
		    header.heads = header.indirectStringTable + (islLength - headCount - 1) * sizeof(int);
		    header.headCount = headCount;
		    runningHeadCount = 0;
		    *(NXRMHeader *)rmData = header;
		    bcopy((char *)list, rmData + header.entries, rmCount * sizeof(NXRMEntry));
		    bcopy(strings, rmData + header.stringTable, stringsLength);
		    if (indirectStrings) {
			bcopy((char *)indirectStrings, rmData + header.indirectStringTable, islLength * sizeof(int));
		    }
		}
		free(list); list = NULL;
		free(indirectStrings); indirectStrings = NULL;
	    }
	}
	NXDestroyStringTable(sTable);
    }

    if (rmData && rmDataLength) {
	NXWrite(stream, rmData, rmDataLength);
	if (debug) printRMData(rmData);
    }

    return retval;
}

static void istrings(int index, char *strings, int *indirects, BOOL allstrings)
{
    BOOL first = YES, dostring = YES;

    while (indirects[index]) {
	if (allstrings || dostring) {
	    if (first) {
		printf("(%d) <%d> %s", index, indirects[index], strings + indirects[index]);
		first = NO;
	    } else {
		printf(", <%d> %s", indirects[index], strings + indirects[index]);
	    }
	} else {
	    printf(", %c", (char)indirects[index]);
	}
	dostring = !dostring;
	index++;
    }
}

#define PRINT(s, i) if (i) printf(s, i, strings + i);

static void printEntry(NXRMEntry *entry, int index, char *strings, int *indirects)
{
    printf("Message #%d: <%d> %s\nPort: <%d> %s",
	index, entry->msgName, strings+entry->msgName, entry->portName, strings+entry->portName);
    PRINT("\nPath: <%d> %s", entry->applicationPath);
    PRINT("\nUser Data: <%d> %s", entry->userData);
    PRINT("\nHost: <%d> %s", entry->hostName);
    if (entry->sendTypes) {
	printf("\nSend Types: ");
	istrings(entry->sendTypes, strings, indirects, YES);
    }
    if (entry->returnTypes) {
	printf("\nReturn Types: ");
	istrings(entry->returnTypes, strings, indirects, YES);
    }
    if (entry->menuEntries) {
	printf("\nMenu Items: ");
	istrings(entry->menuEntries, strings, indirects, YES);
    }
    if (entry->keyEquivalents) {
	printf("\nKey Equivalents: ");
	istrings(entry->keyEquivalents, strings, indirects, NO);
    }
    if (entry->timeout) printf("\nTimeout: %d", entry->timeout);
    printf("\nisHead: %s\nNext: %d\nHead: %d\n\n", entry->isHead ? "YES" : "NO", entry->next, entry->head);
}

static void printRMData(char *rmData)
{
    int i;
    NXRMHeader *header = (NXRMHeader *)rmData;

    printf("Number of entries: %d\n", header->numEntries);
    printf("Head count: %d, heads:", header->headCount);
    for (i = 0; i < header->headCount; i++) printf(" %d", *((int *)(rmData + header->heads) + i));
    printf("\n\n");
    for (i = 0; i < header->numEntries; i++) {
	printEntry((NXRMEntry *)(rmData+header->entries+(i*header->entrySize)), i+1,
		    rmData+header->stringTable, (int *)(rmData+header->indirectStringTable));
    }
}

/* Main routine. */

void main(int argc, char *argv[])
{
    BOOL debug;
    struct stat st;
    char *error, *data;
    struct timezone tz;
    struct timeval time;
    int fd, length, maxlen;
    NXStream *instream, *outstream;
    char buffer[MAXPATHLEN+1];
    char lock[MAXPATHLEN+1];

    if (argc > 2) {
	NXLogError("make_services: usage: make_services\n");
	exit(1);
    }

    debug = (argc == 2);

    if (!_NXGetHomeDir(buffer)) {
	NXLogError("make_services: couldn't get home directory.\n");
	exit(1);
    }

    strcpy(lock, buffer);
    strcat(lock, "/.NeXT/services/.lock");
    strcat(buffer, "/.NeXT/services/.applist");

    if (access(buffer, R_OK)) {
	NXLogError("make_services: cannot read services applist file.\n");
	exit(1);
    }

    if (!(instream = NXMapFile(buffer, NX_READONLY))) {
	NXLogError("make_services: cannot create input stream.\n");
	exit(1);
    }

    fd = open(lock, O_CREAT|O_EXCL|O_WRONLY, 0644);
    if (fd < 0) {
	if (!stat(lock, &st)) {
	    gettimeofday(&time, &tz);
	    if (time.tv_sec - st.st_mtime > 60) {
		unlink(lock);
		fd = open(lock, O_CREAT|O_EXCL|O_WRONLY, 0644);
		if (fd < 0) {
		    NXLogError("make_services: error creating lock file.\n");
		    exit(1);
		}
	    } else {
		NXLogError("make_services: someone else is already making services.\n");
		exit(1);
	    }
	} else {
	    NXLogError("make_services: can't stat lock file.\n");
	    exit(1);
	}
    }

    if (outstream = NXOpenFile(fd, NX_WRITEONLY)) {
	NXGetMemoryBuffer(instream, &data, &length, &maxlen);
	error = processRequestData(data, length, outstream, debug);
	if (error) NXLogError("make_services: %s.\n", error);
	NXClose(outstream);
	if (close(fd)) {
	    unlink(lock);
	    NXLogError("make_services: couldn't close file.");
	    exit(1);
	}
    } else {
	close(fd);
	unlink(lock);
	NXLogError("make_services: couldn't create output stream.");
	exit(1);
    }

    strcpy(buffer, lock);
    strcpy(strrchr(buffer, '/'), "/.cache");
    unlink(buffer);
    link(lock, buffer);
    unlink(lock);

    NXClose(instream);
    vm_deallocate(task_self(), (vm_address_t)data, maxlen);
}

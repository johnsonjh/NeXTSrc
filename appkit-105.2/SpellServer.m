#import "SpellServer.h"
#define c_plusplus 1
#import <mach.h>
#import <netname_defs.h>
#import <servers/netname.h>
#import <signal.h>
#import <stdio.h>
#import <stdlib.h>
#import <sys/file.h>
#import <sys/ioctl.h>
#import <sys/message.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <syslog.h>
#import <string.h>
#import <libc.h>
#import <pwd.h>
#import "ss.h"
#import "Dictionary.h"

extern int map_fd();

SpellServer *ssServer = nil;
static UserInfo *gUserInfo = nil;
extern NXHashTablePrototype userProto;
  /* constains UserInfo, and hashes on the name field */
  
#define GUESSBUFSIZE 3

@implementation SpellServer

+ new
{
    id dict;
    if (ssServer)
	return ssServer;
    self = [super new];
    ssServer = self;
    dict = [Dictionary newResource:"/usr/lib/NextStep"];
    return self;
}

- fork
{
    int fd;
    signal(SIGHUP, SIG_IGN);
    if (fork())
	exit(0);

    /*
     * Close file descriptors
     */
    for (fd = getdtablesize(); fd >= 0; fd--)
	close(fd);
    open("/", 0);
    dup2(0, 1);
    dup2(0, 2);
    fd = open("/dev/tty", 3);
    if (fd >= 0) {
	(void) ioctl(fd, (int) TIOCNOTTY, (char *)0);
	(void) close(fd);
    }
    return self;
}

- initServer
{
    openlog("spell", LOG_NOWAIT|LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "spell started");
    ret = port_allocate(task_self(), &server_port);
    if (ret != KERN_SUCCESS)
	[self errmsg:"port_allocate" fatal:YES];
    ret = port_set_allocate(task_self(), &notify_set);
    if (ret != KERN_SUCCESS)
	[self errmsg:"port_set_allocate" fatal:YES];
    ret = port_set_add(task_self(), notify_set, server_port);
    if (ret != KERN_SUCCESS)
	[self errmsg:"port_set_add" fatal:YES];
    ret = port_allocate(task_self(), &notify_port);
    if (ret != KERN_SUCCESS)
	[self errmsg:"port_allocate" fatal:YES];
    ret = port_set_add(task_self(), notify_set, notify_port);
    if (ret != KERN_SUCCESS)
	[self errmsg:"port_set_add" fatal:YES];
    task_set_notify_port(task_self(), notify_port);
    ret = netname_check_in(
	name_server_port, SS_NAME, PORT_NULL, server_port);
    if (ret != KERN_SUCCESS)
	[self errmsg:"name_server_port" fatal:YES];
    return self;
}

- serverLoop
{
    struct message_t  msg, reply;
    boolean_t       foundMsg;

    while (TRUE) {
	msg.head.msg_local_port = notify_set;
	msg.head.msg_size = sizeof(struct message_t);
	ret = msg_receive(&msg.head, MSG_OPTION_NONE, 0);
	if (ret != RCV_SUCCESS) {
	    [self errmsg:"msg_receive" fatal:NO];
	    continue;
	}
	if (msg.head.msg_id == NOTIFY_PORT_DELETED) {
	    [self cleanUp:((notification_t *) &msg)];
	    continue;
	}
	foundMsg = ss_server(&msg, &reply);
	if (foundMsg != TRUE)
	    [self errmsg:"ss_server" fatal:NO];
	else  {
	    reply.head.msg_local_port = server_port;
	    reply.head.msg_remote_port = msg.head.msg_remote_port;
	    ret = msg_send(&reply.head, MSG_OPTION_NONE, 0);
	    if (ret != SEND_SUCCESS) {
		[self errmsg:"msg_send" fatal:NO];
		continue;
	    }
	}
    }
    return self;
}

- cleanUp:(notification_t *) msg
{
    NXHashState	state;
    UserInfo *info;

    state = NXInitHashState(userInfo);
    while (NXNextHashState(userInfo, &state, &((void *)info))) {
	if ([info containsPort:msg->notify_port]) {
	    [info removePort:msg->notify_port];
	    return self;
	}
    }
    return self;
}

- errmsg:(char *)str fatal:(BOOL)fatal
{
    syslog(fatal ? LOG_EMERG : LOG_CRIT, "%s error: %d\n", str, ret);
    if (fatal)
	exit(-1);
    return self;
}

- validInfo:(int)handle
{
    NXHashState	state;
    UserInfo *info;
    UserInfo *result = (UserInfo *)handle;
    if (!userInfo)
	return NO;
    state = NXInitHashState(userInfo);
    while (NXNextHashState(userInfo, &state, &((void *)info))) {
	if (info == result)
	    return result;
    }
    return nil;
}

- openSpellingFor:(const char *)name port:(port_t)notify 
    path:(const char *)path
{
    UserInfo *info;
    
    if (!userInfo) 
	userInfo = NXCreateHashTable(userProto, 0, 0);
    info = NXHashGet(userInfo, [UserInfo gUserInfo:name path:path]);
    if (!info) {
	info = [UserInfo new];
	[info initName:name path:path];
	NXHashInsert(userInfo, info);
    }
    [info addPort:notify];
    return info;
}


@end



@implementation UserInfo

+ gUserInfo:(const char *)theName path:(const char *)thePath
{
    if (!gUserInfo)
	gUserInfo = [self new];
    gUserInfo->name = (char *)theName;
    gUserInfo->path = (char *)thePath;
    return gUserInfo;
}

- addPort:(port_t)port
{
    void *portInfo;
    if (!portList) 
	portList = NXCreateHashTable(NXPtrPrototype, 0, 0);
    portInfo = NXHashGet(portList, (void *)port);
    if (portInfo)
	return self;
    NXHashInsert(portList, (void *)port);
    port_set_add(task_self(), ssServer->notify_set, port); 
    return self;
}

- removePort:(port_t)port
{
    port_set_remove(task_self(), port); 
    NXHashRemove(portList, (void *)port);
    return self;
}

- (BOOL) containsPort:(port_t)port
{
    if (!portList)
	return NO;
    return NXHashGet(portList, (void *)port) ? YES : NO;
}

- inDict:(const char *)word
{
    if (!localDict)
	[self openLocal];
    return NXHashGet(localDict, word) ? self : nil;
}

- initName:(const char *)theName path:(const char *)thePath
{
    name = strcpy(malloc(strlen(theName) + 1), theName);
    path = strcpy(malloc(strlen(thePath) + 1), thePath);
    gettimeofday(&lastAccess, 0);
    proc = [self methodFor:@selector(inDict:)];
    guessSize = GUESSBUFSIZE * vm_page_size;
    vm_allocate(task_self(), &((vm_address_t) guesses), guessSize, 1);
    return self;
}

- free
{
    free(name);
    free(path);
    if (file)
	fclose(file);
    NXFreeHashTable(localDict);
    return [super free]; 
}

- openLocal
{
    char buf[FILENAME_MAX];
    int fd;
    struct stat info;
    char *last, *cur, *old;
    struct passwd *pwd;
    
    pwd = getpwnam(name);
    if (!pwd)
	return nil;
    strcpy(buf, pwd->pw_dir);
    strcat(buf, "/.NeXT/");
    strcat(buf, path);
    localDict = NXCreateHashTable(NXStrPrototype, 0, 0);
    fd = open(buf, O_RDONLY, 0660);
    if (fd >= 0) {
	if (fstat(fd, &info) >= 0) {
	    mappedWordLength = info.st_size;
	    map_fd(fd, 0, (vm_offset_t *)&mappedWords, 1, mappedWordLength);
	    close (fd);
	    last = mappedWords + mappedWordLength;
	    for (cur = mappedWords; cur < last; cur += strlen(cur) + 1) {
		old = NXHashGet(localDict, cur);
		if (old)
		    NXHashRemove(localDict, cur);
		else
		    NXHashInsert(localDict, cur);
	    }
	}
    }
    file = fopen(buf, "a");
    return self;
}

- learnWord:(const char *)word
{
    char *copy;
    int length;
    if (!localDict)
	[self openLocal];
    length = strlen(word) + 1;
    copy = strcpy(malloc(length), word);
    lowerit(copy);
    if (NXHashGet(localDict, copy))
	return self;
    NXHashInsert(localDict, copy);
    if (file) {
	fwrite(copy, sizeof(char), length, file);
	fflush(file);
	fsync(fileno(file));
    }
    return self;
}

- forgetWord:(const char *) word
{
    char lower[MAX_WORD_LENGTH];
    if (!localDict)
	[self openLocal];
    strcpy(lower, word);
    lowerit(lower);
    if (!NXHashGet(localDict, lower))
	return self;
    NXHashRemove(localDict, lower);
    if (file) {
	fwrite(lower, sizeof(char), strlen(lower) + 1, file);
	fflush(file);
	fsync(fileno(file));
    }
    return self;
}

- spellCheck:(char *) text
    length:(unsigned int)length
    start:(int *)startPos
    end:(int *)endPos
{
    return [[Dictionary new] findMisspelled:text length:length 
	info:self loc1:startPos loc2:endPos] ? self:nil;
}

- (int)guess:(const char *)word guesses:(data_t *)ptrGuess 
    length:(unsigned int *)guessesCnt
{
    int count;
    count = guess(word, self);
    *ptrGuess = guesses;
    *guessesCnt = curGuess - guesses;
    return count;
}

@end


static unsigned int hashInfo(const void *info, const void *data)
{
    const UserInfo *p = data;
    return NXStrHash(info, p->name) + NXStrHash(info, p->path);
}

static int infoEqual(const void *info, const void *data1, const void *data2)
{
    const UserInfo *p1 = data1;
    const UserInfo *p2 = data2;
    int val;
    val = NXStrIsEqual(info, p1->name, p2->name);
    if (val) {
	val = NXStrIsEqual(info, p1->path, p2->path);
    }
    return val;
}

static void freeInfo(const void *info, void *data)
{
    UserInfo *p = data;
    [p free];
}

static NXHashTablePrototype userProto = {
    hashInfo,
    infoEqual,
    freeInfo,
    0
};








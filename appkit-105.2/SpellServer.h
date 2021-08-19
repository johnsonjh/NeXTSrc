#import <objc/Object.h>
#import <mach.h>
#import <mig_errors.h>
#import <msg_type.h>
#import <sys/notify.h>
#import <stdio.h>
#import <objc/hashtable.h>
#import <sys/time.h>
#import "ss.h"

@interface UserInfo:Object
{
@public
    char *name;
    char *path;
    FILE *file;
    port_t notify;
    NXHashTable *localDict;
    NXHashTable *portList;
    char *mappedWords;
    int mappedWordLength;
    IMP proc;
    char *guesses, *curGuess;
    int guessSize;
    struct timeval lastAccess;
}
+ gUserInfo:(const char *)theName path:(const char *)thePath;
- initName:(const char *)theName path:(const char *)thePath;
- addPort:(port_t)port;
- removePort:(port_t)port;
- (BOOL) containsPort:(port_t)port;
- learnWord:(const char *)word;
- forgetWord:(const char *) word;
- spellCheck:(char *) text    length:(unsigned int)length    start:(int *)startPos    end:(int *)endPos;
- (int)guess:(const char *)word guesses:(data_t *)ptrGuess     length:(unsigned int *)guessesCnt;
- openLocal;
@end

#define NUSERS 3

@interface SpellServer:Object
{
@public
    port_t server_port, notify_port;
    kern_return_t ret;
    port_set_name_t notify_set;
    NXHashTable *userInfo;
}

- fork;
- initServer;
- serverLoop;
- cleanUp:(notification_t *) msg;
- errmsg:(char *)str fatal:(BOOL)fatal;
- validInfo:(int)handle;
- openSpellingFor:(const char *)name port:(port_t)notify     path:(const char *)path;
@end

extern boolean_t ss_server();	/* defined by Mig */
struct message_t {
    msg_header_t    head;	/* standard header field */
    char            buf[4096];
};

extern id ssServer;













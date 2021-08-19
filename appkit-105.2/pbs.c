
/*
	pbs.c
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	This file holds the main loop and stub implementations of the
	pasteboard server.
*/

#import <mach.h>
extern port_t   name_server_port;

/*
#include <mach_error.h>
*/
#import "afmprivate.h"
#import "perfTimer.h"
#import <mig_errors.h>
#import <sys/message.h>
#import <msg_type.h>
#import <signal.h>
#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import <syslog.h>
#import <sys/ioctl.h>
#import <servers/netname.h>
#import <sys/time_stamp.h>
#import <sys/time.h>
#import <sys/file.h>
#import <sys/wait.h>
#import <syslog.h>
#import <objc/hashtable.h>
#import <kern/sched.h>
/*	we cant use these prototypes since we added the waitTime args
#import "pbs.h"
*/
#import "app.h"
#import "pbtypes.h"

#define DNON_KIT_TIMING
#include "perfTimerInc.m"

extern fork();
extern close();
extern open();
extern dup2();
extern ioctl();
extern close();
extern getdtablesize();
extern kern_timestamp();
extern convert_ts_to_tv();

extern boolean_t pbs_server();	/* defined by Mig */

typedef struct {
    msg_header_t	head;
    msg_type_t		retcodetype;
    kern_return_t	return_code;
    char		buf[600];
} PBMessage;

typedef struct {
    char *typename;
    char *data;
    int length;
    char mustFree;
    char hasData;
} pbentry;

typedef struct {
    char *name;			/* name for this Pasteboard */
    int numEntries;		/* number of current scrap types */
    pbentry *entries;		/* actual scrap data */
    char *names;		/* vm_allocated page from setTypes */
    int nameLength;		/* length of above data */
    int session;		/* current change version */
    port_t owner;		/* where to ask for types */
    int ownerData;		/* blind data passed back to owner */
} PBInfo;

typedef struct _PBRequest {  /* request for promised data */
    int expiration;		/* time this request expires in sec */
    PBInfo *info;		/* pasteboard this request if for */
    char *type;			/* type desired */
    int session;		/* session count */
    PBMessage *query;		/* msg that asked us for the data */
    struct _PBRequest *next;
} PBRequest;


static void handleMsg(PBMessage *inMsg);

static void errmsg(char *str, int val, int fatal);
static void server_loop(void);
static void freeInfo(PBInfo *info);
static void buildEntries(PBInfo *info);
static pbentry *lookupType(char *typeName, PBInfo *info);
static PBInfo *lookupPasteboard(const char *name);
static int getDataCommon(int doBlock, pbtype_t pbName, pbtype_t pbType, int ourSession, data_t *data, unsigned int *dataCnt, int *session);
static void addRequest(PBInfo *info, char *type, int session);
static void freeRequest(PBRequest *req);
static void clearRequests(void);
static void checkRequests(void);
static void freeRequestsForPB(PBInfo *badInfo);
static void getMsgParams(msg_option_t *option, msg_timeout_t *wait);
static int getTime(void);
static int addTimes(int t1, int t2);
static int subTimes(int t1, int t2);
static void setupPriorities(void);
static void portInfo(port_t port);
#ifdef DEBUG
static void handle_signal(int);
#endif
static void handle_death_signal(int);

static PBMessage MsgBuffer = {0};
static PBMessage *InMsg = &MsgBuffer;	/* last message recieved */
int SendReply;			/* do we reply to the current message? */

int PrintTimes = FALSE;
#ifdef DEBUG
int DoMarks = TRUE;
#else
int DoMarks = FALSE;
#endif

static port_t ServerPort;
static PBRequest *Requests = NULL;
static void *ToBeFreed = NULL;		/* pointer for delayed free hack */
static NXHashTable *PasteBoardTable;


#include "logErrorInc.c"

int main(int argc, char *argv[])
{
    kern_return_t ret;
    char *debugPortName = NULL;
    int mask = 15;
    int freeFontData = FALSE;

    (void)signal(SIGHUP, SIG_IGN);
    setupPriorities();
    if (argc == 1)
	if (fork())
	    exit(0);

    _NXInitAFMModule();
    (void)signal(SIGCHLD, handle_death_signal);

#ifdef DEBUG
    if (DoMarks) {
	INITMARKTIME(0);
	(void)signal(SIGUSR1, &handle_signal);
    }
#endif

    /*
     * Close file descriptors
     */
     if (argc == 1) {
#ifndef DEBUG
	int fd;

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
#endif
    } else {
	++argv;
	while (*argv)
	    if (!strcmp(argv[0], "-n")) {
		debugPortName = argv[1];
		argv += 2;
	    } else if (!strcmp(argv[0], "-f")) {
		while (*++argv) {
		    parseAFMFile(*argv, NULL, mask);
		    if (freeFontData) {
			if (CurrWidthsStack)
			    NXDestroyStack(CurrWidthsStack);
			if (CurrDataStack)
			    NXDestroyStack(CurrDataStack);
			if (CurrCCStack)
			    NXDestroyStack(CurrCCStack);
			if (CurrStringTable)
			    NXDestroyStringTable(CurrStringTable);
			CurrWidthsStack = CurrDataStack = CurrCCStack = NULL;
			CurrStringTable = NULL;
		    }
		}
		exit(0);
	    } else if (!strcmp(argv[0], "-m")) {
		mask = atoi(argv[1]);
		argv += 2;
	    } else if (!strcmp(argv[0], "-t")) {
		PrintTimes = TRUE;
		++argv;
#ifdef DEBUG
	    } else if (!strcmp(argv[0], "-F")) {
		freeFontData = TRUE;
		++argv;
	    } else if (!strcmp(argv[0], "-max")) {
		WidthsMaxSize = atoi(argv[1]);
		FullDataMaxSize = atoi(argv[1]);
		StringsMaxSize = atoi(argv[1]);
		argv += 2;
	    } else if (!strcmp(argv[0], "-maxW")) {
		WidthsMaxSize = atoi(argv[1]);
		argv += 2;
	    } else if (!strcmp(argv[0], "-maxD")) {
		FullDataMaxSize = atoi(argv[1]);
		argv += 2;
	    } else if (!strcmp(argv[0], "-maxS")) {
		StringsMaxSize = atoi(argv[1]);
		argv += 2;
#endif
	    } else {
		++argv;
	    }
    }

    openlog("pbs", LOG_NOWAIT|LOG_CONS, LOG_USER);
    ret = port_allocate(task_self(), &ServerPort);
    if (ret != KERN_SUCCESS)
	errmsg("port_allocate", ret, TRUE);
    if (!debugPortName) {
	ret = netname_check_in(name_server_port, PBS_NAME, PORT_NULL, ServerPort);
	if (ret != KERN_SUCCESS)
	    errmsg("netname_check_in", ret, TRUE);
    } else {
	ret = netname_check_in(name_server_port, debugPortName, PORT_NULL, ServerPort);
	if (ret != KERN_SUCCESS)
	    errmsg("netname_check_in", ret, TRUE);
    }

    PasteBoardTable = NXCreateHashTable(NXStrStructKeyPrototype, 1, NULL);
    server_loop();
    return 0;
}

static void server_loop()
{
    kern_return_t ret;
    msg_timeout_t wait;
    msg_option_t option;

    while (TRUE) {
	checkRequests();
	InMsg->head.msg_local_port = ServerPort;
	InMsg->head.msg_size = sizeof(PBMessage);
	getMsgParams(&option, &wait);
	ret = msg_receive(&(InMsg->head), option, wait);
	if (ret == RCV_TIMED_OUT)
	    /* go around again, letting requests get checked */;
	else if (ret == RCV_SUCCESS)
	    handleMsg(InMsg);
	else
	    errmsg("msg_receive", ret, FALSE);
    }
}

/* handle a message:  dispatch is to the right stub, and send the reply */
static void handleMsg(PBMessage *inMsg)
{
    PBMessage reply;
    kern_return_t ret;
    int msgId;

    SendReply = TRUE;		/* assume proc wants us to send a reply */
    MARKTIME2(DoMarks, "handleMsg %d", inMsg->head.msg_id, 0, 0);
    msgId = inMsg->head.msg_id;
    if (pbs_server(inMsg, &reply)) {
	if (SendReply && reply.return_code != MIG_NO_REPLY) {
	    MARKTIME(DoMarks, "getting next message", 0);
	    reply.head.msg_local_port = ServerPort;
	    reply.head.msg_remote_port = inMsg->head.msg_remote_port;
	    if (reply.head.msg_size > sizeof(PBMessage))
		errmsg("handleMsg: reply message too big", reply.head.msg_size, FALSE);
	    ret = msg_send((msg_header_t *)&reply, MSG_OPTION_NONE, 0);
	    if (ret != SEND_SUCCESS) {
		NXLogError("msg_send error: %d, with msg_id = %d", ret, msgId);
#ifdef DEBUG
		if (ret == SEND_INVALID_PORT) {
		    portInfo(reply.head.msg_local_port);
		    portInfo(reply.head.msg_remote_port);
		}
#endif
	    }
	    if (ToBeFreed) {
		free(ToBeFreed);
		ToBeFreed = NULL;
	    }
	}
    } else {
	MARKTIME(DoMarks, "getting next message after error", 0);
	errmsg("pbs_server() failed", 0, FALSE);
    }
}

int _NXSetTypes(port_t server, pbtype_t pbName, port_t client, int data,
		data_t typeList, unsigned int typeListCnt, int numTypes,
		int *session)
{
    PBInfo *info;
    int oldCount;

    info = lookupPasteboard(pbName);
    oldCount = info->session;
    freeInfo(info);
    info->numEntries = numTypes;
    info->names = typeList;
    info->nameLength = typeListCnt;
    info->session = oldCount + 1;
    info->owner = client;
    info->ownerData = data;
    clearRequests();
    buildEntries(info);
    *session = info->session;
    return PBS_OK;
}


static void freeInfo(PBInfo *info)
{
    pbentry *cur, *last;

    if (info->numEntries > 0) {
	last = info->entries + info->numEntries;
	for (cur = info->entries; cur < last; cur++) {
	    if (cur->data) {
	        if (cur->mustFree)
		    free(cur->data);
		else
		    vm_deallocate(task_self(), (vm_address_t)cur->data, cur->length);
	    }
	}
	free(info->entries);
	vm_deallocate(task_self(), (vm_address_t)info->names, info->nameLength);
    }
}

static void buildEntries(PBInfo *info)
{
    pbentry *cur;
    char *curText = info->names;
    char *lastText = curText + info->nameLength;
    int i;

    info->entries = malloc(info->numEntries * sizeof(pbentry));
    cur = info->entries;
    for (i = 0; i < info->numEntries; i++) {
        cur->typename = curText;
	cur->data = 0;
	cur->length = 0;
	cur->mustFree = FALSE;
	cur->hasData = FALSE;
	while (curText < lastText) 
	    if (!(*curText++))
		break;
	cur++;
    }
}

int _NXGetTypes(port_t server, pbtype_t pbName, data_t *typeList,
			unsigned int *typeListCnt, int *numTypes, int *session)
{
    PBInfo *info;

    info = lookupPasteboard(pbName);
    *typeList = info->names;
    *typeListCnt = info->nameLength;
    *numTypes = info->numEntries;
    *session = info->session;
    return PBS_OK;
}

int _NXGetSession(port_t server, pbtype_t pbName, int *session)
{
    PBInfo *info;

    info = lookupPasteboard(pbName);
    *session = info->session;
    return PBS_OK;
}

int _NXSetData(port_t server, pbtype_t pbName, int ourSession, pbtype_t pbType,
		data_t data, unsigned int dataCnt, int *session)
{
    PBInfo *info;
    pbentry *cur;
    int i;
    
    info = lookupPasteboard(pbName);
    *session = info->session;
    if (info->session != ourSession) {
	vm_deallocate(task_self(), (vm_address_t)data, dataCnt);
        return PBS_OOSYNC;
    }
    if (!pbType || !pbType[0]) {
	vm_deallocate(task_self(), (vm_address_t)data, dataCnt);
        return ILLEGAL_TYPE;
    }
    for (i = info->numEntries, cur = info->entries; i--; cur++)
        if (cur->typename && !strcmp(cur->typename, pbType)) {
	   cur->data = data;
	   cur->length = dataCnt;
	   cur->mustFree = FALSE;
	   cur->hasData = TRUE;
	   return PBS_OK; 
	}
    vm_deallocate(task_self(), (vm_address_t)data, dataCnt);
    return TYPE_NOT_FOUND;
}

int _NXNonBlockingGetData(port_t server, pbtype_t pbName, pbtype_t pbType,
					int ourSession,
					data_t *data, unsigned int *dataCnt,
					int *session)
{
    return getDataCommon(FALSE, pbName,  pbType, ourSession, data, dataCnt, session);
}

int _NXBlockingGetData(port_t server, pbtype_t pbName, pbtype_t pbType,
				int ourSession,
				data_t *data, unsigned int *dataCnt,
				int *session)
{
    return getDataCommon(TRUE, pbName, pbType, ourSession, data, dataCnt, session);
}

static int getDataCommon(int doBlock, pbtype_t pbName, pbtype_t pbType, int ourSession, data_t *data, unsigned int *dataCnt, int *session)
{
    PBInfo *info;
    pbentry *cur;
    int ret;
    
    info = lookupPasteboard(pbName);
    *session = info->session;
    *data = 0;
    *dataCnt = 0;
    if (!pbType || !pbType[0])
	ret = ILLEGAL_TYPE;
    else if (info->session != ourSession)
	ret = PBS_OOSYNC;
    else if (cur = lookupType(pbType, info)) {
	if (cur->hasData) {
	    *data = cur->data;
	    *dataCnt = cur->length;
	    ret = PBS_OK;
	} else {
	    if (doBlock) {
	      /* Try to ask the owner for promised data.  If we cant even
	       * send him the request message, return OOSYNC, since that is
	       * an error that pasters deal with gracefully (after all, its not
	       * their fault that the copier died or hung).
	       */
		if (_NXRequestData(TIMEOUT*1000, info->owner, info->session,
				info->ownerData, pbType) == SEND_SUCCESS) {
		    addRequest(info, pbType, ourSession);
		    SendReply = FALSE;	/* dont send reply to client */
		    ret = DATA_DELAYED;	/* this return val shouldnt matter */
		} else
		    ret = PBS_OOSYNC;
	    } else
		ret = DATA_DELAYED;
	}
    } else
	ret = TYPE_NOT_FOUND;
    return ret;
}

int _NXFreePasteboard(port_t server, pbtype_t pbName)
{
    PBInfo *info;

    info = NXHashGet(PasteBoardTable, &pbName);
    if (info) {
	freeRequestsForPB(info);
	freeInfo(info);
	NXHashRemove(PasteBoardTable, info);
	free(info);
    }
    return PBS_OK;
}

static PBInfo *lookupPasteboard(const char *name)
{
    PBInfo *info;

    info = NXHashGet(PasteBoardTable, &name);
    if (!info) {
	info = calloc(1, sizeof(PBInfo));
	info->name = NXCopyStringBuffer(name);
	(void)NXHashInsert(PasteBoardTable, info);
    }
    return info;
}

/* looks for the entry for a given type */
static pbentry *lookupType(char *typeName, PBInfo *info)
{
    pbentry *curr;
    int i;
        
    for (curr = info->entries, i = info->numEntries; i--; curr++)
        if (curr->typename && !strcmp(curr->typename, typeName))
	    return curr;
    return NULL;
}


void _NXPBSRendezVous(port_t server, int kitVersion, int *pbsVersion)
{
    *pbsVersion = atoi(PBS_VERS);	/* passed in from make */
}


#ifdef DEAD_CODE_FOR_NOW
/* for debugging, add this line to pbs.defs
function _NXPrintpb(server: port_t) : int;
*/

int _NXPrintpb(port_t server)
{
    PBInfo *info;
    pbentry *cur;
    int i;
    char buf[100];
    
    info = lookupPasteboard(pbName);
    fprintf(stderr, "number of scrap types: %d\n", info->numEntries);
    fprintf(stderr, "session: %d\n", info->session);
    fprintf(stderr, "type data:\n");
    cur = info->entries;
    for (i = 0; i < info->numEntries; i++) {
	fprintf(stderr, "  entry %d\n", i);
        if (cur->typename)
	    fprintf(stderr, "    type: %s\n", cur->typename);
	fprintf(stderr, "    length: %d\n", cur->length);
	strncpy(buf, cur->data, 40);
	buf[40] = 0;
	fprintf(stderr, "    data: %s\n", buf);
	fprintf(stderr, "    data: ...\n");
	strncpy(buf, cur->data + (cur->length - 40), 40);
	buf[40] = 0;
	fprintf(stderr, "    data: %s\n", buf);
	cur++;
    }
    return PBS_OK;
}
#endif

static void addRequest(PBInfo *info, char *type, int session)
{
    PBRequest *new;

    new = malloc(sizeof(PBRequest));
    new->expiration = addTimes(getTime(), TIMEOUT);
    new->info = info;
    new->type = NXCopyStringBuffer(type);
    new->session = session;
    new->query = malloc(InMsg->head.msg_size);
    bcopy(InMsg, new->query, InMsg->head.msg_size);
    new->next = Requests;
    Requests = new;
}

static void freeRequest(PBRequest *req)
{
    free(req->type);
    free(req->query);
    free(req);
}

static void clearRequests()
{
    PBRequest *r, *next;

    for(r = Requests; r; ) {
	next = r->next;
	freeRequest(r);
	r = next;
    }
    Requests = NULL;
}

/* check all requests to see if either they've timed out, if something
   new has been put in the scrap, or if the request has been satisfied.
 */
static void checkRequests()
{
    PBRequest *curr, **currPtr;
    int nukeIt;
    PBInfo *info;
    pbentry *entry;
    
    for(currPtr = &Requests; *currPtr;) {
	nukeIt = FALSE;
	curr = *currPtr;
	info = curr->info;
	if (curr->session != info->session) {
	    nukeIt = TRUE;
	} else {
	    entry = lookupType(curr->type, info);
	    if (entry && entry->hasData) {
		handleMsg(curr->query);
		nukeIt = TRUE;
	    } else if (subTimes(getTime(), curr->expiration) > 0)
		nukeIt = TRUE;
	}
	if (nukeIt) {
	    *currPtr = curr->next;	/* remove him from this list */
	    freeRequest(curr);
	} else
	    currPtr = &((*currPtr)->next);	/* bop to next element */
    }
}

/* frees all requests that reference a certain pasteboard. */
static void freeRequestsForPB(PBInfo *badInfo)
{
    PBRequest *curr, **currPtr;
    
    for(currPtr = &Requests; *currPtr;) {
	curr = *currPtr;
	if (curr->info == badInfo) {
	    *currPtr = curr->next;	/* remove him from this list */
	    freeRequest(curr);
	} else
	    currPtr = &(curr->next);	/* bop to next element */
    }
}

/* determines how long to wait before the next request times out */
static void getMsgParams(msg_option_t *option, msg_timeout_t *wait)
{
    PBRequest *r;
    int diff;
    int now;

    *option = MSG_OPTION_NONE;
    now = getTime();
    for(r = Requests; r; r = r->next) {
	diff = subTimes(r->expiration, now);
	if (*option == MSG_OPTION_NONE || now < *wait) {
	    *option = RCV_TIMEOUT;
	    *wait = diff;
	}
    }
}


/* gets the current time in seconds */
static int getTime()
{
    struct tsval ts;
    struct timeval tv;

    kern_timestamp(&ts);
    convert_ts_to_tv(TS_FORMAT, &ts, &tv);
    return tv.tv_sec;
}


/* adds two times, taking into account the fact that they wrap */
static int addTimes(int t1, int t2)
{
#ifdef NeXT
    return t1 + t2;
#else
    intentional compiler error: timestamp wrapping is system dependent
#endif
}

/* subtracts two times, taking into account the fact that they wrap */
static int subTimes(int t1, int t2)
{
#ifdef NeXT
    return t1 - t2;
#else
    intentional compiler error: timestamp wrapping is system dependent
#endif
}


/* implements a delayed free machanism, for freeing space after the reply message is sent */
void freeAfterReply(void *data)
{
    ToBeFreed = data;
}

static void errmsg(char *str, int val, int fatal)
{
    NXLogError("%s error: %d", str, val);
    if (fatal)
	exit(-1);
}


static void handle_death_signal(int i)
{
    int	pid;
    
    do {
	pid = wait3(NULL, (WNOHANG|WUNTRACED), NULL);
    } while (pid > 0);
}


#ifdef DEBUG
static void handle_signal(int i)
{
    static int fd = -2;

    if (fd < 0) {
	fd = open("/tmp/pbs.markers", O_WRONLY|O_CREAT|O_TRUNC, 0666);
	if (fd != -1 ) {
	    close(2);
	    dup2(fd, 2);
	}
    }
    if (fd >= 0)
	DUMPTIMES(DoMarks);
}
#endif

static void setupPriorities(void)
{
    port_t myThread = thread_self();
    kern_return_t kret;

    kret = thread_priority(myThread, MAXPRI_USER - 2, FALSE);
    if (kret != KERN_SUCCESS)
	errmsg("thread_priority", kret, FALSE);
    kret = thread_policy(myThread, POLICY_INTERACTIVE, 0);
    if (kret != KERN_SUCCESS)
	errmsg("thread_policy", kret, FALSE);
}


/* prints out info about a port */
static void portInfo(port_t port)
{
    port_name_t set;
    int num_msgs;
    int backlog;
    boolean_t owner;
    boolean_t receiver;
    kern_return_t ret;

    ret = port_status(task_self(), port, &set, &num_msgs, &backlog,
							&owner, &receiver);
    fprintf(stderr,
	"ret=%d set=%d num_msgs=%d backlog=%d owner=%d recvr=%d\n",
			ret, set, num_msgs, backlog, owner, receiver);
}


/*

Modifications (starting at 0.8):

01/06/89 trey	added this file to appkit
		fixed up error code returns from server
		implemented pbs asking for an unsupplied data type
10/29/89 trey	support for little endian machines getting AFM data

80
--
 4/09/90 trey	support for getting kerns and other AFM data
		support for named pasteboards

94
--
 9/25/90 trey	deal with asking for data from a dead app

*/

/*
	Pasteboard.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Pasteboard_Private.h"
#import "Application.h"
#define c_plusplus 1		/* for prototypes */
#include "pbs.h"
/*	we cant use these prototypes since we added the waitTime arg
#include "app.h"
*/
#undef c_plusplus
#import "pbtypes.h"
#import <defaults.h>
#import "nextstd.h"
#import "errors.h"

#import <objc/List.h>
#import <mach.h>
#import <stdio.h>
#import <netinet/in.h>		/* to please <nfs/nfs_clnt.h> */
#import <nfs/nfs_clnt.h>	/* for HOSTNAMESZ */

typedef struct {
    msg_header_t        head;
    msg_type_t          retCodeType;
    kern_return_t       retCode;
}                   reply_struct;

extern int app_server(msg_header_t *msg, reply_struct * reply);

static void freeBuffers(id self);
static void pbsMsgHandler(msg_header_t *msg, void *userData);
static void checkReturnVal(int ret);

/* List of all existing Pasteboards */
static id PBList = nil;

/* the max message size pbs may send to us */
#define MAX_PBS_MESSAGE		(sizeof( msg_header_t ) +		\
					6 * sizeof( msg_type_long_t ) +	\
					sizeof(port_t) +		\
					sizeof(int) +			\
					sizeof(pbtype_t) +		\
					sizeof(int) +			\
					sizeof(data_t) + 100 )

/* a non-sensical value for the Pasteboard owner */
#define NON_OWNER	((id)-1)

/* each time we get an updated change count, we may no longer be the
   owner of the Pasteboard.  This macro makes that check.
 */
#define FIXUP_OWNER(count)	\
	(owner = ((count) > 0 && (count) != _ourChangeCount) ?	\
						NON_OWNER : owner )

/* for remembering the last msg sent to pbs.  Call TRACE_MSGS before calling
   one of the MIG functions.  Call it with NULL afterwards, or call
   checkReturnVal (which does this for you).
 */
static char *LastTransaction = NULL;
#define TRACE_MSGS(str)		LastTransaction = (str)

@interface Pasteboard(Obsolete)
- provideData:(const char *)type;
@end

@implementation Pasteboard

+ initialize
{
  /* hack so kit doesnt depend on soundkit for builds */
    extern NXAtom NXSoundPboardType;

    if (self == [Pasteboard class]) {
	NXAsciiPboardType = NXUniqueStringNoCopy(NXAsciiPboardType);
	NXPostScriptPboardType = NXUniqueStringNoCopy(NXPostScriptPboardType);
	NXTIFFPboardType = NXUniqueStringNoCopy(NXTIFFPboardType);
	NXRTFPboardType = NXUniqueStringNoCopy(NXRTFPboardType);
	NXFilenamePboardType = NXUniqueStringNoCopy(NXFilenamePboardType);
	NXTabularTextPboardType = NXUniqueStringNoCopy(NXTabularTextPboardType);
	NXFontPboardType = NXUniqueStringNoCopy(NXFontPboardType);
	NXRulerPboardType = NXUniqueStringNoCopy(NXRulerPboardType);
	NXSelectionPboard = NXUniqueStringNoCopy(NXSelectionPboard);
	NXFindPboard = NXUniqueStringNoCopy(NXFindPboard);
	NXFontPboard = NXUniqueStringNoCopy(NXFontPboard);
	NXRulerPboard = NXUniqueStringNoCopy(NXRulerPboard);
	NXSoundPboardType = NXUniqueStringNoCopy(NXSoundPboardType);
    }
    return self;
}

+ (BOOL)_canAlloc { return NO; }

+ allocFromZone:(NXZone *)zone { return [self doesNotRecognize:_cmd]; }

+ alloc { return [self doesNotRecognize:_cmd]; }


+ new
{
    return [self newName:NXSelectionPboard];
}


+ newName:(const char *)name
{
    return [self _newName:name host:NULL];
}


+ _newName:(const char *)name host:(const char *)host
{
    char hostFound[HOSTNAMESZ];
    port_t newServer;
    NXAtom newName, newHost;
    id existingPB = nil;
    int i;
    Pasteboard **objPtr;

    newServer = _NXLookupAppkitServer(NULL, host, hostFound);
    newName = NXUniqueString(name);
    newHost = NXUniqueString(hostFound);
    
    if (PBList)
	for (i = [PBList count], objPtr = NX_ADDRESS(PBList); i--; objPtr++)
	    if ((*objPtr)->_name == newName && (*objPtr)->_host == newHost) {
		existingPB = *objPtr;
		break;
	    }

    if (existingPB)
	return existingPB;
    else {
	NXZone *zone;

	zone = [NXApp zone];
	if (!zone)
	    zone = NXDefaultMallocZone();
	self = [[super allocFromZone:zone] init];
	_server = newServer;
	_name = newName;
	_host = newHost;
	[self changeCount];			/* to init _realChangeCount */
	_ourChangeCount = -1;
	_client = PORT_NULL;
	owner = NON_OWNER;
	if (!PBList)
	    PBList = [[List allocFromZone:zone] initCount:1];
	[PBList addObject:self];
	return self;
    }
}


- free
{
    freeBuffers(self);
    if (_client) {
	DPSRemovePort(_client);
	port_deallocate(task_self(), _client);
/* _server is shared among all named Pasteboards
	if (_name != NXSelectionPboard)
	    port_deallocate(task_self(), _server);
*/
    }
    [PBList removeObject:self];
    return [super free];
}


- freeGlobally
{
    TRACE_MSGS("_NXFreePasteboard");
    checkReturnVal(_NXFreePasteboard(_NXNetTimeout, _server, (char *)_name));
    return [self free];
}


- (const char *)name
{
    return _name;		/* This must always be an NXAtom */
}


/* Ownership */

/* first tries the new delegate method, then the obsolete one from 1.0 */
- (BOOL)_doProvideData:(const char *)dataType
{
    NXAtom atomizedType = NXUniqueString(dataType);

    if (owner && [owner respondsTo:@selector(pasteboard:provideData:)]) {
	[owner pasteboard:self provideData:atomizedType];
    } else if (owner && [owner respondsTo:@selector(provideData:)]) {
	[owner provideData:atomizedType];
    } else {
	return NO;			/* we can't provide it */
    }
    return YES;
}

- declareTypes:(const char *const *)newTypes num:(int)numTypes owner:newOwner
{
    const char * const *aType;	/* loop index through types */
    NXAtom	       *types;	/* loop index through types */
    int			i;
    char	       *cur;
    int			realNum = 0;
    int			ret;
    kern_return_t	kr;
    NXZone	       *zone = [self zone];
    int			typesLength;
    vm_address_t	typesBuf;

    owner = newOwner;
    freeBuffers(self);
    typesLength = 0;
    for (i = numTypes, aType = newTypes; i--; aType++)
	if (*aType) {
	    typesLength += strlen(*aType) + 1;
	    realNum++;
	}
    if (!typesLength)
	return self;

    kr = vm_allocate(task_self(), &typesBuf, typesLength, TRUE);
    if (kr != KERN_SUCCESS)
	NX_RAISE(NX_appkitVMError, (void *)kr, "vm_allocate() in Pasteboard");

    _typesArray = NXZoneMalloc(zone, sizeof(NXAtom) * (numTypes + 1));
    _typesProvided = NXZoneCalloc(zone, sizeof(BOOL) * numTypes, 1);
    cur = (char *)typesBuf;
    types = _typesArray;
    for (i = numTypes, aType = newTypes; i--; aType++, types++) {
	if (*aType) {
	    strcat(cur, *aType);
	    *types = NXUniqueString(cur);
	    cur += strlen(*aType) + 1;
	}
    }
    *types = NULL;

    if (_client == PORT_NULL) {
	if (port_allocate(task_self(), &_client) == KERN_SUCCESS)
	    DPSAddPort(_client, pbsMsgHandler, MAX_PBS_MESSAGE,
		       NULL, NX_BASETHRESHOLD);
	else
	     AK_ASSERT(FALSE, "Couldnt allocate port in Pasteboard.m");
    }

    TRACE_MSGS("_NXSetTypes");
    ret = _NXSetTypes(_NXNetTimeout, _server, (char *)_name, _client, (int)self, (char *)typesBuf, typesLength, realNum, &_realChangeCount);

    kr = vm_deallocate(task_self(), typesBuf, typesLength);
    if (kr != KERN_SUCCESS)
	NX_RAISE(NX_appkitVMError, (void *)kr, "vm_deallocate() in Pasteboard");

    checkReturnVal(ret);
    _ourChangeCount = _realChangeCount;
    return self;
}


- (int)changeCount
{
    int                 ret;

    TRACE_MSGS("_NXGetSession");
    ret = _NXGetSession(_NXNetTimeout, _server, (char *)_name, &_realChangeCount);
    FIXUP_OWNER(_realChangeCount);
    checkReturnVal(ret);
    return _realChangeCount;
}


/* Writing to the Pasteboard */


- writeType:(const char *)dataType data:(const char *)theData
    length:(int)numBytes
{
    int                 ret;
    NXAtom	       *s;

    TRACE_MSGS("_NXSetData");
    ret = _NXSetData(_NXNetTimeout, _server, (char *)_name, _ourChangeCount, (char *)dataType, (char *)theData, numBytes, &_realChangeCount);
    TRACE_MSGS(NULL);
    FIXUP_OWNER(_realChangeCount);
    if (ret != PBS_OK && ret != PBS_OOSYNC)
	checkReturnVal(ret);
    if (owner != NON_OWNER) {
	for (s = _typesArray; *s; s++)
	    if (!strcmp(*s, dataType))
		break;
	if (*s)
	    _typesProvided[s - _typesArray] = YES;
    }
    return (ret == PBS_OK) ? self : nil;
}


/* Retrieving things from the pasteboard */

- (const NXAtom *)types
{
    int                 numTypes, i;
    NXAtom	       *types;
    char	       *cur;
    int                 ret;
    unsigned int	typesLength;
    char	       *typesBuf = NULL;
    kern_return_t	kr;

    [self changeCount];
    if (_typesArray && _realChangeCount == _ourChangeCount)
	return _typesArray;
    freeBuffers(self);

    TRACE_MSGS("_NXGetTypes");
    ret = _NXGetTypes(_NXNetTimeout, _server, (char *)_name, &typesBuf, &typesLength, &numTypes, &_realChangeCount);
    FIXUP_OWNER(_realChangeCount);
    NX_DURING {
	checkReturnVal(ret);
    } NX_HANDLER {
	if (typesBuf)
	    (void)vm_deallocate(task_self(), (vm_address_t)typesBuf, typesLength);
    } NX_ENDHANDLER

    _ourChangeCount = _realChangeCount;
    _typesArray = NXZoneMalloc([self zone], (sizeof(char *)) * (numTypes + 1));
    types = _typesArray;
    cur = typesBuf;
    for (i = 0; i < numTypes; i++, types++) {
	*types = NXUniqueString(cur);
	cur += strlen(cur) + 1;
    }
    *types = NULL;
    if (typesBuf) {
	kr = vm_deallocate(task_self(), (vm_address_t)typesBuf, typesLength);
	if (kr != KERN_SUCCESS)
	    NX_RAISE(NX_appkitVMError, (void *)kr, "vm_deallocate() in Pasteboard");
    }

    return _typesArray;
}


/* Reading from the Pasteboard */

- readType:(const char *)dataType data:(char **)theData length:(int *)numBytes
{
    int                 sysRet;
    id			ourRet;

    *theData = NULL;		/* assume failure */
    *numBytes = 0;
    ourRet = nil;
    TRACE_MSGS("_NXNonBlockingGetData");
    sysRet = _NXNonBlockingGetData(_NXNetTimeout, _server, (char *)_name, (char *)dataType, _ourChangeCount, theData, (unsigned int *)numBytes, &_realChangeCount);
    FIXUP_OWNER(_realChangeCount);
    TRACE_MSGS(NULL);
    switch (sysRet) {
    case PBS_OOSYNC:
	break;			/* contents changed out from under us */
    case PBS_OK:
	ourRet = self;
	break;
    case DATA_DELAYED:		/* if data wasnt available */
	if (owner != NON_OWNER)	/* if we're the owner */
	    if (![self _doProvideData:dataType]) break;
	NX_DURING {
	    TRACE_MSGS("_NXBlockingGetData");
	    sysRet = _NXBlockingGetData(_NXNetTimeout, _server, (char *)_name, (char *)dataType, _ourChangeCount, theData, (unsigned int *)numBytes, &_realChangeCount);
	    TRACE_MSGS(NULL);
	    FIXUP_OWNER(_realChangeCount);
	    if (sysRet != PBS_OOSYNC && sysRet != PBS_OK)
		checkReturnVal(sysRet);
	    ourRet = (sysRet == PBS_OK) ? self : nil;
	} NX_HANDLER {
	    if (NXLocalHandler.code == NX_pasteboardComm &&
		  (kern_return_t) NXLocalHandler.data1 == RCV_TIMED_OUT)
		 /* timed out w/o data */ ;
	    else
		NX_RERAISE();
	} NX_ENDHANDLER
	break;
    default:
	checkReturnVal(sysRet);
    }
    return ourRet;
}


+ (void)_provideAllPromisedData
{
    [PBList makeObjectsPerform:@selector(_providePromisedData)];
}


/* provides any delayed data that we have promised and not delivered */
- (void)_providePromisedData;
{
    NXAtom	       *s;
    BOOL	       *flag;

    [self changeCount];
    FIXUP_OWNER(_realChangeCount);
    if (owner != NON_OWNER && owner)
	for (s = _typesArray, flag = _typesProvided; *s; s++, flag++)
	    if (!*flag)
		[self _doProvideData:*s];
}


@end

static void freeBuffers(Pasteboard *self)
{
    if (self->_typesArray) {
	free(self->_typesArray);
	self->_typesArray = NULL;
    }
    if (self->_typesProvided) {
	free(self->_typesProvided);
	self->_typesProvided = NULL;
    }
}


void _NXMsgError(kern_return_t errorCode)
{
    char *msgArg = LastTransaction;

    TRACE_MSGS(NULL);
    NX_RAISE(NX_pasteboardComm, (void *)errorCode, msgArg);
}


/* called by pbs to ask for previously promised data */
kern_return_t _NXRequestData(port_t server, int session, int self, pbtype_t pbType)
{
    if (((Pasteboard *)self)->_ourChangeCount == session)
	[((Pasteboard *)self) _doProvideData:pbType];
    return KERN_SUCCESS;
}


/* called by system when there's a message from pbs */
static void pbsMsgHandler(msg_header_t *msg, void *userData)
{
    reply_struct        reply;
    int                 ret;

    ret = app_server(msg, &reply);	/* dispatches msg to C routine */
    NX_ASSERT(ret, "error in app_server in Pasteboard.m");
}


static void checkReturnVal(int ret)
{
    TRACE_MSGS(NULL);
    if (ret != PBS_OK)
	NX_RAISE(NX_pasteboardComm, 0, (void *)ret); 
}

/*
  
Modifications (starting at 0.8):
  
   11/88 trey	added methods for easy archiving to and from Pasteboard
01/06/89 trey	added _client and reserved instance vars
		fixed up error code returns from server
		implemented pbs asking for an unsupplied data type
		got rid of garbage external symbols from mig
01/16/89 trey	Added readSelf and writeSelf methods
02/02/89 trey	Nuked priority args to readType and writeType
 2/20/89 trey	Nuked methods for easy archiving to and from Pasteboard, use
		 typedStreams intead
 6/20/89 trey	_server instance var removed.  Uses _NXLookupAppkitServer()
 7/06/89 trey	types method returns constant data

0.94
----
 7/07/89 trey	_providePromisedData added for providing outstanding data
		 upon termination
 7/12/89 trey	nuked awake method
 7/17/89 trey	_providePromisedData doesnt ask owner for data if it doesnt
		 respond to provideData

10/29/89 trey	_NXLookupAppkitServer now takes an argument
12/14/89 trey	Pasteboard fixed to not message owner object if all data
		 types have been provided

80
--
 4/09/90 trey	added NXFilenamePboard, NXTabularTextPboard, NXFontPboard,
		 NXRulerPboard pasteboard types.
		added NXSelectionPBName, NXFindInfoPBName, NXFontPBName,
		 NXRulerPBName pasteboard names
		support for named pasteboards added, +newName:, -name,
		 -freeGlobally
		 
81
--
 4/12/90 trey	added NXFindInfoPboard type
		 
95
--
 10/1/90 trey	added debugging code to report transaction causing error

*/

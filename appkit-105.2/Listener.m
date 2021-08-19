/*
	Listener.m
	Copyright 1988, NeXT, Inc. 
	Responsibility: Greg Cockroft
	Author: Chris Franklin
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "NXJournaler_Private.h"
#import "Listener_Private.h"
#import "Pasteboard_Private.h"
#import "Panel.h"
#import "Speaker.h"
#import "Text.h"
#import "nextstd.h"
#import "errors.h"

#import <objc/hashtable.h>
#import "listenerprivate.h"
#import <mach.h>
#import <sys/message.h>
#import <servers/netname.h>

#import <dpsclient/dpsclient.h>
#import <objc/objc.h>

static id errorSpeaker = nil;

static void         receiver();

static const SEL * const remoteMethodSels[] = {
	&@selector(openFile:ok:),
	&@selector(unhide),
	&@selector(launchProgram:ok:),
	&@selector(iconEntered:at::iconWindow:iconX:iconY:iconWidth:iconHeight:pathList:),
	&@selector(iconMovedTo::),
	&@selector(iconReleasedAt::ok:),
	&@selector(iconExitedAt::),
	&@selector(registerWindow:toPort:),
	&@selector(unregisterWindow:),
	&@selector(msgQuit:),
	&@selector(msgCalc:),
	&@selector(msgDirectory:ok:),
	&@selector(msgVersion:ok:),
	&@selector(msgFile:ok:),
	&@selector(msgPrint:ok:),
	&@selector(msgSelection:length:asType:ok:),
	&@selector(msgSetPosition:posType:andSelect:ok:),
	&@selector(msgPosition:posType:ok:),
	&@selector(msgCopyAsType:ok:),
	&@selector(msgCutAsType:ok:),
	&@selector(msgPaste:),
	&@selector(unmounting:ok:),
	&@selector(powerOffIn:andSave:),
	&@selector(extendPowerOffBy:actual:),
	&@selector(openTempFile:ok:),
	&@selector(getFileInfoFor:app:type:ilk:ok:),
	&@selector(getFileIconFor:TIFF:TIFFLength:ok:),
	&@selector(_initJournaling:::),
	&@selector(_requestInitJournaling::::),
	&@selector(_request:pb:host:userData:app:menu:error:unhide:),
	&@selector(_request:pb:host:userData:error:),
	&@selector(_rmError:app:error:),
	&@selector(_getWindowServerMemory:backing:)
#ifdef DEBUG
	, &@selector(copyRuler:),
	&@selector(toggleRuler:)
#endif
};


/* table of methods we understand */
static NXRemoteMethod remoteMethods[] = {
	{0, "cI"},
	{0, ""},
	{0, "cI"},
	{0, "iddiddddc"},
	{0, "dd"},
	{0, "ddI"},
	{0, "dd"},
	{0, "is"},
	{0, "i"},
	{0, "I"},
	{0, "I"},
	{0, "CI"},
	{0, "CI"},
	{0, "CI"},
	{0, "cI"},
	{0, "BcI"},
	{0, "ciiI"},
	{0, "CII"},
	{0, "cI"},
	{0, "cI"},
	{0, "I"},
	{0, "cI"},
	{0, "ii"},
	{0, "iI"},
	{0, "cI"},
	{0, "cCCII"},
	{0, "cBI"},
	{0, "isS"},
	{0, "csIS"},
	{0, "ccccccsi"},
	{0, "ccccC"},
	{0, "ccc"},
	{0, "II"},
#ifdef DEBUG
	{0, ""},
	{0, ""},
#endif
	{0, NULL}
};

#define REMOTEMETHODS (sizeof(remoteMethods)/sizeof(NXRemoteMethod)-1)

extern port_t       name_server_port;

@implementation Listener

/* These variables will be generated automatically by <<remote message wrap>>.
 * They are only referenced locally by instances of this class.
 */

+ initialize
{
    int i;
    NXRemoteMethod *methPtr;
    const SEL * const *selPtr;

    [self setVersion:1];
    AK_ASSERT( REMOTEMETHODS == sizeof(remoteMethodSels)/sizeof(SEL*), "Sizes of remote method tables dont line up");
    if (self == [Listener class]) {
	methPtr = remoteMethods;
	selPtr = remoteMethodSels;
	for (i = REMOTEMETHODS; i--; )
	    (methPtr++)->key = **selPtr++;
    }
    return self;
}

+ run
{
    NXEvent dummy;

    for (;;) {
	NX_DURING
	    DPSGetEvent(DPS_ALLCONTEXTS, &dummy, 0, NX_FOREVER, NX_BASETHRESHOLD);
	NX_HANDLER
	    NXLogError("Error raised in Listener run loop, continuing ...\n");
	NX_ENDHANDLER
    }

    return self;
}

+ new
{
    return [[super allocFromZone:NXDefaultMallocZone()] init];
}

- init
{
    [super init];
    portName = NULL;
    listenPort = PORT_NULL;
    signaturePort = PORT_NULL;
    delegate = nil;
    _delegate2 = nil;
    timeout = _NXNetTimeout;
    priority = NX_BASETHRESHOLD;
    return self;
}

- free
{
    if (portName)
	[self checkOut];
    if (listenPort != PORT_NULL)
	port_deallocate(task_self(), listenPort);
    if (signaturePort != PORT_NULL)
	port_deallocate(task_self(), signaturePort);
    return[super free];
}

- (int)timeout
{
    return timeout;
}

- setTimeout:(int)ms
{
    timeout = ms;
    return self;
}


- (int)priority
{
    return priority;
}

- setPriority:(int)level
{
    priority = level;
    return self;
}

- (port_t)listenPort
{
    return listenPort;
}

- (port_t)signaturePort
{
    return signaturePort;
}

- delegate
{
    return delegate;
}

- setDelegate:anObject
{
    delegate = anObject;

    _delegate2 = ([anObject respondsTo:@selector(delegate)]) ?
      [anObject delegate] : nil;
    return self;
}

- (const char *)portName
{
    return portName;
}

- (int)checkInAs:(const char *)name
{
    int                 ec;

    ec = port_allocate(task_self(), &signaturePort);
    if (ec != KERN_SUCCESS)
	return ec;
    ec = port_allocate(task_self(), &listenPort);
    if (ec != KERN_SUCCESS) {
	port_deallocate(task_self(), signaturePort);
	signaturePort = PORT_NULL;
	return ec;
    }
    ec = netname_check_in(name_server_port, (char *)name, signaturePort, listenPort);
    if (ec != NETNAME_SUCCESS) {
	port_deallocate(task_self(), signaturePort);
	signaturePort = PORT_NULL;
	port_deallocate(task_self(), listenPort);
	listenPort = PORT_NULL;
	return ec;
    }
    portName = NXCopyStringBufferFromZone(name, [self zone]);
    [NXApp _removeServicesMenuPort:name];
    return 0;
}

- (int)usePrivatePort
{
    int                 ec;

    ec = port_allocate(task_self(), &listenPort);
    return ec != KERN_SUCCESS ? ec : 0;
}

- (int)privatePort
{
    return [self usePrivatePort];
}

- (int)checkOut
{
    if (portName)
	netname_check_out(name_server_port, portName, signaturePort);
    free (portName);
    portName = (char *)0;
    return 0;
}

- addPort
{
    DPSAddPort(listenPort, receiver, sizeof(NXMessage), (char *)self, priority);
    return self;
}

- removePort
{
    DPSRemovePort(listenPort);
    return self;
}

id NXResponsibleDelegate(Listener *self, SEL selector)
{

    if (self->_delegate2 && [self->_delegate2 respondsTo:selector])
	return self->_delegate2;
    if (self->delegate && [self->delegate respondsTo:selector])
	return self->delegate;
    return nil;
}

- (int)openFile:(const char *)fullPath ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(openFile:ok:)))
	return[d openFile:fullPath ok:flag];
    return -1;
}

- (int)openTempFile:(const char *)fullPath ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(openTempFile:ok:)))
	return [d openTempFile:fullPath ok:flag];
    else
	return -1;
}

- (int)unhide
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(unhide)))
	return[d unhide];
    return -1;
}

- (int)launchProgram:(const char *)name ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(launchProgram:ok:)))
	return[d launchProgram:name ok:flag];
    return -1;
}

- (int)iconEntered:(int)windowNum at:(double)x :(double)y
    iconWindow:(int)iconWindowNum
    iconX:(double)iconX
    iconY:(double)iconY
    iconWidth:(double)iconWidth
    iconHeight:(double)iconHeight
    pathList:(const char *)pathList
{
    id                  d;

    if (d = NXResponsibleDelegate(self,
				  @selector(iconEntered:at::iconWindow:iconX:iconY:iconWidth:iconHeight:pathList:)))
	return[d iconEntered:windowNum at:x :y
	       iconWindow:iconWindowNum
	       iconX:iconX iconY:iconY
	       iconWidth:iconWidth iconHeight:iconHeight
	       pathList:pathList];
    return -1;
}

- (int)iconMovedTo:(double)x :(double)y
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(iconMovedTo::)))
	return[d iconMovedTo:x :y];
    return -1;
}

- (int)iconReleasedAt:(double)x :(double)y ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(iconReleasedAt::ok:)))
	return[d iconReleasedAt:x :y ok:flag];
    return -1;
}

- (int)iconExitedAt:(double)x :(double)y
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(iconExitedAt::)))
	return[d iconExitedAt:x :y];
    return -1;
}

- (int)registerWindow:(int)windowNum toPort:(port_t)aPort
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(registerWindow:toPort:)))
	return[d registerWindow:windowNum toPort:aPort];
    return -1;
}

- (int)unregisterWindow:(int)windowNum
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(unregisterWindow:)))
	return[d unregisterWindow:windowNum];
    return -1;
}

- (int)getFileInfoFor:(char *)fullPath app:(char **)name type:(char **)type
	ilk:(int *)ilk ok:(int *)flag
{
    id                  d;
    if (d = NXResponsibleDelegate(self, 
	@selector(getFileInfoFor:app:type:ilk:ok:)))
	return[d getFileInfoFor:fullPath app:name type:type ilk:ilk ok:flag];
    return -1;
}

- (int)getFileIconFor:(char *)fullPath TIFF:(char **)tiff
	TIFFLength:(int *)length ok:(int *)flag
{
	id              d;

    if (d = NXResponsibleDelegate(self,
	@selector(getFileIconFor:TIFF:TIFFLength:ok:)))
	return[d getFileIconFor:fullPath TIFF:tiff TIFFLength:length ok:flag];
    return -1;
}
- (int)msgQuit:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgQuit:)))
	return[d msgQuit:flag];
    return -1;
}

- (int)msgCalc:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgCalc:)))
	return[d msgCalc:flag];
    return -1;
}

- (int)msgDirectory:(char *const *)fullPath ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgDirectory:ok:)))
	return[d msgDirectory:fullPath ok:flag];
    return -1;
}

- (int)msgVersion:(char *const *)aString ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgVersion:ok:)))
	return[d msgVersion:aString ok:flag];
    return -1;
}

- (int)msgFile:(char *const *)fullPath ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgFile:ok:)))
	return[d msgFile:fullPath ok:flag];
    return -1;
}

- (int)msgPrint:(const char *)fullPath ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgPrint:ok:)))
	return[d msgPrint:fullPath ok:flag];
    return -1;
}

- (int)msgSelection:(char *const *)bytes length:(int *)len
    asType:(const char *)aType ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self,
				  @selector(msgSelection:length:asType:ok:)))
	return[d msgSelection:bytes length:len asType:aType ok:flag];
    return -1;
}

- (int)msgSetPosition:(const char *)aString posType:(int)anInt
    andSelect:(int)sflag ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self,
			   @selector(msgSetPosition:posType:andSelect:ok:)))
	return[d msgSetPosition:aString posType:anInt
	       andSelect:sflag ok:flag];
    return -1;
}

- (int)msgPosition:(char *const *)aString posType:(int *)anInt
    ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgPosition:posType:ok:)))
	return[d msgPosition:aString posType:anInt ok:flag];
    return -1;
}

- (int)msgCopyAsType:(const char *)aType ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgCopyAsType:ok:)))
	return[d msgCopyAsType:aType ok:flag];
    return -1;
}

- (int)msgCutAsType:(const char *)aType ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgCutAsType:ok:)))
	return[d msgCutAsType:aType ok:flag];
    return -1;
}

- (int)msgPaste:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(msgPaste:)))
	return[d msgPaste:flag];
    return -1;
}

- (int)unmounting:(const char *)fullPath ok:(int *)flag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(unmounting:ok:)))
	return[d unmounting:fullPath ok:flag];
    return -1;
}

- (int)powerOffIn:(int)ms andSave:(int)aFlag
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(powerOffIn:andSave:)))
	return[d powerOffIn:ms andSave:aFlag];
    return -1;
}

- (int)extendPowerOffBy:(int)requestedMs actual:(int *)actualMs
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(extendPowerOffBy:actual:)))
	return[d extendPowerOffBy:requestedMs actual:actualMs];
    return -1;
}

- (int)_initJournaling:(int)appNum
     :(port_t) masterPort
     :(port_t *) slavePort
{
    register id     slaveJournaler;

    slaveJournaler = [NXJournaler _newSlave];
    [slaveJournaler _setApplicationNum:appNum];
    [[slaveJournaler speaker] setSendPort:masterPort];
    *slavePort = [[slaveJournaler listener] listenPort];
    return 0;
}


- (int)_requestInitJournaling:(const char *)appName
     :(port_t) slavePort
     :(int *)appNumPtr
     :(port_t *) masterPortPtr
 /*
  * This method should only be received by a NXJournalListener.  I have to
  * put it in Listener because Speaker must be able the send the message. 
  */
{
    id                  d;

    if (d = NXResponsibleDelegate(self, @selector(_requestInitJournaling::::)))
	return[d _requestInitJournaling:appName:slavePort:appNumPtr:masterPortPtr];
    return -1;
}


#ifdef DEBUG
- (int)_testCopy
{
    id first;
    
    first = [[NXApp keyWindow] firstResponder];
    if (first && [first respondsTo:@selector(copyRuler:)])
	[first copyRuler:self];
    return 0;
}
- (int)_testRuler
{
    id first;
    
    first = [[NXApp keyWindow] firstResponder];
    if (first && [first respondsTo:@selector(toggleRuler:)])
	[first toggleRuler:self];
    return 0;
}
#endif

- (int)performRemoteMethod:(NXRemoteMethod *)method
    paramList:(NXParamValue *)params
{

    switch (method - remoteMethods) {
    case 0:
	return[self openFile:params[0].bval.p ok:&params[1].ival];
    case 1:
	return[self unhide];
    case 2:
	return[self launchProgram:params[0].bval.p ok:&params[1].ival];
    case 3:
	return[self iconEntered:params[0].ival at:params[1].dval:params[2].dval
	       iconWindow:params[3].ival
	       iconX:params[4].dval iconY:params[5].dval
	       iconWidth:params[6].dval iconHeight:params[7].dval
	       pathList:params[8].bval.p];
    case 4:
	return[self iconMovedTo:params[0].dval:params[1].dval];
    case 5:
	return[self iconReleasedAt:params[0].dval:params[1].dval
	       ok:&params[2].ival];
    case 6:
	return[self iconExitedAt:params[0].dval:params[1].dval];
    case 7:
	return[self registerWindow:params[0].ival toPort:params[1].pval];
    case 8:
	return[self unregisterWindow:params[0].ival];
    case 9:
	return[self msgQuit:&params[0].ival];
    case 10:
	return[self msgCalc:&params[0].ival];
    case 11:
	return[self msgDirectory:&params[0].bval.p ok:&params[1].ival];
    case 12:
	return[self msgVersion:&params[0].bval.p ok:&params[1].ival];
    case 13:
	return[self msgFile:&params[0].bval.p ok:&params[1].ival];
    case 14:
	return[self msgPrint:params[0].bval.p ok:&params[1].ival];
    case 15:
	return[self msgSelection:&params[0].bval.p
	       length:&params[0].bval.len
	       asType:params[1].bval.p
	       ok:&params[2].ival];
    case 16:
	return[self msgSetPosition:params[0].bval.p
	       posType:params[1].ival
	       andSelect:params[2].ival
	       ok:&params[3].ival];
    case 17:
	return[self msgPosition:&params[0].bval.p
	       posType:&params[1].ival
	       ok:&params[2].ival];
    case 18:
	return[self msgCopyAsType:params[0].bval.p ok:&params[1].ival];
    case 19:
	return[self msgCutAsType:params[0].bval.p ok:&params[1].ival];
    case 20:
	return[self msgPaste:&params[0].ival];
    case 21:
	return[self unmounting:params[0].bval.p ok:&params[1].ival];
    case 22:
	return[self powerOffIn:params[0].ival andSave:params[1].ival];
    case 23:
	return[self extendPowerOffBy:params[0].ival
	       actual:&params[1].ival];
    case 24:
	return[self openTempFile:params[0].bval.p ok:&params[1].ival];
    case 25:
	return [self getFileInfoFor:params[0].bval.p
		app:&params[1].bval.p type:&params[2].bval.p
		ilk:&params[3].ival ok:&params[4].ival];
    case 26:
	return [self getFileIconFor:params[0].bval.p TIFF:&params[1].bval.p 
		TIFFLength:&params[1].bval.len ok:&params[2].ival];
    case 27:
    	return[self _initJournaling:params[0].ival
	   :params[1].pval
	   :&params[2].pval];
    case 28:
	return[self _requestInitJournaling:params[0].bval.p
	   :params[1].pval
	   :&params[2].ival
	   :&params[3].pval];
    case 29:
	return [self _request:params[0].bval.p pb:params[1].bval.p host:params[2].bval.p userData:params[3].bval.p app:params[4].bval.p menu:params[5].bval.p error:params[6].pval unhide:params[7].ival];
    case 30:
	return [self _request:params[0].bval.p pb:params[1].bval.p host:params[2].bval.p userData:params[3].bval.p error:&params[4].bval.p];
    case 31:
	return [self _rmError:params[0].bval.p app:params[1].bval.p error:params[2].bval.p];
    case 32:
	return [self _getWindowServerMemory:&params[0].ival 
				    backing:&params[1].ival];
#ifdef DEBUG
    case 33: 
	return [self _testCopy];
    case 34: 
	return [self _testRuler];
#endif
    default:
	return -1;
    }
}

- (NXRemoteMethod *)remoteMethodFor:(SEL)aSelector
{
    return NXRemoteMethodFromSel(aSelector, remoteMethods);
}

extern NXMessage   *NXInputMessage;
extern NXRemoteMethod *NXInputMethod;

- messageReceived:(NXMessage *)msg
{
    NXRemoteMethod     *remoteMethod;
    NXParamValue        paramValues[NX_MAXMSGPARAMS];
    register NXParamValue *ap;
    int                 rpc;
    register msg_type_t *st;
    register msg_type_t *endc;
    msg_type_long_t    *lt;
    int                 moffset;
    register char      *p;
    char               *cval;
    int                 ival;
    NXMessage           m;
    int                 ec;
    register int        msize = 0;
    register int        esize = 0;
    int                 arrays;
    int                 asize;
    int                 inLine;
    int                 mtype;
    int                 noPorts;
    int                 mustFree;
    SEL			tempSel;

    mustFree = YES;
    rpc = NO;
    mtype = msg->header.msg_id;
    if (!NXValidMessage(msg) ||
	(mtype != NX_SELECTORFMSG && mtype != NX_SELECTORPMSG)) {
	NXFreeMsgVM(msg, YES);
	mustFree = NO;
	goto exitPoint;
    }
    bcopy(msg, &m, msg->header.msg_size);

    endc = (msg_type_t *)((char *)&m + m.header.msg_size);

    if (!(tempSel = sel_getUid(m.action)) ||
	!(remoteMethod = [self remoteMethodFor:tempSel]))
	goto exitPoint;
    NXInputMethod = remoteMethod;
    moffset = (int)(m.action - (char *)&m) +
      ((m.actionType.msg_type_number + 3) / 4) * 4;
    p = remoteMethod->types;
    st = (msg_type_t *)((char *)&m + moffset);
    ap = paramValues;
    while (*p) {
	switch (*p) {
	case NX_MSG_DOUBLE_IN:
	    msize = sizeof(double);
	    esize = sizeof(msg_type_t);
	    if (st >= endc)
		goto exitPoint;
	    if (st->msg_type_name != MSG_TYPE_REAL ||
		st->msg_type_size != sizeof(double) * 8 ||
		st->msg_type_number != 1 ||
		!st->msg_type_inline ||
		st->msg_type_longform)
		goto exitPoint;
	    ap->dval = *((double *)((char *)st + esize));
	    break;
	case NX_MSG_INT_IN:
	    msize = sizeof(int);
	    esize = sizeof(msg_type_t);
	    if (st >= endc)
		goto exitPoint;
	    if (st->msg_type_name != MSG_TYPE_INTEGER_32 ||
		st->msg_type_size != sizeof(int) * 8 ||
		st->msg_type_number != 1 ||
		!st->msg_type_inline ||
		st->msg_type_longform)
		goto exitPoint;
	    ap->ival = *((int *)((char *)st + esize));
	    break;
	case NX_MSG_PORT_RCV_IN:
	case NX_MSG_PORT_SEND_IN:
	    msize = sizeof(port_t);
	    esize = sizeof(msg_type_t);
	    if (st >= endc)
		goto exitPoint;
	    if (*p == NX_MSG_PORT_RCV_IN) {
		if (st->msg_type_name != MSG_TYPE_PORT_ALL)
		    goto exitPoint;
	    } else if (st->msg_type_name != MSG_TYPE_PORT)
		goto exitPoint;
	    if (st->msg_type_size != sizeof(port_t) * 8 ||
		st->msg_type_number != 1 ||
		!st->msg_type_inline ||
		st->msg_type_longform)
		goto exitPoint;
	    ap->pval = *((port_t *)((char *)st + esize));
	    break;
	case NX_MSG_CHARS_IN:
	case NX_MSG_BYTES_IN:
	    if (st >= endc)
		goto exitPoint;
	    inLine = st->msg_type_inline;
	    if (inLine == st->msg_type_longform)
		goto exitPoint;
	    if (inLine) {
		esize = sizeof(msg_type_t);
		if (st->msg_type_name != MSG_TYPE_BYTE ||
		    st->msg_type_size != 8)
		    goto exitPoint;
		msize = ap->bval.len = st->msg_type_number;
		ap->bval.p = ((char *)st + esize);
	    } else {
		lt = (msg_type_long_t *)st;
		esize = sizeof(msg_type_long_t);
		if (lt->msg_type_long_name != MSG_TYPE_BYTE ||
		    lt->msg_type_long_size != 8)
		    goto exitPoint;
		ap->bval.len = lt->msg_type_long_number;
		ap->bval.p = *((char **)((char *)st + esize));
		msize = sizeof(p);
	    }
	    if (*p == NX_MSG_CHARS_IN &&
		(ap->bval.len <= 0 ||
		 ap->bval.p[ap->bval.len - 1]))
		goto exitPoint;
	    msize = ((msize + 3) / 4) * 4;
	    break;
	case NX_MSG_PORT_RCV_OUT:
	case NX_MSG_PORT_SEND_OUT:
	case NX_MSG_DOUBLE_OUT:
	case NX_MSG_INT_OUT:
	case NX_MSG_BYTES_OUT:
	case NX_MSG_CHARS_OUT:
	    rpc = YES;
	    goto continuePt1;
	}
	st = (msg_type_t *)((char *)st + esize + msize);
continuePt1:
	p++;
	ap++;
    }

    if ((rpc && mtype == NX_SELECTORPMSG) ||
	(!rpc && mtype == NX_SELECTORFMSG))
	goto exitPoint;

    NXInputMessage = &m;
    if ([self performRemoteMethod:remoteMethod paramList:paramValues]) {
	NXInputMessage = (NXMessage *)0;
	goto exitPoint;
    }
    NXInputMessage = (NXMessage *)0;
    NXFreeMsgVM(&m, NO);
    mustFree = NO;

    if (rpc) {
	m.header.msg_type = MSG_TYPE_NORMAL;
	m.header.msg_local_port = listenPort;
	m.header.msg_id = NX_RESPONSEMSG;

	m.sequenceType.msg_type_name = MSG_TYPE_INTEGER_32;
	m.sequenceType.msg_type_size = sizeof(int) * 8;
	m.sequenceType.msg_type_number = 1;
	m.sequenceType.msg_type_inline = TRUE;
	m.sequenceType.msg_type_longform = FALSE;
	m.sequenceType.msg_type_deallocate = FALSE;


	msize = sizeof(NXResponse);
	p = remoteMethod->types;
	arrays = 0;
	asize = 0;
	ap = paramValues;
	noPorts = 1;
	while (*p) {
	    switch (*p) {
	    case NX_MSG_DOUBLE_OUT:
		msize += sizeof(double) + sizeof(msg_type_t);
		break;
	    case NX_MSG_INT_OUT:
		msize += sizeof(int) + sizeof(msg_type_t);
		break;
	    case NX_MSG_CHARS_OUT:
		ap->bval.len = strlen(ap->bval.p) + 1;
	    case NX_MSG_BYTES_OUT:
		ival = ap->bval.len;
		arrays++;
		ival = ((ival + 3) / 4) * 4;
		asize += ival;
		msize += sizeof(msg_type_t);
		break;
	    case NX_MSG_PORT_RCV_OUT:
	    case NX_MSG_PORT_SEND_OUT:
		msize += sizeof(port_t) + sizeof(msg_type_t);
		noPorts = 0;
		break;
	    case NX_MSG_PORT_RCV_IN:
	    case NX_MSG_PORT_SEND_IN:
	    case NX_MSG_DOUBLE_IN:
	    case NX_MSG_INT_IN:
	    case NX_MSG_CHARS_IN:
	    case NX_MSG_BYTES_IN:
		break;
	    }
	    p++;
	    ap++;
	}

	if ((msize + asize) < sizeof(NXMessage)) {
	    inLine = TRUE;
	    m.header.msg_size = msize + asize;
	    m.header.msg_simple = noPorts;
	} else {
	    inLine = FALSE;
	    m.header.msg_size = msize +
	      arrays *
	      (sizeof(msg_type_long_t) -
	       sizeof(msg_type_t) +
	       sizeof(p));
	    m.header.msg_simple = FALSE;
	}

	if (m.header.msg_size > sizeof(NXMessage)) {
	    ec = SEND_MSG_TOO_LARGE;
	    goto exitPoint;
	}
	p = remoteMethod->types;
	st = (msg_type_t *)&m.actionType;
	ap = paramValues;
	while (*p) {
	    switch (*p) {
	    case NX_MSG_DOUBLE_OUT:
		msize = sizeof(double);
		esize = sizeof(msg_type_t);
		st->msg_type_name = MSG_TYPE_REAL;
		st->msg_type_size = sizeof(double) * 8;
		st->msg_type_number = 1;
		st->msg_type_inline = TRUE;
		st->msg_type_longform = FALSE;
		st->msg_type_deallocate = FALSE;
		*((double *)((char *)st + esize)) = ap->dval;
		break;
	    case NX_MSG_INT_OUT:
		msize = sizeof(int);
		esize = sizeof(msg_type_t);
		st->msg_type_name = MSG_TYPE_INTEGER_32;
		st->msg_type_size = sizeof(int) * 8;
		st->msg_type_number = 1;
		st->msg_type_inline = TRUE;
		st->msg_type_longform = FALSE;
		st->msg_type_deallocate = FALSE;
		*((int *)((char *)st + esize)) = ap->ival;
		break;
	    case NX_MSG_PORT_RCV_OUT:
	    case NX_MSG_PORT_SEND_OUT:
		msize = sizeof(port_t);
		esize = sizeof(msg_type_t);
		st->msg_type_name =
		  (*p == NX_MSG_PORT_RCV_OUT ? MSG_TYPE_PORT_ALL :
		   MSG_TYPE_PORT);
		st->msg_type_size = sizeof(port_t) * 8;
		st->msg_type_number = 1;
		st->msg_type_inline = TRUE;
		st->msg_type_longform = FALSE;
		st->msg_type_deallocate = FALSE;
		*((port_t *)((char *)st + esize)) = ap->pval;
		break;
	    case NX_MSG_BYTES_OUT:
	    case NX_MSG_CHARS_OUT:
		cval = ap->bval.p;
		msize = ap->bval.len;
		st->msg_type_inline = inLine;
		st->msg_type_longform = !inLine;
		st->msg_type_deallocate = FALSE;
		if (inLine) {
		    esize = sizeof(msg_type_t);
		    st->msg_type_name = MSG_TYPE_BYTE;
		    st->msg_type_size = 8;
		    st->msg_type_number = msize;
		    bcopy(cval, ((char *)st + esize), msize);
		} else {
		    lt = (msg_type_long_t *)st;
		    esize = sizeof(msg_type_long_t);
		    lt->msg_type_long_name = MSG_TYPE_BYTE;
		    lt->msg_type_long_size = 8;
		    lt->msg_type_long_number = msize;
		    *((char **)((char *)st + esize)) = cval;
		    msize = sizeof(p);
		}
		msize = ((msize + 3) / 4) * 4;
		break;
	    case NX_MSG_PORT_RCV_IN:
	    case NX_MSG_PORT_SEND_IN:
	    case NX_MSG_DOUBLE_IN:
	    case NX_MSG_INT_IN:
	    case NX_MSG_CHARS_IN:
	    case NX_MSG_BYTES_IN:
		goto continuePt2;
	    }
	    st = (msg_type_t *)((char *)st + esize + msize);
    continuePt2:
	    p++;
	    ap++;
	}
	mtype = 0;
	ec = _NXSafeSend(&m,
		       timeout ? (SEND_TIMEOUT | SEND_SWITCH) : SEND_SWITCH,
			 timeout);
        TRACE2("%s=sent rpc reply %d\n", [NXApp appName], ec);
    }
exitPoint:
    if (mustFree)
	NXFreeMsgVM(&m, YES);
    if (mtype == NX_SELECTORFMSG)
	NXSendAcknowledge(m.header.msg_remote_port,
			  PORT_NULL,
			  m.sequence,
			  NX_INCORRECTMESSAGE,
			  timeout);
    return self;
}


/*
 * NXRemoteMethod is called by subclasses of Listener to look up selector
 * in table of selectors understood by this class.  Returns entry in
 * table or NULL if selector not understood by this class.
 */

NXRemoteMethod     *
NXRemoteMethodFromSel(s, pt)
    register SEL        s;
    register NXRemoteMethod *pt;
{

    while (pt->key) {
	if (pt->key == s)
	    return pt;
	pt++;
    }
    return (NXRemoteMethod *)0;
}

/* !!! MUCH OF THE FOLLOWING CODE IS MACHINE SPECIFIC -- LOW BYTE/HIGH BYTE
 * !!! ADDRESSING PROBLEMS MUST BE RESOLVED INDIVIDUALLY
 */

/* gets called whenever the sender sends something */
static void receiver(msg, data)
    msg_header_t       *msg;
    char               *data;
{
    TRACE2("%s=received on %d\n", [NXApp appName], msg->header.msg_local_port);
    [(id)data messageReceived:(NXMessage *)msg];
}

/* NXNextSequence returns a series of sequence numbers (that wraps after
 * the precision of int is reached.  Listener and Speaker use sequence
 * number to recognize stale messages in protocol.
 */
int NXNextSequence(void)
{
    static int sequenceNumber = 0;

    sequenceNumber++;
    if (sequenceNumber == 0)
	sequenceNumber++;
    return sequenceNumber;
}

/* NXCopyInputData returns a vm_allocated pointer to the parameter in position  
 * parameter. NOTE: the byte array pointer and the byte array length count for
 * only one parameter, even thought they are 2 obj-c parameters. if data was
 * not inline, we clear ptr in msg so that when NXFreeMsgVM is called, we won't
 * free this data.  if data was inline we vm_allocate and copy the
 * data.  consequently, NXCopyInputData is moderately expensive for
 * inline data and you might want to malloc and copy it.
 *
 * returns null on vm_allocate failure.
 *
 * do not access field after this operation!!!
 */
char *NXCopyInputData(parameter)
    register int        parameter;
{
    char               *vmp;
    NXMessage          *m;
    register char      *p;
    register msg_type_t *st;
    register int        msize = 0;
    register int        esize = 0;

    if (!(m = NXInputMessage))
	return (char *)0;

    p = NXInputMethod->types;
    st = (msg_type_t *)((char *)&m->action +
			((m->actionType.msg_type_number + 3) / 4) * 4);
    while (*p && (parameter >= 0)) {
	switch (*p) {
	case NX_MSG_DOUBLE_IN:
	    msize = sizeof(double);
	    esize = sizeof(msg_type_t);
	    break;
	case NX_MSG_INT_IN:
	    msize = sizeof(int);
	    esize = sizeof(msg_type_t);
	    break;
	case NX_MSG_PORT_RCV_IN:
	case NX_MSG_PORT_SEND_IN:
	    msize = sizeof(port_t);
	    esize = sizeof(msg_type_t);
	    break;
	case NX_MSG_CHARS_IN:
	case NX_MSG_BYTES_IN:
	    if (st->msg_type_inline) {
		esize = sizeof(msg_type_t);
		msize = st->msg_type_number;
		if (!parameter) {
		    kern_return_t kr;
		    kr = vm_allocate(task_self(), (vm_address_t *)&vmp, msize, YES);
		    if (kr != KERN_SUCCESS) {
			NX_RAISE(NX_appkitVMError, (void *)kr, "vm_allocate() in Listener");
		    }
		    bcopy(((char *)st + esize), vmp, msize);
		    return vmp;
		}
		msize = ((msize + 3) / 4) * 4;
	    } else {
		esize = sizeof(msg_type_long_t);
		if (!parameter) {
		    vmp = *((char **)((char *)st + esize));
		    *((char **)((char *)st + esize)) = (char *)0;
		    return vmp;
		}
		msize = sizeof(p);
	    }
	    break;
	case NX_MSG_PORT_RCV_OUT:
	case NX_MSG_PORT_SEND_OUT:
	case NX_MSG_DOUBLE_OUT:
	case NX_MSG_INT_OUT:
	case NX_MSG_BYTES_OUT:
	case NX_MSG_CHARS_OUT:
	    goto continuePt1;
	}
	st = (msg_type_t *)((char *)st + esize + msize);
continuePt1:
	p++;
	parameter--;
    }
    return (char *)0;
}

/* ??? Post 2.0 change various calls to netname_lookup() to NXPortNameLookup().
 */
port_t NXPortNameLookup(const char *portName, const char *hostName)
{
    port_t              sport;

    if (!hostName) hostName = "";
    if (netname_look_up(name_server_port, (char *)hostName, (char *)portName, &sport) == NETNAME_SUCCESS) {
	return sport;
    } else {
    	return PORT_NULL;
    }
}

/* NXPortFromName. returns send rights to the port, if any, registered on
 * hostName (NULL or "" for same machine, * for broadcast).  returns
 * PORT_NULL if can't find  port under that name.  If necessary, messages
 * to workspace to run the desired program.
 */
port_t NXPortFromName(portName, hostName)
    const char         *portName;
    const char         *hostName;
{
    port_t              sport;
    id                  s = nil;	/* for clean -Wall */
    int                 ok;
    BOOL		madeOwnSpeaker = NO;

    if (!hostName)
	hostName = "";
    if (netname_look_up(name_server_port, (char *)hostName, (char *)portName, &sport) == NETNAME_SUCCESS)
	return sport;
    if (strcmp(portName, NX_WORKSPACEREPLY) &&
	strcmp(portName, NX_WORKSPACEREQUEST) &&
	(netname_look_up(name_server_port,
			 (char *)hostName,
			 (char *)NX_WORKSPACEREQUEST,
			 &sport) == NETNAME_SUCCESS)) {
	s = [NXApp appSpeaker];
	if (!s) {
	    s = [Speaker new];
	    madeOwnSpeaker = YES;
	}
	[s setSendPort:sport];
	if (![s launchProgram:(char *)portName ok:&ok] &&
	    ok &&
	    (netname_look_up(name_server_port,
			     (char *)hostName,
			     (char *)portName,
			     &sport) == NETNAME_SUCCESS)) {
	    TRACE3("%s=%s found as %d\n", [NXApp appName], portName, sport);
	    if (madeOwnSpeaker)
		[s free];
	    return sport;
	}
    }
    TRACE2("%s=%s not found\n", [NXApp appName], portName);
    if (madeOwnSpeaker)
	[s free];
    return PORT_NULL;
}

/* NXSendAcknowledge can be used to send NXAcknowledge messages, such as
 * are used in the remote method protocol or in certain other
 * protocols.  These messages contain only a sequence number
 * and a message type == NX_ACKNOWLEDGE and an error number.
 */
int NXSendAcknowledge(sendPort, replyPort, msgSequence, error, timeout)
    port_t              sendPort;
    port_t              replyPort;
    int                 msgSequence;
    int                 error;
    int                 timeout;
{
    NXAcknowledge       m;
    int                 ec;

    m.header.msg_simple = TRUE;
    m.header.msg_size = sizeof(NXAcknowledge);
    m.header.msg_type = MSG_TYPE_NORMAL;
    m.header.msg_local_port = replyPort;
    m.header.msg_remote_port = sendPort;
    m.header.msg_id = NX_ACKNOWLEDGE;
    m.sequenceType.msg_type_name = MSG_TYPE_INTEGER_32;
    m.sequenceType.msg_type_size = sizeof(int) * 8;
    m.sequenceType.msg_type_number = 1;
    m.sequenceType.msg_type_inline = TRUE;
    m.sequenceType.msg_type_longform = FALSE;
    m.sequenceType.msg_type_deallocate = FALSE;
    m.sequence = msgSequence;
    m.errorType.msg_type_name = MSG_TYPE_INTEGER_32;
    m.errorType.msg_type_size = sizeof(int) * 8;
    m.errorType.msg_type_number = 1;
    m.errorType.msg_type_inline = TRUE;
    m.errorType.msg_type_longform = FALSE;
    m.errorType.msg_type_deallocate = FALSE;
    m.error = error;
    ec = _NXSafeSend((NXMessage *)(&m),
		     timeout ? SEND_TIMEOUT : MSG_OPTION_NONE,
		     timeout);
    TRACE3("%s=acknowledge (%d) %d\n", [NXApp appName], m.error, ec);
    return ec;
}

/* NXValidMessage, verifies that message is proper size and includes
 * minimum fields for the defined message types.
 */
int NXValidMessage(msg)
    register NXMessage *msg;
{
    register int        actionLen;
    register int        hsize;
    register NXAcknowledge *ack;

    hsize = msg->header.msg_size;
    if (msg->header.msg_type != MSG_TYPE_NORMAL ||
	hsize > sizeof(NXMessage))
	return NO;
    if (msg->sequenceType.msg_type_longform ||
	msg->sequenceType.msg_type_name != MSG_TYPE_INTEGER_32 ||
	msg->sequenceType.msg_type_size != (sizeof(int) * 8) ||
	!msg->sequenceType.msg_type_inline)
	return NO;

    switch (msg->header.msg_id) {
    case NX_SELECTORFMSG:
    case NX_SELECTORPMSG:
	if (hsize < (sizeof(NXMessage) - NX_MAXMESSAGE))
	    return NO;
	if (msg->actionType.msg_type_longform ||
	    msg->actionType.msg_type_name != MSG_TYPE_BYTE ||
	    msg->actionType.msg_type_size != 8 ||
	    !msg->actionType.msg_type_inline ||
	    !(actionLen = msg->actionType.msg_type_number) ||
	    msg->action[actionLen - 1])
	    return NO;
	break;
    case NX_RESPONSEMSG:
	if (hsize < sizeof(NXResponse))
	    return NO;
	break;
    case NX_ACKNOWLEDGE:
	ack = (NXAcknowledge *)msg;
	if (hsize != sizeof(NXAcknowledge))
	    return NO;
	if (ack->errorType.msg_type_longform ||
	    ack->errorType.msg_type_name != MSG_TYPE_INTEGER_32 ||
	    ack->errorType.msg_type_size != (sizeof(int) * 8) ||
	    !ack->errorType.msg_type_inline)
	    return NO;
	break;
    default:
	return NO;
    }
    return YES;
}

/* NXFreeMsgVM, frees all vm allocated (out of line) data that wasn't spoken
 * for by <<NXTakeMsgData()>> (indicated by null pointer). we permit arbitrary
 * messages here.
 */
void NXFreeMsgVM(msg, freePortsToo)
    NXMessage          *msg;
    register int        freePortsToo;
{
    register msg_type_t *st;
    register msg_type_t *endc;
    register int        bytes;
    register int        esize;
    register msg_type_long_t *lt;
    char              **p;
    int                 name;
    port_t             *port;

    endc = (msg_type_t *)((char *)msg + msg->header.msg_size);
    st = &msg->sequenceType;
    while (st < endc) {
	if (st->msg_type_longform) {
	    lt = (msg_type_long_t *)st;
	    esize = sizeof(msg_type_long_t);
	    bytes = ((lt->msg_type_long_size + 7) / 8) * lt->msg_type_long_number;
	    name = lt->msg_type_long_name;
	} else {
	    bytes = ((st->msg_type_size + 7) / 8) * st->msg_type_number;
	    esize = sizeof(msg_type_t);
	    name = st->msg_type_name;
	}
	if (freePortsToo &&
	    (name == MSG_TYPE_PORT_ALL ||
	     name == MSG_TYPE_PORT)) {
	    port = (port_t *)((char *)st + esize);
	    if (*port != PORT_NULL)
		(void)port_deallocate(task_self(), *port);
	}
	if (!st->msg_type_inline) {
	    p = (char **)((char *)st + esize);
	    if (*p) {
		kern_return_t kr;
		kr = vm_deallocate(task_self(), (vm_address_t)*p, bytes);
		if (kr != KERN_SUCCESS) {
		    NX_RAISE(NX_appkitVMError, (void *)kr, "vm_deallocate() in Listener");
		}
	    }
	    bytes = sizeof(p);
	}
	st = (msg_type_t *)((char *)st + esize + ((bytes + 3) / 4) * 4);
    }
}

/* _NXSafeReceive covers msg_receive so that the task_notify() port
 * is polled periodically so that we can detect death of postscript.
 * or loginwindow blowing us away.
 */
msg_return_t _NXSafeReceive(msg, options, timeout)
    NXMessage          *msg;
    int                 options;
    int                 timeout;
{
    msg_return_t        success;
    NXEvent             dumEvent;
    int                 totalTime;

    totalTime = 0;
    if (NXApp)
	while (1) {
	    success = msg_receive((msg_header_t *)msg,
				    options | RCV_TIMEOUT,
				    (timeout && timeout < 5000) ? timeout : 5000);
	    if (success != RCV_TIMED_OUT)
		return success;
	    DPSPeekEvent(0 /* NULL */ , &dumEvent, 0, 0.0, 31);
	    totalTime += 5000;
	    if ((options & RCV_TIMEOUT) && (totalTime >= timeout))
		return RCV_TIMED_OUT;
	}
    else
	return msg_receive((msg_header_t *)msg, options | RCV_TIMEOUT, timeout);
}

/* _NXSafeSend covers msg_send so that the task_notify() port
 * is polled periodically so that we can detect death of postscript.
 * or loginwindow blowing us away.
 */

msg_return_t _NXSafeSend(msg, options, timeout)
    NXMessage          *msg;
    int                 options;
    int                 timeout;
{
    msg_return_t        success;
    NXEvent             dumEvent;
    int                 totalTime;

    totalTime = 0;
    if (NXApp)
	while (1) {
	    success = msg_send((msg_header_t *)msg,
				options | SEND_TIMEOUT,
				(timeout && timeout < 5000) ? timeout : 5000);
	    if (success != SEND_TIMED_OUT)
		return success;
	    DPSPeekEvent(NULL, &dumEvent, 0, 0.0, 31);
	    totalTime += 5000;
	    if ((options & SEND_TIMEOUT) && (totalTime >= timeout))
		return SEND_TIMED_OUT;
	}
    else
	return msg_send((msg_header_t *)msg, options | SEND_TIMEOUT, timeout);
}

/* NXReceiveMessage waits for a message on receivePort with a 
 * sequence number of sequence.
 */
int NXReceiveMessage(msg, receivePort, sequence, timeout, rtype)
    NXMessage          *msg;
    port_t              receivePort;
    int                 sequence;
    int                 timeout;
    int                *rtype;
{
    int                 ec;
    int                 seq;

/* unless you expect at lot of out of sequence message on this port,
 * this is ok.  otherwise we should reduce timeout to cleave to
 * absolute timeout amount.
 */

    while (1) {
	msg->header.msg_size = sizeof(NXMessage);
	msg->header.msg_local_port = receivePort;
	ec = _NXSafeReceive(msg, timeout ? RCV_TIMEOUT : MSG_OPTION_NONE, timeout);
	if (ec != RCV_SUCCESS)
	    return -1;
	if (NXValidMessage(msg)) {
	    seq = msg->sequence;
	    *rtype = msg->header.msg_id;
	    if (!sequence || sequence == seq) {
		ec = 0;
		break;
	    }
	}
	NXFreeMsgVM(msg, YES);
    }
    return ec;
}

/* NXWaitForMessage is like NXReceiveMessage except it has it's own
 * local message and so presents a simpler interface.
 */
int NXWaitForMessage(receivePort, sequence, replyPort, timeout)
    port_t              receivePort;
    int                 sequence;
    int                 timeout;
    port_t             *replyPort;
{
    NXMessage           m;
    int                 mtype;
    int                 ec;

/* unless you expect at lot of out of sequence message on this port,
 * this is ok.  otherwise we should reduce timeout to cleave to
 * absolute timeout amount.
 */

    ec = NXReceiveMessage(&m, receivePort, sequence, timeout, &mtype);
    if (ec >= 0 && replyPort)
	*replyPort = m.header.msg_remote_port;
    if (ec >= 0)
	NXFreeMsgVM(&m, YES);
    else
	mtype = -1;
    return mtype;
}


- setServicesDelegate:anObject
{
    _requestDelegate = anObject;
    return self;
}

- servicesDelegate
{
    return _requestDelegate;
}

- setRequestDelegate:anObject { return [self setServicesDelegate:anObject]; }
- requestDelegate { return [self servicesDelegate]; }

#define BUFSIZE 256


- (id)_doRequest:(const char *)requestName
    pb:(const char *)pasteboardName
    host:(const char *)host
    userData:(const char *)userData
    error:(char **)errorString
    unhide:(BOOL)unhide
{
    id pb;
    SEL uid;
    int selectorLength;
    char selectorBuffer[BUFSIZE];
    char *selector = selectorBuffer;

    if (requestName) {
	*errorString = NULL;
	selectorLength = strlen(requestName)+17;
	if (selectorLength > BUFSIZE) selector = 
			(char *)NXZoneMalloc([self zone], selectorLength);
	strcpy(selector, requestName);
	strcat(selector, ":userData:error:");
	if (uid = sel_getUid(selector)) {
	    pb = [Pasteboard _newName:pasteboardName host:host];
	    if (pb && [_requestDelegate respondsTo:uid]) {
		if (unhide) {
		    [NXApp unhideWithoutActivation:self];
		    [NXApp activateSelf:NO];
		}
		objc_msgSend(_requestDelegate, uid, pb, userData, errorString);
		if (!*errorString) *errorString = "";
		return pb;
	    }
	}
    }

    if (!*errorString) *errorString = "";

    return nil;
}

- (int)_request:(const char *)requestName
    pb:(const char *)pasteboardName
    host:(const char *)host
    userData:(const char *)userData
    error:(char **)errorString
{
    return [self _doRequest:requestName pb:pasteboardName host:host userData:userData error:errorString unhide:NO] ? 0 : -1;
}

- (int)_rmError:(const char *)menuEntry
    app:(const char *)application
    error:(const char *)error
{
    if (error && *error) {
	_NXKitAlert("ServicesMenu", "Services", "Service \"%s\" could not be provided by %s: %s", NULL, NULL, NULL, menuEntry, application, error);
    }
    return 0;
}

- (int)_request:(const char *)requestName
    pb:(const char *)pasteboardName
    host:(const char *)host
    userData:(const char *)userData
    app:(const char *)appName
    menu:(const char *)menuEntry
    error:(port_t)errorPort
    unhide:(int)flag
{
    id pb, speaker;
    char *error = NULL;

    if (pb = [self _doRequest:requestName pb:pasteboardName host:host userData:userData error:&error unhide:flag ? YES : NO]) {
	if (error && *error) {
	    speaker = [NXApp appSpeaker];
	    if (!speaker) speaker = errorSpeaker;
	    if (!speaker) speaker = errorSpeaker = [Speaker new];
	    [speaker setSendPort:errorPort];
	    [speaker _rmError:menuEntry app:appName error:error];
	    /* ??? get the pb and freeGlobally here */
	}
	[pb freeGlobally];
	return 0;
    }

    return -1;
}

- (int)_getWindowServerMemory:(int *)virtualMemory backing:(int *)backing
{
    if (NXGetWindowServerMemory(NULL, virtualMemory, backing, NULL) == 0) {
	return 0;
    }
    return -1;
}

- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "@ii@@", &delegate, &timeout, &priority, &_delegate2, &_requestDelegate);
    return self;
}


- read:(NXTypedStream *) stream
{
    int version;

    [super read:stream];
    portName = NULL;
    version = NXTypedStreamClassVersion(stream, "Listener");
    if (version < 1) {
	NXReadTypes(stream, "@ii@", &delegate, &timeout, &priority, &_delegate2);
    } else {
	NXReadTypes(stream, "@ii@@", &delegate, &timeout, &priority, &_delegate2, &_requestDelegate);
    }
    listenPort = PORT_NULL;
    signaturePort = PORT_NULL;

    return self;
}


@end

/*
Modifications (starting at 0.8):
  
1/23/89  cmf    Removed unnecessary awake method.
2/7/89	 cmf	Added ok : (int *) flag to all msg[*] methods
2/7/89   cmf	Added unmounting:ok: method

0.91
----
 5/19/89 trey	minimized static data

0.93
----
 6/15/89 pah	make portName return const char *
 7/25/89 trey	added openTempFile:ok:

80
--
  4/8/90 pah	added request server support
		 setRequestDelegate: sets the object to which request menu
		  requests are sent
		 the new _request:... messages are translated into messages
		  sent to the requestDelegate
		 perhaps we should reevaluate whether this is the best API
		  for the request stuff (since it really rolls two different
		  but similar functionalities into one object)
		  we can't really have a subclass of Listener because you
		   want to be able to use the appListener.
		  we can't really use the Listener's delegate's delegate since
		   that changes its semantic a bit too much

83
--
  5/3/90 pah	freed up the pasteboard used to pass asynchronous request data
		unhide application before servicing asynchronous request

86
--
 6/11/90 aozer	Added _getWindowServerMemory:backing:
 
87
--
 6-30-90 wot	We now free the portName in checkOut:.  This was
 		causing a memory leak.

104
---
 11/2/90 aozer	Added NXPortNameLookup(); bug 12021.
 
*/

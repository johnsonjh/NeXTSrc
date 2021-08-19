/*
	Speaker.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Greg Cockroft
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Speaker.h"
#import "Listener.h"
#import "errors.h"
#import <mach.h>
#import <stdarg.h>
#import <sys/message.h>
#import <servers/netname.h>
#import <string.h>
extern port_t       name_server_port;

#import "listenerprivate.h"


extern id           NXApp;

extern int          NXMustFreeRPCVM;	/* TRUE iff must free VM and ports in
					 * NXRPCMessage */
extern NXMessage    NXRPCMessage;	/* Message that holds last RPC result
					 * message */
extern char         NXParamTypes[NX_MAXMSGPARAMS + 1];	/* types of current
							 * messages parameters */

@implementation Speaker

+ new
{
    return [[super allocFromZone:NXDefaultMallocZone()] init];
}

- init
{
    [super init];
    sendPort = PORT_NULL;
    replyPort = PORT_NULL;
    sendTimeout = _NXNetTimeout;
    replyTimeout = _NXNetTimeout;
    return self;
}

- free
{
    return [super free];
}


- delegate
{
    return delegate;
}

- setDelegate:anObject
{
    delegate = anObject;
    return self;
}

- (port_t)sendPort
{
    return sendPort;
}

- setSendPort:(port_t)aPort
{
    sendPort = aPort;
    return self;
}

- (port_t)replyPort
{
    return replyPort;
}

- setReplyPort:(port_t)aPort
{
    replyPort = aPort;
    return self;
}

- (int)sendTimeout
{
    return sendTimeout;
}

- setSendTimeout:(int)ms
{
    sendTimeout = ms;
    return self;
}

- (int)replyTimeout
{
    return replyTimeout;
}

- setReplyTimeout:(int)ms
{
    replyTimeout = ms;
    return self;
}

- (int)sendOpenFileMsg:(const char *)fullPath ok:(int *)flag andDeactivateSelf:(BOOL)doDeact
{
    if (doDeact)
	[NXApp deactivateSelf];
    if (NXApp) {
	_NXSetLastOpenFileTime();
	NXPing();
    }
    return[self selectorRPC:"openFile:ok:" paramTypes:"cI", fullPath, flag];
}

- (int)sendOpenTempFileMsg:(const char *)fullPath ok:(int *)flag andDeactivateSelf:(BOOL)doDeact
{
    if (doDeact && [NXApp respondsTo:@selector(deactivateSelf)])
	[NXApp deactivateSelf];
    return[self selectorRPC:"openTempFile:ok:" paramTypes:"cI", fullPath, flag];
}


- (int)performRemoteMethod:(const char *)msgSelector
{
    return[self selectorRPC:msgSelector paramTypes:""];
}

- (int)performRemoteMethod:(const char *)msgSelector with:(const char *)data
    length:(int)numBytes
{
    return[self selectorRPC:msgSelector paramTypes:"b", data, numBytes];
}

- (int)selectorRPC:(const char *)msgSelector paramTypes:(char *)params,...
{
    register va_list    ap;
    va_list             ae;
    register char      *p;
    char               *cval;
    int                 ival;
    double              dval;
    port_t              pval;
    char              **pcval;
    int                *pival = NULL;
    double             *pdval;
    port_t             *ppval;
    int                 ec;
    int                 len;
    int                 moffset;
    register int        esize = 0;
    register int        msize;
    int                 arrays;
    int                 asize;
    int                 inLine;
    register msg_type_t *st;
    register msg_type_t *endc;
    msg_type_long_t    *lt;
    int                 rpc;
    int                 sequence;
    int                 mtype;
    port_t              rport;
    int                 noPorts;

    rport = replyPort;
    if (!rport)
	rport = [NXApp replyPort];
    if (!rport) {
	ec = port_allocate(task_self(), &rport);
	if (ec == KERN_SUCCESS)
	    replyPort = rport;
	else
	    return ec;
    }
    if (NXMustFreeRPCVM) {
	NXMustFreeRPCVM = 0;
	NXParamTypes[0] = '\0';
	NXFreeMsgVM(&NXRPCMessage, NO);
    }
    NXRPCMessage.header.msg_type = MSG_TYPE_NORMAL;
    NXRPCMessage.header.msg_local_port = rport;
    NXRPCMessage.header.msg_remote_port = sendPort;

    NXRPCMessage.sequenceType.msg_type_name = MSG_TYPE_INTEGER_32;
    NXRPCMessage.sequenceType.msg_type_size = sizeof(int) * 8;
    NXRPCMessage.sequenceType.msg_type_number = 1;
    NXRPCMessage.sequenceType.msg_type_inline = TRUE;
    NXRPCMessage.sequenceType.msg_type_longform = FALSE;
    NXRPCMessage.sequenceType.msg_type_deallocate = FALSE;
    sequence = NXRPCMessage.sequence = NXNextSequence();

    len = strlen(msgSelector) + 1;
    NXRPCMessage.actionType.msg_type_name = MSG_TYPE_BYTE;
    NXRPCMessage.actionType.msg_type_size = 8;
    NXRPCMessage.actionType.msg_type_number = len;
    NXRPCMessage.actionType.msg_type_inline = TRUE;
    NXRPCMessage.actionType.msg_type_longform = FALSE;
    NXRPCMessage.actionType.msg_type_deallocate = FALSE;
    bcopy(msgSelector, NXRPCMessage.action, len);

    rpc = NO;
    moffset = (int)(NXRPCMessage.action - (char *)&NXRPCMessage) + ((len + 3) / 4) * 4;
    msize = moffset;
    p = params;
    noPorts = 1;
    arrays = 0;
    asize = 0;
    va_start(ap, params);
    while (*p) {
	switch (*p) {
	case NX_MSG_DOUBLE_IN:
	    msize += sizeof(double) + sizeof(msg_type_t);
	    dval = va_arg(ap, double);
	    break;
	case NX_MSG_INT_IN:
	    msize += sizeof(int) + sizeof(msg_type_t);
	    ival = va_arg(ap, int);
	    break;
	case NX_MSG_PORT_RCV_IN:
	case NX_MSG_PORT_SEND_IN:
	    noPorts = 0;
	    msize += sizeof(port_t) + sizeof(msg_type_t);
	    pval = va_arg(ap, port_t);
	    break;
	case NX_MSG_BYTES_IN:
	    cval = va_arg(ap, char *);
	    ival = va_arg(ap, int);
    sizeBytes:
	    arrays++;
	    ival = ((ival + 3) / 4) * 4;
	    asize += ival;
	    msize += sizeof(msg_type_t);
	    break;
	case NX_MSG_CHARS_IN:
	    cval = va_arg(ap, char *);
	    ival = strlen(cval) + 1;
	    goto sizeBytes;
	case NX_MSG_PORT_RCV_OUT:
	case NX_MSG_PORT_SEND_OUT:
	case NX_MSG_DOUBLE_OUT:
	case NX_MSG_INT_OUT:
	case NX_MSG_CHARS_OUT:
	    rpc = YES;
	    pcval = va_arg(ap, char **);
	    break;
	case NX_MSG_BYTES_OUT:
	    rpc = YES;
	    pcval = va_arg(ap, char **);
	    pival = va_arg(ap, int *);
	    break;
	}
	p++;
    }
    ae = ap;

    NXRPCMessage.header.msg_id = (rpc ? NX_SELECTORFMSG : NX_SELECTORPMSG);
    if ((msize + asize) < sizeof(NXMessage)) {
	inLine = TRUE;
	NXRPCMessage.header.msg_size = msize + asize;
	NXRPCMessage.header.msg_simple = noPorts;
    } else {
	inLine = FALSE;
	NXRPCMessage.header.msg_size = msize +
	  arrays *
	  (sizeof(msg_type_long_t) -
	   sizeof(msg_type_t) +
	   sizeof(p));
	NXRPCMessage.header.msg_simple = FALSE;
    }

    if (NXRPCMessage.header.msg_size > sizeof(NXMessage)) {
	ec = SEND_MSG_TOO_LARGE;
	goto exitPoint;
    }
    p = params;
    st = (msg_type_t *)((char *)&NXRPCMessage + moffset);
    va_start(ap, params);
    while (*p) {
	switch (*p) {
	case NX_MSG_DOUBLE_IN:
	    msize = sizeof(double);
	    esize = sizeof(msg_type_t);
	    st->msg_type_name = MSG_TYPE_REAL;
	    st->msg_type_size = sizeof(double) * 8;
	    st->msg_type_number = 1;
	    st->msg_type_inline = TRUE;
	    st->msg_type_longform = FALSE;
	    st->msg_type_deallocate = FALSE;
	    *((double *)((char *)st + esize)) = va_arg(ap, double);
	    break;
	case NX_MSG_INT_IN:
	    msize = sizeof(int);
	    esize = sizeof(msg_type_t);
	    st->msg_type_name = MSG_TYPE_INTEGER_32;
	    st->msg_type_size = sizeof(int) * 8;
	    st->msg_type_number = 1;
	    st->msg_type_inline = TRUE;
	    st->msg_type_longform = FALSE;
	    st->msg_type_deallocate = FALSE;
	    *((int *)((char *)st + esize)) = va_arg(ap, int);
	    break;
	case NX_MSG_PORT_RCV_IN:
	case NX_MSG_PORT_SEND_IN:
	    msize = sizeof(port_t);
	    esize = sizeof(msg_type_t);
	    st->msg_type_name =
	      (*p == NX_MSG_PORT_RCV_IN ? MSG_TYPE_PORT_ALL :
	       MSG_TYPE_PORT);
	    st->msg_type_size = sizeof(port_t) * 8;
	    st->msg_type_number = 1;
	    st->msg_type_inline = TRUE;
	    st->msg_type_longform = FALSE;
	    st->msg_type_deallocate = FALSE;
	    *((port_t *)((char *)st + esize)) = va_arg(ap, port_t);
	    break;
	case NX_MSG_BYTES_IN:
	    cval = va_arg(ap, char *);
	    msize = va_arg(ap, int);
    putBytes:
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
	case NX_MSG_CHARS_IN:
	    cval = va_arg(ap, char *);
	    msize = strlen(cval) + 1;
	    goto putBytes;
	case NX_MSG_PORT_RCV_OUT:
	case NX_MSG_PORT_SEND_OUT:
	case NX_MSG_DOUBLE_OUT:
	case NX_MSG_INT_OUT:
	case NX_MSG_CHARS_OUT:
	    pcval = va_arg(ap, char **);
	    goto continuePt1;
	case NX_MSG_BYTES_OUT:
	    pcval = va_arg(ap, char **);
	    pival = va_arg(ap, int *);
	    goto continuePt1;
	}
	st = (msg_type_t *)((char *)st + esize + msize);
continuePt1:
	p++;
    }

    TRACE4("%s=send %s on %d %d\n", [NXApp appName], rpc?"rpc":"send", NXRPCMessage.header.msg_remote_port, NXRPCMessage.header.msg_local_port);
    ec = _NXSafeSend(&NXRPCMessage,
		   sendTimeout ? (SEND_TIMEOUT | SEND_SWITCH) : SEND_SWITCH,
		     sendTimeout);
    if (rpc && (ec == SEND_SUCCESS)) {
	NXRPCMessage.header.msg_size = sizeof(NXMessage);
	ec = _NXSafeReceive(&NXRPCMessage,
			    replyTimeout ? RCV_TIMEOUT : MSG_OPTION_NONE,
			    replyTimeout);
    }
    TRACE3("%s=%s sent %d\n", [NXApp appName], rpc ? "rpc" : "send", ec);
    if (rpc && (ec == RCV_SUCCESS)) {
	mtype = NXRPCMessage.header.msg_id;
	if (!NXValidMessage(&NXRPCMessage) || NXRPCMessage.sequence != sequence) {
	    ec = NXReceiveMessage(&NXRPCMessage, rport, sequence, replyTimeout, &mtype);
	    if (ec < 0)
		goto exitPoint;
	}
	if (mtype != NX_RESPONSEMSG) {
	    if (mtype == NX_ACKNOWLEDGE)
		ec = ((NXAcknowledge *)&NXRPCMessage)->error;
	    goto errorPt;
	}
	NXMustFreeRPCVM = 1;
	p = params;
	st = (msg_type_t *)&NXRPCMessage.actionType;
	va_start(ap, params);
	endc = (msg_type_t *)((char *)&NXRPCMessage + NXRPCMessage.header.msg_size);
	while (*p) {
	    switch (*p) {
	    case NX_MSG_DOUBLE_IN:
		dval = va_arg(ap, double);
		goto continuePt2;
	    case NX_MSG_INT_IN:
		ival = va_arg(ap, int);
		goto continuePt2;
	    case NX_MSG_PORT_RCV_IN:
	    case NX_MSG_PORT_SEND_IN:
		pval = va_arg(ap, port_t);
		goto continuePt2;
	    case NX_MSG_BYTES_IN:
		cval = va_arg(ap, char *);
		ival = va_arg(ap, int);
		goto continuePt2;
	    case NX_MSG_CHARS_IN:
		cval = va_arg(ap, char *);
		goto continuePt2;
	    case NX_MSG_DOUBLE_OUT:
		if (st >= endc)
		    goto errorPt;
		msize = sizeof(double);
		esize = sizeof(msg_type_t);
		if (st->msg_type_name != MSG_TYPE_REAL ||
		    st->msg_type_size != sizeof(double) * 8 ||
		    st->msg_type_number != 1 ||
		    !st->msg_type_inline ||
		    st->msg_type_longform)
		    goto errorPt;
		pdval = va_arg(ap, double *);
		*pdval = *((double *)((char *)st + esize));
		break;
	    case NX_MSG_INT_OUT:
		if (st >= endc)
		    goto errorPt;
		msize = sizeof(int);
		esize = sizeof(msg_type_t);
		if (st->msg_type_name != MSG_TYPE_INTEGER_32 ||
		    st->msg_type_size != sizeof(int) * 8 ||
		    st->msg_type_number != 1 ||
		    !st->msg_type_inline ||
		    st->msg_type_longform)
		    goto errorPt;
		pival = va_arg(ap, int *);
		*pival = *((int *)((char *)st + esize));
		break;
	    case NX_MSG_PORT_RCV_OUT:
	    case NX_MSG_PORT_SEND_OUT:
		if (st >= endc)
		    goto errorPt;
		msize = sizeof(port_t);
		esize = sizeof(msg_type_t);
		if (*p == NX_MSG_PORT_RCV_OUT) {
		    if (st->msg_type_name != MSG_TYPE_PORT_ALL)
			goto errorPt;
		} else if (st->msg_type_name != MSG_TYPE_PORT)
		    goto errorPt;
		if (st->msg_type_size != sizeof(port_t) * 8 ||
		    st->msg_type_number != 1 ||
		    !st->msg_type_inline ||
		    st->msg_type_longform)
		    goto errorPt;
		ppval = va_arg(ap, port_t *);
		*ppval = *((port_t *)((char *)st + esize));
		break;
	    case NX_MSG_CHARS_OUT:
		pcval = va_arg(ap, char **);
		goto getBytes;
	    case NX_MSG_BYTES_OUT:
		pcval = va_arg(ap, char **);
		pival = va_arg(ap, int *);
	getBytes:
		if (st >= endc)
		    goto errorPt;
		inLine = st->msg_type_inline;
		if (inLine == st->msg_type_longform)
		    goto errorPt;
		if (inLine) {
		    esize = sizeof(msg_type_t);
		    if (st->msg_type_name != MSG_TYPE_BYTE ||
			st->msg_type_size != 8)
			goto errorPt;
		    len = msize = st->msg_type_number;
		    if (*p == NX_MSG_BYTES_OUT)
			*pival = len;
		    *pcval = ((char *)st + esize);
		} else {
		    lt = (msg_type_long_t *)st;
		    esize = sizeof(msg_type_long_t);
		    if (lt->msg_type_long_name != MSG_TYPE_BYTE ||
			lt->msg_type_long_size != 8)
			goto errorPt;
		    len = lt->msg_type_long_number;
		    if (*p == NX_MSG_BYTES_OUT)
			*pival = len;
		    *pcval = *((char **)((char *)st + esize));
		    msize = sizeof(p);
		}
		if (*p == NX_MSG_CHARS_OUT && (len <= 0 || (*pcval)[len - 1]))
		    goto errorPt;
		msize = ((msize + 3) / 4) * 4;
	    }
	    st = (msg_type_t *)((char *)st + esize + msize);
    continuePt2:
	    p++;
	}
	goto exitPoint;
errorPt:
	NXFreeMsgVM(&NXRPCMessage, YES);
	if (!ec)
	    ec = -1;
    }
exitPoint:
    if (!ec)
	strcpy(NXParamTypes, params);
    va_end(ae);
    return ec;
}

- (int)openFile:(const char *)fullPath ok:(int *)flag
{
    return[self sendOpenFileMsg:fullPath ok:flag andDeactivateSelf:YES];
}

- (int)openTempFile:(const char *)fullPath ok:(int *)flag
{
    return[self sendOpenTempFileMsg:fullPath ok:flag andDeactivateSelf:YES];
}

- (int)launchProgram:(const char *)name ok:(int *)flag
{
    return[self selectorRPC:"launchProgram:ok:"
	   paramTypes:"cI", name, flag];
}

- (int)iconEntered:(int)windowNum at:(double)x :(double)y
    iconWindow:(int)iconWindowNum
    iconX:(double)iconX
    iconY:(double)iconY
    iconWidth:(double)iconWidth
    iconHeight:(double)iconHeight
    pathList:(const char *)pathList
{
    return[self
	   selectorRPC:"iconEntered:at::iconWindow:iconX:iconY:iconWidth:iconHeight:pathList:"
	   paramTypes:"iddiddddc",
	   windowNum, x, y,
	   iconWindowNum, iconX, iconY, iconWidth, iconHeight,
	   pathList];
}

- (int)iconMovedTo:(double)x :(double)y
{
    return[self selectorRPC:"iconMovedTo::" paramTypes:"dd", x, y];
}

- (int)iconReleasedAt:(double)x :(double)y ok:(int *)flag
{
    return[self selectorRPC:"iconReleasedAt::ok:" paramTypes:"ddI", x, y, flag];
}

- (int)iconExitedAt:(double)x :(double)y
{
    return[self selectorRPC:"iconExitedAt::" paramTypes:"dd", x, y];
}

- (int)registerWindow:(int)windowNum toPort:(port_t)aPort
{
    return[self selectorRPC:"registerWindow:toPort:" paramTypes:"is",
	   windowNum, aPort];
}

- (int)unregisterWindow:(int)windowNum
{
    return[self selectorRPC:"unregisterWindow:" paramTypes:"i", windowNum];
}


-(int)getFileInfoFor:(char *)fullPath app:(char **)name type:(char **)type
    ilk:(int *)ilk ok:(int *)flag
{
    return [self selectorRPC:"getFileInfoFor:app:type:ilk:ok:" 
	paramTypes:"cCCII", fullPath, name, type, ilk, flag];
}

-(int)getFileIconFor:(char *)fullPath TIFF:(char **)tiff
    TIFFLength:(int *)length ok:(int *)flag
{
    return [self selectorRPC:"getFileIconFor:TIFF:TIFFLength:ok:"
	paramTypes:"cBI", fullPath, tiff, length, flag];
}

- (int)msgQuit:(int *)flag
{
    return[self selectorRPC:"msgQuit:" paramTypes:"I", flag];
}

- (int)msgCalc:(int *)flag
{
    return[self selectorRPC:"msgCalc:" paramTypes:"I", flag];
}

- (int)msgDirectory:(char *const *)fullPath ok:(int *)flag
{
    return[self selectorRPC:"msgDirectory:ok:" paramTypes:"CI", fullPath, flag];
}

- (int)msgVersion:(char *const *)aString ok:(int *)flag
{
    return[self selectorRPC:"msgVersion:ok:" paramTypes:"CI", aString, flag];
}

- (int)msgFile:(char *const *)fullPath ok:(int *)flag
{
    return[self selectorRPC:"msgFile:ok:" paramTypes:"CI", fullPath, flag];
}

- (int)msgPrint:(const char *)fullPath ok:(int *)flag
{
    return[self selectorRPC:"msgPrint:ok:" paramTypes:"cI", fullPath, flag];
}

- (int)msgSelection:(char *const *)bytes length:(int *)len
    asType:(const char *)aType ok:(int *)flag
{
    return[self selectorRPC:"msgSelection:length:asType:ok:"
	   paramTypes:"BcI", bytes, len, aType, flag];
}

- (int)msgSetPosition:(const char *)aString posType:(int)anInt
    andSelect:(int)sflag ok:(int *)flag
{
    return[self selectorRPC:"msgSetPosition:posType:andSelect:ok:"
	   paramTypes:"ciiI", aString, anInt, sflag, flag];
}

- (int)msgPosition:(char *const *)aString posType:(int *)anInt ok:(int *)flag
{
    return[self selectorRPC:"msgPosition:posType:ok:"
	   paramTypes:"CII", aString, anInt, flag];
}

- (int)msgCopyAsType:(const char *)aType ok:(int *)flag
{
    return[self selectorRPC:"msgCopyAsType:ok:" paramTypes:"cI", aType, flag];
}

- (int)msgCutAsType:(const char *)aType ok:(int *)flag
{
    return[self selectorRPC:"msgCutAsType:ok:" paramTypes:"cI", aType, flag];
}

- (int)msgPaste:(int *)flag
{
    return[self selectorRPC:"msgPaste:" paramTypes:"I", flag];
}

- (int)unmounting:(const char *)fullPath ok:(int *)flag
{
    return[self selectorRPC:"unmounting:ok:" paramTypes:"cI", fullPath, flag];
}

- (int)powerOffIn:(int)ms andSave:(int)aFlag
{
    return[self selectorRPC:"powerOffIn:andSave:" paramTypes:"ii", ms, aFlag];
}

- (int)extendPowerOffBy:(int)requestedMs actual:(int *)actualMs
{
    return[self selectorRPC:"extendPowerOffBy:actual:" paramTypes:"iI",
	   requestedMs, actualMs];
}

- (int)_initJournaling:(int)appNum
     :(port_t) masterPort
     :(port_t *) slavePortPtr
{
    return[self selectorRPC:"_initJournaling:::"
	   paramTypes:"isS", appNum, masterPort, slavePortPtr];
}

- (int)_requestInitJournaling:(const char *)appName     
     :(port_t) slavePort
     :(int *)appNumPtr
     :(port_t *) masterPortPtr
{
    return[self selectorRPC:"_requestInitJournaling::::"
	   paramTypes:"csIS", appName, slavePort, appNumPtr, masterPortPtr];
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
    return [self selectorRPC:"_request:pb:host:userData:app:menu:error:unhide:"
		  paramTypes:"ccccccsi", requestName, pasteboardName, host,
					userData, appName, menuEntry,
					errorPort, flag];
}

- (int)_request:(const char *)requestName
    pb:(const char *)pasteboardName
    host:(const char *)host
    userData:(const char *)userData
    error:(char **)errorString
{
    return [self selectorRPC:"_request:pb:host:userData:error:"
		  paramTypes:"ccccC", requestName, pasteboardName, host,
				      userData, errorString];
}

- (int)_rmError:(const char *)menuEntry
    app:(const char *)application
    error:(const char *)error
{
    return [self selectorRPC:"_rmError:app:error:" paramTypes:"ccc",
		menuEntry, application, error];
}

- (int)_getWindowServerMemory:(int *)virtualMemory backing:(int *)backing
{
    return [self selectorRPC:"_getWindowServerMemory:backing:"
		paramTypes:"II", virtualMemory, backing];
}

- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "@ii", &delegate, &sendTimeout, &replyTimeout);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    sendPort = PORT_NULL;
    replyPort = PORT_NULL;
    NXReadTypes(stream, "@ii", &delegate, &sendTimeout, &replyTimeout);
    return self;
}


/* NXCopyOutputData returns a vm_allocated pointer to the parameter in position
 * parameter. NOTE: the byte array pointer and the byte array length count for
 * only one parameter, even thought they are 2 obj-c parameters. if data was
 * not inline, we clear ptr in msg so that when NXFreeMsgVM is called, we won't
 * free this data.  if data was inline we vm_allocate and copy the
 * data.  consequently, NXCopyOutputData is moderately expensive for
 * inline data and you might want to malloc and copy it.
 *
 * returns null on vm_allocate failure.
 *
 * do not access field after this operation!!!
 */
char               *
NXCopyOutputData(parameter)
    register int        parameter;
{
    char               *vmp;
    register char      *p;
    register msg_type_t *st;
    register int        msize = 0;
    register int        esize = 0;

    p = NXParamTypes;
    if (!NXMustFreeRPCVM || !*p)
	return (char *)0;

    st = (msg_type_t *)&NXRPCMessage.actionType;
    while (*p && (parameter >= 0)) {
	switch (*p) {
	case NX_MSG_DOUBLE_OUT:
	    msize = sizeof(double);
	    esize = sizeof(msg_type_t);
	    break;
	case NX_MSG_INT_OUT:
	    msize = sizeof(int);
	    esize = sizeof(msg_type_t);
	    break;
	case NX_MSG_PORT_RCV_OUT:
	case NX_MSG_PORT_SEND_OUT:
	    msize = sizeof(port_t);
	    esize = sizeof(msg_type_t);
	    break;
	case NX_MSG_CHARS_OUT:
	case NX_MSG_BYTES_OUT:
	    if (st->msg_type_inline) {
		esize = sizeof(msg_type_t);
		msize = st->msg_type_number;
		if (!parameter) {
		    kern_return_t kr;
		    kr = vm_allocate(task_self(), (vm_address_t *)&vmp, msize, YES);
		    if (kr != KERN_SUCCESS) {
			NX_RAISE(NX_appkitVMError, (void *)kr,
			    "vm_allocate() in Speaker");
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
	case NX_MSG_PORT_RCV_IN:
	case NX_MSG_PORT_SEND_IN:
	case NX_MSG_DOUBLE_IN:
	case NX_MSG_INT_IN:
	case NX_MSG_BYTES_IN:
	case NX_MSG_CHARS_IN:
	    goto continuePt1;
	}
	st = (msg_type_t *)((char *)st + esize + msize);
continuePt1:
	p++;
	parameter--;
    }
    return (char *)0;
}

@end

/*
Modifications (starting at 0.8):
 
 1/12/89 wrp	Removed commented out code for old form of varargs usage.
 1/23/89 cmf	Removed unnecessary awake method.
 2/07/89 cmf	Added ok : (int *) flag to all msg[*] methods
 2/07/89 cmf	Added unmounting:ok: method
 2/07/89 cmf	Fixed bug in unregisterWindow:
 7/25/89 trey	added openTempFile:ok:
 8/08/89 bgy	added getFileInfoFor:... and getFileIconFor:...

80
--
  4/7/90 pah	added _request: methods to support Request Menu

86
--
 6/11/90 aozer	Added _getWindowServerMemory:backing: to allow monitor apps
		to find out about server memory usage for an app. 

95
--
 10/1/90 trey	fixed but where replyPort we port_alloc'ed wasnt retained, 
		 causing us to realloc them per message 
*/

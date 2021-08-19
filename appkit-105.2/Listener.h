/*
	Listener.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import <sys/port.h>
#import <sys/message.h>

/*
 * Functions to enable/disable Services Menu items.  These should usually
 * only be called by service PROVIDERS (since they are the only ones who
 * know the name of the services, requestors don't).  The itemName in the
 * two functions below is the language-independent "Menu Item:" entry in
 * the __services section (which all provided services must have).  The
 * set function returns whether it was successful.
 * NXUpdateDynamicServices() causes the services information for the
 * system to be updated.  This will only be necessary if your program
 * adds dynamic services to the system (i.e. services not found in macho
 * segments of executables).
 */

extern BOOL NXIsServicesMenuItemEnabled(const char *itemName);
extern int NXSetServicesMenuItemEnabled(const char *itemName, BOOL enabled);
extern void NXUpdateDynamicServices(void);

/*
 * Names of workspace ports for requests and for acknowledging machlaunch
 * from the Workspace Manager.
 */

extern const char *NXWorkspaceName;
extern const char *const NXWorkspaceReplyName;
#define NX_WORKSPACEREQUEST NXWorkspaceName
#define NX_WORKSPACEREPLY NXWorkspaceReplyName

/* reserved message numbers */

#define NX_SELECTORPMSG 35555
#define NX_SELECTORFMSG 35556
#define NX_RESPONSEMSG 35557
#define NX_ACKNOWLEDGE 35558

/* rpc return result error returns */

#define NX_INCORRECTMESSAGE -20000

/* maximum number of remote method parameters allowed */

#define NX_MAXMSGPARAMS 20

/* default timeouts in milliseconds */

#define NX_SENDTIMEOUT 10000
#define NX_RCVTIMEOUT 10000

#define NX_MAXMESSAGE	(2048-sizeof(msg_header_t)-\
			 sizeof(msg_type_t)-sizeof(int)-\
			 sizeof(msg_type_t)-8)
			 
typedef struct _NXMessage {	/* a message via mach */
    msg_header_t header;	/* every message has one of these */
    msg_type_t sequenceType;	/* sequence number type */
    int sequence;		/* sequence number */
    msg_type_t actionType;	/* selector string */
    char action[NX_MAXMESSAGE];
} NXMessage;

typedef struct _NXResponse {	/* a message via mach */
    msg_header_t header;	/* every message has one of these */
    msg_type_t sequenceType;	/* sequence number type */
    int sequence;		/* sequence number */
} NXResponse;

typedef struct _NXAcknowledge {	/* a message via mach */
    msg_header_t header;	/* every message has one of these */
    msg_type_t sequenceType;	/* sequence number type */
    int sequence;		/* sequence number */
    msg_type_t errorType;	/* error number type */
    int error;			/* error number, 0 is ok */
} NXAcknowledge;

typedef struct _NXRemoteMethod {/* defines method understood by Listener */
    SEL key;			/* obj-c selector */
    char *types;		/* defines types of parameters */
} NXRemoteMethod;

typedef union {			/* used to pass parameters to method */
    int ival;
    double dval;
    port_t pval;
    struct _bval {
        char *p;
        int len;
    } bval;
} NXParamValue;

extern char *NXCopyInputData(int parameter);
extern char *NXCopyOutputData(int parameter);
extern int NXReceiveMessage(NXMessage *msg, port_t receivePort, int sequence, 
	int timeout, int *rtype);
extern int NXWaitForMessage(port_t receivePort, int sequence, 
	port_t *replyPort, int timeout);
extern int NXValidMessage(NXMessage *msg);
extern int NXSendAcknowledge(port_t sendPort, port_t replyPort,
	int msgSequence, int error, int timeout);
extern int NXNextSequence(void);
extern void NXFreeMsgVM(NXMessage *msg, int freePortsToo);
extern port_t NXPortFromName(const char *portName, const char *hostName);
extern port_t NXPortNameLookup(const char *portName, const char *hostName);
extern NXRemoteMethod *NXRemoteMethodFromSel(SEL s, NXRemoteMethod *pt);
extern id NXResponsibleDelegate(id self, SEL selector);

/*
 * permissible values of posType for setPosition:posType:andSelect:
 * and postion:posType:
 */
 
#define NX_TEXTPOSTYPE 0
#define NX_REGEXPRPOSTYPE 1
#define NX_LINENUMPOSTYPE 2
#define NX_CHARNUMPOSTYPE 3
#define NX_APPPOSTYPE 4

@interface Listener : Object
{
    char               *portName;
    port_t              listenPort;
    port_t              signaturePort;
    id                  delegate;
    int                 timeout;
    int                 priority;
    id                  _delegate2;
    id                  _requestDelegate;
    int                 _reservedListener2;
}
 
+ initialize;
+ run;

- init;
- free;
- (int)timeout;
- setTimeout:(int)ms;
- (int)priority;
- setPriority:(int)level;
- (port_t)listenPort;
- (port_t)signaturePort;
- delegate;
- setDelegate:anObject;
- setServicesDelegate:anObject;
- servicesDelegate;
- (const char *)portName;
- (int)checkInAs:(const char *)name;
- (int)usePrivatePort;
- (int)checkOut;
- addPort;
- removePort;
- (int)openFile:(const char *)fullPath ok:(int *)flag;
- (int)openTempFile:(const char *)fullPath ok:(int *)flag;
- (int)unhide;
- (int)launchProgram:(const char *)name ok:(int *)flag;
- (int)iconEntered:(int)windowNum at:(double)x :(double)y iconWindow:(int)iconWindowNum iconX:(double)iconX iconY:(double)iconY iconWidth:(double)iconWidth iconHeight:(double)iconHeight pathList:(const char *)pathList;
- (int)iconMovedTo:(double)x :(double)y;
- (int)iconReleasedAt:(double)x :(double)y ok:(int *)flag;
- (int)iconExitedAt:(double)x :(double)y;
- (int)registerWindow:(int)windowNum toPort:(port_t)aPort;
- (int)unregisterWindow:(int)windowNum;
- (int)getFileInfoFor:(char *)fullPath app:(char **)appname type:(char **)type ilk:(int *)ilk ok:(int *)flag;
- (int)getFileIconFor:(char *)fullPath TIFF:(char **)tiff TIFFLength:(int *)length ok:(int *)flag;
- (int)msgQuit:(int *)flag;
- (int)msgCalc:(int *)flag;
- (int)msgDirectory:(char * const *)fullPath ok:(int *)flag;
- (int)msgVersion:(char * const *)aString ok:(int *)flag;
- (int)msgFile:(char * const *)fullPath ok:(int *)flag;
- (int)msgPrint:(const char *)fullPath ok:(int *)flag;
- (int)msgSelection:(char * const *)bytes length:(int *)len asType:(const char *)aType ok:(int *)flag;
- (int)msgSetPosition:(const char *)aString posType:(int)anInt andSelect:(int)sflag ok:(int *)flag;
- (int)msgPosition:(char * const *)aString posType:(int *)anInt ok:(int *)flag;
- (int)msgCopyAsType:(const char *)aType ok:(int *)flag;
- (int)msgCutAsType:(const char *)aType ok:(int *)flag;
- (int)msgPaste:(int *)flag;
- (int)unmounting:(const char *)fullPath ok:(int *)flag;
- (int)powerOffIn:(int)ms andSave:(int)aFlag;
- (int)extendPowerOffBy:(int)requestedMs actual:(int *)actualMs;
- (int)performRemoteMethod:(NXRemoteMethod *)method paramList:(NXParamValue *)params;
- (NXRemoteMethod *)remoteMethodFor:(SEL)aSelector;
- messageReceived:(NXMessage *)msg;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;

@end

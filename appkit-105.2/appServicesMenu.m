/*
	appServices.m
	Copyright 1989, NeXT, Inc.
	Responsibility: Paul Hegarty
  	
	Category of the Application object to deal with the Services Menu.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "Listener_Private.h"
#import "Menu_Private.h"
#import "Pasteboard_Private.h"
#import "Matrix.h"
#import "MenuCell.h"
#import "Speaker.h"
#import "Text.h"
#import "Window.h"
#import "pbtypes.h"
#import "publicWraps.h"
#import "pbs.h"
#import "nextstd.h"
#import <defaults.h>
#import "perfTimer.h"
#import <objc/HashTable.h>
#import <objc/List.h>
#import <mach.h>
#import <servers/netname.h>
#import <sys/time_stamp.h>
#import <sys/file.h>
#import <libc.h>

/* Ensures category is linked.  See the main class for more details. */

#ifndef SHLIB
    asm(".NXAppCategoryServicesMenu=0\n");
    asm(".globl .NXAppCategoryServicesMenu\n");
#endif

@implementation Application(ServicesMenu)

#define byte char

#define TYPES_BUFFER_SIZE 1024

#define RM_STRING(index) (rmHeader ? \
    ((index < rmHeader->stringTableLength && index >= 0) ? \
	(rmData + rmHeader->stringTable + index) : \
	(rmData + rmHeader->stringTable)) : \
	"")

#define RM_INDR(index) (rmHeader ? \
    ((index < rmHeader->indirectStringTableLength && index >= 0) ? \
	*((int *)(rmData + rmHeader->indirectStringTable) + index) : 0) : \
	0)

#define RM_INDR_STRING(index) RM_STRING(RM_INDR(index))

#define RM_ENTRY_AT(index, entry) entryAt(index, &entry)

#define ISACTIVE 1
#define HASBEENCHECKED 2
#define INUSE 4

#define RM_HEAD(index) (rmHeader ? \
    ((index < rmHeader->headCount && index >= 0) ? \
	*((int *)(rmData + rmHeader->heads) + index) : \
	0) : \
	0)

static id registeredTypes = nil;

static id servicesMenu = nil;
static id pendingServicesMenu = nil;

static id rmSpeaker = nil;
static id rmListener = nil;

static char *rmData = NULL;
static unsigned int rmDataLength = 0;

static NXRMHeader *rmHeader = NULL;

static BOOL noServicesMenu = NO;
static BOOL servicesMenuTimings = NO;

static byte *enabledMask = NULL;
static byte *activeMask = NULL;

static NXHashTable *validTypes = NULL;

static char **invalidRequestPorts = NULL;
static int invalidRequestPortCount = 0;

static BOOL entryAt(int offset, NXRMEntry **entry)
{
    *entry = NULL;
    if (!rmHeader || offset > rmHeader->numEntries || !offset) return NO;
    offset--;
    *entry = (NXRMEntry *)(rmData + rmHeader->entries + offset * rmHeader->entrySize);
    return !(((*entry)->flags) & SERVICE_DISABLED);
}

- servicesMenu
{
    return servicesMenu ? servicesMenu : pendingServicesMenu;
}

- (BOOL)_isValidRequestPort:(const char *)name
{
    int i;

    if (!name || !*name) return NO;
    for (i = 0; i < invalidRequestPortCount; i++) {
	if (!strcmp(name, invalidRequestPorts[i])) return NO;
    }

    return YES;
}

void NXUpdateDynamicServices(void)
{
    int akserverVersion;
    _NXUpdateDynamicServices(_NXLookupAppkitServer(&akserverVersion, NULL, NULL), (char *)NXUserName(), strlen(NXUserName())+1);
}

BOOL NXIsServicesMenuItemEnabled(const char *itemName)
{
    int akserverVersion;

    return _NXIsServicesMenuItemEnabled(MAXINT, _NXLookupAppkitServer(&akserverVersion, NULL, NULL), (char *)itemName, strlen(itemName)+1, (char *)NXUserName(), strlen(NXUserName())+1);
}

int NXSetServicesMenuItemEnabled(const char *itemName, BOOL enabled)
{
    int akserverVersion;

    return _NXDoSetServicesMenuItemEnabled(MAXINT, _NXLookupAppkitServer(&akserverVersion, NULL, NULL), (char *)itemName, strlen(itemName)+1, (char *)NXUserName(), strlen(NXUserName())+1, enabled);
}

typedef struct {
    NXAtom sendType;
    NXAtom returnType;
    id requestor;
    BOOL checked;
} NXTypePair;

typedef struct {
    const NXAtom *sendTypes;
    const NXAtom *returnTypes;
} NXTypesPair;

static unsigned hashTypePair(const void *info, const void *data)
{
    unsigned retval = 0;
    NXTypePair *typePair = (NXTypePair *)data;

    if (typePair) {
	retval = typePair->sendType ? (NXPtrHash(info, typePair->sendType) << 1) : 2;
	retval += typePair->returnType ? NXPtrHash(info, typePair->returnType) : 1;
    }

    return retval;
}

static int typePairsAreEqual(const void *info, const void *data1, const void *data2)
{
    NXTypePair *tp1 = (NXTypePair *)data1;
    NXTypePair *tp2 = (NXTypePair *)data2;

    return ((tp1 ? tp1->sendType : NULL) == (tp2 ? tp2->sendType : NULL)) &&
	   ((tp1 ? tp1->returnType : NULL) == (tp2 ? tp2->returnType : NULL));
}

static NXAtom *getTypes(int index)
{
    int count, offset;
    NXAtom *types, *retval = NULL;

    if (index) {
	for (offset = index, count = 0; RM_INDR(offset); offset++, count++);
	if (count) {
	    retval = types = (NXAtom *)NXZoneMalloc([Menu menuZone], sizeof(NXAtom)*(count+1));
	    for (offset = index; RM_INDR(offset); offset++, types++) {
		*types = NXUniqueString(RM_INDR_STRING(offset));
	    }
	    *types = NULL;
	}
    }

    return retval;
}

+ _loadServicesMenuData
/*
 * Asks pbs for the services menu data (which pbs calculates by
 * looking in the machos of applications found by WSM's pass
 * through /NextApps, /LocalApps, ~/Apps to have an __services
 * section in their __ICON segment).
 */
{
    port_t server;
    int akserverVersion;

    if (!noServicesMenu) {
	server = _NXLookupAppkitServer(&akserverVersion, NULL, NULL);
	if (server && !_NXGetServicesMenuData(MAXINT, server, _NX_BYTEORDER, (char *)NXUserName(), strlen(NXUserName())+1, &rmData, &rmDataLength)) {
	    if (rmData && rmDataLength) {
		rmHeader = (NXRMHeader *)rmData;
		return self;
	    }
	}
	noServicesMenu = YES;
    }

    return nil;
}

static id findMenuItem(id matrix, int rows, char *title)
{
    int i;
    id cell;

    if (rows < 0) [matrix getNumRows:&rows numCols:&i];
    for (i = 0; i < rows; i++) {
	cell = [matrix cellAt:i:0];
	if ([cell title] && !strcmp([cell title], title)) return cell;
    }

    return nil;
}

static BOOL strEqual(const char *s1, const char *s2)
{
    char *s, buffer[256];

    if (!s1 || !s2 || strlen(s2) > 256) return NO;

    s = buffer;
    while (*s2) {
	if (*s2 != '	' && *s2 != ' ' || *s2 != '\n') *s++ = *s2;
	s2++;
    }
    *s = '\0';

    return NXOrderStrings((unsigned char *)s1, (unsigned char *)buffer, NO, -1, NULL) ? NO : YES;
}

static int getLocalizedIndexOf(int offset)
{
    const char *const *languages;
    int index, defaultIndex = 0;

    if (!offset) return 0;

    languages = [NXApp systemLanguages];
    do {
	for (index = offset; RM_INDR(index); index += 2) {
	    if (languages && strEqual(RM_INDR_STRING(index), *languages)) {
		index++;
		return RM_INDR(index);
	    } else if (!defaultIndex && !strcmp(RM_INDR_STRING(index), SERVICES_MENU_DEFAULT_LANGUAGE)) {
		defaultIndex = index+1;
		defaultIndex = RM_INDR(defaultIndex);
	    }
	}
    } while (languages && *languages++);

    return defaultIndex ? defaultIndex : RM_INDR(offset+1);
}

static const NXAtom *newTypeList(const char *const *types)
{
    int count;
    NXAtom *tp, *retval = NULL;

    if (types && *types) {
	for (tp = (char **)types, count = 0; *tp; tp++, count++);
	retval = tp = (NXAtom *)NXZoneMalloc([Menu menuZone], sizeof(NXAtom)*(count+1));
	for (; *types; tp++, types++) *tp = NXUniqueString(*types);
	*tp = NULL;
    }

    return retval;
}

static id checkForRequestor(NXAtom sendType, NXAtom returnType, id responder, BOOL useCache)
{
    id requestor;
    NXTypePair testTypePair, *typePair = NULL;

    if (useCache) {
	testTypePair.sendType = sendType;
	testTypePair.returnType = returnType;
	typePair = NXHashGet(validTypes, &testTypePair);
	if (!typePair) {
	    typePair = (NXTypePair *)NXZoneMalloc([Menu menuZone], sizeof(NXTypePair));
	    typePair->sendType = sendType;
	    typePair->returnType = returnType;
	    NXHashInsert(validTypes, typePair);
	} else {
	    if (typePair->checked) return typePair->requestor;
	}
    }
    requestor = [responder validRequestorForSendType:sendType andReturnType:returnType];
    if (useCache) {
	typePair->checked = YES;
	typePair->requestor = requestor;
    }

    return requestor;
}

static id getValidRequestor(const NXAtom *sendTypes, const NXAtom *returnTypes, BOOL useCache)
{
    NXHashState state;
    NXTypePair *typePair;
    NXHashTablePrototype proto;
    id responder, requestor, keyWindow;

    keyWindow = [NXApp keyWindow];
    responder = [keyWindow firstResponder];
    if (!responder) responder = keyWindow;

    if (useCache) {
	if (!validTypes) {
	    proto.hash = hashTypePair;
	    proto.isEqual = typePairsAreEqual;
	    proto.free = 0;
	    proto.style = 0;
	    validTypes = NXCreateHashTableFromZone(proto, 0, NULL, [Menu menuZone]);
	} else {
	    state = NXInitHashState(validTypes);
	    while (NXNextHashState(validTypes, &state, (void **)&typePair)) typePair->checked = NO;
	}
    }

    if (!sendTypes || !*sendTypes) {
	while (returnTypes && *returnTypes) {
	    requestor = checkForRequestor(NULL, *returnTypes, responder, useCache);
	    if (requestor) return requestor;
	    returnTypes++;
	}
    } else {
	while (*sendTypes) {
	    if (!returnTypes || !*returnTypes) {
		requestor = checkForRequestor(*sendTypes, NULL, responder, useCache);
		if (requestor) return requestor;
	    } else {
		while (returnTypes && *returnTypes) {
		    requestor = checkForRequestor(*sendTypes, *returnTypes, responder, useCache);
		    if (requestor) return requestor;
		    returnTypes++;
		}
	    }
	    sendTypes++;
	}
    }

    return nil;
}

#define PORT_BAD (port_t)-1

static int waitFlag = 0;

- (port_t)_getPortForApplication:(const char *)portName onHost:(const char *)host
{
    int ok, err;
    port_t sport;

    if (!host) host = "";
    if (strcmp(portName, NX_WORKSPACEREPLY) &&
	strcmp(portName, NX_WORKSPACEREQUEST) &&
	(netname_look_up(name_server_port,
			 (char *)host,
			 (char *)NX_WORKSPACEREQUEST,
			 &sport) == NETNAME_SUCCESS)) {
	[rmSpeaker setSendPort:sport];
	if (err = [rmSpeaker selectorRPC:"launchService:andWait:ok:" paramTypes:"ciI", portName, waitFlag, &ok]) {
	    NXLogError("appkit error: cannot send launch services to Workspace (err = %d).", err);
	    return PORT_BAD;
	}
    }

    return PORT_NULL;
}

- (port_t)_getPortForApplication:(const char *)portName onHost:(const char *)host wait:(BOOL)flag
{
    waitFlag = flag ? 1 : 0;
    return [self _getPortForApplication:portName onHost:host];
}

typedef struct {
    char *msgName;
    char *pbName;
    id pb;
    char *hostName;
    char *userData;
    char *portName;
    char *menuItem;
    port_t error;
    int unhide;
    int timeout;
} NXRequestArgs;

static void freeRequestArgs(NXRequestArgs *requestArgs)
{
    free(requestArgs->msgName);
    free(requestArgs->pbName);
    free(requestArgs->hostName);
    free(requestArgs->userData);
    free(requestArgs->portName);
    free(requestArgs->menuItem);
    free(requestArgs);
}

- _asyncRequest:(NXRequestArgs *)requestArgs
{
    int ec;
    port_t port = PORT_NULL;

    if (netname_look_up(name_server_port, requestArgs->hostName, requestArgs->portName, &port) != NETNAME_SUCCESS) {
	requestArgs->timeout -= 200;
	if (requestArgs->timeout > 0) {
	    [self perform:@selector(_asyncRequest:) with:(id)requestArgs afterDelay:200 cancelPrevious:NO];
	} else {
	    _NXKitAlert("ServicesMenu", "Services", "Error providing service %s.", NULL, NULL, NULL, requestArgs->menuItem);
	    freeRequestArgs(requestArgs);
	}
    } else {
	[rmSpeaker setSendPort:port];
	[rmSpeaker setSendTimeout:requestArgs->timeout];
	[rmSpeaker setReplyTimeout:requestArgs->timeout];
	ec = [rmSpeaker _request:requestArgs->msgName
			    pb:requestArgs->pbName
			    host:requestArgs->hostName
			    userData:requestArgs->userData
			    app:requestArgs->portName
			    menu:requestArgs->menuItem
			    error:requestArgs->error
			    unhide:requestArgs->unhide];
	if (ec) {
	    [requestArgs->pb freeGlobally];
	    _NXKitAlert("ServicesMenu", "Services", "Error providing service %s.", NULL, NULL, NULL, requestArgs->menuItem);
	}
	freeRequestArgs(requestArgs);
    }

    return self;
}

- _processRequest:sender
{
    char *error;
    NXRMEntry *entry;
    struct timezone tzp;
    struct timeval starttp, tp;
    const char *portName;
    int ec, timeout;
    const char *menuItem;
    port_t port = PORT_NULL;
    NXRequestArgs *requestArgs;
    id pb = nil, requestor = nil;
    BOOL async, couldntSend = YES;
    NXAtom *sendTypes = NULL, *returnTypes = NULL;
    char pbName[1024];

    if (!(menuItem = [[sender selectedCell] title])) return self;

    MARKTIME(servicesMenuTimings, "Getting valid requestor.", 1);

    gettimeofday(&starttp, &tzp);

    if (RM_ENTRY_AT([[sender selectedCell] tag], entry)) {
	sendTypes = getTypes(entry->sendTypes);
	returnTypes = getTypes(entry->returnTypes);
	requestor = getValidRequestor(sendTypes, returnTypes, NO);
    }

    MARKTIME(servicesMenuTimings, "Validating the valid requestor.", 1);

    if ([requestor respondsTo:@selector(writeSelectionToPasteboard:types:)]) {
	async = !returnTypes || ![requestor respondsTo:@selector(readSelectionFromPasteboard:)];
	MARKTIME(servicesMenuTimings, "Creating Speaker and Listener.", 1);
	if (!rmSpeaker) rmSpeaker = [[Speaker allocFromZone:[Menu menuZone]] init];
	if (!async && !rmListener) {
	    rmListener = [[Listener allocFromZone:[Menu menuZone]] init];
	    [rmListener usePrivatePort];
	    [rmListener addPort];
	}
	if (rmSpeaker && (rmListener || async)) {
	    MARKTIME(servicesMenuTimings, "Looking up the port.", 1);
	    portName = strcmp(RM_STRING(entry->portName), "Workspace") ? RM_STRING(entry->portName) : NX_WORKSPACEREQUEST;
	    if (netname_look_up(name_server_port, RM_STRING(entry->hostName), (char *)portName, &port) != NETNAME_SUCCESS) {
		timeout = entry->timeout ? entry->timeout : 30000;
		if (RM_STRING(entry->applicationPath)[0] == '/') {
		    MARKTIME(servicesMenuTimings, "Launching executable.", 1);
		    if (!_NXExecDetached(RM_STRING(entry->hostName), RM_STRING(entry->applicationPath))) {
			timeout = 0;
		    }
		} else {
		    MARKTIME(servicesMenuTimings, "Asking Workspace for the port.", 1);
		    port = [self _getPortForApplication:portName onHost:RM_STRING(entry->hostName) wait:!async];
		}
		if (!port && !async) {
		    MARKTIME(servicesMenuTimings, "Polling for the port.", 1);
		    while (timeout > 0 && netname_look_up(name_server_port, RM_STRING(entry->hostName), (char *)portName, &port) != NETNAME_SUCCESS) {
			usleep(200000);
			timeout -= 200;
		    }
		    if (timeout <= 0) port = PORT_NULL;
		}
	    }
	    if (port != PORT_BAD && (port || async)) {
		gettimeofday(&tp, &tzp);
		timeout = entry->timeout ? entry->timeout : 30000;
		timeout -= (tp.tv_sec - starttp.tv_sec) * 1000 + (tp.tv_usec - starttp.tv_usec) / 1000;
		if (timeout > 0) {
		    MARKTIME(servicesMenuTimings, "Setting up the Speaker.", 1);
		    if (port) {
			[rmSpeaker setSendPort:port];
			[rmSpeaker setSendTimeout:timeout];
			[rmSpeaker setReplyTimeout:timeout];
		    }
		    MARKTIME(servicesMenuTimings, "Creating the Pasteboard.", 1);
		    sprintf(pbName,"%d%d %s requesting %d",tp.tv_sec,tp.tv_usec,[NXApp appName],RM_STRING(entry->msgName));
		    pb = [Pasteboard newName:pbName];
		    if (pb && (!sendTypes || [requestor writeSelectionToPasteboard:pb types:sendTypes])) {
			MARKTIME(servicesMenuTimings, "Making the request.", 1);
			if (async) {
			    if (!(entry->flags & SERVICE_DONTDEACTIVATE)) [self deactivateSelf];
			    if (!port) {
				MARKTIME(servicesMenuTimings, "Queueing up an asynchronous request.", 2);
				NX_ZONEMALLOC([Menu menuZone], requestArgs, NXRequestArgs, 1);
				if (requestArgs) {
				    requestArgs->msgName = NXCopyStringBufferFromZone(RM_STRING(entry->msgName), [Menu menuZone]);
				    requestArgs->pbName = NXCopyStringBufferFromZone(pbName, [Menu menuZone]);
				    requestArgs->pb = pb;
				    requestArgs->hostName = NXCopyStringBufferFromZone((hostName ? hostName : ""), [Menu menuZone]);
				    requestArgs->userData = NXCopyStringBufferFromZone(RM_STRING(entry->userData), [Menu menuZone]);
				    requestArgs->portName = NXCopyStringBufferFromZone(RM_STRING(entry->portName), [Menu menuZone]);
				    requestArgs->menuItem = NXCopyStringBufferFromZone(menuItem, [Menu menuZone]);
				    requestArgs->error = [rmListener listenPort];
				    requestArgs->unhide = !(entry->flags & SERVICE_DONTACTIVATE);
				    requestArgs->timeout = timeout;
				    [self perform:@selector(_asyncRequest:) with:(id)requestArgs afterDelay:200 cancelPrevious:NO];
				    couldntSend = NO;
				}
			    } else {
				MARKTIME(servicesMenuTimings, "Making an asynchronous request.", 2);
				ec = [rmSpeaker _request:RM_STRING(entry->msgName)
						    pb:pbName
						    host:hostName ? hostName : ""
						    userData:RM_STRING(entry->userData)
						    app:RM_STRING(entry->portName)
						    menu:menuItem
						    error:[rmListener listenPort]
						    unhide:!(entry->flags & SERVICE_DONTACTIVATE)];
				couldntSend = ec ? YES : NO;
			    }
			    if (couldntSend) [pb freeGlobally];
			} else {
			    [pb _providePromisedData];
			    ec = [rmSpeaker _request:RM_STRING(entry->msgName)
						    pb:pbName
						host:hostName ? hostName : ""
					    userData:RM_STRING(entry->userData)
						error:&error];
			    if (!ec) {
				MARKTIME(servicesMenuTimings, "Giving data back to the requestor.", 1);
				[requestor readSelectionFromPasteboard:pb];
				if (error && *error) {
				    BOOL inOrder = YES;
				    const char *msg;
				    msg = KitString2(ServicesMenu, ">Service %s could not be provided: %s", &inOrder, "Use > if the first %s is the service, < if it is the error.");
				    NXRunAlertPanel(NULL,
					KitString(ServicesMenu, "Services", "Title of the alert the user sees if there was an error."),
					msg, NULL, NULL, NULL,
					inOrder ? menuItem : error, inOrder ? error : menuItem);
				}
				couldntSend = NO;
			    }
			    MARKTIME(servicesMenuTimings, "Freeing the Pasteboard.", 1);
			    [pb freeGlobally];
			}
		    } else {
			[pb freeGlobally];
		    }
		}
	    }
	}
    }

    DUMPTIMES(servicesMenuTimings);

    free(sendTypes);
    free(returnTypes);

    if (couldntSend) _NXKitAlert("ServicesMenu", "Services", "Error providing service %s.", NULL, NULL, NULL, menuItem);

    return self;
}

static BOOL matchTypes(const NXAtom *requesteeTypes, const NXAtom *requestorTypes)
{
    const NXAtom *eeTypes, *orTypes;

    if (!requesteeTypes) return YES;
    if (!requestorTypes) return NO;

    for (eeTypes = requesteeTypes; *eeTypes; eeTypes++) {
	for (orTypes = requestorTypes; *orTypes; orTypes++) {
	    if (*eeTypes == *orTypes) return YES;
	}
    }

    return NO;
}

static int findNewMatchingEntries(const NXAtom *sendTypes, const NXAtom *returnTypes, int startAt)
{
    NXAtom *types;
    NXRMEntry *entry;
    int headIndex, entryOffset;

    headIndex = startAt+1;

    if (headIndex < rmHeader->headCount && headIndex >= 0) {
	if (!activeMask) activeMask = (byte *)NXZoneCalloc([Menu menuZone], rmHeader->numEntries, sizeof(byte));
	for (; entryOffset = RM_HEAD(headIndex); headIndex++) {
	    if (!(activeMask[entryOffset] & INUSE)) {
		int firstEnabledOffset = entryOffset;
		while (!RM_ENTRY_AT(firstEnabledOffset, entry) && entry && (firstEnabledOffset = entry->next));
		if (entry && firstEnabledOffset) {
		    types = getTypes(entry->sendTypes);
		    if (matchTypes(types, sendTypes)) {
			free(types);
			types = getTypes(entry->returnTypes);
			if (matchTypes(types, returnTypes)) {
			    free(types);
			    activeMask[entryOffset] |= INUSE;
			    return headIndex;		/* found one */
			} else {
			    free(types);
			}
		    } else {
			free(types);
		    }
		}
	    }
	}
    }

    return -1;
}

static int cellcmp(const void *cell1, const void *cell2)
{
    const char *t1, *t2;

    t1 = [*((id *)cell1) title];
    t2 = [*((id *)cell2) title];

    return NXOrderStrings((const unsigned char *)t1, (const unsigned char *)t2, NO, -1, NULL);
}

static void sortMatrix(id matrix)
{
    int rows;
    id *cells;
    id cellList;

    cellList = [matrix cellList];
    if (cellList) {
	cells = NX_ADDRESS(cellList);
	[matrix getNumRows:&rows numCols:NULL];
	if (cells) qsort(cells, rows, sizeof(id), cellcmp);
    }
}

- _registerServicesMenuSendTypes:(const NXAtom *)sendTypes andReturnTypes:(const NXAtom *)returnTypes
/*
 * The user calls this repeatedly to let the system
 * know what types of servicess it can request if it
 * is the first responder and has a selection.
 * Must be called once for each distince pair of
 * types an object wants to register for.
 *
 * It adds a send/return type pair to the registeredTypes.
 * If the Services Menu exists, then it adds all appropriate entries
 * based on that type pair to the services menu.
 * Eliminates duplicate registrations.
 */
{
    NXRMEntry *entry;
    unsigned short key;
    NXTypesPair *typesPair;
    id cell, rmMatrix, menu, menusToUpdate;
    char *itemName, *slash = NULL, *buffer;
    int i, rmRows, dummy, entryOffset, entryIndirect;

    if ((!sendTypes || !*sendTypes) && (!returnTypes || !*returnTypes)) return nil;

    if (!registeredTypes) registeredTypes = [[List allocFromZone:[Menu menuZone]] init];
    typesPair = (NXTypesPair *)NXZoneMalloc([Menu menuZone], sizeof(NXTypesPair));
    typesPair->sendTypes = sendTypes;
    typesPair->returnTypes = returnTypes;
    [registeredTypes addObject:(id)typesPair];

    if (servicesMenu && (rmHeader || [[self class] _loadServicesMenuData])) {
	rmMatrix = [servicesMenu itemList];
	[rmMatrix getNumRows:&rmRows numCols:&dummy];
	entryIndirect = findNewMatchingEntries(sendTypes, returnTypes, -1);
	[servicesMenu disableDisplay];
	menusToUpdate = [List new];
	[menusToUpdate addObject:servicesMenu];
	while (entryIndirect >= 0) {
	    entryOffset = RM_HEAD(entryIndirect);
	    while (entryOffset) {
		if (RM_ENTRY_AT(entryOffset, entry) && [self _isValidRequestPort:RM_STRING(entry->portName)]) {
		    itemName = RM_STRING(getLocalizedIndexOf(entry->menuEntries));
		    if (itemName) {
			if ((slash = strchr(itemName, '/')) && slash != itemName && *(slash-1) != '\\' && *(slash+1)) {
			    buffer = (char *)NXZoneMalloc([Menu menuZone], slash-itemName+1);
			    strncpy(buffer, itemName, slash-itemName);
			    buffer[slash-itemName] = '\0';
			    itemName = slash+1;
			    cell = findMenuItem(rmMatrix, rmRows, buffer);
			} else {
			    slash = buffer = NULL;
			    cell = findMenuItem(rmMatrix, rmRows, itemName);
			}
			if (slash) {
			    if (!cell) {
				menu = [[Menu allocFromZone:[Menu menuZone]] initTitle:buffer];
				cell = [servicesMenu addItem:buffer action:NULL keyEquivalent:0];
				[servicesMenu setSubmenu:menu forItem:cell];
				rmRows++;
				cell = nil;
			    } else if ([cell hasSubmenu]) {
				menu = [cell target];
				cell = findMenuItem([menu itemList], -1, itemName);
			    } else {
				itemName = RM_STRING(getLocalizedIndexOf(entry->menuEntries));
				menu = servicesMenu;
				cell = findMenuItem(rmMatrix, rmRows, itemName);
			    }
			} else {
			    menu = servicesMenu;
			}
			key = getLocalizedIndexOf(entry->keyEquivalents);
			if (!cell) {
			    if ([menusToUpdate indexOf:menu] == NX_NOT_IN_LIST) {
				[menu disableDisplay];
				[menusToUpdate addObject:menu];
			    }
			    cell = [menu addItem:NULL action:@selector(_processRequest:) keyEquivalent:key];
			    [cell setTitleNoCopy:itemName];
			    if (menu == servicesMenu) rmRows++;
			} else {
			    [[cell setKeyEquivalent:key] setKeyEquivalent:[cell userKeyEquivalent]];
			}
			[[[[cell setTag:entryOffset] setAction:@selector(_processRequest:)] setTarget:self] setEnabled:NO];
			if (buffer) free(buffer);
		    }
		}
		entryOffset = entry ? entry->next : 0;
	    }
	    entryIndirect = findNewMatchingEntries(sendTypes, returnTypes, entryIndirect);
	}
	for (i = [menusToUpdate count]-1; i >= 0; i--) {
	    menu = [menusToUpdate objectAt:i];
	    sortMatrix([menu itemList]);
	    [menu reenableDisplay];
	    [menu display];
	}
	[menusToUpdate free];
    }

    return self;
}

- registerServicesMenuSendTypes:(const char **)sendTypes andReturnTypes:(const char **)returnTypes
{
    if ((!sendTypes || !*sendTypes) && (!returnTypes || !*returnTypes)) return nil;
    return [self _registerServicesMenuSendTypes:newTypeList(sendTypes) andReturnTypes:newTypeList(returnTypes)];
}

- _initServicesMenu:aMenu
{
    int i;
    NXTypesPair *typesPair;

    if (aMenu) {
	servicesMenu = aMenu;
	for (i = [registeredTypes count]-1; i >= 0; i--) {
	    typesPair = (NXTypesPair *)[registeredTypes objectAt:i];
	    [self _registerServicesMenuSendTypes:typesPair->sendTypes andReturnTypes:typesPair->returnTypes];
	}
    }

    return self;
}

- _initServicesMenu
{
    return [self setServicesMenu:[findMenuItem([_mainMenu itemList], -1, "Request") target]];
}

- _freeServicesMenu
{
    [servicesMenu _removeFromHierarchy];
    [servicesMenu free]; servicesMenu = nil;
    [rmSpeaker free]; rmSpeaker = nil;
    [rmListener free]; rmListener = nil;
    if (rmData && rmDataLength) {
	vm_deallocate(task_self(), (vm_address_t)rmData, rmDataLength);
	rmData = NULL; rmDataLength = 0;
    }
    free(enabledMask); free(activeMask);
    enabledMask = NULL; activeMask = NULL;
    noServicesMenu = NO;
    if (validTypes) NXFreeHashTable(validTypes); validTypes = NULL;
    return self;
}

static BOOL checkVisible(id matrix)
{
    id cell;
    int i, rows;

    if (matrix) {
	[matrix getNumRows:&rows numCols:NULL];
	for (i = 0; i < rows; i++) {
	    cell = [matrix cellAt:i :0];
	    if ([cell hasSubmenu]) {
		if ([[cell target] isVisible]) {
		    return YES;
		} else {
		    if (checkVisible([[cell target] itemList])) return YES;
		}
	    }
	}
    }

    return NO;
}

- (BOOL)_servicesMenuIsVisible
{
    if (servicesMenu) {
	if ([servicesMenu isVisible]) {
	    return YES;
	} else {
	    return checkVisible([servicesMenu itemList]);
	}
    } else {
	return NO;
    }
}

- setServicesMenu:aMenu
{
    if ([aMenu isKindOf:[MenuCell class]]) aMenu = [aMenu target];

    if (servicesMenu != aMenu) {
	pendingServicesMenu = aMenu;
	[pendingServicesMenu _makeServicesMenu];
	[self _freeServicesMenu];
	servicesMenuTimings = NXGetDefaultValue(_NXAppKitDomainName, "ServicesMenuTimings") ? YES : NO;
    }

    return self;
}

- setRequestMenu:aMenu { return [self setServicesMenu:aMenu]; }
- requestMenu { return [self servicesMenu]; }
- updateRequestMenu:sender { return [self _doUpdateServicesMenu:sender]; }
- registerRequestMenuSendTypes:(const char *const *)sendTypes andReturnTypes:(const char *const *)returnTypes
{
    return [self registerServicesMenuSendTypes:sendTypes andReturnTypes:returnTypes];
}
- registerRequestMenuTypes:(const char *)sendType :(const char *)returnType
{
    const char *sendTypes[2];
    const char *returnTypes[2];

    sendTypes[0] = sendType;
    sendTypes[1] = NULL;
    returnTypes[0] = returnType;
    returnTypes[1] = NULL;

    return [self registerServicesMenuSendTypes:sendTypes andReturnTypes:returnTypes];
}


static BOOL hasKeyEquivalents(id matrix)
{
    int i;
    id cellList;

    if ([NXApp currentEvent]->type != NX_KEYDOWN) return YES;
    [matrix getNumRows:&i numCols:NULL];
    cellList = [matrix cellList];
    for (; i >= 0; i--) {
	if ([[cellList objectAt:i] keyEquivalent]) return YES;
    }

    return NO;
}

- _updateServicesMenu:menu
{
    NXRMEntry *entry;
    NXRect cellFrame;
    id cell, matrix, cellList;
    int i, row, col, entryOffset;
    NXAtom *sendTypes, *returnTypes;
    BOOL lockedFocus = NO, enabled = NO, canDraw;

    if (rmHeader && (matrix = [menu itemList])) {
	cellList = [matrix cellList];
	canDraw = [matrix canDraw];
	if (!canDraw || [menu isVisible] || hasKeyEquivalents(matrix)) {
	    [matrix getNumRows:&i numCols:NULL];
	    for (; i >= 0; i--) {
		cell = [cellList objectAt:i];
		entryOffset = [cell tag];
		if (RM_ENTRY_AT(entryOffset, entry)) {
		    if (!(enabledMask[entry->head] & HASBEENCHECKED)) {
			entryOffset = RM_HEAD(entry->head);
			RM_ENTRY_AT(entryOffset, entry);
			if (entry) {
			    sendTypes = getTypes(entry->sendTypes);
			    returnTypes = getTypes(entry->returnTypes);
			    if (getValidRequestor(sendTypes, returnTypes, YES)) enabledMask[entry->head] |= ISACTIVE;
			    free(returnTypes);
			    free(sendTypes);
			    enabledMask[entry->head] |= HASBEENCHECKED;
			}
		    }
		    enabled = enabledMask[entry->head] & ISACTIVE;
		    if (cell && [cell isEnabled] != enabled) {
			[menu disableDisplay];
			[cell setEnabled:enabled];
			[menu reenableDisplay];
			[matrix getRow:&row andCol:&col ofCell:cell];
			[matrix getCellFrame:&cellFrame at:row :col];
			if (!lockedFocus && canDraw) {
			    [matrix lockFocus];
			    lockedFocus = YES;
			}
			if (lockedFocus) [cell drawInside:&cellFrame inView:matrix];
		    }
		}
	    }
	}
	if (lockedFocus) {
	    [menu flushWindow];
	    [matrix unlockFocus];
	}
	[matrix getNumRows:&row numCols:&col];
	for (i = 0; i < row; i++) {
	    cell = [matrix cellAt:i:0];
	    if ([cell hasSubmenu]) [self _updateServicesMenu:[cell target]];
	}
    }

    return self;
}

- _doUpdateServicesMenu:sender
{
    if (!servicesMenu && pendingServicesMenu) {
	[self _initServicesMenu:pendingServicesMenu];
	pendingServicesMenu = nil;
    }
    if (rmHeader) {
	if (!enabledMask) {
	    enabledMask = (byte *)NXZoneCalloc([Menu menuZone], rmHeader->headCount, sizeof(byte));
	} else {
	    bzero(enabledMask, rmHeader->headCount * sizeof(byte));
	}
	return [self _updateServicesMenu:servicesMenu];
    } else {
	return self;
    }
}

- _removeServicesMenuPort:(const char *)name
{
    id matrix;
    NXRMEntry *entry;
    int i = 0, rows, cols;
    BOOL needsSizeToFit = NO;

    if (!name || !*name) return self;
    if (!strcmp(name, NX_WORKSPACEREQUEST)) name = "Workspace";

    if (!invalidRequestPorts) {
	invalidRequestPortCount = 1;
	invalidRequestPorts = (char **)NXZoneMalloc([Menu menuZone], sizeof(char *));
    } else {
	for (i = 0; i < invalidRequestPortCount; i++) {
	    if (!strcmp(invalidRequestPorts[i], name)) return self;
	}
	invalidRequestPortCount++;
	invalidRequestPorts = (char **)NXZoneRealloc([Menu menuZone], invalidRequestPorts, sizeof(char *)*invalidRequestPortCount);
    }
    invalidRequestPorts[i] = NXCopyStringBufferFromZone(name, [Menu menuZone]);
    matrix = [servicesMenu itemList];
    if (matrix) {
	[matrix getNumRows:&rows numCols:&cols];
	for (i = rows-1; i >= 0; i--) {
	    if (RM_ENTRY_AT([[matrix cellAt:i :0] tag], entry) && !strcmp(RM_STRING(entry->portName), name)) {
		[matrix removeRowAt:i andFree:YES];
		needsSizeToFit = YES;
	    }
	}
	if (needsSizeToFit) [servicesMenu sizeToFit];
    }

    return self;
}

@end

/*
	pbtypes.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson

	Declarations shared between the appkit and pbs.
*/

#ifndef PBSYPES_H
#define PBSYPES_H 1

#include <sys/port.h>
#include <sys/kern_return.h>

/* rendez-vous names for nmserver */
#define PBS_NAME  "NeXT Pasteboard Server"

/* rendez-vous name for Workspace Manager */
#define NX_WSMSERVICES "WorkspaceServices"

/* errors from the server.  Keep errors.m in sync!! */
#define PBS_OK		(-1)
#define ILLEGAL_TYPE	(-2)
#define TYPE_NOT_FOUND	(-3)
#define DATA_DELAYED	(-4)
#define PBS_OOSYNC	(-5)
#define AFM_OK		(-6)
#define AFM_NOT_FOUND	(-7)
#define FONTDIR_OK	(-8)
#define FONTDIR_BAD	(-9)
#define FONTDIR_STALE	(-10)
#define FONTDIR_ROTTEN	(-11)

/* timeout for fulfilling promised data */
#define TIMEOUT 15

/*
 *  These types are unfortunately duplicated in pbs.defs and app.defs.
 */
#define MAX_TYPE_LENGTH		256
typedef char *data_t;
typedef char *deallocData_t;
typedef char pbtype_t[MAX_TYPE_LENGTH];

extern void _NXMsgError(kern_return_t errorCode);
extern void MsgError(kern_return_t errorCode);

/* Services Menu entry structure */

/*
 * The string table is a bunch of concatenated NULL-terminated strings.
 * The indirect string table is a bunch of 0 terminated lists of integers
 * which are interpreted as indexes into the string table.
 * Index 0 into the string table is always the empty string.
 */

#define SERVICES_MENU_VERSION 1
#define SERVICES_MENU_DEFAULT_LANGUAGE "default"

typedef struct {
    int msgName;		/* offset into string table (cannot be 0) */
    int portName;		/* offset into string table (cannot be 0) */
    int applicationPath;	/* offset into string table */
    int userData;		/* offset into string table */
    int hostName;		/* offset into string table - questionable attribute (pah) */
    short sendTypes;		/* offset into indirect string table (0 means no send types) */
    short returnTypes;		/* offset into indirect string table (0 means asynchronous request) */
    short menuEntries;		/* offset into indirect string table (language/menu entry pairs - cannot be 0) */
    short keyEquivalents;	/* offset into indirect string table (language/key equivalent pairs) */
    int timeout;		/* ms to wait before timing out */
    short isHead;		/* start of a list of items with same send/return types */
    short next;			/* index of next item with same send/return types */
    short head;			/* pointer to the head of the list (if !isHead) */
    short flags;		/* status flags */
} NXRMEntry;

/* definitions for the status flags in NXRMEntry */

#define SERVICE_DISABLED (1)
#define SERVICE_DONTACTIVATE (2)
#define SERVICE_DONTDEACTIVATE (4)

typedef struct {
    int version;
    int numEntries;
    int entrySize;
    int entries;
    int stringTableLength;
    int stringTable;
    int indirectStringTableLength;
    int indirectStringTable;
    int headCount;
    int heads;
} NXRMHeader;

#endif

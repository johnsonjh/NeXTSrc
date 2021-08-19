/*
	errors.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  
	Contains default appkit error handler and reporter.
  
	Each time you run across an error that you want to generate, add
	it to the following list and the one in errors.h.
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application.h"
#import "errors.h"
#import "nextstd.h"
#import <stdio.h>
#import <sys/syslog.h>
#import <sys/file.h>


typedef struct {		/* a range of errors and a proc to call */
    int                 min;
    int                 max;
    void                (*proc) ();
}                   NXErrorRange;

static NXErrorRange *Ranges = NULL;
static int          NumRanges = 0;

/* INDENT OFF */
/* dps errors */
static const char * const dpsAdobeMessages[] = {
	/* dps_err_ps */		"PostScript program error",
	/* dps_err_nameTooLong */	"PostScript name too long",
	/* dps_err_resultTagCheck */	"Invalid tag in returned object",
	/* dps_err_resultTypeCheck */	"Invalid tag of returned object",
	/* dps_err_invalidContext */	"Invalid DPSContext"
};

static const char * const dpsNextMessages[] = {
	/* dps_err_select */		"Error while selecting",
	/* dps_err_connectionClosed */	"Connection closed unexpectedly",
	/* dps_err_read */		"Error while reading connection",
	/* dps_err_write */		"Error while writing to connection",
	/* dps_err_invalidFD */		"Invalid file descriptor",
	/* dps_err_invalidTE */		"Invalid DPSTimedEntry",
	/* dps_err_invalidPort */	"Invalid port",
	/* dps_err_outOfMemory */	"Not enough memory",
	/* dps_err_cantConnect */	"Could not form connection"
};

/* kit errors */
static const char * const appkitMessages[] = {
	/* NX_longLine */	"Text - Line longer than 16384 characters",
	/* NX_nullSel */	"Text - Empty selection",
	/* NX_wordTablesWrite */ "Error while writing word tables to stream",
	/* NX_wordTablesRead */	 "Error while reading word tables to stream",
	/* NX_textBadRead */	"Text - Error reading file",
	/* NX_textBadWrite */	"Text - Error writing file",
	/* NX_powerOff */	NULL,
	/* NX_pasteboardComm */	NULL,
	/* NX_mallocError */	NULL,
	/* NX_printingComm */	NULL,
	/* NX_abortModal */	"-abortModal called while not running modal",
	/* NX_abortPrinting */	"Unexpected NX_abortPrinting exception",
	/* NX_illegalSelector */NULL,
	/* NX_appkitVMError */	NULL,
    	/* NX_badRtfDirective */NULL,
	/* NX_badRtfFontTable */NULL,
	/* NX_badRtfStyleSheet*/NULL,
	/* NX_newerTypedStream*/"Version of file more recent than software.",
	/* NX_tiffError*/	NULL,
	/* NX_printPackageError */ NULL,
	/* NX_badRtfColorTable */ NULL,
	/* NX_journalAborted */ NULL
};

/* streams errors */
static const char * const streamsMessages[] = {
	/* NX_illegalWrite */	"Write error",
	/* NX_illegalRead */	"Read error",
	/* NX_illegalSeek */	"Seek error",
	/* NX_illegalStream */	"Invalid stream",
	/* NX_streamVMError */	NULL
};

/* pasteboard sub-errors */
static const char * const pbMsgs[] = {
	/* filler */		NULL, NULL,	
	/* ILLEGAL_TYPE */	"Illegal type string",
	/* TYPE_NOT_FOUND */	"Unrecognized data type",
	/* DATA_DELAYED */	"Pasteboard data will be delayed",
	/* PBS_OOSYNC */	"Pasteboard communications are out of sync"
};
/* INDENT ON */

#include "logErrorInc.c"

/* Default error procedure.  This is what all the appkit top-level handlers
   call (in the second block of a NX_DURING/NX_HANDLER construct) via a
   global pointer, which can be overridden by the app.  This routine is
   stuck in that pointer by default.
   
   By default this routine exits when there is an error upon context creation,
   or if the NXApp's PS connection goes away.  Apps wishing to override this
   can use DURING/HANDLER constructs, or override this routine.
 */
void NXDefaultTopLevelErrorHandler(errorState)
    NXHandler          *errorState;
{
    if (errorState->next)
	NX_RAISE(errorState->code, errorState->data1, errorState->data2);
    else {
	NXReportError(errorState);
	if (errorState->code == dps_err_cantConnect)
	    exit(-1);
	else if ((errorState->code == dps_err_connectionClosed ||
			errorState->code == dps_err_write) &&
		 (DPSContext)(errorState->data1) == [NXApp context]) {
	    NXLogError("Exiting due to Window Server death\n");
	    exit(-1);
	}
    }
}


/* Reports the error passed to it.  It loops through the ranges that have
   been registered with it and calls the approriate proc.  This routine
   will always return.
 */
void NXReportError(errorState)
    NXHandler          *errorState;
{
    int                 i;
    NXErrorRange       *r;

    for (i = 0, r = Ranges; i < NumRanges; i++, r++)
	if (errorState->code >= r->min && errorState->code <= r->max) {
	    (*r->proc) (errorState);
	    return;
	}
    NXLogError("Unknown error code %d in NXReportError", errorState->code);
}


/* Registers a procedure to be the reporter for a given range of errors */
void NXRegisterErrorReporter(min, max, proc)
    int                 min;
    int                 max;
    void                (*proc) ();

{
    Ranges = NXZoneRealloc(NXDefaultMallocZone(), Ranges, sizeof(NXErrorRange) * (NumRanges + 1));
    Ranges[NumRanges].min = min;
    Ranges[NumRanges].max = max;
    Ranges[NumRanges].proc = proc;
    NumRanges++;
}


/* Removes an error reporter from the list that covered the given code */
void NXRemoveErrorReporter(code)
    int                 code;
{
    int                 i;
    NXErrorRange       *r;

    for (i = 0, r = Ranges; i < NumRanges; i++, r++)
	if (code >= r->min && code <= r->max) {
	    NumRanges--;
	    bcopy(r + 1, r, (NumRanges - i) * sizeof(NXErrorRange));
	    return;
	}
    NXLogError("Unknown error code %d in NXRemoveErrorReporter", code);
}


/* The reporter proc registered to report appkit errors */
void _NXAppkitReporter(NXHandler *errorState)
{
    switch (errorState->code) {
    case NX_powerOff:
	return;
    case NX_pasteboardComm:
	if (errorState->data1)
	    NXLogError("App Kit: Pasteboard Mach messaging error #%d, in %s\n", errorState->data1, errorState->data2);
	else
	    NXLogError("App Kit: Pasteboard error: %s\n", pbMsgs[-(int)(errorState->data2)]);
	break;
    case NX_mallocError:
	NXLogError("App Kit: Malloc error (see the malloc man page). code = %d\n", errorState->data1);
	break;
    case NX_printingComm:
	NXLogError("App Kit: Error messaging to npd. return code = %d\n", errorState->data1);
	break;
    case NX_illegalSelector:
	NXLogError("App Kit: Illegal selector %d received in [%s]\n", errorState->data1, errorState->data2);
	break;
    case NX_appkitVMError:
	NXLogError("App Kit: VM error (%s) error %d\n", errorState->data2, errorState->data1);
	break;
    case NX_tiffError:
	NXLogError("App Kit: TIFF error %d\n", errorState->data1);
	break;

    default:
	NXLogError("App Kit error: %s\n", appkitMessages[errorState->code - NX_APPKITERRBASE]);
    }
}


/* The reporter proc registered to report DPS errors */
void _NXDpsReporter(NXHandler *errorState)
{
    const char         *msg;
    NXStream	       *st;
    char	       *errBuf;
    int			errLen;
    int			errMaxLen;

    if (errorState->code < DPS_NEXTERRORBASE)
	msg = dpsAdobeMessages[errorState->code - DPS_ERRORBASE];
    else
	msg = dpsNextMessages[errorState->code - DPS_NEXTERRORBASE];

    if (errorState->code == dps_err_cantConnect && errorState->data2)
	NXLogError("DPS client library error: %s, host %s",
					msg, errorState->data2);
    else if (errorState->code == dps_err_ps) {
	NXLogError("DPS client library error: %s, DPSContext %x",
						msg, (int)(errorState->data1));
	if (st = NXOpenMemory(NULL, 0, NX_WRITEONLY)) {
	    DPSPrintErrorToStream(st, (DPSBinObjSeq)(errorState->data2));
	    NXPutc(st, '\0');
	    NXGetMemoryBuffer(st, &errBuf, &errLen, &errMaxLen);
	    NXLogError("%s", errBuf);
	    NXCloseMemory(st, NX_FREEBUFFER);
	}
    } else
	NXLogError("DPS client library error: %s, DPSContext %x, data %d",
		msg, (int)(errorState->data1), (int)(errorState->data2));
}


/* The reporter proc registered to report streams errors */
void _NXStreamsReporter(NXHandler *errorState)
{
    NXLogError("Streams library error: %s, stream = %x, data = %d",
			streamsMessages[errorState->code - NX_STREAMERRBASE],
			errorState->data1, errorState->data2);
}


/* The reporter proc registered to report streams errors */
void _NXTypedStreamsReporter(NXHandler *errorState)
{
    if (errorState->data1)
	NXLogError("Typed streams library error: %s\n", errorState->data1);
    else
	NXLogError("Typed streams library error: %d\n", errorState->code);
}


#ifdef DEBUG
void _NXCheckSelector(SEL aSelector, const char *classAndMethod)
{
    if (aSelector && !ISSELECTOR(aSelector))
	NX_RAISE(NX_illegalSelector, (void *)aSelector, classAndMethod);
}
#endif

/*
  
Modifications (starting at 0.8):
  
12/12/88 trey	added NXRemoveErrorReporter
01/03/89 trey	added NX_mallocError and NX_pasteboardComm error codes
 3/12/89 trey	fixed error table offset probs for dps errors
 6/21/89 trey	added NX_abortPrinting error code
 6/24/89 trey	added NX_illegalSelector error code
		added _NXCheckSelector
		added NXLogError, converted kit to use it
 7/10/89 trey	dps_err_write dps_err_read errors cause exit
		NX_readOnly and NX_badPboard removed
 7/11/89 trey	typed streams error reported added
 7/20/89 trey	better error reporting for dps errors
		dps_err_read doesnt cause exit, since its not clear
		 what port the error refers to

12/21/89 trey	errors for word table reading and writing added

79
--
 3/29/90 aozer	Added error NX_newerTypedStream

80
--
 4/09/90 trey	made _NXLogError public

83
--
 4/30/90 aozer	Added error NX_tiffError, used by old and new TIFF stuff

86
--
 6/2/90 trey	added NX_printPackageError

95
--
 10/1/90 trey	reports transaction causing pbs error

*/

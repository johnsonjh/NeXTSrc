/*
	errors.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

/* Contains error numbers for the appkit. */

#ifndef APPKIT_ERROR_H
#define APPKIT_ERROR_H
#ifndef ERROR_H
#import <objc/error.h>
#endif ERROR_H

#define NX_APPKITERRBASE 3000	/* Kit errors start here.  We get 1000 */

#define NX_APPBASE 10000000	/* User defined errors start here */

typedef enum _NXAppkitErrorTokens {
    NX_longLine	= NX_APPKITERRBASE,  /* Text, line longer than 16384 chars */
    NX_nullSel,		/* Text, operation attempted on empty selection */
    NX_wordTablesWrite,		/* error while writing word tables */
    NX_wordTablesRead,		/* error while reading word tables */
    NX_textBadRead,		/* Text, error reading from file */
    NX_textBadWrite,		/* Text, error writing to file */
    NX_powerOff,		/* poweroff */
    NX_pasteboardComm,		/* communications prob with pbs server */
    NX_mallocError,		/* malloc problem */
    NX_printingComm,		/* sending to npd problem */
    NX_abortModal,		/* used to abort modal panels */
    NX_abortPrinting,		/* used to abort printing */
    NX_illegalSelector,		/* bogus selector passed to appkit */
    NX_appkitVMError,		/* error from vm_ call */
    NX_badRtfDirective,
    NX_badRtfFontTable,
    NX_badRtfStyleSheet,
    NX_newerTypedStream,
    NX_tiffError,
    NX_printPackageError,	/* problem loading the print package */
    NX_badRtfColorTable,
    NX_journalAborted
} NXAppkitErrorTokens;

/*
 * The proc called when raised exceptions bubble up to the [Application run]
 * level.
 */
typedef void NXTopLevelErrorHandler(NXHandler *errorState);
extern NXTopLevelErrorHandler *_NXTopLevelErrorHandler;
#define NXSetTopLevelErrorHandler( proc ) (_NXTopLevelErrorHandler = (proc))
#define NXTopLevelErrorHandler() (_NXTopLevelErrorHandler)
extern NXTopLevelErrorHandler NXDefaultTopLevelErrorHandler;

/*
 * Mechanism for reporting information on errors.
 */
typedef void NXErrorReporter(NXHandler *errorState);
extern void NXReportError(NXHandler *errorState); 
extern void NXRegisterErrorReporter(int min, int max, NXErrorReporter *proc); 
extern void NXRemoveErrorReporter(int code);

#endif APPKIT_ERROR_H

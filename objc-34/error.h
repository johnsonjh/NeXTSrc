
/*
    error.h

    This file defines the interface to the exception raising scheme.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#ifndef _ERROR_H
#define _ERROR_H

#include <setjmp.h>

typedef struct _NXHandler {	/* a node in the handler chain */
    jmp_buf jumpState;			/* place to longjmp to */
    struct _NXHandler *next;		/* ptr to next handler */
    int code;				/* error code of exception */
    const void *data1, *data2;		/* blind data for describing error */
} NXHandler;


/* Handles RAISE's with nowhere to longjmp to */
typedef void NXUncaughtExceptionHandler(int code, const void *data1,
						const void *data2);
extern NXUncaughtExceptionHandler *_NXUncaughtExceptionHandler;
#define NXGetUncaughtExceptionHandler() _NXUncaughtExceptionHandler
#define NXSetUncaughtExceptionHandler(proc) \
			(_NXUncaughtExceptionHandler = (proc))

/* NX_DURING, NX_HANDLER and NX_ENDHANDLER are always used like:

	NX_DURING
	    some code which might raise an error
	NX_HANDLER
	    code that will be jumped to if an error occurs
	NX_ENDHANDLER

   If any error is raised within the first block of code, the second block
   of code will be jumped to.  Typically, this code will clean up any
   resources allocated in the routine, possibly case on the error code
   and perform special processing, and default to RERAISE the error to
   the next handler.  Within the scope of the handler, a local variable
   called NXLocalHandler of type NXHandler holds information about the
   error raised.

   It is illegal to exist the first block of code by any other means than
   NX_VALRETURN, NX_VOIDRETURN, or just falling out the bottom.
 */

/* private support routines.  Do not call directly. */
extern void _NXAddHandler( NXHandler *handler );
extern void _NXRemoveHandler( NXHandler *handler );
extern _setjmp(jmp_buf env);

#define NX_DURING { NXHandler NXLocalHandler;			\
		    _NXAddHandler(&NXLocalHandler);		\
		    if( !_setjmp(NXLocalHandler.jumpState) ) {

#define NX_HANDLER _NXRemoveHandler(&NXLocalHandler); } else {

#define NX_ENDHANDLER }}

#define NX_VALRETURN(val)  { _NXRemoveHandler(&NXLocalHandler); \
			return(val); }

#define NX_VOIDRETURN	{ _NXRemoveHandler(&NXLocalHandler); \
			return; }

/* RAISE and RERAISE are called to indicate an error condition.  They
   initiate the process of jumping up the chain of handlers.
 */

#ifdef __GNUC__
#ifndef __STRICT_ANSI__
__volatile	/* never returns */
#endif /* not __STRICT_ANSI__ */
#endif /* __GNUC__ */
extern void _NXRaiseError(int code, const void *data1, const void *data2);

#define NX_RAISE( code, data1, data2 )	\
		_NXRaiseError( (code), (data1), (data2) )

#define NX_RERAISE() 	_NXRaiseError( NXLocalHandler.code,	\
				NXLocalHandler.data1, NXLocalHandler.data2 )

/* These routines set and return the procedure which is called when
   exceptions are raised.  This procedure must NEVER return.  It will
   usually either longjmp, or call the uncaught exception handler.
   The default exception raiser is also declared
 */
typedef void NXExceptionRaiser(int code, const void *data1, const void *data2);
extern void NXSetExceptionRaiser(NXExceptionRaiser *proc);
extern NXExceptionRaiser *NXGetExceptionRaiser(void);
extern NXExceptionRaiser NXDefaultExceptionRaiser;


/* The error buffer is used to allocate data which is passed up to other
   handlers.  Clients should clear the error buffer in their top level
   handler.  The Application Kit does this.
 */
extern void NXAllocErrorData(int size, void **data);
extern void NXResetErrorData(void);

#endif /* _ERROR_H */


#ifdef SHLIB
#include "shlib.h"
#endif
/*
    except.c

    This file implements the exception raising scheme.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#import "error.h"
#import <stdlib.h>
#import <stdio.h>
#import <sys/syslog.h>

extern _setjmp(jmp_buf env);
extern _longjmp(jmp_buf env, int val);

#define TRUE	1
#define FALSE	0

typedef void AltProc(void *context, int code, const void *data1, const void *data2);

/*  These nodes represent handlers that are called as normal procedures
    instead of longjmp'ed to.  When these procs return, the next handler
    in the chain is processed.
 */
typedef struct {		/* an alternative node in the handler chain */
    struct _NXHandler *next;		/* ptr to next handler */
    AltProc *proc;			/* proc to call */
    void *context;			/* blind data for client */
} AltHandler;

static int ErrorBufferSize = 0;
static NXHandler *HandlerStack = NULL;	/* global start of handler chain */
static NXExceptionRaiser *ExceptionRaiser = &NXDefaultExceptionRaiser;
static AltHandler *altHandlers = NULL;
static int altHandlersAlloced = 0;
static int altHandlersUsed = 0;

#define IS_ALT(ptr)			((int)(ptr) % 2)
#define NEXT_HANDLER(ptr)		\
	(IS_ALT(ptr) ? ALT_CODE_TO_PTR(ptr)->next : ((NXHandler *)ptr)->next)
#define ALT_PTR_TO_CODE(ptr)		(((ptr) - altHandlers) * 2 + 1)
#define ALT_CODE_TO_PTR(code)		(altHandlers + ((int)(code) - 1) / 2)


static void trickyRemoveHandler(void *handler, int removingAlt)
{
    AltHandler *altNode;
    NXHandler *node;
    NXHandler **nodePtr;

  /* if the node passed in isnt on top of the stack, something's fishy */
    if ((NXHandler *)handler != HandlerStack) {
      /* try to find the node anywhere on the stack */
	node = HandlerStack;
	while (node != (NXHandler *)handler && node)
	    node = NEXT_HANDLER(node);
	if (node) {
	  /* 
	   * Clean off the stack up to the out of place node.  If we are trying
	   * to remove an non-alt handler, we pop eveything off the stack
	   * including that handler, calling any alt procs along the way.  If
	   * we are removing an alt handler, we leave all the non-alt handlers
	   * alone as we clean off the stack, but pop off all the alt handlers
	   * we find, including the node we were asked to remove.
	   */
	    syslog(LOG_ERR, "Exception handlers were not properly removed.");
	    nodePtr = &HandlerStack;
	    do {
		node = *nodePtr;
		if (IS_ALT(node)) {
		    altNode = ALT_CODE_TO_PTR(node);
		    if (removingAlt) {
			if (node == (NXHandler *)handler)
			    *nodePtr = altNode->next;	/* del matching node */
			else
			    nodePtr = &altNode->next;	/* skip node */
		    } else {
			if (node != (NXHandler *)handler)
			    (*altNode->proc)(altNode->context, 1, 0, 0);
			altHandlersUsed = altNode - altHandlers;
			*nodePtr = altNode->next;	/* del any alt node */
		    }
		} else {
		    if (removingAlt)
			nodePtr = &node->next;		/* skip node */
		    else
			*nodePtr = node->next;		/* nuke non-alt node */
		}
	    } while (node != (NXHandler *)handler);
	} else
	    syslog(LOG_ERR, "Attempt to remove unrecognized exception handler.");
    } else {
	if (removingAlt) {
	    altNode = ALT_CODE_TO_PTR(handler);
	    altHandlersUsed = altNode - altHandlers;
	    HandlerStack = altNode->next;
	} else
	    HandlerStack = HandlerStack->next;
    }
}


NXHandler *_NXAddAltHandler(AltProc *proc, void *context)
{
    AltHandler *new;

    if (altHandlersUsed == altHandlersAlloced)
	altHandlers = realloc(altHandlers, ++altHandlersAlloced * sizeof(AltHandler));
    new = altHandlers + altHandlersUsed++;
    new->next = HandlerStack;
    HandlerStack = (NXHandler *)ALT_PTR_TO_CODE(new);
    new->proc = proc;
    new->context = context;
    return HandlerStack;
}


void _NXRemoveAltHandler(NXHandler *node)
{
    trickyRemoveHandler(node, TRUE);
}


void _NXAddHandler(NXHandler *handler)
{
    handler->next = HandlerStack;
    HandlerStack = handler;
    handler->code = 0;
}


void _NXRemoveHandler(NXHandler *handler)
{
    trickyRemoveHandler(handler, FALSE);
}


void NXSetExceptionRaiser(void (*proc)(int code, const void *data1,
					const void *data2))
{
    ExceptionRaiser = proc;
}


extern void (*NXGetExceptionRaiser(void))(int code, const void *data1,
						const void *data2)
{
    return ExceptionRaiser;
}

#ifdef __GNUC__
#ifndef __STRICT_ANSI__
__volatile	/* declare this function to never return */
#endif /* not __STRICT_ANSI__ */
#endif /* __GNUC__ */
void _NXRaiseError(int code, const void *data1, const void *data2)
{
    (*ExceptionRaiser)(code, data1, data2);
}


/* forwards the error to the next handler */
void NXDefaultExceptionRaiser(int code, const void *data1, const void *data2)
{
    NXHandler *destination;
    AltHandler *altDest;

    while (1) {
	destination = HandlerStack;
	if (!destination) {
	    if (_NXUncaughtExceptionHandler)
		(*_NXUncaughtExceptionHandler)(code, data1, data2);
	    else
		fprintf(stderr, "Uncaught exception #%d\n", code);
	    exit(-1);
	} else if (IS_ALT(destination)) {
	    altDest = ALT_CODE_TO_PTR(destination);
	    HandlerStack = altDest->next;
	    altHandlersUsed = altDest - altHandlers;
	    (*altDest->proc)(altDest->context, code, data1, data2);
	} else {
	    destination->code = code;
	    destination->data1 = data1;
	    destination->data2 = data2;
	    HandlerStack = destination->next;
	    _longjmp(destination->jumpState, 1);
	}
    }
}


static char *ErrorBuffer = NULL;
static int ErrorBufferOffset = 0;

/* stack allocates some space from the error buffer */
void NXAllocErrorData(int size, void **data)
{
    int goalSize;

    goalSize = (ErrorBufferOffset + size + 7) & ~7;
    if (goalSize > ErrorBufferSize) {
	ErrorBuffer = realloc(ErrorBuffer, (unsigned)goalSize);
	ErrorBufferSize = goalSize;
    }
    *data = ErrorBuffer + ErrorBufferOffset;
    ErrorBufferOffset = goalSize;
}


void NXResetErrorData(void)
{
    ErrorBufferOffset = 0;
}

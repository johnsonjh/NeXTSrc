/*
	objectDelayedPerform.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "nextstd.h"
#import "Application.h"
#import <objc/Object.h>
#import <objc/hashtable.h>
#import <dpsclient/dpsclient.h>
#import <dpsclient/dpsNeXT.h>
#import <zone.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXObjectCategoryDelayedPerform=0\n");
    asm(".globl .NXObjectCategoryDelayedPerform\n");
#endif

@implementation Object(DelayedPerform)

typedef struct _NXDelayedPerform {
    SEL action;
    id target;
    id userData;
    double timeout;
    DPSTimedEntry te;
    struct _NXDelayedPerform *next;
    struct _NXDelayedPerform *prev;
} NXDelayedPerform;

static unsigned hashDelayedPerform(const void *info, const void *data)
{
    int i, j;
    NXDelayedPerform *dp = (NXDelayedPerform *)data;

    i = dp ? (int)(dp->action) : 0;
    i <<= 24;
    j = dp ? (int)(dp->target) : 0;
    j &= 0xffffff;

    return (unsigned)(i | j);
}

static int delayedPerformIsEqual(const void *info, const void *data1, const void *data2)
{
    NXDelayedPerform *dp1 = (NXDelayedPerform *)data1;
    NXDelayedPerform *dp2 = (NXDelayedPerform *)data2;

    if (dp1 && dp2) {
	return (dp1->action == dp2->action && dp1->target == dp2->target);
    } else if (!dp1 && !dp2) {
	return YES;
    } else {
	return NO;
    }
}

static NXHashTable *delayedPerformTable = NULL;

static void doDelayedPerform(DPSTimedEntry te, double now, void *data)
{
    NXDelayedPerform *dp, *thisdp = (NXDelayedPerform *)data;

    if (thisdp) {
	dp = NXHashGet(delayedPerformTable, thisdp);
	if (!dp) {
	    NX_ASSERT(NO, "Delayed perform not in hash table!");
	} else {
	    DPSRemoveTimedEntry(te);
	    if (dp == thisdp) {
		NXHashRemove(delayedPerformTable, thisdp);
		if (thisdp->next) {
		    NXHashInsert(delayedPerformTable, thisdp->next);
		    thisdp->next->prev = NULL;
		}
	    } else {
		if (thisdp->prev) thisdp->prev->next = thisdp->next;
		if (thisdp->next) thisdp->next->prev = thisdp->prev;
	    }
	}
	[thisdp->target perform:thisdp->action with:thisdp->userData];
	free(thisdp);
    }
}

- perform:(SEL)aSelector with:anArg afterDelay:(int)ms cancelPrevious:(BOOL)cancel
{
    NXDelayedPerform tmp;
    NXDelayedPerform *dp, *prevdp, *freedp, *nextdp;
    NXHashTablePrototype prototype;

    if (!ms) [self perform:aSelector with:anArg];
    if (ms && !delayedPerformTable) {
	prototype.hash = hashDelayedPerform;
	prototype.isEqual = delayedPerformIsEqual;
	prototype.free = 0;
	prototype.style = 0;
	delayedPerformTable = NXCreateHashTableFromZone(prototype, 2, NULL, [self zone]);
    }
    if (!delayedPerformTable) return [self perform:aSelector with:anArg];
    tmp.action = aSelector;
    tmp.target = self;
    tmp.userData = anArg;
    tmp.timeout = (double)(ms / 1000.0);
    tmp.next = NULL;
    tmp.prev = NULL;
    dp = NXHashGet(delayedPerformTable, &tmp);
    if (!dp) {
	if (ms > 0) {
	    dp = NXZoneMalloc([self zone], sizeof(NXDelayedPerform));
	    *dp = tmp;
	    dp->te = DPSAddTimedEntry(dp->timeout, doDelayedPerform, dp, NX_RUNMODALTHRESHOLD);
	    NXHashInsert(delayedPerformTable, dp);
	}
    } else if (dp) {
	if (cancel) {
	    freedp = dp->next;
	    while (freedp) {
		nextdp = freedp->next;
		DPSRemoveTimedEntry(freedp->te);
		free(freedp);
		freedp = nextdp;
	    }
	    DPSRemoveTimedEntry(dp->te);
	    if (ms > 0) {
		*dp = tmp;
		dp->te = DPSAddTimedEntry(dp->timeout, doDelayedPerform, dp, NX_RUNMODALTHRESHOLD);
	    } else {
		NXHashRemove(delayedPerformTable, dp);
		free(dp);
	    }
	} else if (ms > 0) {
	    prevdp = dp;
	    while (prevdp->next) prevdp = prevdp->next;
	    dp = NXZoneMalloc([self zone], sizeof(NXDelayedPerform));
	    *dp = tmp;
	    prevdp->next = dp;
	    dp->prev = prevdp;
	    dp->te = DPSAddTimedEntry(dp->timeout, doDelayedPerform, dp, NX_RUNMODALTHRESHOLD);
	}
    }

    return self;
}

- perform:(SEL)aSelector with:anArg afterDelay:(int)ms
{
    return [self perform:aSelector with:anArg afterDelay:ms cancelPrevious:YES];
}

@end



/*
	appLocality.m
	Copyright 1990, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	Category of the Application object to deal with the window cache for 
	better locality of reference.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "Window.h"
#import <objc/Storage.h>

typedef struct {
    id window;
    int windowNum;
    struct _flags{
	unsigned int free:1;
	unsigned int visible:1;
	unsigned int overridesUpdate:1;
	unsigned int delegateUpdates:1;
	unsigned int _RESERVED:12;
    } flags;
} WindowCache;

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXAppCategoryCache=0\n");
    asm(".globl .NXAppCategoryCache\n");
#endif

static Storage *windowCache = nil;

@implementation Application (WindowCache)

- (WindowCache *) getWindowCache:window add:(BOOL)add
{
    WindowCache *cur, *last, temp, *unused = 0;
    
    if (!windowCache)
	windowCache = [[Storage allocFromZone:[self zone]]
	    initCount:0 elementSize:sizeof(WindowCache) description:"{@is}"];
    cur = windowCache->dataPtr;
    last = cur + windowCache->numElements;
    for (; cur < last; cur++) {
	if (cur->flags.free) {
	    if (!unused)
		unused = cur;
	} else if (cur->window == window)
	    return cur;
    }
    if (!add)
	return NULL;
    if (unused) {
	bzero(unused, sizeof(WindowCache));
	unused->window = window;
	return unused;
    }
    bzero(&temp, sizeof(WindowCache));
    temp.window = window;
    [windowCache addElement:&temp];
    cur = windowCache->dataPtr;
    last = cur + windowCache->numElements;
    return last - 1;
}

- findWindowUsingCache:(int)windowNum
{
    WindowCache *cur, *last;
    
    if (!windowCache)
	return nil;
    cur = windowCache->dataPtr;
    last = cur + windowCache->numElements;
    for (; cur < last; cur++) {
	if (!cur->flags.free) {
	    if (cur->windowNum == windowNum)
		return cur->window;
	}
    }
    return nil;
}

- updateWindowUsingCache
{
    WindowCache *cur;
    int numToDo, i;
    
    if (!windowCache)
	return nil;
    numToDo = windowCache->numElements;
    for (i = 0; i < numToDo; i++) {
	cur = (WindowCache *)(windowCache->dataPtr) + i;
	if (!cur->flags.free) {
	    if (cur->flags.visible && 
		(cur->flags.overridesUpdate || cur->flags.delegateUpdates))
		[cur->window update];
	}
    }
    return self;
}


- setWindowNum:(int)windowNum forWindow:window
{
    WindowCache *cache = [self getWindowCache:window add:YES];
    cache->windowNum = windowNum;
    return self;
}

- setVisible:(BOOL)visible forWindow:window
{
    WindowCache *cache = [self getWindowCache:window add:YES];
    cache->flags.visible = visible;
    return self;
}

- setOverridesUpdate:(BOOL)overridesUpdate forWindow:window
{
    WindowCache *cache = [self getWindowCache:window add:YES];
    cache->flags.overridesUpdate = overridesUpdate;
    return self;
}

- setDelegateUpdates:(BOOL)delegateUpdates forWindow:window
{
    WindowCache *cache = [self getWindowCache:window add:YES];
    cache->flags.delegateUpdates = delegateUpdates;
    return self;
}

- removeWindowFromCache:window
{
    WindowCache *cache = [self getWindowCache:window add:NO];
    if (cache)
	cache->flags.free = YES;
    return self;
}

@end

/*
  
Modifications (starting at 2.0):
  
92
--
 8/19/90 bryan	this category implements the app cache for caching information
 		about windows. This is necessary for the findWindow: and 
		updateWindows methods because using the window list and 
		querying each of the windows will bring in a significant
		amount of memory.

*/

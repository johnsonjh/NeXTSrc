/*
	NibData.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
  
	DEFINED IN: The Application Kit
	HEADER FILES: appkit.h, nibprivate.h 
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <objc/hashtable.h>
#import <objc/Storage.h>
#import <ctype.h>
#import "appkitPrivate.h"
#import "nextstd.h"
#import "NibData.h"
#import "WindowTemplate.h"
#import "MenuTemplate.h"
#import "NameTable.h"
#import "ScrollView.h"
#import "CustomView.h"
#import "Menu.h"
#import "Control.h"
#import "CustomObject.h"
#import "Application.h"
#import "nibprivate.h"
#import "perfTimer.h"

typedef struct {
    @defs(View)
}                  *viewId;

typedef struct {
    @defs(Storage)
}                  *storageId;

typedef struct {
    @defs(Window)
}                  *windowId;

typedef struct {
    @defs(CustomObject)
}                  *customObjectId;

#define SPTR(x) 	(((storageId)(x))->dataPtr)

@implementation NibData

- free
{
    [objectList free];
    [connectList free];
    [visibleWindows free];
    return [super free];
}

static void methodCap(char *str)
{
    if (islower(str[3]))
	str[3] = toupper(str[3]);
}

- nibInstantiateIn:nameTable owner:ownerArg
{
    int	i, size;
    id firstWindow = nil;
    id ViewFactory = [View class];	/* get these out of the inner loops */
    id MenuFactory = [Menu class];
    id WindowFactory = [Window class];
    id ScrollViewFactory = [ScrollView class];
    id CustomViewFactory = [CustomView class];

    /*
     * 1. Instantiate all objects and add them in nameTable.
     */
    MARKTIME(_NXLaunchTiming, "[NibData nibInstantiateIn] instantiate objs", 2);
    PROFILEMARK('I', YES);		/* profile nib instantiation */
    if (size = [objectList count]) {
    	NamePtr		namePtr;
	const char	*theName = NULL;
	id		theObject, theOwner, rootObject;
	    
	if (size == 1) 
	    return self;
	namePtr = (NamePtr)SPTR(objectList);
	rootObject = namePtr->object;
	((customObjectId)rootObject)->realObject = ownerArg;
	namePtr->object = ownerArg;
	namePtr++;
	for (i=1; i<size; i++) {
	    MARKTIME2(_NXLaunchTiming, "instantiating %s", (int)[namePtr->object name], 0, 3);
	    if (nameTable) {
		theName = (char *)NXUniqueString(namePtr->name);
		if (namePtr->name && (theName != namePtr->name))
		    free(namePtr->name);
		namePtr->name = (char *)theName;
	    } else {
		free(namePtr->name);
	    }
	    theOwner = namePtr->owner;
	    if (theOwner && [theOwner respondsTo:@selector(nibInstantiate)]) {
	        theOwner = [theOwner nibInstantiate];
		namePtr->owner = theOwner;
	    }
	    theObject = namePtr->object;
	    if ([theObject respondsTo:@selector(nibInstantiate)]) {
	        theObject = [theObject nibInstantiate];
		namePtr->object = theObject;
	    }
	    if (theOwner == rootObject)
		theOwner = ownerArg;
	    if (nameTable && ownerArg && theName) {
	    	if (![nameTable addName:theName 
				 owner:theOwner 
				 forObject:theObject]) {
		    [nameTable removeName:theName owner:theOwner];
		    [nameTable addName:theName 
				 owner:theOwner 
				 forObject:theObject];
		}
	    }
	  /* dont set the autosize flags when loading palette */
	    if ([theObject isKindOf:ViewFactory])
	        if (((viewId)theObject)->_vFlags.autosizing)
		    [[theObject superview] setAutoresizeSubviews:YES];
	    namePtr++;
	}
    }
    PROFILEMARK('I', NO);

    /*
     * 2. All window objects are displayed. Due to CustomViews we cannot
     *    do it before all objects are instantiated.
     */
    MARKTIME(_NXLaunchTiming, "[NibData nibInstantiateIn] displaying", 2);
    if (size = [objectList count]) {
    	register NamePtr	namePtr;
	register id		theObject;
	    
	namePtr = (NamePtr)SPTR(objectList);
	for (i=0; i<size; i++) {
	    theObject = namePtr->object;
	    if ([theObject isKindOf:WindowFactory]) {
	    	if (!(((windowId)theObject)->wFlags2.deferred))
			[theObject display];
		if (!firstWindow && ![theObject isKindOf:MenuFactory]) 
		    firstWindow = theObject;
	    } else if ([theObject isKindOf:ScrollViewFactory]) {
	        if ([[theObject docView] isKindOf:CustomViewFactory])
		  /* ?? Since ClipView caches docView */
		    [theObject setDocView:[[theObject docView] nibInstantiate]];
	    }
	    namePtr++;
	}
    }

    /*
     * 3. All connections are made
     */
    MARKTIME(_NXLaunchTiming, "[NibData nibInstantiateIn] connecting", 2);
    if (size = [connectList count]) {
	register ConnectPtr	connectPtr;
    	id			source, dest;
	SEL			theAction;
		
	connectPtr = (ConnectPtr)SPTR(connectList);
	for (i=0; i<size; i++) {
    	    source = connectPtr->source;
    	    if ([source respondsTo:@selector(nibInstantiate)]) {
        	source = [source nibInstantiate];
    	    }
    	    dest = connectPtr->dest;
    	    if ([dest respondsTo:@selector(nibInstantiate)]) {
        	dest = [dest nibInstantiate];
    	    }
    	    switch (connectPtr->type) {
        	case CONTROL:
	    	    [source setTarget:dest];
		    theAction = sel_getUid(connectPtr->label);
		    if (!dest || [dest respondsTo:theAction])
			[source setAction:theAction];
	    	    break;
        	case OUTLET: {
		    char methodString[256];
		    SEL theSelector;
		    strcpy(methodString, "set");
		    strcat(methodString, connectPtr->label);
		    strcat(methodString, ":");
		    methodCap(methodString);
		    theSelector = sel_getUid(methodString);
		    if ([source respondsTo:theSelector]) {
			[source perform:theSelector with:dest];
		    } else {
			object_setInstanceVariable(source, connectPtr->label, dest);
		    }
		    break;
		}
    	    }
	    /* ASSUMING LABELS IN CONNECTLIST ARE NEVER UNIQUESTRINGS
	     * WHEN CALLING NIBINSTANTIATE
	     */
	    free(connectPtr->label);
	    connectPtr++;
	}
    }

    /*
     * 4. The visible windows are shown.
     */
    MARKTIME(_NXLaunchTiming, "[NibData nibInstantiateIn] visible windows", 2);
    PROFILEMARK('D', YES);		/* profile window draw */
    if (size = [visibleWindows count]) {
    	register id	*windowPtr;
	register id	 newWin;
		
	windowPtr = (id *)NX_ADDRESS(visibleWindows);
	for (i=0; i<size; i++) {
	    MARKTIME2(_NXLaunchTiming, "instantiating a %s", (int)[*windowPtr name], 0, 3);
	    newWin = [*windowPtr nibInstantiate];
	    MARKTIME2(_NXLaunchTiming, "ordering a %s, title %s", (int)[newWin name], (int)[newWin title], 3);
	    [newWin orderFront:nil];
	    windowPtr++;
	}
    }
    PROFILEMARK('D', NO);
    return firstWindow;
}

- write:(NXTypedStream *)s
{
    NXWriteTypes(s, "@@@@s", &objectList, &connectList, &visibleWindows,
		 &extension, &nFlags);
    return self;
}

- read:(NXTypedStream *)s
{
    NXReadTypes(s, "@@@@s", &objectList, &connectList, &visibleWindows,
		&extension, &nFlags);
    return self;
}

@end

/*
  
Modifications (starting at 0.8):

0.91
----
 5/19/89 trey	added launch profiling code

85
--
  6/4/90 pah	Fixed bug: unarchiving CustomView in a ScrollView broken

87
--
  7/4/90 trey	Factory objects cached in -nibInstantiateIn:owner:
		object_setInstanceVariable used to hookup outlets
		methodCap rewritten to use ctypes.h macros

*/

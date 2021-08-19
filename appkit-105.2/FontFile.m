/*
	FontFile.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
  
	DEFINED IN: The Application Kit
	HEADER FILES: FontFile.h
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "FontFile.h"
#import "Application.h"
#import "Panel.h"
#import "pbtypes.h"
#define c_plusplus 1
#include "pbs.h"
#include "app.h"
#undef c_plusplus
#import <dpsclient/dpsclient.h>
#import <string.h>
#import <sys/param.h>
#import "nextstd.h"
#import <mach.h>
#import <zone.h>

#define WEIGHT_TOLERANCE 1

/*
 * Used only when the byteSex of the file differs from the byteSex of the
 * machine on which the code is executing.  Reverses the bytes in the int.
 */

static int
readInt(char *data, int byteSex)
{
    int retval = 0;
    if (byteSex)
	while (byteSex--)
	    retval = (retval << 8) + (*data++ & 0xff);
    return retval;
}

#define INTSIZE (byteSex ? byteSex : sizeof(int))
#define READINT(data) (byteSex ? readInt(data, byteSex) : *((int*)data))
#define ENTRY_SIZE ((INTSIZE << 2) + 4)
#define STRINGS (data + ENTRY_SIZE * count)
#define ENTRY(offset) (data+ENTRY_SIZE*offset)

typedef struct _EventQueue {
    NXEvent event;
    struct _EventQueue *next;
} EventQueue;

@implementation FontFile

static EventQueue *saveEventQueue(NXZone *zone)
/*
 * Saves the entire event queue including currentEvent.
 * Note that restoreEventQueue reposts currentEvent, then gets it so that
 * currentEvent will not be changed by a saveEventQueue/restoreEventQueue
 * pair.
 */
{
    EventQueue *nlink, *alink = NULL;

    NXPing();
    NX_ZONEMALLOC(zone, alink, EventQueue, 1);
    NX_ZONEMALLOC(zone, alink->next, EventQueue, 1);
    alink->next->next = NULL;
    alink->next->event = *[NXApp currentEvent];
    if (alink->next->event.type == 0)
	alink->next->event.type = -1;
    while ([NXApp peekNextEvent:NX_ALLEVENTS into:&(alink->event)]) {
	[NXApp getNextEvent:NX_ALLEVENTS];
	NX_ZONEMALLOC(zone, nlink, EventQueue, 1);
	nlink->next = alink;
	alink = nlink;
    }
    nlink = alink->next;
    NX_FREE(alink);

    return nlink;
}

static void restoreEventQueue(EventQueue *eq)
{
    NXEvent dummy;
    EventQueue *alink;
    BOOL currEventNull;

    if (eq) {
	currEventNull = NO;
	while (eq) {
	    if (eq->event.type != -1) {
		DPSPostEvent(&(eq->event), 1);
	    } else {
		currEventNull = YES;
	    }
	    alink = eq;
	    eq = eq->next;
	    NX_FREE(alink);
	}
	if (!currEventNull && [NXApp peekNextEvent:NX_ALLEVENTS into:&dummy]) {
	    [NXApp getNextEvent:NX_ALLEVENTS];
	}
    }
}

+ new:(const char *)fontsDirectory
{
    return [[self allocFromZone:NXDefaultMallocZone()] init:fontsDirectory];
}

- init:(const char *)fontsDirectory
{
    id alert;
    port_t server;
    NXEvent event;
    int retval, runState;
    NXModalSession session;
    EventQueue *eq;
    char path[MAXPATHLEN+1];
    char *colon, *info, *user, *home;
    unsigned int pathLen, userLen, homeLen, infoLen;
    struct timeval start, now;
    struct timezone tz;
    const int timeout = 60;

    if (server = _NXLookupAppkitServer(NULL, NULL, NULL)) {
	if (colon = strchr(fontsDirectory, ':')) {
	    strncpy(path, fontsDirectory, colon - fontsDirectory);
	    path[colon - fontsDirectory] = '\0';
	} else {
	    strcpy(path, fontsDirectory);
	}
	user = (char *)NXUserName();
	home = (char *)NXHomeDirectory();
	pathLen = strlen(path);
	userLen = strlen(user);
	homeLen = strlen(home);
	retval = _NXBuildFontDirectory(server, timeout*1000, path, pathLen,
				       user, userLen, home, homeLen,
				       &info, &infoLen);
	if (retval == FONTDIR_STALE) {
	    alert = NXGetAlertPanel(NULL,
		KitString(FontManager, "Incorporating information about new fonts.\nPlease wait (this may take up to %d seconds).", NULL),
		KitString(FontManager, "Stop", "Button user presses to abort the incorporation of new fonts."),
		NULL, NULL, timeout);
	    gettimeofday(&start, &tz);
	    [NXApp beginModalSession:&session for:alert];
	    eq = saveEventQueue([self zone]);
	    runState = NX_RUNCONTINUES;
	    while (retval != FONTDIR_OK && runState == NX_RUNCONTINUES) {
		vm_deallocate(task_self(), (vm_address_t)info, infoLen);
		[NXApp peekNextEvent:NX_ALLEVENTS into:&event
			waitFor:5.0 threshold:31];
		runState = [NXApp runModalSession:&session];
		retval = _NXGetFontDirectory(server, path, pathLen, user,
		    userLen, home, homeLen, &info, &infoLen);
		gettimeofday(&now, &tz);
		if (now.tv_sec - start.tv_sec > timeout)
		    runState = NX_RUNABORTED;
	    }
	    [NXApp endModalSession:&session];
	    [alert orderOut:self];
	    NXFreeAlertPanel(alert);
	    if (retval != FONTDIR_OK && runState != NX_ALERTDEFAULT) {
		_NXKitAlert("Error", NULL, "There was a problem incorporating information about new fonts.  Using old information.", NULL, NULL, NULL);
	    }
	    restoreEventQueue(eq);
	}
	if (retval == FONTDIR_BAD) {
	    info = NULL;
	}
    }

    if (info) {
	[super init];
	data = info;
	byteSex = *data;
	data += 4;
	count = READINT(data);
	data += byteSex ? byteSex : 4;
	return self;
    } else {
	[self free];
	return nil;
    }
}


- (unsigned int)count
{
    return (unsigned int)count;
}

- (int)offset:(const char *)name
{
    int                 i;
    char               *s, *dp = data;

    if (name) {
	for (i = 0; i < count; i++) {
	    dp += ENTRY_SIZE - INTSIZE;
	    s = STRINGS + READINT(dp);
	    if (!strcmp(s, name)) {
		return i;
	    }
	    dp += INTSIZE;
	}
    }
    return -1;
}

static const int    MATCH_MASK = ~(NX_BOLD | NX_UNBOLD);

static BOOL
traitsMatch(NXFontTraitMask traits,
	    NXFontTraitMask myTraits, char weight, char myWeight)
{
    if ((traits & MATCH_MASK) == (myTraits & MATCH_MASK)) {
	if (traits & NX_UNBOLD) {
	    if (myTraits & NX_UNBOLD) {
		return YES;
	    } else if (traits & NX_BOLD) {
		if (myTraits & NX_BOLD) {
		    return YES;
		}
	    }
	} else if (traits & NX_BOLD) {
	    if (myTraits & NX_BOLD) {
		return YES;
	    }
	} else if (weight) {
	    if ((weight >= myWeight - WEIGHT_TOLERANCE) &&
		(weight <= myWeight + WEIGHT_TOLERANCE)) {
		return YES;
	    }
	}
    }
    return NO;
}

static NXFontTraitMask
readTraits(char *dp)
{
    NXFontTraitMask     retval = 0;

    retval = (*dp++) & 0xff;
    retval |= ((*dp++) & 0xff) << 8;
    retval |= ((*dp++) & 0xff) << 16;
    if (!(retval & NX_ITALIC))
	retval |= NX_UNITALIC;
    return retval;
}

static int
findAMatch(char *data, int count, int byteSex,
	   const char *family, int weight, NXFontTraitMask traits)
{
    int                 i;
    char               *dp = data;
    char               *myFamily;
    int                 myWeight;
    NXFontTraitMask     myTraits;
    int			bestOffset = -1, bestDiff = 100;

    if (family) {
	for (i = 0; i < count; i++) {
	    myFamily = STRINGS + READINT(dp);
	    if (myFamily && (family == myFamily || !strcmp(myFamily, family))) {
		dp += INTSIZE * 2;
		myWeight = (int)(*dp++);
		myTraits = readTraits(dp);
		if (traitsMatch(traits, myTraits, weight, myWeight)) {
		    if (weight == myWeight) {
			return i;
		    } else if (bestOffset<0 || ABS(weight-myWeight)<bestDiff) {
			bestOffset = i;
			bestDiff = ABS(weight-myWeight);
		    }
		}
		dp += 3 + INTSIZE * 2;
	    } else {
		dp += ENTRY_SIZE;
	    }
	}
    }
    return bestOffset;
}

/*
 * The most general matcher.  This just tries to find a font which matches
 * the specified family, traits and weight.
 */

- (int)matchFamily:(const char *)family
    traits:(NXFontTraitMask)traits
    weight:(int)weight
{
    return findAMatch(data, count, byteSex, family, weight, traits);
}

/*
 * We resolve traits which are opposites here.  For example, you can't
 * have a font which is both expanded and condensed or italic and unitalic.
 */

static NXFontTraitMask
applyTrait(NXFontTraitMask trait,
	   NXFontTraitMask traits)
{
    if (trait & (NX_EXPANDED | NX_CONDENSED)) {
	traits &= ~(NX_EXPANDED | NX_CONDENSED);
    } else if (trait & (NX_UNITALIC | NX_ITALIC)) {
	traits &= ~(NX_UNITALIC | NX_ITALIC);
    } else if (trait & (NX_BOLD | NX_UNBOLD)) {
	traits &= ~(NX_BOLD | NX_UNBOLD);
    }
    traits |= trait;
    return traits;
}

static NXFontTraitMask
excludeTrait(NXFontTraitMask trait,
	   NXFontTraitMask traits)
{
    traits &= ~(trait);
    if (trait & NX_ITALIC) traits |= NX_UNITALIC;
    if ((trait & NX_BOLD) && !(trait & NX_UNBOLD)) {
	traits |= NX_UNBOLD;
    } else if (!(trait & NX_BOLD) && (trait & NX_UNBOLD)) {
	traits |= NX_BOLD;
    }
    return traits;
}

/*
 * This is used to find an italic version of a font, or a narrow version
 * of a font or the bold version of a font, etc ...
 */

- (int)matchTrait:(NXFontTraitMask)trait to:fontFile at:(int)offset
{
    return findAMatch(data, count, byteSex,[fontFile family:offset],
		      [fontFile weight:offset],
		      applyTrait(trait,[fontFile traits:offset]));
}

- (int)excludeTrait:(NXFontTraitMask)trait from:fontFile at:(int)offset
{
    int retval = findAMatch(data, count, byteSex,[fontFile family:offset],
		      [fontFile weight:offset],
		      excludeTrait(trait, [fontFile traits:offset]));
    if (retval < 0) {
	if ((trait & NX_BOLD) && !(trait & NX_UNBOLD)) {
	    return findAMatch(data, count, byteSex,[fontFile family:offset],
		      [fontFile weight:offset],
		      excludeTrait(trait|NX_UNBOLD, [fontFile traits:offset]));
	} else if (!(trait & NX_BOLD) && (trait & NX_UNBOLD)) {
	    return findAMatch(data, count, byteSex,[fontFile family:offset],
		      [fontFile weight:offset],
		      excludeTrait(trait|NX_BOLD, [fontFile traits:offset]));
	}
    }
    return retval;
}

/*
 * This matches the traits of the specified font into the specified
 * family (if possible, of course).
 */

- (int)matchFamily:(const char *)family to:fontFile at:(int)offset
{
    return findAMatch(data, count, byteSex, family,
		      [fontFile weight:offset],[fontFile traits:offset]);
}

- (int)modifyWeight:(BOOL)up of:fontFile at:(int)offset
{
    int                 i;
    int                 weight;
    char               *dp = data, *family;
    NXFontTraitMask     traits;
    int                 myWeight;
    char               *myFamily;
    NXFontTraitMask     myTraits;
    int			bestOffset = -1;
    int			smallestDiff = 100;

    family = [fontFile family:offset];
    weight = [fontFile weight:offset];
    traits = [fontFile traits:offset] & MATCH_MASK;

    if (family) {
	for (i = 0; i < count; i++) {
	    myFamily = STRINGS + READINT(dp);
	    if (myFamily && (family == myFamily || !strcmp(myFamily, family))) {
		dp += INTSIZE * 2;
		myWeight = (int)(*dp++);
		myTraits = readTraits(dp) & MATCH_MASK;
		if ((up && weight < myWeight) || (!up && weight > myWeight)) {
		    if (traits == myTraits) {
			if (bestOffset < 0 ||
		    	    (ABS(weight - myWeight) < smallestDiff)) {
			    bestOffset = i;
			    smallestDiff = ABS(weight - myWeight);
			}
		    }
		}
		dp += 3 + INTSIZE * 2;
	    } else {
		dp += ENTRY_SIZE;
	    }
	}
    }

    return bestOffset;
}


- (char *)family:(int)offset
{
    char               *entry;

    if (offset < count) {
	entry = ENTRY(offset);
	return STRINGS + READINT(entry);
    } else {
	return NULL;
    }
}

- (char *)face:(int)offset
{
    char               *entry;

    if (offset < count) {
	entry = ENTRY(offset) + INTSIZE;
	return STRINGS + READINT(entry);
    } else {
	return NULL;
    }
}

- (char *)name:(int)offset
{
    char               *entry;

    if (offset < count) {
	entry = ENTRY(offset) + INTSIZE * 3 + 4;
	return STRINGS + READINT(entry);
    } else {
	return NULL;
    }
}

- (int)weight:(int)offset
{
    if (offset < count) {
	return (int)(*(ENTRY(offset) + INTSIZE * 2));
    } else {
	return 0;
    }
}

- (NXFontTraitMask)traits:(int)offset
{
    if (offset < count) {
	return readTraits(ENTRY(offset) + INTSIZE * 2 + 1);
    } else {
	return 0;
    }
}

- (char *)getFullName:(char *)buf of:(int)offset withSize:(float)size
{
    char               *family, *face, *entry;

    if (offset < count) {
	entry = ENTRY(offset);
	family = STRINGS + READINT(entry);
	entry += INTSIZE;
	face = STRINGS + READINT(entry);
	if (size) {
	    sprintf(buf, "%s %s %.1f %s", family, face, size, KitString(FontManager, "pt.", "Abbreviation for point size of a font."));
	} else {
	    sprintf(buf, "%s %s", family, face);
	}
	return buf;
    } else {
	return NULL;
    }
}

- (BOOL)isItalic:(int)offset
{
    return ([self traits:offset] & NX_ITALIC);
}

- (BOOL)isBold:(int)offset
{
    return ([self traits:offset] & NX_BOLD);
}

- (BOOL)isUnbold:(int)offset
{
    return ([self traits:offset] & NX_UNBOLD);
}

@end

/*
  
Modifications (since 0.8):
  
 2/18/89 pah	new for 0.82
		supports the FontManager and FontPanel classes
 3/14/89 pah	chain SIGCHLD so it doesn't interfere with app's usage
 3/17/89 pah	convert to NXFontTraitMask

0.91
----
 5/19/89 trey	minimized static data

0.93
----
 6/14/89 pah	fix small bug with byte-sex independence
 6/25/89 pah	move responsibility for getting font info to the appkit server

10/29/89 trey	_NXLookupAppkitServer now takes an argument

96
--
 10/8/90 trey	saveEventQueue/restoreEventQueue handle an empty currentEvent

*/

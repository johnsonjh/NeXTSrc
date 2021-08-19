/*
	StreamTable.m
	Pickling data structures
  	Copyright 1989, NeXT, Inc.
	Responsability: Bertrand Serlet
  
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import "StreamTable.h"

typedef struct {
    id		object;
    char	*buffer;
    int		length;
    } Pickle;
    
@implementation  StreamTable

+ newKeyDesc: (const char *) aKeyDesc {
    return [super newKeyDesc: aKeyDesc valueDesc: "!"];
    };

+ new {
    return [self newKeyDesc: "@"];
    };

- initKeyDesc: (const char *) aKeyDesc {
    return [self initKeyDesc: aKeyDesc valueDesc: "!"];
    };

- init {
    return [self initKeyDesc: "@"];
    };

static void freePickle (void *value) {
    Pickle	*pick;
    if (! value) return;
    pick = (Pickle *) value;
    free (pick->buffer); /* we use free and not NXFreeObjectBuffer because malloc was used for creation */
    free (pick);
    };

static void freePickleAndObject (void *value) {
    Pickle	*pick;
    if (! value) return;
    pick = (Pickle *) value;
    [pick->object free];
    free (pick->buffer); /* we use free and not NXFreeObjectBuffer because malloc was used for creation */
    free (pick);
    };

static id getObject (void *value, NXZone *zone) {
    Pickle	*pick;
    if (! value) return nil;
    pick = (Pickle *) value;
    if (! pick->object)
	pick->object = NXReadObjectFromBufferWithZone (pick->buffer, pick->length, zone);
    return pick->object;
    };
    
static void freeObject (void *item) {
    [(id) item free];
    };
    
static void noFree (void *item) {
    };
    
- free {
    [self freeKeys: noFree values: freePickle];
    return [super free];
    };
        
- freeObjects {
    return [self freeKeys: (keyDesc[0] == '@') ? freeObject : noFree values: freePickleAndObject];
    };

- (id) valueForStreamKey: (const void *) aKey {
    void	*value = [self valueForKey: aKey];
    return getObject (value, [self zone]);
    };
     
- (id) insertStreamKey: (const void *) aKey value: (id) aValue {
    Pickle	*pick;
    char	*vmbuffer;
    NXZone *zone = [self zone];

    pick = (Pickle *) NXZoneMalloc (zone, sizeof (Pickle));
    vmbuffer = NXWriteRootObjectToBuffer ((id) aValue, &pick->length);
    pick->buffer = NXZoneMalloc (zone, pick->length);
    bcopy (vmbuffer, pick->buffer, pick->length);
    NXFreeObjectBuffer (vmbuffer, pick->length);
    pick->object = (id) aValue;
    [self insertKey: aKey value: pick];
    return nil;
    };

- (id) removeStreamKey: (const void *) aKey {
    freePickle ([self removeKey: aKey]);
    return nil;
    };

- (NXHashState) initStreamState {
    return [self initState];
    };
    
- (BOOL) nextStreamState: (NXHashState *) aState key: (const void **) aKey value: (id *) aValue {
    void	*aux;
    BOOL	result = [self nextState: aState key: aKey value: &aux];
    *aValue = getObject (aux, [self zone]);
    return result;
    };
    
- write:(NXTypedStream *) stream
{
    NXHashState	state = [self initState];
    const void	*key;
    Pickle	*pick;
    [super write: stream];
    while ([self nextState: &state key: &key value: &((void *) pick)]) {
	NXWriteType (stream, keyDesc, &key);
	NXWriteType (stream, "i", &pick->length);
	NXWriteArray (stream, "c", pick->length, pick->buffer);
	};
    return self;
    }

- read:(NXTypedStream *) stream
{
    int		nb;
    NXZone *zone = [self zone];

    [super read: stream];
    nb = count;
    while (nb--) {
	const void	*key;
	Pickle	*pick = (Pickle *) NXZoneMalloc (zone, sizeof (Pickle));
	NXReadType (stream, keyDesc, &key);
	NXReadType (stream, "i", &pick->length);
	pick->buffer = (char *) NXZoneMalloc (zone, pick->length);
	NXReadArray (stream, "c", pick->length, pick->buffer);
	pick->object = nil;
	[self insertKey: key value: pick];
	};
    return self;
    }


@end



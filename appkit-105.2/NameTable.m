/*
	NameTable.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
  
	DEFINED IN: The Application Kit
	HEADER FILES: appkit.h, nibprivate.h
	MODIFIED:
		28 Sep 88 lrb - added ability to get object from a pathname.
				The '_' is the path delimiter.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import <string.h>
#import <objc/Storage.h>

#import "NameTable.h"
#import "nibprivate.h"

#define SPTR(x) 	(((storageId *) (x))->dataPtr)
typedef struct {
    @defs (Storage)
}                   storageId;

/*	HASHING: I've duplicated all that code because, the way things are saved on file, it is really important that local_objc_strhash does not change (BS, 02/16/89) */

#define TST() if (*s == '\0') break;
#define SHFT(N)	(*s++ << ((N)*8))

static unsigned local_objc_strhash(const unsigned char *s)
{
    register unsigned int hash = 0;

 /* unroll the loop */
    for (;;) {
	TST();
	hash ^= *s++;
	TST();
	hash ^= SHFT(1);
	TST();
	hash ^= SHFT(2);
	TST();
	hash ^= SHFT(3);
    }
    return hash;
}

#define MAXNAMELEN 256

@implementation NameTable

/* Static C functions */

static NamePtr getSymbolForName(NameTable *self, const char *name, id owner)
{
    id                  bucket;
    register NamePtr    current;
    register int        i, size;

    if (!name)
	return ((NamePtr) 0);
    bucket = [self objectAt:local_objc_strhash((unsigned char *)name) % self->hashSize];
    size = [bucket count];
    current = ((NamePtr) SPTR(bucket));
    for (i = 0; i < size; i++) {
	if (!strcmp(name, current->name) && (owner == current->owner))
	    return current;
	current++;
    }
    return ((NamePtr) 0);
}

static int extractRootFromPath(char *buf, const char *path,
		    unsigned short delimiter)
{
    int                 i, j, max;

    if (!path) {
	buf[0] = '\0';
	return 0;
    }
    max = strlen(path);
    for (i = 0; i < max; i++)
	if (path[i] == delimiter)
	    break;
    for (j = 0; j < i; j++)
	buf[j] = path[j];
    buf[i] = '\0';
    return i == max;
}

static void extractSubpath(char *buf, const char *path, unsigned short delimiter)
{
    char               *p;

    if (!path)
	buf[0] = '\0';
    else if ((p = strchr(path, delimiter)) && strlen(p))
	strcpy(buf, ++p);
    else
	buf[0] = '\0';
}

static NamePtr getSymbolForPath(id self, const char *path, id owner, unsigned short delimiter)
{
    NamePtr             symbol;
    char                buf1[MAXNAMELEN], buf2[MAXNAMELEN];
    BOOL                lastOne = NO;

    lastOne = extractRootFromPath(buf1, path, delimiter);
    symbol = getSymbolForName(self, buf1, owner);
    strcpy(buf1, path);
    while (!lastOne && symbol) {
	extractSubpath(buf2, buf1, delimiter);
	lastOne = extractRootFromPath(buf1, buf2, delimiter);
	symbol = getSymbolForName(self, buf1, symbol->object);
	strcpy(buf1, buf2);
    }
    return symbol;
}

static NamePtr getSymbolForObject(NameTable *self, id object)
{
    register int        i, size, j, bSize;
    register id        *bucketPtr;
    register NamePtr    current;

    size = [self count];
    bucketPtr = (id *)NX_ADDRESS(self);
    for (i = 0; i < size; i++) {
	bSize = [*bucketPtr count];
	current = ((NamePtr) SPTR(*bucketPtr));
	for (j = 0; j < bSize; j++) {
	    if (object == current->object)
		return current;
	    current++;
	}
	bucketPtr++;
    }
    return ((NamePtr) 0);
}

static void removeSymbol(NameTable *self, NamePtr symbol)
{
    unsigned            index;
    register NamePtr    current;
    register int        i, size;
    id                  bucket;

    bucket = [self objectAt:index = local_objc_strhash((unsigned char *)(symbol->name)) % self->hashSize];
    size = [bucket count];
    current = ((NamePtr) SPTR(bucket));
    for (i = 0; i < size; i++) {
	if (current == symbol)
	    break;
	current++;
    }
    if (i < size) {
	[self replaceObjectAt:index with:[bucket removeAt:i]];
    }
}

/* Objective C */

+ new
{
    return [self newHashSize:HASHSIZE];
}

+ newHashSize:(int)theSize
{
    return [[self allocFromZone:NXDefaultMallocZone()] initCount:theSize];
}

- init
{
    return [self initHashSize:HASHSIZE];
}

- initHashSize:(int)theSize
{
    register int        i;

    [super initCount:theSize];
    hashSize = theSize;
    for (i = 0; i < theSize; i++) {
	[self addObject:[[Storage allocFromZone:[self zone]] initCount:0 elementSize:NAMESIZE description:NAMEDESCR]];
    }
    return self;
}

- free
{
    [self freeObjects];
    return[super free];
}

- addName:(const char *)theName owner:theOwner forObject:theObject
{
    NamePtr             symbol;
    id                  bucket;
    NameRecord          symbolRec;
    unsigned            index;

    symbol = getSymbolForName(self, theName, theOwner);
    if (symbol)
	return nil;
    bucket = [self objectAt:index = local_objc_strhash((unsigned char *)theName) % hashSize];
    symbolRec.name = (char *)theName;
    symbolRec.owner = theOwner;
    symbolRec.object = theObject;
    symbol = &symbolRec;
    [self replaceObjectAt:index with:[bucket insert:((char *)symbol) at :0]];
    return self;
}

- getObjectForName:(const char *)theName owner:theOwner
{
    NamePtr             symbol;
    unsigned short      delimiter = '_';

/*    symbol = getSymbolForName(self, theName, theOwner); */
    symbol = getSymbolForPath(self, theName, theOwner, delimiter);
    if (symbol)
	return symbol->object;
    else
	return nil;
}

- (const char *)getObjectName:theObject andOwner:(id *)theOwner
{
    NamePtr             symbol;

    symbol = getSymbolForObject(self, theObject);
    if (symbol) {
	*theOwner = symbol->owner;
	return symbol->name;
    } else
	return ((char *)0);
}

- removeName:(const char *)theName owner:theOwner
{
    NamePtr             symbol;

    symbol = getSymbolForName(self, theName, theOwner);
    if (symbol)
	removeSymbol(self, symbol);
    return self;
}

- removeObject:theObject
{
    NamePtr             symbol;

    symbol = getSymbolForObject(self, theObject);
    if (symbol)
	removeSymbol(self, symbol);
    return self;
}

@end

/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the new List object;
2/16/89	bs	removed dependency on hashing function;
  
  
*/




/*
	NXImage.m
	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXImage_Private.h"
#import "NXImageRep_Private.h"
#import "imageWraps.h"

#import "appkitPrivate.h"
#import "graphics.h"
#import "nextstd.h"
#import "tiff.h"
#import "errors.h"
#import "color.h"
#import "publicWraps.h"
#import "Bitmap.h"
#import "View.h"
#import "Application_Private.h"
#import <dpsclient/dpsclient.h>
#import <dpsclient/wraps.h>

#import <zone.h>
#import <string.h>
#import <sys/loader.h>
#import <objc/HashTable.h>
#import <objc/List.h>

@interface Bitmap(Private)
+ (BOOL)_findBuiltInBitmap:(const char *)name 
	rect:(NXRect *)rect win:(Window **)win;
+ _findExistingBitmap:(const char *)name;
@end

/*
 * The macro SIZEOK(NXSize *) returns YES or NO depending on whether
 * size encloses any area at all.
 */
#define SIZEOK(s) ((((s)->width > 0.0) && ((s)->height > 0.0)) ? YES : NO)

/*
 * The various types of streams/files we can parse.
 * The ImageExtensions[] array holds the names of the extensions
 * Image can recognize. This array should have NULL as the last
 * extension.
 */
#define UNKNOWNTYPE 0
#define EPSTYPE 1
#define TIFFTYPE 2

typedef struct _ExtensionTypePair {
    const char *extension;
    int type;
} ExtensionTypePair;

static const ExtensionTypePair ImageExtensions[] = {
    {"tiff", TIFFTYPE},
    {"eps", EPSTYPE},
    {NULL, 0}
};

/*
 * Use imageHashTable to store images by name. Created in +initialize.
 * Use delegateHashTable to store delegates for images. Created lazily.
 */
static HashTable *imageHashTable = nil;
static HashTable *delegateHashTable = nil;

#define REPRESENTATIONS		((RepresentationInfo *)_reps)

/*
 * The following macros are used to quickly return from functions in certain
 * cases.
 */
#define RETURNIFSUBIMAGE(retVal) \
	if (_flags.subImage) return(retVal)

#define RETURNIFBUILTIN(retVal) \
	if (_flags.builtIn) return(retVal)

#define RETURNIFSUBIMAGEORBUILTIN(retVal) \
	if (_flags.builtIn || _flags.subImage) return(retVal)

/*
 * _NXTypeOfFile returns the type of image file based on extension.
 * We only care about the extensions and file types enumerated in
 * the ImageExtensions array.
 */
static int _NXTypeOfFile (const char *name)
{
    char *extension = rindex(name, '.');
    if (extension) {
	const ExtensionTypePair *ext = &ImageExtensions[0];
	extension++;
	while (ext->extension) {
	    if (strcmp(ext->extension, extension) == 0) {
		return ext->type;
	    }
	    ext++;
	}
    }
    return UNKNOWNTYPE;
}

/*
 * _NXTypeOfStream returns the type of stream based on the first few
 * characters in the stream.  Stream has to be seekable.
 */
static int _NXTypeOfStream (NXStream *stream)
{
    int startOfStream = NXTell(stream);
    int ch1, ch2;
    int streamType = UNKNOWNTYPE;
   
    ch1 = NXGetc(stream);
    ch2 = NXGetc(stream);

    if ((ch1 == '%') && (ch2 == '!')) {
	streamType = EPSTYPE;
    } else if ((ch1 == ch2) && ((ch1 == 'I') || (ch2 == 'M'))) {
	streamType = TIFFTYPE;
    };

    NXSeek(stream, startOfStream, NX_FROMSTART);

    return streamType;
}

/*
 * These functions keep a list of Views to be used in focusing on NXImage
 * caches.
 */
static List *keptViews = nil;

static View *getTempFocusView()
{
    id view = [keptViews removeLastObject];
    if (view == nil) {
	view = [[View allocFromZone:[NXApp zone]] init];
    }
    return view;
}

static void freeTempFocusView(View *view)
{
    if (!keptViews) {
	keptViews = [[List allocFromZone:[NXApp zone]] initCount:3];
    }
    [keptViews addObject:view];
}

static void rectFillWithColor (const NXRect *rect, NXColor color)
{
    PSgsave ();
    NXSetColor (color);
    NXRectFill (rect);
    PSgrestore ();
}

static void clearChecked (RepresentationInfo *rep)
{
    while (rep) {
	rep->flags.checked = NO;
	rep = rep->next;
    }
}

@implementation NXImage

+ initialize
{
    [self setVersion:2];
    if (!imageHashTable) {
	NXZone *zone = [NXApp zone];
	if (!zone) zone = NXDefaultMallocZone();
	imageHashTable = [[HashTable allocFromZone:zone]
				initKeyDesc:"*" valueDesc:"@"];
    }

    return self;
}

+ new 
{
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

+ newFromFile:(const char *)fileName
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromFile:fileName];
}

- initFromFile:(const char *)fileName
{
    [self initSize:NULL];
    if ([self useFromFile:fileName]) {
	return self;
    } else {
	[self free];
	return nil;
    }
}

+ newFromSection:(const char *)sName
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromSection:sName];
}

- initFromSection:(const char *)sName
{
    [self initSize:NULL];
    if ([self useFromSection:sName]) {
	return self;
    } else {
	[self free];
	return nil;
    }
}
    
  
+ newFromStream:(NXStream *)stream
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromStream:stream];
}

- initFromStream:(NXStream *)stream
{
    [self initSize:NULL];
    if ([self loadFromStream:stream]) {
	return self;
    } else {
	[self free];
	return nil;
    }
}

+ newSize:(const NXSize *)aSize
{
    return [[super allocFromZone:NXDefaultMallocZone()] initSize:aSize];
}

- initSize:(const NXSize *)aSize
{    
    [super init];

    if (aSize) {
	[self setSize:aSize];
    } else {
	_size.width = _size.height = 0.0;
	_flags.sizeWasExplicitlySet = NO;
    }

    _flags.multipleResolutionMatching = YES;
    _flags.dataRetained = NO;
    _flags.uniqueWindow = NO;
    _flags.flipDraw = NO;
    _flags.scalable = NO;
    _flags.aSynch = NO;
    _flags.unboundedCacheDepth = NO;
    _flags.needsToExpand = NO;

    _reps = NULL;
    name = NULL;

    return self;
}

- init
{
    return [self initSize:NULL];
}

/*
 * SubImages are images that actually are defined as a section of another
 * image.  Their size and location cannot changed once they are defined.
 */
+ newFromImage:(NXImage *)image rect:(const NXRect *)rect
{
    return [[self allocFromZone:NXDefaultMallocZone()]
			initFromImage:image rect:rect];
}

- initFromImage:(NXImage *)image rect:(const NXRect *)rect
{
    [self initSize:&(rect->size)];
    _flags.subImage = YES;
    ((NXImage *)_reps) = image;
    NX_ZONEMALLOC([self zone], (NXPoint *)_repList, NXPoint, 1);
    *((NXPoint *)_repList) = rect->origin;
    return self;
}

- getImage:(NXImage **)image rect:(NXRect *)rect
{
    if (!_flags.subImage) {
	*image = nil;
	return self;
    }
    if (image) {
	*image = (NXImage *)_reps;
    }
    if (rect) {
	rect->origin = *(NXPoint *)_repList;
	rect->size = _size;
    }
    return self;
}

+ _imageNamed:(const char *)aString
{
    return [imageHashTable valueForKey:aString];
}

- (BOOL)_sizeSet
{
    return (SIZEOK(&_size) || _flags.sizeWasExplicitlySet) ? YES : NO;
}

- _setNeedsToExpand:(BOOL)flag
{
    _flags.needsToExpand = flag;
    return self;
}

/*
 * Call expand whenever you look at the representations in the object; it makes
 * sure all lazily evaluated items are evaluated to some degree.
 */
- _expand
{
    if (_flags.needsToExpand) {
	RepresentationInfo *rep = REPRESENTATIONS;
	while (rep) {
	    if ((rep->image == nil) && REPLAZY(rep)) {
		[self _expandRep:rep];
	    }
	    rep = rep->next;
	}
	[self _setNeedsToExpand:NO];
    }
    return self;
}

- recache
{
    RepresentationInfo *rep = REPRESENTATIONS;

    RETURNIFSUBIMAGEORBUILTIN(self);

    while (rep) {
	rep->flags.rendered = NO;
	rep->flags.drawError = NO;
	rep = rep->next;
    }
    return self;
}

/*
 * Call with setName:NULL to unname the image and unregister the name.
 */
- (BOOL)setName:(const char *)aString
{
    RETURNIFBUILTIN(NO);

    /*
     * First check to see if the proposed name is in use; if so,
     * return NO to indicate we can't use this name.
     */
    if (aString && [[self class] _imageNamed:aString]) {
	return NO;
    }
   
    /* If we have a previous name, delete & unregister it. */
    if (name) {
	[imageHashTable removeKey:name];
	NX_FREE(name);
    }

    if (aString) {
    	NXZone *zone = [imageHashTable zone];
	name = NXCopyStringBufferFromZone(aString,  zone ? zone : [NXApp zone]);
	[imageHashTable insertKey:(void *)name value:(void *)self];
    }

    return YES;
}

- (const char *)name
{
    return name;
}

/*
 * For the moment assume that names of built-in images will be
 * prefixed with "NX".  Builtin Bitmaps will be available as builtin
 * images with the same name change. 
 *
 * ??? We can rewrite (and clean up) this method once Bitmap is history.
 */
+ _findSystemImageNamed:(const char *)aString
{
    NXRect rect;
    Window *win;

    if ((strlen(aString) > 2) && (!strncmp(aString, "NX", 2)) &&
	([Bitmap _findBuiltInBitmap:aString+2 rect:&rect win:&win] ||
	 [Bitmap _findBuiltInBitmap:aString  rect:&rect win:&win])) {	    
	id imageRep;
    
	self = [[self allocFromZone:SHAREDIMAGEZONE] initSize:&(rect.size)];
	[self setName:aString];
	[self _useCacheGState:win rect:&rect];
	_flags.builtIn = YES;
	imageRep = [self lastRepresentation];
	[imageRep setNumColors:1];
	[imageRep setAlpha:NO];
	[imageRep setBitsPerSample:2];
	[imageRep setPixelsWide:(int)NX_WIDTH(&rect)];
	[imageRep setPixelsHigh:(int)NX_HEIGHT(&rect)];
	return self;
    }
    return nil;
}


+ findImageNamed:(const char *)aString
{
    id image;
    if (((image = [self _imageNamed:aString])) ||
	((image = [self _findSystemImageNamed:aString])) ||
	((image = [self _searchForImageNamed:aString]))) {
	return image;
    } else {
	return nil;
    }
}

static BOOL sectExistsInSegment (const char *seg, const char *sect)
{
    int size;
    return (getsectdata((char *)seg, (char *)sect, &size) && size) ? YES : NO;
}

/* 
 * ??? This method can be made somewhat more efficient; it doesn't need to
 * create and destroy streams to check for the existence of the file.
 */
static BOOL fileExistsInLaunchDir (const char *fileName)
{
    NXStream *stream;
    if (stream = _NXOpenStreamOnSection(NULL, fileName)) {
	NXCloseMemory (stream, NX_FREEBUFFER);
        return YES;
    } else {
	return NO;
    }
}

/*
 * _searchForImageNamed: looks for images in the MachO segments and the
 * app wrapper.
 *
 * Here's the search order given the desired name in aString:
 *
 *   1. Search __ICON for section named aString.
 *   2. Search __TIFF and __EPS for sections named aString.tiff & aString.eps.
 *   3. Search __TIFF and __EPS for sections named aString.
 *   4. Search the launch directory for files named aString.tiff & aString.eps.
 *
 * If multiple things are found in any step, they are treated as different
 * representations. If something is found in a step, search doesn't go on with
 * the rest of the steps. 
 *
 * If the search is successful, the image is named aString.
 */
+ _searchForImageNamed:(const char *)aString
{
    NXImage *image = nil;
    int type = _NXTypeOfFile(aString);

    /*
     * First look in __ICON segment. If nothing there, check out the
     * various segments (__TIFF, __EPS, etc).
     */
    if (sectExistsInSegment (NX_ICONSEGMENT, aString)) {
	NXSize iconSize = {48.0, 48.0};
	if (![[(image = [[self allocFromZone:SHAREDIMAGEZONE] initSize:&iconSize]) setScalable:YES] _useIconNamed:aString from:NULL]) {
	    [image free];
	    image = nil;
	}
    }

    if (!image) {

	if (type == UNKNOWNTYPE) {
	    char epsName[MAXPATHLEN+1], tiffName[MAXPATHLEN+1];

	    strcpy (epsName, aString);
	    strcat (epsName, ".eps");
	    strcpy (tiffName, aString);
	    strcat (tiffName, ".tiff");

	    if (sectExistsInSegment (NX_TIFFSEGMENT, tiffName) ||
		sectExistsInSegment (NX_EPSSEGMENT, epsName) ||
		sectExistsInSegment (NX_TIFFSEGMENT, aString) ||
		sectExistsInSegment (NX_EPSSEGMENT, aString) ||
		fileExistsInLaunchDir (epsName) ||
		fileExistsInLaunchDir (tiffName)) {
		image = [[self allocFromZone:SHAREDIMAGEZONE] 
				initFromSection:aString];
	    }
	} else {
	    const char *seg = (type == TIFFTYPE) ?
					NX_TIFFSEGMENT : NX_EPSSEGMENT;
	    if (sectExistsInSegment (seg, aString) ||
		fileExistsInLaunchDir (aString)) {
		image = [[self allocFromZone:SHAREDIMAGEZONE] 
				initFromSection:aString];
	    }
	}
    }

    if (image) {
	(void)[image setName:aString];
	image->_flags.archiveByName = YES;
    }

    return image;
}

- setBackgroundColor:(NXColor)aColor
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    if (!_color) {
	NX_ZONEMALLOC([self zone], _color, NXColor, 1);
    }
    *_color = aColor;
    return self;
}
	
- (NXColor)backgroundColor
{
    RETURNIFSUBIMAGE([(NXImage *)_reps backgroundColor]);

    return (_color ? *_color : NX_COLORCLEAR);
}

/*
 * Delegates are kept in a hash table; they are going to be rare.
 * (None of the system images will have them, for instance...)
 */
- setDelegate:(id)anObject
{
    RETURNIFSUBIMAGEORBUILTIN(self);

    if (anObject) {
	if (!delegateHashTable) {
	    NXZone *zone = [NXApp zone];
	    if (!zone) zone = NXDefaultMallocZone();
	    delegateHashTable = [[HashTable allocFromZone:zone] initKeyDesc:"@" valueDesc:"@"];
        }
	[delegateHashTable insertKey:(void *)self value:(void *)anObject];
    } else {
	[delegateHashTable removeKey:self];
    }
    return self;	    
}

- delegate
{
    RETURNIFSUBIMAGE([(NXImage *)_reps delegate]);
    return [delegateHashTable valueForKey:self];
}

/*
 * Error handler.
 */
- _handleError:(int)op delta:(float)delta
	fromRect:(const NXRect *)fromRect toPoint:(const NXPoint *)point;
{
    id delegate = [self delegate], errHandler;
    NXRect rect;

    if (fromRect) {
	rect.size = fromRect->size;
    } else {
	[self getSize:&(rect.size)];
    }
    NX_X(&rect) = point->x;
    NX_Y(&rect) = point->y - [NXApp _flipState] ? NX_HEIGHT(&rect) : 0.0;

    if (delegate && [delegate respondsTo:@selector(imageDidNotDraw:inRect:)]) {
	if (errHandler = [delegate imageDidNotDraw:self inRect:&rect]) {
	    if (op == -1) {
		[errHandler dissolve:delta fromRect:fromRect toPoint:point];
	    } else {
		[errHandler composite:op fromRect:fromRect toPoint:point];
	    }	    
	}
    }
    return self;
}

/*
 * This method frees the specified representation and all the
 * data it points to.  The representation is also removed from the
 * list of representations.
 */ 
- _freeRepresentation:(RepresentationInfo *)rep
{
    if (rep == REPRESENTATIONS) {
	REPRESENTATIONS = rep->next;
    } else {
	RepresentationInfo *curRep = REPRESENTATIONS;
	while (curRep && (curRep->next != rep)) {
	    curRep = curRep->next;
	}
	if (curRep) {
	    curRep->next = curRep->next->next;
	}
    }
    
    [self _freeCache:rep];
    [rep->image free];
    NX_FREE(rep);	/* This frees both the fileName & appName. */

    return self;
}

/*
 * free the NXImage and all its representations & caches. Note that any
 * representation that has been added to NXImage (implicitly or explicitly)
 * is considered to belong to NXImage and is freed.
 */
- free
{
    RETURNIFBUILTIN(self);

    if (!_flags.subImage) {
	while (REPRESENTATIONS) {
	    [self _freeRepresentation:REPRESENTATIONS];
	}
	[_repList free];
    } else {
	NX_FREE((NXPoint *)_repList);
    }

    [self setName:NULL];
    [self setDelegate:nil];
    if (_color) {
	NX_FREE(_color);
    }
    return [super free];
}

/*
 * Called with _size containing the new size.
 */
- _resizeAllCaches
{
    RepresentationInfo *rep = REPRESENTATIONS;
    while (rep) {
	BOOL repCanRedraw = YES;
	if (rep->cache && [rep->image isKindOf:[NXCachedImageRep class]]) {
	    Window *window;
	    NXRect rect;
	    [rep->image getWindow:&window andRect:&rect];
	    if (window == WINDOW(rep)) {
		repCanRedraw = NO;
		if ([self isScalable] || !UNIQUE(rep)) {
		    CacheWindowInfo *tmpCache;
		    CacheRect tmpCacheRect;
		    NXRect newRect = {{0.0, 0.0}, _size};
		    _NXAllocateImageCache ([self zone], YES, /* Now unique! */
				    ceil(_size.width), ceil(_size.height),
				    [rep->image bitsPerSample], 
				    [rep->image numColors],
				    [rep->image hasAlpha],
				    [self isCacheDepthBounded] ? NO : YES, 
				    &tmpCache, &tmpCacheRect);
		    NX_X(&newRect) = (NXCoord)tmpCacheRect.x;
		    NX_Y(&newRect) = (NXCoord)tmpCacheRect.y;
		    NX_WIDTH(&newRect) = (NXCoord)tmpCacheRect.w;
		    NX_HEIGHT(&newRect) = (NXCoord)tmpCacheRect.h;
		    [[tmpCache->window contentView] lockFocus];
		    if ([self isScalable]) {	
			[rep->image drawIn:&newRect];
		    } else {
			[rep->image drawAt:&(newRect.origin)];
		    }
		    [[tmpCache->window contentView] unlockFocus];		
		    [rep->image free];
		    _NXFreeImageCache(rep->cache, &(rep->cacheRect));
		    rep->cache = tmpCache;
		    rep->cacheRect = tmpCacheRect;
		    rep->image = [[NXCachedImageRep allocFromZone:[self zone]]
					initFromWindow:tmpCache->window
					rect:&newRect];
		} else {
		    _NXResizeImageCache ([self isUnique],
				    ceil(_size.width), ceil(_size.height),
				    [rep->image bitsPerSample], 
				    [rep->image numColors],
				    [rep->image hasAlpha], NO, 
				    &(rep->cache), &(rep->cacheRect));
		    [rep->image setSize:&_size];
		}
	    }
	}
	if (repCanRedraw) {
	    [self _freeCache:rep];
	    rep->flags.rendered = NO;
	}
	rep = rep->next;
    }
    return self;
}

- _freeCache:(RepresentationInfo *)rep
{
    if (rep->cache) {
	_NXFreeImageCache (rep->cache, &(rep->cacheRect));
	rep->cache = NULL;
    }
    return self;
}

#if 0
- _getCacheGState:(int *)gState andRect:(NXRect *)rect 
	forRep:(RepresentationInfo *)rep
{
    if (rep->cache) {
	*gState = GSTATE(rep);
	NXSetRect(rect, CACHEX(rep), CACHEY(rep), CACHEW(rep), CACHEH(rep));
    } else {
	NX_ASSERT([rep->image isKindOf:[NXCachedImageRep class]],
		  "Trying get cache values of a non-cached rep");
	[rep->image _getCacheGState:gState andRect:rect];
    }
    return self;
}
#endif

- _getCacheWindow:(Window **)window andRect:(NXRect *)rect 
	forRep:(RepresentationInfo *)rep
{
    if (rep->cache) {
	*window = WINDOW(rep);
	NXSetRect(rect, CACHEX(rep), CACHEY(rep), CACHEW(rep), CACHEH(rep));
    } else {
	NX_ASSERT([rep->image isKindOf:[NXCachedImageRep class]],
		  "Trying get cache values of a non-cached rep");
	[rep->image getWindow:window andRect:rect];
	if (!window) {
	    NXLogError ("NXImage given NXCachedImageRep with no Kit window\n");
	}
    }
    return self;
}


/*
 * Assumptions with size of NXImage:
 * Always use getSize: to get the size.
 * Illegal sizes are stored as {0.0, 0.0}.
 * Sizes are not necessarily integers.
 * _flags.sizeWasExplicitlySet indicates that the user called newSize:
 * or setSize: to set the size explicitly.
 */
- setSize:(const NXSize *)aSize
{
    RETURNIFSUBIMAGEORBUILTIN(self);

    if (SIZEOK(aSize)) {
	_size.width = aSize->width;
	_size.height = aSize->height;
	_flags.sizeWasExplicitlySet = YES;
    } else {
	_size.width = _size.height = 0.0;
    }
    [self _resizeAllCaches];
    return self;
}

- getSize:(NXSize *)aSize
{
    /*
     * If the size isn't set see if we can get it from any of the
     * representations.
     */
    if ((!_flags.sizeWasExplicitlySet) && !SIZEOK(&_size) && REPRESENTATIONS) {
	[self _expand];
	[REPRESENTATIONS->image getSize:&_size];
	if (!SIZEOK(&_size)) {	
	    _size.width = _size.height = 0.0;
	} 
    }
    *aSize = _size;
    return self;
}

/*
 * Methods to set/get values of boolean flags.
 */

- setFlipped:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.flipDraw = flag;
    return self;
}

- (BOOL)isFlipped
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isFlipped]);
    return _flags.flipDraw;
}

- setScalable:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.scalable = flag;
    return self;
}

- (BOOL)isScalable
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isScalable]);
    return _flags.scalable;
}

- setDataRetained:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.dataRetained = flag;
    return self;
}

- (BOOL)isDataRetained
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isDataRetained]);
    return _flags.dataRetained;
}

- setUnique:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.uniqueWindow = flag;
    return self;
}

- (BOOL)isUnique
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isUnique]);
    return _flags.uniqueWindow;
}

- setAsynchronous:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.aSynch = flag;
    return self;
}

- (BOOL)isAsynchronous
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isAsynchronous]);
    return _flags.aSynch;
}

- setCacheDepthBounded:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.unboundedCacheDepth = flag ? NO : YES;
    return self;
}

- (BOOL)isCacheDepthBounded
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isCacheDepthBounded]);
    return _flags.unboundedCacheDepth ? NO : YES;
}

- setEPSUsedOnResolutionMismatch:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.useEPSOnResolutionMismatch = flag;
    return self;
}

- (BOOL)isEPSUsedOnResolutionMismatch
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isEPSUsedOnResolutionMismatch]);
    return _flags.useEPSOnResolutionMismatch;
}

- setColorMatchPreferred:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.colorMatchPreferred = flag;
    return self;
}

- (BOOL)isColorMatchPreferred
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isColorMatchPreferred]);
    return _flags.colorMatchPreferred;
}

- setMatchedOnMultipleResolution:(BOOL)flag
{
    RETURNIFSUBIMAGEORBUILTIN(self);
    _flags.multipleResolutionMatching = flag;
    return self;
}

- (BOOL)isMatchedOnMultipleResolution
{
    RETURNIFSUBIMAGE([(NXImage *)_reps isMatchedOnMultipleResolution]);
    return _flags.multipleResolutionMatching;
}

/*
 * The following method is used to let ButtonCell & Cell know if this
 * NXImage has alpha. Sort of a kludge for now.
 */
- (BOOL)_asIconHasAlpha
{
    RepresentationInfo *rep;

    RETURNIFSUBIMAGE([(NXImage *)_reps _asIconHasAlpha]);

    [self _expand];

    rep = REPRESENTATIONS;
    while (rep) {
	if ([rep->image hasAlpha]) {
	    return YES;
	}
	rep = rep->next;
    } 

    return NO;
}

- lastRepresentation
{
    RETURNIFSUBIMAGE([(NXImage *)_reps lastRepresentation]);

    [self _expand];
    return REPRESENTATIONS ? REPRESENTATIONS->image : nil;
}

/*
 * representationList returns a List of all the representations
 * in the image.  The list belongs to NXImage; the caller should not
 * free it.
 */
- (List *)representationList
{
    int cnt = 0;
    RepresentationInfo *rep;

    RETURNIFSUBIMAGE([(NXImage *)_reps representationList]);

    [self _expand];

    [_repList free];
    
    rep = REPRESENTATIONS;

    while (rep) {
	rep = rep->next;
	cnt++;
    }

    _repList = [[List allocFromZone:[self zone]] initCount:cnt];
   
    rep = REPRESENTATIONS;
    while (rep) {
	[_repList addObject:rep->image];
	rep = rep->next;
    }

    return _repList;
}

/*
 * _newRepresentation: provides the only way to add new representations
 * to the image.
 */
- (RepresentationInfo *)_newRepresentation:image
{
    RepresentationInfo *representation;

    NX_ZONEMALLOC([self zone], representation, RepresentationInfo, 1);
    representation->image = image;
    representation->cache = NULL;
    representation->next = REPRESENTATIONS;
    representation->flags.rendered = CACHED(representation);
    representation->flags.drawError = NO;
    representation->flags.from = NOTLAZY;
    representation->fileName = representation->appName = NULL;

    REPRESENTATIONS = representation;

    return representation;     
}

- (RepresentationInfo *)_newLazyRepresentation:(int)from
	:(const char *)fileName :(const char *)appName copy:(BOOL)copy
{
    RepresentationInfo *representation = [self _newRepresentation:nil];

    // Copy the strings into the representation. Once the real image is
    // loaded, the strings will be freed.

    representation->fileName = (char *) (copy ? 
    	NXCopyStringBufferFromZone(fileName, [self zone]) : fileName);
    representation->appName = (char *) ((copy && appName) ? 
    	NXCopyStringBufferFromZone(appName, [self zone]) : appName);
    representation->flags.from = from;

    return representation;     
}

/*
 * _focusOnCache is called when we need to do some
 * on-screen rendering for the specified representation.
 * If no cache has been created for this representation of this
 * image, this method will create it.  If the size of the image is
 * not set or is zero, then this method will simply return NO.
 *
 * This method assumes that the representation is expanded!
 */
- (BOOL)_focusOnCache:(RepresentationInfo *)rep
{
    NXSize size;
    CacheWindow window;
    NXRect cacheRect;
    View *tmpView;

    [self getSize:&size];

    if (!SIZEOK(&size)) {
	return NO;
    }

    if (!CACHED(rep)) {    
	_NXAllocateImageCache ([self zone], [self isUnique],
			ceil(size.width), ceil(size.height),
			[rep->image bitsPerSample], 
			[rep->image numColors],
			[rep->image hasAlpha],
			[self isCacheDepthBounded] ? NO : YES,
			&(rep->cache), &(rep->cacheRect));
    }

    [self _getCacheWindow:&window andRect:&cacheRect forRep:rep];
    tmpView = getTempFocusView();
    [tmpView setFrame:&cacheRect];
    [[window contentView] addSubview:tmpView];
    if ([NXApp focusView] == nil) {	/* Kludge! View doesn't do this. */
	PSgsave();
    }
    [tmpView lockFocus];
    if (!rep->cache || !UNIQUE(rep)) {
	PSrectclip (0.0, 0.0, // NX_X(&cacheRect), NX_Y(&cacheRect),
		    NX_WIDTH(&cacheRect), NX_HEIGHT(&cacheRect));
    }
    [tmpView setFlipped:[self isFlipped]];
    return YES;
}

- (BOOL)drawRepresentation:(NXImageRep *)imageRep inRect:(const NXRect *)rect
{
    BOOL status = NO, fillsTheRect;
    NXColor backColor = [self backgroundColor];
    NXSize size;

    [self getSize:&size];
    fillsTheRect = [self isScalable] ||
	((NX_WIDTH(rect) == size.width) && (NX_HEIGHT(rect) == size.height));

    if ((NXDrawingStatus == NX_DRAWING &&
	(![imageRep isKindOf:[NXBitmapImageRep class]] || !fillsTheRect)) ||
	(NXDrawingStatus != NX_DRAWING && NXAlphaComponent(backColor) != 0.0)){ 
	rectFillWithColor (rect, backColor);
    }

#ifndef DISABLEASYNCHIMAGING
    if (NXDrawingStatus == NX_DRAWING && [self isAsynchronous] && [imageRep _asynchOK]) {
	int tag;
	if ([self isScalable]) {
	    tag = [imageRep _drawAsynchIn:rect]; 
	} else {
	    tag = [imageRep _drawAsynchAt:&(rect->origin)];
	}
        if (tag) {
	    rep->tag = (tag == -1) ? 0 : tag;
	    status = YES;
	}
    }
#endif

    if (!status) {
	if ([self isScalable]) {
	    status = [imageRep drawIn:rect]; 
	} else {
	    status = [imageRep drawAt:&(rect->origin)];
	} 
    }
    return status;
}

- (BOOL)_printRepresentation:(RepresentationInfo *)rep op:(int)op
{
    NXRect drawRect = {{0.0, 0.0}, {0.0, 0.0}};
    NXSize size, imageSize;
    BOOL status;

    /*
     * Make sure the representation has a size.
     */
    [self getSize:&imageSize];
    drawRect.size = imageSize;

    [rep->image getSize:&size];
    if (!SIZEOK(&size)) {
	[rep->image setSize:&(drawRect.size)];
    }

    if ([self isFlipped] && ![rep->image isKindOf:[NXCachedImageRep class]]) {
	PStranslate (0.0, imageSize.height);
	PSscale (1.0, -1.0);
    }

    /*
     * Well, draw the thing!
     */
    status = [self drawRepresentation:rep->image inRect:&drawRect];
    if (!status || ![self isDataRetained]) {
	[self _forgetData:rep];
    }
    if (!status) {
	rep->flags.drawError = YES;
    }
    return status;
}

/*
 * _drawRepresentation draws the specified representation in the currently
 * locked focus.  
 *
 * This method should not be called if the representation is rendered.
 */
- (BOOL)_drawRepresentation:(RepresentationInfo *)rep
{
    NXRect drawRect = {{0.0, 0.0}, {0.0, 0.0}};
    NXSize size;
    BOOL status;

    /*
     * Make sure the representation has a size.
     */
    [self getSize:&(drawRect.size)];
    [rep->image getSize:&size];
    if (!SIZEOK(&size)) {
	[rep->image setSize:&(drawRect.size)];
    }
	
    status = [self drawRepresentation:rep->image inRect:&drawRect];
    [self _drawDone:rep success:status];

    return status;
}

- _drawDone:(RepresentationInfo *)rep success:(BOOL)success
{
    if (!success || ![self isDataRetained]) {
	[self _forgetData:rep];
    }
    if (!success) {
	rep->flags.drawError = YES;
    }
#ifndef DISABLEASYNCHIMAGING
    rep->tag = 0;
#endif
    return self;
}

- (BOOL)_cacheRepresentation:(RepresentationInfo *)rep
{
    return [self _cacheRepresentation:rep stayFocused:NO];
}

/*
 * This method will draw the specified representation in the cache.
 * If the stayFocused flag is YES, then this method will return with the
 * focus on the cache; the caller must eventually call PSgrestore().
 * If this method returns NO, the focusing was unsuccesful; the caller need
 * not worry about restoring any state.
 */
- (BOOL)_cacheRepresentation:(RepresentationInfo *)rep stayFocused:(BOOL)flag
{
    BOOL status;

    if ((status = [self _focusOnCache:rep]) && (!rep->flags.rendered) && 
	(!_flags.builtIn)) {
	if (flag) {
	    PSgsave ();
	}
	status = rep->flags.rendered = [self _drawRepresentation:rep];
	if (flag) {
	    PSgrestore ();
	}
    }
    if (!status || !flag) {
	[self unlockFocus];
    }

    return status;
}

/*
 * This method assures that the data for a representation is flushed out of 
 * memory.  If the representation is lazy, then its no problem; we just go 
 * ahead and tell the representation to forget its data.  However, if the
 * representation is not lazy, then it will not be able to reload the data,
 * so we turn it into an instance of NXCachedImageRep.
 *
 * If an NXImageRep doesn't respond to _forgetData, then we assume it 
 * doesn't deal with data. 
 */
- _forgetData:(RepresentationInfo *)rep
{
    if (REPLAZY(rep)) {
	if ([rep->image respondsTo:@selector(_forgetData)]) {
	    [rep->image _forgetData];
	}
    } else if (rep->cache && [rep->image respondsTo:@selector(_forgetData)]) {
	NXRect rect = {{CACHEX(rep), CACHEY(rep)}, {CACHEW(rep), CACHEH(rep)}};
	[rep->image free];
	rep->image = [[NXCachedImageRep allocFromZone:[self zone]] 
				initFromWindow:WINDOW(rep) rect:&rect];
    }
    return self;		
}

- (BOOL)loadFromStream:(NXStream *)stream
{
    int streamType = _NXTypeOfStream(stream);
    id image;

    RETURNIFSUBIMAGEORBUILTIN(NO);

    switch (streamType) {
	case EPSTYPE:
	    if (image = [[NXEPSImageRep allocFromZone:[self zone]] initFromStream:stream]) {
		[self _newRepresentation:image];
		return YES;
	    }
	    break;
	case TIFFTYPE:
	    if (image = [NXBitmapImageRep newListFromStream:stream
						       zone:[self zone]]) {
		int cnt = [image count];
		while (cnt--) {
		    [self _newRepresentation:[image objectAt:cnt]];
		}
		[image free];
		return YES;
	    }
	    break;

	default:
	    break;
    }

    return NO;
}

static BOOL expandFrom (NXImage *self, id repClass, RepresentationInfo *rep,
			  const char *segName,
			  const char *aString, const char *extension,
			  NXZone *zone)
{
    char fileName[MAXPATHLEN+1];
	
    strcpy (fileName, aString);
    strcat (fileName, extension);

    if ((segName && sectExistsInSegment(segName, fileName)) ||
	(!segName && fileExistsInLaunchDir(fileName))) {
	return [self _addRepsFrom:[repClass newListFromSection:fileName zone:zone] 
			    toRep:rep];
    } else {
	return NO;
    }
}

/*
 * This method will take a lazy representation an expand it.
 * Lazy representations have an image field of nil and a lazyInfo field
 * pointing to a block containing the file name & such. After this method
 * is invoked the representation's image field will be non-nil. If, as
 * a result of the expansion, more than one new representation gets generated,
 * the new representations get added to the list with a lazyInfo field
 * of (-1) to indicate that they are just dummies and shouldn't be written
 * out.
 */
- (BOOL)_expandRep:(RepresentationInfo *)rep
{
    const char *fName = FILENAME(rep);
    int from = rep->flags.from;
    int fileType = _NXTypeOfFile(fName);
    NXZone *zone = [self zone];

    if (from == LAZYFROMSECTION) {
	if (fileType == UNKNOWNTYPE) {
	    /* First look in __TIFF & __EPS for fName.tiff & fName.eps */
	    BOOL tiffOK = expandFrom (self, [NXBitmapImageRep class], rep,
					NX_TIFFSEGMENT, fName, ".tiff", zone);
	    BOOL epsOK = expandFrom (self, [NXEPSImageRep class], rep,
					NX_EPSSEGMENT, fName, ".eps", zone);
	    /* Failing that, look in the same place for fName */
	    if (!(tiffOK || epsOK)) {
		tiffOK = expandFrom (self, [NXBitmapImageRep class], rep,
					NX_TIFFSEGMENT, fName, "", zone);
		epsOK = expandFrom (self, [NXEPSImageRep class], rep,
					NX_EPSSEGMENT, fName, "", zone);
	    }
	    /* Hmm. Look in launch dir for fName.tiff & fName.eps */
	    if (!(tiffOK || epsOK)) {
		tiffOK = expandFrom (self, [NXBitmapImageRep class], rep,
					NULL, fName, ".tiff", zone);
		epsOK = expandFrom (self, [NXEPSImageRep class], rep,
					NULL, fName, ".eps", zone);
	    }
	    return tiffOK || epsOK;
	} else { /* We know the file type; don't need to search as much... */
	    char *seg = (fileType == TIFFTYPE) ?
			NX_TIFFSEGMENT : NX_EPSSEGMENT;
	    id class = (fileType == TIFFTYPE) ?
			[NXBitmapImageRep class] : [NXEPSImageRep class];
	    /* Look in appropriate segment for section named fName */
	    if (!expandFrom (self, class, rep, seg, fName, "", zone)) {
		/* Look in launch dir for file named fName */
		return expandFrom (self, class, rep, NULL, fName, "", zone);
	    } else {
		return YES;
	    }
	}
    } else if (from == LAZYFROMICON) {
	
	const char *appName = APPNAME(rep);
	id imageList = [NXBitmapImageRep _newListFromIcon:fName
					inApp:appName zone:[self zone]];
	if (!imageList) {
	    imageList = [NXEPSImageRep _newListFromIcon:fName
					inApp:appName zone:[self zone]];
	}
	return [self _addRepsFrom:imageList toRep:rep];

    } else if (from == LAZYFROMFILE) {

	id imageList = nil;
	if (fileType == TIFFTYPE || fileType == UNKNOWNTYPE) {
	    imageList = [NXBitmapImageRep newListFromFile:fName
						     zone:[self zone]];
	}
	if (fileType == EPSTYPE || (fileType == UNKNOWNTYPE && !imageList)) {
	    imageList = [NXEPSImageRep newListFromFile:fName
						  zone:[self zone]];
	}
	return [self _addRepsFrom:imageList toRep:rep];

    }
#ifdef DEBUG
	else NXLogError ("NXImage: Unknown from value if _expandRep:\n");
#endif

    return NO;
}

/*
 * This method will add all the representations in the list to the NXImage.
 * The representations are assumed to be generated from the expansion of the
 * lazy representation specified in rep.
 */
-(BOOL)_addRepsFrom:imageList toRep:(RepresentationInfo *)rep
{
    if (!imageList) {
	return NO;
    } else {
	int cnt = [imageList count];
	while (cnt--) {
	    id image = [imageList objectAt:cnt];
	    if (rep->image == nil) {
		rep->image = image;
		/* Free the strings; now we get them from the rep... */
		free(rep->fileName);
		if (rep->appName) free(rep->appName);
		rep->fileName = rep->appName = NULL;
	    } else {
		RepresentationInfo *nRep = [self _newRepresentation:image];
		MAKEREPDUMMY(nRep);
	    }
	}
	[imageList free];
	return YES;
    }
}


/*
 * Truly lazy stuff.
 */
- (BOOL)_useFromFile:(const char *)fileName inSection:(BOOL)inSection
{
    [self _newLazyRepresentation:
	(inSection ? LAZYFROMSECTION : LAZYFROMFILE) :fileName :NULL copy:YES];
    [self _setNeedsToExpand:YES];
    return YES;
}

- (BOOL)_useIconNamed:(const char *)iconName from:(const char *)appName
{
    if (![self _sizeSet]) {
	NXSize iconSize = {48.0, 48.0};
	[self setSize:&iconSize];
	[self setScalable:YES];
    }
    [self _newLazyRepresentation:LAZYFROMICON  :iconName :appName copy:YES];
    [self _setNeedsToExpand:YES];
    return YES;
}

- (BOOL)useFromFile:(const char *)fileName
{
    RETURNIFSUBIMAGEORBUILTIN(NO);
    return [self _useFromFile:fileName inSection:NO];
}

- (BOOL)useFromSection:(const char *)fileName
{
    RETURNIFSUBIMAGEORBUILTIN(NO);
    return [self _useFromFile:fileName inSection:YES];
}

- (BOOL)_useCacheGState:(Window *)window rect:(const NXRect *)rect
{
    id image = [NXCachedImageRep newFromWindow:window rect:rect];
    (void)[self _newRepresentation:image];
    return YES;
}

- (BOOL)useCacheWithDepth:(NXWindowDepthType)depth
{
    CacheWindowInfo *cache;
    CacheRect cacheRect;
    NXSize size;
    int bps, numColors;
    NXWindowDepth dDepth = (depth != NX_DefaultDepth) ? depth :
						[Window defaultDepthLimit];

    RETURNIFSUBIMAGEORBUILTIN(NO);

    [self getSize:&size];

    if (!SIZEOK(&size)) {
	return NO;
    }

    bps = NXBPSFromDepth(dDepth);
    numColors = NXNumberOfColorComponents(NXColorSpaceFromDepth(dDepth));
   
    if (_NXAllocateImageCache ([self zone], [self isUnique],
			ceil(size.width), ceil(size.height),
			bps, numColors, YES, (depth != NX_DefaultDepth),
			&cache, &cacheRect)) {
	NXRect rect = {{cacheRect.x, cacheRect.y}, {cacheRect.w, cacheRect.h}};
	id image = [NXCachedImageRep newFromWindow:cache->window rect:&rect];
	RepresentationInfo *rep = [self _newRepresentation:image];

	/* Are these necessary? */
	[image setNumColors:numColors];
	[image setBitsPerSample:bps];
	[image setAlpha:YES];

	rep->cache = cache;
	rep->cacheRect = cacheRect;

	return YES;
    }
    return NO;
}

- _subImageFocus
{
    NXRect subImageRect = {*(NXPoint *)_repList, _size};
    NXRectClip (&subImageRect);
    PStranslate (NX_X(&subImageRect), NX_Y(&subImageRect));
    return self;
}

/*
 * lockFocusOn: locks focus on the specified representation.
 * If representation is NULL, then the one best suitable for the best 
 * frame buffer on the system is used.
 */
- (BOOL)lockFocusOn:imageRepresentation
{
    RepresentationInfo *rep;

    if (_flags.subImage) {
	if ([(NXImage *)_reps lockFocusOn:imageRepresentation]) {
	    [self _subImageFocus];
	    return YES;
	}
        return NO;
    }

    [self _expand];
    if (imageRepresentation) {
	rep = REPRESENTATIONS;
	while (rep && (rep->image != imageRepresentation)) {
	    rep = rep->next;
	}
    } else {
	/*
	 * If no representation was provided, use the one that matches
	 * the best frame buffer we have...
	 */
	rep = [self _bestRepresentation:YES checkFlag:NO];
    }

    if (rep) {
	return [self _lockFocusOnRep:rep];
    } else {
	return NO;
    }
}

/*
 * lockFocus locks focus on the representation best suitable for the best 
 * frame buffer on the system.  If there are no representations, then
 * lockFocus creates a cache with default depth.
 */
- (BOOL)lockFocus
{
    RepresentationInfo *rep = NULL;

    if (_flags.subImage) {
	if ([(NXImage *)_reps lockFocus]) {
	    [self _subImageFocus];
	    return YES;
	}
        return NO;
    }

    [self _expand];
    if (REPRESENTATIONS) {
	rep = [self _bestRepresentation:YES checkFlag:NO];
    } else if ([self useCacheWithDepth:NX_DefaultDepth]) {
	rep = REPRESENTATIONS;
    } 

    return rep ? [self _lockFocusOnRep:rep] : NO;
} 

- (BOOL)_lockFocusOnRep:(RepresentationInfo *)rep
{
    if ([self _cacheRepresentation:rep stayFocused:YES]) {
	return YES;
     } else {
	return NO;
     }
}

- unlockFocus
{
    id tmpView = [NXApp focusView];

    [tmpView unlockFocus];
    [tmpView removeFromSuperview];
    freeTempFocusView(tmpView);

    if ([NXApp focusView] == nil) {	/* Kludge! View doesn't do this. */
	PSgrestore();
    }
    return self;
}

- (BOOL)useDrawMethod:(SEL)aSelector inObject:anObject
{
    id image;

    RETURNIFSUBIMAGEORBUILTIN(NO);

    if (image = [[NXCustomImageRep allocFromZone:[self zone]]
    			initDrawMethod:aSelector inObject:anObject]) {
	(void)[self _newRepresentation:image];
	return YES;
    } else {
	[image free];
	return NO;
    }
}

- (BOOL)useRepresentation:(NXImageRep *)imageRepresentation
{
    RETURNIFSUBIMAGEORBUILTIN(NO);
    (void)[self _newRepresentation:imageRepresentation];
    return YES;
}

- removeRepresentation:imageRepresentation
{
    RepresentationInfo *rep;

    RETURNIFSUBIMAGEORBUILTIN(self);

    [self _expand];
    rep = REPRESENTATIONS;
    while (rep && (rep->image != imageRepresentation)) {
	rep = rep->next;
    }
    if (rep) {
	[self _freeRepresentation:rep];
    }
    return self;
}

- read:(NXTypedStream *)stream
{
    [super read:self];

    NXReadTypes (stream, "s*", &_flags, &name);

    if (_flags.archiveByName || _flags.builtIn) {
	return self;
    }
    if (_flags.sizeWasExplicitlySet) {
	NXReadSize (stream, &_size);
    }
    if (_flags.subImage) {
	NXPoint origin;
	NXReadPoint (stream, &origin);
	(NXImage *)_reps = NXReadObject (stream);
	NX_ZONEMALLOC([self zone], (NXPoint *)_repList, NXPoint, 1);
	*(NXPoint *)_repList = origin;
    } else {
	short cnt, numReps;
	NXReadType (stream, "s", &numReps);
	for (cnt = 0; cnt < numReps; cnt++) {
	    unsigned char repType;
	    char *fileName, *appName = NULL;
	    NXReadType (stream, "c", &repType);
	    switch (repType) {

		case NOTLAZY:
		    [self _newRepresentation:NXReadObject(stream)];
		    break;

		case LAZYFROMFILE:
		case LAZYFROMSECTION:
		case LAZYFROMICON:
		    NXReadType (stream, "*", &fileName);
		    if (repType == LAZYFROMICON) {
			NXReadType (stream, "*", &appName);
		    }
		    [self _newLazyRepresentation:(int)repType
						:fileName :appName copy:NO];
		    [self _setNeedsToExpand:YES];
		    break;

		default:
		    NX_RAISE(NX_newerTypedStream, 0, 0);
	    }
	}

	/* Finally read the color & delegate */
	if (NXTypedStreamClassVersion(stream, "NXImage") > 1) {
	    NXColor tmpColor;
	    id tmpDelegate;
	    if (_NXIsValidColor(tmpColor = NXReadColor(stream))) {
		[self setBackgroundColor:tmpColor];
	    }
	    if (tmpDelegate = NXReadObject(stream)) {
		[self setDelegate:tmpDelegate];
	    }
	}
	 
    }
    return self;
}

- write:(NXTypedStream *)stream
{
    [super write:self];

    NXWriteTypes (stream, "s*", &_flags, &name);

    if (_flags.archiveByName || _flags.builtIn) {
	return self;
    }
    if (_flags.sizeWasExplicitlySet) {
	NXWriteSize (stream, &_size);
    }
    if (_flags.subImage) {
	NXWritePoint (stream, (NXPoint *)_repList);
	NXWriteObject (stream, (NXImage *)_reps); 
    } else { 
	RepresentationInfo *rep;
	short numReps = 0;
	/*
	 * First figure out how many representations there are.
	 */
	rep = REPRESENTATIONS;
	while (rep) {
	    if ((REPLAZY(rep) && !REPDUMMY(rep)) ||
		(!REPLAZY(rep) && rep->image)) {
	        numReps++;
	    }
	    rep = rep->next;
	}
	NXWriteType (stream, "s", &numReps);
	/*
	 * Write them out one by one; each prefixed with a type byte.
	 */
	rep = REPRESENTATIONS;
	while (rep) {
	    if (REPLAZY(rep) && !(REPDUMMY(rep))) {
		const char *fileName = FILENAME(rep);
		const char *appName = APPNAME(rep);
		char from = (char)rep->flags.from;
		NXWriteType (stream, "c", &from);
		NXWriteType (stream, "*", &fileName);
		if (from == LAZYFROMICON) {
		    NXWriteType (stream, "*", &appName);
		}
	    } else if (rep->image && (!(REPLAZY(rep)))) {
		unsigned char repType = NOTLAZY;
		NXWriteType (stream, "c", &repType);
		NXWriteObject (stream, rep->image); 
	    }
	    rep = rep->next;
	}
	/* Finally write out the color & delegate */
	NXWriteColor (stream, _color ? *_color : _NXNoColor()); 	
	NXWriteObjectReference(stream, [self delegate]);
    }
    return self;
}

- finishUnarchiving
{
    NXImage *actualImage;

    if (!name) {
	return nil;
    }

    if ((actualImage = [[self class] _imageNamed:name]) ||
	(_flags.builtIn &&
	 (actualImage = [[self class] _findSystemImageNamed:name])) ||
	(_flags.archiveByName &&
	 (actualImage = [[self class] findImageNamed:name]))) {

	/* Free method tries to unregister the name, so free it first. */
	free (name);
	name = NULL;
	_flags.builtIn = 0;
        [self free];
        return actualImage;

    } else {

	/* The name wasn't found, so register it by brute force. */
	char *oName = name;
	name = NULL;
	[self setName:oName];
	free (oName);
	return nil;

    }
}

/*
 * This function returns the global window number and the rect in which
 * the specified representation is cached. If imageRepresentation is NULL,
 * then this function uses the best representation for all the frame buffers
 * on the system. NO is returned if the representation cannot be cached for
 * some reason.
 *
 * Mostly for use by Workspace.
 */
- (BOOL)_getGlobalWindowNumber:(unsigned int *)winNumber
	andRect:(NXRect *)rect 
	forRepresentation:(NXImageRep *)imageRepresentation
{
    if ([self lockFocusOn:imageRepresentation]) {
	[self getSize:&(rect->size)];
	_NXGetWinAndOrigin (winNumber, &NX_X(rect), &NX_Y(rect));
	if ([self isFlipped]) {
	    NX_Y(rect) -= NX_HEIGHT(rect);
	}
	[self unlockFocus];
	return YES;
    } else {
	return NO;
    }
}

- writeTIFF:(NXStream *)stream
{
    return [self writeTIFF:stream allRepresentations:NO];
}

- writeTIFF:(NXStream *)stream allRepresentations:(BOOL)all
{
    RepresentationInfo *bestRep;

    RETURNIFSUBIMAGE([(NXImage *)_reps writeTIFF:stream allRepresentations:all]);

    [self _expand];

    clearChecked(REPRESENTATIONS);

    while (bestRep = [self _bestRepresentation:YES checkFlag:YES]) {
	bestRep->flags.checked = YES;
	if ([bestRep->image isKindOf:[NXBitmapImageRep class]]) {
	    (void)[bestRep->image data];	/* Make sure the data is in */
	    [bestRep->image writeTIFF:stream];	/* This might raise error */
	} else {
	    id bitmap;
	    Window *window;
	    NXRect cacheRect;
	    BOOL wasNotCached = NO;
	    if ((!CACHED(bestRep)) || (!bestRep->flags.rendered)) {
		wasNotCached = YES;
		if (![self _cacheRepresentation:bestRep]) {
		    continue;
		}
		bestRep->flags.rendered = NO;
	    }
	    [self _getCacheWindow:&window andRect:&cacheRect forRep:bestRep];
	    PSgsave ();
	    NXSetGState ([window gState]);
	    bitmap = [NXBitmapImageRep readImage:&cacheRect into:NULL];
	    PSgrestore ();
	    if (wasNotCached) {
		[self _freeCache:bestRep];
	    }
	    if (!bitmap) {
		continue;
	    }
	    NX_DURING
		[bitmap writeTIFF:stream];
	    NX_HANDLER
		[bitmap free];
		NX_RERAISE();
	    NX_ENDHANDLER
	    [bitmap free];
	}
	if (!all) {
	    break;
	}
    }
    return self;
}

/* Call with res & oRes containing valid res values (not -1 or MAXRES) */
#define RESMATCH(res, oRes) \
  ((res==oRes) || ([self isMatchedOnMultipleResolution] && ((res % oRes)==0)))
#define BPSBITS		12
#define RESBITS		16
#define COLORBITS	2
#define MAXBPS		((1<<BPSBITS)-1)
#define MAXRES		((1<<RESBITS)-1)
#define MAXCOLOR	((1<<COLORBITS)-1)
#define POINTSPERINCH	72

/*
 * This huge method finds the bestRepresentation for the current output
 * device. Some funkiness: If the output device is a printer, we just assume
 * it's color.  For the display, we assume the resolution is 72dpi.  This
 * should be remedied with PDF files.
 *
 * If allScreens is YES, this function looks for the best representation for
 * the deepest frame buffer available on the system.
 *
 * If checkFlag is YES, then this method looks at the value of checkFlag
 * and rejects a rep if the flag is set. Call clearChecked() to reset these
 * flags.
 */
- (RepresentationInfo *)_bestRepresentation:(BOOL)allScreens
				  checkFlag:(BOOL)checkFlag
{
    RepresentationInfo *bestRep = NULL, *rep = REPRESENTATIONS;
    unsigned int bestMatchFactor = 0;
    int deviceBPS, deviceRes, deviceHasColor;
    int colorShift, resShift;

    /* Take care of the trivial cases: No reps, or only one rep. */

    if (!rep) {
	return NULL;
    }
    
    if (!rep->next) {
	return (rep->flags.drawError || (checkFlag && rep->flags.checked)) ? 
		NULL : rep;
    }

    /* Determine the shift values */
    colorShift = BPSBITS + ([self isColorMatchPreferred] ? RESBITS : 0);
    resShift = BPSBITS + ([self isColorMatchPreferred] ? 0 : COLORBITS);

    if (NXDrawingStatus == NX_PRINTING) {
	deviceBPS = NX_MATCHESDEVICE;
	deviceRes = MAXRES;
	deviceHasColor = YES;
    } else {
	Window *win;
	NXWindowDepthType depth = NX_DefaultDepth;
	if (!allScreens) {
	    if (win = [[NXApp focusView] window]) {
		depth = [win depthLimit]; 
	    }
	}
	if (depth == NX_DefaultDepth) {
	    depth = [Window defaultDepthLimit];
	}
	deviceBPS = NXBPSFromDepth(depth);
	deviceHasColor = NXNumberOfColorComponents
				(NXColorSpaceFromDepth (depth)) > 1;
	deviceRes = 72;	/* ??? Hardwired! */
    }

    /*
     * Now that we've determined the device characteristics, go through
     * the representations.  To find the best representation, we just assign
     * each representation a "device match factor" (which changes depending
     * on the various image display preferences) and choose the rep with the
     * best match.
     */
    while (rep) {

	if (rep->image && !rep->flags.drawError && 
	    (!checkFlag || !rep->flags.checked)) {

	    /* Variables for representation info */
	    int pixelsWide = [rep->image pixelsWide];
	    int pixelsHigh = [rep->image pixelsHigh];
	    int numColors = [rep->image numColors];
	    int bitsPerSample = [rep->image bitsPerSample];
	    int xRes = 0, yRes = 0;

	    /* Variables used in computing match factor for a rep */
	    unsigned int res = 0, color = 0, bps = 0, matchFactor;
    
	    /* Determine the representation resolution */
    
	    if (pixelsWide == NX_MATCHESDEVICE ||
		pixelsHigh == NX_MATCHESDEVICE) {
		xRes = yRes = MAXRES;
	    } else if (pixelsWide > 0 || pixelsHigh > 0) {
		NXSize repSize = {0.0, 0.0};
		if (![self isScalable]) {
		    [rep->image getSize:&repSize];
		}
		if (!SIZEOK(&repSize)) {
		    [self getSize:&repSize];
		}
		if (SIZEOK(&repSize)) {
		    xRes = (pixelsWide*POINTSPERINCH) / (int)(repSize.width);
		    yRes = (pixelsHigh*POINTSPERINCH) / (int)(repSize.height);
		}
	    }
    
	    /* Determine the resolution match, 0..MAXRES */
    
	    if (deviceRes == MAXRES) {
		res = MAX((xRes + yRes), MAXRES);
	    } else if (xRes == MAXRES) {
		res = [self isEPSUsedOnResolutionMismatch] ?
						MAXRES - 2 : deviceRes;
	    } else if (RESMATCH(xRes,deviceRes) && RESMATCH(yRes,deviceRes)) {
		res = MAXRES - 1;
	    } else {
		res = MAX(xRes + yRes, MAXRES - 3);
	    }
    
	    /* Color match, 0..MAXCOLOR */
    
	    if (numColors == NX_MATCHESDEVICE) {
		color = MAXCOLOR;
	    } else if (numColors > 1) {
		color = deviceHasColor ? MAXCOLOR : MAXCOLOR-1;
	    } else if (numColors == 1) {
		color = deviceHasColor ? MAXCOLOR-1 : MAXCOLOR;
	    }
    
	    /* Finally bits per sample */

	    if (bitsPerSample==NX_MATCHESDEVICE || bitsPerSample==deviceBPS) {
		bps = MAXBPS;
	    } else {
		bps = MAX(MAXBPS-1, bitsPerSample);
	    }
    

	    matchFactor = bps + (color << colorShift) + (res << resShift);
    
	    if (matchFactor > bestMatchFactor) {
		bestRep = rep;
		bestMatchFactor = matchFactor;
	    }
	}
	rep = rep->next;
    }	   

    return bestRep;        
}

- (NXImageRep *)bestRepresentation
{
    RepresentationInfo *rep;

    RETURNIFSUBIMAGE([(NXImage *)_reps bestRepresentation]);

    [self _expand];
    rep = [self _bestRepresentation:YES checkFlag:NO];
    return rep ? rep->image : NULL;
}

- composite:(int)op toPoint:(const NXPoint *)point
{
    return [self composite:op fromRect:NULL toPoint:point];
}

- dissolve:(float)delta toPoint:(const NXPoint *)point
{
    return [self dissolve:delta fromRect:NULL toPoint:point];
}

/*
 * These next two are the overridable methods.
 */
- composite:(int)op 
   fromRect:(const NXRect *)rect toPoint:(const NXPoint *)point;
{
    return [self _composite:op delta:0.0 fromRect:rect toPoint:point] ? self : nil;
}

- dissolve:(float)delta 
   fromRect:(const NXRect *)rect toPoint:(const NXPoint *)point;
{
    return [self _composite:(-1) delta:delta fromRect:rect toPoint:point] ? self : nil;
}

/*
 * All requests to draw the image go through this method.
 * Pass in op of (-1) for dissolve operation; then you also should provide 
 * delta (0..1). If rect is NULL, the whole image is assumed.
 */
- (BOOL)_composite:(int)op delta:(float)delta
   fromRect:(const NXRect *)fromRect toPoint:(const NXPoint *)point;
{
    RepresentationInfo *bestRep;
    NXRect rect = {{0.0, 0.0}, {0.0, 0.0}};

    if (_flags.subImage) {
	NXRect imageRect;
	[self getImage:NULL rect:&imageRect];
	if (fromRect) {
	    NX_X(&rect) = NX_X(fromRect) + NX_X(&imageRect);
	    NX_Y(&rect) = NX_Y(fromRect) + NX_Y(&imageRect);
	    rect.size = fromRect->size;
	    imageRect = rect;
	}
	return [(NXImage *)_reps _composite:op delta:delta
		fromRect:&imageRect toPoint:point];
    }

    [self _expand];
    [self getSize:&(rect.size)];

    if (fromRect) {
	rect = *fromRect;
    }

    if (!SIZEOK(&(rect.size))) {
	return NO;
    }
    
    if (NXDrawingStatus == NX_DRAWING) {

	NXRect cacheRect;
	Window *window;

	while (bestRep = [self _bestRepresentation:NO checkFlag:NO]) {
#ifndef DISABLEASYNCHIMAGING
	    if (bestRep->tag) {
		int status;
		if ((status = [NXImageRep _drawDone:bestRep->tag]) != ASYNCHNOTDONE) {
		    [self _drawDone:bestRep success:(status == ASYNCHDRAWOK)];
		}
		break;
	    } else
#endif
	    if ((!CACHED(bestRep)) || (!bestRep->flags.rendered)) {
		if ([self _cacheRepresentation:bestRep]) break;
		bestRep->flags.drawError = YES;
	    } else {
		break;
	    }
	}

	if (!bestRep) {
	    [self _handleError:op delta:delta
		  fromRect:fromRect toPoint:point];
	    return NO;
	}

	[self _getCacheWindow:&window andRect:&cacheRect forRep:bestRep];

	if (op == -1) {	/* dissolve */

	    PSdissolve (NX_X(&cacheRect) + NX_X(&rect),
			NX_Y(&cacheRect) + NX_Y(&rect),
			NX_WIDTH(&rect), NX_HEIGHT(&rect),
			[window gState], point->x, point->y,
			delta);
	
	} else { /* composite */

	    PScomposite(NX_X(&cacheRect) + NX_X(&rect),
			NX_Y(&cacheRect) + NX_Y(&rect),
			NX_WIDTH(&rect), NX_HEIGHT(&rect),
			[window gState], point->x, point->y,
			op);
	}

    } else {

	NXSize size, repSize;
	[self getSize:&size];

#ifdef DEBUG
        DPSPrintf (DPSGetCurrentContext(),
                    "\n%% NXImage %x %s, size %.1f x %.1f\n",
                    self, name ? name : "<noname>", size.width, size.height);
#endif

	DPSPrintf (DPSGetCurrentContext(),
		   "%f %f transform\n", point->x, point->y);

	while (bestRep = [self _bestRepresentation:NO checkFlag:NO]) {
	    [bestRep->image getSize:&repSize];
#ifdef DEBUG
	    DPSPrintf (DPSGetCurrentContext(),
		"%%  Representation %s, size %.1f x %.1f\n",
		[bestRep->image name], repSize.width, repSize.height);
#endif
	    DPSPrintf (DPSGetCurrentContext(),
		"gsave __NXbasematrix setmatrix itransform translate\n");
	    if ((NX_X(&rect) != 0.0) || (NX_Y(&rect) != 0.0)) {
		PStranslate (-NX_X(&rect), -NX_Y(&rect));
	    }
	    if ((NX_X(&rect) != 0.0) || (NX_Y(&rect) != 0.0) ||
		(NX_WIDTH(&rect) < repSize.width) ||
		(NX_HEIGHT(&rect) < repSize.height) ||
		[bestRep->image _canDrawOutsideOfItsBounds]) {
		NXRectClip (&rect);
	    }	    
	    if ([self _printRepresentation:bestRep op:op]) {
		DPSPrintf (DPSGetCurrentContext(), "grestore\n");
		break;
	    }	
	    DPSPrintf (DPSGetCurrentContext(), "grestore\n");
	}

#ifdef DEBUG
	DPSPrintf (DPSGetCurrentContext(), "%% NXImage %x end\n", self);
#endif
	if (!bestRep) {
	    return NO;
	}
    }

    return YES;
}


@end

/*

Created: 2/27/90	aozer
  
Modifications (starting at 79):

79
--
 3/23/90 aozer	Separated representation functionality into NXImageRep
		and subclasses. Lots of changes!

80
--
 4/4/90 aozer	Moved parsing of multiple images in one file to NXImageRep.
 4/5/90 aozer	Added _expand for truly lazy manipulation of data files.
		Now useFromFile: just remembers file name & simply returns YES.
 4/9/90 aozer	Added lockFocus functionality and newDrawArea: method to create
		old Bitmap-style drawing areas on offscreen windows.
 4/9/90 aozer	Fixed findImageNamed: to look in the __ICON section (using
		the provided name) and the __TIFF and __EPS sections (using
		the provided name followed by .tiff or .eps if no extension
		is supplied) after the current set of existing images.

81
--
 4/12/90 aozer	Some name changes after meeting with Don

82
--
 4/18/90 aozer	Added newFromImage:rect:

83
--
 4/23/90 aozer	Added printing capability
 4/25/90 aozer	_searchForImageNamed: sets name of image if found
 5/1/90 aozer	NXImage searches existing named Bitmaps

84
--
 5/6/90 aozer	Made sure that compositing cached reps doesn't cause any
		PS other than the composite itself (set rendered to YES).
 5/10/90 aozer	Implemented dataRetained functionality.

85
--
 5/15/90 aozer	Got rid of the _reservedInt to bring NXImage down to 32 bytes.
		If we need more stuff, use one of the many private variables.
 5/15/90 aozer	Made cacheRect into four shorts rather than four floats to
		keep RepresentationInfo to under 32 bytes.
 5/24/90 aozer	Generate some helpful comments when printing if DEBUG on.
 5/25/90 aozer	Sharing of cache info between NXImage and NXCachedImageRep.

86
--
 6/6/90 aozer	Added means to get global window number and rect in window
		for a given representation.
 6/7/90 aozer	Added _bestRepresentation: method to determine the best
		representation for the current output device.
 6/8/90 aozer	Error handling support through delegates.  If an instance
		of NXImage cannot composite: or dissolve: (after having tried
		all its reps), it will call its error handler which will try
		to send a imageDidNotDraw:inRect: to the delegate.  If there's 
		no delegate or it doesn't grok this method, then the NXImage
		will simply clear the destination area with the background
		color. Otherwise if the delegate is assumed to return an
		instance of NXImage to which the same dissolve: or composite:
		message will be sent.  The delegate can of course also return
		nil, in which case it is assumed to have taken care of things.
 6/11/90 aozer	Added writeTIFF: and writeTIFF:allRepresentations:. First one
		writes the best rep, second one writes all the reps out to
		a stream. Note that writeTIFF: may append a TIFF image to
		an existing TIFF image (already written to the stream); in
		that case the stream should be opened NX_READWRITE. The same 
		thing is true if you use writeTIFF:allRepresentations:.
 6/11/90 aozer	Got resizing fully working.

87
--
 7/9/90 aozer	Flush window if on screen (with _NXShowAllWindows)
 7/10/90 aozer	Fixed bug in _searchForImageNamed: where search in ICONSEGMENT
		would find the file in the appwrapper (called
		_NXOpenStreamOnSection instead of _NXOpenStreamOnMachO). 

89
--
 7/23/90 aozer	Made search for images in sections lazy by deferring some of
		the work from _searchForImageNamed: to _expandRep:.
 7/24/90 aozer	Fixed bug with finishUnarchiving where unarchived, non-builtin
		images with the same name weren't being coalesced.
 7/25/90 aozer	lockFocus & lockFocusOn:NULL look at best representation
		instead of last one.
 7/27/90 aozer	Got rid of the lazyInfo structure that used to store info
		about lazy representations in order to reduce the number
		of memory chunks (from four to one for lazy representations). 
 7/27/90 aozer	Removed archivable & alphaWindow from the API.
 7/29/90 aozer	Moved image cache creation & management code out to
		imageUtils.pswm.

90
--
 8/3/90 aozer	Zonified.

91
--
 8/8/90 aozer	Fixed bug in writeTIFF:allRepresentations: where the rendered
		bit was getting stuck at YES after the TIFF was drawn. This
		caused the "white icons" problem in Workspace...
 8/8/90 aozer	Removed drawing white on error.
 8/8/90 aozer	Added archiveByName functionality; any image which was created 
		as a result of findImageNamed: gets this bit which allows it to
		be written out by name so that on unarchive it goes through the
		same lookup mechanism. Mostly to allow nib to work fine.
 8/12/90 aozer	Added setAsynchronous:, isAsynchronous. Currently not 
		functioning.
		Added setCacheDepthBounded, isCacheDepthBounded.

92
--
 8/14/90 aozer	Changed to use Kit windows	
 8/17/90 aozer	Put extra gsave/grestore around lock/unlockFocus if at the top
		of focus stack. View optimizes this top level gsave/grestore
		out and runs into trouble.

93
--
 8/24/90 aozer	In writeTIFF:allRepresentations:, change lockFocus/unlockFocus
		pair to gsave/grestore to fix bug 8171
 9/7/90 aozer	Made composite:fromRect:toPoint: (& dissolve) return nil on
		error.

94
--
 9/10/90 aozer	Moved common code from _drawRepresentation and 
		_printRepresentation to drawRepresentation:inRect:.
 9/23/90 aozer	Removed code to intersect fromRect with the cache when
		compositing; it caused problems (fix #9620).

95
--
 9/28/90 aozer	Bumped version to 2 & added archiving of color & delegate.
 10/1/90 aozer	Force icons to be 48 x 48.

97
--
 10/10/aozer	Made SIZEOK() accept sizes with w & h > 0 instead of >= 1.0.

*/

/*
	Bitmap.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"

#import "bitmapPrivate.h"
#import "obsoleteBitmap.h"
#import "Application.h"
#import "View.h"
#import "Window.h"
#import "tiff.h"
#import "publicWraps.h"
#import "packagesWraps.h"
#import "nextstd.h"
#import <dpsclient/wraps.h>
#import <streams/streams.h>
#import <sys/file.h>
#import <stdio.h>
#import <string.h>
#import <objc/List.h>
#import <objc/Storage.h>
#import <objc/hashtable.h>

static id	bitmapsStorage = nil;
static float	managerY = 0.0; // for debug -> 0.0 otherwise 2000.0

typedef struct {
	@defs (Storage)
} *storageId;

#define SPTR(x) 	(((storageId) (x))->dataPtr)
#define BIGHEIGHT	(256.0)
#define WININIT		(48.0)
#define UNIQUE(type)	(type == NX_UNIQUEBITMAP || type == NX_UNIQUEALPHABITMAP)
#define HASALPHA(x) (((BitmapManager *)x->_manager)->type == NX_ALPHABITMAP || ((BitmapManager *)x->_manager)->type == NX_UNIQUEALPHABITMAP)
#define WINGROW		(64.0)

#define MAXWINDOWWIDTH	(4000.0)

    extern void _NXReadBitmapBits(NXTypedStream * s, id object);
    extern void _NXWriteBitmapBits(NXTypedStream * s, id object, NXSize size);
    static BOOL addIconData(const char *name, char *bitmapData, int intWidth, int intHeight, int bps, int spp, id factory);
    static BOOL addIconId(const char *name, Bitmap *bitmapObj);
    static BOOL disposeBitmap(Bitmap *self);
    static BOOL focusFrame(BitmapManager *managerPtr, NXRect *aRect, BOOL flipIt);
    static BOOL iconFromMacho(const char *name, const char *segmentName, const char *sectionName, id factory);
    static BOOL iconFromTiff(const char *name, const char *filename, id factory);
    static BitmapManager *allocManager (int w, int h, int type);
    static BitmapManager *findManager (int w, int h, int type);
    static BitmapManager *getManager (int w, int h, int type);
    static PrebuiltInfo *checkBuiltInIcons(const char *name);
    static IconInfo *fetchIcon(const char *name);
    static void enterIcon(const char *name, Bitmap *bitmap, int width, int height, BOOL original);
    static void getIconSize(const char *name, NXSize *size, id factory);
    static id allocBitmap(int w, int h, int type, id bitmap);
    static id buildBitmapFromFile(const char *filename, id factory);
    static id buildBitmapFromStream(NXStream *stream, id factory);
    static id findBitmap(const char *iconName, id factory);
    static id getWindow(BitmapManager *managerPtr);
    static void attachManager(Bitmap *bitmap, int w, int h, int type);
    static void compactManager(BitmapManager *managerPtr, Bitmap *dontFree);
    static void compactManagers();
    static void filterHeight(int *heightPtr, int *typePtr);
    static void initManagers();
    static void placeBitmap(Bitmap *self);
    static void renewBitmap(Bitmap *self, int w, int h, BOOL alpha);
    static void resetManagerPointers();
    static void resizeBitmap(Bitmap *self, int w, int h);
    static void setFrameInManager(Bitmap *self, NXRect *aRect);
    static void setManager(Bitmap *self, BitmapManager *managerPtr);
    static void unlockFocus(BitmapManager *managerPtr);
    static BitmapManager *newClientManager(id window, int type);
    static void removeIcon(const char *name);
    

 /*
  * buffer allocated on stack to read images from server.  Anything bigger
  * gets temporarily allocated from the heap. 
  */
#define MAX_DATA_READ 2048	/* ???tune */



@implementation Bitmap

+ (BOOL)_canAlloc { return NO; }

+ allocFromZone:(NXZone *)zone { return [self doesNotRecognize:_cmd]; }

+ alloc { return [self doesNotRecognize:_cmd]; }


+ compact
{
    compactManagers();
    return self;
}

+ findBitmapFor:(const char *)name
{
    return findBitmap(name, self);
}

+ getSize:(NXSize *)size for:(const char *) name
{
    getIconSize(name, size, self);
    return self;
}

+ (BOOL)addName:(const char *)name data:(void *)bitmapData
    width:(int)samplesWide height:(int)samplesHigh
    bps:(int)bitsPerSample spp:(int)samplesPerPixel
{
    return addIconData(name, bitmapData, samplesWide, samplesHigh,
		       bitsPerSample, samplesPerPixel, self);
}


+ (BOOL)addName:(const char *)name fromTIFF:(const char *)filename

{
    return iconFromTiff(name, filename, self);
}


+ (BOOL)addName:(const char *)name fromMachO:(const char *)sectionName

{
    return iconFromMacho(name, "__TIFF", sectionName, self);
}


+ (BOOL)addName:(const char *)name bitmap:bitmapObj
{
    return addIconId(name, bitmapObj);
}


+ newFromStream:(NXStream *)stream
{
    return buildBitmapFromStream(stream, self);
}

+ newFromTIFF:(const char *)filename
{
    return buildBitmapFromFile(filename, self);
}

+ newFromMachO:(const char *)sectionName
{
    return _NXBuildBitmapFromMacho("__TIFF", sectionName, self);
}

+ newSize:(NXCoord)width :(NXCoord)height type:(int)aType
{
    return allocBitmap((int)ceil(width), (int)ceil(height), aType, 
	[super new]);
}

+ newRect:(const NXRect *)aRect type:(int)aType window:window
{
    BitmapManager *manager;
    self = [super new];
    manager = newClientManager(window, aType);
    frame.origin.x = NX_X(aRect);
    frame.origin.y = NX_Y(aRect);
    frame.size.width = ceil(NX_WIDTH(aRect));
    frame.size.height = ceil(NX_HEIGHT(aRect));
    type = aType;
    _bFlags._flipDraw = NO;
    _manager = manager;
    return self;
}

- finishUnarchiving
{
    id                  newBitmap;

    if (!iconName)
	return nil;
    newBitmap = [Bitmap findBitmapFor:iconName];
    if (newBitmap == self || !newBitmap) 
	return nil;
    free(iconName);
    iconName = 0;
    _builtIn = 0;	/* So it will free itself. */
    [self free];
    return newBitmap;
}


- free
{
    if (_builtIn > 0)
	return nil;
    if (iconName) {
	removeIcon(iconName);
	free(iconName);
	iconName = 0;
    }
    if (!_manager)
	[super free];
    else if (_builtIn <= 0 && disposeBitmap(self)) {
	[super free];
    }
    return nil;
}

- setFlip:(BOOL)flag
{
    _bFlags._flipDraw = flag;
    return self;
}

- _getFrame:(NXRect *)aRect
{
    *aRect = frame;
    return self;
}


- (const char *)name
{
    return iconName;
}


- getSize:(NXSize *)theSize
{
    *theSize = frame.size;
    return self;
}


- (BOOL)lockFocus
{
    return focusFrame(_manager, &frame, _bFlags._flipDraw);
}

- unlockFocus
{
    unlockFocus(_manager);
    return self;
}

- (int)type
{
    return type;
}

- writeTIFF:(NXStream *)stream
{
    int                 bytes;
    NXImageInfo         info;
    unsigned char      *image = NULL;

    [self imageSize:&bytes width:&info.width height:&info.height
     bps:&info.bitsPerSample spp:&info.samplesPerPixel 
	 config:&info.planarConfig interp:&info.photoInterp inRect:NULL];
    if (info.samplesPerPixel > 1 && info.planarConfig == NX_PLANAR) {
		if (info.samplesPerPixel == 2 && (info.photoInterp & NX_ALPHAMASK)) {
			image = (unsigned char *)malloc(bytes * 2);
			[self readImage:image withAlpha:image+bytes];
		} else {
			NXLogError ("Bitmap writeTIFF: can't write out planar color data");
		}
    } else {
		image = (unsigned char *)malloc(bytes);
		[self readImage:image];
    }
	if (image) {
	    NXWriteTIFF(stream, &info, image);
	    free(image);
	}
    return self;
}


#define ALPHA (NX_MONOTONICMASK|NX_ALPHAMASK)

- image:(void *)data withAlpha:(void *)alpha
    width:(int)samplesWide height:(int)sampleHigh
    bps:(int)bitsPerSample
{
    NXRect              destRect;

    [self lockFocus];
    destRect.origin.x = destRect.origin.y = 0.0;
    destRect.size.width = samplesWide;
    destRect.size.height = sampleHigh;
    NXImageBitmap(&destRect, samplesWide, sampleHigh, bitsPerSample, 2,
		  NX_PLANAR, ALPHA, data, alpha, NULL, NULL, NULL);
    [self unlockFocus];
    return self;
}

/*
 * The following method added in an effort to make Bitmap deal with
 * color better; it allows you to specify planar configuration and
 * photoInterp values. 
 */
- image:(void *)data toRect:(const NXRect *)rect 
	width:(int)samplesWide height:(int)samplesHigh
    bps:(int)bitsPerSample spp:(int)samplesPerPixel 
	config:(int)planarConfig interp:(int)photoInterp
{
	NXRect destRect;
	int planeSize;
	int numArgs;

	if (rect) {
		destRect = *rect;
	} else {
    	destRect.origin.x = destRect.origin.y = 0.0;
    	destRect.size.width = samplesWide;
    	destRect.size.height = samplesHigh;
	}

	planeSize = (((bitsPerSample * samplesWide * 
				 ((planarConfig == NX_MESHED) ? samplesPerPixel : 1)) + 7) / 8) * samplesHigh;

    [self lockFocus];
	numArgs = (planarConfig == NX_MESHED) ? 1: samplesPerPixel;
	NXImageBitmap (&destRect, samplesWide, samplesHigh,
		bitsPerSample, samplesPerPixel, planarConfig, photoInterp,
		data, 
		(numArgs > 1 ? data + planeSize : NULL),
		(numArgs > 2 ? data + planeSize * 2 : NULL),
		(numArgs > 3 ? data + planeSize * 3 : NULL),
		(numArgs > 4 ? data + planeSize * 4 : NULL)); 
    [self unlockFocus];
    return self;
}

- image:(void *)data width:(int)samplesWide height:(int)sampleHigh
    bps:(int)bitsPerSample spp:(int)samplesPerPixel
{
    NXRect              destRect;

    destRect.origin.x = destRect.origin.y = 0.0;
    destRect.size.width = samplesWide;
    destRect.size.height = sampleHigh;
    return[self image:data toRect:&destRect
	   width:samplesWide height:sampleHigh
	   bps:bitsPerSample spp:samplesPerPixel];
    return self;
}


- image:(void *)data toRect:(const NXRect *)rect
    width:(int)samplesWide height:(int)sampleHigh
    bps:(int)bitsPerSample spp:(int)samplesPerPixel
{
    unsigned char      *alpha = NULL;
    int                 mask = NX_MONOTONICMASK;

    if (samplesPerPixel == 2) {
	alpha = (unsigned char *)data + (((samplesWide * bitsPerSample + 7) / 8) * sampleHigh);
	mask |= ALPHA;
    }
    [self lockFocus];
    NXImageBitmap(rect, samplesWide, sampleHigh,
		  bitsPerSample, samplesPerPixel, NX_PLANAR, mask,
		  data, alpha, NULL, NULL, NULL);
    [self unlockFocus];
    return self;
}

/*
 * The following method added to get Bitmap to deal with color
 * better.  This method returns the planarConfig and photoInterp
 * values (which the imageSize:... method didn't).
 */
- imageSize:(int *)sizeInBytes
    width:(int *)ptrWidth height:(int *)ptrHeight
    bps:(int *)ptrBps spp:(int *)ptrSpp 
	config:(int *)ptrConfig interp:(int *)ptrInterp
	inRect:(const NXRect *)rect
{
    register NXRect    *r;
    NXRect              tRect;

    if (!rect) {
		r = &tRect;
		NX_X(r) = 0.0;
		NX_Y(r) = 0.0;
		NX_WIDTH(r) = frame.size.width;
		NX_HEIGHT(r) = frame.size.height;
    } else {
		r = (NXRect *)rect;
	}

    [self lockFocus];
    NXSizeBitmap(r, sizeInBytes, ptrWidth, ptrHeight, ptrBps, ptrSpp,
		 ptrConfig, ptrInterp);
    [self unlockFocus];
    if (*ptrConfig == NX_PLANAR) {
		*sizeInBytes /= *ptrSpp;
	}
    return self;
}

- imageSize:(int *)sizeInBytes
    width:(int *)ptrWidth height:(int *)ptrHeight
    bps:(int *)ptrBps spp:(int *)ptrSpp inRect:(const NXRect *)rect
{
	int config, interp;	/* Ignored */

	return [self imageSize:sizeInBytes width:ptrWidth height:ptrHeight
			bps:ptrBps spp:ptrSpp config:&config interp:&interp
			inRect:rect];
}
		

- readImage:(void *)image
{
    NXRect              _imageRect;
    register NXRect    *r = &_imageRect;

    NX_X(r) = 0.0;
    NX_Y(r) = 0.0;
    NX_WIDTH(r) = frame.size.width;
    NX_HEIGHT(r) = frame.size.height;
    return[self readImage:image inRect:r];
}

/* 
 * Note that readImage: originally was meant to return only the first
 * channel of the image.  With meshed images, it returns the whole thing.
 * With planar images, color or not, returns just the first channel.
 */
- readImage:(void *)image inRect:(const NXRect *)rect
{
    int width, height, bps, spp, size, config, interp;

    [self lockFocus];
    [self imageSize:&size width:&width height:&height
		  bps:&bps spp:&spp config:&config interp:&interp inRect:rect];
	if (config == NX_PLANAR) {
    	NXReadBitmap(rect, width, height, bps, 1, 
			config, interp & (NX_COLORMASK | NX_MONOTONICMASK),
		 	image, NULL, NULL, NULL, NULL);
	} else {
	    NXReadBitmap(rect, width, height, bps, spp, config, interp,
		 	image, NULL, NULL, NULL, NULL);
	}
    [self unlockFocus];
    return self;
}


- readImage:(void *)image withAlpha:(void *)alpha
{
    NXRect              _imageRect;
    register NXRect    *r = &_imageRect;

    NX_X(r) = 0.0;
    NX_Y(r) = 0.0;
    NX_WIDTH(r) = frame.size.width;
    NX_HEIGHT(r) = frame.size.height;
    return[self readImage:image withAlpha:alpha inRect:r];
}

/* 
 * Note that readImage:alpha: originally was meant to return only the first
 * channel of the image.  With meshed images, it returns the whole thing.
 * With planar images, color or not, returns just the first channel of 
 * color and the alpha.
 */
- readImage:(void *)image withAlpha:(void *)alpha inRect:(NXRect *)rect
{
    int width, height, bps, spp, size, config, interp;

    [self lockFocus];
    [self imageSize:&size width:&width height:&height
		  bps:&bps spp:&spp config:&config interp:&interp inRect:rect];
	if (config == NX_PLANAR) {
    	NXReadBitmap(rect, width, height, bps, spp, 
			config, interp,
		 	image, 
			(spp == 2 ? alpha : NULL), 
			(spp == 3 ? alpha : NULL), 
			(spp == 4 ? alpha : NULL), 
			(spp == 5 ? alpha : NULL));
	} else {
	    NXReadBitmap(rect, width, height, bps, spp, config, interp,
		 	image, NULL, NULL, NULL, NULL);
	}
    [self unlockFocus];
    return self;
}


- composite:(int)op toPoint:(const NXPoint *)aPoint
{
    NXRect              tRect;

    tRect.origin.x = tRect.origin.y = 0.0;
    tRect.size = frame.size;
    return[self composite:op fromRect:&tRect toPoint:aPoint];
}


- composite:(int)op fromRect:(const NXRect *)aRect toPoint:(const NXPoint *)aPoint
{
    NXPoint		srcOrigin;
    NXPoint		destOrigin;
    NXSize		destSize;
    id			destView;

    srcOrigin.x = frame.origin.x + aRect->origin.x;
 /*
  * compensate for flipping since we dont focus on source bitmap, but instead
  * just use the coord system of the backing window. 
  */
    srcOrigin.y = _bFlags._flipDraw ? NX_MAXY(&frame) - NX_MAXY(aRect) :
      frame.origin.y + aRect->origin.y;

    destView = [NXApp focusView];
    if (NXDrawingStatus == NX_DRAWING) {
    /*
     * since origin window is never flipped, and composite preserves the
     * direction that the rect hangs, we must compensate for flipped-ness in
     * the destination. 
     */
	destOrigin = *aPoint;
	if ([destView isFlipped]) {
	    destSize.width = 0.0;
	    destSize.height = NX_HEIGHT(aRect);
	    [destView convertSize:&destSize fromView:nil];
	    destOrigin.y += ABS(destSize.height);
	}
	PScomposite(srcOrigin.x, srcOrigin.y,
		    aRect->size.width, aRect->size.height,
		    [getWindow(_manager) gState],
		    destOrigin.x,
		    destOrigin.y,
		    op);
    } else {
	int                 height, width, bps, spp;
	int                 totalBytes, channelBytes;
	unsigned char       imageSpace[MAX_DATA_READ];
	unsigned char      *data[5];
	int                 planarConfig;
	int                 photoInt;
	NXRect              srcRect, destRect;
	int                 i;
	int		    srcGState;
	int		    newWindow = 0;


	srcRect.origin = srcOrigin;
	srcRect.size = aRect->size;
	DPSSetContext([NXApp context]);	/* talk to the server */
    /*
     * We have to do our own focusing here, since the focusing machinery
     * doesnt support two DPS contexts.  Focusing is a relative operation to
     * the current focus, which has not been established in the server since
     * we've been talking to the spool file. 
     */
    /*
     * ??? because compositing ignores scaling in the destination, wont give
     * the same results printing when composting to a scaled view. 
     */
	PSgsave();
	srcGState = [getWindow(_manager) gState];
	NXSetGState(srcGState);
	NXSizeBitmap(&srcRect, &totalBytes, &width, &height, &bps, &spp,
		     &planarConfig, &photoInt);
	if (photoInt & NX_ALPHAMASK) {
	    _NXCreateAndSetScratchWindow(width, height);
	    newWindow = DPSDefineUserObject(0);
	    PScomposite(srcRect.origin.x, srcRect.origin.y,
	    		srcRect.size.width, srcRect.size.height,
			srcGState, 0.0, 0.0, NX_SOVER);
	    srcRect.origin.x = srcRect.origin.y = 0.0;
	    NXSizeBitmap(&srcRect, &totalBytes, &width, &height, &bps, &spp,
		     &planarConfig, &photoInt);
	}
	NX_ASSERT(!(photoInt & NX_ALPHAMASK), "Trying to print alpha");
	if (totalBytes > MAX_DATA_READ)
	    NX_MALLOC(data[0], unsigned char, totalBytes);
	else
	    data[0] = imageSpace;
	channelBytes = totalBytes / spp;
	for (i = 1; i < spp; i++)
	    data[i] = data[i - 1] + channelBytes;
	NXReadBitmap(&srcRect, width, height, bps, spp,
		     planarConfig, photoInt,
		     data[0], data[1], data[2], data[3], data[4]);
	if (newWindow)
	    _NXTermWindow(newWindow);
	PSgrestore();
	DPSSetContext([[NXApp printInfo] context]);	/* talk to printer */
	destRect.origin = *aPoint;
	destRect.size.width = width;
	destRect.size.height = height;
	[destView convertSize:&destRect.size fromView:nil];
	NXImageBitmap(&destRect, width, height, bps, spp,
		      planarConfig, photoInt,
		      data[0], data[1], data[2], data[3], data[4]);
	if (totalBytes > MAX_DATA_READ)
	    NX_FREE(data[0]);
    }
    return self;
}

- write:(NXTypedStream *) s
{
    [super write:s];
    NXWriteTypes(s, "*iis", &iconName, &type, &_builtIn, &_bFlags);
    if (_builtIn)
	return self;
    NXWriteSize(s, &frame.size);
 /* redundant, but makes life easier for NIB */
    NXWriteTypes(s, "i", &type);
    _NXWriteBitmapBits(s, self, frame.size);
    return self;
}


- read:(NXTypedStream *) s
{
    [super read:s];
    NXReadTypes(s, "*iis", &iconName, &type, &_builtIn, &_bFlags);
    if (_builtIn)
	return self;
    NXReadSize(s, &frame.size);
    attachManager(self, (int)frame.size.width, (int) frame.size.height, type);
 /* redundant, but makes life easier for NIB */
    NXReadTypes(s, "i", &type);
    _NXReadBitmapBits(s, self);
    if (iconName)
	[Bitmap addName:iconName bitmap:self];
    return self;
}

- _objcfree
{
    return [super free];
}

- window
{
    return ((BitmapManager *)_manager)->window;
}

- _renewBitmapWidth:(int)w height:(int)h alpha:(BOOL)aFlag
{
    renewBitmap(self, w, h, aFlag);
    return self;
}

- resize:(NXCoord)theWidth :(NXCoord)theHeight
{
    resizeBitmap(self, (int)ceil(theWidth), (int)ceil(theHeight));
    return self;
}


/*
 * If the named prebuilt bitmap return its window gState & location in the
 * window.   If no such prebuilt bitmap exists, return NO.
 */
+ (BOOL)_findBuiltInBitmap:(const char *)name 
	rect:(NXRect *)rect win:(Window **)win;
{
    PrebuiltInfo *info = checkBuiltInIcons(name);
    extern id NXSharedCursors, NXSharedIcons;
    
    if (info) {
	*rect = info->rect;
	*win = info->alpha ? NXSharedCursors : NXSharedIcons;
	return YES;
    } else {
	return NO;
    }
}

+ _findExistingBitmap:(const char *)name
{
    IconInfo *iptr = fetchIcon(name);

    return (iptr ? iptr->bitmap : nil);
}

@end


static void initManagers()
{
    if (!bitmapsStorage) {
    	bitmapsStorage = 
    	    [Storage newCount:0 
	    	  elementSize:BITMAPMGRSIZE 
		  description:BITMAPMGRDESCR];
    }
}

static void filterHeight(int *heightPtr, int *typePtr)
{
    int	height = *heightPtr;
    int type = *typePtr;
    
    height = (height <= 16) ? 16 : 
             (height <= 24) ? 24 : 
             (height <= 32) ? 32 :
             (height <= 48) ? 48 :  /* Many 48 Icons */
             (height <= 64) ? 64 :
             (height <= 128) ? 128 :
             (height <= 192) ? 192 :
             (height <= (int)BIGHEIGHT) ? (int)BIGHEIGHT : height;
    if (height > (int)BIGHEIGHT) 
	*typePtr = (type == NX_ALPHABITMAP)? 
	    NX_UNIQUEALPHABITMAP : NX_UNIQUEBITMAP;
    *heightPtr = height;
}

static BitmapManager *allocManager (int w, int h, int type)
{
    BitmapManager	manager;
    NXRect		wFrame;
    unsigned		pos;
    BitmapManager	*startPtr;			

    startPtr = (BitmapManager *)SPTR(bitmapsStorage);
    wFrame.origin.x = 0.0;
    wFrame.origin.y = managerY;
    wFrame.size.width = (float)w;
    wFrame.size.height = (float)h;
    if (!UNIQUE(type) && h <= ((int)BIGHEIGHT)) 
    	wFrame.size.width = MAX(wFrame.size.width, WININIT);
    manager.window =
    	[Window newContent:&wFrame
		style:NX_PLAINSTYLE
		backing:NX_RETAINED
		buttonMask:0
		defer:NO
		screen:NULL];
    [manager.window setFreeWhenClosed:YES];
    [manager.window reenableDisplay];
    manager.bitmapList = [List new];
    manager.focusView = [View new];
    [manager.focusView setOpaque:YES];
    [[manager.window contentView] addSubview:manager.focusView];
    [[manager.window contentView] setClipping:NO];
    manager.type = type;
    manager.maxX = 0.0;
    pos = [bitmapsStorage count];
    [bitmapsStorage insert:(char *)&manager at:pos];
    if (startPtr != (BitmapManager *)SPTR(bitmapsStorage)) 
    	resetManagerPointers();
    return (BitmapManager *)[bitmapsStorage elementAt:pos];
}

static void resetManagerPointers()
/* 
 * When bitmapsStorage has been reallocated we have to set the 
 * manager pointers of the bitmaps to the new values.
 */
{
    BitmapManager	*managerPtr;
    id			*bitmapPtr;
    int			i, size, j, lSize;

    managerPtr = (BitmapManager *)SPTR(bitmapsStorage);
    size = [bitmapsStorage count];
    for (i = 0; i < size; i++) {
        if (lSize = [managerPtr->bitmapList count]) {
	    bitmapPtr = NX_ADDRESS(managerPtr->bitmapList);
	    for (j = 0; j < lSize; j++) {
	        setManager(*bitmapPtr, managerPtr);
	        bitmapPtr++;
	    }
	}
        managerPtr++;
    }
}

static void setManager(Bitmap *self, BitmapManager *managerPtr)
{
    self->_manager = managerPtr;
}

static BitmapManager *findManager (int w, int h, int type)
{
    BitmapManager	*managerPtr;
    int			i, size;
    NXRect		wFrame;
    
    managerPtr = (BitmapManager *)SPTR(bitmapsStorage);
    size = [bitmapsStorage count];
    for (i = 0; i < size; i++) {
    	if (managerPtr->type == type) {
	    [managerPtr->window getFrame:&wFrame];
	    if (!UNIQUE(type)) {
	        if (h == (int)wFrame.size.height) {
		    if ((managerPtr->maxX + (float)w) < MAXWINDOWWIDTH) break;
		    else {
		        compactManager(managerPtr, nil);
		        if ((managerPtr->maxX + (float)w) < MAXWINDOWWIDTH)
			    break;
		    }
		}
	    } else {
		if (![managerPtr->bitmapList count]) break;
	    }
	}
        managerPtr++;
    }
    if (i < size) return managerPtr;
    else return ((BitmapManager *)0);
}

static void compactManager(BitmapManager *managerPtr, Bitmap *dontFree)
{
    Bitmap	**bitmapPtr = 0, *bitmap = 0;
    int		i, size, gState;
    NXRect	aRect, bRect;
    NXCoord	delta, offset;
    BOOL	lastIsFreed = NO, cleaned;
    
    gState = [managerPtr->window gState];
    [managerPtr->window getFrame:&aRect];
    offset = 0.0;
    NXSetRect(&aRect, 0.0, 0.0, 0.0, aRect.size.height);
    size = [managerPtr->bitmapList count];
    if (size) bitmapPtr = NX_ADDRESS(managerPtr->bitmapList);
    for (i = 0; i < size; i++) {
        bitmap = *bitmapPtr;
	bitmap->frame.origin.x -= offset;
        if (bitmap->_bFlags._willFree) {
	    if (lastIsFreed) {
	        aRect.size.width += bitmap->frame.size.width;
	    } else {
	        aRect.origin.x = bitmap->frame.origin.x;
	        aRect.size.width = bitmap->frame.size.width;
		lastIsFreed = YES;
	    }
	} else {
	    if (lastIsFreed) {
	        bRect.origin = aRect.origin;
	        bRect.size.width = managerPtr->maxX - aRect.origin.x;
		bRect.size.height = aRect.size.height;
		delta = aRect.size.width;
		PSgsave();
		NXSetGState(gState);
    		PScomposite (aRect.origin.x+delta,
    		     0.0,
		     bRect.size.width-delta,
		     bRect.size.height,
		     gState,
		     bRect.origin.x,
		     0.0,
		     NX_COPY);
		PSgrestore();
    		managerPtr->maxX -= delta;
		bitmap->frame.origin.x -= delta;
		offset += delta;
	        lastIsFreed = NO;
	    }
	}
    	bitmapPtr++;
    }
    if (lastIsFreed) managerPtr->maxX -= aRect.size.width;
    cleaned = NO;
    while (!cleaned) {
    	size = [managerPtr->bitmapList count];
	if (!size) cleaned = YES;
	else {
    	    bitmapPtr = NX_ADDRESS(managerPtr->bitmapList);
	    for (i = 0; i < size; i++) {
	        bitmap = *bitmapPtr;
		if (bitmap->_bFlags._willFree) break;
		bitmapPtr++;
	    }
	    if (i == size) cleaned = YES;
	    else {
	    	[managerPtr->bitmapList removeObjectAt:i];
		if (bitmap != dontFree) [bitmap _objcfree];
	    }
	}
    }
}

static void compactManagers()
{
    BitmapManager	*managerPtr;
    int			i, size;
    NXRect		winFrame;

    managerPtr = (BitmapManager *)SPTR(bitmapsStorage);
    size = [bitmapsStorage count];
    for (i = 0; i < size; i++) {
	if (!UNIQUE(managerPtr->type)) {
	    compactManager(managerPtr, nil);
	    [managerPtr->window getFrame:&winFrame];
	    if(winFrame.size.width - managerPtr->maxX > (2 * WINGROW)) {
	       winFrame.size.width = managerPtr->maxX + WINGROW;
	       [managerPtr->window placeWindow:&winFrame];
	    }
	}
        managerPtr++;
    }
}

static void placeBitmap(Bitmap *self)
/* Width and height have been set before */
{
    NXRect		winFrame, *bmFrame;
    NXCoord		newWidth;
    BOOL		sizeFlag;
    BitmapManager	*managerPtr;

    sizeFlag = NO;
    managerPtr = self->_manager;
    [managerPtr->window getFrame:&winFrame];
    bmFrame = &self->frame;
    bmFrame->origin.x = managerPtr->maxX;
    bmFrame->origin.y = 0.0;
    newWidth = NX_MAXX(bmFrame);
    if (!UNIQUE(managerPtr->type)) {
    	if (newWidth > winFrame.size.width) {
	    compactManager(managerPtr, nil);
	    if((winFrame.size.width - managerPtr->maxX > bmFrame->size.width)
	       && (winFrame.size.width - managerPtr->maxX > (2 * WINGROW))) {
	       winFrame.size.width = 
	       		managerPtr->maxX + bmFrame->size.width + WINGROW;
    	       [managerPtr->window placeWindow:&winFrame];
	    }
    	    bmFrame->origin.x = managerPtr->maxX;
    	    bmFrame->origin.y = 0.0;
    	    newWidth = NX_MAXX(bmFrame);
    	    managerPtr->maxX = newWidth;
	    if (newWidth > winFrame.size.width) {
    	    	winFrame.size.width = 
			MIN((newWidth + 
			     ((winFrame.size.height <= BIGHEIGHT) 
			     				? WINGROW : 0.0)),
			     MAXWINDOWWIDTH);
	    	sizeFlag = YES;
	    }
	} else managerPtr->maxX = newWidth;
    } else {
        managerPtr->maxX = newWidth;
        if (winFrame.size.width != bmFrame->size.width) {
            winFrame.size.width = bmFrame->size.width;
	    sizeFlag = YES;
	}
	if (winFrame.size.height != bmFrame->size.height) {
            winFrame.size.height = bmFrame->size.height;
	    sizeFlag = YES;
	}
    }
    if (sizeFlag) {
    	[managerPtr->window placeWindow:&winFrame];
    }
    DPSFlush();
    if (managerPtr->type == NX_ALPHABITMAP || managerPtr->type == NX_UNIQUEALPHABITMAP) {	
	PSgsave();
	NXSetGState([managerPtr->window gState]);
	PScompositerect(
	    bmFrame->origin.x, bmFrame->origin.y,
	    bmFrame->size.width, bmFrame->size.height,
	    NX_CLEAR);
	PSgrestore();
    } else {
        focusFrame(managerPtr, bmFrame, NO);
	winFrame.origin.x = winFrame.origin.y = 0.0;
	winFrame.size = bmFrame->size;
	NXEraseRect(&winFrame);
        unlockFocus(managerPtr);
    }
}

static BOOL focusFrame(BitmapManager *managerPtr, NXRect *aRect, BOOL flipIt)
{
    [managerPtr->focusView setFlip:flipIt];
    [managerPtr->focusView setFrame:aRect];
    return [managerPtr->focusView lockFocus];
}

static void unlockFocus(BitmapManager *managerPtr)
{
    [managerPtr->focusView unlockFocus];
}

static BOOL disposeBitmap(Bitmap *self)
{
    BitmapManager	*managerPtr;
    BOOL mustFree = NO;
    
    self->_bFlags._willFree = 1;
    managerPtr = self->_manager;
    if (UNIQUE(managerPtr->type)) {
	BitmapManager *start = SPTR(bitmapsStorage);
	[managerPtr->bitmapList free];
    	[managerPtr->window close];
	[bitmapsStorage removeAt: managerPtr - start];
	resetManagerPointers();
	mustFree = YES;
    }
    return mustFree;
}

static void attachManager(Bitmap *bitmap, int w, int h, int type)
{
    BitmapManager	*managerPtr;
    NXRect		aFrame;

    initManagers();
    managerPtr = getManager(w, h, type);
    setManager(bitmap,  managerPtr);
    aFrame.size.width = (float)w;
    aFrame.size.height = (float)h;
    /* The real values will be set by placeBitmap */
    aFrame.origin.x = aFrame.origin.y = 0.0;
    setFrameInManager(bitmap, &aFrame);
    bitmap->_bFlags._flipDraw = YES;
    bitmap->type = type;
    [managerPtr->bitmapList addObject:bitmap];
    placeBitmap(bitmap);
}

static id allocBitmap(int w, int h, int type, id bitmap)
{
    attachManager(bitmap, w, h, type);
    return bitmap;
}

static BitmapManager *getManager (int w, int h, int type)
{
    BitmapManager	*managerPtr;
    
    if (!UNIQUE(type)) filterHeight(&h, &type);
    managerPtr = findManager(w, h, type);
    if (!managerPtr) managerPtr = allocManager(w, h, type);
    return managerPtr;
}

static void setFrameInManager(Bitmap *self, NXRect *aRect)
{
    self->frame = *aRect;
}

static void renewBitmap(Bitmap *self, int w, int h, BOOL alpha)
{
    BitmapManager	*managerPtr = self->_manager;

    if (((int)self->frame.size.width == w) 
    	&& ((int)self->frame.size.height == h)
	&& (HASALPHA(self) == alpha))
	return;
    if (!UNIQUE(managerPtr->type)) {
        NXRect	aFrame;
	
        disposeBitmap(self);
	compactManager(self->_manager, self);
    	managerPtr = 
	    getManager(w, h, (alpha ? NX_ALPHABITMAP : NX_NOALPHABITMAP));
	self->_bFlags._willFree = 0;
    	setManager(self,  managerPtr);
    	aFrame.size.width = (float)w;
    	aFrame.size.height = (float)h;
    	/* The real values will be set by placeBitmap */
    	aFrame.origin.x = aFrame.origin.y = 0.0;
    	setFrameInManager(self, &aFrame);
    	[managerPtr->bitmapList addObject:self];
        placeBitmap(self);
    } else {
        managerPtr->type = (alpha ? 
	    NX_UNIQUEALPHABITMAP : NX_UNIQUEBITMAP);
    	self->frame.size.width = (float)w;
    	self->frame.size.height = (float)h;
    	managerPtr->maxX = 0.0;
        placeBitmap(self);
    }
}

static void resizeBitmap(Bitmap *self, int w, int h)
{
    renewBitmap(self, w, h, HASALPHA(self));
}


#define HASHSIZE 31

static unsigned iconHash(const void *info, const void *data)
{
    const IconInfo *icon = data;
    return NXStrHash(info, icon->name);
}

static int iconEqual(const void *info, const void *data1, const void *data2)
{
    const IconInfo *icon1 = data1;
    const IconInfo *icon2 = data2;
    return NXStrIsEqual(info, icon1->name, icon2->name);
}

static unsigned builtInHash(const void *info, const void *data)
{
    const PrebuiltInfo *obj = data;
    return NXStrHash(info, obj->name);
}

static int builtInEqual(const void *info, const void *data1, const void *data2)
{
    const PrebuiltInfo *obj1 = data1;
    const PrebuiltInfo *obj2 = data2;
    return NXStrIsEqual(info, obj1->name, obj2->name);
}

static unsigned clientHash(const void *info, const void *data)
{
    const ClientAllocated *obj = data;
    return NXPtrHash(info, obj->window);
}

static int clientEqual(const void *info, const void *data1, const void *data2)
{
    const ClientAllocated *obj1 = data1;
    const ClientAllocated *obj2 = data2;
    return NXPtrIsEqual(info, obj1->window, obj2->window);
}


static NXHashTable *hashTable = NULL;
static NXHashTable *builtIn = NULL;
static NXHashTable *clientManagers = NULL;

#ifdef SHLIB
static void dontFree(const void *info, void *data)
{

}

static NXHashTablePrototype iconProto = {
    iconHash, iconEqual, dontFree, 0
};
static NXHashTablePrototype builtInProto = {
    builtInHash, builtInEqual, dontFree, 0
};
static NXHashTablePrototype clientProto = {
   clientHash, clientEqual, dontFree, 0
};
#else
static NXHashTablePrototype iconProto = {
    iconHash, iconEqual, NXNoEffectFree, 0
};
static NXHashTablePrototype builtInProto = {
    builtInHash, builtInEqual, NXNoEffectFree, 0
};
static NXHashTablePrototype clientProto = {
   clientHash, clientEqual, NXNoEffectFree, 0
};
#endif

/* this list had better be in alphabetic order */

static const PrebuiltInfo builtInIcons[] = {
    {"    ", 			{{0., 0.}, {16., 16.}}, NO },
    {"auto", 			{{0., 181.}, {77., 11.}}, NO },
    {"choose", 			{{128., 128.}, {57., 54.}}, NO },
    {"chooseH", 		{{192., 128.}, {57., 54.}}, NO },
    {"circle16", 		{{16., 64.}, {16., 16.}}, NO },
    {"circle16H", 		{{0., 64.}, {16., 16.}}, NO },
    {"close", 			{{48., 64.}, {15., 15.}}, NO },
    {"closeH", 			{{32., 64.}, {15., 15.}}, NO },
    {"defaultappicon", 		{{208., 0.}, {48., 48.}}, NO },
    {"defaulticon", 		{{128., 0.}, {48., 48.}}, NO },
    {"divider", 		{{80., 64.}, {12., 7.}}, NO },
    {"dividerH", 		{{64., 64.}, {12., 7.}}, NO },
    {"editing", 		{{192., 0.}, {15., 15.}}, NO },
    {"hSliderKnob", 		{{20., 128.}, {20., 14.}}, NO },
    {"iconify", 		{{112., 64.}, {15., 15.}}, NO },
    {"iconifyH", 		{{96., 64.}, {15., 15.}}, NO },
    {"landscape", 		{{0., 192.}, {64., 50.}}, NO },
    {"landscapeH", 		{{192., 192.}, {64., 50.}}, NO },
    {"manual", 			{{77., 170.}, {77., 11.}}, NO },
    {"menuArrow", 		{{144., 64.}, {12., 9.}}, NO },
    {"menuArrowH", 		{{128., 64.}, {12., 9.}}, NO },
    {"miniWindow", 		{{0., 0.}, {64., 64.}}, NO },
    {"miniWorld", 		{{64., 0.}, {64., 64.}}, NO },
    {"popup", 			{{192., 17.}, {11., 8.}}, NO },
    {"popupH", 			{{192., 25.}, {11., 8.}}, NO },
    {"portrait", 		{{64., 192.}, {64., 50.}}, NO },
    {"portraitH", 		{{128., 192.}, {64., 50.}}, NO },
    {"pulldown", 		{{160., 112.}, {9., 12.}}, NO },
    {"pulldownH", 		{{176., 112.}, {9., 12.}}, NO },
    {"radio", 			{{0., 80.}, {16., 15.}}, NO },
    {"radioH", 			{{160., 64.}, {16., 15.}}, NO },
    {"resize", 			{{32., 80.}, {15., 15.}}, NO },
    {"resizeH", 		{{16., 80.}, {15., 15.}}, NO },
    {"resizeKnob", 		{{128., 112.}, {12., 12.}}, NO },
    {"resizeKnobH", 		{{144., 112.}, {12., 12.}}, NO },
    {"returnSign", 		{{64., 80.}, {16., 10.}}, NO },
    {"returnSignH", 		{{48., 80.}, {16., 10.}}, NO },
    {"scrollDown", 		{{80., 80.}, {16., 16.}}, NO },
    {"scrollDownH", 		{{96., 80.}, {16., 16.}}, NO },
    {"scrollKnob", 		{{112., 80.}, {16., 16.}}, NO },
    {"scrollLeft", 		{{128., 80.}, {16., 16.}}, NO },
    {"scrollLeftH", 		{{144., 80.}, {16., 16.}}, NO },
    {"scrollMenuDown", 		{{16., 96.}, {12., 12.}}, NO },
    {"scrollMenuDownD", 	{{160., 80.}, {12., 12.}}, NO },
    {"scrollMenuDownH", 	{{0., 96.}, {12., 12.}}, NO },
    {"scrollMenuLeft", 		{{64., 96.}, {12., 11.}}, NO },
    {"scrollMenuLeftD", 	{{32., 96.}, {12., 11.}}, NO },
    {"scrollMenuLeftH", 	{{48., 96.}, {12., 12.}}, NO },
    {"scrollMenuRight", 	{{112., 96.}, {12., 11.}}, NO },
    {"scrollMenuRightD", 	{{80., 96.}, {12., 11.}}, NO },
    {"scrollMenuRightH", 	{{96., 96.}, {12., 12.}}, NO },
    {"scrollMenuUp", 		{{160., 96.}, {12., 12.}}, NO },
    {"scrollMenuUpD", 		{{128., 96.}, {12., 12.}}, NO },
    {"scrollMenuUpH", 		{{144., 96.}, {12., 12.}}, NO },
    {"scrollRight", 		{{0., 112.}, {16., 16.}}, NO },
    {"scrollRightH", 		{{16., 112.}, {16., 16.}}, NO },
    {"scrollUp", 		{{32., 112.}, {16., 16.}}, NO },
    {"scrollUpH", 		{{48., 112.}, {16., 16.}}, NO },
    {"square16", 		{{64., 112.}, {16., 16.}}, NO },
    {"square16H", 		{{80., 112.}, {16., 16.}}, NO },
    {"switch", 			{{112., 112.}, {15., 15.}}, NO },
    {"switchH", 		{{96., 112.}, {15., 15.}}, NO },
    {"vSliderKnob", 		{{0., 128.}, {14., 20.}}, NO },
    {"NXleftindent", 		{{27., 11.}, { 9.,  6.}}, YES },
    {"NXrightindent", 		{{27., 11.}, { 9.,  6.}}, YES },
    {"NXfirstindent", 		{{16.,  6.}, { 9., 11.}}, YES },
    {"NXtab", 			{{39.,  6.}, { 6., 11.}}, YES },
    {"NXleftmargin",		{{ 8.,  0.}, { 5., 17.}}, YES },
    {"NXrightmargin",		{{ 0.,  0.}, { 5., 17.}}, YES },
    {"NXarrow",			{{0.,  32.}, {16., 16.}}, YES },
    {"NXibeam",			{{32., 32.}, {16., 16.}}, YES },
    {"NXwait",			{{48., 32.}, {16., 16.}}, YES },
    {0},
};

#define NUM_ICONS (sizeof(builtInIcons)/sizeof(PrebuiltInfo))

static void hashBuiltIn()
{
    const PrebuiltInfo *icons;
    builtIn = NXCreateHashTable(builtInProto, 0, 0);
    for (icons = builtInIcons; icons->name; icons++) 
	NXHashInsert(builtIn, icons);
}

static BitmapManager *newClientManager(id window, int type)
{
    ClientAllocated temp, *found, *new;
    BitmapManager *manager;
    if (!clientManagers)
	clientManagers = NXCreateHashTable(clientProto, 0, 0);
    temp.window = window;
    found = NXHashGet(clientManagers, &temp);
    if (found)
	return found->manager;
    NX_MALLOC(new, ClientAllocated, 1);
    NX_MALLOC(manager, BitmapManager, 1);
    manager->window = window;
    manager->bitmapList = [List new];
    manager->type = type;
    manager->maxX = 0.;
    manager->focusView = [View new];
    [manager->focusView setOpaque:YES];
    [[window contentView] addSubview:manager->focusView];
    [[window contentView] setClipping:NO];
    new->window = window;
    new->manager = manager;
    NXHashInsert(clientManagers, new);
    return new->manager;
}


static PrebuiltInfo *checkBuiltInIcons(const char *name)
 /* checks static data for icon called name */
{

    PrebuiltInfo temp;
    if (!builtIn)
	hashBuiltIn();
    temp.name = (char *) name;
    return NXHashGet(builtIn, &temp);
}

static IconInfo *fetchIcon(const char *name)
 /* fetches icon from static hashtable */
{
    IconInfo temp;

    if (!hashTable)
	return NULL;
    temp.name = (char *) name;
    return NXHashGet(hashTable, &temp);
}

static id findBitmap(const char *iconName, id factory)
 /*
  * TYPE: Querying 
  *
  * You sometimes call this method. This method returns the bitmap object
  * associated with iconName.  If it does not exist, a default bitmap is
  * returned. 
  */
{
    IconInfo *iptr = fetchIcon(iconName);
    Bitmap *bitmap = 0;

    if (iptr)
	return iptr->bitmap;
    else {
	PrebuiltInfo *         icon;

	icon = checkBuiltInIcons(iconName);
	if (icon) {
	    extern id NXSharedIcons, NXSharedCursors;
	    if (icon->alpha) {
		bitmap = [factory newRect:&icon->rect type:NX_ALPHABITMAP
			    window:NXSharedCursors];
	    } else {
		bitmap = [factory newRect:&icon->rect type:NX_NOALPHABITMAP
			    window:NXSharedIcons];
	    }
	    addIconId(iconName, bitmap);
	    bitmap->_builtIn = icon - builtInIcons;
	    bitmap->iconName = icon->name;
	} else {
	    bitmap = _NXBuildBitmapFromMacho("__TIFF", iconName, factory);
	    if (!bitmap)
		bitmap = _NXBuildBitmapFromMacho("__ICON", iconName, factory);
	    if (bitmap)
		addIconId(iconName, bitmap);
	}
    }
    return bitmap;
}

extern id _NXBuildBitmapFromMacho(
 const char *segmentName, const char *sectionName, id factory)
{
    id                  bitmap = nil;
    NXStream           *stream = NULL;

    stream = _NXOpenStreamOnMacho(segmentName, sectionName);
    if (!stream)
	goto nope;
    bitmap = buildBitmapFromStream(stream, factory);
nope:
    if (stream)
	NXCloseMemory(stream, NX_FREEBUFFER);
    return bitmap;
}

static id buildBitmapFromFile(const char *filename, id factory)
{
    id                  bitmap = nil;
    NXStream           *stream = 0;

    if (!filename)
	goto nope;
    if (!(stream = NXMapFile(filename, NX_READONLY)))
	goto nope;
    bitmap = buildBitmapFromStream(stream, factory);
  nope:
    if (stream)
	NXCloseMemory(stream, NX_FREEBUFFER);
    return bitmap;
}


static id buildBitmapFromStream(NXStream *stream, id factory)
{
    void               *image = 0, *alpha = 0;
    int                 imageSize, malloced = 0;
    id                  bitmap = nil;
    NXTIFFInfo          _info;
    register NXTIFFInfo *info = &_info;
    register NXImageInfo *imageInfo = &info->image;

    NXSeek(stream, 0, NX_FROMSTART);
    imageSize = NXGetTIFFInfo(0, stream, info);
    if (!imageSize)
	goto nope;
	/*
	 * There's some code here to deal with the special case
	 * of uncompressed TIFF files with one strip of data.
	 */
    if (info->stripsPerImage == 1 && info->compression == 1 /* ??? && and this is a memory stream */ ) {
		if (imageInfo->samplesPerPixel == 2 && 
			imageInfo->planarConfig == NX_PLANAR) {
			image = stream->buf_base + info->stripOffsets[0];
			alpha = stream->buf_base + info->stripOffsets[1];
		} else if (imageInfo->samplesPerPixel == 1 || 
					imageInfo->planarConfig == NX_MESHED) {
			image = stream->buf_ptr;
		}
    }
    if (!image) {
		NXSeek(stream, 0, NX_FROMSTART);
		if (!(image = NXReadTIFF(0, stream, info, NULL))) goto nope;
		malloced = YES;
    }

    bitmap = [factory newSize:(NXCoord)imageInfo->width
			     :(NXCoord)imageInfo->height
	      type:(imageInfo->samplesPerPixel == 2 || imageInfo->samplesPerPixel == 4) ?
	      NX_ALPHABITMAP : NX_NOALPHABITMAP];
    if (alpha)
	[bitmap image:image withAlpha:alpha width:imageInfo->width
	 height:imageInfo->height bps:imageInfo->bitsPerSample];
    else
	[bitmap image:image toRect:NULL 
			width:imageInfo->width height:imageInfo->height
	 		bps:imageInfo->bitsPerSample spp:imageInfo->samplesPerPixel 
			config:imageInfo->planarConfig 
			interp:imageInfo->photoInterp];
  nope:
    if (malloced)
	free(image);
    return bitmap;
}


static BOOL iconFromTiff(const char *name, const char *filename, id factory)
{
    id                  bitmap = nil;
    BOOL                retval = NO;

    if (!filename)
	goto nope;
    if (!(bitmap = buildBitmapFromFile(filename, factory)))
	goto nope;
    retval = addIconId(name, bitmap);
  nope:
    return retval;
}

static BOOL iconFromMacho(
    const char *name, const char *segmentName, const char *sectionName,
    id factory)
{
    id                  bitmap = nil;
    BOOL                retval = NO;

    if (!sectionName)
	goto nope;
    if (!(bitmap = _NXBuildBitmapFromMacho(segmentName, sectionName, factory)))
	goto nope;
    retval = addIconId(name, bitmap);
  nope:
    return retval;
}


static BOOL addIconData(const char *name, char *bitmapData,
	    int intWidth, int intHeight, int bps, int spp,
	    id factory)
 /*
  * this Method creates a new icon called name.  bitmapData is a pointer to
  * the bitmap image. The bitmaps are store as an array of bytes, where each
  * byte contains 4 two bit pixels, where pixel = 0 is black, and pixel = 3
  * is white.  width and height are the dimensions of the bitmap. 
  */
{
    IconInfo *iptr;

    iptr = fetchIcon(name);
    if (iptr && iptr->original)
	return NO;		/* already exists */
    if (iptr && !bitmapData)
	return YES;		/* another null entry */
    if (bitmapData) {
	id                  bitmap;

	bitmap = [Bitmap newSize:(NXCoord)intWidth :(NXCoord)intHeight
		  type:(spp == 2) ? NX_ALPHABITMAP : NX_NOALPHABITMAP];

	[bitmap image:bitmapData width:intWidth height:intHeight bps:bps spp:spp];

	if (iptr) {		/* replace null entry */
	    iptr->bitmap = bitmap;
	    iptr->width = intWidth;
	    iptr->height = intHeight;
	    iptr->original = YES;
	} else
	    enterIcon(name, bitmap, intWidth, intHeight, YES);	/* create new entry */
    } else {
	IconInfo *sq16;	/* create null entry */

	findBitmap("square16H", factory);
	sq16 = fetchIcon("square16H");
	enterIcon(name, sq16->bitmap, sq16->width, sq16->height, NO);
    }
    return YES;
}


static BOOL addIconId(const char *name, Bitmap *bitmapObj)
 /*
  * this Method creates a new icon called name.  bitmapObj is a Bitmap object
  * that has been imaged or painted into by the client. 
  */
{
    IconInfo *iptr;
    int                 w, h;
    NXRect              bitRect;

    iptr = fetchIcon(name);
    if (iptr && iptr->original)
	return NO;		/* already exists */
    [bitmapObj _getFrame:&bitRect];
    w = (int)ceil(bitRect.size.width);
    h = (int)ceil(bitRect.size.height);
    if (iptr) {			/* replace null entry */
	iptr->bitmap = bitmapObj;
	iptr->width = w;
	iptr->height = h;
	iptr->original = YES;
    } else
	enterIcon(name, bitmapObj, w, h, YES);	/* create new entry */
    return YES;
}


static void getIconSize(const char *name, NXSize *size, id factory)
 /*
  * gets the NXSize of the icon called name 
  */
{
    IconInfo *iptr = fetchIcon(name);

    if (!iptr) {
	findBitmap(name, factory);
	iptr = fetchIcon(name);
    }
    if (iptr) {
	size->width = (NXCoord)iptr->width;
	size->height = (NXCoord)iptr->height;
    };
    return;
}


static void enterIcon(
    const char *name, Bitmap *bitmap, int width, int height, BOOL original)
 /* enters an element into the static hashtable */
{
    IconInfo *iptr;
    PrebuiltInfo *dptr;

    if (!hashTable)
	hashTable = NXCreateHashTable(iconProto, 0, 0);
    if (!builtIn)
	hashBuiltIn();
    iptr = (IconInfo *) malloc(sizeof(IconInfo));
    iptr->width = width;
    iptr->height = height;
    iptr->bitmap = bitmap;
    iptr->original = original;
    dptr = checkBuiltInIcons(name);
    if (dptr) {
	iptr->name = dptr->name;
	bitmap->iconName = dptr->name;
    } else { 
	iptr->name = malloc(strlen(name) + 1);
	strcpy(iptr->name, name);
	bitmap->iconName = iptr->name;
    }
    NXHashInsert(hashTable, iptr);
}

static void removeIcon(const char *name)
{
    IconInfo temp, *remove;
    if (!hashTable)
	return;
    temp.name = (char *) name;
    remove = NXHashRemove(hashTable, &temp);
    if (remove)
	free(remove);
}



extern void _NXRefreshSize(char *name)
 /*
  * jmh ---- used when the bitmap size has been changed by someone else. It
  * resets the width and height of the icon to the one of its bitmap. 
  */
{
    IconInfo *iptr;
    NXRect              bitRect;

    iptr = fetchIcon(name);
    if (iptr) {
	[iptr->bitmap _getFrame:&bitRect];
	iptr->width = (int)ceil(bitRect.size.width);
	iptr->height = (int)ceil(bitRect.size.height);
    }
    return;
}


void _NXComplement(const unsigned char *src, unsigned char *dest, int size)
{
    const unsigned char      *last = src + size;
    const unsigned int       *bigSrc;
    unsigned int	     *bigDest;

    if (size % sizeof(unsigned int) == 0 &&
		(unsigned int)src % sizeof(unsigned int) == 0 &&
		(unsigned int)dest % sizeof(unsigned int) == 0) {
	bigSrc = (unsigned int *)src;
	bigDest = (unsigned int *)dest;
	while (bigSrc < (unsigned int *)last)
	    *bigDest++ = ~(*bigSrc++);
    } else
	while (src < last)
	    *dest++ = ~(*src++);
}


/*
 * Big hack-o for writing/reading bitmaps.  We used to write out just
 * the width, height and bps properties for the image.  Now we write
 * out a zero where bps used to be, and then the real bps, the number
 * of colors, the planar config and whether it has alpha.  All of
 * this is mirrored on reading.
 */

/* Although the following function is private, NIB uses it! */
void _NXWriteBitmapBits(NXTypedStream * s, id object, NXSize size)
{
    int                 height, width, bps, spp;
    int                 channelBytes, totalBytes;
    unsigned char      *data[5];
    int                 photoInt, planarConfig;


    int                 i;
    int                 zero = 0;
    NXRect              srcRect;

    [object lockFocus];
    srcRect.origin.x = srcRect.origin.y = 0.0;
    srcRect.size = size;
    NXSizeBitmap(&srcRect, &totalBytes, &width, &height,
		 &bps, &spp, &planarConfig, &photoInt);
    channelBytes = totalBytes / spp;
    NX_MALLOC(data[0], unsigned char, totalBytes);
    for (i = 1; i < spp; i++)
	data[i] = data[i - 1] + channelBytes;
    NXWriteTypes(s, "iii", &width, &height, &zero);
    NXWriteTypes(s, "iiii", &bps, &spp, &planarConfig, &photoInt);
    NXReadBitmap(&srcRect, width, height, bps, spp, planarConfig, photoInt,
		 data[0], data[1], data[2], data[3], data[4]);
    for (i = 0; i < spp; i++)
	NXWriteArray(s, "c", channelBytes, data[i]);
    [object unlockFocus];
    NX_FREE(data[0]);
}


/* Although the following function is private, NIB uses it! */
void _NXReadBitmapBits(NXTypedStream * s, id object)
{
    int                 height, width, bps, spp;
    int                 channelBytes;
    int                 planarConfig, photoInt;
    int                 i;
    unsigned char      *data[5];
    NXRect              destRect;

    NXReadTypes(s, "iii", &width, &height, &bps);
    if (!bps)
	NXReadTypes(s, "iiii", &bps, &spp, &planarConfig, &photoInt);
    else {
	spp = 1;
	planarConfig = NX_PLANAR;
	photoInt = NX_MONOTONICMASK;
    }
    channelBytes = height * (((width * bps) + 7) / 8);
    NX_MALLOC(data[0], unsigned char, spp * channelBytes);
    for (i = 1; i < spp; i++)
	data[i] = data[i - 1] + channelBytes;
    for (i = 0; i < spp; i++)
	NXReadArray(s, "c", channelBytes, data[i]);
    [object lockFocus];
    destRect.origin.x = destRect.origin.y = 0.0;
    destRect.size.width = width;
    destRect.size.height = height;
    NXImageBitmap(&destRect, width, height, bps, spp, planarConfig, photoInt,
		  data[0], data[1], data[2], data[3], data[4]);
    [object unlockFocus];
    NX_FREE(data[0]);
}

static id getWindow(BitmapManager *managerPtr)
{
    return managerPtr->window;
}


/*
Modifications (starting at 0.8):
  
 7/19/88 wrp 	made default flip state be flipped.  This makes image 
 		 and readimage work more consistently. 
12/10/88 trey	Added printing support to bitmaps.
 9/11/88 wrp	Added composite:fromRect:toPoint: method.
 1/27/89 bs	Added read: and write:.
 2/13/88 trey	Added private support to read bitmaps from shlib segment
		Added +newFromStream:
 2/16/89 pah	Add: popup, popupH, defaultappicon, editing, pulldown,
		 and pulldownH built-in icons
  
 2/24/89 trey	added methods to read alpha (unimplemented in 0.82)
 3/14/89 trey	implemented read alpha methods
		archives by writing all data and alpha
		-sizeImage returns the real spp
		-setAlphaToData nuked (PS operator gone)
 5/26/89 bgy	used hashtable.h instead of private hash

0.93
----
 6/13/89 trey	added _NXOpenStreamOnMacho
 6/14/89 bgy	added BitmapN functionality, removed BitmapMgr.
		 this adds resizable bitmaps, managements of offscreen bitmaps
		 freeing of unique bitmaps
 6/15/89 pah	make name method return const char *

0.94
----
 7/07/89 trey	fixed dealing printing of bitmaps with alpha to correctly
		 adjust spp
 7/08/89 trey	changed complement to _NXComplement, and optimized it to
		 work in 4 byte words if possible
		composite: now chooses origin correctly in scaled views
 7/16/89 trey	readImage: hardwires spp to 1
 7/16/89 bryan	writeTiff: reads alpha if it exists
 7/31/89 wrp	fixed bug 1868, 'free' now does not free system bitmaps.

77
--
 2/26/90 aozer	Added image:toRect:width:height:bps:spp:planarConfig: in a
		 (quick) attempt to get Bitmap to handle more forms of data and
		 fixed buildBitmapFromStream to use above method if needed.
 2/27/90 aozer	Generalized the code in buildBitmapFromStream to delay reading 
		 data from (uncompressed, 1 strip) TIFF files to color.
  3/4/90 aozer	Got rid of the MegaPixel assumptions in imageSize:, readImage:, 
		 readImage:withAlpha:, etc. Added the imageSize: method that
		 returns planarConfig and photoInterp values.
79
--
 3/20/90 bgy	added some built-in bitmaps for the text object's rulers.
 		 these bitmaps have alpha, and are stored with the cursors.
 3/27/90 aozer	Added the factory method _findBuiltInBitmap: to allow searching
		for built in bitmaps (by NXImage).

80
--
 4/9/90 aozer	Added screen:NULL to the Window creation code

83
--
 5/1/90 aozer	Added the factory method _findExistingBitmap: to let NXImage 
		find existing named bitmaps.

85
--
 5/16/90 aozer	Changed _findBuiltInBitmap: so that it just returns prebuilt
		Bitmap info rather than creating the named Bitmap.

87
--
 6/29/90 aozer	Fixed Bitmap leak: Builtin Bitmaps were not being freed
		properly in finishUnarchiving (due to fix to bug 1868).

91
--
 8/11/90 glc	New icons for text ruler
 8/12/90 aozer	Changed WININIT from 256 to 48 and WINGROW from 256 to 64.

95
--
 10/1/90 aozer	Moved auto & manual down to cut down shared window size. Too
		bad we can't get rid of them! (Maybe in 3.0.)

*/

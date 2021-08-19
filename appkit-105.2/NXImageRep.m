/*
	NXImageRep.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXImageRep_Private.h"
#import "NXImage_Private.h"
#import "imageWraps.h"
#import "publicWraps.h"
#import "appkitPrivate.h"
#import <defaults.h>
#import "errors.h"
#import "graphics.h"
#import "nextstd.h"
#import "tiff.h"
#import "tiffPrivate.h"
#import "View.h"
#import <mach.h>
#import <sys/loader.h>
#import <string.h>
#import <limits.h>
#import <objc/List.h>
#import <dpsclient/wraps.h>

#define SIZEOK(size) (((size)->width > 0.0) && ((size)->height > 0.0))

/* 
 * ??? Need to implement the search through the various image directories.
 */
static NXStream *_NXOpenStreamOnImageFile(const char *fileName)
{
    return NXMapFile(fileName, NX_READONLY);
}

NXStream *_NXOpenStreamOnMacho(const char *segName, const char *sectName)
{
    const char *data;
    int size;

    if (segName && sectName &&
	(data = getsectdata((char *)segName, (char *)sectName, &size))) {
	return NXOpenMemory(data, size, NX_READONLY);
    } else {
	return NULL;
    }
}

NXStream *_NXOpenStreamOnSection(const char *segName, const char *sectName)
{
    NXStream *stream = NULL;

    if (!(stream = _NXOpenStreamOnMacho(segName, sectName))) {
	if (NXArgv[0][0] == '/') {
	    char fullPath[MAXPATHLEN+1];
	    strcpy(fullPath, NXArgv[0]);
	    *(rindex(fullPath, '/')+1) = 0;
	    strcat(fullPath, sectName);
	    stream = NXMapFile(fullPath, NX_READONLY);
	} else {
	    stream = NXMapFile(sectName, NX_READONLY);
	}
    }	    

    return stream;
}

static BOOL checkHeaderMagic(NXStream *mapped) {
    struct mach_header	mhp;
    if (NXRead(mapped, &mhp, sizeof(struct mach_header)) != 
						sizeof(struct mach_header)) {
	return NO;
    }
    return (mhp.magic == MH_MAGIC) && (mhp.filetype == MH_EXECUTE) && 
	   (mhp.sizeofcmds <= mapped->buf_size);
}

/*
 * This function tries to open a stream on another file's MachO.
 * executable is a stream on the file; open it before invoking this function.
 * (Use NXMapFile()). 
 */
NXStream *_NXOpenStreamOnMachoOfFile (const char *segName, const char *sectName, NXStream *app) 
{
    if (checkHeaderMagic(app)) {
	struct mach_header *mhp = (struct mach_header *)(app->buf_base);
	const struct section *sect = getsectbynamefromheader(mhp,
					(char *)segName, (char *)sectName);
	if (sect) {
	    /*
	     * Make sure the file is intact (ie, the section is all there).
	     */
	    NXSeek(app, 0, NX_FROMEND);
	    if (NXTell (app) >= sect->offset + sect->size) {
		return NXOpenMemory((const char *) mhp + sect->offset, 
				    sect->size, NX_READONLY);
	    }
	}
   }
   return NULL;
}

/*
 * Come in with *stream and *appStream set to NULL.
 * This function returns with YES or NO if a stream could be opened
 * on the specified icon of the specified app. The caller should
 * close *appStream if it is not NULL.
 */
static BOOL getIconInFile (NXStream **stream, NXStream **appStream,
			   const char *iconName, const char *appName)
{
    *appStream = NULL;
    if (appName && strlen(appName)) {
	if (*appStream = NXMapFile(appName, NX_READONLY)) {
	    if (!(*stream = _NXOpenStreamOnMachoOfFile(NX_ICONSEGMENT,
						iconName, *appStream))) {
		NXCloseMemory (*appStream, NX_FREEBUFFER);
		*appStream = NULL;
	    }
	}
    } else {
	*stream = _NXOpenStreamOnMacho(NX_ICONSEGMENT, iconName);
    }
    return *stream != NULL;
}

/*
 * This routine will open a stream.
 *
 * from determines the source (file, section, or icon)
 * If from file, use fileName. If from section, use segName as segment
 * name and fileName as section name. If from icon, use appName as file
 * name and fileName as icon name; ignore segName. If appName is NULL, then
 * use the current file.
 *
 * *stream & *appStream are pointers to NXStreams; appStream may be NULL
 * if from is not LAZYFROMICON. Caller is responsible for closing both
 * stream & appStream (if they are not NULL).
 */
static BOOL openStream (NXStream **stream, NXStream **appStream, int from, 
		const char *fileName, const char *segName, const char *appName)
{
    *stream = NULL;
    if (appStream) {
	*appStream = NULL;
    }
    switch (from) {
	case LAZYFROMICON:
	    return getIconInFile (stream, appStream, fileName, appName);
	case LAZYFROMSECTION:
	    *stream = _NXOpenStreamOnSection(segName, fileName);
	    return *stream ? YES : NO;
	case LAZYFROMFILE:
	    *stream = _NXOpenStreamOnImageFile(fileName);
	    return *stream ? YES : NO;
#ifdef DEBUG
	default:
	    NXLogError ("openStream in NXImageRep called with bad from\n");
#endif
    }
    return NO;	    
}

/*
 * Functions/macros to be used in determining if vm_copy can be used
 * on the contents of a stream...
 */
BOOL _NXIsMemoryStream (NXStream *stream)
{
    static const struct stream_functions *memFunctions = NULL;
    if (!memFunctions) {
	NXStream *memStream = NXOpenMemory (NULL, 0, NX_READONLY);
	memFunctions = memStream->functions;
	NXCloseMemory (memStream, NX_FREEBUFFER);
    }
    return (stream->functions == memFunctions);
}

#define LARGEENOUGHFORVMCOPY(len) ((len) > vm_page_size / 2)
#define NUMVMPAGES(len) ((int)((len + vm_page_size - 1) / vm_page_size))
#define VALLOC(len, zone) \
	(LARGEENOUGHFORVMCOPY((len)) ? valloc(len) : NXZoneMalloc((zone), (len)))

/*
 * This function tries to copy the data in the stream to a memory a buffer
 * as efficiently as possible. copyOffset is the place to copy from in the
 * stream, copyLen is the number of bytes to copy.  *dest is the memory 
 * buffer allocated for the copy (allocated by THIS function), and *destOffset
 * is the offset in this buffer where the data is copied to.
 *
 * On a system without real Mach, where vm_allocate()
 * is really malloc(), the following code might hurt some; in that case
 * define VMCOPYNOTEFFICIENT.
 *
 * The following code used to use malloc(), which 
 * once returned page-aligned memory if the request was for a big
 * enough chunk.  Under 2.0, malloc() will probably not do this, so it's more
 * appropriate to call valloc(), which returns page-aligned memory.
 * If your machine does not have valloc(), you might want to change it to
 * vm_allocate().  Should be real simple.
 *
 * Function returns number of pages actually allocated and copied.
 */
static short vmCopyFromStream(NXStream *stream, int copyOffset, int copyLen,
			     unsigned char **dest, short *destOffset)
{
    int numPagesCopied = 0;

#ifndef VMCOPYNOTEFFICIENT
    if (LARGEENOUGHFORVMCOPY(copyLen) && _NXIsMemoryStream(stream)) {
	char *data, *copyFrom, *mem;
	int length, maxlen, numPages;

	NXGetMemoryBuffer(stream, &data, &length, &maxlen);
	data += copyOffset;

	copyFrom = (char *)((((int)data) / vm_page_size) * vm_page_size);
	numPages = NUMVMPAGES(((int)data) + copyLen - ((int)copyFrom));

	mem = valloc(numPages * vm_page_size);

	if ((((int)mem) % vm_page_size) == 0) {
	    if (vm_copy(task_self(), (vm_address_t)copyFrom,
				numPages * vm_page_size,
				(vm_address_t)mem) == KERN_SUCCESS) {
		*destOffset = (int)(data - copyFrom);
		*dest = (unsigned char *)mem;
		numPagesCopied = numPages;
	    } else {
		NX_FREE(mem);
#ifdef DEBUG
		NXLogError("Bad vm_copy in vmCopyFromStream "
			    "copy %d pages from %x to %x\n",
			    numPages, copyFrom, *dest);
#endif
	    }
	}
    }
#endif

    return numPagesCopied;
}

#define NUMBYTES(n) (((n) + 7) / 8)

@implementation NXImageRep

- setSize:(const NXSize *)aSize
{
    size = *aSize;
    return self;
}

- getSize:(NXSize *)aSize
{
    *aSize = size;
    return self;
}

- (BOOL)drawAt:(const NXPoint *)point
{
    if (!SIZEOK(&size)) {
	return NO;
    }
    if ((point->x != 0.0) || (point->y != 0.0)) {
	PStranslate(point->x, point->y);
    }
    return [self draw];
}

- (BOOL)drawIn:(const NXRect *)rect
{
    if (!SIZEOK(&size)) {
	return NO;
    }
    if ((NX_X(rect) != 0.0) || (NX_Y(rect) != 0.0)) {
	PStranslate(NX_X(rect), NX_Y(rect));
    }
    if ((NX_WIDTH(rect) != size.width) ||
	(NX_HEIGHT(rect) != size.height)) {
	PSscale ((NX_WIDTH(rect)/size.width), (NX_HEIGHT(rect)/size.height));
    }
    return [self draw];
}

- (BOOL)draw
{
    return YES;
}

- setNumColors:(int)anInt
{
    _repFlags.numColors = anInt;
    return self;
}

- (int)numColors
{
    return _repFlags.numColors;
}

- setAlpha:(BOOL)flag
{
    _repFlags.hasAlpha = flag ? 1 : 0;
    return self;
}

- (BOOL)hasAlpha
{
    return _repFlags.hasAlpha ? YES : NO;
}

- setBitsPerSample:(int)anInt
{
    _repFlags.bitsPerSample = anInt;
    return self;
}

- (int)bitsPerSample
{
    return _repFlags.bitsPerSample;
}

- setPixelsWide:(int)anInt
{
    _pixelsWide = anInt;
    return self;
}

- (int)pixelsWide
{
    return _pixelsWide;
}

- setPixelsHigh:(int)anInt
{
    _pixelsHigh = anInt;
    return self;
}

- (int)pixelsHigh
{
    return _pixelsHigh;
}

#ifndef DISABLEASYNCHIMAGING
- (BOOL)_asynchOK
{
    return NO;
}
#endif

- (BOOL)_canDrawOutsideOfItsBounds
{
    return YES;
}

- read:(NXTypedStream *)stream
{
    unsigned short hasAlpha, numColors, bitsPerSample, dataSource;

    [super read:stream];
    NXReadSize (stream, &size);
    NXReadTypes (stream, "iissss", &_pixelsWide, &_pixelsHigh,
		 &hasAlpha, &numColors, &bitsPerSample, &dataSource);
    _repFlags.hasAlpha = hasAlpha;
    _repFlags.numColors = numColors;
    _repFlags.bitsPerSample = bitsPerSample;
    _repFlags.dataSource = dataSource;
    return self;
}

- write:(NXTypedStream *)stream
{
    unsigned short hasAlpha, numColors, bitsPerSample, dataSource;
    hasAlpha = _repFlags.hasAlpha;
    numColors = _repFlags.numColors;
    bitsPerSample = _repFlags.bitsPerSample;
    dataSource = _repFlags.dataSource;

    [super write:stream];
    NXWriteSize (stream, &size);
    NXWriteTypes (stream, "iissss", &_pixelsWide, &_pixelsHigh,
		  &hasAlpha, &numColors, &bitsPerSample, &dataSource);
    return self;
}

@end



@implementation NXBitmapImageRep

/*
 * It's evil to create instances of NXBitmapImageRep without any
 * fields, so we make sure none get created...
 */
+ new { return [self notImplemented:_cmd]; }
- init { return [self notImplemented:_cmd]; }

/*
 * No support for bits per pixel in 2.0.
 */
#define NOBITSPERPIXEL
	
/*
 * Values for _repFlags.dataSource
 */
#define NOTLAZY 0	/* This is actually indicated by NULL lazyInfo */
#define LAZYFROMFILE 1
#define LAZYFROMSECTION 2
#define LAZYFROMICON 3

#define ISLAZY (_repFlags.dataSource != NOTLAZY)

/*
 * The following tries to make sure the data
 * is loaded.  Call with the error value to return if the
 * data cannot be loaded.
 */
#define LOADDATA(errorReturn) \
    if ((ISLAZY) && \
	(_repFlags.dataLoaded == NO) && (![self _loadData])) \
	return (errorReturn);

#define PHOTOINTERP \
    (((_colorSpace == NX_CMYKColorSpace) ? 3 : _colorSpace) | \
     (_repFlags.hasAlpha ? NX_ALPHAMASK : 0))

#define COLORSPACEFROMPHOTO(photoInt) \
    (((photoInt & 3) == 3) ? NX_CMYKColorSpace : (photoInt & 3))

#define SAMPLESPERPIXEL \
    (NUMCOLORS + (HASALPHA == 1 ? 1 : 0))

/*
 * The following macro will create the five plane pointers in the array given
 * as argument.  Call in a method. Typical use might be:
 *	unsigned char *pointers[5]; ... SETUPDATAPTRS(pointers)
 *
 * If ISPLANAR is false, then _data points to the only plane of data.
 * If ISPLANAR is true but _moreRepFlags.explicitPlanes is FALSE,
 * then _data points to the first plane; other planes follow the first one.
 * If ISPLANAR and _moreRepFlags.explicitPlanes are both true, then _data
 * points to an array of 5 plane pointers.
 */
#define SETUPDATAPTRS(ptr) \
    {int cnt, bpp = [self bytesPerPlane]; for (cnt = 0; cnt < 5; cnt++) \
	if (ISPLANAR) ptr[cnt] = _moreRepFlags.explicitPlanes ? *(((unsigned char **)_data)+cnt) : (_data + bpp*cnt) ; \
	else ptr[cnt] = (cnt==0 ? _data : NULL);}

/* Macros to access info about the rep */

#define BITSPERSAMPLE	(_repFlags.bitsPerSample)
#define ISPLANAR	(_moreRepFlags.isPlanar)
#define PIXELSWIDE	_pixelsWide
#define PIXELSHIGH	_pixelsHigh
#define BYTESPERROW	_bytesPerRow
#define COLORSPACE	(_colorSpace)
#define HASALPHA	(_repFlags.hasAlpha)
#define NUMCOLORS	(_repFlags.numColors)
#ifndef NOBITSPERPIXEL
#define BITSPERPIXEL	_bitsPerPixel
#endif

+ newData:(unsigned char *)data
     pixelsWide:(int)width
     pixelsHigh:(int)height
     bitsPerSample:(int)bps
     samplesPerPixel:(int)spp
     hasAlpha:(BOOL)hasAlpha
     isPlanar:(BOOL)isPlanar
     colorSpace:(NXColorSpaceType)colorSpace
{
    return [self newData:data
		pixelsWide:width
		pixelsHigh:height
		bitsPerSample:bps
		samplesPerPixel:spp
		hasAlpha:hasAlpha
		isPlanar:isPlanar
		colorSpace:colorSpace
		bytesPerRow:0
		bitsPerPixel:0];
}

+ newData:(unsigned char *)data
     pixelsWide:(int)width
     pixelsHigh:(int)height
     bitsPerSample:(int)bps
     samplesPerPixel:(int)spp
     hasAlpha:(BOOL)hasAlpha
     isPlanar:(BOOL)isPlanar
     colorSpace:(NXColorSpaceType)colorSpace
     bytesPerRow:(int)rowBytes
     bitsPerPixel:(int)pixelBits
{
     return [[self allocFromZone:NXDefaultMallocZone()]	
		initData:data
		pixelsWide:width
		pixelsHigh:height
		bitsPerSample:bps
		samplesPerPixel:spp
		hasAlpha:hasAlpha
		isPlanar:isPlanar
		colorSpace:colorSpace
		bytesPerRow:rowBytes
		bitsPerPixel:pixelBits];
}

+ newDataPlanes:(unsigned char **)planes
     pixelsWide:(int)width
     pixelsHigh:(int)height
     bitsPerSample:(int)bps
     samplesPerPixel:(int)spp
     hasAlpha:(BOOL)hasAlpha
     isPlanar:(BOOL)isPlanar
     colorSpace:(NXColorSpaceType)colorSpace
     bytesPerRow:(int)rowBytes
     bitsPerPixel:(int)pixelBits
{
     return [[self allocFromZone:NXDefaultMallocZone()]	
		initDataPlanes:planes
		pixelsWide:width
		pixelsHigh:height
		bitsPerSample:bps
		samplesPerPixel:spp
		hasAlpha:hasAlpha
		isPlanar:isPlanar
		colorSpace:colorSpace
		bytesPerRow:rowBytes
		bitsPerPixel:pixelBits];
}

- initData:(unsigned char *)data
     pixelsWide:(int)width
     pixelsHigh:(int)height
     bitsPerSample:(int)bps
     samplesPerPixel:(int)spp
     hasAlpha:(BOOL)hasAlpha
     isPlanar:(BOOL)isPlanar
     colorSpace:(NXColorSpaceType)colorSpace
     bytesPerRow:(int)rowBytes
     bitsPerPixel:(int)pixelBits
{
    unsigned char *planes[5] = {data, NULL, NULL, NULL, NULL};
    return [self initDataPlanes:planes
		pixelsWide:width
		pixelsHigh:height
		bitsPerSample:bps
		samplesPerPixel:spp
		hasAlpha:hasAlpha
		isPlanar:isPlanar
		colorSpace:colorSpace
		bytesPerRow:rowBytes
		bitsPerPixel:pixelBits];
}
 
- initDataPlanes:(unsigned char **)planes
     pixelsWide:(int)width
     pixelsHigh:(int)height
     bitsPerSample:(int)bps
     samplesPerPixel:(int)spp
     hasAlpha:(BOOL)hasAlpha
     isPlanar:(BOOL)isPlanar
     colorSpace:(NXColorSpaceType)colorSpace
     bytesPerRow:(int)rowBytes
     bitsPerPixel:(int)pixelBits
{
    short nColorsForColorSpace[6] = {1, 1, 3, -1, -1, 4};

    if ((bps != 1 && bps != 2 && bps != 4 && bps != 8) ||
	width < 1 || height < 1 || spp < 1 || spp > 5 ||
	colorSpace < 0 || colorSpace > 5 ||
	nColorsForColorSpace[colorSpace] != (spp - (hasAlpha ? 1 : 0))) {
	NXLogError ("Inconsistent set of values to create NXBitmapImageRep\n");
	[self free];
	return nil;
    }

    if (pixelBits && (pixelBits != (bps * (isPlanar ? 1 : spp)))) {
	NXLogError ("NXBitmapImageRep: Bad bitsPerPixel\n");
	[self free];
	return nil;
    }

    [super init];
    _repFlags.dataSource = NOTLAZY;
    _repFlags.dataLoaded = YES;
    BYTESPERROW = rowBytes ? rowBytes :
		  NUMBYTES(bps * width * (isPlanar ? 1 : spp));
#ifndef NOBITSPERPIXEL
    BITSPERPIXEL = pixelBits ? pixelBits : bps * (isPlanar ? 1 : spp);
#endif
    PIXELSHIGH = height;
    PIXELSWIDE = width;
    BITSPERSAMPLE = bps;
    NUMCOLORS = spp - (hasAlpha ? 1 : 0);
    ISPLANAR = isPlanar;
    HASALPHA = hasAlpha;
    COLORSPACE = colorSpace;

    if (planes && planes[0]) {
	_memory = NULL;
	if (isPlanar && planes[1]) {
	    int cnt;
	    _moreRepFlags.explicitPlanes = YES;
	    NX_ZONEMALLOC([self zone], ((unsigned char **)_data), unsigned char *, 5);
	    for (cnt = 0; cnt < SAMPLESPERPIXEL; cnt++) {
		((unsigned char **)(_data))[cnt] = planes[cnt];
	    }
	} else {
	    _data = planes[0];
	}
    } else {
	int bytesNeeded = [self bytesPerPlane] * [self numPlanes];		    
	NX_ZONEMALLOC([self zone], _memory, unsigned char, bytesNeeded);
	_data = _memory;
    }

    {NXSize bitmapSize = {width, height}; [self setSize:&bitmapSize];}

    return self;
}

+(int)sizeImage:(const NXRect *)rect
{
    int bytes, width, height, bps, spp, config, photo;

    NXSizeBitmap(rect, &bytes, &width, &height, &bps, &spp, &config, &photo);
    return bytes;
}

+(int)sizeImage:(const NXRect *)rect
    pixelsWide:(int *)width
    pixelsHigh:(int *)height
    bitsPerSample:(int *)bps
    samplesPerPixel:(int *)spp
    hasAlpha:(BOOL *)hasAlpha
    isPlanar:(BOOL *)isPlanar
    colorSpace:(NXColorSpaceType *)colorSpace
{
    int bytes, config, photo;

    NXSizeBitmap(rect, &bytes, width, height, bps, spp, &config, &photo);

    *hasAlpha = (photo & NX_ALPHAMASK) ? YES : NO;
    *isPlanar = (config == NX_PLANAR);
    *colorSpace = COLORSPACEFROMPHOTO(photo);

    return bytes;
}

- initData:(unsigned char *)data fromRect:(const NXRect *)rect
{
    int bytes, width, height, bps, spp, config, photo;
    unsigned char *plane[5];
    NXSize bitmapSize;

    NXSizeBitmap(rect, &bytes, &width, &height, &bps, &spp, &config, &photo);

    if (!([self initData:data
		    pixelsWide:width
		    pixelsHigh:height
		    bitsPerSample:bps
		    samplesPerPixel:spp
		    hasAlpha:((photo & NX_ALPHAMASK) ? YES : NO)
		    isPlanar:(config == NX_PLANAR)
		    colorSpace:COLORSPACEFROMPHOTO(photo)
		    bytesPerRow:0
		    bitsPerPixel:0])) {
	return nil;
    }

    SETUPDATAPTRS(plane);
    NXReadBitmap(rect, width, height, bps, spp, config, photo,
		 plane[0], plane[1], plane[2], plane[3], plane[4]);

    bitmapSize.width = width;
    bitmapSize.height = height;
    [self setSize:&bitmapSize];

    return self;
}  

+ readImage:(const NXRect *)rect
    into:(unsigned char *)data
{
    return [[self alloc] initData:data fromRect:rect];
}


/*
 * If the writeTIFF: methods are given streams that are not positioned at
 * location zero, then they assume that the TIFF image is being appended
 * to an existing one.  Thus the stream better be read/write!
 */
- writeTIFF:(NXStream *)stream
{
    return [self writeTIFF:stream usingCompression:NX_TIFF_COMPRESSION_NONE andFactor:0.0];
}

- writeTIFF:(NXStream *)stream usingCompression:(int)compression
{
    return [self writeTIFF:stream usingCompression:compression andFactor:0.0];
}


- writeTIFF:(NXStream *)stream usingCompression:(int)compression andFactor:(float)compressionFactor
{
    int err;
    TIFF *tiff;
    float xRes = 0.0, yRes = 0.0;

    LOADDATA (self);

    if (!(tiff = _NXTIFFOpenStreamWithMode (stream,
			NXTell(stream) ? NX_TIFFAPPENDMODE : NX_TIFFWRITEMODE,
			&err))) {
	NX_RAISE(NX_tiffError, (void *)err, NULL);
    }

    if ((size.width > 0.0) && (PIXELSWIDE > 0) &&
	(size.height > 0.0) && (PIXELSHIGH > 0)) {
	xRes = (PIXELSWIDE / size.width) * 72.0;
	yRes = (PIXELSHIGH / size.height) * 72.0;
    }

    NX_DURING
	unsigned char *data[5];
	SETUPDATAPTRS(data);
	if (err = _NXTIFFWrite (tiff, data,
				PIXELSWIDE, PIXELSHIGH,
				ISPLANAR, HASALPHA, COLORSPACE, 
				SAMPLESPERPIXEL, BITSPERSAMPLE,
				compression,
				xRes, yRes, (xRes > 0.0 ? RESUNIT_INCH : 0), 
				NULL, compressionFactor)) {
	    NX_RAISE(NX_tiffError, (void *)err, NULL);
	}
    NX_HANDLER
	_NXTIFFClose (tiff);
	NX_RERAISE();
    NX_ENDHANDLER

    _NXTIFFClose (tiff);
   
    return self;
}

- writeEPS:(NXStream *)stream
{
    return self;
}

/*
 * Getting back lists of NXBitmapImageReps.
 */

static void addToList (id *list, id item, int listSize, NXZone *zone)
{
    if (!*list) {
	*list = [[List allocFromZone:zone] initCount:listSize];
    }
    [*list addObject:item];
}

/*
 * Public method; gets all images in a TIFF file and returns them in a list.
 * Not lazy.
 */
+ (List *)newListFromStream:(NXStream *)stream
{
    return [self newListFromStream:stream zone:NXDefaultMallocZone()];
}

+ (List *)newListFromStream:(NXStream *)stream zone:(NXZone *)zone
{
    TIFF *tiff = _NXTIFFOpenForRead (stream);
    id list = nil;

    if (tiff) {
	int cnt, numDirs = _NXTIFFNumDirectories(tiff);
	for (cnt = 0; cnt < numDirs; cnt++) {
	    id rep = [[self allocFromZone:zone]
			_initFromTIFF:tiff imageNumber:cnt];
	    if (rep) {
		addToList(&list, rep, numDirs, zone);
	    }
	}
	_NXTIFFClose(tiff);
    }
    return list;
}

/*
 * Returns a list of images read from the TIFF file.
 * Only the TIFF header information is read in; the data is left alone.
 * Come in with stream properly positioned at the top of a TIFF file.
 */
+ (List *)_newListFromStream:(NXStream *)stream
	fileName:(const char *)fileName
	from:(int)dataSource
	zone:(NXZone *)zone
{
    TIFF *tiff = _NXTIFFOpenForRead (stream);
    int cnt, numImages;
    id list = nil, rep;

    if (tiff) {
	numImages = _NXTIFFNumDirectories(tiff);
	for (cnt = 0; cnt < numImages; cnt++) {
	    /*
	     * Because opening the TIFF sets the directory to zero, no need
	     * to call _NXTIFFSetDirectory() for cnt==0.
	     */
	    if (((cnt == 0) || _NXTIFFSetDirectory(tiff, cnt)) &&
		(rep = [[self allocFromZone:zone] _initFrom:fileName
			from:dataSource imageNumber:cnt tiff:tiff])) {
		addToList(&list, rep, numImages, zone);
	    }
	}
	_NXTIFFClose(tiff);
    }
    return list;
}

/*
 * These next two methods are public covers for reading lists of TIFF images
 * from files or sections.
 */
+ (List *)newListFromFile:(const char *)fileName 
{
    return [self newListFromFile:fileName zone:NXDefaultMallocZone()];
}

+ (List *)newListFromFile:(const char *)fileName zone:(NXZone *)zone
{
    NXStream *stream = NULL;
    id list = nil;

    if (stream = _NXOpenStreamOnImageFile(fileName)) {
	list = [self _newListFromStream:stream
			fileName:fileName
			from:LAZYFROMFILE
			zone:zone];
	NXCloseMemory (stream, NX_FREEBUFFER);
    }
    return list;
}

+ (List *)newListFromSection:(const char *)fileName 
{
    return [self newListFromSection:fileName zone:NXDefaultMallocZone()];
}

+ (List *)newListFromSection:(const char *)fileName zone:(NXZone *)zone
{
    NXStream *stream = NULL;
    id list = nil;

    if (stream = _NXOpenStreamOnSection(NX_TIFFSEGMENT, fileName)) {
	list = [self _newListFromStream:stream
			fileName:fileName
			from:LAZYFROMSECTION
			zone:zone];
	NXCloseMemory (stream, NX_FREEBUFFER);
    }
    return list;
}

/*
 * Come in with app = NULL to check the current executable.
 */
+ (List *)_newListFromIcon:(const char *)icon
		inApp:(const char *)app zone:(NXZone *)zone
{
    NXStream *stream = NULL, *appStream = NULL;
    List *list = nil;

    if (openStream (&stream, &appStream, LAZYFROMICON, icon, NULL, app)) {
	int cnt;
	list = [self _newListFromStream:stream
			    fileName:icon
			    from:LAZYFROMICON
			    zone:zone];
	for (cnt = [list count]-1; cnt >= 0; cnt--) {
	    NXBitmapImageRep *item = (NXBitmapImageRep *) [list objectAt:cnt];
	    item->_otherName = app ? NXCopyStringBufferFromZone(app, zone):NULL;
	}
	NXCloseMemory (stream, NX_FREEBUFFER);
	if (appStream) NXCloseMemory (appStream, NX_FREEBUFFER);
    }
    return list;
}

/*
 * This method will set the various variables in the instance from the
 * specified tiff. This method won't fail; the tiff is assumed to be valid.
 */
- _getFromTIFF:(TIFF *)tiff
{
    TIFFDirectory *td = &(tiff->tif_dir);

    BYTESPERROW = _NXTIFFScanlineSize(tiff);
    PIXELSWIDE = td->td_imagewidth;
    PIXELSHIGH = td->td_imagelength;
    BITSPERSAMPLE = td->td_bitspersample;
    ISPLANAR = (td->td_planarconfig == NX_PLANAR) ? YES : NO;
    HASALPHA = td->td_matteing == 0 ? NO : YES;
    NUMCOLORS = td->td_samplesperpixel - (HASALPHA ? 1 : 0);
#ifndef NOBITSPERPIXEL
    BITSPERPIXEL = BITSPERSAMPLE * (ISPLANAR ? 1 : SAMPLESPERPIXEL);
#endif
    COLORSPACE = (td->td_photometric == 5) ? NX_CMYKColorSpace : (td->td_photometric & 3);

    if (td->td_photometric == PHOTOMETRIC_PALETTE) {
	COLORSPACE = NX_RGBColorSpace;
	ISPLANAR = NO;
	NUMCOLORS = 3;
	BYTESPERROW = _NXTIFFScanlineSize(tiff) * 3;
    }

    {
	NXSize bitmapSize = {PIXELSWIDE, PIXELSHIGH};
	if (_NXTIFFFieldSet(tiff, FIELD_RESOLUTION) &&
	    (td->td_xresolution > 0) && (td->td_yresolution > 0)) {
	    bitmapSize.width *= (72.0 / td->td_xresolution);
	    bitmapSize.height *= (72.0 / td->td_yresolution);
	    if (td->td_resolutionunit == RESUNIT_CENTIMETER) {
		bitmapSize.width /= 2.54;	/* this is cm per inch */
		bitmapSize.height /= 2.54;
	    } /* Other res units would go here, when defined */
	}
	[self setSize:&bitmapSize];
    }

    return self;    
}

/* Various covers for creating lazy instances of NXBitmapImageRep. */

+ newFromSection:(const char *)fName
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromSection:fName];
}

- initFromSection:(const char *)fName
{
    return [self _initFrom:fName from:LAZYFROMSECTION imageNumber:0];
}

+ newFromFile:(const char *)fName
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromFile:fName];
}

- initFromFile:(const char *)fName
{
    return [self _initFrom:fName from:LAZYFROMFILE imageNumber:0];
}

/*
 * This method is called to initialize lazy instances of NXBitmapImageRep.
 * This method either returns self or nil to indicate success; if it returns
 * nil the instance is actually freed.
 */
- _initFrom:(const char *)fileName from:(int)dataSource imageNumber:(int)num 
{
    NXStream *stream = NULL;
    BOOL success = NO;

    if (openStream (&stream, NULL, dataSource,
		    fileName, NX_TIFFSEGMENT, NULL)) {

	TIFF *tiff = _NXTIFFOpenForRead (stream);
	if (tiff) {
	    if ((num == 0) || _NXTIFFSetDirectory(tiff, num)) {
		[super init];
		success = ([self _initFrom:fileName 
			    from:dataSource
			    imageNumber:num
			    tiff:tiff] != nil);
	    }
	    _NXTIFFClose(tiff);
	}	    
        NXCloseMemory(stream, NX_FREEBUFFER);
    }

    if (!success) {
	[self free];
	self = nil;
    }	
    return self;
}

/*
 * The following method is the method through which all lazy instances of
 * NXImageBitmapRep are created.
 */
- _initFrom:(const char *)fileName
	from:(int)from
	imageNumber:(int)num
	tiff:(TIFF *)tiff
{
    [self _getFromTIFF:tiff];
    _fileName = NXCopyStringBufferFromZone(fileName, [self zone]);
    _repFlags.dataLoaded = NO;
    _repFlags.dataSource = from;
    _imageNumber = num;
    return self;
}

/*
 * The next two methods allow you to create non-lazy instances of 
 * NXBitmapImageRep.
 */
+ newFromStream:(NXStream *)stream
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromStream:stream];
}

/*
 * initFromStream will free the instance and return nil if the stream
 * does not contain a valid file.
 */
- initFromStream:(NXStream *)stream
{
    TIFF *tiff = _NXTIFFOpenForRead (stream);
    if (tiff) {
	self = [self _initFromTIFF:tiff imageNumber:0];
	_NXTIFFClose(tiff);
	return self;
    } else {
	[self free];
	return nil;  
    }
}

/*
 * Method through which all non-lazy instances of NXBitmapImageRep are
 * initialized. This method will free the instance and return nil if 
 * the tiff file is bad for some reason.
 */
- _initFromTIFF:(TIFF *)tiff imageNumber:(int)num;
{
    [super init];
    if ([self _loadFromTIFF:tiff imageNumber:num]) {
	_repFlags.dataSource = NOTLAZY;
	_repFlags.dataLoaded = YES;
	return self;
    } else {
	[self free];
	return nil;
    }
}

/*
 * This is the real workhorse for loading TIFF images.
 */
- (BOOL)_loadFromTIFF:(TIFF *)tiff imageNumber:(int)num
{
    TIFFDirectory *td = &tiff->tif_dir;

    if (!_NXTIFFSetDirectory (tiff, num)) {
	return NO;
    }

    _data = NULL;	/* Will be set if we can vm_copy() the data */

    if ((td->td_compression == COMPRESSION_NONE) &&
	(td->td_stripsperimage == 1) &&
	LARGEENOUGHFORVMCOPY(td->td_stripbytecount[0]) &&
	(td->td_photometric == 0 || td->td_photometric == 1 || 
	 td->td_photometric == 2 || td->td_photometric == 5)) {

	short offset;

	if (td->td_planarconfig == NX_PLANAR) {
	    int cnt, min = INT_MAX, max = -1, numBytes;

	    /*
	     * Should we vmcopy()? If there are big gaps within the strips,
	     * then maybe not.
	     */	   
	    for (cnt = 0; cnt < td->td_samplesperpixel; cnt++) {
		if (td->td_stripoffset[cnt] < min) {
		    min = td->td_stripoffset[cnt];
		}
		if (td->td_stripoffset[cnt] > max) {
		    max = td->td_stripoffset[cnt]+td->td_stripbytecount[cnt];
		}
	    }
	    numBytes = max - min;
	    if ((numBytes < 
		    (td->td_stripbytecount[0]*(td->td_samplesperpixel+1))) &&
		(_memoryPages = vmCopyFromStream(tiff->tif_fd, min, numBytes,
						 &_memory, &offset))) {
		_moreRepFlags.explicitPlanes = YES;
		NX_ZONEMALLOC([self zone], 
			      ((unsigned char **)_data), unsigned char *, 5);
		for (cnt = 0; cnt < td->td_samplesperpixel; cnt++) {
		    ((unsigned char **)_data)[cnt] =
			_memory + offset + td->td_stripoffset[cnt];
		}
	    }
	} else {
	    if (_memoryPages = vmCopyFromStream(tiff->tif_fd,
						td->td_stripoffset[0],
						td->td_stripbytecount[0],
						&_memory, &offset)) {
		_data = _memory + offset;
	    }
	}
    }

    /*
     * If we can't do that, then do ahead and read the data anyway.
     * We have to remember to free the pointer when we are done with it.
     * When we use _NXTIFFReadFromZone() to read the data, the data comes nicely
     * packed with the planes following each other in the planar case; so we
     * don't need to worry about storing explicit plane pointers.
     */
    if (!_data) {
	if (!(_memory = _NXTIFFReadFromZone(tiff, NULL, [self zone]))) {
	    return NO;
	}
	_data = _memory;
    }

    [self _getFromTIFF:tiff];

    return YES;
}

/*
 * The following two methods, _loadData & _forgetData, are called for
 * lazy TIFF images.
 */
- (BOOL)_loadData
{
    NXStream *stream = NULL, *appStream = NULL;
    TIFF *tiff = NULL;

    if (openStream (&stream, &appStream, _repFlags.dataSource,
		    _fileName, NX_TIFFSEGMENT, _otherName)) {
	if (tiff = _NXTIFFOpenForRead (stream)) {
	    _repFlags.dataLoaded = [self _loadFromTIFF:tiff 
					  imageNumber:_imageNumber];
	    _NXTIFFClose(tiff);
	}
	NXCloseMemory (stream, NX_FREEBUFFER);
	if (appStream) NXCloseMemory (appStream, NX_FREEBUFFER);
    }

    return _repFlags.dataLoaded;
}

- _forgetData
{
    if (ISLAZY && _repFlags.dataLoaded) {
	if (_memory) {
	    NX_FREE(_memory);
	    _memory = NULL;
	}
	_repFlags.dataLoaded = NO;
    }
    return self;
}

/*
 * Call _NXRemoveAlpha only if the image contains an alpha channel.
 * This function will take data+alpha (where data is premultiplied towards
 * black) and premultiply the data towards white and throw the alpha away.
 *
 * destData points to an array of 5 pointers pointing to nothing.
 * srcData contains the source image.
 * The rest of the values define srcData.
 *
 * Don't be scared by the size of this function; it's just a bunch of distinct
 * if statements for the different cases:
 *
 * 	planar, gray, 1, 2, 4, or 8 bps (most common case for monochrome)
 *	planar, color, 1, 2, 4, or 8 bps (not so common)
 *	meshed, 8 bps, color or gray (most common case for color)
 *	meshed, 1, 2, or 4 bps, color or gray (common) this one's messy
 *
 * This function will return NO if it can't remove the alpha. If it returns YES,
 * then be sure to free destData[0] after you are done.
 */
BOOL _NXRemoveAlpha(unsigned char *destData[5], unsigned char *srcData[5],
		    int width, int height, int bps, int spp,
		    int rowBytes, BOOL isPlanar)
{
    int destRowBytes = NUMBYTES(width * bps);
    int destPlaneBytes = destRowBytes * height;
    int destSpp = spp - 1;
    int rowCnt, cnt;
    unsigned char *destMemory;

    if ((bps != 1 && bps != 2 && bps != 4 && bps != 8) || 
	(spp != 2 && spp != 4 && spp != 5)) {
	return NO;
    }

    NX_ZONEMALLOC(NXDefaultMallocZone(), 
		  destMemory, unsigned char, destPlaneBytes * destSpp);

    if (isPlanar) {

	register int byteCnt, planeCnt;

	if (destSpp == 1) {		/* Monochrome planar data */

	    register unsigned char *alpha, *src, *dest = destMemory;
	    for (rowCnt = 0; rowCnt < height; rowCnt++) {
		src = srcData[0] + rowBytes * rowCnt;
		alpha = srcData[1] + rowBytes * rowCnt;
		for (byteCnt = 0; byteCnt < destRowBytes; byteCnt++) {
		    *dest++ =  (*src++) + ~(*alpha++);
		}
	    }

	} else {			/* Color planar data */

	    unsigned char *src[5], *dest[5];
	    for (planeCnt = 0; planeCnt < destSpp; planeCnt++) {
		dest[planeCnt] = destMemory + destPlaneBytes * planeCnt;
	    }
	    for (rowCnt = 0; rowCnt < height; rowCnt++) {
		for (planeCnt = 0; planeCnt < destSpp+1; planeCnt++) {
		    src[planeCnt] = srcData[planeCnt] + rowBytes * rowCnt;
		}
		for (byteCnt = 0; byteCnt < destRowBytes; byteCnt++) {
		    register unsigned char curAlpha = ~(*(src[destSpp])++);
		    /* Could unroll this loop but this is a rare case? */
		    for (planeCnt = 0; planeCnt < destSpp; planeCnt++) {
			*(dest[planeCnt])++ = curAlpha + *(src[planeCnt])++;
		    }
		}
	    }

	}	

    } else {	/* Meshed */

	register int sampleCnt;
	int pixelCnt;

	if (bps == 8) {	 /* This should be a special case. */

	    register unsigned char *src, *dest = destMemory;
	    for (rowCnt = 0; rowCnt < height; rowCnt++) {
		src = srcData[0] + rowBytes * rowCnt;
		for (pixelCnt = 0; pixelCnt < width; pixelCnt++) {
		    register unsigned char curAlpha = ~(*(src+destSpp));
		    for (sampleCnt = 0; sampleCnt < destSpp; sampleCnt++) {
			*dest++ = (*src++) + curAlpha;
		    }
		    src++;
		}
	    }

	} else {	/* less than 8 bits per sample meshed */

#define ADVSRC \
    if ((src.bit += bps) == 8) INITSRC(src.addr + 1) 
#define INITSRC(new) \
    {src.addr = (new); src.bit = 0; curSrc = *(src.addr);}		
#define ADVDEST \
    if ((dest.bit += bps) == 8) {*(dest.addr)=curDest; INITDEST(dest.addr+1);}
#define INITDEST(new) \
    {dest.addr = (new); dest.bit = 0; curDest = *(dest.addr);}		
#define GETSRC \
    ((curSrc & mask[src.bit]) >> (8 - src.bit - bps))
#define PUTDEST(val) \
    curDest = (((val) << (8 - dest.bit - bps)) & mask[dest.bit]) | \
		(curDest & ~mask[dest.bit]) 

	    /* curDest & curSrc are 1-byte buffers for dest & src */

	    unsigned char mask[8], data[4];
	    register unsigned char curSrc, curDest, curAlpha;
	    struct {unsigned char *addr; int bit;} src, dest;

	    /* Create the mask for accessing individual samples. */

	    for (sampleCnt = 0; sampleCnt < 8; sampleCnt += bps) {
		mask[sampleCnt] = ((1 << bps) - 1) << (8 - sampleCnt - bps);
	    }

	    INITDEST (destMemory);

	    for (rowCnt = 0; rowCnt < height; rowCnt++) {
		INITSRC (srcData[0] + rowBytes * rowCnt);
		for (pixelCnt = 0; pixelCnt < width; pixelCnt++) {
		    /* Worth unrolling for the 3 spp case? Hmm. No. */
		    for (sampleCnt = 0; sampleCnt < destSpp; sampleCnt++) {
			data[sampleCnt] = GETSRC; ADVSRC;
		    }
		    curAlpha = ~GETSRC; ADVSRC;    
		    for (sampleCnt = 0; sampleCnt < destSpp; sampleCnt++) {
			PUTDEST(data[sampleCnt] + curAlpha); ADVDEST;
		    }
		}
		/* Flush out the rest of the last byte */
		while (dest.bit) {
		    PUTDEST(0); ADVDEST;
		}
	    }
	}
    }

    for (cnt = 0; cnt < destSpp; cnt++) {
	destData[cnt] = destMemory + destPlaneBytes * cnt;
    }

    return YES;

}

- (BOOL)drawIn:(const NXRect *)rect
{
    LOADDATA (NO);
    return [super drawIn:rect];
}

- (BOOL)draw
{
    NXRect rect = {{0.0, 0.0}, {0.0, 0.0}};
    int imagingPhotoInterp, imagingSpp;
#ifdef REMOVEALPHAATNXIMAGELEVEL
    unsigned char *data[5];
#endif
    unsigned char *noAlpha[5];

    LOADDATA(NO);

    rect.size = size;

#ifdef REMOVEALPHAATNXIMAGELEVEL
    if ((NXDrawingStatus == NX_PRINTING) && HASALPHA) {

	SETUPDATAPTRS(data);
	if (_NXRemoveAlpha(noAlpha, data,
			PIXELSWIDE, PIXELSHIGH,
			BITSPERSAMPLE, SAMPLESPERPIXEL,
			BYTESPERROW, ISPLANAR)) {
	    imagingPhotoInterp = PHOTOINTERP & ~NX_ALPHAMASK;
	    imagingSpp = SAMPLESPERPIXEL - 1;
	}

    } else 
#endif

    {

	SETUPDATAPTRS(noAlpha);
	imagingPhotoInterp = PHOTOINTERP;
	imagingSpp = SAMPLESPERPIXEL;

    }

    _NXImageBitmapWithFlip (&rect,
			    PIXELSWIDE, PIXELSHIGH,
			    BITSPERSAMPLE, imagingSpp,
			    ISPLANAR ? NX_PLANAR : NX_MESHED,
			    imagingPhotoInterp, noAlpha[0], noAlpha[1], 
			    noAlpha[2], noAlpha[3], noAlpha[4], 
			    NO);

#ifdef REMOVEALPHAATNXIMAGELEVEL
    if (imagingSpp == SAMPLESPERPIXEL - 1) {
	NX_FREE(noAlpha[0]);
    }
#endif

    return YES;
}

- (unsigned char *)data
{
    LOADDATA (NULL);
    if (_moreRepFlags.explicitPlanes) {
	return ((unsigned char **)_data)[0];
    } else {
	return _data;
    }
}

- getData:(unsigned char **)data
{
    return [self getDataPlanes:data];
}

- getDataPlanes:(unsigned char **)planes
{
    LOADDATA (self);
    SETUPDATAPTRS(planes);
    return self;
}

- (BOOL)isPlanar
{
    return ISPLANAR;
}

    
- (int)samplesPerPixel
{
    return SAMPLESPERPIXEL;
}

- (int)bytesPerRow
{
    return BYTESPERROW;
}

- (int)bitsPerPixel
{
#ifndef NOBITSPERPIXEL
    return BITSPERPIXEL;
#else
    return BITSPERSAMPLE * (ISPLANAR ? 1 : SAMPLESPERPIXEL);
#endif
}

- (int)bytesPerPlane
{
    return BYTESPERROW * PIXELSHIGH;
}

- (int)numPlanes
{
    return (ISPLANAR ? SAMPLESPERPIXEL : 1);
}
    
- (NXColorSpaceType)colorSpace
{
    return COLORSPACE;
}

- (BOOL)_canDrawOutsideOfItsBounds
{
    return NO;
}

- read:(NXTypedStream *)stream
{
    [super read:stream];
    NXReadType (stream, "s", &_moreRepFlags);
    if (ISLAZY) {
	NXReadType (stream, "*", &_fileName);
	if (_repFlags.dataSource == LAZYFROMICON) {
	    NXReadType (stream, "*", &_otherName);
	}
	_repFlags.dataLoaded = NO;
    } else {
	int plane, numPlanes = [self numPlanes];
	NXReadTypes (stream, "is", &BYTESPERROW, &COLORSPACE);
	NX_ZONEMALLOC([self zone], 
		_memory, unsigned char, [self bytesPerPlane] * numPlanes);
	_memoryPages = 0;
	_data = _memory;
	for (plane = 0; plane < numPlanes; plane++) {
	    unsigned char *data = _memory + plane * [self bytesPerPlane];
	    NXReadArray (stream, "c", [self bytesPerPlane], data);
	}
    }
    return self;
}

- write:(NXTypedStream *)stream
{
    [super write:stream];
    NXWriteType (stream, "s", &_moreRepFlags);
    if (ISLAZY) {
	NXWriteType (stream, "*", &_fileName);
	if (_repFlags.dataSource == LAZYFROMICON) {
	    NXWriteType (stream, "*", &_otherName);
	}
    } else {
	int plane, numPlanes = [self numPlanes];
	unsigned char *data[5];
	SETUPDATAPTRS(data);
	NXWriteTypes (stream, "is", &BYTESPERROW, &COLORSPACE);
	for (plane = 0; plane < numPlanes; plane++) {
	    NXWriteArray (stream, "c", [self bytesPerPlane], data[plane]);
	}
    }
    return self;
}

- free
{
    if (_memory) {
	NX_FREE (_memory);
    }
    if (_fileName) {
	NX_FREE (_fileName);
    }
    if (_otherName) {
	NX_FREE (_otherName);
    }
    if (_moreRepFlags.explicitPlanes) {
	NX_FREE((unsigned char **)_data);
    }
    return [super free];
}

/*
 * copy for NXBitmapImageReps is tricky ---
 * _memory points to the block of data if the data was allocated by the
 * NXBitmapImageRep itself.  _data either points to the single plane of data
 * or to an array of 5 pointers to the individual planes (if explicitPlanes
 * flag is set). If _memory is NULL, then the data can be shared! 
 */
- copy
{
    NXBitmapImageRep *new = [super copy];
    unsigned char *planes[5];
    int cnt;

    SETUPDATAPTRS(planes);

    new->_fileName =
    	_fileName ? NXCopyStringBufferFromZone(_fileName, [new zone]) : NULL;
    new->_otherName =
    	_otherName ? NXCopyStringBufferFromZone(_otherName, [new zone]) : NULL;

    if (!_memory) {
	if (_moreRepFlags.explicitPlanes) {
	    NX_ZONEMALLOC([new zone], 
		((unsigned char **)(new->_data)), unsigned char *, 5);
	    for (cnt = 0; cnt < SAMPLESPERPIXEL; cnt++) {
		((unsigned char **)(new->_data))[cnt] = planes[cnt];
	    }
	}
	return new;
    }
	    
    /* If we come this far, we know that _memory is not NULL and we
     * need to copy the data pointed to by _memory. First try vm_copy()...
     */
    if (_memoryPages) {
	new->_memory = valloc(_memoryPages * vm_page_size);
	if (vm_copy(task_self(), (vm_address_t)_memory,
		    _memoryPages * vm_page_size,
		    (vm_address_t)(new->_memory) == KERN_SUCCESS)) {
	    if (_moreRepFlags.explicitPlanes) {
		NX_ZONEMALLOC([new zone],
			((unsigned char **)(new->_data)), unsigned char *, 5);
		for (cnt = 0; cnt < SAMPLESPERPIXEL; cnt++) {
		    ((unsigned char **)(new->_data))[cnt] =
				(planes[cnt] - _memory) + (new->_memory);
		}
	    } else {
		new->_data = _data - _memory + new->_memory;
	    }
	    _memoryPages = new->_memoryPages;
	} else {
	    NX_FREE(new->_memory);
	    new->_memoryPages = 0;
	}
    }

    /* If new->_memoryPages is zero, then it means the data can't be 
     * vm_copy()ed; just do a simple copy instead.
     */
    if (!new->_memoryPages) {
	int bytesNeeded = [self bytesPerPlane] * [self numPlanes];		    
	NX_ZONEMALLOC([new zone], new->_memory, unsigned char, bytesNeeded);
	for (cnt = 0; cnt < [self numPlanes]; cnt++) {	
	    bcopy(planes[cnt], new->_memory + [self bytesPerPlane] * cnt,
		  [self bytesPerPlane]);
	}
	new->_moreRepFlags.explicitPlanes = NO;
	new->_data = new->_memory;
    }

    return new;
}

- (const char *)_fileName
{
    return _fileName;
}

- (const char *)_appName
{
    return _otherName;
}

@end


@implementation NXEPSImageRep

/*
 * It's evil to create instances of NXEPSImageRep without any
 * fields, so we make sure none get created...
 */
+ new { return [self notImplemented:_cmd]; }
- init { return [self notImplemented:_cmd]; }

+ newFromStream:(NXStream *)stream
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromStream:stream];
}

- initFromStream:(NXStream *)stream
{
    [super init];
    if (![self _loadFromStream:stream]) {
	[self free];
	return nil;
    }
    _repFlags.dataSource = NOTLAZY;
    _repFlags.dataLoaded = YES;
    return self;
}

- _initFrom:(const char *)fileName inApp:(const char *)appName
      from:(int)dataSource
{
    NXStream *stream, *appStream;
    NXRect bBox;
    BOOL status = NO;

    if (openStream(&stream, &appStream, dataSource,
		   fileName, NX_EPSSEGMENT, appName)) {
	status = _NXEPSGetBoundingBox(stream, &bBox);
	NXCloseMemory (stream, NX_FREEBUFFER);
	if (appStream) NXCloseMemory (appStream, NX_FREEBUFFER);
    }

    if (!status) {
	[self free];
	return nil;
    }

    [super init];
    _bBoxOrigin = bBox.origin;
    size = bBox.size;
    _fileName = NXCopyStringBufferFromZone(fileName, [self zone]);
    _otherName =
    	appName ? NXCopyStringBufferFromZone(appName, [self zone]) : NULL;
    _repFlags.dataSource = dataSource;
    _repFlags.dataLoaded = NO;
    return self;
}

+ (List *)newListFromFile:(const char *)fileName 
{
    return [self newListFromFile:fileName zone:NXDefaultMallocZone()];
}

+ (List *)newListFromFile:(const char *)fileName zone:(NXZone *)zone
{
    id rep = [[self allocFromZone:zone] initFromFile:fileName];
    List *list = nil;
    if (rep) {
	addToList (&list, rep, 1, zone);
    }
    return list; 
}

+ (List *)newListFromSection:(const char *)fileName 
{
    return [self newListFromSection:fileName zone:NXDefaultMallocZone()];
}

+ (List *)newListFromSection:(const char *)fileName zone:(NXZone *)zone
{
    id rep = [[self allocFromZone:zone] initFromSection:fileName];
    List *list = nil;
    if (rep) {
	addToList (&list, rep, 1, zone);
    }
    return list; 
}

/*
 * Come in with app = NULL to check the current executable.
 */
+ (List *)_newListFromIcon:(const char *)icon
		inApp:(const char *)app zone:(NXZone *)zone
{
    id rep = [[self allocFromZone:zone]
			_initFrom:icon inApp:app from:LAZYFROMICON];
    List *list = nil;
    if (rep) {
	addToList (&list, rep, 1, zone);
    }
    return list; 
}

+ (List *)newListFromStream:(NXStream *)stream
{
    return [self newListFromStream:stream zone:NXDefaultMallocZone()];
}

+ (List *)newListFromStream:(NXStream *)stream zone:(NXZone *)zone
{
    id rep = [[self allocFromZone:zone] initFromStream:stream];
    List *list = nil;
    if (rep) {
	addToList (&list, rep, 1, zone);
    }
    return list; 
}


+ newFromSection:(const char *)fName
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromSection:fName];
}

- initFromSection:(const char *)fName
{
    return [self _initFrom:fName inApp:NULL from:LAZYFROMSECTION];
}

+ newFromFile:(const char *)fName
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromFile:fName];
}

- initFromFile:(const char *)fName
{
    return [self _initFrom:fName inApp:NULL from:LAZYFROMFILE];
}

- free
{
    if (_memory) {
	NX_FREE (_memory);
    }
    if (_fileName) {
	NX_FREE (_fileName);
    }
    if (_otherName) {
	NX_FREE (_otherName);
    }
    return [super free];
}

- copy
{
    NXEPSImageRep *new = [super copy];

    new->_fileName =
    	_fileName ? NXCopyStringBufferFromZone(_fileName, [new zone]) : NULL;
    new->_otherName =
    	_otherName ? NXCopyStringBufferFromZone(_otherName, [new zone]) : NULL;
    
    if (_memory) {
	NXStream *stream = NXOpenMemory(_memory + _epsOffset,
					_epsLen, NX_READONLY);
	if (stream) {
	    if (!vmCopyFromStream(stream, 0, _epsLen,
		    &(unsigned char *)(new->_memory), &(new->_epsOffset))) {
		NX_ZONEMALLOC([new zone], new->_memory, char, _epsLen);
		bcopy(_memory, new->_memory, _epsLen);
		new->_epsOffset = 0;
	    }
	    NXCloseMemory (stream, NX_FREEBUFFER);
	}
    }

    return new;
}

/*
 * Next two methods, _loadData and _forgetData, take care of lazy
 * loading & unloading of data.
 */
- (BOOL)_loadData
{
    NXStream *stream = NULL, *appStream = NULL;

    if (openStream (&stream, &appStream, _repFlags.dataSource,
		    _fileName, NX_EPSSEGMENT, _otherName)) {
	_repFlags.dataLoaded = [self _loadFromStream:stream];
	NXCloseMemory (stream, NX_FREEBUFFER);
	if (appStream) NXCloseMemory (appStream, NX_FREEBUFFER);
    }

    return _repFlags.dataLoaded;
}

- _forgetData
{
    if (ISLAZY && _repFlags.dataLoaded) {
	NX_FREE(_memory);
	_memory = NULL;
	_epsLen = 0;
	_repFlags.dataLoaded = NO;
    }
    return self;
}

- getEPS:(char **)epsString length:(int *)epsLength
{
    *epsLength = 0;
    LOADDATA (self);
    *epsString = _memory + _epsOffset;
    *epsLength = _epsLen;
    return self;
}

- getBoundingBox:(NXRect *)rect
{
    rect->size = size;
    rect->origin = _bBoxOrigin;
    return self;
}

- (BOOL)drawIn:(const NXRect *)rect
{
    LOADDATA (NO);
    return [super drawIn:rect];
}

- prepareGState
{
    return self;
}

- (BOOL)draw 
{
    static DPSContext epsContext = NULL;

    LOADDATA (NO);

    if ((_bBoxOrigin.x != 0.0) || (_bBoxOrigin.y != 0.0)) {
	PStranslate(-_bBoxOrigin.x, -_bBoxOrigin.y);
    }

    if (NXDrawingStatus == NX_DRAWING) {
	float c1x, c1y, c2x, c2y;
	float winCTM[6];
	int realWinNum;
	DPSContext curContext = DPSGetCurrentContext();
	NXHandler exception;

	exception.code = 0;

	_NXGetFocus(&c1x, &c1y, &c2x, &c2y, winCTM, &realWinNum);
	if (epsContext == NULL) {
	    epsContext = DPSCreateContext(NXGetDefaultValue([NXApp appName], "NXHost"),
					  NXGetDefaultValue([NXApp appName], "NXPSName"),
					    NULL, NULL);
	/* ??? Get the defaultDepthLimit of old context and apply it to new */
	}
	DPSSetContext(epsContext);
	_NXFocus(realWinNum, winCTM, c1x, c1y, c2x, c2y);

	NX_DURING
	    /*
	     * No need to initialize graphics state --- this context is
	     * always in initialized state (because of save/restore)...
	     * We clear the stack before the restore (to avoid brain-dead EPS
	     * files that leave stuff on the stack to work). When printing,
	     * we remember the stack count and do some popping to accomplish
	     * the same result.
	     */
	    DPSPrintf(epsContext,
			"/__NXEPSSave save def "
			"/__NXEPSDictCount countdictstack def\n");
	    [self prepareGState];
	    DPSWriteData(epsContext, _memory + _epsOffset, _epsLen);
	    DPSPrintf(epsContext,
			"\ncountdictstack __NXEPSDictCount sub {end} "
			"repeat clear __NXEPSSave restore\n");
	    NXPing();
	NX_HANDLER
	    exception = NXLocalHandler;
	NX_ENDHANDLER

	DPSSetContext(curContext);

	if (exception.code != 0) {
	    DPSDestroyContext(epsContext);
	    epsContext = NULL;
	    if (exception.code == dps_err_ps) {
		NXLogError ("Error in EPS file %s\n",
			    _fileName ? _fileName : "");
		NXReportError(&exception);
	    } else if (exception.code) {
		NX_RAISE(exception.code, exception.data1, exception.data2);
	    }
	}

	return (exception.code == 0) ? YES : NO;

    } else {
	NXStream *tmpStream;

	if (tmpStream = NXOpenMemory(_memory+_epsOffset,_epsLen,NX_READONLY)) {
	    _NXEPSDeclareDocumentFonts (tmpStream);
	    NXCloseMemory (tmpStream, NX_SAVEBUFFER);
	}

	_NXEPSBeginDocument (_fileName);
	[self prepareGState];
	DPSWriteData(DPSGetCurrentContext(), _memory+_epsOffset, _epsLen);
	_NXEPSEndDocument ();

	return YES;
    }

}

- (BOOL)_loadFromStream:(NXStream *)stream
{
    NXRect boundingBox;

    NXSeek(stream, 0, NX_FROMSTART);

    if (!_NXEPSGetBoundingBox(stream, &boundingBox)) {
	return NO;
    }

    size = boundingBox.size;
    _bBoxOrigin = boundingBox.origin;

    {
	int len;

	NXSeek(stream, 0, NX_FROMEND);
	len = NXTell(stream);
	NXSeek(stream, 0, NX_FROMSTART);
	if (!vmCopyFromStream(stream, 0, len,
			      &(unsigned char *)_memory, &_epsOffset)) {
	    _epsOffset = 0;
	    if (_memory == NULL) {
		NX_ZONEMALLOC([self zone], _memory, char, len);
	    }
	    NXRead(stream, _memory, len);
	}
	_epsLen = len;
    }

    return YES;
}

#ifndef DISABLEASYNCHIMAGING
- (BOOL)_asynchOK
{
    return NO;
}

- _drawAsynchIn:


- _drawAsynchIn:



#endif

- read:(NXTypedStream *)stream
{
    [super read:stream];
    if (ISLAZY) {
	NXReadType (stream, "*", &_fileName);
	if (_repFlags.dataSource == LAZYFROMICON) {
	    NXReadType (stream, "*", &_otherName);
	}
	_repFlags.dataLoaded = NO;
    } else {
	short dummy;
	NXReadPoint (stream, &_bBoxOrigin);
	NXReadTypes (stream, "si", &dummy, &_epsLen);
	NX_ZONEMALLOC([self zone], _memory, char, _epsLen);
	NXReadArray (stream, "c", _epsLen, _memory);
	_epsOffset = 0;
    }
    return self;
}

- write:(NXTypedStream *)stream
{
    [super write:stream];
    if (ISLAZY) {
	NXWriteType (stream, "*", &_fileName);
	if (_repFlags.dataSource == LAZYFROMICON) {
	    NXWriteType (stream, "*", &_otherName);
	}
    } else {
	short dummy = 0;
	NXWritePoint (stream, &_bBoxOrigin);
	NXWriteTypes (stream, "si", &dummy, &_epsLen);
	NXWriteArray (stream, "c", _epsLen, _memory + _epsOffset);
    }
    return self;
}

- (const char *)_fileName
{
    return _fileName;
}

- (const char *)_appName
{
    return _otherName;
}

@end


@implementation NXCustomImageRep

/*
 * It's evil to create instances of NXCustomImageRep without any
 * fields, so we make sure none get created...
 */
+ new { return [self notImplemented:_cmd]; }
- init { return [self notImplemented:_cmd]; }

+ newDrawMethod:(SEL)aMethod inObject:anObject
{
    return [[self allocFromZone:NXDefaultMallocZone()]
			initDrawMethod:aMethod inObject:anObject];
}

- initDrawMethod:(SEL)aMethod inObject:anObject
{
    [super init];
    drawMethod = aMethod;
    drawObject = anObject;
    return self;
}

- (BOOL)draw
{
    if ((!drawObject) || (![drawObject respondsTo:drawMethod])) {
	return NO;
    }

    [drawObject perform:drawMethod with:self];
    return YES;
}

- read:(NXTypedStream *)stream
{
    [super read:stream];
    drawObject = NXReadObject (stream);
    NXReadType (stream, ":", &drawMethod);
    return self;
}

- write:(NXTypedStream *)stream
{
    [super write:stream];
    NXWriteObjectReference (stream, drawObject);
    NXWriteType (stream, ":", &drawMethod);
    return self;
}

@end

@implementation NXCachedImageRep

/*
 * The private _cache pointer points to a cache window if the cache 
 * belongs to this guy (after a read, for instance). Otherwise _cache
 * is NULL. Use the following macro to make life more pleasant.
 */

#define cacheInfo ((CacheWindowInfo *)_cache)

/*
 * It's evil to create instances of NXCachedImageRep without any
 * fields, so we make sure none get created...
 */
+ new { return [self notImplemented:_cmd]; }
- init { return [self notImplemented:_cmd]; }

+ newFromWindow:(Window *)win rect:(const NXRect *)rect
{
    return [[self allocFromZone:NXDefaultMallocZone()]
			initFromWindow:win rect:rect];
}

- initFromWindow:(Window *)win rect:(const NXRect *)rect
{
    [super init];
    _window = win;
    _origin = rect->origin;
    size = rect->size;
    return self;
}

- _getCacheWindow:(Window **)win andRect:(NXRect *)rect
{
    *win = _window;
    if (rect) {
	rect->origin = _origin;
	rect->size = size;
    }
    return self;
}

- getWindow:(Window **)win andRect:(NXRect *)rect
{
    return [self _getCacheWindow:win andRect:rect];
}

- (BOOL)_canDrawOutsideOfItsBounds
{
    return NO;
}

- (BOOL)draw
{
    NXRect rect = {_origin, size};
    NXBitmapImageRep *bitmap;
    DPSContext printContext = DPSGetCurrentContext();

    if (NXDrawingStatus != NX_DRAWING) {
	DPSSetContext ([NXApp context]);
    }

    PSgsave ();
    NXSetGState (_window ? [_window gState] : _gState);
    bitmap = [NXBitmapImageRep readImage:&rect into:NULL];
    PSgrestore();

    if (NXDrawingStatus != NX_DRAWING) {
	DPSSetContext (printContext);
    }

    if (bitmap) {
	NX_X(&rect) = NX_Y(&rect) = 0.0;
	[bitmap drawIn:&rect];
	[bitmap free];
	return YES;
    }

    return NO;
}

/*
 * Wish we could just use NXBitmapImageRep here --- things would be so much
 * cleaner! However, given the way typedstreams read/write, we can't.
 */

- read:(NXTypedStream *)stream
{
    int bytes, width, height, bps, spp, config, photo, bytesPerPlane, plane;
    unsigned char *data[5];
    BOOL isPlanar, hasAlpha;
    CacheRect cRect;
    NXRect rect;

    [super read:stream];
    NXReadTypes (stream, "iiiiii", &width, &height, &bps,
				   &spp, &config, &photo);
    isPlanar = (config == NX_PLANAR);
    hasAlpha = (photo & NX_ALPHAMASK) != 0;
    bytesPerPlane = height * NUMBYTES(width * bps * (isPlanar ? 1 : spp));
    bytes = bytesPerPlane * (isPlanar ? spp : 1);
    NX_ZONEMALLOC(NXDefaultMallocZone(), data[0], unsigned char, bytes);
    for (plane = 1; plane < (isPlanar ? spp : 1); plane++) {
	data[plane] = data[plane-1] + bytesPerPlane;
    }
    for (plane = 0; plane < (isPlanar ? spp : 1); plane++) {
	NXReadArray(stream, "c", bytesPerPlane, data[plane]);
    }
   
    (void)_NXAllocateImageCache ([self zone], NO, width, height, bps,
				 spp - (hasAlpha ? 1 : 0), hasAlpha, NO,
				 &cacheInfo, &cRect);

    NXSetRect(&rect, cRect.x, cRect.y, cRect.w, cRect.h); 
    _origin = rect.origin;
    size = rect.size;
    _window = cacheInfo->window;
     
    PSgsave();
    NXSetGState ([_window gState]);
    _NXImageBitmapWithFlip (&rect, width, height,
			    bps, spp, config, photo,
			    data[0], data[1], data[2], data[3], data[4], 
			    NO);
    PSgrestore();

    NX_FREE(data[0]);  

    return self;
}

- write:(NXTypedStream *)stream
{
    int bytes, width, height, bps, spp, config, photo, bytesPerPlane, plane;
    unsigned char *data[5];
    NXRect rect = {_origin, size};
    BOOL isPlanar;

    [super write:stream];

    PSgsave();
    NXSetGState (_window ? [_window gState] : _gState);
    NXSizeBitmap(&rect, &bytes, &width, &height, &bps, &spp, &config, &photo);
    NX_ZONEMALLOC(NXDefaultMallocZone(), data[0], unsigned char, bytes);
    isPlanar = (config == NX_PLANAR);
    bytesPerPlane = bytes / (isPlanar ? spp : 1);
    for (plane = 1; plane < (isPlanar ? spp : 1); plane++) {
	data[plane] = data[plane-1] + bytesPerPlane;
    }
    NXWriteTypes (stream, "iiiiii", &width, &height, &bps,
				    &spp, &config, &photo);
    NXReadBitmap(&rect, width, height, bps, spp, config, photo,
		 data[0], data[1], data[2], data[3], data[4]);
    for (plane = 0; plane < (isPlanar ? spp : 1); plane++) {
	NXWriteArray(stream, "c", bytesPerPlane, data[plane]);
    }

    NX_FREE(data[0]);

    PSgrestore();

    return self;
}

- free
{
    if (cacheInfo) {
	CacheRect cRect = {_origin.x, _origin.y, size.width, size.height};
	_NXFreeImageCache (cacheInfo, &cRect);
    }
    return [super free];
}

- copy
{
    NXLogError ("NXCachedImageRep can't copy itself yet.\n");
    return nil;
}

@end


/*

Created: 3/23/90	aozer
  
Modifications (starting at appkit-80):
  
80
--
 4/4/90 aozer	Implemented list-returns (newListFromFile:, etc)
 4/6/90 aozer	Changed to new image description terminology
 4/9/90 aozer	Implemented the +newData:... methods and the methods to
		read/size images from the server.
 4/10/90 aozer	Added draw methods and made drawIn: and drawAt: invoke draw.

82
--
 4/19/90 aozer	Added the ability to parse number of pages in EPS files.
		Don't know how useful this is; EPS files are not supposed to
		have pages.
 4/19/90 aozer	Fixed EPS read/write methods (can't read/write EPS with "*").

83
--
 5/1/90 aozer	Made NXCachedImageRep read/write/draw.

84
--
 5/9/90 aozer	Added _NXRemoveAlpha to allow NXBitmapImageRep to print bitmaps
		with alpha by removing the alpha (premultiply towards white) on
		the client side. Works for all cases but interleaved bps < 8.
 5/14/90 aozer	Fixed NXEPSImageRep bug (it was not closing the stream).

85
--
 5/15/90 aozer	Moved EPS parsing into epsUtil.m.
		No more %%Pages: parsing. (Decided it was a useless feature)
 5/15/90 aozer	NXEPSImageRep now calls _NXEPSDeclareDocumentFonts() while 
		printing.
 5/15/90 aozer	Made DPS context used for drawing EPS hang around between
		draws; only time we kill it is after an error.
 5/18/90 aozer	Made _NXRemoveAlpha() work for interleaved cases with bps < 8.
 5/18/90 aozer	Added value consistency checking to NXBitmapImageRep creation.
		We currently pay no attention to bitsPerPixel.
 5/21/90 aozer	Switched from using NXGetTIFFInfo()/NXReadTIFF() to the more
		efficient _NXTIFFOpen() interface; don't need to open/close
		streams multiple times & can (finally) read resolution values.
 5/22/90 aozer	Efficient vm handling in NXBitmapImageRep & NXEPSImageRep:
		Under certain circumstances, loading data is many times faster
		and/or uses less memory.
 5/22/90 aozer	Made NXEPSImageRep instance vars eps and epsLen into
		_memory & _epsLen.
 6/4/90 aozer	Added -copy to NXEPSImageRep and NXBitmapImageRep.

86
--
 6/6/90	king	Changed %%BeginFile:/%%Endfile that is emitted around
 		EPS files to be %%BeginDocument:/%%EndDocument.
 6/11/90 aozer	Have NXEPSImageRep initialize EPS (see epsUtil.m).  

87
--
 6/21/90 aozer	Got NXEPSImageRep to be more lenient with various forms of 
		illegal EPS files (like those that leave stuff on the op & 
		dictionary stacks & would normally cause invalidrestore
		errors). However, if a program leaves stuff on the exec stack,
		then that will generate an invalidrestore error.
 7/12/90 aozer	Removed NX_UNKNOWNIMAGEPARAMETER, changed NX_MATCHESDEVICE
		to be 0. Changed NXImageRep's read: & write: before too late.
		In 2.0, no support for bitsPerPixel --- pass in the correct
		value or 0 (to denote the correct value).

89
--
 7/26/90 aozer	Got rid of NXIconImageRep class; this adds some code to both
		NXEPSImageRep & NXBitmapImageRep but overall the code is
		smaller and we can now have EPS icons in apps.
 7/30/90 aozer	Added check for truncated executables in 
		_NXOpenStreamOnMachoOfFile().  

90
--
 8/4/90 aozer	Zonified the five classes.

92
--
 8/16/90 aozer	Minor changes to make NXCachedImageRep use Kit-window caches.
 8/17/90 aozer	Removed call to _NXRemoveAlpha() from NXBitmapImageRep's draw
		method to _NXImageBitmapWithFlip(), a lower level beast.

93
--
 8/26/90 aozer	Added writeTIFF:usingCompression:andFactor:

94
--
 9/21/90 aozer	Added initDataPlanes:..., newDataPlanes:, and getDataPlanes:.
		Also fixed up SETUPDATAPOINTERS to deal correctly with this.
 9/25/90 aozer	Removed newFromGState: and initFromGState: from
		NXCachedImageRep.
 9/26/90 aozer	In _loadFromTIFF:imageNumber: made sure we don't use the
		fast vmcopy() code to read in palette-based images.

96
--
 10/4/90 aozer	Added prepareGState.

98
--
 10/15/90 aozer	Added initData:fromRect: as zonified version of readImage:into:

99
--
 10/22/90 aozer	Changed NXBitmapImageRep to self in _newListFromStream:...

*/

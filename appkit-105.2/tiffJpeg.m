/*
	tiffJpeg.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <libc.h>
#import <stdio.h>
#import <zone.h>
#import <mach.h>
#import "appkitPrivate.h"
#import "nextstd.h"
#import "tiffPrivate.h"
#import "tiff.h"
#import "imagemessage.h"

extern int _NXConvertBitmapToJPEG (
    int w, int h, int bps, int spp, BOOL isPlanar, BOOL hasAlpha,
    NXColorSpace colorSpace,
    unsigned char *data[5], int dataLen, BOOL wait,
    unsigned char **jpeg, int *jpegLen, int *memoryLen,
    float compression, NXJPEGInfo *jpegInfo);

extern int _NXConvertJPEGToBitmap (
    int w, int h, int bps, int spp, BOOL isPlanar, BOOL hasAlpha,
    NXColorSpace colorSpace,
    unsigned char *jpeg, int jpegLen, BOOL wait,
    unsigned char **data, int *memoryLen, NXJPEGInfo *jpegInfo);

#define NUMVMPAGES(len) ((int)(((len) + vm_page_size - 1) / vm_page_size))
#define VALLOC(len) valloc(NUMVMPAGES(len) * vm_page_size)

/*
 * Returns the number of bytes necessary to store a tile.
 */
static int _NXTIFFTileSize(TIFF *tif)
{
    TIFFDirectory *td = &tif->tif_dir;
    int tilerow = td->td_bitspersample * td->td_tilewidth;
    if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
	tilerow *= td->td_samplesperpixel;
    }
    tilerow = ((tilerow + 7) / 8) * td->td_tilelength;
    return tilerow;
}

#define PIXELSTOBYTES(td,p) (((p) * (td)->td_bitspersample * (((td)->td_planarconfig == PLANARCONFIG_SEPARATE) ? 1 : (td)->td_samplesperpixel) + 7) / 8)

/*
 * Returns, in pixels, the area that is actually stores 
 * the image in the tile.
 */
static void getImageAreaInTile (TIFF *tif, int tileNum, int *horiz, int *vert)
{
    register TIFFDirectory *td =  &tif->tif_dir;

    *horiz = *vert = 0;

    if ((tileNum / td->td_tilesacross) == (td->td_tilesdown - 1)) {
	*vert = td->td_imagelength % td->td_tilelength;
    }
    if (*vert == 0) {
	*vert = td->td_tilelength;
    }

    if ((tileNum % td->td_tilesacross) == (td->td_tilesacross - 1)) {
	*horiz = td->td_imagewidth % td->td_tilewidth;
    }
    if (*horiz == 0) {
	*horiz = td->td_tilewidth;
    }
}

/*
 * srcW, srcH, destW, and destH are all in pixels.
 * planes, planar, bps, and spp all describe the source
 * destination is destW, destH in size, 8bps, meshed.
 * Need srcW <= destW and srcH <= destH.
 */
static unsigned char *_NXMakeEightBitMeshed (
			    unsigned char *planes[5],
			    BOOL planar, int bps, int spp,
			    int srcW, int srcH, int destW, int destH)
{
    unsigned char *destMem, *dest;
    int x, p, row, cnt;

    unsigned char toEight[256];		/* Convert bps pixel to 8-bit pixel */
    unsigned char mask[8];		/* Mask to extract pixel */
    unsigned char *lastPixel, *lastLine;
    int powTwo = (1 << bps);		/* 2, 4, 16, 256 */
    int factor = 255 / (powTwo-1);	/* 255 / 1,3,15,255 = 255,85,17,1*/
    int curMask;
    int numMasks = 8 / bps;		/* 8, 4, 2, 1 */
    int destRowBytes = destW * spp;

    if (bps == 8 && !planar && srcW == destW && srcH == destH) {
	return planes[0];
    }
	    
    for (cnt = 0; cnt < powTwo; cnt++) {
	toEight[cnt] = factor * cnt;
    }

    for (cnt = powTwo; cnt < 256; cnt *= powTwo) {
	for (x = 0; x < powTwo; x++) {
	    toEight[cnt * x] = toEight[x];
	}
    }
    
    mask[numMasks - 1] = powTwo-1;
    for (cnt = numMasks - 2; cnt >= 0; cnt--) 
	mask[cnt] = mask[cnt + 1] << bps;

    destMem = valloc(spp * destW * destH);	/* Our real memory */
    dest = destMem;				/* Our moving pointer... */
    
    curMask = 0;
    for (row = 0; row < srcH; row++) {
	if (curMask != 0) {
	    curMask = 0;
	    for (p = 0; p < (planar ? spp : 1); p++) planes[p]++;
	}
	for (x = srcW * (planar ? 1 : spp); x > 0; x--) {
	    for (p = 0; p < (planar ? spp : 1); p++) {
		*dest++ = toEight[*planes[p] & mask[curMask]];
	    }
	    curMask = (curMask + 1) % numMasks;
	    if (curMask == 0) {
		    for (p = 0; p < (planar ? spp : 1); p++) planes[p]++;
	    }
	}
	lastPixel = dest - spp;
	for (x = destW - srcW; x > 0; x--) {
	    for (cnt = 0; cnt < spp; cnt++) {
		*dest++ = lastPixel[cnt];		
	    }
	}	    
    }

    lastLine = dest - destRowBytes;

    for (row = srcH; row < destH; row++) {
	bcopy (lastLine, dest, destRowBytes);
	dest += destRowBytes;
    }

    return destMem;
}

/*
 * Low level tile-writing code. buf points at the first byte; rowBytes
 * indicates the number of bytes from one scanline in the tile to the other.
 * tile is the tile number; tiles are ordered left-to-right, top-to-bottom.
 * If planar, the tiles for the first plane are written out before the 2nd.
 *
 * Returns -1 on error.
 */
int _NXTIFFWriteJPEG(register TIFF *tif, u_char *planes[5], float compression)
{
    register TIFFDirectory *td =  &tif->tif_dir;
    int result;
    u_char *tileMem = planes[0];

    if ((td->td_photometric != PHOTOMETRIC_RGB &&
	td->td_photometric != PHOTOMETRIC_MINISBLACK &&
	td->td_photometric != PHOTOMETRIC_MINISWHITE) ||
	(td->td_bitspersample != 4 && td->td_bitspersample != 8)) {
	TIFFError (NULL, "With JPEG only allowed image formats are 8 or 4 bps grayscale or RGB images\n");
	return -1;
    }

    tileMem = _NXMakeEightBitMeshed(planes,
    		td->td_planarconfig == PLANARCONFIG_SEPARATE ? YES : NO,
		td->td_bitspersample,
		td->td_samplesperpixel,
		td->td_imagewidth, td->td_imagelength,
		((td->td_imagewidth+15)/16)*16, ((td->td_imagelength+15)/16)*16);

    /* Set the rest of the parameters. Change COLORSPACE to YUV. */

    if (td->td_photometric == PHOTOMETRIC_RGB) {
	_NXTIFFSetField(tif, TIFFTAG_PHOTOMETRIC, 6 /* YUV */);
    }
    _NXTIFFSetField(tif, TIFFTAG_TILEWIDTH, ((td->td_imagewidth+15)/16)*16);
    _NXTIFFSetField(tif, TIFFTAG_TILELENGTH, ((td->td_imagelength+15)/16)*16);
    _NXTIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    _NXTIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);

    if (!TIFFWriteCheck(tif, "")) {
	return -1;
    }

    /* Now tileMem points to our tile. */   

    {
	unsigned char *jpeg;
	int jpegLen, bytesAllocated, base, cnt;
	NXJPEGInfo jpegInfo;
    
	if (compression < 1.0) compression = 10.0;
    
	if (_NXConvertBitmapToJPEG (
	    (int)td->td_tilewidth,
	    (int)td->td_tilelength,
	    (int)td->td_bitspersample,
	    (int)td->td_samplesperpixel, NO,
	    td->td_matteing != 0 ? YES : NO,
	    (int)td->td_photometric,
	    &tileMem,
	    _NXTIFFTileSize(tif),
	    YES, &jpeg, &jpegLen, &bytesAllocated,
	    compression, &jpegInfo)) {
    
	    result = _NXTIFFWriteRawStrip(tif, 0, jpeg, jpegLen);

	    /* At this point, we've written the whole bitstream out. */

	    base = td->td_stripoffset[0];
	    td->td_stripoffset[0] += jpegInfo.dataOffset;
	    td->td_stripbytecount[0] -= jpegInfo.dataOffset;

	    _NXTIFFSetField(tif, TIFFTAG_JPEGQFACTOR, jpegInfo.JPEGQFactor);
	    _NXTIFFSetField(tif, TIFFTAG_JPEGMODE, jpegInfo.JPEGMode);
	    _NXTIFFSetField(tif, TIFFTAG_JPEGQMATRIXPRECISION, jpegInfo.JPEGQMatrixPrecision);

	    for (cnt = 0; cnt < td->td_samplesperpixel; cnt++) {
		jpegInfo.JPEGQMatrixData[cnt] += base;
		jpegInfo.JPEGACTables[cnt] += base;
		jpegInfo.JPEGDCTables[cnt] += base;
	    }
	    _NXTIFFSetField(tif, TIFFTAG_JPEGQMATRIXDATA, jpegInfo.JPEGQMatrixData);
	    _NXTIFFSetField(tif, TIFFTAG_JPEGACTABLES, jpegInfo.JPEGACTables);
	    _NXTIFFSetField(tif, TIFFTAG_JPEGDCTABLES, jpegInfo.JPEGDCTables);
	    _NXTIFFSetField(tif, TIFFTAG_NEWCOMPONENTSUBSAMPLE, jpegInfo.Subsample);

	    vm_deallocate(task_self(),
			    (vm_address_t)jpeg, (vm_size_t)bytesAllocated);
    
	} else {
	    result = -1;
	}
    }
    
    if (tileMem != planes[0]) {
	free (tileMem);
    }

    /* On error, result will be -1. */

    return result;
}

/*
 * Returns the desired tile.
 * The scanlines are byte-aligned. If *data is not NULL, it's assumed
 * to have been allocated. Otherwise it is allocated by this routine.
 *
 * Returns -1 on error.
 */
int _NXTIFFReadJPEG (TIFF *tif, unsigned char **data)
{
    register TIFFDirectory *td =  &tif->tif_dir;
    int hAvailable, vAvailable;
    int sizeNecessary, processedTileSize, cnt;
    u_char *rawTile, *processedTile;
    BOOL rawBufferAllocated = NO, success;
    int tile = 0;    
    NXJPEGInfo jpegInfo;

    if (td->td_usetiles == 0) {
	TIFFError (NULL, "JPEG data should be tiled.");	return -1;
    } else if (td->td_nstrips != 1) {
	TIFFError (NULL, "Can only read one tile for now."); return -1;
    } else if (td->td_bitspersample != 8) {
	TIFFError (NULL, "Can only read 8 bps JPEG data."); return -1;
    }

    /*
     * ??? The number 7 below is magic; it's the size of the header in the
     * stream...
     */
    td->td_stripbytecount[0] += 
	td->td_stripoffset[0] - (td->td_jpegqmatrixdata[0] - 7);
    td->td_stripoffset[0] = td->td_jpegqmatrixdata[0] - 7;

    if (td->td_stripbytecount[0] < 1 || td->td_stripoffset[0] < 0) { 
	TIFFError (NULL, "Can't read tile."); return -1;
    }

    if (_NXIsMemoryStream (tif->tif_fd)) {
	int length, maxlen;
	NXGetMemoryBuffer(tif->tif_fd, (void *)&rawTile, &length, &maxlen);
	rawTile += td->td_stripoffset[0];
    } else {
	rawTile = valloc(td->td_stripbytecount[tile]);
	rawBufferAllocated = YES;
        if (_NXTIFFReadRawStrip(tif, tile, rawTile, -1) <= 0) {
	    free (rawTile);
	    return -1;
	}
    }

    /* Get the JPEG-related fields from the tiff. */

    jpegInfo.JPEGMode = td->td_jpegmode;
    jpegInfo.JPEGQFactor = td->td_jpegqfactor;
    jpegInfo.JPEGQMatrixPrecision = td->td_jpegqmatrixprecision;
    for (cnt = 0; cnt < td->td_samplesperpixel; cnt++) {
	jpegInfo.JPEGQMatrixData[cnt] = td->td_jpegqmatrixdata[cnt];
	jpegInfo.JPEGACTables[cnt] = td->td_jpegactables[cnt];
	jpegInfo.JPEGDCTables[cnt] = td->td_jpegdctables[cnt];
    }
    for (cnt = 0; cnt < td->td_samplesperpixel * 2; cnt++) {
	jpegInfo.Subsample[cnt] = td->td_newcomponentsubsample[cnt];
    }

    /* rawTile points at length bytes that form the specified tile. */

    success = _NXConvertJPEGToBitmap (
	(int)td->td_tilewidth, (int)td->td_tilelength,
	(int)td->td_bitspersample, (int)td->td_samplesperpixel,
	(td->td_planarconfig == PLANARCONFIG_CONTIG) ? NO : YES, 
	td->td_matteing != 0 ? YES : NO,
	(int)td->td_photometric, rawTile, td->td_stripbytecount[0],
	YES, &processedTile, &processedTileSize, &jpegInfo);

    if (rawBufferAllocated) free(rawTile);

    if (!success || !processedTileSize) {
	return -1;
    }

    /*
     * At this point processedTile points to an out of line buffer
     * that is processedTileSize bytes long. vm_deallocate it when done.
     */

    getImageAreaInTile (tif, tile, &hAvailable, &vAvailable);

    sizeNecessary = vAvailable * PIXELSTOBYTES(td, hAvailable);

    if (hAvailable != td->td_tilewidth) {
	u_char *src, *dest;
	int srcRowBytes = PIXELSTOBYTES(td, td->td_tilewidth);
	int destRowBytes = PIXELSTOBYTES(td, td->td_imagewidth);
	int row;

	if (*data == NULL) *data = malloc(sizeNecessary);
	src = processedTile;
	dest = *data;

	for (row = 0; row < vAvailable; row++) {
	    bcopy ((char *)src, (char *)dest, destRowBytes);
	    src += srcRowBytes;
	    dest += destRowBytes;
	}
    } else {
	/* The image size is the same as the tile; this is the easy case. */
	if (*data) {
	    bcopy (processedTile, *data, sizeNecessary);
	} else {
	    *data = VALLOC(sizeNecessary);
	    if (vm_copy(task_self(), (vm_address_t)processedTile,
			NUMVMPAGES(sizeNecessary) * vm_page_size,
			(vm_address_t)(*data)) != KERN_SUCCESS) {
		bcopy (processedTile, *data, sizeNecessary);
	    }
	}
    }

    vm_deallocate(task_self(), (vm_address_t)processedTile, (vm_size_t)processedTileSize);

    return 1;
}


/*
  
Created: 10/20/90

This code needs major rework when we really support JPEG TIFF files.

100
---
 10/22/90 aozer	Added _NXMakeEightBitMeshed(); this function converts any
		1, 2, 4, or 8bps image to 8bps meshed format. Used in
		_NXTIFFWriteJPEG.
 10/22/90 aozer	If JPEGcompression is not specified use 10.

*/



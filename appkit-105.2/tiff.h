/*
	tiff.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "graphics.h"

#define NX_PAGEHEIGHT 2048

#define NX_BIGENDIAN 0
#define NX_LITTLEENDIAN 1

/* TIFF errors */

#define NX_BAD_TIFF_FORMAT 1
#define NX_IMAGE_NOT_FOUND 2
#define NX_ALLOC_ERROR 3
#define NX_FORMAT_NOT_YET_SUPPORTED 4
#define NX_FILE_IO_ERROR 5
#define NX_COMPRESSION_NOT_YET_SUPPORTED 6
#define NX_TIFF_CANT_APPEND 7

/* Compression values. */

#define NX_TIFF_COMPRESSION_NONE	1
#define NX_TIFF_COMPRESSION_CCITTFAX3	3
#define NX_TIFF_COMPRESSION_LZW		5
#define NX_TIFF_COMPRESSION_NEXT	32766
#define NX_TIFF_COMPRESSION_PACKBITS	32773
#define NX_TIFF_COMPRESSION_JPEG	32865

/*
 * Structures used with the 1.0 TIFF read/write routines.
 */
typedef struct _NXImageInfo {
    int width;             /* image width in pixels */
    int height;            /* image height in pixels */
    int bitsPerSample;     /* number of bits per data channel */
    int samplesPerPixel;   /* number of channels per pixel */
    int planarConfig;      /* NX_MESHED for mixed data channels */
	                   /* NX_PLANAR for separate data planes */
    int photoInterp;       /* photometric interpretation of bitmap data, */
                           /* formed by ORing the constants NX_ALPHAMASK, */
			   /* NX_COLORMASK, and NX_MONOTONICMASK.  */
} NXImageInfo;

typedef struct _NXTIFFInfo {
    int imageNumber;
    NXImageInfo image;
    int	subfileType;
    int rowsPerStrip;
    int stripsPerImage;
    int compression;		/* compression id */
    int numImages;		/* number of images in tiff */
    int endian;			/* either NX_BIGENDIAN or NX_LITTLEENDIAN */
    int version;		/* tiff version */
    int error;
    int firstIFD;		/* offset of first IFD entry */
    unsigned int stripOffsets[NX_PAGEHEIGHT];
    unsigned int stripByteCounts[NX_PAGEHEIGHT];
} NXTIFFInfo;

extern int NXGetTIFFInfo(int imageNumber, NXStream *s, NXTIFFInfo *info);
 /*
  * NXGetTIFFInfo will read the information for image imageNumber in a
  * TIFF format file.  The info parameter is a pointer to an unitialized
  * NXTIFFInfo, which you should allocate on the stack. The total number
  * of bytes for the image is returned unless there is an error.  If there
  * is an error, the error field of the info struct will have a non-zero
  * value and NXGetTIFFInfo will return zero. 
  */
  
extern void *NXReadTIFF(int imageNumber, NXStream *s, NXTIFFInfo *info, void *data);
 /*
  * NXReadTIFF will read the image data for image imageNumber in a TIFF
  * format file.  The info parameter is a pointer to an unitialized
  * NXTIFFInfo, which you should allocate on the stack. If data is equal
  * to NULL, the image data will be stored into a malloced region of
  * memory, otherwise it will be copied to the memory pointed to by data.
  * If there is an error, the error field of the info struct will be
  * non-zero and NXReadTIFF will return NULL.
  *
  * Under 2.0, to read and write bitmaps in TIFF files use the NXBitmapImageRep 
  * class instead of NXReadTIFF() & NXWriteTIFF().
  */

extern void NXWriteTIFF(NXStream *s, NXImageInfo *image, void *data);

extern void NXImageBitmap(const NXRect *rect, int w, int h, int bps, int spp, 
				int planarConfig, int photoInt,
				const void *data1, const void *data2,
				const void *data3, const void *data4,
				const void *data5 );

extern void NXReadBitmap(const NXRect *rect, int w, int h, int bps, int spp, 
				int planarConfig, int photoInt,
				void *data1, void *data2, void *data3,
				void *data4, void *data5 );

extern void NXSizeBitmap(const NXRect *rect, int *size, int *width, 
    int *height, int *bps, int *spp, int *planarConfig, int *photoInt );

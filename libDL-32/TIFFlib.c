#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

/*	Responsibility: Bryan Yamamoto */

/* TIFFlib.c, Tagged Image File Format C library
 * Ref. TIFF Standard 4.0, Version 42, Aldus/Microsoft Report
 * Prepared by: R. E. Crandall, ETC
 * 22-MAR-88
 * 20-MAY-88 Added TIFF-to-PostScript conversion routines, notably
 *     TIFFtoPSimage(); 
 * Parameter vectors in TIFF headers define scenarios as follows:
 * Nomenclature:
 * ---------------
 * SIX-PARAMETER IMAGE DEFINITION
 * Width: Image width in pixels
 * Height: Image height in pixels  
 * BitsPerSamp:  The number of bits per data channel
 * SampsPerPix:  The number of channels per pixel
 * PlanarConfig: equals 1 for mixed data channels
   	         equals 2 for separate data planes
 * PhotoInterpret: has various bits set for various photometric
 		interpretations, as in the table below		
 * ---------------
	 
 * Parameters for saving and loading via the functions:
 * unsigned char *LoadTIFFimage("",...); OR LoadTIFFfile(fp,...);
 * and the procedures: SaveTIFFimage("",...); OR SaveTIFFfile(fp,...);
 * Note that for palette systems, such as 8-bit Pcolor, the palette
 * information must be stored in the TIFF file color curves directories. 	
   
   Bits/Samp Samp/Pix	PlanarConfig	   Scenario	   PhotoInterpret
	1	1	x (=don't care)	   b&w map		1
	1	2	1(mix), 2(mask)	   b&w alpha		5
	2	1	x		   4 grays		1 
	2	2       1(mix), 2(mask)	   4 grays alpha	5
	8	1	x		   8-bit Pcolor		10 
	8	2	1(mix), 2(mask)	   8-bit Pcolor alpha	14
	8	3	1(mix), 2(mask)    RGB color		2 
	8	4       1(mix), 2(mask)    RGBA color alpha	6
	8	3	1(mix), 2(mask)    CMY color		3
	8	4	1(mix), 2(mask)    CMYA color alpha     7
	8	4	1(mix), 2(mask)    CMYK color 		3
	8	5	1(mix), 2(mask)	   CMYKA color alpha    7
	
 */

#import <stdio.h>
#import <string.h>
#import <stdlib.h>
#import <libc.h>

#define INFINITY (unsigned int)(-1);
#define BYTE 1
#define ASCII 2

#define SHORT 3
#define LONG  4			/* Note: a TIFF standard "long" is a NeXT
				 * "int" */
#define RATIONAL 5
#define MOTOROLA 0
#define INTEL 1

#define CANT_OPEN  1		/* Begin error codes */
#define BAD_FORMAT 2
#define IMAGE_NOT_FOUND 3
#define ALLOC_ERROR 4
#define FORMAT_NOT_YET_SUPPORTED 5
#define FILE_IO_ERROR 6

#define PAGE_HEIGHT 2048

#define NEXT_GRAY_COMPRESSION 32766
#define ESIZE 8

typedef struct {
    char            ByteOrder[2];
    unsigned short  Version;
    unsigned int    Offset;
}               TIFFheader;

typedef struct {
    unsigned short  Tag;
    unsigned short  Type;
    unsigned int    Length;
    unsigned int    ValueOffset;
}               DirectoryEntry;


static TIFFheader BasicHeader = {"MM", 42, 8};

static DirectoryEntry SubfileType = {255, SHORT, 1, 0};
static DirectoryEntry ImageWidth = {256, SHORT, 1, 0};
static DirectoryEntry ImageLength = {257, SHORT, 1, 0};
static DirectoryEntry BitsPerSample = {258, SHORT, 1, 1 << 16};
static DirectoryEntry Compression = {259, SHORT, 1, 1 << 16};
static DirectoryEntry PhotometricInterpretation = {262, SHORT, 1, 1 << 16};
static DirectoryEntry StripOffsets = {273, LONG, 1, 98};
static DirectoryEntry PlanarConfiguration = {284, SHORT, 1, 1 << 16};
static DirectoryEntry SamplesPerPixel = {277, SHORT, 1, 1 << 16};
static DirectoryEntry RowsPerStrip = {278, LONG, 1, -1};
static DirectoryEntry StripByteCounts = {279, LONG, 0, 0};

static int      ErrorCode = 0, ByteOrder = MOTOROLA, CurrentVersion = 42;

static ReadCom2(FILE *fp, unsigned char *image, int w, int h, int bps);

int 
TIFFerror()
{
    return (ErrorCode);
}


int 
TIFFbyteOrder()
{
    return (ByteOrder);
}


int 
TIFFversion()
{
    return (CurrentVersion);
}


static unsigned short 
    getunshort(fp) FILE *fp;
{
    unsigned short  n;

    n = (unsigned char)getc(fp);
    if (ByteOrder == INTEL)
	return (n + 256 * ((unsigned char)getc(fp)));
    else
	return (256 * n + ((unsigned char)getc(fp)));
}


static unsigned int 
    getunint(fp) FILE *fp;
{
    unsigned int    n;

    n = getunshort(fp);
    if (ByteOrder == INTEL)
	return (n + 65536 * getunshort(fp));
    else
	return (65536 * n + getunshort(fp));
}


static 
putunshort(n, fp)
    unsigned short  n;
    FILE           *fp;
{
    putc(n / 256, fp);
    putc(n % 256, fp);
}


static 
putunint(n, fp)
    unsigned int    n;
    FILE           *fp;
{
    putunshort(n / 65536, fp);
    putunshort(n % 65536, fp);
}


static int 
SizeOfImage(w, h, b, s, p)
    int             w, h, b, s, p;
{
    if (p == 2)
	return (h * s * ((7 + b * w) / 8));
    return (h * ((7 + b * w * s) / 8));
}


static 
complement(bits, size)
    unsigned char  *bits;
    int             size;
{
    int             j;

    for (j = 0; j < size; j++)
	*(j + bits) = ~(*(j + bits));
}


static int 
GetNextImageDirectory(fp, LastEntryCount)
/* This routine searches the image directories (IFD's) of the TIFF file.
   Based on the previous entry count, it jumps the read pointer to
   the next TIFF directory's entry number zero.
   The next number of entries is returned, with 0 returned if no more 
   directories exist.
   If LastEntryCount = 0 is passed to this routine, the first
   IFD is found.
*/
    FILE           *fp;
    int             LastEntryCount;
{
    long            a;
    int             b;

    if (LastEntryCount)
	fseek(fp, (long)(12 * LastEntryCount), 1);
    else
	fseek(fp, 4L, 0);
    a = getunint(fp);
    fseek(fp, a, 0);
    if (a == 0)
	return (0);
    b = getunshort(fp);
    return (b);
}


static int 
GetNthImageDirectory(fp, N)
/* If the Nth directory exists this routine jumps the read pointer to
   that image directory, and the returned value is the number of entries.
   If N is not a legitimate directory index, e.g. N = -1, the returned
   value is the total number of valid directories.
*/
    FILE           *fp;
    int             N;
{
    int             EntryCount = 0, j;

    for (j = 0; 1; j++) {
	EntryCount = GetNextImageDirectory(fp, EntryCount);
	if (EntryCount == 0)
	    return (j);
	if (j == N)
	    return (EntryCount);
    }
}


static int 
GetNumberOfImages(fp)
/* Returns the total number of images */
    FILE           *fp;
{
    return (GetNthImageDirectory(fp, -1));
}


static int 
GetTagValues(fp, ImageNumber, Tag, Type)
/* General routine for seeking, if necessary, exotic Tagged directory
   entries.  Use 'SizeTIFFimage' for to get basic data if only width,
   height, bits/sample and samples/pixel are needed.
   For the given ImageNumber = 0,1,...GetNumberOfImages()-1, and the
   Tag in question; the routine jumps the read pointer to the ValueOffset data.
   The returned value is the Length field, or zero if the Tag does not
   exist in the image Directory.
*/

    FILE           *fp;
    int             ImageNumber;
    unsigned short  Tag;
    unsigned short *Type;
{
    int             EntryCount, j;

    EntryCount = GetNthImageDirectory(fp, ImageNumber);
    for (j = 0; j < EntryCount; j++) {
	if (Tag == getunshort(fp))
	    break;
	fseek(fp, 10L, 1);
    }
    if (j == EntryCount) {
	fseek(fp, 0L, 0);
	return (0);
    }
    *Type = getunshort(fp);
    return (getunint(fp));
}


int 
SizeTIFFimage(fp, ImageNumber, Width, Height, BitsPerSamp, SamplesPerPix,
	      PhotoInt, PlanarConfig, RowsPer, StripsPerImage, Compress,
	      StripOffs, StripBytes)
/* Specific routine for simple singleton images, sets the seven integer values 
   to those associated with the ImageNumber = 0,1,2,... directory, and
   stores an array of pointers StripOffs, and an array of counts, StripBytes.
   These arrays are irrelevant if there is one strip for the image.
   The returned value is the total number of bytes for the image, with
   zero returned if the image cannot be located.
   What happens to the file read pointer is as follows:
   1) If there is one strip in the image, pointer is left at the image data,
   2) If there is more than one strip, pointer is left at the first
      StripsPerImage value position
 */
    FILE           *fp;
    int             ImageNumber;
    int            *Width, *Height, *BitsPerSamp, *SamplesPerPix, *PhotoInt, *PlanarConfig;
    unsigned int   *RowsPer, *StripsPerImage, *Compress;
    unsigned int    StripOffs[PAGE_HEIGHT], StripBytes[PAGE_HEIGHT];

{
    int             EntryCount, j, k, type, offsettype, numbytes;
    unsigned short  b;
    long            ImageDataPtr, ByteCountPtr;

    if ((EntryCount = GetNthImageDirectory(fp, ImageNumber)) == 0) {
	fseek(fp, 0L, 0);
	return (0);
    }
    StripOffs[0] = 0;
    StripBytes[0] = 0;
    ByteCountPtr = 0L;
    *SamplesPerPix = 1;		/* Various defaults */
    *PlanarConfig = 1;
    *Compress = 1;
    *PhotoInt = 1;
    *RowsPer = -1;
    *Width = 0;
    *Height = 0;
    *BitsPerSamp = 1;
    k = 0;			/* For subfiletype check */
    for (j = 0; j < EntryCount; j++) {
	b = getunshort(fp);
	if (b == SubfileType.Tag) {
	    fseek(fp, 6L, 1);
	    k = getunshort(fp);
	    fseek(fp, 2L, 1);
	    continue;
	} else if (b == ImageWidth.Tag) {
	    fseek(fp, 6L, 1);
	    *Width = getunshort(fp);
	    fseek(fp, 2L, 1);
	    continue;
	} else if (b == ImageLength.Tag) {
	    fseek(fp, 6L, 1);
	    *Height = getunshort(fp);
	    fseek(fp, 2L, 1);
	    continue;
	} else if (b == RowsPerStrip.Tag) {
	    type = getunshort(fp);
	    fseek(fp, 4L, 1);
	    if (type == LONG)
		*RowsPer = getunint(fp);
	    else {
		*RowsPer = getunint(fp);
		fseek(fp, 2L, 1);
	    }
	    continue;
	} else if (b == BitsPerSample.Tag) {
	    fseek(fp, 6L, 1);
	    *BitsPerSamp = getunshort(fp);
	    fseek(fp, 2L, 1);
	    continue;
	} else if (b == Compression.Tag) {
	    fseek(fp, 6L, 1);
	    *Compress = getunshort(fp);
	    fseek(fp, 2L, 1);
	    continue;
	} else if (b == SamplesPerPixel.Tag) {
	    fseek(fp, 6L, 1);
	    *SamplesPerPix = getunshort(fp);
	    fseek(fp, 2L, 1);
	    continue;
	} else if (b == PlanarConfiguration.Tag) {
	    fseek(fp, 6L, 1);
	    *PlanarConfig = getunshort(fp);
	    fseek(fp, 2L, 1);
	    continue;
	} else if (b == PhotometricInterpretation.Tag) {
	    fseek(fp, 6L, 1);
	    *PhotoInt = getunshort(fp);
	    fseek(fp, 2L, 1);
	    continue;
	} else if (b == StripOffsets.Tag) {
	    offsettype = getunshort(fp);
	    fseek(fp, 4L, 1);
	    if (offsettype == LONG)
		ImageDataPtr = getunint(fp);
	    else {
		ImageDataPtr = getunint(fp);
		fseek(fp, 2L, 1);
	    }
	    continue;
	} else if (b == StripByteCounts.Tag) {
	    fseek(fp, 6L, 1);
	    ByteCountPtr = getunint(fp);
	    continue;
	} else
	    fseek(fp, 10L, 1);
    }
    if (*SamplesPerPix < 2)
	*PlanarConfig = 1;
    if ((*BitsPerSamp < 1) || (*Width == 0) || (*Height == 0) || (k != 1)) {
	fseek(fp, 0L, 0);
	ErrorCode = BAD_FORMAT;
	return (0);
    }
    if ((*RowsPer + 1 == 0) || (*RowsPer >= *Height)) {
	*StripsPerImage = 1;
	*RowsPer = *Height;
    } else
	*StripsPerImage = ((*Height) + (*RowsPer) - 1) / (*RowsPer);

    if ((*BitsPerSamp > 8) || (*StripsPerImage > PAGE_HEIGHT)) {
	fseek(fp, 0L, 0);
	ErrorCode = FORMAT_NOT_YET_SUPPORTED;
	return (0);
    }
    if (*Compress == NEXT_GRAY_COMPRESSION)
	if (*SamplesPerPix != 1) {
	    fseek(fp, 0L, 0);
	    ErrorCode = FORMAT_NOT_YET_SUPPORTED;
	    return (0);
	}
    numbytes = SizeOfImage(*Width, *Height, *BitsPerSamp,
			   *SamplesPerPix, *PlanarConfig);

    fseek(fp, ImageDataPtr, 0);	/* Point to the actual image data */

    if ((*StripsPerImage) * (*SamplesPerPix) == 1)
	return (numbytes);

    for (j = 0; j < (*StripsPerImage) * (*SamplesPerPix); j++) {
	if (offsettype == SHORT)
	    StripOffs[j] = getunshort(fp);
	else
	    StripOffs[j] = getunint(fp);
    }
    if (ByteCountPtr) {
	fseek(fp, ByteCountPtr, 0);
	for (j = 0; j < (*StripsPerImage) * (*SamplesPerPix); j++)
	    StripBytes[j] = getunint(fp);
    }
    fseek(fp, 0L, 0);
    return (numbytes);
}


TIFFcheck(fp, NumImages)
/* Performs bullet-proofing for the (ALREADY OPEN) TIFF file fp.  Error codes 
   result from any discrepancy with the standard format.
   If, and only if, *NumImages starts out as non-zero, then
   *NumImages is set to the number of properly formatted images in the file.
   If instead *NumImages starts out as zero, no time is wasted counting images.
   The function sets the value of currentversion as found in the TIFF file,
   to be read according to the function TIFFversion().  The version supporting
   this library is version 42 (decimal).  However, no error-return occurs
   if this is violated; instead the programmer may check version number at
   his/her leisure.  
*/
    FILE           *fp;
    int            *NumImages;
{
    char            a;

    ErrorCode = 0;
    a = getc(fp);
    if (a != getc(fp)) {
	ErrorCode = BAD_FORMAT;
	*NumImages = 0;
	return;
    }
    if (a == 'M')
	ByteOrder = MOTOROLA;
    else {
	if (a == 'I')
	    ByteOrder = INTEL;
	else {
	    ErrorCode = BAD_FORMAT;
	    *NumImages = 0;
	    return;
	}
    }
    CurrentVersion = getunshort(fp);
    if (*NumImages)
	*NumImages = GetNumberOfImages(fp);
    fseek(fp, 0L, 0);
    return;
}


unsigned char  *
LoadTIFFfile(fp, ImageNumber, Bits,
	     Width, Height, BitsPerSamp,
	     SamplesPerPix, PlanarConfig, PhotoInt)
    FILE           *fp;
    int             ImageNumber;
    unsigned char  *Bits;
    int            *Width, *Height, *BitsPerSamp, *SamplesPerPix, *PlanarConfig, *PhotoInt;
{

    int             NumImages = 1, size, piece, j;
    unsigned int    RowsPer, StripsPerImage, Compress;
    unsigned char  *localbits, *imagebits, c;
    int             StripOffs[PAGE_HEIGHT], StripBytes[PAGE_HEIGHT];

    TIFFcheck(fp, &NumImages);
    if (ImageNumber >= NumImages) {
	ErrorCode = IMAGE_NOT_FOUND;
	return (NULL);
    }
    size =
      SizeTIFFimage(fp, ImageNumber, Width, Height, BitsPerSamp, SamplesPerPix,
	    PhotoInt, PlanarConfig, &RowsPer, &StripsPerImage, &Compress,
		    StripOffs, StripBytes);

    if (!size) {
	return (NULL);
    }
    if (Bits == NULL) {
	localbits = (unsigned char *)malloc(size);
	if (localbits == NULL) {
	    ErrorCode = ALLOC_ERROR;
	    return (NULL);
	}
	imagebits = localbits;
    } else {
	imagebits = Bits;
	localbits = Bits + size;
    }
    if (StripsPerImage == 1) {
	if (Compress == NEXT_GRAY_COMPRESSION)
	    ReadCom2(fp, imagebits, *Width, *Height, *BitsPerSamp);
	else
	    fread(imagebits, size, 1, fp);
    } else {
	for (j = 0; j < StripsPerImage; j++) {
	    fseek(fp, (long)StripOffs[j], 0);
	    if (StripBytes[0])
		piece = StripBytes[j];
	    else
		piece = SizeOfImage(*Width, RowsPer, *BitsPerSamp,
				    *SamplesPerPix, *PlanarConfig);
	    fread(imagebits, piece, 1, fp);
	    imagebits += piece;
	}
    }
    if (((*PhotoInt) & 1) == 0) {
	if (Bits == NULL)
	    complement(localbits, size);
	else
	    complement(Bits, size);
    }
    return (localbits);
}


unsigned char  *
LoadTIFFimage(Name, ImageNumber, Bits,
	      Width, Height, BitsPerSamp,
	      SamplesPerPix, PlanarConfig, PhotoInterpret)
/* Turn-key routine for loading one image and (optionally) preparing for the
   next image.  This routine attempts the opening of the named file.
   SEE HEADER ATOP THIS SOURCE FOR PARAMETER MEANINGS. 
   The pointer Bits has context, as follows:
   	1) If one passes Bits = NULL, memory is automatically allocated
	   and the image loaded at a pointer, said pointer returned
	   as function value.
	2) If one passes a non-NULL Bits pointer, the image is loaded
	   at that pointer and the function returns the NEXT location for
	   a possible subsequent image.  In this way the function can
	   be used to chain images together using arrays of data pointers
	   or instance variables.  This mode does NOT allocate memory.
	   Note that in this mode the loaded image bytesize can be conveniently
	   computed, after the call, as (function value) minus (Bits).
   Note that mode 1 is recommended when you do not have a reliable bound
   on the size of the image.  Mode 2 is recommended when you have a
   series of known images, such as a set of fixed icons, for which
   sufficient memory has already been allocated.
   
   If anything goes awry the return value, however, is NULL, in which
   case TIFFerror() can be used externally to assess the difficulty.   
   Otherwise,the six trailing *integer parameters are forced to the values 
   dictated by the TIFF image.   
*/
    char           *Name;
    int             ImageNumber;
    unsigned char  *Bits;
    int            *Width, *Height, *BitsPerSamp, *SamplesPerPix, *PlanarConfig, *PhotoInterpret;
{
    FILE           *fp;
    unsigned char  *localptr;

    if (strlen(Name) == 0)
	fp = stdin;
    else
	fp = fopen(Name, "r");
    if (fp == NULL) {
	ErrorCode = CANT_OPEN;
	return (NULL);
    }
    localptr = LoadTIFFfile(fp, ImageNumber, Bits,
			    Width, Height, BitsPerSamp,
			    SamplesPerPix, PlanarConfig, PhotoInterpret);
    fclose(fp);
    return (localptr);
}


SaveTIFFfile(fp, bits, Width, Height, BitsPerSamp,
	     SampsPerPix, PlanarConfig, PhotoInterpret)
    FILE           *fp;
    unsigned char  *bits;
    int             Width, Height, BitsPerSamp, SampsPerPix, PlanarConfig, PhotoInterpret;
{
    int             nentries;

    if (SampsPerPix < 2) {	/* Bullet-proofing */
	SampsPerPix = 1;
	PlanarConfig = 1;
	nentries = 7;
    } else
	nentries = 8;

    fwrite(&BasicHeader, sizeof(BasicHeader), 1, fp);
    putunshort((unsigned short)nentries, fp);

    SubfileType.ValueOffset = 1 << 16;
    fwrite(&SubfileType, sizeof(SubfileType), 1, fp);

    ImageWidth.ValueOffset = Width << 16;
    fwrite(&ImageWidth, sizeof(ImageWidth), 1, fp);

    ImageLength.ValueOffset = Height << 16;
    fwrite(&ImageLength, sizeof(ImageLength), 1, fp);

    BitsPerSample.ValueOffset = BitsPerSamp << 16;
    fwrite(&BitsPerSample, sizeof(BitsPerSample), 1, fp);

    PhotometricInterpretation.ValueOffset = PhotoInterpret << 16;
    fwrite(&PhotometricInterpretation, sizeof(PhotometricInterpretation),
	   1, fp);

    StripOffsets.ValueOffset = 10 + 12 * nentries + 4;
    fwrite(&StripOffsets, sizeof(StripOffsets), 1, fp);

    if (SampsPerPix > 1) {
	SamplesPerPixel.ValueOffset = SampsPerPix << 16;
	fwrite(&SamplesPerPixel, sizeof(SamplesPerPixel), 1, fp);
    }
    PlanarConfiguration.ValueOffset = PlanarConfig << 16;
    fwrite(&PlanarConfiguration, sizeof(PlanarConfiguration), 1, fp);

    putunint((unsigned int)0, fp);

    fwrite(bits, SizeOfImage(Width, Height, BitsPerSamp, SampsPerPix,
			     PlanarConfig), 1, fp);
    return;
}


SaveTIFFimage(Name, bits, Width, Height, BitsPerSamp,
	      SampsPerPix, PlanarConfig, PhotoInt)
/* Name is the file name, except if Name = "" the file is stdout.
   This routine saves the image residing at the pointer "bits".
   Errors can be subsequently checked via the function TIFFerror().
   SEE HEADER ATOP THIS SOURCE FOR PARAMETER MEANINGS.   
*/

    char           *Name;
    unsigned char  *bits;
    int             Width, Height, BitsPerSamp, SampsPerPix, PlanarConfig, PhotoInt;
{
    FILE           *fp;

    ErrorCode = 0;
    if (strlen(Name) == 0)
	fp = stdout;
    else
	fp = fopen(Name, "w");
    if (fp == NULL) {
	ErrorCode = CANT_OPEN;
	return;
    }
    SaveTIFFfile(fp, bits, Width, Height, BitsPerSamp,
		 SampsPerPix, PlanarConfig, PhotoInt);
    fclose(fp);
}


psheader(fp, xorg, yorg, xcor, ycor)
/* Create appropriate PostScript header. */
    FILE           *fp;
    float           xorg, yorg, xcor, ycor;
{
    char            c = '%';

    fprintf(fp, "%c!PS-Adobe-2.0 EPSF-1.2\n", c);
    fprintf(fp, "%c%cBoundingBox: %4.3f %4.3f %4.3f %4.3f\n", c, c,
	    xorg, yorg, xcor, ycor);
    fprintf(fp, "%c%cEndComments\n", c, c);
}


hexwrite(imageptr, numbytes, fp)
/* Write image data out as hexadecimal. */
    unsigned char  *imageptr;
    int             numbytes;
    FILE           *fp;
{
    int             n, c1, c2, i;

    for (i = 0; i < numbytes; i++) {
	if ((i % 32 == 0) && i)
	    putc('\n', fp);
	n = *(imageptr + i);
	c1 = '0' + n / 16 + 7 * ((n / 16) / 10);
	c2 = '0' + n % 16 + 7 * ((n % 16) / 10);
	putc(c1, fp);
	putc(c2, fp);
    }
}


ImageToPSfile(FILE *PSfileptr, unsigned char *imageptr, 
	int w, int h, int bps,int spp, 
	float destx, float desty, float destw, float desth)
/* Write a PostScript file using memory array of pixels. */
{
    int             rowbytes = (w * bps + 7) / 8;

    psheader(PSfileptr, destx, desty, destx + destw, desty + desth);
    fprintf(PSfileptr, "/picstr %d string def\n", rowbytes);
    fprintf(PSfileptr, "%4.3f %4.3f translate\n", destx, desty);
    fprintf(PSfileptr, "%4.3f %4.3f scale\n", destw, desth);
    fprintf(PSfileptr, "%d %d %d\n", w, h, bps);
    fprintf(PSfileptr, "[%d 0 0 %d neg 0 %d]\n", w, h, h);
    fprintf(PSfileptr, "{currentfile picstr readhexstring pop}\n");
    if (spp == 1)
	fprintf(PSfileptr, "image\n");
    else
	fprintf(PSfileptr, "dup true 1 alphaimage\n");
    hexwrite(imageptr, h * spp * rowbytes, PSfileptr);
    fprintf(PSfileptr, "\n");
    fprintf(PSfileptr, "showpage\n");
}


TIFFtoPSfile(PSfileptr, TIFFfileptr, destx, desty, magnification)
/* Write a PostScript file using a pointer to a TIFF file. */
    FILE           *PSfileptr, *TIFFfileptr;
    float           destx, desty, magnification;
{
    int             w, h, bps, spp, pc, pi;
    int             rowbytes;
    unsigned char  *image;

    image = LoadTIFFfile(TIFFfileptr, 0, NULL, &w, &h, &bps, &spp, &pc, &pi);
    if (image == NULL) {
	ErrorCode = FILE_IO_ERROR;
	return (0);
    }
    ImageToPSfile(PSfileptr, image,
		  w, h, bps, spp, destx, desty,
		  destx + magnification * w,
		  desty + magnification * h);
    free(image);
    return (1);
}


TIFFtoPSimage(PSfilename, TIFFfilename, destx, desty, magnification)
/* Create a PostScript file, by name; using a TIFF file, also by name.
   Note that degenerate names "" invoke standard I/O.
 */
    char           *PSfilename, *TIFFfilename;
    float           destx, desty, magnification;
{
    FILE           *PSfp, *TIFFfp;

    if (strlen(PSfilename) == 0)
	PSfp = stdout;
    else
	PSfp = fopen(PSfilename, "w");
    if (strlen(TIFFfilename) == 0)
	TIFFfp = stdin;
    else
	TIFFfp = fopen(TIFFfilename, "r");
    if ((TIFFfp == NULL) || (PSfp == NULL)) {
	ErrorCode = FILE_IO_ERROR;
	return (0);
    }
    TIFFtoPSfile(PSfp, TIFFfp, destx, desty, magnification);
    fclose(PSfp);
    fclose(TIFFfp);
    return (1);
}

static unsigned char *thebits;
static int      outset, wid, hei, bpp, bpr, fullcolor, fullword;


setthepixparams(width, height, bitsperpix, bitarray)
/* Call this once, to initialize 'bitarray' image parameters. */
    int             width, height, bitsperpix;
    unsigned char  *bitarray;
{
    wid = width;
    hei = height;
    thebits = bitarray;
    bpp = bitsperpix;
    bpr = (width * bpp + ESIZE - 1) / ESIZE;
    outset = ESIZE - bpp;
    fullcolor = (1 << bpp) - 1;
    fullword = (1 << ESIZE) - 1;
}


getthepixel(x, y) int 
    x, y;

/* Return integer pixel value
   EXCEPT return -1 if (x,y) is not in the image.
 */
{
    register int    n;
    register unsigned char c;

    if ((x < 0) || (y < 0) || (x >= wid) || (y >= hei))
	return (-1);
    n = y * bpr + (x * bpp) / ESIZE;
    c = *(thebits + n);
    return ((c >> (outset - (x * bpp) % ESIZE)) & fullcolor);
}


setthepixel(x, y, color) int 
    x, y, color;

/* Set pixel value to the integer 'color'
   EXCEPT do nothing if (x,y) is not in the image.
 */
{
    register int    c, d, e, n, shift;
    unsigned char  *p;

    if ((x < 0) || (y < 0) || (x >= wid) || (y >= hei) || (color < 0))
	return;
    n = y * bpr + (x * bpp) / ESIZE;
    shift = outset - (x * bpp) % ESIZE;
    d = color << shift;
    e = fullword - (fullcolor << shift);
    p = thebits + n;
    *p = ((*p) & e) | d;
}


ReadCom2(FILE *fp, unsigned char *image, int w, int h, int bps)
{
    register int    x, y, color, rowbytes = (w * bps + 7) / 8, howmany, n, m, white;
    unsigned char   c;

    if (bps > 2) {
	fread(image, rowbytes * h, 1, fp);
	return;
    }
    setthepixparams(w, h, bps, image);
    white = (1 << bps) - 1;
    if (bps <= 2)
	for (x = 0; x < h * rowbytes; x++)
	    *(image + x) = 255;	/* Start white ! */
    for (y = 0; y < h; y++) {
	c = (unsigned char)getc(fp);
	if (c == 0) {
	    fread(image + rowbytes * y, rowbytes, 1, fp);
	} else if (c == 64) {
	    n = (unsigned char)getc(fp);
	    n = n * 256 + (unsigned char)getc(fp);
	    m = (unsigned char)getc(fp);
	    m = m * 256 + (unsigned char)getc(fp);
	    fread(image + rowbytes * y + n, m, 1, fp);
	} else {
	    x = 0;
	    while (1) {
		color = c / 64;
		howmany = c % 64;
		if (color < white)
		    for (n = 0; n < howmany; n++)
			setthepixel(x + n, y, color);
		x += howmany;
		if (x >= w)
		    break;
		c = (unsigned char)getc(fp);
	    }
	}
    }
}


WriteCom2(fp, image, w, h, bps)
    FILE           *fp;
    unsigned char  *image;
    int             w, h, bps;
{
    int             x, y, howmany, index, color, nextcolor, rowbytes = (w * bps + 7) / 8;
    int             white, start, stop, startbytes, darkbytes;
    unsigned char   scan[1024 + 256];

    if (bps > 2) {
	fwrite(image, rowbytes * h, 1, fp);
	return;
    }
    setthepixparams(w, h, bps, image);
    white = (1 << bps) - 1;
    for (y = 0; y < h; y++) {
	start = -1;
	stop = w;
	color = getthepixel(0, y);
	if (color != white)
	    start = 0;
	howmany = 0;
	index = 0;
	for (x = 1; x < w; x++) {
	    nextcolor = getthepixel(x, y);
	    if (nextcolor != white) {
		stop = x;
		if (start < 0)
		    start = x;
	    }
	    ++howmany;
	    if ((color != nextcolor) || (howmany > 62)) {
		scan[index++] = (color << 6) + howmany;
		howmany = 0;
		color = nextcolor;
	    }
	}
	scan[index++] = (nextcolor << 6) + howmany + 1;
/* Index is now the number of bytes for the compression attempt. */
/* Next, get white margin sizes.*/
	if (start < 0) {
	    startbytes = rowbytes;
	    darkbytes = 0;
	} else {
	    startbytes = (start * bps) / 8;
	    darkbytes = ((stop + 1) * bps + 7) / 8 - startbytes;
	}
	if ((start < 0) && (stop >= w))
	    darkbytes = rowbytes;
	if (index < darkbytes + 5) {
	    fwrite(scan, index, 1, fp);
	} else {
	    if (darkbytes >= rowbytes - 5) {
		putc(0, fp);
		fwrite(image + rowbytes * y, rowbytes, 1, fp);
	    } else {
		putc(64, fp);
		putc(startbytes / 256, fp);
		putc(startbytes % 256, fp);
		putc(darkbytes / 256, fp);
		putc(darkbytes % 256, fp);
		fwrite(image + rowbytes * y + startbytes, darkbytes, 1, fp);
	    }
	}
    }
}


SaveGrayCompressedFile(fp, bits, Width, Height, BitsPerSamp)
    FILE           *fp;
    unsigned char  *bits;
    int             Width, Height, BitsPerSamp;
{
    int             nentries = 8;
    int             SampsPerPix = 1, PlanarConfig = 1, PhotoInterpret = 1;

    fwrite(&BasicHeader, sizeof(BasicHeader), 1, fp);
    putunshort((unsigned short)nentries, fp);

    SubfileType.ValueOffset = 1 << 16;
    fwrite(&SubfileType, sizeof(SubfileType), 1, fp);

    ImageWidth.ValueOffset = Width << 16;
    fwrite(&ImageWidth, sizeof(ImageWidth), 1, fp);

    ImageLength.ValueOffset = Height << 16;
    fwrite(&ImageLength, sizeof(ImageLength), 1, fp);

    BitsPerSample.ValueOffset = BitsPerSamp << 16;
    fwrite(&BitsPerSample, sizeof(BitsPerSample), 1, fp);

    Compression.ValueOffset = NEXT_GRAY_COMPRESSION << 16;
    fwrite(&Compression, sizeof(Compression), 1, fp);

    PhotometricInterpretation.ValueOffset = PhotoInterpret << 16;
    fwrite(&PhotometricInterpretation, sizeof(PhotometricInterpretation),
	   1, fp);

    StripOffsets.ValueOffset = 10 + 12 * nentries + 4;
    fwrite(&StripOffsets, sizeof(StripOffsets), 1, fp);

    PlanarConfiguration.ValueOffset = PlanarConfig << 16;
    fwrite(&PlanarConfiguration, sizeof(PlanarConfiguration), 1, fp);

    putunint((unsigned int)0, fp);

    WriteCom2(fp, bits, Width, Height, BitsPerSamp);
    return;
}


SaveGrayCompressedImage(Name, bits, Width, Height, BitsPerSamp)
/* Name is the file name, except if Name = "" the file is stdout.
   This routine saves the image residing at the pointer "bits".
   Errors can be subsequently checked via the function TIFFerror().
   SEE HEADER ATOP THIS SOURCE FOR PARAMETER MEANINGS.   
*/
    char           *Name;
    unsigned char  *bits;
    int             Width, Height, BitsPerSamp;
{
    FILE           *fp;

    ErrorCode = 0;
    if (strlen(Name) == 0)
	fp = stdout;
    else
	fp = fopen(Name, "w");
    if (fp == NULL) {
	ErrorCode = CANT_OPEN;
	return;
    }
    SaveGrayCompressedFile(fp, bits, Width, Height, BitsPerSamp);
    fclose(fp);
}


CompressGrayImage(InputName, OutputName)
    char           *InputName, *OutputName;
{
    unsigned char  *image;
    int             w, h, b, s, pc, pi;

    ErrorCode = 0;
    image = LoadTIFFimage(InputName, 0, NULL,
			  &w, &h, &b, &s, &pc, &pi);
    if ((image == NULL) || (s != 1)) {
	ErrorCode = FORMAT_NOT_YET_SUPPORTED;
	return;
    }
    SaveGrayCompressedImage(OutputName, image, w, h, b);
    free(image);
}


DecompressGrayImage(InputName, OutputName)
    char           *InputName, *OutputName;
{
    unsigned char  *image;
    int             w, h, b, s, pc, pi;

    ErrorCode = 0;
    image = LoadTIFFimage(InputName, 0, NULL,
			  &w, &h, &b, &s, &pc, &pi);
    if ((image == NULL) || (s != 1)) {
	ErrorCode = FORMAT_NOT_YET_SUPPORTED;
	return;
    }
    SaveTIFFimage(OutputName, image, w, h, b, s, pc, pi);
    free(image);
}



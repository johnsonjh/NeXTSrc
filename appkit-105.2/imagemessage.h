#import <sys/message.h>
#import <objc/HashTable.h>
#import <appkit/graphics.h>

typedef struct _NXImageServerMessage {

    msg_header_t header;

#define NX_ISM_NUMINTPARAMS 43

    msg_type_t intParams;

    int pixelsWide;	// Description of the image data
    int pixelsHigh;
    int bytesPerRow;
    int bitsPerSample;
    int samplesPerPixel;
    int colorSpace;
    int isPlanar;
    int hasAlpha;

    int destWin;	// Destination window if imaging
    int imageDataLen;	// Number of bytes in each channel

    int tag;		// Tag to identify the request

    int numChannels;	// Actual number of channels we are passing down
    
#define MAXSAMPLES 5
    int	NewComponentSubsample[2*MAXSAMPLES];
    int JPEGMode;
    int	JPEGQFactor;
    int	JPEGQMatrixPrecision;
    int	JPEGQMatrixData[MAXSAMPLES];
    int	JPEGDCTables[MAXSAMPLES];
    int	JPEGACTables[MAXSAMPLES];

    int dataOffset;      /* The byte offset to the first byte of data */
    int reservedOne;
    int reservedTwo;

    /* Where to image or readimage */

#define NX_ISM_NUMFLOATPARAMS 12

    msg_type_t floatParams;

    float destCTM[6];		// Matrix for imaging into destWin
    float llx, lly, urx, ury;	// Location in destWin

    float compressionFactor;	// Compression factor
    float reservedThree;

    /* The data */

#define NX_ISM_MAXCHANNELS 5

    struct {

	msg_type_long_t dataParams; 
	unsigned char *imageData;

    } channels[NX_ISM_MAXCHANNELS];

} NXImageServerMessage;

/* IDs for messages, which serve as commands. */

typedef enum _NXImageServerRequest {
    NX_ImageJPEGCompress = 0x0acdc,
    NX_ImageJPEGDecompress = 0x0acdd,
    NX_ImageJPEGDecompressAndDisplay = 0x0acde,
    NX_ImagePostScript = 0x0acdb,
    NX_ImageBitmap = 0x0acda
} NXImageServerRequest;

#define NX_IMAGE_PORT "NeXTstep Images compressed/decompressed/displayed"

#define NX_IMAGE_SERVER "/usr/etc/imageserver"

typedef struct _NXImageServerConnection {
    int tag, lastFullfilledRequest;
    HashTable *processedData;
    port_t clientPort, imagePort; 
    char *hostName;   
} NXImageServerConnection;

typedef struct _NXJPEGInfo {
    short Subsample[2*MAXSAMPLES];
    int JPEGQMatrixData[MAXSAMPLES];
    int JPEGACTables[MAXSAMPLES];
    int JPEGDCTables[MAXSAMPLES];
    int JPEGMode;
    int JPEGQFactor;
    int JPEGQMatrixPrecision;
    int dataOffset;
} NXJPEGInfo;




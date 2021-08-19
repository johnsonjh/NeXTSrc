/*
	imageClient.m
	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer

	This file contains the code to implement the client side of the image
	server. 
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "imagemessage.h"
#import "nextstd.h"
#import "graphics.h"
#import "tiffPrivate.h"
#import <defaults.h>
#import "appkitPrivate.h"
#import "Application.h"
#import <mach.h>
#import <string.h>
#import <zone.h>
#import <libc.h>
#import <servers/netname.h>
#import <objc/HashTable.h>

static NXImageServerConnection *conn = NULL;

static NXImageServerConnection *newImageServerConnection (const char *hostName)
{
    port_t clientPort, imagePort;    
    kern_return_t ret;
    NXImageServerConnection *conn;
    NXZone *zone = [NXApp zone];

    if (!zone) zone = NXDefaultMallocZone ();

    if ((ret = port_allocate(task_self(), &clientPort)) != KERN_SUCCESS) {
	NXLogError ("Error %d creating image server port", ret);
	return NULL;
    }

    if (!hostName) {
	hostName = "";
    }

    if ((ret = netname_look_up(name_server_port, (char *)hostName,
			NX_IMAGE_PORT, &imagePort)) != KERN_SUCCESS) {
	int attempt = 15;
	if (!_NXExecDetached(hostName, NX_IMAGE_SERVER))
	    attempt = 0;
	while ((attempt-- > 0) &&
	       ((ret = netname_look_up(name_server_port, (char *)hostName,
			NX_IMAGE_PORT, &imagePort)) != KERN_SUCCESS)) {
	    sleep(1);
	}
	if (ret != KERN_SUCCESS) {
	    (void)port_deallocate (task_self(), clientPort);
	    NXLogError ("Error %d looking up image server", ret);
	    return NULL;
	}
    }

    conn = NXZoneMalloc (zone, sizeof(NXImageServerConnection));

    conn->clientPort = clientPort;
    conn->imagePort = imagePort;
    conn->processedData = nil;
    conn->tag = conn->lastFullfilledRequest = 0;    
    conn->hostName = NXCopyStringBufferFromZone (hostName, zone);

    return conn;
}

BOOL _NXImageServerOK ()
{
    if (!conn && !(conn = newImageServerConnection(""))) {
	return NO;
    } else {
	return YES;
    }
}

static int InitMessage (NXImageServerMessage *msg, int numChannels)
{
    int cnt;

    if (!_NXImageServerOK()) {
	return 0;
    }

    NX_ASSERT (numChannels <= NX_ISM_MAXCHANNELS, "InitMessage got more than 5 channels");

    msg->header.msg_simple = 0;
    msg->header.msg_type = MSG_TYPE_NORMAL;
    msg->header.msg_size = sizeof(NXImageServerMessage) - (NX_ISM_MAXCHANNELS - numChannels) * (sizeof(msg_type_long_t) + sizeof(unsigned char *));
    msg->header.msg_local_port = conn->clientPort;
    msg->header.msg_remote_port = conn->imagePort;

    msg->intParams.msg_type_name = MSG_TYPE_INTEGER_32;
    msg->intParams.msg_type_size = 32;
    msg->intParams.msg_type_number = NX_ISM_NUMINTPARAMS;
    msg->intParams.msg_type_inline = 1;
    msg->intParams.msg_type_longform = 0;
    msg->intParams.msg_type_deallocate = 0;

    msg->floatParams.msg_type_name = MSG_TYPE_REAL;
    msg->floatParams.msg_type_size = 32;
    msg->floatParams.msg_type_number = NX_ISM_NUMFLOATPARAMS;
    msg->floatParams.msg_type_inline = 1;
    msg->floatParams.msg_type_longform = 0;
    msg->floatParams.msg_type_deallocate = 0;

    for (cnt = 0; cnt < numChannels; cnt++) {     
	static const msg_type_long_t DataParamTemplate = 
		    {{0, 0, 0, 0, 1, 0, 0}, MSG_TYPE_BYTE, 8, 0};
	msg->channels[cnt].dataParams = DataParamTemplate;
    }

    msg->tag = ++(conn->tag);

    msg->numChannels = numChannels;
  
    return msg->tag;
}


static kern_return_t SendMessage (NXImageServerMessage *msg)
{
    kern_return_t ret;
    if ((ret = msg_send((msg_header_t *)msg, SEND_TIMEOUT, 10000)) != 
		KERN_SUCCESS){
	NXLogError ("Error %d sending message to image server", ret);
    }
    return ret;
}

BOOL _NXImageRequestDone (int request, unsigned char **data, int *dataLen, int *memLen, BOOL wait, NXJPEGInfo *jpeg)
{
    kern_return_t ret;
    NXImageServerMessage msg;
    typedef struct _RequestMemory {	/* Structure used to remember fulfilled requests. */
	unsigned char *imageData;
	int imageDataLen;
	int memoryLen;
	NXJPEGInfo jpeg;
    } RequestMemory;
    int cnt;

    *data = NULL;

    msg.header.msg_size = sizeof(NXImageServerMessage);
    msg.header.msg_local_port = conn->clientPort;

    do {
	if ((request > conn->lastFullfilledRequest) && wait) {
	    /* Try to hear from the server, quitting only if the server is dead. */	    
	    while ((ret = msg_receive((msg_header_t *)&msg, RCV_TIMEOUT, 15000))
			!= KERN_SUCCESS ) {
		port_t imagePort;
		if ((ret = netname_look_up(name_server_port, conn->hostName,
			NX_IMAGE_PORT, &imagePort)) != KERN_SUCCESS) break;
	    }
	} else {
	    ret = msg_receive((msg_header_t *)&msg, RCV_TIMEOUT, 0);
	}
	if (ret == KERN_SUCCESS) {
	    BOOL failed = (msg.numChannels != 1);
	    conn->lastFullfilledRequest = msg.tag;
	    AK_ASSERT(msg.numChannels < 2, "More than one channel returned by imageserver.");
	    if (request == msg.tag) {
		/* Copy the result instead of storing it. */
		*data = failed ? NULL : msg.channels[0].imageData;
		*dataLen = failed ? 0 : msg.imageDataLen;
		*memLen = failed ? 0 : msg.channels[0].dataParams.msg_type_long_number;
		for (cnt = 0; cnt < MAXSAMPLES; cnt++) {
		    jpeg->JPEGQMatrixData[cnt] = msg.JPEGQMatrixData[cnt];
		    jpeg->JPEGACTables[cnt] = msg.JPEGACTables[cnt];
		    jpeg->JPEGDCTables[cnt] = msg.JPEGDCTables[cnt];
		}
		for (cnt = 0; cnt < 2 * MAXSAMPLES; cnt++) {
		    jpeg->Subsample[cnt] = msg.NewComponentSubsample[cnt];
		}
		jpeg->dataOffset = msg.dataOffset;
		jpeg->JPEGMode = msg.JPEGMode;
		jpeg->JPEGQFactor = msg.JPEGQFactor;
		jpeg->JPEGQMatrixPrecision = msg.JPEGQMatrixPrecision;
	    } else {	/* Keep the data around for someone else... */
		RequestMemory *mem = malloc(sizeof(RequestMemory));		    
		if (!conn->processedData) {
		    conn->processedData = [HashTable newKeyDesc:"i" valueDesc:"!"];
		}
		mem->imageData = failed ? NULL : msg.channels[0].imageData;
		mem->imageDataLen = failed ? 0 : msg.imageDataLen;
		mem->memoryLen = failed ? 0 : msg.channels[0].dataParams.msg_type_long_number;
		for (cnt = 0; cnt < MAXSAMPLES; cnt++) {
		    mem->jpeg.JPEGQMatrixData[cnt] = msg.JPEGQMatrixData[cnt];
		    mem->jpeg.JPEGACTables[cnt] = msg.JPEGACTables[cnt];
		    mem->jpeg.JPEGDCTables[cnt] = msg.JPEGDCTables[cnt];
		}
		for (cnt = 0; cnt < 2 * MAXSAMPLES; cnt++) {
		    mem->jpeg.Subsample[cnt] = msg.NewComponentSubsample[cnt];
		}
		mem->jpeg.dataOffset = msg.dataOffset;
		mem->jpeg.JPEGMode = msg.JPEGMode;
		mem->jpeg.JPEGQFactor = msg.JPEGQFactor;
		mem->jpeg.JPEGQMatrixPrecision = msg.JPEGQMatrixPrecision;
		[conn->processedData insertKey:(void *)msg.tag value:(void *)mem];
	    }
	}

    } while (ret == KERN_SUCCESS);

    if (request <= conn->lastFullfilledRequest) {
	RequestMemory *mem;
	if (!*data && 
	    (mem = (RequestMemory *)[conn->processedData removeKey:(void *)request])) {
	    *data = mem->imageData;
	    *dataLen = mem->imageDataLen;
	    *memLen = mem->memoryLen;
	    *jpeg = mem->jpeg;
	    free (mem);
	}
	return *data ? YES : NO;
    } else {
	return NO;				
    }
}

/*
 * This function takes either one channel of JPEG (stored in data[0])
 * or 1..5 channels of bitmap data (data[n]) and decopresses or
 * compresses it. dataLen is the number of bytes in each channel or
 * source, numChannels is the number of channels, and compression is 
 * the desired JPEG compression factor. 
 */
static int DoJPEGRequest (
    NXImageServerRequest request,
    unsigned char *data[5],
    int dataLen,
    int numChannels,
    int w, int h, int bps, int spp, BOOL isPlanar, BOOL hasAlpha,
    NXColorSpace colorSpace,
    float llx,
    float lly,
    float urx,
    float ury,
    float destCTM[6],
    float compression,
    int destWin,
    NXJPEGInfo *jpeg)
{
    NXImageServerMessage msg;
    int tag = InitMessage (&msg, numChannels);
    int cnt;

    if (!tag) {
	return 0;
    }

    msg.header.msg_id = request;
     
    msg.pixelsWide = w;
    msg.pixelsHigh = h;
    msg.bytesPerRow = 0;
    msg.bitsPerSample = bps;
    msg.samplesPerPixel = spp;
    msg.colorSpace = (int)colorSpace;
    msg.isPlanar = isPlanar ? 1 : 0;
    msg.hasAlpha = hasAlpha ? 1 : 0;

    msg.imageDataLen = dataLen;

    msg.compressionFactor = compression;

    for (cnt = 0; cnt < numChannels; cnt++) {
	msg.channels[cnt].dataParams.msg_type_long_number = dataLen;
	msg.channels[cnt].imageData = data[cnt];
    }

    return (SendMessage (&msg) == KERN_SUCCESS) ? tag : 0;

}

/*
 * Convert a JPEG bitstream to uncompressed bitmap.
 * jpeg points to the JPEG data whose length is given in jpegLen. On return,
 * *data points to a chunk of memory of size *memoryLen; this data block
 * contains all the data (if planar, planes are laid consecutively). The returned
 * block of memory should be freed with vm_deallocate() using a size of memoryLen.
 *
 * If wait is YES, then this function only returns once the data is compressed;
 * return value of 0 indicates failure.  Otherwise a tag that can be used
 * with _NXImageRequestDone() is returned.
 */
int _NXConvertJPEGToBitmap (
    int w, int h, int bps, int spp, BOOL isPlanar, BOOL hasAlpha,
    NXColorSpace colorSpace,
    unsigned char *jpeg, int jpegLen, BOOL wait,
    unsigned char **data, int *memoryLen, NXJPEGInfo *jpegInfo)
{
    float dummy[6];
    int tag, dataLen;
    unsigned char *jpegArr[5];

    jpegArr[0] = jpeg;    
    tag = DoJPEGRequest (NX_ImageJPEGDecompress, jpegArr, jpegLen, 1,
			w, h, bps, spp,
			isPlanar, hasAlpha, colorSpace,
			0.0, 0.0, 0.0, 0.0, dummy, 0.0, 0, jpegInfo);
    if (tag && wait) {
	/* ??? Should do a sanity check with the return value */
	return _NXImageRequestDone (tag, data, &dataLen, memoryLen, YES, jpegInfo) ? tag : 0;
    }
	
    return tag;        
}

/*
 * Convert a uncompressed bitmap to a JPEG bitstream.
 * data[0..numChannels-1] point at the data channels, each of
 * which contain dataLen bytes. On return, *jpeg points to a chunk of memory
 * *memoryLen in size; the first *jpegLen bytes of this memory are the JPEG
 * compressed data. The caller should free this memory with vm_deallocate().
 *
 * If wait is YES, then this function only returns once the data is compressed;
 * return value of 0 indicates failure.  Otherwise a tag that can be used
 * with _NXImageRequestDone() is returned.
 */
int _NXConvertBitmapToJPEG (
    int w, int h, int bps, int spp, BOOL isPlanar, BOOL hasAlpha, NXColorSpace colorSpace,
    unsigned char *data[5], int dataLen, BOOL wait,
    unsigned char **jpeg, int *jpegLen, int *memoryLen,
    float compression, NXJPEGInfo *NXJPEGInfo)
{
    float dummy[6];
    int tag;

    tag = DoJPEGRequest (NX_ImageJPEGCompress, data, dataLen,
			isPlanar ? spp : 1, w, h, bps, spp,
			isPlanar, hasAlpha, colorSpace,
			0.0, 0.0, 0.0, 0.0, dummy, compression, 0, NXJPEGInfo);
    if (tag && wait) {
	return _NXImageRequestDone (tag, jpeg, jpegLen, memoryLen, YES, NXJPEGInfo) ? tag : 0;
    }

    return tag;        
}


#if 0
/* Not really working yet. -Ali */

static int DoPSRequest (
    unsigned char *ps,
    int dataLen,
    float llx,
    float lly,
    float urx,
    float ury,
    float destCTM[6],
    int destWin)
{
    NXImageServerMessage msg;
    int tag = InitMessage (&msg);

    if (!tag) {
	return 0;
    }

    msg->header.msg_id = NX_ImagePostScript;
     
    msg.pixelsWide = 
    msg.pixelsHigh =
    msg.bytesPerRow =
    msg.bitsPerSample =
    msg.samplesPerPixel =
    msg.colorSpace =
    msg.isPlanar = 0;

    msg.destWin = destWin;
    msg.imageDataLen = psLen;

    {int cnt; for (cnt = 0; cnt < 6; cnt++) msg.destCTM[cnt] = destCTM[cnt];}

    msg.llx = llx;
    msg.lly = lly;
    msg.urx = urx;
    msg.ury = ury;

    msg.dataParams.msg_type_long_number = psLen;
    msg.imageData = ps;

    return (SendMessage (&msg) == KERN_SUCCESS) ? tag : 0;

}

#endif

/*

Created: 8/10/90 by aozer

Modifications (since 91):

93
--
 9/4/90 aozer	Made the JPEG (de)compression functions deal with
		alpha and multiple channels of data. Also return the
		memory size allocated by the server back to the client.

98
--
 10/19/90 aozer	Augmented to pass the new JPEG fields back and forth.
 10/20/90 aozer	Made imageserver always run on the client machine; at least
		for 2.0, running imageserver on the server machine will be less
		efficient.

*/


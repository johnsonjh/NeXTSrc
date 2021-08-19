/*
	chunk.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <zone.h>
/*
 *  NXChunks are implement variable sized arrays of records.  allocation is by
 *  the given size (in bytes) typically a multiple number of records, say 10.
 *  The block of memory never shrinks, and the chunk records the current number
 *  of elements.  To use the NXchunks, you declare a struct w/ NXChunk as its
 *  first field.
 */
 
typedef struct _NXChunk {
    short           growby;	/* increment to grow by */
    int             allocated;	/* how much is allocated */
    int             used;	/* how much is used */
} NXChunk;

extern NXChunk *NXChunkMalloc(int growBy, int initUsed);
extern NXChunk *NXChunkRealloc(NXChunk *pc);
extern NXChunk *NXChunkGrow(NXChunk *pc, int newUsed);
extern NXChunk *NXChunkCopy(NXChunk *pc, NXChunk *dpc);

extern NXChunk *NXChunkZoneMalloc(int growBy, int initUsed, NXZone *zone);
extern NXChunk *NXChunkZoneRealloc(NXChunk *pc, NXZone *zone);
extern NXChunk *NXChunkZoneGrow(NXChunk *pc, int newUsed, NXZone *zone);
extern NXChunk *NXChunkZoneCopy(NXChunk *pc, NXChunk *dpc, NXZone *zone);

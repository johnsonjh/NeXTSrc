/*
	cursorprivate.h
  	Copyright 1987, NeXT, Inc.
	Responsibility: Ali Ozer
  	
  	This file is private to Cursor.m
*/

#ifndef GRAPHICS_H
#import "graphics.h"
#endif GRAPHICS_H

#define CURSORSIZE      (16.0)

#define NORTH_ARROW 	(0)
#define NORTHEAST_ARROW (1)
#define EAST_ARROW      (2)
#define SOUTHEAST_ARROW (3)
#define SOUTH_ARROW     (4)
#define SOUTHWEST_ARROW (5)
#define WEST_ARROW      (6)
#define NORTHWEST_ARROW (7)
#define NULL_ARROW      (8)

#define VERTICAL	(0)
#define POSITIVE	(1)
#define HORIZONTAL	(2)
#define NEGATIVE	(3)

typedef char    NXBits16x16x2[64];

typedef struct _NXCursorData {
    NXPoint pos;
    char     hotX, hotY;
}               NXCursorData;




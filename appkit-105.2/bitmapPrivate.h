/*
	bitmapPrivate.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#import "graphics.h"

typedef struct _BitmapManager {
    id		window;
    id          bitmapList;
    id		focusView;
    int        	type;
    float	maxX;
}               BitmapManager;

#define BITMAPMGRSIZE	(sizeof(BitmapManager))
#define BITMAPMGRDESCR	"{@@@if}"

typedef struct  {
    char               *name;
    int                 width, height;
    id                  bitmap;
    BOOL                original;
} IconInfo;

typedef struct {
    char               *name;
    NXRect              rect;
    BOOL		alpha;
} PrebuiltInfo;

typedef struct {
    id window;
    BitmapManager *manager;
} ClientAllocated;





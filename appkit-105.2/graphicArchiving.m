/*
	graphicArchiving.m
  	Copyright 1989, NeXT, Inc.
	Responsibility: Bill Parkhurst
	  
	DEFINED IN: The Application Kit
	HEADER FILES: appkit.h
*/

/*	This file provides read and write operations for graphics */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "graphics.h"

void
NXWritePoint(NXTypedStream * s, const NXPoint *aPoint)
{
    NXWriteType(s, "ff", aPoint);
};


void
NXReadPoint(NXTypedStream * s, NXPoint *aPoint)
{
    NXReadType(s, "ff", aPoint);
};


void
NXWriteSize(NXTypedStream * s, const NXSize *aSize)
{
    NXWriteType(s, "ff", aSize);
};


void
NXReadSize(NXTypedStream * s, NXSize *aSize)
{
    NXReadType(s, "ff", aSize);
};


void
NXWriteRect(NXTypedStream * s, const NXRect *aRect)
{
    NXWriteType(s, "ffff", aRect);
};


void
NXReadRect(NXTypedStream * s, NXRect *aRect)
{
    NXReadType(s, "ffff", aRect);
};


/*
  
Modifications (starting at 0.8):
  
1/27/89 bs	Created; added functions for read/write.
3/22/89 wrp	fixed NX{Read,Write}{Point,Size,Rect} to not pass structs on the stack
		 but instead pass pointers to said structs.
  
  
*/

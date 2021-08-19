/*
	ColumnCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Greg Cockroft
	  
	DEFINED IN: The Application Kit
	HEADER FILES: ColumnCell.h
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "ColumnCell.h"
#import "Text.h"
#import <dpsclient/wraps.h>
#import <stdlib.h>
#import <string.h>

static void doHighlight(id self, BOOL flag, const NXRect *cellFrame);
static void realocDataPtrs(ColumnCell *self);

@implementation ColumnCell:Cell


- free
{
    if (data)
	free(data);
    if (_stringBuf)
	free(_stringBuf);
    return[super free];
}


- (int)numColumns
{
    return numColumns;
}


- setNumColumns:(int)num
{
    numColumns = num;
    return self;
}


- (float *)tabs
{
    return tabs;
}


- setTabs:(const float *)tabList
{
    tabs = (float *)tabList;
    return self;
}


- (const char * const *)data
{
    return data;
}


- setData:(const char * const *)stringList
{
    unsigned int        totalLen;
    int                 i;
    const char         * const * sPtr;
    const char         *src;
    char               *dest;

    totalLen = 0;
    for (i = numColumns, sPtr = stringList; i--; sPtr++)
	totalLen += strlen(*sPtr) + 1;
    _stringBuf = NXZoneRealloc([self zone], _stringBuf, totalLen);
    dest = _stringBuf;
    realocDataPtrs(self);
    for (i = 0, sPtr = stringList; i < numColumns; i++, sPtr++) {
	src = *sPtr;
	data[i] = dest;
	while (*dest++ = *src++)
	    ;
    }
 /* gives calcCellSize something to calc */
    [self setStringValueNoCopy:data[0]];
    return self;
}


- setDataNoCopy:(const char *const *)stringList
{
    const char         * const * sPtr;
    int			i;

    if (_stringBuf) {
	free(_stringBuf);
	_stringBuf = NULL;
    }
    realocDataPtrs(self);
    for (i = 0, sPtr = stringList; i < numColumns; i++, sPtr++)
	data[i] = *sPtr;
 /* gives calcCellSize something to calc */
    [self setStringValueNoCopy:data[0]];
    return self;
}


static void realocDataPtrs(ColumnCell *self)
{
    if (self->numColumns != self->_colsAlloced) {
	self->data = NXZoneRealloc([self zone], self->data, self->numColumns * sizeof(char *));
	self->_colsAlloced = self->numColumns;
    }
}


- (BOOL)isOpaque
{
    return YES;
}


- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect
{
    return[super calcCellSize:theSize inRect:aRect];
 /* should calc the size based on how we layout the cols??? */
}


- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    return[self drawInside:cellFrame inView:controlView];
}


- drawInside:(const NXRect *)cellFrame inView:controlView
{
    NXRect              aRect;       
    int                 i;
    char               *oldContents;
    float               offset;

    aRect = *cellFrame;
    if (!cFlags1.bezeled) {
	PSsetgray(NX_LTGRAY);
	NXRectFill(&aRect);
    }
    oldContents = contents;
    offset = (cFlags1.bordered || cFlags1.bezeled) ? (cFlags1.bordered ? 2.0 : 3.0) : 0.0;
    if (!cFlags1.bezeled) NXInsetRect(&aRect, offset, offset);
    for (i = 0; i < numColumns; i++) {
	contents = (char *)data[i];
	aRect.origin.x = tabs[i];
	if (i == numColumns - 1) {
	    aRect.size.width = NX_MAXX(cellFrame) - (cFlags1.bezeled ? 0.0 : offset) - tabs[i];
	} else {
	    aRect.size.width = tabs[i+1] - tabs[i];
	}
	if (NX_MAXX(&aRect) > NX_MAXX(cellFrame)) break;
	if (i < numColumns - 2 && tabs[i+2] > NX_MAXX(cellFrame)) {
	    aRect.size.width = NX_MAXX(cellFrame) - aRect.origin.x;
	}
	if (cFlags1.bezeled) {
	    if (NX_MAXX(&aRect) != NX_MAXX(cellFrame)) aRect.size.width -= 4.0;
	    NXDrawGrayBezel(&aRect, NULL);
	    PSsetgray(NX_DKGRAY);
	    NXInsetRect(&aRect, 2.0, 2.0);
	    NXRectFill(&aRect);
	    NXInsetRect(&aRect, 1.0, 1.0);
	}
	_NXDrawTextCell(self, controlView, &aRect, NO);
	if (cFlags1.bezeled) NXInsetRect(&aRect, -3.0, -3.0);
    }
    contents = oldContents;
    if (cFlags1.state || cFlags1.highlighted)
	doHighlight(self, YES, cellFrame);
    return self;
}

- setTextAttributes:textObj
{
    [super setTextAttributes:textObj];
    if (cFlags1.bezeled) [textObj setTextGray:NX_WHITE];
    return self;
}

- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag
{
    if (cFlags1.highlighted != flag) {
	cFlags1.highlighted = (int)flag;
	if (!cFlags1.state)
	    doHighlight(self, flag, cellFrame);
    }
    return self;
}


static void
doHighlight(ColumnCell *self, BOOL flag, const NXRect *cellFrame)
{
    NXRect              _aRect;
    register NXRect    *aRect = &_aRect;

    NXHighlightRect(cellFrame);
    if (self->cFlags1.bordered) {
	*aRect = *cellFrame;
	PSsetgray(flag ? NX_DKGRAY : NX_LTGRAY);
	aRect->size.height = 1.0;
	NXRectFill(aRect);
	aRect->origin.y += cellFrame->size.height - 1.0;
	NXRectFill(aRect);
    }
}

/*??? ColumnCells are currently not public and not archived anywhere.
      This code needs some work, especially dealing with the shared
      layout info between cells.
 */

- write:(NXTypedStream *) stream
{
    int                 i;
    int                 dumNum;
    int                 len;

    [super write:stream];
    NXWriteType(stream, "s", &numColumns);
    NXWriteArray(stream, "f", numColumns, tabs);
    for (i = 0, len = 0; i < numColumns; i++)
	len += strlen(data[i]) + 1;
    NXWriteType(stream, "i", &len);
    NXWriteArray(stream, "c", len, _stringBuf);
    for (i = 0; i < numColumns; i++) {
	dumNum = data[i] - _stringBuf;
	NXWriteType(stream, "i", &dumNum);
    }
    return self;
}


- read:(NXTypedStream *) stream
{
    int                 i;
    int                 len;
    int                 dumNum;

    [super read:stream];
    NXReadType(stream, "s", &numColumns);
    tabs = NXZoneMalloc([self zone], numColumns * sizeof(float));
    NXReadArray(stream, "f", numColumns, tabs);
    data = NXZoneMalloc([self zone], numColumns * sizeof(char *));
    NXReadType(stream, "i", &len);
    _stringBuf = NXZoneMalloc([self zone], len * sizeof(char));
    NXReadArray(stream, "c", len, _stringBuf);
    for (i = 0; i < numColumns; i++) {
	NXReadType(stream, "i", &dumNum);
	data[i] = _stringBuf + dumNum;
    }
    return self;
}

@end


/*
  
Modifications (starting at 0.8):
  
0.92
----
 6/12/89	added setDataNoCopy:
*/


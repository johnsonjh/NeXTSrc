/*

	ColumnCell.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Greg Cockroft
*/


#import "Cell.h"

@interface ColumnCell:Cell
{
    short numColumns;	
    short _colsAlloced;	
    float *tabs;	
    const char **data;	
    char *_stringBuf;	
}

- free;
- (int)numColumns;
- setNumColumns:(int)num;
- (float *)tabs;
- setTabs:(const float *)tabList;
- (const char **)data;
- setData:(const char * const *)stringList;
- setDataNoCopy:(const char * const *)stringList;
- (BOOL)isOpaque;
- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- drawInside:(const NXRect *)cellFrame inView:controlView;
- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;
@end

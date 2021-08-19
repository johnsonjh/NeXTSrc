/*
 * font.h -- definitions used in generating bitmap fonts for newblit
 */

typedef struct {
	short width;
	short height;
	short xoff;
	short yoff;
} bbox_t;

typedef struct {
	bbox_t bbx;
	short dwidth;
	int bitx;
} bitmap_t;

#define	FONTNAMELEN		128
#define	ENCODEBASE		0x20
#define	ENCODELAST		0x7e

typedef struct {
	char font[FONTNAMELEN+1];
	unsigned short size;
	bbox_t bbx;
	bitmap_t bitmaps[ENCODELAST - ENCODEBASE + 1];
	unsigned char bits[0];
} font_t;



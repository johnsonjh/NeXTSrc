/*
 * fbshow -- show text strings and images in frame buffer
 *
 * FIXME: What a mess all the args to this thing are!
 *
 * Mike DeMoney, NeXT Inc.
 * Copyright 1990.  All rights reserved.
 *
 * TODO:  Add option to display arbitrary eps images.
 *	Ask Keith how to calculate default lead.
 *	Ask Keith preferred format of arbitrary images.
 *	Get images for boot panel.
 * 
 * fbshow [-f FONT] [-d FONTDIR] [-p POINT] [-x XPOS] [-y YPOS] [-h HEIGHT]
 *	[-w WIDTH] [-l LEAD] [-g BGRNDCOLOR] [-c FRGRNCOLOR] [-s LINES_TO_CENTER]
 *  [-m X_MARGIN] [-A] [-S] [-N] [-M] [-E] [MSGS]
 */
#import <stdio.h>
#import <ctype.h>
#import <stdarg.h>
#import <mach.h>
#import <libc.h>
#import <sys/param.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sys/file.h>
#import <sys/boolean.h>
#import <sys/ioctl.h>
#import <nextdev/video.h>
#import <nextdev/kmreg.h>
#import "font.h"

#define	BITSPIXEL		2
#define PIXELS_PER_BYTE		(8/BITSPIXEL)
#define	FBWIDTH			1152
#define	FRAME_BUFFER	"/dev/vid0"
#define CONSOLE		"/dev/console"

#define	BLACK_PX		3
#define	DKGRAY_PX		2
#define	LTGRAY_PX		1
#define	WHITE_PX		0

typedef enum {
	ANIMATE, SUSPEND_ANIMATION, CLEAR_ANIMATION
} animate_t;

/*
 * Command line parameters and defaults
 */
const char *font = "Helvetica";
const char *fontdir = "/usr/lib/bootimages/";
int point = 18;
int xorg = 490;
int yorg = 330;
int height = 130;
int width = 265;
int bg = LTGRAY_PX;
int fg = BLACK_PX;
int lead;
boolean_t lead_set = FALSE;
boolean_t autonl = TRUE;
animate_t animate = CLEAR_ANIMATION;
int xmargin = 20;
int ymargin = 0;
int Erase = 1;			/* Always erase.  Otherwise, it looks bad... */

/*
 * The current position
 */
int xpos;
int ypos;

/*
 * Internal types
 */
typedef struct thought thought_t;
struct thought {
	char *filename;
	font_t *fontp;
	thought_t *next;
};

/*
 * General globals
 */
unsigned char *fb;					/* pseudo-frame buffer */
int fb_dev = -1;					/* The bitmap console device */
font_t *fontp;						/* current font */
const char *program_name;				/* program name for error messages */
thought_t *thoughts = NULL;

/*
 * Internal procedure declarations
 */
static int atob(const char *str);
static void blit_string(const char *string);
static void blit_bm(bitmap_t *bmp);
static font_t *open_font(const char *fontname);
static void blit_bm(bitmap_t *bmp);
static void fatal(const char *format, ...);
static void check_console(void);
static void open_fb(char *);
static void center(int lines);
static font_t *recollect(char *fontname);
static void remember(char *fontname, font_t *fontp);
static char *newstr(char *string);
static unsigned char * bm_malloc( unsigned size );
static void image_bitmap();
    
/*
 * Internal inline procedures
 */
static inline bitmap_t *getbm(char c);
static inline int bmbit(bitmap_t *bmp, int x, int y);
static inline void setfbbit(int x, int y, int v);
static inline void flushbits(void);
static inline unsigned digit(char c);
static inline void *ckmalloc(int len);
static inline char *newstr(char *string);
/*
 * Handy internal macros
 */
#define	streq(a, b)		(strcmp(a, b) == 0)
#define	NEW(type, num)	((type *)ckmalloc(sizeof(type)*(num)))

void
main(int argc, const char * const argv[])
{
	const char *arg = NULL, *argp;
	char c;
	
 	program_name = *argv++; argc--;

	/* Parse command line args */
	while (argc > 0) {
		if (**argv == '-') {
			argp = *argv++ + 1; argc--;
			while (c  = *argp++) {
				if (islower(c)) {
					if (argc < 1)
						fatal("Option %c takes arg", c);
					arg = *argv++; argc--;
				}
				switch (c) {
				case 'A':
					animate = ANIMATE;
					break;
				case 'S':
					animate = SUSPEND_ANIMATION;
					break;
				case 'g':
					bg = atob(arg);
					break;
				case 'f':
					font = arg;
					break;
				case 'p':
					point = atob(arg);
					break;
				case 'x':
					xorg = atob(arg);
					break;
				case 'y':
					yorg = atob(arg);
					break;
				case 'h':
					height = atob(arg);
					break;
				case 'w':
					width = atob(arg);
					break;
				case 'l':
					lead = atob(arg);
					lead_set = 1;
					break;
				case 'L':
					lead_set = 0;
					break;
				case 'c':
					fg = atob(arg);
					break;
				case 'd':
					fontdir = arg;
					break;
				case 'm':
					xmargin = atob(arg);
					break;
				case 'M':
					autonl = FALSE;
					break;
				case 'N':
					autonl = TRUE;
					break;
				case 'B':
					check_console();
					break;
				case 'E':
					Erase = 1;
					break;
				case 's':
					center(atob(arg));
					break;
				default:
					fatal("Unknown option: %c", c);
				}
			}
		} else {
			/* Blit the string */
			blit_string(*argv++); argc--;
		}
	}
	image_bitmap();	/* Draw the offscreen bitmap onto the display */
}

void
image_bitmap()
{
	struct km_drawrect rect;
	
	if ( fb_dev == -1 )
		open_fb( FRAME_BUFFER );
	
	if ( fb == NULL )
		return;		/* Nothing to image??? */
	
	rect.x = xorg;
	rect.y = yorg;
	rect.width = width;
	rect.height = height;
	rect.data.bits = (void *)fb;
	
	if ( ioctl( fb_dev, KMIOCDRAWRECT, &rect ) == -1 )
		perror( "KMIOCDRAWRECT" );
}

void
image_rect( x, y, w, h )
    int x, y, w, h;
{
	struct km_drawrect rect;
	
	if ( fb_dev == -1 )
		open_fb( FRAME_BUFFER );
	
	if ( fb == NULL )
		return;		/* Nothing to image??? */
	
	rect.x = x;
	rect.y = y;
	rect.width = w;
	rect.height = h;
	rect.data.bits = (void *)fb;
	
	if ( ioctl( fb_dev, KMIOCDRAWRECT, &rect ) == -1 )
		perror( "KMIOCDRAWRECT" );
}


static void
center(int lines)
{
	int deflead, space;

	if (lines <= 0)
		return;	
	if (fontp == NULL || strcmp(font, fontp->font) != 0)
		fontp = open_font(font);
		
	/* Round up origin points. */
	xorg = (xorg + PIXELS_PER_BYTE - 1) & ~(PIXELS_PER_BYTE - 1);
	/* Round down width */
	width &= ~(PIXELS_PER_BYTE - 1);
	
	deflead = fontp->bbx.height + (fontp->bbx.height + 9) / 10;
	space = height - lines * (lead_set ? lead : deflead);
	if (lead_set)
		ymargin = space / 2;
	else {
		space /= 6 + lines - 1;
		ymargin = space * 3;
		lead = deflead + space;
		lead_set = TRUE;
	}
}

static void
check_console(void)
{
	int cfd;
	int kmflags;
	
	if ((cfd = open("/dev/console", O_RDONLY, 0)) < 0)
		exit(0);
	if (ioctl(cfd, KMIOCGFLAGS, &kmflags) < 0)
		exit(0);
	if (kmflags & KMF_SEE_MSGS)
		exit (0);
	(void) close (cfd);
}

static font_t *
open_font(const char *fontname)
{
	char filename[MAXPATHLEN];
	char *slash;
	struct stat st;
	kern_return_t result;
	int ffd;
	font_t *fontp;
	
	slash = (fontdir[strlen(fontdir)-1] == '/') ? "" : "/";
	sprintf(filename, "%s%s%s.%d", fontdir, slash, fontname, point);
	if (fontp = recollect(filename))
		return fontp;
	if ((ffd = open(filename, O_RDONLY, 0)) < 0)
		fatal("Can't open font file %s", filename);
	if (fstat(ffd, &st) < 0)
		fatal("Can't stat font file %s", filename);
	result = map_fd(ffd, (vm_offset_t)0, (vm_offset_t *)&fontp, TRUE, st.st_size);
	if (result != KERN_SUCCESS)
		fatal("Can't map font file %s", filename);
	close(ffd);
	remember(filename, fontp);
	return fontp;
}

static inline void *
ckmalloc(int len)
{
	void *p;
	
	p = malloc(len);
	if (p == NULL)
		fatal("Out of memory");
	return p;
}

static inline char *
newstr(char *string)
{
	int len;
	char *newstrp;
	
	len = strlen(string) + 1;
	newstrp= (char *)ckmalloc(len);
	if (newstrp == NULL)
		fatal("Out of memory");
	bcopy(string, newstrp, len);
	return newstrp;
}

static void
remember(char *filename, font_t *fontp)
{
	thought_t *tp;
	
	if (recollect(filename))
		return;
	tp = NEW(thought_t, 1);
	tp->filename = newstr(filename);
	tp->fontp = fontp;
	tp->next = thoughts;
	thoughts = tp;
}

static font_t *
recollect(char *filename)
{
	thought_t *tp;
	
	for (tp = thoughts; tp; tp = tp->next)
		if (streq(tp->filename, filename))
			return tp->fontp;
	return NULL;
}

static inline bitmap_t *
getbm(char c)
{
	return &fontp->bitmaps[(c & 0x7f) - 0x20];
}

/*
 * Currently, this function just calls /dev/vid0 to control the animation.
 * The actual drawing code is driven off of /dev/console.
 */
static void
open_fb(char *video_dev)
{
	int vfd;
	unsigned char *fbp;
	
	if ((vfd = open(video_dev, O_RDWR, 0)) < 0)
		fatal("Can't open frame buffer %s", video_dev);
	switch (animate) {
	case ANIMATE:
		fbp = (unsigned char *)CONTINUE_ANIM;
		break;
	case SUSPEND_ANIMATION:
		fbp = (unsigned char *)STOP_ANIM;
		break;
	default:
	case CLEAR_ANIMATION:
		fbp = NULL;
		break;
	}
	/* Called solely for animation control side effects! */
	if (ioctl(vfd, DKIOCGADDR, &fbp) < 0)
		fatal("Can't map frame buffer");
	close(vfd);
	
	if ((fb_dev = open(CONSOLE, O_WRONLY, 0)) < 0)
		fatal("Can't open console %s", CONSOLE);
}

static void
blit_string(const char *string)
{
	int c;
	bitmap_t *bmp;
	int deflead;
	static boolean_t first_line = TRUE;
	
	if ( fb_dev == -1 )
		open_fb( FRAME_BUFFER );
	if (fontp == NULL || strcmp(font, fontp->font) != 0)
		fontp = open_font(font);
		
	/* Round up origin points. */
	xorg = (xorg + PIXELS_PER_BYTE - 1) & ~(PIXELS_PER_BYTE - 1);
	/* Round down width and height */
	width &= ~(PIXELS_PER_BYTE - 1);
	
	deflead = fontp->bbx.height + (fontp->bbx.height + 9) / 10;
	if (first_line) {
		first_line = FALSE;
		xpos = xmargin;
		ypos = ymargin + fontp->bbx.height;
	}
	while (c = *string++) {
		if (c == '\\') {
			switch (c = *string++) {
			case 'n':
				c = '\n';
				break;
			}
		}				
		if (c == '\n') {
			xpos = 0;
			ypos += lead_set ? lead : deflead;
			continue;
		}
		bmp = getbm(c);
		blit_bm(bmp);
	}
	if (autonl) {
		xpos = xmargin;
		ypos += lead_set ? lead : deflead;
	}
}

static inline int
bmbit(bitmap_t *bmp, int x, int y)
{
	int bitoffset;
	
	bitoffset = bmp->bitx + y * bmp->bbx.width + x;
	return (fontp->bits[bitoffset >> 3] >> (7 - (bitoffset & 0x7))) & 1;
}

static unsigned char *lastfbaddr;
static unsigned char fbbyte;

/*
 * Set a bit in the 'frame buffer'.  Assumes that left-most displayed pixel
 * is most significant in the frame buffer byte.  Also that 0,0 is at lowest
 * address of frame buffer.
 */
static inline void
setfbbit(int x, int y, int color)
{
	unsigned char *fbaddr;
	int bitoffset;
	int mask;
	int bitshift;
	
	if (fb == NULL)
		fb = bm_malloc( (height * width * BITSPIXEL) / 8 );
	bitoffset = ((y * width) + x) * BITSPIXEL;
	fbaddr = fb + (bitoffset >> 3);
	if (fbaddr != lastfbaddr) {
		if (lastfbaddr)
			*lastfbaddr = fbbyte;
		fbbyte = *fbaddr;
		lastfbaddr = fbaddr;
	}
	bitshift = (8 - BITSPIXEL) - (bitoffset & 0x7);
	color <<= bitshift;
	mask = ~(-1 << BITSPIXEL) << bitshift;
	fbbyte = (fbbyte & ~mask) | (color & mask);
}

static inline void
flushbits(void)
{
	if (lastfbaddr)
		*lastfbaddr = fbbyte;
}

/*
 * Assumes bitmap is in PS format, ie. origin is lower left corner.
 */
static void
blit_bm(bitmap_t *bmp)
{
	int x, y;
	/*
	 * x and y are in fb coordinate system
	 * xoff and yoff are in ps coordinate system
	 */
	for (y = 0; y < bmp->bbx.height; y++) {
		for (x = 0; x < bmp->bbx.width; x++)
			setfbbit(xpos + bmp->bbx.xoff + x, ypos - bmp->bbx.yoff - y,
			   bmbit(bmp, x, (bmp->bbx.height - y - 1)) ? fg : bg);
	}
	flushbits();
	xpos += bmp->dwidth;
}

static unsigned char *
bm_malloc( size )
    unsigned size;
{
	unsigned char * bitmap = (unsigned char *)malloc( size );
	unsigned char value;
	unsigned int i;
	
	if ( bitmap == (unsigned char *) NULL )
		fatal( "malloc: couldn't allocate %d bytes.\n" );
	/* Sleazoid clear to background */
	value = 0;
	for ( i = 0; i < PIXELS_PER_BYTE; ++i )
		value |= (bg << (i * BITSPIXEL));
	for ( i = 0; i < size; ++i )
		bitmap[i] = value;
	return ( bitmap );
}
/*
 * digit -- convert the ascii representation of a digit to its
 * binary representation
 */
static inline unsigned
digit(char c)
{
	unsigned d;

	if (isdigit(c))
		d = c - '0';
	else if (isalpha(c)) {
		if (isupper(c))
			c = tolower(c);
		d = c - 'a' + 10;
	} else
		d = 999999; /* larger than any base to break callers loop */

	return(d);
}

/*
 * atob -- convert ascii to binary.  Accepts all C numeric formats.
 */
static int
atob(const char *cp)
{
	int minus = 0;
	int value = 0;
	unsigned base = 10;
	unsigned d;

	if (cp == NULL)
		return(0);

	while (isspace(*cp))
		cp++;

	while (*cp == '-') {
		cp++;
		minus = !minus;
	}

	/*
	 * Determine base by looking at first 2 characters
	 */
	if (*cp == '0') {
		switch (*++cp) {
		case 'X':
		case 'x':
			base = 16;
			cp++;
			break;

		case 'B':	/* a frill: allow binary base */
		case 'b':
			base = 2;
			cp++;
			break;
		
		default:
			base = 8;
			break;
		}
	}

	while ((d = digit(*cp)) < base) {
		value *= base;
		value += d;
		cp++;
	}

	if (minus)
		value = -value;

	return value;
}

static void
fatal(const char *format, ...)
{
	va_list ap;
	
	va_start(ap, format);
	fprintf(stderr, "%s: ", program_name);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}
















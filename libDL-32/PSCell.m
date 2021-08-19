#import <appkit/appkit.h>
#import <appkit/Bitmap.h>
#import <appkit/Font.h>
#import "PSCell.h"
#import "filetypes.h"
#import <text/text.h>
#import <string.h>

@implementation  PSCell

#import <sys/types.h>
#import <sys/stat.h>

#define BULLETS	"  \267\267\267  "


static long
fdDate(s)
	char *s;
/*
 * Return size of file in bytes.
 */
{
	struct stat b;
	return (stat(s, &b) == -1)? 0L : (long) b.st_mtime;
}

static char *
fileDate(s)
	char *s;
{
	char *ctime();
	long l = fdDate(s);
	if (!l) return "  ---";
	return ctime(&l);
}

- setContents: (char const *)s {
	[super setStringValue:s];
	if (file) {
		free(file);
		file = (char *)0;
	}
}

- setStatus:(BOOL)stat
{
	status = stat;
}

- setStringValue:(char const*)s {
	[self setContents:s];
}

static char *asciiTypes[] = {
	"ascii",
	"c",
	"m",
	"h",
	"cl",
	"s",
	"csh",
	"make",
	"psw",
	"rtf",
	"sh",
	0
};

#define ASCII_TABLE_PATH "/usr/lib/indexing/files/English/textTypeTable"
static ixMatchTable *asciiTypeTable()
{
	static int loaded = 0;
	if( loaded) return &_ixTextTypeTable;
	loaded += _ixFileToMatchTable( ASCII_TABLE_PATH, &_ixTextTypeTable);
	return loaded ? &_ixTextTypeTable : NULL;
}
	
static int determineViewType(char *filetype)
{
	if (!filetype) return ASCII_TYPE;

	if ( !strcmp(filetype, "nr"))
		return TROFF_TYPE;
	if ( !strcmp(filetype, "man"))
		return MAN_TYPE;
	
	if( _ixCheckMatchTable( filetype, asciiTypeTable()))
		return ASCII_TYPE;
	
	return GENERIC_TYPE;
}

- (int) currentFileType {
	return viewType;
}

- setFileType: (int) itsType{
	viewType = itsType;
}

- setSmallIcons:(char *)name:lo:hi {
	iconName = name;
	viewType = determineViewType(name);
	icon[HI] = hi? hi : lo;
	icon[LO] = lo;
	icon[OFF] = nil;
}

- setContents: (char const *)s file:(char *)f :(char *)name:lo:hi
{
	extern char *strsave();

	[self setContents:s];

	file = strsave(f);
	[self setSmallIcons:(char *)name:lo:hi];
}

- setFonts:plain:bold {
	NXCoord	plainHt, plainAscend, plainDescend;
	NXCoord	boldHt, boldAscend, boldDescend, offset;
	NXSize s;

	plainFont = plain;
	boldFont = bold;
#	define max(a,b) (a>b? a : b)
	NXTextFontInfo(plain, &plainAscend, &plainDescend, &plainHt);
	NXTextFontInfo(bold, &boldAscend, &boldDescend, &boldHt);
	fontHeight = (float)max(boldHt, plainHt) - 
		     (float)max(boldDescend,plainDescend) + 5;
	mwidth = max([plain getWidthOf:"m"], [bold getWidthOf:"m"]);
#	undef max
}

- (char *) file { return file; }
- (char *) iconName { return iconName; }

static id curFont = (id)0, pFont, bFont;

static const float backColor[3] = { BACKLO_COLOR, BACKLO_COLOR, NX_WHITE};
static const float textColor[3] = { NX_DKGRAY, NX_BLACK, NX_BLACK};

#define lineGray NX_LTGRAY

static float curGray = NX_BLACK;

#define _rect(c,r) \
	PSsetgray(c); \
	PSrectfill(r.origin.x,r.origin.y,r.size.width,r.size.height)

#define isFont(c) ((c)==BOLD || (c)==PLAIN || (c)==TAB || (c)==CENTER)
#define setGray(c) if (curGray != c) PSsetgray(curGray=c)

/*
 * boldening routine
 */
#import <ctype.h>

static
same(s,t)
	char *s, *t;
{
	char a,b;
	/*if (Literal) return strcmp(s,t)==0;*/
	for (;*s && *t; s++, t++){
		a = isupper(*s)? tolower(*s) : *s;
		b = isupper(*t)? tolower(*t) : *t;
		if (a != b) return 0;
	}
	return 1;
}

bolden(s,word)
	char *s, *word;
/*
 * embolden occurrences of 'word' in bold face.
 */
{
	char t[1024],*p=t;
	int n = strlen(word), i;
	char *start = s;
	while (s && *s){
		if (same(s,word)){
			*p++ = BOLD;
			strncpy(p,s,n); p += n;
			*p++ = PLAIN;
			s += n;
		} else
			*p++ = *s++;
	}
	*p = '\0';
	strcpy(start,t);
}

static NXPoint marker[2];

typedef struct {
	@defs (Font)
} _IDFont ; 
#define FONTTOSCREEN(fid) (((_IDFont *) fid)->otherFont)
#define ISSCREENFONT(fid) (((_IDFont *) fid)->fFlags.isScreenFont)

static
setFont(s) char s; {
	id f;

	if (!isFont(s)) return 0;

	if (s>=TAB){
		PSmoveto(marker[MARK(s)].x, marker[MARK(s)].y);
		return 1;
	}

	f = (s == PLAIN? pFont : bFont);
	if (NXDrawingStatus != NX_PRINTING && !ISSCREENFONT(f) &&
		FONTTOSCREEN(f))
		f = FONTTOSCREEN(f);
	if (f != curFont)
		[curFont=f set];
	return 1;
}

static
_text(c,s)
	float c;
	char *s;
/*
 * draw 's' in the current fonts (pFont, bFont).
 * 's' is a string, possibly containing special characters
 * 'BOLD' and 'PLAIN' which trigger font changes.
 */
{
	char *p, x;
	BOOL center;

	center = (*s == CENTER);
	setGray(c);
	setFont(center ? BOLD : PLAIN);
	while (setFont(*s)) ++s;
	if (center) PSshow(BULLETS);
	p = s;
	while (*s){
		for (; *p && !isFont(*p); p++) ;
		x = *p;
		*p = '\0';
		while (setFont(*s)) ++s;
		PSshow(s);
		s = p;
		if (x) *p++ = x;
	}
	if (center) PSshow(BULLETS);
}

- drawSelf:(NXRect const *) R inView:view {

	NXRect r;
	NXPoint P;
	char	*myContents = [self stringValue];
	
	int state = (![self isEnabled] ? OFF : (cFlags1.highlighted ? HI : LO));
	if (status == NOTVALID) return;
	
	r = *R;
	pFont=curFont=plainFont;
	[plainFont set];
	bFont = boldFont;
	curGray = backColor[state];
	if (state == HI) r.origin.x++, r.size.width--;
	_rect(curGray, r);
	if (state == HI) r.origin.x--, r.size.width++;
	if (state == HI) {
		NXRect t;
		t = r;
		t.size.height = 1;
		if ([self tag])
		    _rect(NX_DKGRAY,t);
		t.origin.y = r.origin.y + r.size.height - 1.;
		_rect(NX_DKGRAY,t);
	}

	if (icon[LO]){
		NXRect R;
		id b = icon[state];
		if (b){
		   R.origin.x = R.origin.y = 0.;
		   [b getSize:&R.size];
		   R.origin.x = r.origin.x + 6;
		   R.origin.y = floor(r.origin.y+(r.size.height-R.size.height)/2);
        	   [b composite:NX_COPY toPoint:&R.origin];
		}
	}

	P.x = r.origin.x + 30.; P.y = r.origin.y+fontHeight-2.;
	marker[MARK(TAB)].y = marker[MARK(CENTER)].y = P.y;
	marker[MARK(TAB)].x = P.x + 12*mwidth;
	marker[MARK(CENTER)].x = r.origin.x - [boldFont getWidthOf:BULLETS] + 
			(r.size.width-[boldFont getWidthOf:myContents])/2;

	PSmoveto(P.x,P.y);
	_text(textColor[state], myContents);
	r.origin.y += r.size.height; r.size.height = 1;
	_rect(lineGray, r);
}

- (int)changeHighlight:(BOOL)flag {
	if (!cFlags1.disabled && cFlags1.highlighted != (int)flag) {
		cFlags1.highlighted = (int)flag;
		return YES;
	}
	return NO;
}

- highlight:(NXRect *) aRect inView:controlView lit:(BOOL)flag {
	if ([self changeHighlight:flag]) {
		[self drawSelf:aRect inView:controlView];
	}
	return self;
}

@end	
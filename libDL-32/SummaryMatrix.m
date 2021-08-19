#import <objc/List.h>

#import <appkit/appkit.h>
#import <appkit/Font.h>

#import "SummaryMatrix.h"
#import "PSCell.h"
#import "aux.h"
#import "filetypes.h"

#if 1
typedef struct {
	@defs (Font)
} _IDFont ; 

#define FONTTOSIZE(fid) (((_IDFont *) fid)->size)
#define FONTTOSCREEN(fid) (((_IDFont *) fid)->otherFont)
#define ISSCREENFONT(fid) (((_IDFont *) fid)->fFlags.isScreenFont)
#define ScreenFont(f) (ISSCREENFONT(f)? f : FONTTOSCREEN(f))
#endif

static id boldfont;

extern int AutoOpen;

@implementation  SummaryMatrix 

+ newFrame:(NXRect *)r {
	id			plainFont, boldFont;
	float			defaultSize = 12.0;
	const char	*defaultFontStr = NXGetDefaultValue([NXApp appName], "NXFont");
	const char	*defaultSizeStr =  NXGetDefaultValue([NXApp appName], "NXFontSize");

	self = [super newFrame:r];
	[self setBackgroundGray:BACKLO_COLOR];
	[self setAutoscroll:YES];
	[self allowEmptySel:YES];
	[self setTarget:self];
	[self setAction:@selector(click:)];
	[self setMode:NX_RADIOMODE];
	if( !defaultFontStr || !*defaultFontStr) {
		[self setFonts
			:[Font newFont:"Times-Roman" size:12.]
			:[Font newFont:"Times-Bold" size:12.]];
	} else {
		if( defaultSizeStr && *defaultSizeStr)
			sscanf( defaultSizeStr, "%f", &defaultSize);
		plainFont = [Font newFont: defaultFontStr size: defaultSize];
		boldFont = [[FontManager new] convert: plainFont toHaveTrait: NX_BOLD];
		[self setFonts: plainFont :boldFont];
	}
	
    	[self setAutoresizeSubviews:YES];
	return self;
}

- setFonts:plain:bold {
	int i;

	[self setFont:plain];
	boldfont = bold;
	for (i=0;i<numRows;i++){
		[[self cellAt:i:0] setFonts:font:boldfont];
	}
}

- clearCells { 
	id cells = [self cellList];
	int i;

	if (!numCells || !cells) return;
	for (i = 0; i < numCells; i++) {
		[[cells objectAt:i] setStatus:NOTVALID];
	}
	[self selectCellAt:-1:-1];
	numCells = 0;
}

- addCell: (char *)desc:(char *)file:(char *)name:lo:hi{
	static int i=0;
	static id cells;
	id c = (id)0;
	static int x,y;

	if (numCells==0){
		i=0;
		[self getNumRows:&y numCols:&x];
		cells = [self cellList];
	}
	if (i<y) c = [cells objectAt:i];
	if (!c){
		[self renewRows:i+1 cols:1];
		c = [PSCell newTextCell:desc];
		[c setFonts:font:boldfont];
		[cells addObject:c];
		[self putCell:c at:i:0];
		[c setEditable:NO];
		[c setBordered:NO];
		[c setTarget:self];
	}
	[c changeHighlight:NO];
	[c setContents:desc file:file :name:lo:hi];
	[c setStatus:VALID];

	[c setEnabled:(file?YES:NO)];
	[c setTag:i];
	i++;
	numCells++;
	return c;
}

- sizeToFit {
	NXSize s;
	NXRect r;
	id f;

	[self getFrame:&r];

	f = [font screenFont];
	if (!f) f = font;

	s.width = r.size.width;
	s.height = ceil([f pointSize] + 10);
	[self setCellSize:&s];
	[self sizeTo:s.width: (float)(numCells?numCells:1)*(s.height+intercell.height)];
}

- (int) currentNumVisible {
	NXRect visRect;
	id scroll = [superview superview];
	
	if ([scroll isKindOf:[ScrollView class]]) {
		[scroll getDocVisibleRect:&visRect];
	}
	else {
		visRect = bounds;
	}
	return (visRect.size.height/(cellSize.height + intercell.height) + 2);
}

- sizeTo:(NXCoord)x :(NXCoord)y {
	NXSize s;

	[window disableFlushWindow];
	[super sizeTo:x:y];
	[self getCellSize:&s];
	s.width = x;
	[self setCellSize:&s];
	[window reenableFlushWindow];
	[window flushWindow];
}

- (int) numCells { return numCells; }

- selection {
	return [self selectedCell];
}

- (int) currentFileType {
	id c = [self selectedCell]; 
	if (!c) return NO_FILE;
	return [c currentFileType];
}

- hiliteFirst {
	int n;

	for (n = 0; n < numCells; n++) {
		if ([[self cellAt:n:0] isEnabled]) {
			[self hilite:n];
			return;
		}
	}
}

- hilite:(int)n {
	[self selectCellAt:n:0];
	[self lockFocus];
	[self highlightCellAt:n:0 lit:YES];
	[self unlockFocus];
	[self scrollCellToVisible:n:0];
	[self click:self];
	return self;
}

- click:sender {
	char	*file;
	id	c = [self selectedCell];
	if( c  && (file = [c file]) )
		[NXApp setFile: file];
	return self;
}


-(int)_inc:(int)inc {
	int i = [self selectedRow] + inc;
	id c;

	while (i>=0 && i<numCells && (c=[self cellAt:i:0]) && ![c isEnabled])
		i += inc;
	if (i < 0 || i >= numCells)
		return -1;
	return i;
}
	
- incr:(int)dir {
	int n = [self selectedRow], i = [self _inc:dir];

	if (i<0) return (id)0;
	[self hilite:i];
	return self;
}

- prev {
	if ([self selectedRow] <= 0){
		/*	clunk();	*/
		return;
	}
	[self incr:-1];
}

- next {
	if ([self selectedRow] >= (numCells-1)){
		/*	clunk();	*/
		return;
	}
	[self incr:1];
}

@end

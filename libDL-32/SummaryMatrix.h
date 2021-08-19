#import <appkit/appkit.h>
#import <appkit/Matrix.h>

@interface  SummaryMatrix :Matrix
{
	int numCells;
}

- setFonts:plain:bold ;
- clearCells ;
- addCell: (char *)desc :(char *)file :(char *)name:lo:hi;
- sizeToFit;
- sizeTo:(NXCoord)x :(NXCoord)y ;
- (int) numCells ;
- selection ;
- select:(int) n ;
- hilite:(int)n ;
- hiliteFirst;
- click:sender ;
- (int)_inc:(int)inc ;
- incr:(int)dir ;
- prev ;
- next ;
- (int) currentFileType;
- (int) currentNumVisible;

@end

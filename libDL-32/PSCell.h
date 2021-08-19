#import <appkit/ActionCell.h>

#define BOLD	1
#define PLAIN	2
#define TAB	3
#define CENTER	4
#define MARK(moveCh) (moveCh==CENTER?1:0)

#define OFF	0
#define LO	1
#define HI	2

#define VALID	 0
#define NOTVALID 1

#define BACKLO_COLOR	NX_LTGRAY

@interface  PSCell :ActionCell
{
	char *file;
	float fontHeight, mwidth;
	int status;
	id plainFont, boldFont;
	id icon[3];
	char *iconName;
	int viewType;
}

- setContents: (char const *)s ;
- setStatus: (BOOL)status ;
- setStringValue:(char const*)s;
- setSmallIcons:(char *)name:lo:hi ;
- setContents: (char const *)s file:(char *)f :(char *)name:lo:hi;
- setFonts:plain:bold ;
- (char *) file ;
- (char *) iconName ;
- (int)changeHighlight:(BOOL)flag;
- (int) currentFileType;
- setFileType: (int) itsType;

@end

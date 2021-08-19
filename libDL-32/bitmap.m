#import <appkit/appkit.h>
#import <string.h>
#import <appkit/Bitmap.h>
#import "bitmap.h"
#import <sys/file.h>
#import "confirm.h"

#define IMAGES	"images"
#define OBJ(x,y)  NXGetNamedObject(x,y)

static char *bitmapFolder = NULL;


void
putItemInView(id v, id i)
/*
 * Add item 'i' to view 'v' (in v's coordinates)
 */
{
	NXRect r;
	if (!i) return;
	[i getFrame:&r];
	[v convertRectFromSuperview:&r];
	[i setFrame:&r];
	[v addSubview:i];
}

void
setIcons(id b,char *lo,char *hi)
	/* id b;	 	the button */
	/* char *lo, *hi;	bitmaps for default and highlighted states */
/*
 * set the icons for 'button' to 'lo' and 'hi', respectively
 */
{
	if (!b) return;
	(void) bitmap(lo);
	(void) bitmap(hi);
	[b setIcon:lo];
	[b setAltIcon:hi];
	[b setBordered:NO];
	[[b cell] setParameter:NX_CHANGECONTENTS to:0];
	[[b cell] setParameter:NX_LIGHTBYCONTENTS to:1];
	[[b cell] setParameter:NX_LIGHTBYGRAY to:0];
	[b sizeToFit];
}

id
makeButton(id parent, char *name, char *icon, id view, int commandKey)
/*
 * Make a button for the given name with an image called 'icon'
 * (it defaults to 'name' if 0) and put it in 'view'.
 */
{
	char lo[64],hi[64];
	id b = OBJ(name,parent);
	if (!b) return b;

	strcpy(lo,icon? icon : name);
	sprintf(hi,"%sH",icon? icon : name);
	setIcons(b,lo,hi);
	if (view) putItemInView(view,b);
	if (commandKey) [b setKeyEquivalent:(unsigned short)commandKey];
	return b;
}

void
setBitmapFolder(char *folder, char *tail)
{
	int size;

	if (bitmapFolder)
		free(bitmapFolder);

	if (!tail) tail = IMAGES;

	size = strlen(folder) + strlen(tail) + 2;	/* add one for slash */
	bitmapFolder = malloc(size);

	sprintf(bitmapFolder,"%s/%s", folder, tail);

	if (access(bitmapFolder, R_OK))  {
		error("Ancillary material does not exist (%s).", bitmapFolder);
	}
}

id
bitmap(char *s)
{
	char t[1024];
	const char *name;
	id bm;
	
	if (bm = [Bitmap findBitmapFor:s]) {
		return bm;
	}
		
	if (!bitmapFolder)	
		setBitmapFolder(".", NULL);

	sprintf(t,"%s/%s.tiff", bitmapFolder, s);
	if ([Bitmap addName:s fromTIFF:t])
	    return [Bitmap findBitmapFor:s];
	return (id)0;
}

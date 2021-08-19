
/*
	appWindows.m
	Copyright 1990, NeXT, Inc.
	Responsibility: Greg Cockroft
  	
	Category of the Application object to deal with the Windows Menu.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "Menu_Private.h"
#import "MenuCell.h"
#import "Matrix.h"
#import <objc/List.h>
#import <objc/Storage.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXAppCategoryWindows=0\n");
    asm(".globl .NXAppCategoryWindows\n");
#endif


typedef struct {
    id		window;   	/* Window object Controlled by menu item */
    char	*filename;	/* In case of a filename window title contains full pathname */
    id		cell;		/* cell in the Windows menu matrix */
    }  winentry;
   
static void  getwin(id win, winentry **winentryp,unsigned int *indexp);
static void addW(id win,const char *aString,BOOL isFilename);
static void removeW(id win);
static void uniquefilenames(BOOL reset);
static char *lastpart(char *cp);
static char *backup(char *cp,char *cpstart);
static void maketitle(char *name,char *title, int wholepath);
static	void deconvertname(char *dst, const char *aString);

static	Menu		*windowsMenu = nil;
static	Storage		*winstore = nil;   /* winentrys behind menu items */
			

@implementation Application (WindowsMenu)

- windowsMenu
{
    return windowsMenu;
}

/*
 * Make this menu be the Windows menu.
 */
- setWindowsMenu:(id)menu
{
    if([menu isKindOf:[Menu class]] && menu != windowsMenu)
	{
	windowsMenu = menu;
	winstore = [[Storage allocFromZone:[self zone]] initCount:0 elementSize:sizeof(winentry) description:"@*@"];
	}
	
	
    return self;
}

/*
 * Take all the windows and arrange in a nice order.
 * Do this on a per screen basis, with the edited documents on top.
 */
- arrangeInFront:sender
{
const NXScreen  *screenlist;
int	numScreens,screen,wins,winnumber,i;
Window	*topwin,*topeditedwin,*lastwin;
NXCoord	x,y,dx,dy,totalwidth,totalheight,marginx,marginy;
NXRect	winrect;
const NXRect	*scrRect;
winentry	*winep;

    [self getScreens:&screenlist count:&numScreens];
    wins = [winstore count];
    for(screen = 0; screen < numScreens; screen++,screenlist++)
	{
	  /* find window on top, and count num of wins on this screen */
	topeditedwin = NULL;
	topwin = NULL;
	winnumber = 0;
	for(i = 0; i < wins; i++)
	    {
	    winep = (winentry *)[winstore elementAt:(unsigned int)i];
	    if([winep->window screen] == screenlist)
		{
		winnumber++;
		if(!topwin)
		    topwin = winep->window;
		if([winep->window isDocEdited])
		    {
		    if(!topeditedwin)
			topeditedwin = winep->window;
		    }
		}
	    }
	if(topeditedwin)
	    topwin = topeditedwin;
	if(!topwin)  
	    break;
	    
	    /* Find the location for the topmost window, by centering
	       whole group of windows */
	[topwin getFrame:&winrect];
	scrRect = &(screenlist->screenBounds);
#define DELTAX 22
#define DELTAY 24
	dx = (NX_WIDTH(scrRect) - NX_WIDTH(&winrect)) / (winnumber - 1);
	if(dx > DELTAX)
	    dx = DELTAX;
	dy = (NX_HEIGHT(scrRect) - NX_HEIGHT(&winrect)) / (winnumber - 1);
	if(dy > DELTAY)
	    dy = DELTAY;
	totalwidth = (winnumber - 1) * dx + NX_WIDTH(&winrect);
	totalheight = (winnumber - 1) * dy + NX_HEIGHT(&winrect);
	marginx = (NX_WIDTH(scrRect) - totalwidth) / 2;
	marginy = (NX_HEIGHT(scrRect) - totalheight) / 2;
	x = NX_WIDTH(scrRect) - (NX_WIDTH(&winrect) + marginx);
	y = marginy + NX_HEIGHT(&winrect);
	
		/* Set position of topwin and bring to front, then move the rest */
	[topwin moveTopLeftTo:x : y];
	[topwin makeKeyAndOrderFront:topwin];
	lastwin = topwin;
	
		/* First all edited windows */
	for(i = 0; i < wins; i++)
	    {
	    winep = (winentry *)[winstore elementAt:(unsigned int)i];
	    if([winep->window screen] == screenlist && [winep->window isDocEdited])
		{
		if(winep->window == topwin)
		    continue;
		x -= dx;
		y += dy;
		[winep->window orderWindow:NX_BELOW relativeTo:[lastwin windowNum]];
		[winep->window moveTopLeftTo:x : y];
		lastwin = winep->window;
		}
	    }
		/* Now all non-edited windows */
	for(i = 0; i < wins; i++)
	    {
	    winep = (winentry *)[winstore elementAt:(unsigned int)i];
	    if([winep->window screen] == screenlist && ![winep->window isDocEdited])
		{
		if(winep->window == topwin)
		    continue;
		x -= dx;
		y += dy;
		[winep->window orderWindow:NX_BELOW relativeTo:[lastwin windowNum]];
		[winep->window moveTopLeftTo:x : y];
		lastwin = winep->window;
		}
	    }
	}
    return self;
}

- _sizeWindowsMenu:theMenu
{
    [theMenu sizeToFit];
    return self;
}

/* 
 * The window title has changed on one of the windows in the windows menu.
 */
- changeWindowsItem:(id)win title:(const char *)aString filename:(BOOL)isFilename
{
    if(windowsMenu)
	{
	[windowsMenu disableDisplay];
	removeW(win);
	if (aString && *aString)
	    {
	    addW(win,aString,isFilename);
	    }
	uniquefilenames(YES);
	[self perform:@selector(_sizeWindowsMenu:) with:windowsMenu afterDelay:1 cancelPrevious:YES];
	[windowsMenu reenableDisplay];
	}
    return(self);
}

- updateWindowsItem:(id)win
{
    winentry *entry;
    unsigned int index;

    getwin(win, &entry, &index);
    if (entry && entry->cell && [entry->cell _isEditing] != [win isDocEdited]) {
	[entry->cell _setEditing:[win isDocEdited]];
	[[entry->cell controlView] drawCellInside:entry->cell];
    }

    return self;
}

/* 
 * Add a window to the windows menu.
 */
- addWindowsItem:(id)win title:(const char *)aString filename:(BOOL)isFilename
{
winentry	*entry;
unsigned int	index;
    if(windowsMenu && aString && *aString)
	{
	[windowsMenu disableDisplay];
	getwin(win, &entry, &index);  /* Check if already in menu */
	if(!entry)
	    {
	    addW(win,aString,isFilename);
	    [self perform:@selector(_sizeWindowsMenu:) with:windowsMenu afterDelay:1 cancelPrevious:YES];
	    }
	[windowsMenu reenableDisplay];
	}
    return(self);
}

- _removeFromWindowsMenu:win
{
    id matrix;
    winentry *entry;
    unsigned int index;
    int row, col;

    if (windowsMenu) {
	getwin(win, &entry, &index);
	if (entry) {
	    [windowsMenu disableDisplay];
	    matrix = [windowsMenu itemList];
	    [matrix getRow:&row andCol:&col ofCell:entry->cell];
	    [matrix removeRowAt:row andFree:YES];
	    if (entry->filename) free(entry->filename);
	    [winstore removeAt:index];
	    uniquefilenames(YES);
	    [windowsMenu reenableDisplay];
	    [windowsMenu sizeToFit];
	}
    }

    return self;
}

/* 
 * Remove this window from the Windows menu.
 */
- removeWindowsItem:(id)win 
{
    [self perform:@selector(_removeFromWindowsMenu:) with:win afterDelay:1 cancelPrevious:NO];
    return self;
}

static void removeW(win)
id	win;
{
id		matrix;
winentry	*entry;
int		row,col;
unsigned int	index;

    getwin(win, &entry, &index);
    if(entry)
	{
	matrix = [windowsMenu itemList];
	[matrix getRow:&row andCol:&col ofCell:entry->cell];
	[matrix removeRowAt:row andFree:YES];
	if(entry->filename)
	    free(entry->filename);
	[winstore removeAt:index];
	}
}

/*
 * Create a filename from the long dashed name window converts a filname to.
 * Depends on format set in Window setTitleAsFilename:
 */
static	void deconvertname(char *dst, const char *aString)
{
char *longdash;
char temp[MAXPATHLEN+5];

    strcpy(temp, aString);
    longdash = rindex(temp,'\320');
    if(longdash) {
	strcpy(dst,longdash+3);
	strcat(dst,"/");
	*(longdash-2) = 0;
	strcat(dst,temp);
    }
    else {
	strcpy(dst,temp);
    }

}

static void addW(id win,const char *aString,BOOL isFilename)
{
winentry  	wine,*winep;
char		*name;
unsigned int	i,last;
int		rowindex,rows,cols;
id		matrix;

    wine.window = win;
    wine.filename = 0;
    if(isFilename)
	{
	wine.filename = NXZoneMalloc([NXApp zone], strlen(aString)+1);
	deconvertname(wine.filename, aString);
	name = lastpart(wine.filename);
	}
    else
	name = (char *)aString;

	/* Insert into menu and into winstore storage */
    last = [winstore count];
    for(i = 0; i < last; i++)  /* sorted insertion */
	{
	winep = (winentry *)[winstore elementAt:i];
	if(strcmp(name, [winep->cell title]) < 0)
	    break;
	}
    matrix = [windowsMenu itemList];
    [matrix getNumRows:&rows numCols:&cols];
    while (rows && ([[matrix cellAt:rows-1 :0] action] == @selector(performClose:) || [[matrix cellAt:rows-1 :0] action] == @selector(performMiniaturize:))) rows--;
    rowindex = rows - last + i;       /* Convert from index in winstore to mat index */
    [matrix insertRowAt:rowindex];
    wine.cell = [matrix cellAt:rowindex :0];
    [wine.cell setTitle:(const char *)name];
    [wine.cell _setEditing:[win isDocEdited]];
    [matrix setTag:i target:win action:@selector(makeKeyAndOrderFront:) at:rowindex:0]; 
    [winstore insert:(void *)&wine at:i];
    uniquefilenames(NO);
      
}


/*
 * Make sure all the entries corresponding to filenames are long enough to be unique
 * Its N^2, but only for menu items with filenames. Inner loop  is normally 2 string operations.
 * 
 */
static void uniquefilenames(BOOL reset)
{
int	i,j;
winentry	*win1,*win2;
char 		title1[MAXPATHLEN+5];
char 		title2[MAXPATHLEN+5];
char		*name1,*name2,*lastname1,*lastname2;

    if (reset) {
	[windowsMenu disableDisplay];
	i = [winstore count];
	for (i--; i >= 0; i--)
	    {
	    win1 = (winentry *)[winstore elementAt:(unsigned int)i];
	    if (win1->filename) [win1->cell setTitle:lastpart(win1->filename)];
	}
	[windowsMenu reenableDisplay];
    }
    i = [winstore count];
    for(i--; i >= 0; i--)
	{
	win1 = (winentry *)[winstore elementAt:(unsigned int)i];
	if(!win1->filename)
	    continue;
	j = [winstore count];
	for(j--; j >= 0; j--)
	    {
	    if(j == i)
	    	continue;
	    win2 = (winentry *)[winstore elementAt:(unsigned int)j];
	    if(!win2->filename)
		continue;
	    name1 = lastpart(win1->filename);
	    name2 = lastpart(win2->filename);
	    if(strcmp(name1,name2))
		continue;
		/* Path have the same filename, make a unique title */
	    lastname1 = name1; lastname2 = name2;
	    do {
		name1 = backup(name1,win1->filename);
		name2 = backup(name2,win2->filename);
		if(name1 == lastname1 && name2 == lastname2)
		    break;    /* names are the same */
		lastname1 = name1; lastname2 = name2;
		} while(!strcmp(name1,name2));
	    maketitle(name1,title1,name1 == win1->filename);
	    if(strlen(title1) > strlen([win1->cell title]))
		[win1->cell setTitle:title1];
	    maketitle(name2,title2,name2 == win2->filename);
	    if(strlen(title2) > strlen([win2->cell title]))
		[win2->cell setTitle:title2];
	    }
	}
}

/*
 * get filename on a path
 */
static char *lastpart(cp)
char	*cp;
{
char	*slash;
    slash = rindex(cp,'/');
    if(slash) {
	if(strlen(slash+1) != 0)  /* This is for directory names */
	    return(slash+1);
	else
	    return(cp);
	}
    else
	return(cp);
}

/*
 * backup a slash on a path
 */
static char *backup(cp,cpstart)
char	*cp,*cpstart;
{
    cp--;
    while(cp >= cpstart)
	{
	if(*cp == '/')
	    return(cp);
	cp--;
	}
    return(cpstart);
}

/*
 * Given a filename, make a good menu item title.
 */
static void maketitle(name,title,wholepath)
char	*name,*title;
int	wholepath;  /* True is this is complete path, not last part of a path */
{
char *slash;

    slash = rindex(name, '/');
    if (!slash) 
	{
	strcpy(title, name);
	} 
    else 
	{
	strcpy(title, slash+1);
	strcat(title, "  \320  ");	/* \320 is a "long dash" */
	if(!wholepath)
	    strcat(title, "...");		
	if (slash == name)
	    strcat(title, "/");		/* for files in / */
	else
	    strncat(title, name, slash - name);
	}    
}

/*
 * Locate entry with this win id.
 */
static void  getwin(win, winentryp, indexp)
id	win;
winentry	**winentryp;
unsigned int	*indexp;
{
int	i;
winentry	*winep;

    *winentryp = NULL;
    *indexp = 0;
    
    i = [winstore count];
    for(i--; i >= 0; i--)
	{
	winep = (winentry *)[winstore elementAt:(unsigned int)i];
	if(winep->window == win)
	    {
	    *winentryp = winep;
	    *indexp = i;
	    break;
	    }
	}
}
@end

/*
87
--
7/12/90	glc	Correct handling of name -- path window titles. Method name changes.

91
--
 8/11/90 glc	bug fix for directory names.

*/

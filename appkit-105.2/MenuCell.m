/*
	MenuCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "Cell_Private.h"
#import "Matrix.h"
#import "Menu.h"
#import "MenuCell.h"
#import "NXImage.h"
#import "View.h"
#import "Text.h"
#import "nextstd.h"
#import "graphics.h"
#import <defaults.h>
#import <dpsclient/wraps.h>
#import <objc/HashTable.h>

@implementation MenuCell:ButtonCell

static id closeIcon = nil;
static id editingIcon = nil;

static id keHashTable = nil;
static id usedKeys[256];
static BOOL customKeyEquivalents = YES;
static BOOL useUserKeyEquivalents = YES;

#define skipspace(ck) while (*ck && (*ck == ' ' || *ck == '\n' || *ck == '	')) ck++;

static void loadKEHashTable(const char *defaultDomain)
{
    int i, ke;
    const unsigned char *ck;
    char ckbuf[1024];

    if (ck = (unsigned char *)NXGetDefaultValue(defaultDomain, "NXCommandKeys")) {
	while (*ck) {
	    skipspace(ck);
	    if (*ck == ',') break;
	    i = 0;
	    while (*ck && *ck != ',') {
		if (*ck == '\\') ck++;
		ckbuf[i++] = *ck++;
	    }
	    if (!*ck++) break;
	    ckbuf[i] = '\0';
	    skipspace(ck);
	    ke = (*ck == ',' && *(ck+1) != 0 && *(ck+1) != ',') ? 0 : *ck++;
	    skipspace(ck);
	    if ((*ck == ',' || !*ck) && ![keHashTable valueForKey:ckbuf]) {
		if (!keHashTable) keHashTable = [[HashTable allocFromZone:[Menu menuZone]] initKeyDesc:"*" valueDesc:"i"];
		[keHashTable insertKey:NXUniqueString(ckbuf) value:(void *)ke];
	    }
	    if (*ck++ != ',') break;
	}
    }
}

static id findKeyEquivalent(id menu, unsigned short ke)
{
    id cell, matrix;
    int i, rows, cols;

    matrix = [menu itemList];
    if (matrix) {
	[matrix getNumRows:&rows numCols:&cols];
	for (i = 0; i < rows; i++) {
	    cell = [matrix cellAt:i :0];
	    if ([cell hasSubmenu]) {
		cell = findKeyEquivalent([cell target], ke);
		if (cell) return cell;
	    } else if ([cell keyEquivalent] == ke) {
		return cell;
	    }
	}
    }

    return nil;
}

+ useUserKeyEquivalents:(BOOL)flag
{
    useUserKeyEquivalents = flag ? YES : NO;
    return self;
}

static unsigned short getKeyEquivalent(id menucell, const char *title, unsigned short oldke)
{
    int ke;
    id cell;
    BOOL userChosen;

    if (!useUserKeyEquivalents || !title) return oldke;

    if (!keHashTable && customKeyEquivalents) {
	loadKEHashTable(NXSystemDomainName);
	loadKEHashTable([NXApp appName]);
	bzero(usedKeys, 256 * sizeof(id));
	customKeyEquivalents = keHashTable ? YES : NO;
    }
    if (!customKeyEquivalents) return oldke;
    userChosen = [keHashTable isKey:title];
    ke = userChosen ? (int)[keHashTable valueForKey:title] : 0;
    if (!ke) ke = oldke;
    if (ke < 256) {
	if ((cell = usedKeys[ke]) && (cell != menucell)) {
	    if (userChosen) {
		[cell setKeyEquivalent:0];
		[[cell controlView] drawCell:cell];
		usedKeys[ke] = menucell;
	    } else {
		ke = 0;
	    }
	} else {
	    usedKeys[ke] = menucell;
	}
    } else {
	ke = 0;
    }

    return (unsigned short)ke;
}

static void initClassVars()
{
    closeIcon = [NXImage findImageNamed:"NXclose"];
    editingIcon = [NXImage findImageNamed:"NXediting"];
}

+ newTextCell:(const char *)aString
{
    return [[self allocFromZone:[Menu menuZone]] initTextCell:aString];
}

- initTextCell:(const char *)aString
{
    [super initTextCell:aString];
    [self setIconPosition:NX_ICONRIGHT];
    [self setAlignment:NX_LEFTALIGNED];
    [self setHighlightsBy:NX_CHANGEGRAY];
    [self setShowsStateBy:NX_NONE];
    [self setWrap:NO];
    if (!editingIcon) initClassVars();
    return self;
}

+ newTextCell
{
    return [[self newTextCell:NULL] setTitleNoCopy:"MenuItem"];
}

+ new
{
    return [self newTextCell];
}

- awake
{
    if (!editingIcon) initClassVars();
    return [super awake];
}

- init
{
    return [[self initTextCell:NULL] setTitleNoCopy:"MenuItem"];
}

- setUpdateAction:(SEL)aSelector forMenu:aMenu
{
    _NXCheckSelector(aSelector, "[MenuCell -setUpdateAction:forMenu:]");
    if (updateAction = aSelector)
	[aMenu setAutoupdate:YES];
    return self;
}

- (SEL)updateAction
{
    return updateAction;
}

- (unsigned short)userKeyEquivalent
{
    return getKeyEquivalent(self, contents, bcFlags2.keyEquivalent);
}

- setKeyEquivalent:(unsigned short)keyEquivalent
{
    if (_cFlags3.useUserKeyEquivalent) {
	return [super setKeyEquivalent:keyEquivalent ? getKeyEquivalent(self, contents, keyEquivalent) : 0];
    } else {
	return [super setKeyEquivalent:keyEquivalent];
    }
}

- (BOOL)trackMouse:(NXEvent *)theEvent
    inRect:(const NXRect *)cellFrame
    ofView:controlView
{
    [[controlView window] mouseDown:theEvent];
    return YES;
}

- (BOOL)hasSubmenu
{
    return (target && (action == @selector(submenuAction:))) ? YES : NO;
}

- setTitleNoCopy:(const char *)title
{
    id retval = [super setTitleNoCopy:title];
    if (![self hasSubmenu] && title != "MenuItem" && _cFlags3.useUserKeyEquivalent) {
	[super setKeyEquivalent:[self userKeyEquivalent]];
    }
    return retval;
}

- setTitle:(const char *)title
{
    id retval = [super setTitle:title];
    if (![self hasSubmenu] && title != "MenuItem" && _cFlags3.useUserKeyEquivalent) {
	[super setKeyEquivalent:[self userKeyEquivalent]];
    }
    return retval;
}

- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect
{
    NXSize iconSize;

    [super calcCellSize:theSize inRect:aRect];
    if (!_cFlags3.docEditing && !_cFlags3.docSaved) return self;
    [editingIcon getSize:&iconSize];
    theSize->width += iconSize.width+1.0;
    theSize->height = MAX(iconSize.height, theSize->height);

    return self;
}

- drawInside:(const NXRect *)cellFrame inView:controlView
{
    NXRect r;
    NXPoint p;
    NXSize iconSize;

    if (_cFlags3.docEditing || _cFlags3.docSaved) {
	if (!cellFrame) return self;
	r = *cellFrame;
	[editingIcon getSize:&iconSize];
	r.origin.x += iconSize.width;
	r.size.width -= iconSize.width;
	[super drawInside:&r inView:controlView];
	p.x = cellFrame->origin.x + 5.0;
	r.origin.x = 3.0; r.origin.y = 4.0;
	r.size.width = iconSize.width - 7.0;
	r.size.height = iconSize.height - 7.0;
	p.y = cellFrame->origin.y + cellFrame->size.height - floor((cellFrame->size.height - r.size.height + 1.0) / 2.0);
	[(_cFlags3.docEditing ? editingIcon : closeIcon) composite:NX_COPY fromRect:&r toPoint:&p];
    } else {
	[super drawInside:cellFrame inView:controlView];
    }

    return self;
}

- _setEditing:(BOOL)flag
{
    _cFlags3.docEditing = flag ? YES : NO;
    _cFlags3.docSaved = flag ? NO : YES;
    return self;
}

- (BOOL)_isEditing
{
    return _cFlags3.docEditing;
}

- write:(NXTypedStream *) stream
{
    [super write:stream];
    if (action == @selector(submenuAction:))
	NXWriteObject(stream, target);
    else
	NXWriteObjectReference(stream, target);
    NXWriteTypes(stream, ":", &updateAction);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    target = NXReadObject(stream);
    NXReadTypes(stream, ":", &updateAction);
    return self;
}

- free
{
    id *cur, *last;
    last = usedKeys + 256;
    for (cur = usedKeys; cur < last; cur++) {
	if (*cur == self)
	    *cur = 0;
    }
    return [super free];
}

@end

/*
  
Modifications (since 0.8):
  
 1/14/89 pah	MenuCells shouldn't word wrap
 1/27/89 bs	added read: write:

79
--
  4/3/90 pah	added isEnabled in support of automatic updating of menus
		 when running modal.  this is a bit of a hack since really
		 the kit should automatically update menus whether you're
		 running modal or not, but it is too confusing to the user
		 if, while modal, he can choose menu options like Open...
		 or Hide or Quit, so we gray them out.
		 see _updateModalMenus in Application and _updateModal in Menu

80
--
  4/10/90 pah	added support for user-settable command keys and enforcement
		 of standard UI keys (getKeyEquivalentForMenuItem:existingKey:)

83
--
  5/3/90 pah	took out enforcement of UI guidelines about command keys
		took out support for auto-dimming of menu items when modal
		fixed bug where PopUpLists were being assigned user command
		keys
86
--
 6/12/90 pah	Nuked getKeyEquivalentForMenuItem:existingKey: and replaced it
		 with instance method userKeyEquivalent
		Fixed many bugs in getKeyEquivalent()

100
---
 10/24/90 aozer	Allow having ',' as a key equivalent.

*/

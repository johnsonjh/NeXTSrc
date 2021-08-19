#import <ctype.h>
#import <string.h>
#import <text/text.h>
#import <appkit/appkit.h>
#import "Find.h"
#import "NroffText.h"

#define MAXSTR 256

@implementation Find

/* there is only one of these, so keep it around */
static id findPanelObj = NULL;

- enterFirstWord:(char const *)str {
	char *first, *p;

	if (!str || !*str) return self;

	first = p = strsave(str);

	while (*str == ' ')
		++str;
	for (; (*p = *str) && (*str == '_' || !isspace(*str)); p++,str++)
		;
	*p = '\0';

	[searchStr setStringValue:first at:0];
	free(first);
	return self;
}

- enterSelection:(id)sender {
	char sel[MAXSTR];
	id currText = [NXApp calcTargetForAction:@selector(getSelString:maxLen:)];
	[searchStr setStringValue:[currText getSelString:sel maxLen:MAXSTR] at:0];
	return self;
}

- search:(id)sender {
	[[NXApp calcTargetForAction:@selector(search:)] search:sender];
	return self;
}

- search:(id)sender in:(id)textObj{
	int fwd;
	const char *str = [self searchStr];

	if (str && str[0]) {
		if (!sender) {
			fwd = TRUE;
		}
		else if ([sender isKindOf:[Matrix class]]){
			fwd = [[sender selectedCell] tag];
		}
		else {
			fwd = [sender tag];
		}
		[textObj findText:str forward:fwd];
	}
	else {
		[self showPanel:self];
	}
	return self;
}

- (const char *)searchStr {
	return [searchStr stringValueAt:0];
}

- showPanel:sender {
	[panel makeKeyAndOrderFront:self];
	[searchStr selectText:self];
	return self;
}

id
findPanel() { 
	if (!findPanelObj)  {
		findPanelObj = [Find new];
	}
	return findPanelObj;
}

id 
buildFindMenu()
{
    id menuCell;
    id theMenu = [Menu newTitle:"Find"];
    id self = findPanel();

    menuCell = [theMenu addItem:"Find Panel..." action:@selector (showPanel:)
		keyEquivalent :'f'];
    [menuCell setTarget:self];

    menuCell = [theMenu addItem:"Find Next" action:@selector (search:)
		keyEquivalent :'g'];
    [menuCell setTag:FORWARD];	/* set tag for search Fwd */
    [menuCell setTarget:self];

    menuCell = [theMenu addItem:"Find Previous" action:@selector (search:)
		keyEquivalent :'d'];
    [menuCell setTag:BACKWARD];
    [menuCell setTarget:self];

    menuCell = [theMenu addItem:"Enter Selection" action:@selector (enterSelection:)
		keyEquivalent :'e'];
    [menuCell setTag:0];
    [menuCell setTarget:self];

    [theMenu display];
    return theMenu;
}

/* called by loadNibSection */
- setSearchStr:(id)anObject { searchStr = anObject;}
- setPanel:(id)anObject { panel = anObject;}

+ new {
	self = [super new];

	[NXApp loadNibSection:"Find" owner:self];
	return self;
}

@end

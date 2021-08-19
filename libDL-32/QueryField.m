#import <appkit/appkit.h>
#import "QueryField.h"
#import "Find.h"
#import "query.h"
#import <text/text.h>
#import <string.h>

#define QUERY	(0)
#define TEXT	(1)

@implementation QueryField

/* 
 * this sets up the intricate data flow for the query field.
 * 	clickBtn: button that's sent performClick on cmd-dbl click.
 *	textView: the view that 
 *
 *	action:	the action that occurs at <CR> in the QueryField
 *	target: the target that receives the action.
 */
- initPerformClickBtn:cBtn textView:tView queryEndAction:(SEL) action queryEndTarget:target {

	clickBtn = cBtn;
	textView = tView;

	if (!action) action = @selector(performClick:);
	if (!target) target = cBtn;

	[self setTarget:target];
	[self setAction:action];
	return self;
}


- (char *)getSelString:(char *)s maxLen:(int)len
/* Copy the selected string into 's'. */
{
#ifdef DOWITHSELECTION
	int             n = spN.cp - sp0.cp;

	if (spN.cp < 0 || sp0.cp < 0)  {
		n = 0;
	}

	if (n >= len)
		n = len - 1;
	if (n != 0)
		[self getSubstring:s start:sp0.cp length:n];
	s[n] = '\0';
	return s;
#else
	const char *sel = [self stringValue];
	int selLen = strlen(sel);

	strncpy(s, sel, len);
	if (selLen >= len) {
		s[len-1] = '\0';
	}
	[self selectText:self];
	return s;
#endif
}

#define MAXSTR  128
- (char *)getCurrSelection {
	char *s, str[MAXSTR];
	char *p, *realStr;

	realStr = [currSel==TEXT?textView:self getSelString:str maxLen:MAXSTR];

	/* get rid of the underlines
	 * and don't read past carriage returns
	 */
	for(p = s = realStr; *p ; p++, s++) {
		if (*p == '\n') {
			break;
		}
		while (*p == '\b') {
			p +=1;
		}
		while (*p == '_' && *(p+1) == '\b') {
			p +=2;
		}
		*s = *p;
	}
	*s = *p;
	
	return strsave(realStr);
}

- selectText:sender {
	currSel = QUERY;
	return [super selectText:sender];
}

/* show what we really want the query to be */
- setQuery:(char const *)s {
	if (!s || !*s)
		return;

	[self setStringValue:s];
	[self display];
	[self selectText:self];
	[findPanel() enterFirstWord:s];

	return self;
}


/* look up the next/prev valid query */
- nextQ:sender {
	return [self setQuery:nextQuery()];
}

- prevQ:sender {
	return [self setQuery:prevQuery()];
}

/*
* User has COMMAND-selected a word in the Contents area.
* Toss the selection into the query window and look it up.
*/
- click:sender:(char *)t:(NXEvent *)e
{
	if (*t) {
		currSel = TEXT;
		if (e->flags & NX_COMMANDMASK) {
			[clickBtn performClick:self];
		}
	}
	return self;
}

- mouseDown:(NXEvent *) theEvent
{
	currSel = QUERY;
	return [super mouseDown:theEvent];
}

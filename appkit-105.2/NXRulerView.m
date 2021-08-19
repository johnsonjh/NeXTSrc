/*
	NXRulerView.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "NXRulerView.h"
#import "Application.h"
#import "NXImage.h"
#import "Window.h"
#import "Font.h"
#import "Text.h"
#import "ScrollView.h"
#import <dpsclient/wraps.h>
#import <objc/List.h>
#import <math.h>

#define HYSTERESIS 2.0
#define MINWIDTH 20.0
#define RULER_HEIGHT (33.0)
#define LINE_X (14.0)
#define WHOLE_HT (10.0)
#define HALF_HT (8.0)
#define QUARTER_HT (4.0)
#define EIGHTH_HT (2.0)
#define NUM_X (3.0)

#define WHOLE (72)
#define HALF (WHOLE/2)
#define QUARTER (WHOLE/4)
#define EIGHTH (WHOLE/8)

#define LM_INDEX 0
#define RM_INDEX 1
#define LI_INDEX 2
#define RI_INDEX 3
#define FI_INDEX 4
#define TB_INDEX 5
typedef enum {
    leftmargin = LM_INDEX, rightmargin = RM_INDEX,
    leftindent = LI_INDEX, rightindent = RI_INDEX, 
    firstindent = FI_INDEX, tab = TB_INDEX
} TokenType;

typedef struct {
    char *name;
    NXSize size;
    id bitmap;
    NXCoord hotX;
    BOOL above;
    TokenType type;
} TokenInfo;

static TokenInfo tokens[] = {
    { "NXrightmargin", 	{5.0,  17.0}, 0, 0.0, 1, leftmargin},
    { "NXleftmargin", 	{5.0,  17.0}, 0, 4.0, 1, rightmargin},
    { "NXfirstindent", 	{9.0, 11.0}, 0, 4.0, 1, firstindent},
    { "NXleftindent", 	{9.0,  6.0}, 0, 4.0, 1, leftindent},
    { "NXrightindent", 	{9.0,  6.0}, 0, 0.0, 1, rightindent},
    { "NXtab", 		{6.0, 11.0}, 0, 0.0, 1, tab},
    {0}
};

@interface NXRulerToken:Object
{
@public
    NXRect hotSpot;
    NXCoord curX, originalX, deltaX;
    TokenInfo *info;
}

- initToken:(TokenInfo *)newInfo pos:(NXCoord)x;
- paint:(const NXRect *)bounds;
- trackMouse:(const NXPoint *)start inView:view;
- moveTo:(NXCoord)x inView:view oldx:(NXCoord *)rp last:(BOOL)last;
- setX:(NXCoord)x;
@end

@interface ScrollView(Private)
- _rulerline:(NXCoord)old :(NXCoord)new last:(BOOL)last;
@end

@implementation NXRulerView

- initFrame:(NXRect *)aRect
{
    TokenInfo *type;
    NXZone *zone = [self zone];
    [super initFrame:aRect];
    tokenlist = [[List allocFromZone:zone] initCount:0];
    for (type = tokens; type->name; type++) {
	type->bitmap = [NXImage findImageNamed:type->name];
	switch (type->type) {
	    case leftmargin:
	    case rightmargin:
	    case leftindent:
	    case firstindent:
		[tokenlist addObject:
		    [[NXRulerToken allocFromZone:zone] initToken:type pos:0.0]];
		break;
	    case rightindent:
		break;
	    default:
		break;
	}
    }
    font = [Font newFont:KitString(Ruler, "Helvetica", "Font used in the ruler to draw the numbers.") size:8.0 matrix:NX_IDENTITYMATRIX];
    return self;
}

- (NXCoord) height
{
    return RULER_HEIGHT;
}


- setFont:newFont
{
    NXCoord as, lh;
    font = newFont;
    NXTextFontInfo(newFont, &as, &descender, &lh);
    if (descender < 0.0)
	descender = -1.0 * descender;
    return self;
}

- paintTokens:(const NXRect *)rect
{
    int i, count;
    id token;
    count = [tokenlist count];
    for (i = 0; i < count; i++) {
	token = [tokenlist objectAt:i];
	[token paint:rect];
    }
    return self;
}

- drawSelf:(const NXRect *) rects :(int)rectCount
{
    NXRect line, clip;
    int curPos, last, mod, i;
    PSsetgray(NX_LTGRAY);
    NXRectFill(rects);
    line = bounds;				/* draw bottom line */
    line.origin.y = 0.0;
    line.size.height = 1.0;
    PSsetgray(NX_DKGRAY);
    if (NXIntersectionRect(rects, &line)) {
	NXRectFill(&line);
    }
    line = bounds;				/* draw ruler line */
    line.origin.y = LINE_X;
    line.size.height = 1.0;
    line.origin.x = startX;
    line.size.width = endX - startX;
    PSsetgray(NX_BLACK);
    if (NXIntersectionRect(rects, &line)) {
	NXRectFill(&line);
    }
    clip = *rects;
    clip.origin.x = startX;
    clip.size.width = endX - startX;
    if (NXIntersectionRect(rects, &clip)) {	/* paint ticks ? */
	curPos = (int) (NX_X(&clip) - startX);
	last = (int) (NX_MAXX(&clip) - startX);
	if (mod = (curPos % EIGHTH))
	    curPos -= mod;
	if (mod = (last % EIGHTH))
	    last -= mod;
	line.size.width = 1.0;
	[font set];
	for (i = curPos; i <= last; i += EIGHTH) {
	    line.origin.x = startX + (float)i;
	    if (!(i % WHOLE)) {
		char buf[10];
		line.origin.y = LINE_X - WHOLE_HT;
		line.size.height = WHOLE_HT;
		NXRectFill(&line);
		PSmoveto(((float) i + NUM_X) + startX, 
		    descender + line.origin.y);
		sprintf(buf, "%d", i / WHOLE);
		PSshow(buf);
	    } else if (!(i % HALF)) {
		line.origin.y = LINE_X - HALF_HT;
		line.size.height = HALF_HT;
		NXRectFill(&line);
	    } else if (!(i % QUARTER)) {
		line.origin.y = LINE_X - QUARTER_HT;
		line.size.height = QUARTER_HT;
		NXRectFill(&line);
	    } else if (!(i % EIGHTH)) {
		line.origin.y = LINE_X - EIGHTH_HT;
		line.size.height = EIGHTH_HT;
		NXRectFill(&line);
	    }
	}
    }
    [self paintTokens:rects];
    return self;
}

- getToken:(NXPoint *)point
{
    NXRulerToken *token;
    int i, count;
    NXRect  r;
    count = [tokenlist count];
    for (i = count - 1; i >= 0; i--) {
	token = [tokenlist objectAt:i];
	r = token->hotSpot;
	NXInsetRect(&r,-2, -2);  /* Make it easier to hit rectangles */
	if (NXPointInRect(point, &r)) {
	    return token;
	}
    }
    return nil;
}


- mouseDown:(NXEvent *) event
{
    NXPoint mouseLocation;
    id token;
    int eventMask;
    NXZone *zone = [self zone];
    
    mouseLocation = event->location;
    [self convertPoint:&mouseLocation fromView:nil];
    token = [self getToken:&mouseLocation];
    eventMask = [window addToEventMask:NX_MOUSEDRAGGEDMASK];
    [self lockFocus];
    if (token) {
	[token trackMouse:&mouseLocation inView:self];
    } else if (mouseLocation.y < LINE_X) {
	token = [[NXRulerToken allocFromZone:zone]
	    initToken:tokens + TB_INDEX pos:0];
	[tokenlist addObject:token];
	[token moveTo:mouseLocation.x inView:self oldx:NULL last:NO];
	[token trackMouse:&mouseLocation inView:self];
    }
    [self unlockFocus];
    [window setEventMask:eventMask];
    return self;
}

- removeToken:token
{
    [tokenlist removeObject:token];
    return self;
}

- setTextObject:theText
{
    NXRect rect;
    NXCoord l, r, t, b;
    NXCoord newStartX, newEndX, newLMargin;
    [theText getBounds:&rect];
    [theText convertRect:&rect toView:self];
    newStartX = NX_X(&rect);
    newEndX = NX_MAXX(&rect);
    [theText getMarginLeft:&l right:&r top:&t bottom:&b];
    newLMargin = l;
    newStartX += newLMargin;
    newEndX -= r;
    if (newLMargin == leftMargin && newStartX == startX && newEndX == endX)
	return nil;
    leftMargin = newLMargin;
    startX = newStartX;
    endX = newEndX;
    return self;
}

static NXCoord convertLocal(id self, id text, NXCoord x)
{
    NXPoint point;
    NXRect rect;
    [text getBounds:&rect];
    point.x = x + rect.origin.x;
    point.y = 0.0;
    [text convertPoint:&point toView:self];
    return point.x;
}

- setRulerStyle:(NXTextStyle *)style for:theText
{
    int count, i, removeCount = 0;
    NXRulerToken *token;
    NXTabStop *cur, *last;
    NXCoord x;
    NXZone *zone = [self zone];
    
    if (style == lastStyle && theText == text) {
	if (![self setTextObject:theText])
	    return self;
    } else
	[self setTextObject:theText];	
    text = theText;
    lastStyle = style;
    leftIndent = style->indent2nd +  leftMargin;
    firstIndent = style->indent1st + leftMargin;
    count = [tokenlist count];
    cur = style->tabs;
    last = cur + style->numTabs;
    for (i = 0; i < count; i++) {
	token = [tokenlist objectAt:i];
	switch (token->info->type) {
	    case leftindent:
		x = leftIndent;
		break;
	    case firstindent:
		x = firstIndent;
		break;
	    case leftmargin:
		[token setX:startX];
		continue;
		break;
	    case rightmargin:
		[token setX:endX];
		continue;
		break;
	    case tab:
		if (cur && cur < last) {
		    x = cur->x + leftMargin;
		    token->originalX = cur->x;
		    cur++;
		} else {
		    x = -1.0;
		    removeCount++;
		}
		break;
	    default: x = -1.0; break;
	}
	if (x == -1.0)
	    [token setX:x];
	else 
	    [token setX:convertLocal(self, text, x)];
    }
    if (removeCount) {
	for (i = 0; i < removeCount; i++) {
	    [[tokenlist removeLastObject] free];
	}
    } else {
	for (; cur < last; cur++) {
	    token = [[NXRulerToken allocFromZone:zone]
		initToken:tokens + TB_INDEX
		pos:convertLocal(self, text, cur->x + leftMargin)];
	    token->originalX = cur->x;
	    [tokenlist addObject:token];
	}
    }
    [self display];
    return self;
}

- (NXCoord) textX:(NXCoord) x
{
    NXPoint point;
    NXRect rect;
    point.x = x;
    point.y = 0.0;
    [self convertPoint:&point toView:text];
    [text getBounds:&rect];
    return point.x - leftMargin - rect.origin.x;
}

- (NXCoord) convertX:(NXCoord) x
{
    NXPoint point;
    point.x = x;
    point.y = 0.0;
    [self convertPoint:&point toView:text];
    return point.x;
}

@end


@implementation NXRulerToken

- setX:(NXCoord)x
{
    curX = x;
    hotSpot.origin.x = curX - info->hotX;
    return self;
}

- initToken:(TokenInfo *)newInfo pos:(NXCoord) x;
{
    info = newInfo;
    hotSpot.size = info->size;
    hotSpot.origin.y = (info->above) ? LINE_X + 1: LINE_X - info->size.height;
    [self setX:x];
    originalX = -1.0;
    deltaX = -1.0;
    return self;
}

- paint:(const NXRect *)bounds;
{
    NXRect temp;
    NXSize bsize;
    NXPoint p;
 
    temp = *bounds;
    if (NXIntersectionRect(&hotSpot, &temp)) {
	p = hotSpot.origin;
	if ([[NXApp focusView] isFlipped]) {
	    [info->bitmap getSize:&bsize];
	    p.y += bsize.height;
	}
	[info->bitmap composite:NX_SOVER toPoint:&p];
    }

    return self;
}

- (NXEvent *) initialHysteresis:(const NXPoint *)start inView:view
{
    NXEvent *event;
    NXPoint location;
    while (1) {
	event = [NXApp getNextEvent:(NX_MOUSEDRAGGEDMASK | NX_MOUSEUPMASK |
	    NX_MOUSEDOWNMASK)];
	location = event->location;
	[view convertPoint:&location fromView:nil];
	switch (event->type) {
	    case NX_MOUSEUP:
	    case NX_MOUSEDOWN:
		if (fabs(location.x - start->x) <= HYSTERESIS)
		    return NULL;
		else 
		    return event;
	    case NX_MOUSEDRAGGED:
		if (fabs(location.x - start->x) > HYSTERESIS)
		    return event;
		break;
	    default:
		break;
	}
    }
    return NULL;
}

- (BOOL) invalidX:(NXCoord *) ptrX inView:(NXRulerView *)view
{
    NXCoord newX;
    NXRect bounds;
    NXCoord x = *ptrX;
    x -= deltaX;
    switch (info->type) {
	case leftindent:
	    if (x < view->startX) {
		*ptrX = view->startX + deltaX;
		x = view->startX;
	    }
	    if (x > (view->endX - MINWIDTH))
		return YES;
	    break;
	case rightindent:
	    if ((x + tokens[RI_INDEX].size.width) > view->endX)
		return YES;
	    if (x < (view->leftIndent + MINWIDTH))
		return YES;
	    break;
	case firstindent:
	    if (x < view->startX) {
		*ptrX = view->startX + deltaX;
		x = view->startX;
	    }
	    if ((x + tokens[FI_INDEX].size.width) > view->endX)
		return YES;
	    break;
	case leftmargin:
	    [view->text getBounds:&bounds];
	    [view->text convertRect:&bounds toView:view];
	    if (x < bounds.origin.x)
		return YES;
	    newX = x + MINWIDTH;
	    if (newX > view->endX)
		return YES;
	    break;
	case rightmargin:
	    [view->text getBounds:&bounds];
	    [view->text convertRect:&bounds toView:view];
	    if (x > NX_MAXX(&bounds))
		return YES;
	    newX = x - tokens[RM_INDEX].size.width;
	    if (newX < view->leftIndent || newX < view->firstIndent)
		return YES;
	    break;
	default:
	    break;
    }
    return NO;
}

- moveTo:(NXCoord)x inView:view oldx:(NXCoord *)oldx last:(BOOL)last;
{
    NXRect bigRect;
    NXCoord oldX;
    
    if ([self invalidX:&x inView:view])
	return self;

    /*
     * Have the scrollview provide the feedback line.
     */
    if(last) 
	[[view superview] _rulerline:*oldx :0.0 last:last];
    else if(oldx && (x != curX || *oldx < 0)) {
	[[view superview] _rulerline:*oldx :x - deltaX last:last];
	*oldx = x - deltaX;
    	}

    if (x != curX) {
	oldX = curX;
	bigRect = hotSpot;
	[self setX:x - deltaX];
	if (fabs(curX - x) > 100.0) {
	    [view drawSelf:&bigRect:1];
	    [view drawSelf:&hotSpot:1];
	} else {
	    NXUnionRect(&hotSpot, &bigRect);
	    [view drawSelf:&bigRect:1];
	}
    }
    [[view window] flushWindow];
    NXPing();
    return self;
}

- updateRuler:(NXRulerView *)view
{
    NXRect bounds;
    NXCoord result;
    switch (info->type) {
	case rightindent:
	    view->rightIndent = curX;
	    break;
 	case leftindent:
	    view->leftIndent = curX;
	    [view->text setSelProp:NX_INDENT to:[view textX:curX]];
	    break;
	case firstindent:
	    view->firstIndent = curX;
	    [view->text setSelProp:NX_FIRSTINDENT to:[view textX:curX]];
	    break;
	case leftmargin:
	    view->lastStyle = 0;
	    [view->text getBounds:&bounds];
	    result = [view convertX:curX] - NX_X(&bounds);
	    [view->text setSelProp:NX_LEFTMARGIN to:result];
	    break;
	case rightmargin:
	    view->lastStyle = 0;
	    [view->text getBounds:&bounds];
	    result = NX_MAXX(&bounds) - [view convertX:curX];
	    [view->text setSelProp:NX_RIGHTMARGIN to:result];
	    break;
	default:
	    break;
   }
    return self;
}


- checkLocation:(NXRulerView *)view
{
    NXCoord newX;
    view->lastStyle = 0;
    if (info->type != tab) {
	[self updateRuler:view];
	return self;
    }
    if (curX < (view->startX - info->hotX) || curX > view->endX) {
	[view removeToken:self];
	[view drawSelf:&hotSpot:1];
	[[view window] flushWindow];
	NXPing();
	[view->text setSelProp:NX_REMOVETAB to:originalX];
	return [self free];
    }
    newX = [view textX:curX];
    if (originalX  == -1.0) {
	[view->text setSelProp:NX_ADDTAB to:newX];
    } else {
	[view->text changeTabStopAt:originalX to:newX];
    }
    originalX = newX;
    return self;
}

- trackMouse:(const NXPoint *)start inView:view
{
    NXEvent *event;
    NXPoint location;
    NXCoord  oldx;
    
    oldx = -1;
    deltaX = 0;
    [self moveTo:curX inView:view oldx:&oldx last:NO]; /* show line */
    PShidecursor();
    event = [self initialHysteresis:start inView:view];
    if (!event) { 
       if(info->type == tab && originalX == -1.0) {
	    [self moveTo:start->x inView:view oldx:&oldx last:YES];
	    PSshowcursor();
	    return [self checkLocation:view];
	}
	else {
	    [self moveTo:curX inView:view oldx:&oldx last:YES]; 
	    PSshowcursor();
	    return self;
	    }
    }
    location = event->location;
    [view convertPoint:&location fromView:nil];
    deltaX = location.x - (hotSpot.origin.x + info->hotX);
    
    while (event) {
	location = event->location;
	[view convertPoint:&location fromView:nil];
	switch(event->type) {
	    case NX_MOUSEUP:
	    case NX_MOUSEDOWN:
		[self moveTo:location.x inView:view oldx:&oldx last:YES];
		PSshowcursor();
		return [self checkLocation:view];
	    case NX_MOUSEDRAGGED:
		[self moveTo:location.x inView:view oldx:&oldx last:NO];
		break;
	}
	event = [NXApp getNextEvent:(NX_MOUSEDRAGGEDMASK | NX_MOUSEUPMASK |
	    NX_MOUSEDOWNMASK)];
    }
    PSshowcursor();
    return self;
}


@end


/*

Modifications (starting at 1.0a):

 3/20/90 bgy	added to appkit as a private class
 
91
--
 8/11/90 glc	New icons. Worked on hit detection and dragging.
 		Added feedback line.

*/
















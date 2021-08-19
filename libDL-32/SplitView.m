#import <appkit/appkit.h>
#import <appkit/Application.h>
#import <appkit/ScrollView.h>
#import <objc/Storage.h>
#import "SplitView.h"
#import "NroffText.h"
#import "bitmap.h"
#import "frames.h"

#define KNOBSIZE	(7.0)
static float    BETWEEN = 8.0;

#define MINHEIGHT	(40.0)
#define BACKGROUNDGRAY	(NX_LTGRAY)
static char    *desc = "{@@ffffffiiii}";
static id       but = 0, butH = 0;
static NXPoint  null = {0., 0.};

#define H	size.height
#define W	size.width
#define X	origin.x
#define Y	origin.y

static void sizeAllViews(id self);

typedef struct {
	id              view, realView;
	float           height, ypos, minHeight;
	int             wantsScroll, wantsBezel, wantsDivider, stretchable;
}               viewStuff;

/*
 * The split view maintains a vertically tiled list of subviews,
 * and deals with attaching scrollviews, resizing, and spacing.
 * You can attach dividers to views -- horizontal separators
 * with nobs on the end which the user can grab and drag to
 * reproportion the views.
 * After +newFrame, you typically use
 *	- addView:view divider:flag
 * or
 *	- addFixedView:view divider:flag
 * (or some variant of this; other methods include flags for scrollers, etc).
 * To get the ScrollView bound to a view, use
 *	- scrollview:view
 */
@implementation SplitView:View

+ newFrame:(NXRect const *) r {
	self = [super newFrame:r];
	[self setAutoresizeSubviews:YES];
	return self;
}

static NXCoord 
height(id view)
{
	NXRect          r;

	[view getFrame:&r];
	return r.H;
}

- addFixedView:view divider:(BOOL)divider
/*
 * Append 'view' -- a view with fixed height -- onto the list.
 * If 'flag' is true, the view can be dragged (like one of the dividers).
 */
{
	return[self addView:view
	       height:height(view)
	       scroll :NO
	       bezel:NO
	       divider:divider
	       at:nviews];
}

- addView:view divider:(BOOL)divider
/*
 * Add a view with a scroller and optional 'divider'.
 */
{
	return[self addView:view
	       height:height(view)
	       divider :divider];
}

- addView:view
	height:(float)height
	divider:(BOOL)divider
{
	return[self addView:view
	       height:height
	       scroll:YES
	       bezel:YES
	       divider:divider
	       at:nviews];
}

- addView:newView
	height:(NXCoord) height
	scroll:(BOOL)scroll
	divider:(BOOL)divider
{
	return[self addView:newView
	       height:height
	       scroll:scroll
	       bezel:scroll	/* this is right */
	       divider:(BOOL)divider
	       at:nviews];
}

- addView:view
	height:(NXCoord) height
	scroll:(BOOL)scroll
	bezel:(BOOL)bezel
	divider:(BOOL)divider
	at:(int)offset
/*
 * The general method; you probably really want to use a stubbier one.
 */
{
	viewStuff _stuff, *v = &_stuff;

	[view removeFromSuperview];
	v->realView = view;
	v->height = height;
	v->wantsScroll = scroll;
	v->wantsBezel = bezel;
	v->wantsDivider = divider;
	v->stretchable = 0;
	v->minHeight = MINHEIGHT;
	if (!nviews) {
		int size = sizeof(viewStuff);

		views = (id)[Storage newCount:0 elementSize:size description:desc];
		[self setFlip:YES];
	}
	views = (id)[views insert:(char *)v at:offset];
	nviews++;
	valid = NO;
	return self;
}

static NXCoord
calcSpacer(viewStuff * v, int numViews)
{
	int             i;
	NXCoord         y = 0.0;

	for (i = 0; i < (numViews - 1); i++) {
		y += (v->wantsDivider) ? BETWEEN : 1.0;
		v++;
	}
	return y;
}

static
calcHeights(id self, viewStuff * v)
{
	int             n = self->nviews, i, numStretch;
	NXCoord         newY = self->frame.H, spacerHeight = calcSpacer(v, n),
	                oldY = spacerHeight, extraY, y;
	register viewStuff *element;

 /* collect heights of those pre-specified */
	for (i = 0, element = v, numStretch = 0; i < n; i++, element++) {
		oldY += element->height;
		if (element->stretchable)
			numStretch++;
	}
	if (newY == oldY)
		return;

	extraY = newY - oldY;

 /* assign new heights to the stretchable ones */
	for (i = 0, element = v; i < n; i++) {
		if (element->stretchable) {
			element->height += extraY / numStretch;
		}
		element->height = MAX(floor(element->height), element->minHeight);
		element++;
	}

 /* if the stretchy ones didn't let you get small enough... */
	extraY = newY - spacerHeight;
	for (i = 0, element = v; i < n; i++, element++) {
		extraY -= element->height;
	}
	extraY *= (-1);
	for (i = n - 1, element = v + n - 1; extraY && i >= 0; i--, element--) {
	/* this one could use some squeezing */
		if (element->height > element->minHeight) {
			int diff = element->height - element->minHeight;

			if (diff >= extraY) {
				diff = extraY;
			}
			element->height -= diff;
			extraY -= diff;
		}
	}

 /* this is a hack so the resize bar doesn't look cluttered. */
	element = v + n - 1;
	if (element->wantsBezel)
		element->height += 2.0;

 /* assign y positions */
	y = 0.0;
	for (i = 0, element = v; i < n; i++) {
		element->ypos = y;
		y += element->height + (element->wantsDivider ? BETWEEN : 1.0);
		element++;
	}
}

/* this only fiddles with v->realview, v->view and the view heirarchy */
static void 
addSubviews(id self, viewStuff * v)
{
	int             i;
	NXCoord         width = self->frame.W;
	id              scroll;
	NXRect          r;

	for (i = 0; i < self->nviews; i++) {
		NXSetRect(&r, 0., v->ypos, width, v->height);
		if (v->wantsScroll) {
			scroll = [ScrollView newFrame:&r];
			[scroll setBorderType:v->wantsBezel ? NX_BEZEL : NX_LINE];
			[scroll setVertScrollerRequired:YES];
			[scroll setDynamicScrolling:YES];
			[scroll setBackgroundGray:NX_LTGRAY];
			[v->realView moveTo:0.0:0.0];
			[scroll setDocView:v->realView];
			[v->realView setAutosizing:NX_WIDTHSIZABLE];
			[self addSubview:scroll];
			v->view = scroll;
			[scroll setAutoresizeSubviews:YES];
		/* make sure the clip view resizes too */
			[v->realView->superview setAutoresizeSubviews:YES];
		} else {
			v->view = v->realView;
			[v->view moveTo:r.X:r.Y];
			[v->view sizeTo:r.W:r.H];
			[self addSubview:v->view];
		}
		v++;
	}
}

static
adjustSubviews(id self)
{
	viewStuff      *v = (viewStuff *)[self->views elementAt:0];
	int             i;

	if (!but) {
		but = bitmap("nob");
		butH = bitmap("nobH");
	}
	calcHeights(self, v);
	addSubviews(self, v);
	self->valid = YES;
	sizeAllViews(self);
}

/* 
 * have to do an adjustViews if you care about view, realview and the view
 * heirarchy 
 */
- (viewStuff *) _getStuff:view
/*
 * Return the viewStuff pointer containing the given 'view'.
 */
{
	int             i;
	viewStuff      *v = (viewStuff *) 0;

	for (i = 0; i < nviews; i++) {
		v = (viewStuff *)[self->views elementAt:i];
		if (v && v->realView == view)
			return v;
	}
	return (viewStuff *) 0;
}

#define SET(view,name,flag)	\
	viewStuff *v = [self _getStuff:view]; \
	if (v) v->name = flag

#define GET(view,name,default)	\
	viewStuff *v = [self _getStuff:view]; \
	return v? v->name : default

- (int)divider:view
/*
 * True if 'view' has a divider at the bottom.
 */
{
	GET(view, wantsDivider, NO);
}

- setDivider:view :(BOOL)flag
{
	SET(view, wantsDivider, flag);
}

- (float)minHeight:view
/*
 * True if 'view' has a divider at the bottom.
 */
{
	GET(view, minHeight, 0.0);
}

- setMinHeight:view :(float)y
{
	SET(view, minHeight, y);
}

static
fixedHeight(viewStuff * v)
{
	return (v->wantsScroll == NO && v->stretchable == 0);
}

- (int)stretchable:view
{
	GET(view, stretchable, NO);
}

- setStretchable:view :(BOOL)flag
/*
 * 'view' will be proportionally sized (vertically) if it's reshaped.
 */
{
	SET(view, stretchable, flag);
}

- scrollview:view
/*
 * Return the scrollview containing 'view'
 */
{
	viewStuff      *v;

	if (!valid)
		adjustSubviews(self);
	v = [self _getStuff:view];
	return v ? v->view : nil;
}

static          NXCoord
getContentWidth(NXCoord width, int bezel)
{
	NXSize          scrollS, S;

	scrollS.width = width;
	scrollS.height = 100.0;
	[ScrollView getContentSize:&S forFrameSize:&scrollS
	 horizScroller:NO vertScroller:YES
	 borderType:bezel ? NX_BEZEL : NX_LINE];
	return S.width;
}

 /*
  * this resizes the views according to the internal heights that have
  * already been figured out 
  */
static void 
sizeAllViews(id self)
{
	viewStuff      *v = (viewStuff *)[self->views elementAt:0];
	id              window = [self window];
	int             i;
	NXCoord         width = self->frame.W, innerwidth;

	[window disableFlushWindow];
	for (i = 0; i < self->nviews; i++) {
		[v->view moveTo:0.0:v->ypos];
		[v->view sizeTo:width :v->height];
		v++;
	}
	[self display];
	[window reenableFlushWindow];
	[window flushWindow];
}

- sizeTo:(NXCoord) x :(NXCoord) y
{
	[window disableFlushWindow];
	[super sizeTo:x :y];
	if (nviews) {
		if (!valid)
			adjustSubviews(self);
		else {
			viewStuff      *v = (viewStuff *)[self->views elementAt:0];

			calcHeights(self, v);
			sizeAllViews(self);
		}
	}
	[window reenableFlushWindow];
	return self;
}

- superviewSizeChanged:(NXSize *) old
{
	NXRect          r, new;
	NXPoint         gap;

	[window disableFlushWindow];
	[self getFrame:&r];
	gap.x = old->width - r.W;
	gap.y = old->height - r.H;
	[[self superview] getBounds:&new];
	[self moveTo:r.X:r.Y];
	[self sizeTo:new.W - gap.x:new.H - gap.y];
	[window reenableFlushWindow];
	[window flushWindow];
	return self;
}

- drawSelf:(NXRect *) rects :(int)rectCount
{
	int             i;
	NXRect          outer, inner, line;
	viewStuff      *v = ((viewStuff *)[self->views elementAt:0]) + 1;
	float           background = BACKGROUNDGRAY;
	NXCoord         minY, maxY, butX;
	NXPoint         point;

	if (!nviews)
		return nil;
	if (!valid)
		adjustSubviews(self);
	line.origin = inner.origin = outer.origin = null;
	line.W = inner.W = outer.W = frame.W;
	inner.H = outer.H = BETWEEN;
	line.H = 1.0;
	butX = (frame.W - KNOBSIZE) / 2;
	minY = NX_Y(rects);
	maxY = NX_MAXY(rects);
	for (i = 1; i < nviews; i++) {
		viewStuff      *prev = v - 1;
		NXCoord         top = v->ypos - (prev->wantsDivider ? BETWEEN : 1.0);

		if (top >= minY && top <= maxY) {
			if (prev->wantsDivider) {
				outer.Y = inner.Y = top;
			/*
			 * N.B. -- this tests to see if the surrounding
			 * views are scrolling, in which case we've drawn
			 * either a border or a bezel. to guard against
			 * knowing whether or not the view has its own
			 * bordering. in general we need a 'hasBorder'
			 * field in the viewStuff... 
			 */
				if (prev->wantsScroll)
					outer.H--;
				prev = v;
				if (prev->wantsScroll)	/* c.f. "N.B." */
					outer.H--, outer.Y++;
				PSsetgray(NX_BLACK);
				NXRectFill(&outer);
				PSsetgray(background);
				NXRectFill(&inner);
				point.x = butX;
				point.y = v->ypos - 1.0 - 6.;
				[but composite:NX_COPY toPoint:&point];
			} 
		}
		v++;
	}
	return self;
}

static 
getYBounds(id self, viewStuff * v, int offset, NXCoord * minY, NXCoord * maxY)
{
	viewStuff      *this = v + offset, *prev = this - 1;

	*minY = prev->ypos + prev->minHeight;
	*maxY = this->ypos + this->height - (this->minHeight + BETWEEN);
}

static 
setupRects(id self, NXRect * r)
{
	r->X = 0.0;
	r->H = BETWEEN;
	r->W = self->frame.W;
}

static
mouseHit(id self, NXPoint * p)
{
	NXCoord         minY, maxY, y;
	int             i;
	viewStuff      *v;

 /* it's within X because we know we are in the view */
	v = ((viewStuff *)[self->views elementAt:0]) + 1;
	y = p->y;
	for (i = 1; i < self->nviews; i++) {
		viewStuff      *prev = v - 1;

		minY = v->ypos - (prev->wantsDivider ? BETWEEN : 1.0) + 3.0;
		maxY = minY + KNOBSIZE;
		if (y >= minY && y <= maxY && prev->wantsDivider)
			return i;
		if (y < minY)
			return 0;
		v++;
	}
	return 0;
}

static
resizeView(id self, viewStuff * v, int offset, NXCoord linePos)
{
	NXCoord         minY, maxY, width, delta;
	viewStuff      *this = v + offset, *prev = this - 1;
	NXRect          r;
	id              window = [self window];

	[window disableFlushWindow];
	delta = (linePos - prev->ypos) - prev->height;
	minY = prev->ypos;
	maxY = this->ypos + this->height;
	width = self->frame.W;
	if (fixedHeight(prev)) {
		viewStuff      *v;

		this->ypos += delta;
		v = prev - 1;
		minY = v->ypos;
		v->height += delta;
		[prev->view moveTo:0.:prev->ypos];
		[v->view sizeTo:width :v->height + 1];
	} else {
		prev->height = linePos - prev->ypos;
		this->ypos = prev->ypos + prev->height + BETWEEN;
	}
	if (!fixedHeight(this))
		this->height = maxY - this->ypos;
	[this->view moveTo:0.0:this->ypos];
	[this->view sizeTo:width :this->height];
	[prev->view sizeTo:width :prev->height];
	NXSetRect(&r, 0., minY - 1., width, maxY - minY + 2.);
	[self display:&r:1];
	[window reenableFlushWindow];
	[window flushWindow];
}

#define DRAGMASK	(NX_LMOUSEUPMASK|NX_MOUSEDRAGGEDMASK)
- mouseDown:(NXEvent *) e
{
	NXPoint         p = e->location, point;
	viewStuff      *v = (viewStuff *)[self->views elementAt:0];
	register viewStuff *element;
	int             offset, wnum, oldMask, gstack, first = 1;
	BOOL            looping = YES;
	NXRect          instanceRect, line, _outer;
	register NXRect *outer = &_outer;
	NXCoord         minY, maxY, ypos, delta, linePos, origLinePos, lastPos;
	NXCoord         butX = (frame.W - KNOBSIZE) / 2;
	float           background = BACKGROUNDGRAY;
	id              firstResponder;

	[self convertPoint:&p fromView:nil];
	if (!(offset = mouseHit(self, &p))) {
		(void)[NXApp getNextEvent:NX_LMOUSEUPMASK];
		return self;
	}
	element = v + offset;
	if (fixedHeight(element - 1)) {	/* track a window (the fixed view) */
		NXRect          r, s;
		id              wv, w, view = (element - 1)->view;
		int             stack = (int)[window gState];
		NXCoord         x, y, shift, startY, endY;
		NXPoint         prev;

		[view getFrame:&r];
		[[window contentView] convertRect:&r fromView:[view superview]];
		[[window contentView] getFrame:&s];
		r.H += 12.;
		r.Y -= 9.;
		r.X = 1.;
		r.W += (s.W - r.W);
		s = r;
		[window convertBaseToScreen:&r.origin];
		x = r.X;
		y = r.Y;
		r.origin = null;
		w = [Window newContent:&r style:NX_PLAINSTYLE backing:NX_RETAINED buttonMask:0 defer:NO];
		r.X = x;
		r.Y = y;
		[w placeWindow:&r];
		endY = startY = e->location.y;
		prev = p = e->location;
		[window convertBaseToScreen:&p];
		shift = p.y - y;
		maxY = startY + ((element - 2)->height - (element - 2)->minHeight);
		minY = startY - ((element)->height - element->minHeight - 12.);
		r = s;
		wv = [w contentView];
		[wv lockFocus];
		[wv getFrame:&s];
		PSsetgray(0.);
		PSrectfill(0., 0., s.W, s.H);
		PSsetgray(1.);
		PSrectfill(0., 1., s.W - 1., s.H - 1.);
		PSsetgray(.333);
		PSrectfill(2., 1., s.W - 3., s.H - 2.);
		PSsetgray(.666);
		PSrectfill(2., 2., s.W - 4., s.H - 3.);
		PScomposite(r.X + 2., r.Y + 2., s.W - 4., s.H - 8., stack, 2., 2., 1);
		[wv unlockFocus];
		[w orderWindow:NX_ABOVE relativeTo:[window windowNum]];
		oldMask = [window eventMask];
		[window setEventMask:(oldMask | DRAGMASK)];
		p = e->location;
		while (e->type != NX_LMOUSEUP) {
			e = [NXApp getNextEvent:DRAGMASK];
			if (first && e->type != NX_LMOUSEUP) {
				first = 0;
				[[window contentView] lockFocus];
				PSsetgray(.666);
				PSrectfill(r.X + 6., r.Y + 2., s.W - 15., s.H - 8.);
				[[window contentView] unlockFocus];
				[window convertBaseToScreen:&p];
				p.x = x;
				p.y -= shift;
				prev = p = e->location;
			}
			if (e->location.y <= minY) {
				e->location.y = minY;
				p.y--;
			} else if (e->location.y >= maxY) {
				e->location.y = maxY;
				p.y++, p.y++;
			}
			prev = p = e->location;
			[window convertBaseToScreen:&p];
			p.x = x;
			p.y -= shift;
			if (e->type != NX_LMOUSEUP) {
				endY = e->location.y;
				[w moveTo:p.x:p.y];
			}
		}
		if (endY != startY)
			resizeView(self, v, offset, element->ypos - (endY - startY));
		else
			[(element - 1)->view display];
		[w close];
	} else {
		firstResponder = [window firstResponder];
		if (firstResponder)
			[window makeFirstResponder:self];
		[self lockFocus];
		getYBounds(self, v, offset, &minY, &maxY);
		wnum = [window windowNum];
		gstack = [window gState];
		PSsetinstance(YES);
		oldMask = [window eventMask];
		[window setEventMask:(oldMask | DRAGMASK)];
		setupRects(self, outer);
		origLinePos = linePos = element->ypos - BETWEEN;
		outer->Y = linePos;
		if (linePos < minY)
			linePos = minY;
		if (linePos > maxY)
			linePos = maxY;
		delta = linePos - p.y;

	/* start */
		PSgsave();
		PSsetgray(NX_BLACK);
		PSsetalpha(0.3333);
		PScompositerect(0.0, linePos,
				outer->W, outer->H, NX_SOVER);
		point.x = butX;
		point.y = linePos + BETWEEN - 1.0 - 6.;
		[butH composite:NX_COPY toPoint:&point];
		lastPos = linePos;
		while (looping) {
			e = [NXApp getNextEvent:DRAGMASK];
			if (first && e->type != NX_LMOUSEUP) {
				first = 0;
			}
			p = e->location;
			[self convertPoint:&p fromView:nil];
			linePos = p.y + delta;
			if (linePos < minY)
				linePos = minY;
			else if (linePos > maxY)
				linePos = maxY;
			if (linePos != lastPos) {
				NXRect         *rect = &instanceRect;

				PSnewinstance();
				PScompositerect(0.0, linePos,
					   outer->W, outer->H, NX_SOVER);
				point.x = butX;
				point.y = linePos + BETWEEN - 1.0 - 6.;
				[butH composite:NX_COPY toPoint:&point];
				lastPos = linePos;
			}
			if (e->type == NX_LMOUSEUP) {
				looping = NO;
			}
		}
		[window setEventMask:oldMask];
		PSgrestore();
		if (linePos == origLinePos)
			PSnewinstance();
		else {
			PSsetgray(background);
			NXRectFill(outer);
		}
		PSsetinstance(NO);
		[self unlockFocus];
		if (linePos != origLinePos)
			resizeView(self, v, offset, lastPos);
		if (firstResponder)
			[window makeFirstResponder:firstResponder];
	}
	return self;
}

- refreshText:textView
/*
 * make the view fit the data, scroll it to the origin and update the
 * scrollbar.
 */
{
	NXCoord         h;
	NXRect          r;
	id              scrollview = [self scrollview:textView];

	if (!scrollview)
		return;

	if ([textView respondsTo:@selector(sizeToFit)]) {
		[textView sizeToFit];
	}
	[textView getBounds:&r];
	[textView scrollPoint:&r.origin];
	[scrollview resizeSubviews:NULL];
	[textView display];
	return self;
}

void
fixScrolls(id view, id splitView)
/*
 * Some initializing tweaks for scrolling (split) views.
 * A textual view gets a white background and is resizeable.
 * The Summary view has no arrows.
 */
{
	id              scrollV;
	NXRect          r;

	if (scrollV = [splitView scrollview:view]) {
		[scrollV setPageScroll:20.];
		if ([view respondsTo:@selector(resizeText::)]) {
			[scrollV setBackgroundGray:NX_WHITE];
			[view setResizeableText];
			[splitView setStretchable:view :YES];
		}
		[scrollV getBounds:&r];
		[view scrollPoint:&(r.origin)];
	}
}

/* parse the string into the frames structure.  The string is of the
 * form "x,y,w,h,n,h1,h2,h3..." where x,y,w,h are the frame
 * rectangle for the main window, n is the number of subviews, and h1 - hn
 * following numbers are the heights of the subviews in the splitview
 * (from top to bottom). commas seperate the numbers.
 * this string is used for archiving the window positioning.
 */
static char *readCoord(char *str, float *val)
{
	char *next;
	
	if (!str) {
		*val = 0.0;
		return str;
	}
	
	next = index(str, ',');
	if (next) {
		*next = '\0';
		next++;
	}
	
	*val = atof(str);
	return next;
}

frames Frames;

void readFrames(char *frameStr)
{
	float dummy;
	int i;
	
	frameStr = readCoord(frameStr, &(Frames.mainWindow.origin.x));
	frameStr = readCoord(frameStr, &(Frames.mainWindow.origin.y));
	frameStr = readCoord(frameStr, &(Frames.mainWindow.size.width));
	frameStr = readCoord(frameStr, &(Frames.mainWindow.size.height));
	
	frameStr = readCoord(frameStr, &dummy);
	Frames.numSubviews = (int)dummy;
	Frames.subviews = (float *)malloc(dummy*sizeof(float));
	if (!Frames.subviews) {
		Frames.numSubviews = 0;
		return;
	}
	for (i = 0; i < dummy; i++) {
		frameStr = readCoord(frameStr, &(Frames.subviews[i]));
	}
}

static void writeCoord(char *str, float val)
{
	char buf[100];
	
	sprintf(buf, "%d", (int)val);
	strcat(str, buf);
	strcat(str, ",");
}
	
void writeFrames(char *frameStr)
{
	NXRect mainFrame, win;
	int i;
	id self = [NXApp splitView];
	viewStuff *v = (viewStuff *)[self->views elementAt:0];
	
	frameStr[0] = '\0';
	
	[[self window] getFrame:&win];
	[Window getContentRect:&mainFrame forFrameRect:&win style:NX_SIZEBARSTYLE];
	writeCoord(frameStr, mainFrame.origin.x);
	writeCoord(frameStr, mainFrame.origin.y);
	writeCoord(frameStr, mainFrame.size.width);
	writeCoord(frameStr, mainFrame.size.height);
	
	writeCoord(frameStr, (float)self->nviews);
	for (i = 0; i < self->nviews; i++) {
		writeCoord(frameStr, v->height);
		v++;
	}
	/* nuke that extra comma on the end */
	frameStr[strlen(frameStr) - 1] = '\0';
}


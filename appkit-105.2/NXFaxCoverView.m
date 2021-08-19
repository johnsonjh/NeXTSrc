/*
	NXFaxCoverView.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Chris Franklin
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Text.h"
#import <objc/error.h>
#import <defaults.h>
#import "errors.h"
#import "graphics.h"
#import "nextstd.h"
#import <dpsclient/dpsclient.h>
#import <dpsclient/wraps.h>
#import "Application.h"
#import "privateWraps.h"

#import <streams/streams.h>
#import <sys/param.h>

extern id NXApp;

#import "NXFaxCoverView.h"

@implementation NXFaxCoverView

+ new {
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

- init {
    static const NXRect empty = {{0.0,0.0},{0.0,0.0}};
    
    [self setOpaque:YES];
    dataPS = NULL;
    graphicsRect = empty;
    textRect = empty;
    cache = NULL;
    return self;
}

- free {
    [cache free];
    return NULL;
}

- drawGraphics {

/*  Draws background graphics in scaled CoverView.  the resulting bitmap is
    cached in cache, so that it can just be composited next time.
*/

    NXRect cacheFrame;
    NXRect trueBounds;
    char unique[50];
    float sx,sy;
    
    trueBounds = graphicsRect;
    [[self superview] convertSize:&trueBounds.size fromView:self];
    sx = bounds.size.width / frame.size.width;
    sy = bounds.size.height / frame.size.height;
    while (1) {
	if (cache) {
	    [[cache contentView] getFrame:&cacheFrame];
	    if (floor(trueBounds.size.width+1.0) == cacheFrame.size.width &&
		floor(trueBounds.size.height+1.0) == cacheFrame.size.height) {
		PScomposite(cacheFrame.origin.x, cacheFrame.origin.y,
			    floor(graphicsRect.size.width),
			    floor(graphicsRect.size.height),
			    [cache gState],
			    floor(graphicsRect.origin.x),
			    floor(graphicsRect.origin.y),
			    NX_SOVER);
		break;
	    }
	}
    
	[cache free];
	cacheFrame.origin.x = cacheFrame.origin.y = 0;
	cacheFrame.size.width = floor(trueBounds.size.width+1.0);
	cacheFrame.size.height = floor(trueBounds.size.height+1.0);
	cache = [Window allocFromZone:[self zone]];
	[cache initContent:&cacheFrame
			     style:NX_PLAINSTYLE
			   backing:NX_RETAINED
			buttonMask:0
			     defer:NO];
			     
	[[cache contentView] lockFocus];
	sprintf(unique, "-save-fax-");
	DPSPrintf(DPSGetCurrentContext(), "%%%%BeginFile: <Fax subdocument>\n");
	_NXPSStartDrawingCustom(unique, 
		graphicsRect.size.width,
		graphicsRect.size.height,
		0.0 - bboxPS[0], 0.0 - bboxPS[1],
		(graphicsRect.size.width / (bboxPS[2] - bboxPS[0])) / sx,
		(graphicsRect.size.height / (bboxPS[3] - bboxPS[1])) / sy);
	DPSWritePostScript(DPSGetCurrentContext(), dataPS, lengthPS);
	_NXPSEndCustom(unique, bboxPS[0], bboxPS[1],
		    (bboxPS[2] - bboxPS[0]) / graphicsRect.size.width * sx,
		    (bboxPS[3] - bboxPS[1]) / graphicsRect.size.height * sy);
	DPSPrintf(DPSGetCurrentContext(), "%%%%EndFile\n");
	[[cache contentView] unlockFocus];
    } /* while */
    return self;
}

static void drawScaledText(const NXRect *textRect,const NXCoord lineHeight)
{
    int i,lines,linesTotal;
    NXRect rectList[100];
    NXRect baseRect;
    float offset;
    
    baseRect = *textRect;
    baseRect.size.height = 1.0;
    offset = floor((lineHeight-baseRect.size.height)/2.0);
    baseRect.origin.y = textRect->origin.y + textRect->size.height - 
    			offset - baseRect.size.height;
    linesTotal = 
        floor((textRect->size.height-offset-baseRect.size.height) / lineHeight);
    PSsetgray(NX_LTGRAY);
    while (linesTotal) {
        lines = linesTotal <= 100 ? linesTotal : 100;
        for (i = 0 ; i < lines; i++) {
	    rectList[i] = baseRect;
	    baseRect.origin.y -= lineHeight;
	}
	NXRectFillList(rectList,lines);
        linesTotal -= lines; 
    }    
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
/*  We operate in two mutually exclusive modes: in scaled preview, we always
    are drawing, in unscaled cover sheet we always are generating postscript
    into a stream.  In preview, we may or may not show graphics (depending on
    whether dataPS != NULL, but we currently show text view as black rectangle.
    
    When generating cover sheet, we include text and graphics, if we have them.
*/

    char unique[50];
    
    if (NXDrawingStatus != NX_DRAWING) { /* generating PS for cover sheet */
	if (dataPS) {
	    sprintf(unique, "-save-fax-");
	    DPSPrintf(DPSGetCurrentContext(), 
		      "%%%%BeginFile: <fax cover subdocument>\n");
	    _NXPSStartCopyingCustom(unique, 
	    		  graphicsRect.size.width, 
			  graphicsRect.size.height,
			  graphicsRect.origin.x, graphicsRect.origin.y,
			  0.0 - bboxPS[0],  0.0 - bboxPS[1],
			  graphicsRect.size.width / (bboxPS[2] - bboxPS[0]),
			  graphicsRect.size.height / (bboxPS[3] - bboxPS[1]));
	    DPSWritePostScript(DPSGetCurrentContext(), dataPS, lengthPS);
	    _NXPSEndCustom(unique, bboxPS[0], bboxPS[1],
			(bboxPS[2] - bboxPS[0]) / graphicsRect.size.width,
			(bboxPS[3] - bboxPS[1]) / graphicsRect.size.height);
	    DPSPrintf(DPSGetCurrentContext(), "%%%%EndFile\n");
	}
    } else { /* Drawing SCALED! preview in Cover Sheet Panel */
	PSsetgray(NX_WHITE);
	NXRectFill(rects);
	
	if (graphicsRect.size.width > 0.0) { /* exists background */
	    if (!dataPS) {
		PSsetgray(NX_DKGRAY);
		NXRectFill(&graphicsRect);
	    } else
		[self drawGraphics];
	}
	drawScaledText(&textRect,lineHeight);
    }
    return self;	
}

#define	KEYMASKS	(NX_KEYDOWNMASK|NX_KEYUPMASK)

- mouseDown:(NXEvent *) theEvent {
/*  This routine is only used in preview case, and what it does is drag around
    representations of text and background graphics, so that user can reposition
    them.
*/
    NXEvent         *pEvent,locEvent;
    int             emask;
    NXPoint         mouse1st,newPoint,lastPoint;
    NXRect	    newRect,original,*changeRect;
    NXCoord	    left,right,top,bottom;
    int 	    etype;
    
    left = bounds.origin.x;
    right = left+bounds.size.width;
    top = bounds.origin.y;
    bottom = top+bounds.size.height;
    
    mouse1st = theEvent->location;
    [self convertPoint:&mouse1st fromView:NULL];
    lastPoint = mouse1st;
    [self lockFocus];
    changeRect = NULL;
    if (NXPointInRect(&mouse1st,&textRect)) {
	original = textRect;
	changeRect = &textRect;
    } else if (NXPointInRect(&mouse1st,&graphicsRect)){
	original = graphicsRect;
	changeRect = &graphicsRect;
    }
    newRect.size = original.size;

    emask = (int)[window eventMask];
    [window setEventMask:(emask | NX_MOUSEDRAGGEDMASK)];

    while (1) {			/* exit on mouseup */
	pEvent = [NXApp getNextEvent:
		(KEYMASKS | NX_LMOUSEUPMASK | NX_LMOUSEDRAGGEDMASK)
		    waitFor : NX_FOREVER
		    threshold:NX_MODALRESPTHRESHOLD];
	etype = pEvent->type;
	if (etype != NX_LMOUSEDRAGGED && etype != NX_LMOUSEUP)
	    continue;
	locEvent = *pEvent;
	newPoint = locEvent.location;
	[self convertPoint:&newPoint fromView:NULL];
	if (etype == NX_LMOUSEDRAGGED) {
	    if (lastPoint.x != newPoint.x || lastPoint.y != newPoint.y) {
		newRect.origin.x = 
		    floor(original.origin.x + (newPoint.x-mouse1st.x));
		newRect.origin.y = 
		    floor(original.origin.y + (newPoint.y-mouse1st.y));
		if (newRect.origin.x < left)
		    newRect.origin.x = left;
		else if(newRect.origin.x+newRect.size.width > right)
		    newRect.origin.x = right-newRect.size.width;
		if (newRect.origin.y < top)
		    newRect.origin.y = top;
		else if(newRect.origin.y+newRect.size.height > bottom)
		    newRect.origin.y = bottom-newRect.size.height;
		    
		if (changeRect && !NXEqualRect(&newRect,changeRect)) {
		    *changeRect = newRect;
		    [self drawSelf:&bounds:1];
		    [window flushWindow];
		}
	    }
	    lastPoint = newPoint;
	} else if (etype == NX_LMOUSEUP)
	    break;
    }

    [window flushWindow];
    [window setEventMask:emask];
    [self unlockFocus];
    return self;
}


- setCoverText : aTextObject at : (NXCoord) x : (NXCoord) y
{
    [self addSubview : aTextObject];
    [aTextObject moveTo:x:y];
    return self;
}

- setBackgroundData : (char *) data  length : (int) length bbox : (float *) bbox 
{
    if (dataPS != data || lengthPS != length || (!bbox) ||
        bcmp(bboxPS,bbox,4*sizeof(float))) {
        [cache free];
	cache = NULL;
    }
    if (bbox)
	bcopy(bbox,bboxPS,sizeof(bboxPS));
    dataPS = data;
    lengthPS = length;
    return self;
}

- graphicsRect : (NXRect *) aRect {
    *aRect = graphicsRect;
    return self;
}

- textRect : (NXRect *) aRect {
    *aRect = textRect;
    return self;
}

- setGraphicsRect : (NXRect *) aRect {
    graphicsRect = *aRect;
    return self;
}

- setTextRect : (const NXRect *) aRect lineHeight : (NXCoord) height {
    textRect = *aRect;
    lineHeight = height;
    return self;
}

/*
  appkit.79  Created by cmf  
  appkit.80  cmf:modified to show text as grey lines
  
85
--
 5/25/90 chris	fixed bug in setBackgroundData:length:bbox:
*/

@end


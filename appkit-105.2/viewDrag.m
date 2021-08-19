/*
	View.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB


#import "Application.h"
#import "Listener.h"
#import "NXImage.h"
#import "Speaker.h"
#import "View.h"
#import "Window.h"
#import "privateWraps.h"
#import <mach.h>
#import <servers/netname.h>
#import <streams/streams.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXViewCategoryIconDragging=0\n");
    asm(".globl .NXViewCategoryIconDragging\n");
#endif

static port_t getWMport(void);

@implementation View(IconDragging)
- dragFile:(const char *)filename fromRect:(NXRect *)rect 
    slideBack:(BOOL) aFlag event:(NXEvent *)event
{
    id appSpeaker = [NXApp appSpeaker];
    port_t port = getWMport();
    NXRect winRect = *rect;
    double sx, sy, mx, my;
    NXEvent evbuf;
    NXPoint p;
    int eNum, err;

    eNum = event->data.mouse.eventNum;
    
    if (port == PORT_NULL)
	return nil;
    [appSpeaker setSendPort:port];

    p = event->location;
    [window convertBaseToScreen:&p];
    mx = p.x; my = p.y;
    [self convertRect:&winRect toView:nil];
    [window convertBaseToScreen:&winRect.origin];
    sx = winRect.origin.x; sy = winRect.origin.y;
    err = [appSpeaker selectorRPC:"dragFiles:sx:sy:eNum:mx:my:slideBack:"
	paramTypes:"cddiddi", filename, sx, sy, eNum, mx, my, (int)aFlag];
    if (err)
	return nil;
    [NXApp peekNextEvent:NX_MOUSEUPMASK into:&evbuf];
    p = event->location;
    [window convertBaseToScreen:&p];
    sx = p.x; sy = p.y;

    if ([appSpeaker selectorRPC:"endDraggingAtX:y" paramTypes:"dd", sx, sy])
	return nil;
    return self;
}




@end

static port_t getWMport(void) 
{
    static port_t p = PORT_NULL;
    if (!p) {
	int err = netname_look_up(name_server_port,"",(char *)NX_WORKSPACEREQUEST,&p);
	if (err)
	    p = PORT_NULL;
    }
    return p;
}


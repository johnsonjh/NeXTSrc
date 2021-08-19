#import <appkit/Application.h>
#import <appkit/Button.h>
#import <appkit/Cursor.h>
#import <appkit/View.h>

#import "bitmap.h"
#import "confirm.h"
#import "aux.h"

/* Don't need this any more, thanks to Trey and Ted */
/* extern id NXwait; */
void Wait() {/* [NXwait push]; */}
void DoneWaiting(){/* [NXWait pop]; */}

void notImp(char *name)
{
	error("%s is not implemented yet.", name);
}

/* 
 * these routines are useful for checking if a search is interrupted
 * and giving feedback.
 * check interrupted() during a loop, and when the loop is over, display the 
 * real altIcon for the button (the original should be a stop sign).
 */
static id interruptBtn = nil;
static const char *altHighlight = NULL;
static int interruption=0;
static NXRect bbox, *iBtnBounds;

/* 
 * do this after the interruptable loop
 * (it primes for the next time.)
 */
static int setInterrupted()
{
	[interruptBtn setAltIcon:altHighlight];
	[interruptBtn display];
	DPSFlush();
	NXPing();
	[interruptBtn setAltIcon:"stop"];
	interruption++;
	return YES;
}

static int pointInBtn(NXPoint *hit)
{
	[interruptBtn convertPoint:hit fromView:nil];
	if (((NX_X(iBtnBounds) < hit->x) && (hit->x < NX_MAXX(iBtnBounds)) &&
	     (NX_Y(iBtnBounds) < hit->y) && (hit->y < NX_MAXY(iBtnBounds)))) {
		return setInterrupted();
	}
	return NO;
}

int interrupted(){
	NXEvent *e;

	/* if it's already been interrupted */
	if (interruption) return YES;

	/* if they've typed CMD-. */
	if (e = [NXApp peekAndGetNextEvent:NX_KEYDOWNMASK]) {
		if ((e->data.key.charCode == '.') 
				&& (e->flags & NX_COMMANDMASK)) {
			return setInterrupted();
		}
	}

	/* if they've hit the stop button */
	if ((e = [NXApp peekAndGetNextEvent:NX_MOUSEDOWNMASK]) == NULL)
		return NO;
	return pointInBtn(&(e->location));
}

/* 
 * this is useful if you don't want to check EVERY time through the loop
 */
int sampledInterrupt()
{
	static unsigned short sample = 0;

	if (interruption) return YES;

	if ((++sample) % 10) return NO;
	return interrupted();
}

/* 
 * do this when the button is created
 */
void setupInterruptable(id button, const char *otherH)
{
	interruptBtn = button;
	altHighlight = otherH;

	(void)bitmap("stop");
	[button setAltIcon:"stop"];

	iBtnBounds = &bbox;
	[interruptBtn getBounds:iBtnBounds];

	interruption=0;
}

/* do this after the loop is finished to make sure
 * that all the mouse up/down events are cleared
 */
void flushMouseEvents()
{
	/* toss all the mouse events */
	while ([NXApp getNextEvent: (NX_LMOUSEDOWNMASK | NX_LMOUSEUPMASK)
			waitFor:0.1 threshold:NX_MODALRESPTHRESHOLD])
		;
	interruption=0;
}


/*********/

/* 
 * checks the event to see if the alt key was held down.
 */
int alternateMask(){
	NXEvent *e = [NXApp currentEvent];

	return e && (e->flags & NX_ALTERNATEMASK);
}


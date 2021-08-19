#import <appkit/Control.h>

@interface  LRSwitch:Control
{
    id    leftText, rightText, button;
    NXRect leftR, rightR, buttonR;
    float gap;
    struct {
      /* MUST BE PADDED TO 16 BITS!! */
	unsigned int	calcSize:1;		/* recompute cell size? */
	unsigned int    _PADDING:15;
    }		    bFlags;	/* */
}

- setLeftTitle:(char *)aString;
- setRightTitle:(char *)aString;
- redraw;
+ newFrame:(NXRect const *)frameRect;
+ newFrame:(NXRect *)r :(char *)left :(char *)right;
- free;
- target;
- setTarget:anObject;
- (SEL) action;
- setAction:(SEL) aSelector;
- setLeftTitle:(char *)aString;
- setRightTitle:(char *)aString;
- (const char *)icon;
- setIcon:(char const *)name;
- (const char *)altIcon;
- setAltIcon:(char const *)name;
- setFont:fontObj;
- (unsigned short)keyEquivalent;
- setKeyEquivalent:(unsigned short)charCode;
- setEnabled:(BOOL)value;
- setState:(int)value;
- (int)state;
- (int)intValue;
- setIntValue:(int)anInt;
- sizeTo:(NXCoord)width :(NXCoord)height;
- sizeToFit;
- redraw;
- drawSelf:(NXRect *) rects :(int)rectCount;
- (BOOL)_mouseHit :(NXPoint *) basePoint;
- mouseDown:(NXEvent *) theEvent;
- (BOOL)performKeyEquivalent :(NXEvent *) theEvent;
- performClick:sender;
- write:(NXTypedStream *) stream ;
- read:(NXTypedStream *) stream ;
@end

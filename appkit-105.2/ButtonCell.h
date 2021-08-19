/*
	ButtonCell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "ActionCell.h"

/* Button Types */

#define NX_MOMENTARYPUSH	0
#define NX_PUSHONPUSHOFF	1
#define NX_TOGGLE		2
#define NX_SWITCH		3
#define NX_RADIOBUTTON		4
#define NX_MOMENTARYCHANGE	5

@interface ButtonCell : ActionCell
{
    char               *altContents;
    union _icon {
	struct _bmap {
	    id                  normal;
	    id                  alternate;
	}                   bmap;
	struct _ke {
	    id                  font;
	    float               descent;
	}                   ke;
    }                   icon;
    id                  sound;
    struct _bcFlags1 {
	unsigned int        pushIn:1;
	unsigned int        changeContents:1;
	unsigned int        changeBackground:1;
	unsigned int        changeGray:1;
	unsigned int        lightByContents:1;
	unsigned int        lightByBackground:1;
	unsigned int        lightByGray:1;
	unsigned int        hasAlpha:1;
	unsigned int        bordered:1;
	unsigned int        iconOverlaps:1;
	unsigned int        horizontal:1;
	unsigned int        bottomOrLeft:1;
	unsigned int        iconAndText:1;
	unsigned int        lastState:1;
	unsigned int        iconSizeDiff:1;
	unsigned int        iconIsKeyEquivalent:1;
    }                   bcFlags1;
    struct _bcFlags2 {
	unsigned int        keyEquivalent:8;
	unsigned int        transparent:1;
	unsigned int        _RESERVED:6;
	unsigned int        _momentarySound:1;
    }                   bcFlags2;
    unsigned short      periodicDelay;
    unsigned short      periodicInterval;
}

- init;
- initTextCell:(const char *)aString;
- initIconCell:(const char *)iconName;

- copyFromZone:(NXZone *)zone;

- free;
- (const char *)title;
- setTitle:(const char *)aString;
- setTitleNoCopy:(const char *)aString;
- (const char *)altTitle;
- setAltTitle:(const char *)aString;
- (const char *)icon;
- setIcon:(const char *)iconName;
- (const char *)altIcon;
- setAltIcon:(const char *)iconName;
- image;
- setImage:image;
- altImage;
- setAltImage:image;
- (int)iconPosition;
- setIconPosition:(int)aPosition;
- sound;
- setSound:aSound;
- (int)highlightsBy;
- setHighlightsBy:(int)aType;
- (int)showsStateBy;
- setShowsStateBy:(int)aType;
- setType:(int)aType;
- (BOOL)isOpaque;
- (const char *)stringValue;
- setStringValue:(const char *)aString;
- setStringValueNoCopy:(const char *)aString;
- (int)intValue;
- setIntValue:(int)anInt;
- (float)floatValue;
- setFloatValue:(float)aFloat;
- (double)doubleValue;
- setDoubleValue:(double)aDouble;
- setFont:fontObj;
- (BOOL)isBordered;
- setBordered:(BOOL)flag;
- (BOOL)isTransparent;
- setTransparent:(BOOL)flag;
- setPeriodicDelay:(float)delay andInterval:(float)interval;
- getPeriodicDelay:(float *)delay andInterval:(float *)interval;
- (unsigned short)keyEquivalent;
- setKeyEquivalent:(unsigned short)charCode;
- setKeyEquivalentFont:fontObj;
- setKeyEquivalentFont:(const char *)fontName size:(float)fontSize;
- (int)getParameter:(int)aParameter;
- setParameter:(int)aParameter to:(int)value;
- getDrawRect:(NXRect *)theRect;
- getTitleRect:(NXRect *)theRect;
- getIconRect:(NXRect *)theRect;
- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- drawInside:(const NXRect *)aRect inView:controlView;
- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag;
- (BOOL)trackMouse:(NXEvent *)theEvent inRect:(const NXRect *)cellFrame ofView:controlView;
- performClick:sender;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newTextCell;
+ newTextCell:(const char *)aString;
+ newIconCell;
+ newIconCell:(const char *)iconName;

@end

@interface Object(SoundKitMethods)
- (int)play;
- (int)stop;
@end

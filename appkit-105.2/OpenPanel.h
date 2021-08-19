/*
	OpenPanel.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "SavePanel.h"
#import <sys/stat.h>

/* Tags of views in the OpenPanel */

#define NX_OPICONBUTTON		NX_SPICONBUTTON
#define NX_OPTITLEFIELD		NX_SPTITLEFIELD
#define NX_OPCANCELBUTTON	NX_SPCANCELBUTTON
#define NX_OPOKBUTTON		NX_SPOKBUTTON
#define NX_OPFORM		NX_SPFORM

@interface OpenPanel : SavePanel
{
    void	       *_reservedPtr0;
    char              **filterTypes;
    unsigned int        _reservedOPint1;
    unsigned int        _reservedOPint2;
}

+ new;
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;

- free;
- allowMultipleFiles:(BOOL)flag;
- (const char *const *)filenames;
- (int)runModalForDirectory:(const char *)path file:(const char *)name types:(const char *const *)fileTypes;
- (int)runModalForTypes:(const char *const *)fileTypes;
- (int)runModalForDirectory:(const char *)path file:(const char *)name;

@end

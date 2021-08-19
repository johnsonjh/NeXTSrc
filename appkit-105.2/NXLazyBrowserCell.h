/*

	NXLazyBrowserCell.h
  	Copyright 1990, NeXT, Inc.
	Responsibility: Paul Hegarty
	  
	DEFINED IN: The Application Kit
	HEADER FILES: appkit.h
*/

#import "NXBrowserCell.h"

@interface NXLazyBrowserCell : NXBrowserCell

+ new;
+ allocFromZone:(NXZone *)zone;
+ alloc;

- free;
- (BOOL)isLoaded;

@end

/*

	NXFaxText.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Chris Franklin
	  
	DEFINED IN: The Application Kit
	HEADER FILES: NXFaxText.h
*/

#import "Text.h"

@interface NXFaxText:Text

- selectFirstField;
- setField : (const char *) name to : (char *) value;
- setDate : (int) theTime;
- setPages : (int) thePages;
- setTo : (char *) recipient;
- setFrom : (char *) sender;
@end


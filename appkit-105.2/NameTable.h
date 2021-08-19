/*

	NameTable.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#import <objc/List.h>

@interface NameTable : List
{
    int		hashSize;
}

+ new;
+ newHashSize:(int)theSize;
- init;
- initHashSize:(int)theSize;
- free;
- addName:(const char *)theName owner:theOwner forObject:theObject;
- getObjectForName:(const char *)theName owner:theOwner;
- (const char *) getObjectName:theObject andOwner:(id *)theOwner;
- removeName:(const char *)theName owner:theOwner;
- removeObject:theObject;
@end

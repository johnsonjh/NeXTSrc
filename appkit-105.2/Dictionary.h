#import <objc/Object.h>
#import <streams/streams.h>
#import "dict.h"
#import <objc/hashtable.h>
#import <stdio.h>

@interface Dictionary:Object
{
@public
    unsigned char *tables[NDICTS];
    int filesize;
    char *sourceName;
}

- (BOOL) findMisspelled:(char *)text length:(int)length    info:info loc1:(int *) loc1 loc2:(int *) loc2;
+ newResource:(const char *) resourceDir;
@end


extern void lowerit(char *s);
extern int guess(const char *string, id info);






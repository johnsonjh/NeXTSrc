/*
    NXStringTable.h
    Copyright 1990 NeXT, Inc.
	
    DEFINED AS: A common class
    HEADER FILES: objc/NXStringTable.h

*/

#import "HashTable.h"

#define MAX_NXSTRINGTABLE_LENGTH	1024

@interface NXStringTable: HashTable

- init;
- free;
    
- (const char *)valueForStringKey:(const char *)aString;
    
- readFromStream:(NXStream *)stream;
- readFromFile:(const char *)fileName;

- writeToStream:(NXStream *)stream;
- writeToFile:(const char *)fileName;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init method defined in this class to create an NXStringTable and
 * the readFrom... methods to fill it with data.
 */

+ new;
+ newFromStream:(NXStream *)stream;
+ newFromFile:(const char *)fileName;

@end

static inline const char *NXSTR(NXStringTable *table, const char *key) {
    return [table valueForStringKey:key];
}

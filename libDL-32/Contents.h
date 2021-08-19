#import <appkit/Application.h>

@interface Contents : Object
{
	id window, splitView, summary, fileText;
	char *infoPath;
}

- print:sender;
- setFile:(char *)s ;
- nthFile:(int)n ;
- (int) nextFile ;
- (int) prevFile ;
- (int) setFilesFromIndex:(char *)str ;
- setFiles:(char *)str;
- search:sender ;
- (int)findText:(char *) str forward:(BOOL)fwd ;
- contentsText ;
- window ;
+ show:(char *)path ;
@end

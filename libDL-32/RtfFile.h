@interface RtfFile : Object
{
	id window, splitView, fileText;
}

- print:sender;
- search:(id)sender ;
- setFile:(char *)s ;
- contentsText ;
+ new;
+ showFile:(char *)path ;
@end

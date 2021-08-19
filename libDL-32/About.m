#import <appkit/appkit.h>
#import <string.h>
#import "About.h"
#import "aux.h"

extern char 	VERS_NUM[];
extern char 	ReleaseNumber[];

@implementation About;

- setVersionStr:anObject {versionStr = anObject;}
- setPanel:anObject {panel = anObject;}

- setVersion:(char *)str {
	char buf[32], *p;

	p = index(str, 'i');
	if (!p) p = index(str, 'e');
	if (!p) p = str;
	else	p++;
	sprintf(buf, "Release %s (v%s)", ReleaseNumber, p);
	[versionStr setStringValue:buf];
	return self;
}

+ new {
	Wait();
	self = [super new];

	[NXApp loadNibSection:"About" owner:self];
	[self setVersion:VERS_NUM];
	DoneWaiting();
	return self;
}

+ show {
	static id currObj = nil;

	if (!currObj) {
		currObj = [About new];
	}
	self = currObj;

	[panel center];
	[panel makeKeyAndOrderFront:self];
	return self;
}

@end

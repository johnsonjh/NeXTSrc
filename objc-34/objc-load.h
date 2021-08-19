/*
 *	objc-load.h
 *	Copyright 1988, NeXT, Inc.
 */

#import "objc.h"
#import "objc-class.h"
#import <streams/streams.h>
#import <sys/loader.h>

/* dynamically loading Mach-O object files that contain Objective-C code */

extern long objc_loadModules(
	char *moduleList[], 				/* input */
	NXStream *errorStream,				/* input (optional) */
	void (*loadCallback)(Class, Category),		/* input (optional) */
	struct mach_header **headerAddr,		/* output (optional) */
	char *debugFileName				/* input (optional) */
);

extern long objc_unloadModules(
	NXStream *errorStream,				/* input (optional) */
	void (*unloadCallback)(Class, Category)		/* input (optional) */
);

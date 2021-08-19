
#ifdef	SHLIB
#import	"shlib.h"
#endif

/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  It
 * is padded so that new data can take the place of storage occupied by part
 * of it.
 */

char *NXBTreeFileTemporary = "/tmp\0";
char *NXBTreeFileExtension = "nxbf\0";
char *NXBTreeFileDirectory = "NXBTreeFileDirectory\0";

/* global data padding, must NOT be static */
char _btree_padding[210] = { 0 };

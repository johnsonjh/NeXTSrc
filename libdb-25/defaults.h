/*
	defaults.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

/* typedefs and external functions for the NeXT defaults processing. */

#ifndef DEFAULTS_H
#define DEFAULTS_H

/* globals for command line args */
extern int      NXArgc;
extern char   **NXArgv;

#import <db/db.h>

typedef struct _NXDefault {
  char *name;
  char *value;
} NXDefaultsVector[];


extern int NXRegisterDefaults(const char *owner, const NXDefaultsVector vector); 
extern const char *NXGetDefaultValue(const char *owner, const char *name);
extern int NXSetDefault(const char *owner, const char *name, const char *value);
extern int NXWriteDefault(const char *owner, const char *name, const char *value);
extern int NXWriteDefaults(const char *owner, NXDefaultsVector vector);
extern int NXRemoveDefault(const char *owner, const char *name);
extern const char *NXReadDefault(const char *owner, const char *name);
extern void NXUpdateDefaults(void);
extern const char *NXUpdateDefault(const char *owner, const char *name);
extern const char *NXSetDefaultsUser(const char *newUser);

 /* Low level routines not intended for general use */

extern int NXFilePathSearch(const char *envVarName, const char *path,
	int leftToRight, const char *filename, int (*funcPtr)(),
	void *funcArg);

 /*
  * Used to look down a directory list for one or more files by a
  * certain name.  The directory list is obtained from the given
  * environment variable name, using the given default if not.  If
  * leftToRight is true, the list will be searched left to right;
  * otherwise, right to left.  In each such directory, if the file by the
  * given name can be accessed, then the given function is called with 
  * its first argument as the pathname of the file, and its second 
  * argument as the given value.  If the function returned zero, 
  * filePathSearch will then return with zero. If the function 
  * returned a negative value, filePathSearch will return
  * with the negative value. If the function returns a positive value,
  * filePathSearch will continue to traverse the driectory list and call
  * the function.  If it successfully reaches the end of the list, it
  * returns 0. 
  */
  

extern char *NXGetTempFilename(char *name, int pos);

#endif DEFAULTS_H

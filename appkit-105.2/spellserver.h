/*
	spellserver.h
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto

	Declarations shared between the appkit and ss.
*/

#ifndef SPELLTYPES_H
#define SPELLTYPES_H 1

#import <sys/port.h>
#import <sys/kern_return.h>
#import <stdio.h>

/* rendez-vous names for nmserver */
#define SS_NAME  "NeXT Spelling Server, version 1"
#define SS_VERSION  1

/* errors from the server.  Keep errors.m in sync!! */

#define TIMEOUT 15

/*
 *  These types are unfortunately duplicated in pbs.defs and app.defs.
 */
#define MAX_WORD_LENGTH 256
typedef char *data_t;
typedef char word_t[MAX_WORD_LENGTH];
typedef char path_t[MAX_WORD_LENGTH];


extern void _NXSSError(kern_return_t errorCode);


#endif





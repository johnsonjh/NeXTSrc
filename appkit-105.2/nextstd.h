/*
	nextstd.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

/* This file has some standard macros useful in any program. */

#ifndef NEXTSTD_H
#define NEXTSTD_H

#import <math.h>
#import <stdio.h>
#import <libc.h>

#ifndef MAX
#define  MAX(A,B)	((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define  MIN(A,B)	((A) < (B) ? (A) : (B))
#endif
#ifndef ABS
#define  ABS(A)		((A) < 0 ? (-(A)) : (A))
#endif

#define  NX_MALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) malloc( (unsigned)(NUM)*sizeof(TYPE) )) 

#define  NX_REALLOC( VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) realloc((char *)(VAR), (unsigned)(NUM)*sizeof(TYPE)))

#define  NX_FREE( PTR )	free( (char *) (PTR) );

#define  NX_ZONEMALLOC( Z, VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) NXZoneMalloc((Z), (unsigned)(NUM)*sizeof(TYPE) )) 

#define  NX_ZONEREALLOC( Z, VAR, TYPE, NUM )				\
   ((VAR) = (TYPE *) NXZoneRealloc ( (Z), (char *)(VAR), 		\
   	(unsigned)(NUM)*sizeof(TYPE)))

#ifndef NBITSCHAR
#define NBITSCHAR	8
#endif NBITSCHAR

#ifndef NBITSINT
#define NBITSINT	(sizeof(int)*NBITSCHAR)
#endif NBITSINT

#ifndef TRUE
#define TRUE		1
#endif TRUE
#ifndef FALSE
#define FALSE		0
#endif FALSE

/*
 * compile with -DNX_BLOCKASSERTS to turn off asserts in your code
 */

#ifndef NX_BLOCKASSERTS
extern void NXLogError(const char *format, ...);
#define NX_ASSERT(exp,str)		{if(!(exp)) NXLogError("Assertion failed: %s\n", str);}
#else NX_BLOCKASSERTS
#define NX_ASSERT(exp,str) {}
#endif NX_BLOCKASSERTS

/*
 * Used to insert messages in showps output.
 */

#ifdef DEBUG
#define NX_PSDEBUG()		{if ((DPSGetCurrentContext())->chainChild) DPSPrintf( DPSGetCurrentContext(),"\n%% *** Debug *** Object:%d Class:%s Method:%s\n", ((int)self), ((char *)[self name]), SELNAME(_cmd));}
#else DEBUG
#define NX_PSDEBUG() {}
#endif DEBUG

#endif

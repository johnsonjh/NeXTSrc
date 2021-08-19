/*
 *	objc.h
 *	Copyright 1988, NeXT, Inc.
 */

#import "objc.h"

#define GETFRAME(firstArg)	((MSG)&(((MSG*)(&firstArg))[-2/*-3*/]))
#define PREVSELF(firstArg)	(GETFRAME(firstArg)->next->_xxARGS.receiver)
#define PREVSEL(firstArg)	(GETFRAME(firstArg)->next->_xxARGS.cmd)

/* Data-type alignment constants.  Usage is:  while(addr&alignment)addr++ */
#define _A_CHAR		0x0
#define _A_SHORT	0x1
#define _A_INT		0x1
#define _A_LONG		0x1
#define _A_FLOAT	0x1
#define _A_DOUBLE	0x1
#define _A_POINTER	0x1
#define NBITS_CHAR	8
#define NBITS_INT	(sizeof(int)*NBITS_CHAR)

/* Stack Frame layouts */

typedef struct _MSG {			/* Stack frame layout (for messages) */
	struct _MSG *next;		/* Link to next frame */
	/*void *saved_a1;		    GNU uses this for struct returns */
	id (*ret)();			/* Return address from subroutine */
	struct _ARGFRAME {
		id receiver;		/* ID of receiver */
		char *cmd;		/* Message selector */
		int arg;		/* First message argument */
	} _xxARGS;
} *MSG;

typedef struct _MSGSUPER {
	struct _MSG *next;		/* Link to next frame */
	/*void *saved_a1;		    GNU uses this for struct returns */
	id (*ret)();			/* Return address from subroutine */
	struct {
		struct objc_super *caller;
		char *cmd;		/* Message selector */
		int arg;		/* First message argument */
	} _xxARGS;
} *MSGSUPER;

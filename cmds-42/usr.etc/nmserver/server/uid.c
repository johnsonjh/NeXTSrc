/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * uid.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/uid.c,v $
 *
 */
#ifndef	lint
char uid_rcsid[] = "$Header: uid.c,v 1.1 88/09/30 15:42:55 osdev Exp $";
#endif not lint
/*
 * Generates locally unique identifiers.
 */

/*
 * HISTORY:
 *  3-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#define DEBUGOFF 1

#include <cthreads.h>
#include <sys/boolean.h>
#include <sys/time.h>
#include <sys/types.h>

#include "netmsg.h"
#include "uid.h"

extern long	random();
extern char	*initstate();

#define STATE_SIZE	256
static char		random_state[STATE_SIZE];



/*
 * uid_init
 *
 * Initialises random number generator in order to produce unique identifiers.
 *
 */
EXPORT boolean_t uid_init()
BEGIN("uid_init")
    struct timeval tp;
    struct timezone tzp;

    (void)gettimeofday(&tp, &tzp);

    (void)initstate((unsigned int)tp.tv_usec, (char *)random_state, STATE_SIZE);
    
    RETURN(TRUE);
END


/*
 * uid_get_new_uid
 *
 * Returns a new unique identifier from the random number generator.
 *
 */
EXPORT long uid_get_new_uid()
BEGIN("uid_get_new_uid")
    long new_uid;
    new_uid = (long)random();
    RETURN(new_uid);
END

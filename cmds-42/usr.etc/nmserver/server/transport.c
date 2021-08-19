/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * transport.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/transport.c,v $
 *
 */

#ifndef	lint
char transport_rcsid[] = "$Header: transport.c,v 1.1 88/09/30 15:42:48 osdev Exp $";
#endif not lint

/*
 * Generic transport stuff.
 */

/*
 * HISTORY:
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  7-Dec-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Minor syntax errors when NET_TRACE is off.
 *
 *  2-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added a no-op entry for operation with no network.
 *
 * 25-Sep-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added an initialization routine, along with code to catch
 *	illegal transport requests. Use TR_MAX_ENTRY for size of table.
 *
 * 24-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include "netmsg.h"
#include "nm_defs.h"
#include "transport.h"

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_TRBUFF;


EXPORT transport_sw_entry_t	transport_switch[TR_MAX_ENTRY];


/*
 * transport_no_function --
 *
 * Default function for inexistant transport calls.
 *
 * Parameters:
 *
 * Results:
 *
 * TR_FAILURE
 *
 * Side effects:
 *
 * Prints an error message.
 *
 * Design:
 *
 * Note:
 *
 */
EXPORT int transport_no_function()
BEGIN("transport_no_function")

	ERROR((msg,"** Transport: invalid function **"));
	RETURN(TR_FAILURE);
END



/*
 * transport_noop_send --
 *
 * Function for dummy send operation.
 *
 * Parameters:
 *
 * Results:
 *
 * TR_SUCCESS
 *
 * Side effects:
 *
 * Design:
 *
 * Note:
 *
 */
EXPORT int transport_noop_send()
BEGIN("transport_noop_send")

	RETURN(TR_SUCCESS);
END



/*
 * transport_init --
 *	Initialise the transport module.
 *
 * Results:
 *	TRUE or FALSE
 *
 * Design:
 *	Set up the transport_switch.
 *
 */
EXPORT boolean_t transport_init()
BEGIN("transport_init")
    int	i;

    /*
     * Initialize the memory management facilities.
     */
    mem_initobj(&MEM_TRBUFF,"Transport buffer",2000,FALSE,16,4);

    /*
     * Initialize the transport switch.
     */
    for (i = 0; i < TR_MAX_ENTRY; i++) {
	transport_switch[i].send = transport_no_function;
#if	RPCMOD
	transport_switch[i].sendrequest = transport_no_function;
	transport_switch[i].sendreply = transport_no_function;
#endif	RPCMOD
    }

    transport_switch[TR_NOOP_ENTRY].send = transport_noop_send;

    RETURN(TRUE);

END



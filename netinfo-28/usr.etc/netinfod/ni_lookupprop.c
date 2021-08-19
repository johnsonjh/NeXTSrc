/*
 * ni_lookupprop() implementation
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * This function is a copy of the one in the NetInfo client library. 
 * It is duplicated here since we do not wish to pull in any code from
 * the client library.
 */
#include "ni_server.h"
#include <stdio.h>
#include "clib.h"
#include "mm.h"

/*
 * We can do this without an addition to the protocol
 */
ni_status
ni_lookupprop(
	      void *ni,
	      ni_id *id,
	      ni_name_const pname,
	      ni_namelist *nl
	      )
{
	ni_status status;
	ni_namelist list;
	ni_index which;
	
	status = ni_listprops(ni, id, &list);
	if (status != NI_OK) {
		return (status);
	}
	which = ni_namelist_match(list, pname);
	ni_namelist_free(&list);
	if (which == NI_INDEX_NULL) {
		return (NI_NOPROP);
	}
	return (ni_readprop(ni, id, which, nl));
}


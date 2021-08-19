/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * km_utils.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/km_utils.c,v $
 *
 */

#ifndef	lint
char km_utils_rcsid[] = "$Header: km_utils.c,v 1.1 88/09/30 15:40:01 osdev Exp $";
#endif not lint

/*
 * Utility functions for the Key Management module.
 */

/*
 * HISTORY:
 * 31-May-88 Mary Thompson (mrt) at Carnegie Mellon
 *	Removed \ escapes in macro invocations because
 *	they did not work on other machines. MEM_ALLOC
 *	macros has been changed.
 *
 * 24-May-88 Daniel Julin (dpj) at Carnegie Mellon 
 *	Added \ escapes before newline in invocations of
 *	macros so that it would compile on the MMAX
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 28-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Replaced fprintf by ERROR and DEBUG/LOG macros.
 *	Statically allocate km_hash_table_lock.
 *
 *  9-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include <cthreads.h>

#include "debug.h"
#include "key_defs.h"
#include "km_defs.h"
#include "mem.h"
#include "netmsg.h"
#include "nm_defs.h"
#include "nm_extra.h"

#define INIT_HASH_TABLE_SIZE	32	/* Should be multiple of 2. */
#define INIT_HASH_TABLE_LIMIT	25
#define HASH_TABLE_ENTRY_SIZE	(sizeof(key_rec_t))

static key_rec_ptr_t	km_hash_table;
static int		km_hash_table_size = INIT_HASH_TABLE_SIZE;
static int		km_hash_table_limit = INIT_HASH_TABLE_LIMIT;
static int		km_hash_table_num_entries = 0;
static struct mutex	km_hash_table_lock;

#define HASH_HOST(host_id) ((host_id & (km_hash_table_size - 1)))


/*
 * km_utils_init
 *	Initialises the host hash table and lock.
 *
 */
PUBLIC void km_utils_init()
BEGIN("km_utils_init")

    MEM_ALLOC(km_hash_table,key_rec_ptr_t,	
			(INIT_HASH_TABLE_SIZE*HASH_TABLE_ENTRY_SIZE), FALSE);

    mutex_init(&km_hash_table_lock);

END


/*
 * km_grow_hash_table
 *	doubles the size of the hash table.
 *
 * Side effects:
 *	a new hash table with the entries from the old one rehashed into it
 *
 * Design:
 *	Create a new hash table that is twice as big as the old one.
 *	Rehash the entries from the old table into the new one.
 *	Delete the old table.
 *
 * Note:
 *	We are responsible for our own locking;
 *	(assume that caller has not left the hash table locked).
 *
 */
PRIVATE void km_grow_hash_table()
BEGIN("km_grow_hash_table")
    int			old_size, new_size, i;
    key_rec_ptr_t	new_hash_table, old_hash_table;

    mutex_lock(&km_hash_table_lock);

    old_size = km_hash_table_size;
    new_size = km_hash_table_size = 2 * old_size;
    km_hash_table_limit *= 2;

    /*
     * Now create and initialise the new hash table.
     */
    old_hash_table = km_hash_table;
    MEM_ALLOC(new_hash_table,key_rec_ptr_t,	
				(new_size * HASH_TABLE_ENTRY_SIZE), FALSE);
    km_hash_table = new_hash_table;

    /*
     * Rehash the entries from the old hash table into the new one.
     */
    for (i = 0; i < old_size; i++) {
	int	index;

	index = HASH_HOST(old_hash_table[i].kr_host_id);
	while (new_hash_table[index].kr_host_id != 0) {
	    index ++;
	    index %= new_size;
	}
	new_hash_table[index] = old_hash_table[i];
    }

    mutex_unlock(&km_hash_table_lock);
    MEM_DEALLOC((pointer_t)old_hash_table, (old_size * HASH_TABLE_ENTRY_SIZE));
    RET;

END


/*
 * km_host_enter
 *
 * Parameters:
 *	host_id	: the host to create a new record for in the hash table
 *
 * Results:
 *	A pointer to the record for this host.
 *
 * Notes:
 *	We must lock the hash table for mutual exclusion.
 *	If the host already exists in the hash table,
 *	then just return a pointer to its record.
 *
 */
PUBLIC key_rec_ptr_t km_host_enter(host_id)
netaddr_t host_id;
BEGIN("km_host_enter")
    int index, original_index;

    mutex_lock(&km_hash_table_lock);

    km_hash_table_num_entries ++;
    if (km_hash_table_num_entries > km_hash_table_limit) {
	mutex_unlock(&km_hash_table_lock);
	km_grow_hash_table();
	mutex_lock(&km_hash_table_lock);
    }

    original_index = index = HASH_HOST(host_id);
    while ((km_hash_table[index].kr_host_id != 0)
		&& (km_hash_table[index].kr_host_id != host_id))
    {
	index ++;
	index %= km_hash_table_size;
	if (index == original_index) {
	    mutex_unlock(&km_hash_table_lock);
	    LOG0(TRUE, 3, 1150);
	    km_grow_hash_table();
	    /*
	     * Retry this call to enter.
	     */
	    RETURN(km_host_enter(host_id));
	}
    }

    if (km_hash_table[index].kr_host_id != 0) {
	mutex_unlock(&km_hash_table_lock);
	LOG0(TRUE, 0, 1151);
	RETURN(&(km_hash_table[index]));
    }

    /*
     * Otherwise fill in this record.
     */
    km_hash_table[index].kr_host_id = host_id;

    mutex_unlock(&km_hash_table_lock);
    RETURN(&(km_hash_table[index]));

END



/*
 * km_host_lookup
 *
 * Parameters:
 *	host_id	: the host to look up
 *
 * Results:
 *	A pointer to the record for this host (0 if not found).
 *
 * Notes:
 *	Must lock the hash table for mutual exclusion.
 *
 */
PUBLIC key_rec_ptr_t km_host_lookup(host_id)
netaddr_t host_id;
BEGIN("km_host_lookup")
    int			index, original_index;
    key_rec_ptr_t	result;

    original_index = index = HASH_HOST(host_id);

    mutex_lock(&km_hash_table_lock);
    while ((km_hash_table[index].kr_host_id != 0)
		&& (km_hash_table[index].kr_host_id != host_id))
    {
	index ++;
	index %= km_hash_table_size;
	if (index == original_index) {
	    mutex_unlock(&km_hash_table_lock);
	    panic("km_host_lookup: hash table full");
	}
    }

    if (km_hash_table[index].kr_host_id == host_id) result = &(km_hash_table[index]);
    else result = (key_rec_ptr_t)0;

    mutex_unlock(&km_hash_table_lock);
    RETURN(result);

END


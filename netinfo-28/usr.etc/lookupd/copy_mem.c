/*
 * memory-memory copy
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <stdio.h>
#include "copy_mem.h"
#include "clib.h"

#ifndef roundup
#define roundup(x, y) ((((x) + (y) + 1) / (y)) * (y))
#endif

int
copy_mem(
	 char *src,
	 char **dst,
	 int dst_offset,
	 int src_bytes
	 )
{
	int used;
	char *newdst;
	int newsize;
	kern_return_t ret;

	used = dst_offset % vm_page_size;
	if (used == 0) {
		/*
		 * We used the whole page
		 */
		used = vm_page_size;
	}
	if (used + src_bytes <= vm_page_size) {
		bcopy(src, *dst + dst_offset, src_bytes);
		return (1);
	}
	newsize = roundup(dst_offset + src_bytes, vm_page_size);
	ret = vm_allocate(task_self(), (vm_address_t *)&newdst, newsize, TRUE);
	if (ret != KERN_SUCCESS) {
		debug2("vm_allocate:", mach_errormsg(ret));
		return (0);
	}
	ret = vm_copy(task_self(), (vm_address_t)*dst, 
		      newsize - vm_page_size, (vm_address_t)newdst);
	if (ret != KERN_SUCCESS) {
		(void)vm_deallocate(task_self(), (vm_address_t)newdst, 
				    newsize);
		debug2("vm_copy:", mach_errormsg(ret));
		return (0);
	}
	(void)vm_deallocate(task_self(), (vm_address_t)*dst,
			    newsize - vm_page_size);
	*dst = newdst;
	bcopy(src, *dst + dst_offset, src_bytes);
	return (1);
}


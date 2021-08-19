/*
 *	File:	sbrk.c
 *
 *	Unix compatibility for sbrk system call.
 *
 * HISTORY
 * 09-Mar-90  Gregg Kellogg (gk) at NeXT.
 *	include <kern/mach_interface.h> instead of <kern/mach.h>
 *
 * 14-Feb-89  Avadis Tevanian (avie) at NeXT.
 *	Total rewrite using a fixed area of VM from break region.
 */

#include <kern/mach_interface.h>/* for vm_allocate, vm_offset_t */
#include <sys/types.h>		/* for caddr_t */

static int sbrk_needs_init = TRUE;
static vm_size_t sbrk_region_size = 4*1024*1024; /* Well, what should it be? */
static vm_address_t sbrk_curbrk;

caddr_t sbrk(size)
	int	size;
{
	vm_offset_t	addr;
	kern_return_t	ret;
	caddr_t		ocurbrk;
	extern int	end;

	if (sbrk_needs_init) {
		sbrk_needs_init = FALSE;
		/*
		 *	Allocate a big region to simulate break region.
		 */
		ret =  vm_allocate(task_self(), &sbrk_curbrk, sbrk_region_size,
				  TRUE);
		if (ret != KERN_SUCCESS)
			return((caddr_t)-1);
	}
	
	if (size <= 0)
		return((caddr_t)sbrk_curbrk);
	sbrk_curbrk += size;
	sbrk_region_size -= size;
	if (sbrk_region_size < 0)
		return((caddr_t)-1);
	return((caddr_t)(sbrk_curbrk - size));
}

caddr_t brk(x)
	caddr_t x;
{
	return((caddr_t)-1);
}


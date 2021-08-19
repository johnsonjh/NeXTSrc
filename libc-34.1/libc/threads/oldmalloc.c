#include <cthreads.h>
#include "cthread_internals.h"

/*
 * C library imports:
 */
extern bcopy();

/*
 * Structure of memory block header.
 * When free, next points to next block on free list.
 * When allocated, fl points to free list.
 * Size of header is 4 bytes, so minimum usable block size is 8 bytes.
 */
typedef union header {
	union header *next;
	struct free_list *fl;
} *header_t;

#define MIN_SIZE	8	/* minimum block size */

typedef struct free_list {
	int lock;	/* spin lock for mutual exclusion */
	header_t head;	/* head of free list for this size */
#ifdef	DEBUG
	int in_use;	/* # mallocs - # frees */
#endif	DEBUG
} *free_list_t;

/*
 * Free list with index i contains blocks of size 2^(i+3) including header.
 * Smallest block size is 8, with 4 bytes available to user.
 * Size argument to malloc is a signed integer for sanity checking,
 * so largest block size is 2^31.
 */
#define NBUCKETS	29

static struct free_list free_list[NBUCKETS];

#if	MTASK
#define DEFAULT_N_SHARED_PAGES	128
int n_shared_pages = 0;
#endif	MTASK

static void
more_memory(size, fl)
	int size;
	register free_list_t fl;
{
	register int amount;
	register int n;
	vm_address_t where;
	register header_t h;
	kern_return_t r;

#if	MTASK
	/*
	 * To simulate threads with multiple tasks,
	 * we pre-allocate a large pool of shared memory
	 * and allocate from it. This is unsatisfactory,
	 * but there is no easy way for a child to allocate
	 * memory that can be shared by its parent.
	 * Updating the free pointer (next_shared_page) is a
	 * critical section, protected by the page_lock semaphore.
	 */
	static vm_address_t shared_pages = 0;
	static vm_address_t next_shared_page = 0;
	static int page_lock = 0;			/* unlocked */
	extern char etext[], end[];

	if (shared_pages == 0) {
		/*
		 * Make existing global data shared.
		 */
		MACH_CALL(vm_inherit(task_self(),
				     (vm_address_t) etext,
				     (vm_size_t) (end - etext),
				     VM_INHERIT_SHARE),
			  r);
		/*
		 * Allocate pool of shared pages.
		 */
		if (n_shared_pages <= 0)
			n_shared_pages = DEFAULT_N_SHARED_PAGES;
		MACH_CALL(vm_allocate(task_self(),
				      &shared_pages,
				      (vm_size_t) (n_shared_pages * vm_page_size),
				      TRUE),
			  r);
		MACH_CALL(vm_inherit(task_self(),
				     shared_pages,
				     n_shared_pages * vm_page_size,
				     VM_INHERIT_SHARE),
			  r);
	}
#endif	MTASK

	if (size <= vm_page_size) {
		amount = vm_page_size;
		n = vm_page_size / size;
		/*
		 * We lose vm_page_size - n*size bytes here.
		 */
	} else {
		amount = size;
		n = 1;
	}
#if	MTASK
	spin_lock(&page_lock);
	ASSERT(next_shared_page <= n_shared_pages);
	where = shared_pages + (next_shared_page * vm_page_size);
	next_shared_page += (amount + vm_page_size - 1) / vm_page_size;
	if (next_shared_page >= n_shared_pages) {
		next_shared_page = n_shared_pages;
		spin_unlock(&page_lock);
		return;
	}
	spin_unlock(&page_lock);
#else
	MACH_CALL(vm_allocate(task_self(), &where, (vm_size_t) amount, TRUE), r);
#endif	MTASK
	h = (header_t) where;
	do {
		h->next = fl->head;
		fl->head = h;
		h = (header_t) ((char *) h + size);
	} while (--n != 0);
}

char *
malloc(size)
	register unsigned int size;
{
	register int i, n;
	register free_list_t fl;
	register header_t h;

	if ((int) size <= 0)		/* sanity check */
		return 0;
	size += sizeof(union header);
	i = 0;
	n = MIN_SIZE;
	while (n < size) {
		i += 1;
		n <<= 1;
	}
	ASSERT(i < NBUCKETS);
	fl = &free_list[i];
	spin_lock(&fl->lock);
	h = fl->head;
	if (h == 0) {
		more_memory(n, fl);
		h = fl->head;
		if (h == 0) {
			spin_unlock(&fl->lock);
			return 0;
		}
	}
	fl->head = h->next;
#ifdef	DEBUG
	fl->in_use += 1;
#endif	DEBUG
	spin_unlock(&fl->lock);
	h->fl = fl;
	return ((char *) h) + sizeof(union header);
}

#ifdef	hc
#define	RETURN	return 0
#else
#define	RETURN	return
#endif	hc

free(base)
	char *base;
{
	register header_t h;
	register free_list_t fl;
	register int i;

	if (base == 0)
		RETURN;
	h = (header_t) (base - sizeof(union header));
	fl = h->fl;
	i = fl - free_list;
	if (i < 0 || i >= NBUCKETS) {
		ASSERT(0 <= i && i < NBUCKETS);
		RETURN;
	}
	if (fl != &free_list[i]) {
		ASSERT(fl == &free_list[i]);
		RETURN;
	}
	spin_lock(&fl->lock);
	h->next = fl->head;
	fl->head = h;
#ifdef	DEBUG
	fl->in_use -= 1;
#endif	DEBUG
	spin_unlock(&fl->lock);
	RETURN;
}

char *
realloc(ptr, size)
	char *ptr;
	unsigned int size;
{
	header_t h;
	free_list_t fl;
	unsigned int oldsize, bucket;
	char *new;

	h = (header_t) (ptr - sizeof(union header));
	fl = h->fl;
	bucket = h->fl - free_list;
	if (bucket < 0 || bucket >= NBUCKETS) {
		ASSERT(0 <= bucket && bucket < NBUCKETS);
		RETURN;
	}
	if (fl != &free_list[bucket]) {
		ASSERT(fl == &free_list[bucket]);
		RETURN;
	}
	oldsize = MIN_SIZE << bucket;
	oldsize -= sizeof(union header);

	if (oldsize >= size) return ptr;
	new = malloc(size);
	size = oldsize;

	bcopy(ptr, new, (int) size);
	free(ptr);
	return new;
}

#undef	RETURN

#ifdef	DEBUG
void
print_malloc_free_list()
{
  	register int i, size;
	register free_list_t fl;
	register int n;
  	register header_t h;
	int total_used = 0;
	int total_free = 0;

	fprintf(stderr, "      Size     In Use       Free      Total\n");
  	for (i = 0, size = MIN_SIZE, fl = free_list;
	     i < NBUCKETS;
	     i += 1, size <<= 1, fl += 1) {
		spin_lock(&fl->lock);
		if (fl->in_use != 0 || fl->head != 0) {
			total_used += fl->in_use * size;
			for (n = 0, h = fl->head; h != 0; h = h->next, n += 1)
				;
			total_free += n * size;
			fprintf(stderr, "%10d %10d %10d %10d\n",
				size, fl->in_use, n, fl->in_use + n);
		}
		spin_unlock(&fl->lock);
  	}
  	fprintf(stderr, " all sizes %10d %10d %10d\n",
		total_used, total_free, total_used + total_free);
}
#endif	DEBUG

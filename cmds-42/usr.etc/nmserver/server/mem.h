/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * mem.h 
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/mem.h,v $ 
 *
 * $Header: mem.h,v 1.1 88/09/30 15:44:13 osdev Exp $ 
 *
 */

/*
 * Definitions for the memory management module. 
 */

/*
 * HISTORY: 
 * 09-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Changed MEM_ALLOCOBJ to also use calloc.
 *
 * 01-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Changed MEM_ALLOC to always use calloc under the assumption
 *	that a good memory allocator is provided (we have one such).
 *	In the presence of a bogus 4.3 allocator wasted space is
 *	only virtual, so why both with the extra code.
 *
 * 22-Aug-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added list_{first,last,next,prev} to find all buckets in mem_list().
 *
 * 31-May-88 Mary R. Thompson (mrt) @ Carnegie Mellon
 *	Renamed size parameter to MEM_ALLOC and MEM_DEALLOC to avoid bug
 *	in MMAX preprocessor.
 *
 *  3-Apr-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Fixed the comments to describe the current mode of operation.
 *
 * 28-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified to allow type casting.
 *
 * 17-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_MEM_
#define	_MEM_

#include <cthreads.h>
#include "debug.h"
#include "ls_defs.h"

#define	MEM_DEBUG_LO	debug.mem
#define	MEM_DEBUG_HI	(debug.mem & 0x2)


#if	GOOD_MALLOC
#else	GOOD_MALLOC
/*
 * Object bucket information.
 */
typedef struct mem_objbucket {
	int			num;		/* number of free objects */
	pointer_t		freehead;	/* next free object */
	struct mem_objbucket	*next;		/* bucket chain */
	struct mem_objbucket	*prev;		/* bucket chain */
	boolean_t		avail;		/* TRUE if on allocation chain */
	pointer_t		info;		/* back pointer to head */
	struct mem_objbucket	*list_next;	/* chain of all buckets */
	struct mem_objbucket	*list_prev;	/* chain of all buckets */
} mem_objbucket_t, *mem_objbucket_ptr_t;
#endif	GOOD_MALLOC

/*
 * Record for object allocation chain.
 */
typedef struct mem_objrec {
#if	GOOD_MALLOC
#else	GOOD_MALLOC
	struct mutex	lock;		/* lock for allocation */
#endif	GOOD_MALLOC
	char		*name;		/* name of zone */
	unsigned int	obj_size;	/* size of one object */
#if	GOOD_MALLOC
#else	GOOD_MALLOC
	boolean_t	aligned;	/* objects are page-aligned */
	int		reuse_num;	/* threshold for reuse of a bucket */
	int		full_num;	/* max number of objects per bucket */
	unsigned int	bucket_size;	/* size of one bucket */
	unsigned int	mask;		/* mask to locate bucket info */
	int		cur_buckets;	/* current number of buckets */
	int		max_buckets;	/* hwm for number of buckets */
	mem_objbucket_t	*first;		/* head of free buckets chain */
	mem_objbucket_t	*last;		/* tail of free buckets chain */
	mem_objbucket_t	*list_first;	/* chain of all buckets */
	mem_objbucket_t	*list_last;	/* chain of all buckets */
	struct mem_objrec	*mem_chain;	/* link in chain of objects */
#endif	GOOD_MALLOC
} mem_objrec_t, *mem_objrec_ptr_t;

/*
 * Macros for object allocation.
 */

extern mem_objrec_t	MEM_NNREC;

#if	GOOD_MALLOC
#define MEM_ALLOCOBJ(ret,type,ot) { ret = (type) calloc(1, ot.obj_size); }
#else	GOOD_MALLOC
#define	MEM_ALLOCOBJ(ret,type,ot) {						\
	pointer_t	_fh;							\
										\
	mutex_lock(&(ot).lock);							\
	if ((ot).first) {							\
		DEBUG5(MEM_DEBUG_HI,3,1110,&(ot),(ot).first,(ot).first->num,	\
					(ot).first->freehead,			\
					*(pointer_t *)(ot).first->freehead);	\
		(ot).first->num--;						\
		_fh = (ot).first->freehead;					\
		if (((ot).first->freehead = *(pointer_t *)_fh) == NULL) {	\
			(ot).first->avail = FALSE;				\
			(ot).first = (ot).first->next;				\
			if ((ot).first == NULL) {				\
				(ot).last = NULL;				\
				DEBUG0(MEM_DEBUG_HI,3,1111);			\
			} else {						\
				(ot).first->prev = NULL;			\
				DEBUG3(MEM_DEBUG_HI,3,1112,(ot).first,		\
					(ot).last,(ot).first->freehead);	\
			}							\
		}								\
	} else {								\
		_fh = mem_allocobj_proc(&(ot));					\
	}									\
	mutex_unlock(&(ot).lock);						\
	INCSTAT(mem_allocobjs);							\
	DEBUG3(MEM_DEBUG_LO, 3, 1113,_fh,(ot).first,(ot).cur_buckets);		\
	DEBUG_STRING(MEM_DEBUG_LO, 3, 1114, (ot).name);				\
	ret = (type) _fh;							\
}
#endif	GOOD_MALLOC


#if	GOOD_MALLOC
#define	MEM_DEALLOCOBJ(ptr,ot) free(ptr)
#else	GOOD_MALLOC
#define	MEM_DEALLOCOBJ(ptr,ot) {						\
	mem_objbucket_ptr_t	_ob;						\
										\
	mutex_lock(&(ot).lock);							\
	_ob = (mem_objbucket_ptr_t) (((unsigned int) (ptr)) & (ot).mask);	\
	DEBUG6(MEM_DEBUG_HI, 3, 1115, ptr, _ob, _ob->num, (ot).cur_buckets,	\
						(ot).first,_ob->freehead);	\
	if (_ob->info != (pointer_t)&ot)					\
		panic("MEM_DEALLOCOBJ: wrong object");				\
	*(pointer_t *) (ptr) = _ob->freehead;					\
	_ob->freehead = (pointer_t) (ptr);					\
	_ob->num++;								\
	if (									\
				((_ob->num == (ot).reuse_num)			\
			&&							\
				!(_ob->avail))					\
		||								\
				((_ob->num == (ot).full_num)			\
			&&							\
				(_ob != (ot).first))				\
		) {								\
		mem_deallocobj_proc(&(ot),_ob);					\
	}									\
	mutex_unlock(&(ot).lock);						\
	INCSTAT(mem_deallocobjs);						\
	DEBUG5(MEM_DEBUG_LO,3,1116,ptr,_ob,_ob->num,(ot).first,_ob->freehead);	\
	DEBUG_STRING(MEM_DEBUG_LO, 3, 1114, (ot).name);				\
}
#endif	GOOD_MALLOC

#define	MEM_ALLOC(ptr,type,_size,aligned) {					\
	if (_size <= 0) {							\
		ERROR((msg,"MEM_ALLOC: illegal size: %d", _size));		\
		panic("MEM_ALLOC with negative size");				\
	}									\
										\
	ptr = (type) calloc(1, _size);					\
	if (ptr == NULL) {						\
		ERROR((msg,"MEM_ALLOC.calloc returned NULL, size=%d",	\
			_size));						\
		panic("MEM_ALLOC.calloc");				\
	}								\
	INCSTAT(mem_allocs);							\
	DEBUG2(MEM_DEBUG_LO, 3, 1117, _size, (pointer_t)ptr);			\
}


#define	MEM_DEALLOC(ptr,_size) {							\
	DEBUG2(MEM_DEBUG_HI, 3, 1118, _size, (pointer_t)ptr);			\
										\
	if (_size < 0) {							\
		ERROR((msg,"MEM_ALLOC: illegal size: %d, ptr=0x%x",	\
							_size,ptr));	\
		panic("MEM_DEALLOC with negative size");		\
	}								\
	if (_size > 0) {							\
		free((char *)ptr);					\
	}								\
	INCSTAT(mem_deallocs);							\
	DEBUG2(MEM_DEBUG_LO, 3, 1118, _size, (pointer_t)ptr);			\
}


/*
 * To use the object allocator for objects of type foo_t:
 *
 * Declarations:
 *
 *	extern mem_objrec_t	MEM_FOO;
 *
 * Initialization:
 *
 *	PUBLIC mem_objrec_t	MEM_FOO;
 *	mem_initobj(&MEM_FOO,"foo",sizeof(foo_t),
 *			aligned,full_num,reuse_num);
 *
 */


/*
 * External procedures.
 */
extern boolean_t mem_init();
/*
*/

extern int mem_clean();
/*
*/

extern void mem_initobj();
/*
mem_objrec_ptr_t	or;
char			*name;
unsigned int		obj_size;
boolean_t		aligned;
int			full_num;
int			reuse_num;
*/

extern pointer_t mem_allocobj_proc();
/*
mem_objrec_ptr_t	or;
*/

extern void mem_deallocobj_proc();
/*
mem_objrec_ptr_t	or;
mem_objbucket_ptr_t	ob;
*/

#endif	_MEM_

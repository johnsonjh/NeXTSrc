/*
 *	File:	vmmprint.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young,
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Virtual memory map printing module.
 *
 * HISTORY
 * 30-Oct-87  Mary Thompson (mrt) at Carnegie Mellon
 *	Changed reference to entry.pageable to entry.wired_count
 *	to correspond change in vm_map.h
 *
 * 16-Jul-87  Mary Thompson (mrt) at Carnegie Mellon
 *	Removed printing of vm_object.paging_space in vm_object_print
 *	as it is  no longer exported by vm_object.
 *
 *  4-Jun-87  Mary Thomoson (mrt) at Carnegie Mellon
 *	Defined KERNEL_FEATURES so that the task field of
 *	the struc proc will be defined.
 *
 * 18-May-87  Mary Thompson (mrt) at Carnegie Mellon
 *	Added definition of first_page and vm_page_array
 *
 * 18-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Updated to not use VM_PAGE_TO_PHYS, other random cleanup, fixed
 *	initnlist to null terminal the nlist to prevent nlist() from
 *	trashing the kmem file variable.
 *
 * 20-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Fixed iprintf for long argument lists.
 *
 * 10-Apr-86  Jonathan J. Chew (jjc) at Carnegie-Mellon University
 *	Created using code from "vm_map.c", "vm_object.c", and 
 *	"iprintf.c".
 */
/* sys/proc.h include sys/features.h so ... */
#define KERNEL_FEATURES   

#include <nlist.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/proc.h>
#if	NeXT
#define	EXPORT_BOOLEAN
#include <sys/boolean.h>
#include <kern/task.h>
#else
#include <sys/task.h>
#endif	NeXT
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>

extern char	*strcat();

#define KMEMFILE	"/dev/kmem"
#define NLISTFILE	"/mach"
#define NAMETBLSIZE 	7
#define NOPID		-1
#define PROCBUNCH	16

int	indent;		/* how far to indent in iprintf */

char	*names[NAMETBLSIZE] = {
	"_allproc",
#define X_ALLPROC	0
	"_page_shift",
#define X_PAGE_SHIFT	1
	"_first_page",
#define X_FIRST_PAGE	2
	"_vm_page_array",
#define X_VM_PAGE_ARRAY	3
	""
};

int		page_shift;		/* used by VM_PAGE_TO_PHYS */
struct proc	*allproc;		/* pointer to list pf procs */
vm_page_t	vm_page_array;		/* First resident page in table */
long		first_page;		/* first physical page number */
					/* ... represented in vm_page_array */

initnlist(nl, names)
	struct nlist	nl[];
	char		*names[];
{
	int	i;

	for (i = 0; names[i] != 0 && strcmp(names[i], "") != 0; i++)
		nl[i].n_un.n_name = names[i];
	nl[i].n_un.n_name = 0;
}

getkvars(kmem, nl)
	struct nlist	nl[];
{

	lseek(kmem, (long)(nl[X_ALLPROC].n_value), 0);
	if (read(kmem, (char *)&allproc, sizeof(allproc)) != sizeof(allproc)) {
		printf("getkvars:  can't read no. of processes at 0x%x\n",
		nl[X_ALLPROC].n_value);
		return(1);
	}

	lseek(kmem, (long)nl[X_FIRST_PAGE].n_value, 0);
	if (read(kmem, (char *)&first_page, sizeof(first_page)) !=
			sizeof(first_page))
		printf("getkvars:  can't read first_page at 0x%x\n",
			nl[X_FIRST_PAGE].n_value);

	lseek(kmem, (long)nl[X_PAGE_SHIFT].n_value, 0);
	if (read(kmem, (char *)&page_shift, sizeof(page_shift)) !=
			sizeof(page_shift))
		printf("getkvars:  can't read page_shift at 0x%x\n",
				nl[X_PAGE_SHIFT].n_value);

	lseek(kmem, (long)nl[X_VM_PAGE_ARRAY].n_value, 0);
	if (read(kmem, (char *)&vm_page_array, sizeof(vm_page_array)) !=
			sizeof(vm_page_array))
		printf("getkvars:  can't read vm_page_array at 0x%x\n",
				nl[X_VM_PAGE_ARRAY].n_value);

	return(0);
}

vm_map_t getvmmap(kmem, nl, pid)
	int		kmem;
	struct nlist	nl[];
	int		pid;
{
	int		i;
	struct proc	aproc;
	struct proc	*procptr;
	struct task	task;
	task_t		taskptr;

	procptr = allproc;
	while (procptr) {
		lseek(kmem, (long)procptr, 0);
		if (read(kmem, (char *)&aproc, sizeof(struct proc)) !=
				sizeof(struct proc)) {
			printf("getvmmap:  can't read proc struct at 0x%x\n",
					procptr);
			return(0);
		}
		if (aproc.p_pid != pid)
			procptr = aproc.p_nxt;
		else
			break;
	}
	if (!procptr){
		printf("getvmmap:  can't find process for pid %d\n", pid);
		return(0);
	}

	taskptr = aproc.task;
	lseek(kmem, (long)taskptr, 0);
	if (read(kmem, (char *)&task, sizeof(task)) != sizeof(task)){
		printf("getvmmap:  can't read task at 0x%x\n", taskptr);
		return(0);
	}
	return(task.map);
}

/*VARARGS*/
iprintf(a, b, c, d, e, f, g, h, j, k, l, m)
	char *a;
{
	register int i;

	for (i = indent; i > 0;){
		if (i >= 8) {
			putchar('\t');
			i -= 8;
		}
		else {
			putchar(' ');
			i--;
		}
	}

	printf(a, b, c, d, e, f, g, h, j, k, l, m);
}

/*
 *	Routine:	vm_object_print
 */
void vm_object_print(kmem, objectptr)
	int		kmem;
	vm_object_t	objectptr;
{
	struct vm_object	object;
	register vm_page_t	p;
	struct vm_page		vmpage;

#if	DEBUG_LONGFORM
	if (objectptr)
		iprintf("Backing store object 0x%x.\n", objectptr);
	else {
		iprintf("No backing store object.\n");
		return;
	}

	lseek(kmem, (long)objectptr, 0);
	if (read(kmem, (char *)&object, sizeof(object)) != 
	    sizeof(object)){
		printf("vm_object_print: can't read object at 0x%x\n",
			objectptr);
		return;
	}

	indent += 2;
	iprintf("Size = 0x%x(%d).\n", object.size, object.size);
	iprintf("Ref count = %d.\n", object.ref_count);
	iprintf("Pager = 0x%x, pager_info = 0x%x.\n", object.pager,
		object.pager_info);
	iprintf("Memq = (next = 0x%x, prev = 0x%x).\n", object.memq.next,
			object.memq.prev);
	iprintf("Resident memory:\n");
	p = (vm_page_t) object.memq.next;
	while (!queue_end(&objectptr->memq, (queue_entry_t) p) {
		indent += 2;
		iprintf("Mem_entry 0x%x (phys = 0x%x).\n", p,
				VM_PAGE_TO_PHYS(p));

		lseek(kmem, (long)p, 0);
		if (read(kmem, (char *)&vmpage, sizeof(vmpage)) != 
		    sizeof(vmpage)){
			printf("vm_object_print: can't read page struct at 0x%x\n", p);
		break;
	}
		p = (vm_page_t) queue_next(&vmpage.listq);

		indent -= 2;
	}
	indent -= 2;
#else	DEBUG_LONGFORM

	register int count;

	if (objectptr == VM_OBJECT_NULL)
		return;

	lseek(kmem, (long)objectptr, 0);
	if (read(kmem, (char *)&object, sizeof(object)) != 
	    sizeof(object)){
		printf("vm_object_print: can't read object at 0x%x\n",
			objectptr);
		return;
	}

	indent += 2;

	iprintf("Object 0x%x: size=0x%x, resident=%d, ref=%d\n",
		(int) objectptr, (int) object.size, object.resident_page_count,
		object.ref_count);

	indent += 2;
	iprintf("pager=%d, paging offset=0x%x, shadow=(0x%x)+0x%x\n",
		(int) object.pager,
		(int) object.paging_offset,
		(int) object.shadow, (int) object.shadow_offset);

	indent += 2;

	count = 0;
	p = (vm_page_t) queue_first(&object.memq);
	while (!queue_end(&objectptr->memq, (queue_entry_t) p)) {
		if (count == 0) iprintf("memory:=");
		else if (count == 2) {printf("\n"); iprintf(" ..."); count = 0;}
		else printf(",");
		count++;

		lseek(kmem, (long)p, 0);
		if (read(kmem, (char *)&vmpage, sizeof(vmpage)) != 
		    sizeof(vmpage)){
			printf("vm_object_print: can't read page struct at 0x%x\n", p);
			break;
		}

		printf("(off=0x%x,page=0x%x)", vmpage.offset,
				vmpage.phys_addr);
		p = (vm_page_t) queue_next(&vmpage.listq);
	}
	if (count != 0)
		printf("\n");
	indent -= 2;

	vm_object_print(kmem, object.shadow);
	indent -= 4;
#endif	DEBUG_LONGFORM
	fflush(stdout);
}


/*
 *	Routine:	vm_map_print [debug]
 */
void vm_map_print(kmem, mapptr)
	int		kmem;
	vm_map_t	mapptr;
{
	struct vm_map_entry	entry, prev_entry;
	register vm_map_entry_t	entryptr;
	struct vm_map		map;

	lseek(kmem, (long)mapptr, 0);
	if (read(kmem, (char *)&map, sizeof(map)) != sizeof(map)){
		printf("vm_map_print:  can't read vm map at 0x%x!\n",
			mapptr);
		return;
	}

#if	DEBUG_LONGFORM
	iprintf("Address Map 0x%x. (%s map)\n", mapptr, map.is_main_map ? "TASK" : "SHARE");
	indent += 2;
	iprintf("Header = (head(next) = 0x%x, tail(prev) = 0x%x).\n",
			map.header.next, map.header.prev);
	iprintf("Physical map = 0x%x.\n", map.pmap);
	iprintf("Ref count = %d.\n", map.ref_count);
	iprintf("Nentries = %d.\n", map.nentries);
	iprintf("Dumping entries...\n");
	entryptr = map.header.next;		/* head */
	while (entryptr != &mapptr->header) {

		lseek(kmem, (long)entryptr, 0);
		if (read(kmem, (char *)&entry, sizeof(entry)) != 
		    sizeof(entry)){
			printf("vm_map_print:  can't read map entry at 0x%x\n", entryptr);
			break;
		}

		iprintf("Entry 0x%x.\n", entryptr);
		indent += 2;
		iprintf("(next = 0x%x, prev = 0x%x).\n",
				entry.next, entry.prev);
		iprintf("(start = 0x%x, end = 0x%x (len = 0x%x(%d))).\n",
				entry.start, entry.end,
				entry.end - entry.start,
				entry.end - entry.start);
		if (!map.is_main_map) {
			iprintf("(object = 0x%x, offset = 0x%x(%d)).\n",
				entry.object.vm_object, entry.offset, entry.offset);
			iprintf("copy_on_write = %s, needs_copy = %s.\n",
					entry.copy_on_write ? "TRUE" : "FALSE",
					entry.needs_copy ? "TRUE" : "FALSE");
			indent += 2;
			vm_object_print(kmem, entry.object.vm_object);
			indent -= 2;
		}
		else {
			iprintf("(share_map = 0x%x, offset = 0x%x(%d)).\n",
				entry.object.share_map, entry.offset,
				entry.offset);
			iprintf("(prot = %d, max_prot = %d).\n",
					entry.protection, entry.max_protection);
			iprintf("inheritance = %d.\n", entry.inheritance);
			indent += 2;
			vm_map_print(kmem, entry.object.share_map);
			indent -= 2;
		}
		entryptr = entry.next;
		indent -= 2;
	}
	indent -= 2;
#else	DEBUG_LONGFORM

	iprintf("%s map 0x%x: pmap=0x%x,ref=%d,nentries=%d\n",
		(map.is_main_map ? "Task" : "Share"),
 		(int) mapptr, (int) (map.pmap), map.ref_count, map.nentries);
	indent += 2;
	for (entryptr = map.header.next; entryptr != &mapptr->header;
	     entryptr = entry.next) {

		lseek(kmem, (long)entryptr, 0);
		if (read(kmem, (char *)&entry, sizeof(entry)) != 
		    sizeof(entry)){
			printf("vm_map_print:  can't read map entry at 0x%x\n", entryptr);
			break;
		}

		iprintf("map entry 0x%x: start=0x%x, end=0x%x, ",
			(int) entryptr, (int) entry.start, (int) entry.end);
		if (map.is_main_map) {
		     	static char *inheritance_name[4] =
				{ "share", "copy", "none", "donate_copy"};
			printf("prot=%x/%x/%s, ",
				entry.protection,
				entry.max_protection,
				inheritance_name[entry.inheritance]);
			if (entry.wired_count > 0)
				printf("wired, ");
		}

		if (entry.is_a_map) {
		 	printf("share=0x%x, offset=0x%x\n",
				(int) entry.object.share_map,
				(int) entry.offset);

			lseek(kmem, (long)entry.prev, 0);
			if (read(kmem, (char *)&prev_entry,
			   sizeof(prev_entry)) != sizeof(prev_entry)){
				printf("vm_map_print:  can't read previous map entry at 0x%x\n", entry.prev);
				continue;
			}

			if ((entry.prev == &mapptr->header) || (!prev_entry.is_a_map) ||
			    (prev_entry.object.share_map != entry.object.share_map)) {
				indent += 2;
				vm_map_print(kmem, entry.object.share_map);
				indent -= 2;
			}
				
		}
		else {
			printf("object=0x%x, offset=0x%x",
				(int) entry.object.vm_object,
				(int) entry.offset);
			if (entry.copy_on_write)
				printf(", copy (%s)", entry.needs_copy ? "needed" : "done");
			printf("\n");

			lseek(kmem, (long)entry.prev, 0);
			if (read(kmem, (char *)&prev_entry,
			   sizeof(prev_entry)) != sizeof(prev_entry)){
				printf("vm_map_print:  can't read previous map entry at 0x%x\n", entry.prev);
				continue;
			}

			if ((entry.prev == &mapptr->header) || (prev_entry.is_a_map) ||
			    (prev_entry.object.vm_object != entry.object.vm_object)) {
				indent += 2;
				vm_object_print(kmem, entry.object.vm_object);
				indent -= 2;
			}
		}
	}
	indent -= 2;
#endif	DEBUG_LONGFORM
	fflush(stdout);
}


main(argc, argv)
	int	argc;
	char	*argv[];
{
	int		initialized;
	int		kmem;
	char		*kmemfile;
	struct nlist	nl[NAMETBLSIZE];
	char		*nlistfile;
	int		pid;
	char		*s;
	char		underscore[80];
	char		*vmmapname;
	vm_map_t	vmmapptr;

	initialized = 0;
	kmemfile = KMEMFILE;
	nlistfile = NLISTFILE;

	if (argc == 1){	/* no arguments -  must need help */
		printf("Usage:  vmmprint [-a<hex. address>] [-n<map name>] [-p<pid>]\n");
		exit(1);
	}

	while (--argc > 0) {
		pid = NOPID;
		vmmapname = 0;
		vmmapptr = 0;

		/* parse command line */

		if ((*++argv)[0] == '-') {
			s = argv[0] + 1;
			switch (*s) {
			case 'a':
				if (*++s)
					sscanf(s, "%x", &vmmapptr);
				else if (--argc > 0)
					sscanf(*++argv, "%x", &vmmapptr);
				break;
			case 'n':
				if (*++s)
					vmmapname = s;
				else if (--argc > 0)
					vmmapname = *++argv;
				break;
			case 'p':
				if (*++s)
					pid = atoi(s);
				else if (--argc > 0)
					pid = atoi(*++argv);
				break;
			default:
				printf("vmmprint:  illegal option %c\n", *s);
				argc = 0;
				break;
			}
		}
		else if (((*argv)[0] >= 'a' && (*argv)[0] <= 'z') ||
				((*argv)[0] >= 'A' && (*argv)[0] <= 'Z')) {
			vmmapname = *argv;
			printf("Assuming map name specified as %s\n",
					vmmapname);

		}
		else if (((*argv)[0] >= '0' && (*argv)[0] <= '9')) {
			sscanf(*argv, "%x", &vmmapptr);
			if ((int)vmmapptr < 0)
				printf("Assuming map address specified as 0x%x\n",
						vmmapptr);
			else {
				vmmapptr = 0;
				pid = atoi(*argv);
				printf("Assuming pid specified as %d\n", pid);
			}
		}

		if (vmmapptr == 0 && vmmapname == 0 && pid == NOPID) {
			printf("Usage:  vmmprint [-a<hex. address>] [-n<map name>] [-p<pid>]\n");
			exit(1);
		}

		if (!initialized) {
			kmem = open(kmemfile, 0);  /* grab kernel memory */
			if (kmem < 0) {
				perror(kmemfile);
				exit(2);
			}
			initnlist(nl, names);	/* lookup kernel variables */
			nlist(nlistfile, nl);
			if (getkvars(kmem, nl) != 0)
				exit(3);
			initialized = 1;
		}

		if (vmmapptr) {
			printf("\nVirtual Memory Map at 0x%x:\n\n", vmmapptr);
			vm_map_print(kmem, vmmapptr);
		}
		else if (vmmapname) {
			printf("\nVirtual Memory Map named %s:\n\n",
					vmmapname);
			underscore[0] = '_';
			underscore[1] = '\0';
			names[0] = strcat(underscore, vmmapname);
			names[1] = "";
			initnlist(nl, names);
			nlist(nlistfile, nl);
			vmmapptr = 0;
			lseek(kmem, (long)(nl[0].n_value), 0);
			if (read(kmem, (char *)&vmmapptr, sizeof(vmmapptr))
					!= sizeof(vmmapptr))
				printf("vmmprint:  can't read %s at 0x%x\n",
							vmmapname, nl[0].n_value);
				else if (vmmapptr) {
					vm_map_print(kmem, vmmapptr);
				}
		}
		else if (pid != NOPID) {
			printf("\nVirtual Memory Map for pid %d:\n\n", pid);
			vmmapptr = getvmmap(kmem, nl, pid);
			if (vmmapptr)
				vm_map_print(kmem, vmmapptr);
		}
		printf("\n");
	}
}

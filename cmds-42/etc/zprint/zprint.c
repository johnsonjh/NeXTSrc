
/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * $Log:	zprint.c,v $
 * Revision 1.3  89/10/30  12:28:31  mrt
 * 	Changed include of mach/mach_debug.h to mach_debug.h
 * 	[89/10/30            mrt]
 * 
 * 	Rewrote to use host_zone_info instead of /dev/kmem.
 * 	[89/05/06            rpd]
 * 
 * Revision 1.2  89/05/05  18:27:04  mrt
 * 	Cleanup for Mach 2.5
 *
 * Condensed history:
 *	Hacked on by avie/mrt.
 *	Created by jjc.
 */
/*
 *	zprint.c
 *
 *	utility for printing out zone structures
 *
 *	With no arguments, prints information on all zone structures.
 *	With a "-n name" or "-nname" argument, prints information only
 *	on those zones for which the given name is a substring
 *	of the zone's name.
 */

#include <stdio.h>
#include <strings.h>
#include <mach.h>
#if	NeXT
#include <kern/mach_debug.h>
#else
#include <mach_debug.h>
#endif	NeXT
#include <mach_error.h>

#define streql(a, b)		(strcmp((a), (b)) == 0)
#define strneql(a, b, n)	(strncmp((a), (b), (n)) == 0)

static void printzone();
static int strnlen();
static boolean_t substr();

static char *program;

static void
usage()
{
	quit(1, "usage: %s [-n name]\n", program);
}

main(argc, argv)
	int	argc;
	char	*argv[];
{
	zone_name_t	*name;
	unsigned int	nameCnt;
	zone_info_t	*info;
	unsigned int	infoCnt;

	char		*zname = NULL;
	int		znamelen;
	int	currentBytes = 0;
	int totalAllocBytes = 0;

	kern_return_t	kr;
	int		i;

	program = rindex(argv[0], '/');
	if (program == NULL)
		program = argv[0];
	else
		program++;

	for (i = 1; i < argc; i++) {
		if (strneql(argv[i], "-n", 2)) {
			if (argv[i][2] == '\0') {
				if (i+1 < argc)
					zname = argv[++i];
				else
					usage();
			} else
				zname = &argv[i][2];
		} else if (streql(argv[i], "--")) {
			i++;
			break;
		} else if (argv[i][0] == '-')
			usage();
		else
			break;
	}

	if (argc != i)
		usage();

	if (zname == NULL)
		zname = "";
	znamelen = strlen(zname);

	kr = host_zone_info(task_self(), &name, &nameCnt, &info, &infoCnt);
	if (kr != KERN_SUCCESS)
		quit(1, "%s: host_zone_info: %s\n",
		     program, mach_error_string(kr));
	else if (nameCnt != infoCnt)
		quit(1, "%s: host_zone_info: counts not equal?\n", program);

	for (i = 0; i < nameCnt; i++)
		if (substr(zname, znamelen, name[i].name,
			   strnlen(name[i].name, sizeof name[i].name))) {
			printzone(&name[i], &info[i]);
			totalAllocBytes += info[i].cur_size;
			currentBytes += info[i].count * info[i].elem_size;
		}

	printf("\nTotal allocated space: %dK bytes\n", totalAllocBytes / 1024);
	printf("Space actually used: %dK bytes\n", currentBytes / 1024);

	kr = vm_deallocate(task_self(), (vm_address_t) name,
			   (vm_size_t) (nameCnt * sizeof *name));
	if (kr != KERN_SUCCESS)
		quit(1, "%s: vm_deallocate: %s\n",
		     program, mach_error_string(kr));

	kr = vm_deallocate(task_self(), (vm_address_t) info,
			   (vm_size_t) (infoCnt * sizeof *info));
	if (kr != KERN_SUCCESS)
		quit(1, "%s: vm_deallocate: %s\n",
		     program, mach_error_string(kr));

	exit(0);
}

static int
strnlen(s, n)
	char *s;
	int n;
{
	int len = 0;

	while ((len < n) && (*s++ != '\0'))
		len++;

	return len;
}

static boolean_t
substr(a, alen, b, blen)
	char *a;
	int alen;
	char *b;
	int blen;
{
	int i;

	for (i = 0; i <= blen - alen; i++)
		if (strneql(a, b+i, alen))
			return TRUE;

	return FALSE;
}

static void
printzone(name, info)
	zone_name_t *name;
	zone_info_t *info;
{
	printf("%.*s zone:\n", sizeof name->name, name->name);
	printf("\tcur_size:    %dK bytes (%d elements)\n",
	       info->cur_size/1024,
	       info->cur_size/info->elem_size);
	printf("\tmax_size:    %dK bytes (%d elements)\n",
	       info->max_size/1024,
	       info->max_size/info->elem_size);
	printf("\telem_size:   %d bytes\n",
	       info->elem_size);
	printf("\t# of elems:  %d\n",
	       info->count);
	printf("\talloc_size:  %dK bytes (%d elements)\n",
	       info->alloc_size/1024,
	       info->alloc_size/info->elem_size);
	if (info->pageable)
		printf("\tPAGEABLE\n");
}

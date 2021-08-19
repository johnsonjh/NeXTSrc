/*
 * NetInfo object caching layer definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
typedef
enum ni_op {
	NIOP_WRITE,
	NIOP_READ
} ni_op;

ni_status obj_init(char *, void **);
void obj_free(void *);
void obj_shutdown(void *, unsigned);
unsigned obj_getchecksum(void *);

char *obj_dirname(void *);

ni_status obj_alloc_root(void *, ni_object **);
ni_status obj_alloc(void *, ni_object **);
ni_status obj_regenerate(void *, ni_object **, ni_id *);

void obj_unalloc(void *, ni_object *);
void obj_uncache(void *, ni_object *);

ni_status obj_lookup(void *, ni_id *, ni_op, ni_object **);
ni_status obj_lookup_root(void *, ni_object **);

ni_status obj_commit(void *, ni_object *);
void obj_unlookup(void *, ni_object *);
ni_index obj_highestid(void *);
void obj_forget(void *);
void obj_renamedir(void *, char *);

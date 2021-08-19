/*
 * Directory Index defs
 * Copyright (C) 1989 by NeXT, Inc.
 */
typedef struct index_handle {
	void *private;
} index_handle;

index_handle index_alloc(void);
void index_insert(index_handle *, ni_name_const, ni_index);
void index_insert_list(index_handle *, ni_namelist, ni_index);
void index_delete(index_handle *, ni_name_const, ni_index);
ni_index index_lookup(index_handle, ni_name_const, ni_index **vals);
void index_unalloc(index_handle *);

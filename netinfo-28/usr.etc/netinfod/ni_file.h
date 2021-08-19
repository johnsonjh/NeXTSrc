/*
 * Low-level NetInfo file handling definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
ni_status file_init(char *, void **);
void file_renamedir(void *, char *);
char *file_dirname(void *);
void file_free(void *);
void file_shutdown(void *, unsigned);
ni_status file_rootid(void *, ni_id *);
ni_status file_idalloc(void *, ni_id *);
ni_status file_regenerate(void *, ni_id *);
ni_status file_idunalloc(void *, ni_id);
ni_status file_read(void *, ni_id *, ni_object **);
ni_status file_write(void *, ni_object *);

unsigned file_getchecksum(void *);

ni_status file_forcewrite(void *, ni_object *, ni_index);
ni_index file_highestid();
void file_sync(void *);

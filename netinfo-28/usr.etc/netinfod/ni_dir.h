/*
 * NetInfo directory (XXX.nidb) handling definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
void dir_cleanup(ni_name);
void dir_clonecheck(void);
ni_status dir_mastercreate(ni_name);
ni_status dir_clonecreate(ni_name, ni_name, ni_name, ni_name);
void dir_getnames(ni_name, ni_name *, ni_name *, ni_name *);


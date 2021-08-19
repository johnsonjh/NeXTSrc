/*
 * Mail alias lookup routines
 * Copyright (c) 1989 by NeXT, Inc.
 */
typedef struct aliasent {
	char *alias_name;
	unsigned alias_members_len;
	char **alias_members;
	int alias_local;
} aliasent;

void alias_setent(void);
aliasent *alias_getent(void);
void alias_endent(void);

aliasent *alias_getbyname(char *name);
	

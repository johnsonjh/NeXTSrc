/* 
 * Printer database lookup routines
 * Copyright (c) 1989 by NeXT, Inc. 
 */
typedef struct prdb_property {
	char *pp_key;
	char *pp_value;
} prdb_property;

typedef struct prdb_ent {
	char **pe_name;
	unsigned pe_nprops;
	prdb_property *pe_prop;
} prdb_ent;

void prdb_set(const char *);
const prdb_ent *prdb_get(void);
const prdb_ent *prdb_getbyname(const char *);
void prdb_end(void);


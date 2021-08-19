/*
 * Utility routines for NetInfo
 * Copyright (C) 1989 by NeXT, Inc.
 */
#define NI_INDEX_NULL ((ni_index)-1)
#define NI_INIT(objp) bzero((void *)(objp), sizeof(*(objp)))

ni_name ni_name_dup(ni_name_const);
void ni_name_free(ni_name *);
int ni_name_match(ni_name_const, ni_name_const);

ni_namelist ni_namelist_dup(const ni_namelist);
void ni_namelist_free(ni_namelist *);
void ni_namelist_insert(ni_namelist *, ni_name_const, ni_index);
void ni_namelist_delete(ni_namelist *, ni_index);
ni_index ni_namelist_match(const ni_namelist, ni_name_const);

ni_property ni_prop_dup(const ni_property);
void ni_prop_free(ni_property *);

void ni_proplist_insert(ni_proplist *, const ni_property, ni_index);
void ni_proplist_delete(ni_proplist *, ni_index);
ni_index ni_proplist_match(const ni_proplist, ni_name_const, ni_name_const);
ni_proplist ni_proplist_dup(const ni_proplist);
void ni_proplist_free(ni_proplist *);

void ni_proplist_list_free(ni_proplist_list *);

void ni_idlist_insert(ni_idlist *, ni_index, ni_index);
int ni_idlist_delete(ni_idlist *, ni_index);
ni_idlist ni_idlist_dup(const ni_idlist);
void ni_idlist_free(ni_idlist *);

void ni_entrylist_insert(ni_entrylist *, ni_entry);
void ni_entrylist_delete(ni_entrylist *, ni_index);
void ni_entrylist_free(ni_entrylist *);


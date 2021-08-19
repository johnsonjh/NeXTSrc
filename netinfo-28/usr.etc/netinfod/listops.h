void im_entrylist_unalloc(ni_entrylist *);
ni_entrylist im_entrylist_dup(ni_entrylist);
void list_insert(ni_entrylist *, ni_namelist, ni_index, int, ni_index);
void list_delete(ni_entrylist *, ni_index, int);
void list_delete_one(ni_entrylist *, ni_index, ni_index);
void list_insert_one(ni_entrylist *, ni_name_const, ni_index, ni_index);

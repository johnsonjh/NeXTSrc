/*
 * The number chosen here is not necessarily optimal.
 * It is only an educated guess.
 */
#define INDEX_THRESH 32

typedef struct im_handle {
	void *private;
} im_handle;

im_handle im_alloc(void);
void im_free(im_handle *);
void im_forget(im_handle *);

void im_newnode(im_handle *, ni_object *, ni_index);
void im_remove(im_handle *, ni_object *);

void im_create_all(im_handle *, ni_object *, ni_proplist);
void im_destroy_all(im_handle *, ni_object *, ni_proplist);
int im_has_indexed_dir(im_handle *, ni_index, ni_name_const, ni_name_const,
		       ni_index **, ni_index *);
void im_store_index(im_handle *, index_handle, ni_index, ni_name_const);
void im_destroy_list(im_handle *, ni_object *, ni_name_const, ni_namelist);
void im_create_list(im_handle *, ni_object *, ni_name_const, ni_namelist);
void im_destroy(im_handle *, ni_object *, ni_name_const, ni_name_const, ni_index);
void im_create(im_handle *, ni_object *, ni_name_const, ni_name_const, ni_index);

int im_has_saved_list(im_handle *, ni_index, ni_name_const, ni_entrylist *);
void im_store_list(im_handle *, ni_index, ni_name_const, ni_entrylist);

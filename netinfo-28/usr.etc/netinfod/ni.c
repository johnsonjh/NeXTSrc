/*
 * Server-side implementation of NetInfo interface
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <sys/types.h>
#include <stdio.h>
#include "ni_server.h"
#include "ni_impl.h"
#include "ni_globals.h"
#include "system.h"
#include "mm.h"
#include "clib.h"
#include "index.h"
#include "index_manager.h"

/*
 * What's inside the opaque handle we pass to clients of this layer
 */
typedef struct ni_handle {
	ni_entrylist nilistres;	/* result of ni_list (efficiency hack) */
	char *user;		/* username of caller */
	void *obj_hdl;		/* hook into next layer */
	im_handle im_hdl;	/* handle into index manager */
} ni_handle;

#define NH(ni) ((struct ni_handle *)ni)
#define OH(ni) NH(ni)->obj_hdl
#define IMH(ni) (&NH(ni)->im_hdl)

static ni_status ni_validate_dir(void *ni, ni_object *obj);
static ni_status ni_validate_name(void *ni, ni_object *obj, ni_index);

/*
 * Initilialize this layer
 */
ni_status
ni_init(
	char *rootdir,
	void **handle
	)
{
	ni_handle *ni;
	ni_status status;

	MM_ALLOC(ni);
	ni->nilistres.niel_val = NULL;
	ni->user = NULL;
	status = obj_init(rootdir, &ni->obj_hdl);
	if (status != NI_OK) {
		MM_FREE(ni);
	}
	ni->im_hdl = im_alloc();
	*handle = ni;
	return (status);
}

/*
 * Return the database tag
 */
char *
ni_tagname(
	   void *ni
	   )
{
	ni_name tag;
	ni_name dot;

	tag = ni_name_dup(obj_dirname(OH(ni)));
	dot = rindex(tag, '.');
	if (dot != NULL) {
		*dot = 0;
	}
	return (tag);
}

#ifdef NOTNEEDED
/*
 * Set the password (a NOP on the server)
 */
ni_status
ni_setpassword(
	       void *ni,
	       ni_name_const password
	       )
{
	return (NI_OK);
}
#endif

/*
 * Forget what we know (cache flushing)
 */
void
ni_forget(void *ni)
{
	obj_forget(OH(ni));
	im_forget(IMH(ni));
}

/*
 * Rename this database
 */
void
ni_renamedir(
	     void *ni, 
	     char *name
	     )
{
	obj_renamedir(OH(ni), name);
}

/*
 * Set the username of the caller
 */
ni_status
ni_setuser(
	   void *ni,
	   ni_name_const user
	   )
{
	if (NH(ni)->user != NULL) {
		ni_name_free(&NH(ni)->user);
	}
	if (user != NULL) {
		NH(ni)->user = ni_name_dup(user);
	} else {
		NH(ni)->user = NULL;
	}
	return (NI_OK);
}

/*
 * Free up allocated resources
 */
void
ni_free(
	void *ni
	)
{
	ni_list_const_free(ni);
	if (NH(ni)->user != NULL) {
		ni_name_free(&NH(ni)->user);
	}
	obj_free(OH(ni));
	im_free(IMH(ni));
	MM_FREE(NH(ni));
}

unsigned
ni_getchecksum(
	       void *ni
	       )
{
	return (obj_getchecksum(OH(ni)));
}

void
ni_shutdown(
	    void *ni,
	    unsigned checksum
	    )
{
	obj_shutdown(OH(ni), checksum);
}

ni_status
ni_root(
	void *handle,
	ni_id *id_p
	)
{
	ni_object *obj;
	ni_id root_id;
	ni_status status;

	status = obj_lookup_root(OH(handle), &obj);
	if (status != NI_OK) {
		/*
		 * Create root node
		 */
		status = obj_alloc_root(OH(handle), &obj);
		if (status != NI_OK) {
			return (status);
		}
		status = obj_commit(OH(handle), obj);
		if (status != NI_OK) {
			obj_uncache(OH(handle), obj);
			return (status);
		}
	}
	root_id = obj->nio_id;
	obj_unlookup(OH(handle), obj);
	*id_p = root_id;
	return (NI_OK);
}

ni_status
ni_create(
	  void *handle,
	  ni_id *parent_id, 
	  ni_proplist pl, 
	  ni_id *child_id_p,
	  ni_index where
	  )
{
	ni_object *child;
	ni_object *parent;
	ni_status status;

	status = obj_lookup(OH(handle), parent_id, NIOP_WRITE, &parent);
	if (status != NI_OK) {
		return (status);
	}
	status = ni_validate_dir(handle, parent);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), parent);
		return (status);
	}
	if (child_id_p->nii_object != NI_INDEX_NULL) {
		child_id_p->nii_instance--;
		status = obj_regenerate(OH(handle), &child, child_id_p);
	} else {
		status = obj_alloc(OH(handle), &child);
	}
	if (status != NI_OK) {
		obj_unlookup(OH(handle), parent);
		return (status);
	}
	child->nio_props = ni_proplist_dup(pl);
	child->nio_parent = parent_id->nii_object;
	status = obj_commit(OH(handle), child);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), parent);
		obj_unalloc(OH(handle), child);
		return (status);
	}
	*child_id_p = child->nio_id;

	/*
	 * Update parent
	 */
	ni_idlist_insert(&parent->nio_children, child_id_p->nii_object,
		      where);
	status = obj_commit(OH(handle), parent);
	if (status != NI_OK) {
		obj_unalloc(OH(handle), child);
		obj_uncache(OH(handle), parent);
		return (status);
	}
	im_newnode(IMH(handle), child, where);
	*parent_id = parent->nio_id;
	obj_unlookup(OH(handle), child);
	obj_unlookup(OH(handle), parent);

	return (NI_OK);
}

ni_status
ni_destroy(
	   void *handle,
	   ni_id *parent_id,
	   ni_id child_id
	   )
{
	ni_object *parent;
	ni_object *child;
	ni_status status;
	int badid = 0;

	status = obj_lookup(OH(handle), parent_id, NIOP_WRITE, &parent);
	if (status != NI_OK) {
		return (status);
	}
	status = ni_validate_dir(handle, parent);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), parent);
		return (status);
	}
	status = obj_lookup(OH(handle), &child_id, NIOP_WRITE, &child);
	if (status != NI_OK) {
		if (status != NI_BADID) {
			obj_unlookup(OH(handle), parent);
			return (status);
		} else {
			badid++;
		}

	}

	if (!badid) {
		if (child->nio_children.niil_len != 0) {
			obj_unlookup(OH(handle), child);
			obj_unlookup(OH(handle), parent);
			return (NI_NOTEMPTY);
		}
	}
	
	if (!ni_idlist_delete(&parent->nio_children, 
			      child_id.nii_object)) {
		obj_unlookup(OH(handle), parent);
		if (!badid) {
			obj_unlookup(OH(handle), child);
		}
		return (NI_UNRELATED);
	}
	    

	/*
	 * Commit changes
	 */
	status = obj_commit(OH(handle), parent);
	if (status != NI_OK) {
		obj_uncache(OH(handle), parent);
		if (!badid) {
			obj_unlookup(OH(handle), child);
		}
		return (status);
	}
	*parent_id = parent->nio_id;
	obj_unlookup(OH(handle), parent);
	if (!badid) {
		im_remove(IMH(handle), child);
		obj_unalloc(OH(handle), child);
	}
	return (NI_OK);
}

/*
 * Copy properties for read functions ONLY (ni_read, ni_lookupread).
 */
static inline void
copyprops(
	  ni_proplist *inprops,
	  ni_proplist *outprops
	  )
{
#if ENABLE_CACHE
	/*
	 * If caching, should just return read-only copy.
	 * Higher levels must be sure and NOT free the result.
	 */
	*outprops = *inprops;
#else
	/*
	 * Do not copy the data, only the pointers to it. 
	 * Zeroing out the props prevents obj_unlookup() from freeing these
	 * pointers.
	 */
	*outprops = *inprops;
	MM_ZERO(inprops);
#endif
}

ni_status
ni_read(
	void *handle,
	ni_id *id,
	ni_proplist *props
	)
{
	ni_status status;
	ni_object *obj;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	copyprops(&obj->nio_props, props);
	obj_unlookup(OH(handle), obj);
	return (status);
}

ni_status
ni_write(
	 void *handle,
	 ni_id *id,
	 ni_proplist props
	)
{
	ni_status status;
	ni_object *obj;
	ni_proplist saveprops;

	status = obj_lookup(OH(handle), id, NIOP_WRITE, &obj);
	if (status != NI_OK) {
		return (status);
	}
	status = ni_validate_dir(handle, obj);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), obj);
		return (status);
	}
	saveprops = obj->nio_props;
	obj->nio_props = ni_proplist_dup(props);
	status = obj_commit(OH(handle), obj);
	if (status != NI_OK) {
		ni_proplist_free(&saveprops);
		obj_uncache(OH(handle), obj);
		return (status);
	}

	im_destroy_all(IMH(handle), obj, saveprops);
	ni_proplist_free(&saveprops);
	im_create_all(IMH(handle), obj, obj->nio_props);

	*id = obj->nio_id;
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}


static ni_index
ni_prop_find(
	     ni_proplist props,
	     ni_name_const pname
	     )
{
	ni_index i;

	for (i = 0; i < props.nipl_len; i++) {
	  	if (ni_name_match(props.nipl_val[i].nip_name, pname)) {
			return (i);
		}
	}
	return (NI_INDEX_NULL);
}


ni_status
ni_lookup(
	  void *handle,
	  ni_id *id, 
	  ni_name_const pname, 
	  ni_name_const pval, 
	  ni_idlist *children_p
	  )
{
	ni_object *obj;
	ni_object *tmp;
	ni_index i;
	ni_status status;
	ni_id tmpid;
	ni_idlist res;
	ni_index ndirs;
	ni_index *dirs;
	ni_index which;
	index_handle index;
	int please_index;
	ni_index oid;
	ni_property *prop;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	if (im_has_indexed_dir(IMH(handle), id->nii_object, pname, pval, 
			       &dirs, &ndirs)) {
		res.niil_len = ndirs;
		MM_ALLOC_ARRAY(res.niil_val, res.niil_len);
		MM_BCOPY(dirs, res.niil_val, 
			 res.niil_len * sizeof(dirs[0]));
	} else {
		res.niil_len = 0;
		res.niil_val = NULL;
		if (obj->nio_children.niil_len >= INDEX_THRESH) {
			index = index_alloc();
			please_index = 1;
		} else {
			please_index = 0;
		}
		for (i = 0; i < obj->nio_children.niil_len; i++) {
			tmpid.nii_object = obj->nio_children.niil_val[i];
			status = obj_lookup(OH(handle), &tmpid, NIOP_READ, 
					    &tmp);
			if (status != NI_OK) {
				sys_errmsg("cannot lookup child");
				if (please_index) {
					index_unalloc(&index);
					please_index = 0;
				}
				continue;
			}
			which = ni_prop_find(tmp->nio_props, pname);
			if (which != NI_INDEX_NULL) {
				prop = &tmp->nio_props.nipl_val[which];
				oid = tmp->nio_id.nii_object;
				if (ni_namelist_match(prop->nip_val, pval) !=
				    NI_INDEX_NULL) {
					ni_idlist_insert(&res, oid,
							 NI_INDEX_NULL);
				}
				if (please_index) {
					index_insert_list(&index, 
							  prop->nip_val, oid);
					
				}
			}
			obj_unlookup(OH(handle), tmp);
		}
		if (please_index) {
			im_store_index(IMH(handle), index, 
				       obj->nio_id.nii_object, pname);
		}
	}
	obj_unlookup(OH(handle), obj);
	if (res.niil_len == 0) {
		return (NI_NODIR);
	}
	*children_p = res;
	return (NI_OK);
}

ni_status
ni_lookupread(
	      void *handle,
	      ni_id *id, 
	      ni_name_const pname, 
	      ni_name_const pval, 
	      ni_proplist *props
	  )
{
	ni_object *obj;
	ni_object *tmp;
	ni_index i;
	ni_status status;
	ni_id tmpid;
	int found;
	ni_index ndirs;
	ni_index *dirs;
	ni_index which;
	index_handle index;
	int please_index;
	ni_index oid;
	ni_property *prop;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	if (im_has_indexed_dir(IMH(handle), id->nii_object, pname, pval, 
			       &dirs, &ndirs)) {
		found = ndirs;
		if (found) {
			tmpid.nii_object = dirs[0];
			status = obj_lookup(OH(handle), &tmpid, NIOP_READ,
					    &tmp);
			if (status != NI_OK) {
				return (status);
			}
			copyprops(&tmp->nio_props, props);
			obj_unlookup(OH(handle), tmp);
		}
	} else {
		if (obj->nio_children.niil_len >= INDEX_THRESH) {
			index = index_alloc();
			please_index = 1;
		} else {
			please_index = 0;
		}
		found = 0;
		for (i = 0; 
		     (i < obj->nio_children.niil_len && 
		      (please_index || !found)); 
		     i++) {
			tmpid.nii_object = obj->nio_children.niil_val[i];
			status = obj_lookup(OH(handle), &tmpid, NIOP_READ, 
					    &tmp);
			if (status != NI_OK) {
				sys_errmsg("cannot lookup child");
				if (please_index) {
					index_unalloc(&index);
					please_index = 0;
				}
				continue;
			}
			which = ni_prop_find(tmp->nio_props, pname);
			if (which != NI_INDEX_NULL) {	
				prop = &tmp->nio_props.nipl_val[which];
				oid = tmp->nio_id.nii_object;
				if (ni_namelist_match(prop->nip_val, pval) !=
				    NI_INDEX_NULL) {
					if (!found) {
						copyprops(&tmp->nio_props,
							  props);
						found++;
					} 
				}
				if (please_index) {
					index_insert_list(&index, 
							  prop->nip_val, oid);
					
				}
			}
			obj_unlookup(OH(handle), tmp);
		}
		if (please_index) {
			im_store_index(IMH(handle), index, 
				       obj->nio_id.nii_object, pname);
		}
	}
	obj_unlookup(OH(handle), obj);
	return (found ? NI_OK : NI_NODIR);
}

void
ni_list_const_free(
		   void *handle
		   )
{
	if (NH(handle)->nilistres.niel_val != NULL) {
		ni_entrylist_free(&NH(handle)->nilistres);
		NH(handle)->nilistres.niel_val = NULL;
	}
}

/*
 * Like ni_list, but for efficiency, the returned entries are to be
 * considered "const" and NOT freed.
 */
ni_status
ni_list_const(
	      void *handle,
	      ni_id *id, 
	      ni_name_const pname, 
	      ni_entrylist *entries
	      )
{
	ni_object *obj;
	ni_object *tmp;
	ni_index i;
	ni_index j;
	ni_status status;
	ni_id tmpid;
	ni_proplist *pl;
	ni_namelist *nl;
	ni_entrylist res;
	int please_store;

	ni_list_const_free(handle);

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	if (!im_has_saved_list(IMH(handle), id->nii_object, pname, &res)) {
		please_store = (obj->nio_children.niil_len >= INDEX_THRESH);
		res.niel_len = obj->nio_children.niil_len;
		MM_ALLOC_ARRAY(res.niel_val, res.niel_len);
		MM_ZERO_ARRAY(res.niel_val, res.niel_len);
		for (i = 0; i < obj->nio_children.niil_len; i++) {
			tmpid.nii_object = obj->nio_children.niil_val[i];
			status = obj_lookup(OH(handle), &tmpid, NIOP_READ, 
					    &tmp);
			if (status != NI_OK) {
				sys_errmsg("cannot lookup child");
				continue;
			}
			res.niel_val[i].id = tmpid.nii_object;
			res.niel_val[i].names = NULL;
			pl = &tmp->nio_props;
			for (j = 0; j < pl->nipl_len; j++) {
				nl = &pl->nipl_val[j].nip_val;
				if (ni_name_match(pl->nipl_val[j].nip_name, 
						  pname) &&
				    nl->ninl_len > 0) {
					MM_ALLOC(res.niel_val[i].names);
					(*res.niel_val[i].names = 
					 ni_namelist_dup(*nl));
					break;
				}
			}
			obj_unlookup(OH(handle), tmp);
		}
		if (please_store) {
			im_store_list(IMH(handle), obj->nio_id.nii_object, 
				      pname, res);
		}
		NH(handle)->nilistres = res;
	}
	obj_unlookup(OH(handle), obj);
	*entries = res;
	return (NI_OK);
}

ni_status
ni_listall(
	   void *handle,
	   ni_id *id, 
	   ni_proplist_list *entries
	   )
{
	ni_object *obj;
	ni_object *tmp;
	ni_index i;
	ni_status status;
	ni_id tmpid;
	ni_proplist_list res;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	res.nipll_len = obj->nio_children.niil_len;
	MM_ALLOC_ARRAY(res.nipll_val, res.nipll_len);
	MM_ZERO_ARRAY(res.nipll_val, res.nipll_len);
	for (i = 0; i < obj->nio_children.niil_len; i++) {
		tmpid.nii_object = obj->nio_children.niil_val[i];
		status = obj_lookup(OH(handle), &tmpid, NIOP_READ, &tmp);
		if (status != NI_OK) {
			sys_errmsg("cannot lookup child");
			continue;
		}
		res.nipll_val[i] = ni_proplist_dup(tmp->nio_props);
		obj_unlookup(OH(handle), tmp);
	}
	obj_unlookup(OH(handle), obj);
	*entries = res;
	return (NI_OK);
}

ni_status
ni_children(
	    void *handle,
	    ni_id *id, 
	    ni_idlist *children_p
	    )
{
	ni_object *obj;
	ni_status status;
	
	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	*children_p = ni_idlist_dup(obj->nio_children);
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_parent(
	  void *handle,
	  ni_id *id, 
	  ni_index *parent_id_p
	  )
{
	ni_object *obj;
	ni_status status;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	*parent_id_p = obj->nio_parent;
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_self(
	void *handle,
	ni_id *id
	)
{
	ni_object *obj;
	ni_status status;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_readprop(
	    void *handle,
	    ni_id *id, 
	    ni_index prop_index,
	    ni_namelist *propval_p
	    )
{
	ni_status status;
	ni_object *obj;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	if (prop_index >= obj->nio_props.nipl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NOPROP);
	}
	*propval_p = ni_namelist_dup(obj->nio_props.nipl_val[prop_index].nip_val);
	obj_unlookup(OH(handle), obj);
	return (status);
}

ni_status
ni_writeprop(
	     void *handle,
	     ni_id *id, 
	     ni_index prop_index,
	     ni_namelist values
	     )
{
	ni_status status;
	ni_object *obj;
	ni_namelist savelist;

	status = obj_lookup(OH(handle), id, NIOP_WRITE, &obj);
	if (status != NI_OK) {
		return (status);
	}
	if (prop_index >= obj->nio_props.nipl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NOPROP);
	}
	status = ni_validate_name(handle, obj, prop_index);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), obj);
		return (status);
	}
	savelist = obj->nio_props.nipl_val[prop_index].nip_val;
	obj->nio_props.nipl_val[prop_index].nip_val = ni_namelist_dup(values);
	status = obj_commit(OH(handle), obj);
	if (status != NI_OK) {
		obj_uncache(OH(handle), obj);
		ni_namelist_free(&savelist);
		return (status);
	}

	im_destroy_list(IMH(handle), obj, 
			obj->nio_props.nipl_val[prop_index].nip_name,
			savelist);
	ni_namelist_free(&savelist);
	im_create_list(IMH(handle), obj,
		       obj->nio_props.nipl_val[prop_index].nip_name,
		       obj->nio_props.nipl_val[prop_index].nip_val);

	*id = obj->nio_id;
	obj_unlookup(OH(handle), obj);
	return (status);
}

ni_status
ni_createprop(
	      void *handle,
	      ni_id *id, 
	      ni_property prop,
	      ni_index where
	   )
{
	ni_status status;
	ni_object *obj;
	ni_property newprop;

	status = obj_lookup(OH(handle), id, NIOP_WRITE, &obj);
	if (status != NI_OK) {
		return (status);
	}

	status = ni_validate_dir(handle, obj);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), obj);
		return (status);
	}

	/*
	 * Add property 
	 */
	newprop = ni_prop_dup(prop);
	ni_proplist_insert(&obj->nio_props, newprop, where);
	
	status = obj_commit(OH(handle), obj);
	if (status != NI_OK) {
		obj_uncache(OH(handle), obj);
		return (status);
	}
	*id = obj->nio_id;
	im_create_list(IMH(handle), obj, prop.nip_name, prop.nip_val);
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_destroyprop(
	       void *handle,
	   ni_id *id, 
	   ni_index prop_index
	   )
{
	ni_status status;
	ni_object *obj;
	ni_property saveprop;

	status = obj_lookup(OH(handle), id, NIOP_WRITE, &obj);
	if (status != NI_OK) {
		return (status);
	}
	if (prop_index >= obj->nio_props.nipl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NOPROP);
	}
	status = ni_validate_dir(handle, obj);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), obj);
		return (status);
	}

	/*
	 * Save property, zero out old pointers so ni_proplist_delete()
	 * doesn't free them.
	 */
	saveprop = ni_prop_dup(obj->nio_props.nipl_val[prop_index]);
	MM_ZERO(&obj->nio_props.nipl_val[prop_index]);
	ni_proplist_delete(&obj->nio_props, prop_index);

	status = obj_commit(OH(handle), obj);
	if (status != NI_OK) {
		obj_uncache(OH(handle), obj);
		ni_prop_free(&saveprop);
		return (status);
	}
	*id = obj->nio_id;
	im_destroy_list(IMH(handle), obj, saveprop.nip_name, 
			saveprop.nip_val);
	ni_prop_free(&saveprop);
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_renameprop(
	      void *handle,
	      ni_id *id, 
	      ni_index prop_index,
	      ni_name_const name
	      )
{
	ni_status status;
	ni_object *obj;
	ni_name savename;

	status = obj_lookup(OH(handle), id, NIOP_WRITE, &obj);
	if (status != NI_OK) {
		return (status);
	}
	if (prop_index >= obj->nio_props.nipl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NOPROP);
	}
	status = ni_validate_dir(handle, obj);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), obj);
		return (status);
	}
	savename = obj->nio_props.nipl_val[prop_index].nip_name;
	obj->nio_props.nipl_val[prop_index].nip_name = ni_name_dup(name);
	status = obj_commit(OH(handle), obj);
	if (status != NI_OK) {
		ni_name_free(&savename);
		obj_uncache(OH(handle), obj);
		return (status);
	}

	im_destroy_list(IMH(handle), obj, savename, 
			obj->nio_props.nipl_val[prop_index].nip_val);
	ni_name_free(&savename);
	im_create_list(IMH(handle), obj, 
		       obj->nio_props.nipl_val[prop_index].nip_name,
		       obj->nio_props.nipl_val[prop_index].nip_val);
	
	*id = obj->nio_id;
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_listprops(
	     void *handle,
	      ni_id *id, 
	      ni_namelist *propnames
	      )
{
	ni_status status;
	ni_object *obj;
	ni_index i;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	propnames->ninl_len = 0;
	propnames->ninl_val = NULL;
	for (i = 0; i < obj->nio_props.nipl_len; i++) {
		ni_namelist_insert(propnames, 
				obj->nio_props.nipl_val[i].nip_name,
				NI_INDEX_NULL);
	}
	*id = obj->nio_id;
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_createname(
	      void *handle,
	      ni_id *id, 
	      ni_index prop_index,
	      ni_name_const name,
	      ni_index where
	      )
{
	ni_status status;
	ni_object *obj;

	status = obj_lookup(OH(handle), id, NIOP_WRITE, &obj);
	if (status != NI_OK) {
		return (status);
	}

	if (prop_index >= obj->nio_props.nipl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NOPROP);
	}
	status = ni_validate_name(handle, obj, prop_index);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), obj);
		return (status);
	}
	ni_namelist_insert(&obj->nio_props.nipl_val[prop_index].nip_val, 
			   name, where);
	
	status = obj_commit(OH(handle), obj);
	if (status != NI_OK) {
		obj_uncache(OH(handle), obj);
		return (status);
	}
	im_create(IMH(handle), obj, 
		  obj->nio_props.nipl_val[prop_index].nip_name,
		  name, where);
	*id = obj->nio_id;
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_destroyname(
	       void *handle,
	      ni_id *id, 
	      ni_index prop_index,
	      ni_index name_index
	      )
{
	ni_status status;
	ni_object *obj;
	ni_namelist *nl;
	ni_name savename;

	status = obj_lookup(OH(handle), id, NIOP_WRITE, &obj);
	if (status != NI_OK) {
		return (status);
	}

	if (prop_index >= obj->nio_props.nipl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NOPROP);
	}
	nl = &obj->nio_props.nipl_val[prop_index].nip_val;
	if (name_index >= nl->ninl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NONAME);
	}
	status = ni_validate_name(handle, obj, prop_index);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), obj);
		return (status);
	}

	/* 
	 * Copy name and set to NULL so ni_namelist_free() doesn't free it.
	 */
	savename = nl->ninl_val[name_index];
	nl->ninl_val[name_index] = NULL; 
	ni_namelist_delete(nl, name_index);
	
	status = obj_commit(OH(handle), obj);
	if (status != NI_OK) {
		obj_uncache(OH(handle), obj);
		ni_name_free(&savename);
		return (status);
	}
	*id = obj->nio_id;
	im_destroy(IMH(handle), obj, 
		   obj->nio_props.nipl_val[prop_index].nip_name,
		   savename, name_index);
	ni_name_free(&savename);
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_index
ni_highestid(
	     void *handle
	     )
{
	return (obj_highestid(OH(handle)));
}

ni_status
ni_writename(
	     void *handle,
	     ni_id *id, 
	     ni_index prop_index,
	     ni_index name_index,
	     ni_name_const name
	     )
{
	ni_status status;
	ni_object *obj;
	ni_namelist *nl;
	ni_name savename;

	status = obj_lookup(OH(handle), id, NIOP_WRITE, &obj);

	if (status != NI_OK) {
		return (status);
	}

	if (prop_index >= obj->nio_props.nipl_len) {
		return (NI_NOPROP);
	}
	nl = &obj->nio_props.nipl_val[prop_index].nip_val;
	if (name_index >= nl->ninl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NONAME);
	}
	status = ni_validate_name(handle, obj, prop_index);
	if (status != NI_OK) {
		obj_unlookup(OH(handle), obj);
		return (status);
	}
	savename = nl->ninl_val[name_index];
	nl->ninl_val[name_index] = ni_name_dup(name);
	
	status = obj_commit(OH(handle), obj);
	if (status != NI_OK) {
		obj_uncache(OH(handle), obj);
		ni_name_free(&savename);
		return (status);
	}

	im_destroy(IMH(handle), obj, 
		   obj->nio_props.nipl_val[prop_index].nip_name,
		   savename, name_index);
	ni_name_free(&savename);
	im_create(IMH(handle), obj, 
		  obj->nio_props.nipl_val[prop_index].nip_name,
		  name, name_index);

	*id = obj->nio_id;
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

ni_status
ni_readname(
	    void *handle,
	    ni_id *id, 
	     ni_index prop_index,
	     ni_index name_index,
	     ni_name *name
	     )
{
	ni_status status;
	ni_object *obj;
	ni_namelist *nl;

	status = obj_lookup(OH(handle), id, NIOP_READ, &obj);
	if (status != NI_OK) {
		return (status);
	}
	if (prop_index >= obj->nio_props.nipl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NOPROP);
	}
	nl = &obj->nio_props.nipl_val[prop_index].nip_val;
	if (name_index >= nl->ninl_len) {
		obj_unlookup(OH(handle), obj);
		return (NI_NONAME);
	}
	*name = ni_name_dup(nl->ninl_val[name_index]);
	obj_unlookup(OH(handle), obj);
	return (NI_OK);
}

static ni_status
ni_inaccesslist(
		void *ni,
		ni_namelist access_list
		)
{
	if (NH(ni)->user == NULL) {
		if (ni_namelist_match(access_list, 
				      ACCESS_USER_ANYBODY) == NI_INDEX_NULL) {
			return (NI_PERM);
		}
	} else {
		if (ni_namelist_match(access_list, 
				      NH(ni)->user) == NI_INDEX_NULL) {
			return (NI_PERM);
		}
	}
	return (NI_OK);
}

static ni_status 
ni_validate_dir(
		void *ni, 
		ni_object *obj
		)
{
	ni_proplist *pl;
	ni_index i;
	
	if (NH(ni)->user != NULL && ni_name_match(NH(ni)->user, 
						  ACCESS_USER_SUPER)) {
		return (NI_OK);
	}
	pl = &obj->nio_props;
	for (i = 0; i < pl->nipl_len; i++) {
		if (ni_name_match(pl->nipl_val[i].nip_name, ACCESS_DIR_KEY)) {
			return (ni_inaccesslist(ni,
						pl->nipl_val[i].nip_val));
		}
	}
	return (NI_PERM);
}

static ni_status 
ni_validate_name(
		 void *ni, 
		 ni_object *obj,
		 ni_index prop_index
		 )
{
	ni_proplist *pl;
	ni_name key;
	ni_name propkey;
	ni_index i;

	if (NH(ni)->user != NULL && ni_name_match(NH(ni)->user, 
						  ACCESS_USER_SUPER)) {
		return (NI_OK);
	}

	propkey = obj->nio_props.nipl_val[prop_index].nip_name;
	MM_ALLOC_ARRAY(key, (strlen(ACCESS_NAME_PREFIX) + 
			     strlen(propkey) + 1));
	sprintf(key, "%s%s", ACCESS_NAME_PREFIX, propkey);
	
	pl = &obj->nio_props;
	for (i = 0; i < pl->nipl_len; i++) {
		if (ni_name_match(pl->nipl_val[i].nip_name, key)) {
			ni_name_free(&key);
			return (ni_inaccesslist(ni,
						pl->nipl_val[i].nip_val));
		}
	}
	ni_name_free(&key);
	return (NI_PERM);
}

		 
